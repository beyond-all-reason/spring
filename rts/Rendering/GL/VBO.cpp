/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/**
 * @brief ARB_vertex_buffer_object implementation
 * ARB_vertex_buffer_object class implementation
 */

#include <cassert>
#include <vector>
#include <stdint.h>

#include "VBO.h"

#include "Rendering/GlobalRendering.h"
#include "System/Config/ConfigHandler.h"
#include "System/Log/ILog.h"
#include "System/SpringMath.h"

#include "System/Misc/TracyDefs.h"

//CONFIG(bool, UseVBO).defaultValue(true).safemodeValue(false);
CONFIG(bool, UseVBO).deprecated(true);
CONFIG(bool, UsePBO).deprecated(true);


/**
 * Returns if the current gpu drivers support object's buffer type
 */
bool VBO::IsSupported() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(globalRendering->sdlInitVideo); // called before SDL2 was initialized, probably in static member constructor
	return VBO::IsSupported(curBoundTarget);
}

/**
 * Returns if the current gpu drivers support certain buffer type
 */
bool VBO::IsSupported(GLenum target) {
	if (!GLAD_GL_ARB_map_buffer_range) //TODO glBufferSubData() fallback ?
		return false;

	switch (target) {
		case GL_PIXEL_PACK_BUFFER:
		case GL_PIXEL_UNPACK_BUFFER:
			return GLAD_GL_EXT_pixel_buffer_object;
		case GL_ARRAY_BUFFER:
		case GL_ELEMENT_ARRAY_BUFFER:
			return GLAD_GL_ARB_vertex_buffer_object;
		case GL_UNIFORM_BUFFER:
			return GLAD_GL_ARB_uniform_buffer_object;
		case GL_SHADER_STORAGE_BUFFER:
			return GLAD_GL_ARB_shader_storage_buffer_object;
		case GL_COPY_WRITE_BUFFER:
		case GL_COPY_READ_BUFFER:
			return GLAD_GL_ARB_copy_buffer;
		case GL_DISPATCH_INDIRECT_BUFFER:
			return GLAD_GL_ARB_compute_shader;
		default: {
			LOG_L(L_ERROR, "[VBO:%s]: wrong target [%u] is specified", __func__, target);
			return false;
		}
	}
}


VBO::VBO(GLenum _defTarget, const bool storage, bool readable)
{
	RECOIL_DETAILED_TRACY_ZONE;
	curBoundTarget = _defTarget;

	isSupported = IsSupported();
	immutableStorage = storage;
	readableStorage = readable;

	if (immutableStorage && !GLAD_GL_ARB_buffer_storage) {
		//note: We can't fallback to traditional BufferObjects, cause then we would have to map/unmap on each change.
		//      Only sysram/cpu VAs give an equivalent behaviour.
		isSupported = false;
		immutableStorage = false;
		LOG_L(L_ERROR, "VBO: cannot create immutable storage, gpu drivers missing support for it!");
	}
}


VBO::~VBO()
{
	RECOIL_DETAILED_TRACY_ZONE;
	Release();
}


VBO& VBO::operator=(VBO&& other) noexcept
{
	RECOIL_DETAILED_TRACY_ZONE;
	std::swap(vboId, other.vboId);
	std::swap(bound, other.bound);
	std::swap(mapped, other.mapped);
	std::swap(nullSizeMapped, other.nullSizeMapped);

	std::swap(bufSize, other.bufSize);
	std::swap(memSize, other.memSize);

	std::swap(curBoundTarget, other.curBoundTarget);
	std::swap(usage, other.usage);
	std::swap(mapUnsyncedBit, other.mapUnsyncedBit);

	std::swap(isSupported, other.isSupported);
	std::swap(immutableStorage, other.immutableStorage);
	std::swap(readableStorage, other.readableStorage);

	std::swap(data, other.data);
	std::swap(bbrItems, other.bbrItems);
	return *this;
}


void VBO::Generate() const { glGenBuffers(1, &vboId); }
void VBO::Delete() {
	// clear bound BBRs
	for (const auto& kv : bbrItems) {
		glBindBufferRange(kv.first.target, kv.first.index, 0u, kv.second.offset, kv.second.size);
	}
	bbrItems.clear();

	if (GLAD_GL_ARB_vertex_buffer_object)
		glDeleteBuffers(1, &vboId);

	vboId = 0;

	delete[] data;
	data = nullptr;
}

void VBO::Bind(GLenum target) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(!bound);

	if (isSupported)
		glBindBuffer(curBoundTarget = target, GetId());

	bound = true;
}


void VBO::Unbind() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(bound);

	if (isSupported)
		glBindBuffer(curBoundTarget, 0);

	bound = false;
}

bool VBO::BindBufferRangeImpl(GLenum target, GLuint index, GLuint _vboId, GLuint offset, GLsizeiptr size) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(offset + size <= bufSize);

	if (!isSupported)
		return false;

	if (target != GL_UNIFORM_BUFFER && target != GL_SHADER_STORAGE_BUFFER) { //assert(?)
		LOG_L(L_ERROR, "[VBO::%s]: attempt to bind wrong target [%u]", __func__, target);
		return false;
	}

	if (target == GL_UNIFORM_BUFFER && index >= globalRendering->glslMaxUniformBufferBindings) {
		LOG_L(L_ERROR, "[VBO::%s]: attempt to bind UBO with invalid index [%u]", __func__, index);
		return false;
	}

	if (target == GL_SHADER_STORAGE_BUFFER && index >= globalRendering->glslMaxStorageBufferBindings) {
		LOG_L(L_ERROR, "[VBO::%s]: attempt to bind SSBO with invalid index [%u]", __func__, index);
		return false;
	}

	const size_t neededAlignment = GetOffsetAlignment(target);
	if (offset % neededAlignment != 0 || size % neededAlignment != 0) { //assert(?)
		LOG_L(L_ERROR, "[VBO::%s]: attempt to bind with wrong offset [%u] or size [%d]. Needed alignment [%u]", __func__, static_cast<uint32_t>(offset), static_cast<int32_t>(size), static_cast<uint32_t>(neededAlignment));
		return false;
	}

	glBindBufferRange(target, index, _vboId, offset, size);

	BoundBufferRangeIndex bbri = {target, index};
	BoundBufferRangeData  bbrd = {offset, size };
	if (_vboId != 0) {
		bbrItems[bbri] = bbrd;
	} else {
		if (bbrItems[bbri] == bbrd) { //exact match of unbind call
			bbrItems.erase(bbri);
		}
	}

	return true;
}

void VBO::Resize(GLsizeiptr newSize, GLenum newUsage)
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(bound);
	assert(!mapped);

	// no size change -> nothing to do
	if (newSize == bufSize && newUsage == usage)
		return;

	newSize = GetAlignedSize(newSize);

	// first call: no *BO exists yet to copy old data from, so use ::New() (faster)
	if (bufSize == 0)
		return New(newSize, newUsage, nullptr);

	assert(newSize >= bufSize);

	const size_t oldSize = bufSize;
	bufSize = newSize;
	usage = newUsage;

	if (isSupported) {
		glClearErrors("VBO", __func__, globalRendering->glDebugErrors);
		const GLenum oldBoundTarget = curBoundTarget;
		GLint rbglsize = 0;
		GLint wbglsize = 0;
		glGetBufferParameteriv(curBoundTarget, GL_BUFFER_SIZE, &rbglsize);

		if (GLAD_GL_ARB_copy_buffer) {
			VBO vbo(GL_COPY_WRITE_BUFFER, immutableStorage);

			vbo.Bind(GL_COPY_WRITE_BUFFER);
			vbo.New(bufSize, usage);

			glGetBufferParameteriv(GL_COPY_WRITE_BUFFER, GL_BUFFER_SIZE, &wbglsize);
			// gpu internal copy (fast)
			if (oldSize > 0)
				glCopyBufferSubData(curBoundTarget, GL_COPY_WRITE_BUFFER, 0, 0, oldSize);

			vbo.Unbind();
			Unbind();
			*this = std::move(vbo);
			Bind(oldBoundTarget);
		} else {
			void* memsrc = MapBuffer(GL_READ_ONLY);
			Unbind();

			VBO vbo(oldBoundTarget, immutableStorage);
			vbo.Bind(oldBoundTarget);
			vbo.New(bufSize, GL_STREAM_DRAW);

			glGetBufferParameteriv(oldBoundTarget, GL_BUFFER_SIZE, &wbglsize);

			void* memdst = vbo.MapBuffer(GL_WRITE_ONLY);

			// cpu download & copy (slow)
			memcpy(memdst, memsrc, oldSize);
			vbo.UnmapBuffer(); vbo.Unbind();
			Bind(); UnmapBuffer(); Unbind();

			*this = std::move(vbo);
			Bind(oldBoundTarget);
		}

		const GLenum err = glGetError();
		if (err != GL_NO_ERROR) {
			LOG_L(L_ERROR, "[VBO::%s(rbsize=%u,wbsize=%u,rbglsize=%u,wbglsize=%u,usage=%u)] id=%u tgt=0x%x err=0x%x",
				__func__,
				static_cast<uint32_t>(oldSize),
				static_cast<uint32_t>(bufSize),
				static_cast<uint32_t>(rbglsize),
				static_cast<uint32_t>(wbglsize),
				usage,
				vboId,
				curBoundTarget,
				err
			);
			Unbind();

			// disable VBO and fallback to VA/sysmem
			isSupported = false;
			immutableStorage = false;

			// FIXME: copy old vbo data to sysram
			Bind();
			Resize(newSize, usage);
		}

		return;
	}


	if (bufSize > memSize) {
		// grow the buffer if needed, only bufSize is adjusted when shrinking it
		GLubyte* newData = new GLubyte[memSize = bufSize];
		assert(newData != nullptr);

		if (data != nullptr) {
			memcpy(newData, data, oldSize);
			delete[] data;
		}

		data = newData;
	}
}


void VBO::New(GLsizeiptr newSize, GLenum newUsage, const void* newData)
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(bound);
	assert(!mapped || (newData == nullptr && newSize == bufSize && newUsage == usage));

	newSize = GetAlignedSize(newSize);

	// ATI interprets unsynchronized access differently; (un)mapping does not sync
	mapUnsyncedBit = GL_MAP_UNSYNCHRONIZED_BIT * (1 - globalRendering->haveAMD);

	// no-op new, allows e.g. repeated Bind+New with persistent buffers
	if (newData == nullptr && newSize == bufSize && newUsage == usage)
		return;

	if (immutableStorage && bufSize != 0) {
		LOG_L(L_ERROR, "[VBO::%s({cur,new}size={" _STPF_ "," _STPF_ "},{cur,new}usage={0x%x,0x%x},data=%p)] cannot recreate persistent storage buffer", __func__, bufSize, newSize, usage, newUsage, data);
		return;
	}

	if (isSupported) {
		glClearErrors("VBO", __func__, globalRendering->glDebugErrors);

		if (immutableStorage) {
			glBufferStorage(curBoundTarget, newSize, newData, /*newUsage =*/(GL_MAP_READ_BIT * readableStorage) | GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT | GL_DYNAMIC_STORAGE_BIT);
		} else {
			glBufferData(curBoundTarget, newSize, newData, newUsage);
		}

		const GLenum err = glGetError();
		if (err != GL_NO_ERROR) {
			LOG_L(L_ERROR, "[VBO::%s(size=%lu,usage=0x%x,data=%p)] id=%u tgt=0x%x err=0x%x", __func__, (unsigned long) bufSize, usage, data, vboId, curBoundTarget, err);
			Unbind();

			// disable VBO and fallback to VA/sysmem
			isSupported = false;
			immutableStorage = false;

			Bind(curBoundTarget);
			New(newSize, newUsage, newData);
		}

		bufSize = newSize;
		usage = newUsage;
		return;
	}


	usage = newUsage;
	bufSize = newSize;

	if (newSize > memSize) {
		delete[] data;

		// prevent a dead-pointer in case of an OOM exception on the next line
		data = nullptr;
		data = new GLubyte[memSize = bufSize];

		if (newData == nullptr)
			return;

		assert(data != nullptr);
		memcpy(data, newData, newSize);
	} else {
		// keep the larger buffer; reduces fragmentation from repeated New's
		memset(data, 0, memSize);

		if (newData == nullptr)
			return;

		memcpy(data, newData, newSize);
	}
}

bool VBO::ReallocToFit(GLsizeiptr newSize, uint32_t sizeUpMult, uint32_t sizeDownMult, const void* newData)
{
	assert(bound);

	if (newSize > GetSize()) // increase buffer size
		New(newSize * sizeUpMult, usage, nullptr);
	else if (GetSize() >= newSize * sizeDownMult) // decrease buffer size
		New(newSize             , usage, nullptr);
	else
		return false;

	if (newData)
		SetBufferSubData(0, newSize, newData);

	return true;
}


GLubyte* VBO::MapBuffer(GLintptr offset, GLsizeiptr size, GLbitfield access)
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(!mapped);
	assert(offset + size <= bufSize);
	mapped = true;

	// glMapBuffer & glMapBufferRange use different flags for their access argument
	// for easier handling convert the glMapBuffer ones here
	switch (access) {
	case GL_WRITE_ONLY: {
			// https://computergraphics.stackexchange.com/questions/7586/what-are-the-performance-implications-of-the-optional-flags-used-when-mapping-a/7587
			// Also, you can map a buffer and only overwrite part of it. Invalidation is negatively useful for that too.
			const GLbitfield irBit = GL_MAP_INVALIDATE_RANGE_BIT * (offset == 0 && size == bufSize);
			access = GL_MAP_WRITE_BIT | irBit | mapUnsyncedBit;
			if (immutableStorage)
				access = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
			} break;
		case GL_READ_WRITE:
			access = GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;
			break;
		case GL_READ_ONLY:
			access = GL_MAP_READ_BIT;
			break;
		default: break;
	}

	if (size == 0) {
		// nvidia incorrectly returns GL_INVALID_VALUE when trying to call glMapBufferRange with size zero
		// so catch it ourselves
		nullSizeMapped = true;
		return nullptr;
	}

	if (isSupported) {
		GLubyte* ptr = (GLubyte*)glMapBufferRange(curBoundTarget, offset, size, access);
		assert(ptr);
		return ptr;
	}
	assert(data);
	return data + offset;
}


void VBO::UnmapBuffer()
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(mapped);

	if (nullSizeMapped)
		nullSizeMapped = false;
	else if (isSupported)
		glUnmapBuffer(curBoundTarget);

	mapped = false;
}

void VBO::SetBufferSubData(GLintptr offset, GLsizeiptr size, const void* data)
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(!mapped);
	assert((offset + size) <= bufSize);
	glBufferSubData(curBoundTarget, offset, size, data);
}


void VBO::Invalidate() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(bound);
	assert(!immutableStorage);
	assert(!mapped);

	// OpenGL4 way
	if (isSupported && GLAD_GL_ARB_invalidate_subdata) {
		glInvalidateBufferData(GetId());
		return;
	}

	// note: allocating memory doesn't actually block the memory it just makes room in _virtual_ memory space
	glBufferData(curBoundTarget, GetAlignedSize(bufSize), nullptr, usage);
}


const GLvoid* VBO::GetPtr(GLintptr offset) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(bound);

	if (isSupported)
		return (GLvoid*)((char*)nullptr + (offset));

	if (data == nullptr)
		return nullptr;

	return (GLvoid*)(data + offset);
}

size_t VBO::GetAlignedSize(GLenum target, size_t sz)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const size_t alignmentReq = GetOffsetAlignment(target);
	if (alignmentReq > 1)
		return AlignUp(sz, alignmentReq);

	return sz;
}

size_t VBO::GetOffsetAlignment(GLenum target) {
	RECOIL_DETAILED_TRACY_ZONE;

	const auto getOffsetAlignmentUBO = []() -> size_t {
		GLint buffAlignment = 0;
		glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &buffAlignment);
		return static_cast<size_t>(buffAlignment);
	};

	const auto getOffsetAlignmentSSBO = []() -> size_t {
		GLint buffAlignment = 0;
		glGetIntegerv(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT, &buffAlignment);
		return static_cast<size_t>(buffAlignment);
	};

	static size_t offsetAlignmentUBO  = getOffsetAlignmentUBO();
	static size_t offsetAlignmentSSBO = getOffsetAlignmentSSBO();

	switch (target) {
	case GL_UNIFORM_BUFFER:
		return offsetAlignmentUBO;
	case GL_SHADER_STORAGE_BUFFER:
		return offsetAlignmentSSBO;
	case GL_DISPATCH_INDIRECT_BUFFER:
		return 4;
	case GL_PIXEL_PACK_BUFFER:
	case GL_PIXEL_UNPACK_BUFFER:
	case GL_ARRAY_BUFFER:
	case GL_ELEMENT_ARRAY_BUFFER:
	default:
		return 1;
	}
}

