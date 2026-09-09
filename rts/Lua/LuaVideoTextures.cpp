/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LuaVideoTextures.h"

#include <charconv>
#include <atomic>

#include "Rendering/GlobalRendering.h"
#include "System/FileSystem/FileHandler.h"
#include "System/Log/ILog.h"

#include <tracy/Tracy.hpp>

namespace {
	std::atomic<std::uint64_t> nextVideoHandle = 1;
}

std::uint64_t LuaVideoTextures::GetHandle(const std::string& name) const
{
	if (name.size() < 2 || name[0] != prefix)
		return 0;

	std::uint64_t handle = 0;
	const char* begin = name.data() + 1;
	const char* end = name.data() + name.size();
	const auto result = std::from_chars(begin, end, handle);
	if (result.ec != std::errc() || result.ptr != end)
		return 0;
	return handle;
}

LuaVideoTextures::Texture* LuaVideoTextures::Get(std::uint64_t handle)
{
	const auto it = textures.find(handle);
	return (it == textures.end()) ? nullptr : &it->second;
}

const LuaVideoTextures::Texture* LuaVideoTextures::Get(std::uint64_t handle) const
{
	const auto it = textures.find(handle);
	return (it == textures.end()) ? nullptr : &it->second;
}

std::string LuaVideoTextures::Create(const std::string& path, const std::string& vfsModes, const video::VideoOptions& options, std::string& error)
{
	if (!CFileHandler::FileExists(path, vfsModes)) {
		error = "video file not found in permitted VFS modes: " + path;
		return {};
	}

	const std::uint64_t handle = nextVideoHandle.fetch_add(1);
	Texture texture;
	texture.player = std::make_unique<video::VideoPlayer>(path, vfsModes, options);
	const video::VideoInfo info = texture.player->GetInfo();
	if (info.state == video::PlaybackState::Error) {
		error = info.error;
		return {};
	}
	textures.emplace(handle, std::move(texture));
	return std::string(1, prefix) + std::to_string(handle);
}

bool LuaVideoTextures::Free(const std::string& name)
{
	const std::uint64_t handle = GetHandle(name);
	const auto it = textures.find(handle);
	if (it == textures.end())
		return false;

	// Stop and join the decoder before releasing its GL texture.
	it->second.player.reset();
	if (it->second.id != 0)
		glDeleteTextures(1, &it->second.id);
	textures.erase(it);
	return true;
}

void LuaVideoTextures::FreeAll()
{
	for (auto& [handle, texture]: textures) {
		texture.player.reset();
		if (texture.id != 0)
			glDeleteTextures(1, &texture.id);
	}
	textures.clear();
}

bool LuaVideoTextures::Play(const std::string& name)
{
	auto* texture = Get(GetHandle(name));
	if (texture == nullptr)
		return false;
	texture->player->Play();
	return true;
}

bool LuaVideoTextures::Pause(const std::string& name)
{
	auto* texture = Get(GetHandle(name));
	if (texture == nullptr)
		return false;
	texture->player->Pause();
	return true;
}

bool LuaVideoTextures::Stop(const std::string& name)
{
	auto* texture = Get(GetHandle(name));
	if (texture == nullptr)
		return false;
	texture->player->Stop();
	return true;
}

bool LuaVideoTextures::Seek(const std::string& name, double seconds)
{
	auto* texture = Get(GetHandle(name));
	if (texture == nullptr)
		return false;
	texture->player->Seek(seconds);
	return true;
}

bool LuaVideoTextures::SetVolume(const std::string& name, float volume)
{
	auto* texture = Get(GetHandle(name));
	if (texture == nullptr)
		return false;
	texture->player->SetVolume(volume);
	return true;
}

video::VideoInfo LuaVideoTextures::GetInfo(const std::string& name) const
{
	const auto* texture = Get(GetHandle(name));
	if (texture == nullptr) {
		video::VideoInfo info;
		info.state = video::PlaybackState::Error;
		info.error = "invalid or stale video texture handle";
		return info;
	}
	return texture->player->GetInfo();
}

bool LuaVideoTextures::Exists(const std::string& name) const
{
	return Get(GetHandle(name)) != nullptr;
}

	GLuint LuaVideoTextures::GetTextureID(std::uint64_t handle)
{
	ZoneNamedNC(tracyGetTexID, "VideoGetTextureID", tracy::Color::MediumAquamarine, true);
	auto* texture = Get(handle);
	if (texture == nullptr)
		return 0;

	const video::VideoInfo info = texture->player->GetInfo();
	GLint previousTexture = 0;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTexture);
	if (texture->id == 0 && info.width > 0 && info.height > 0) {
		ZoneNamedNC(tracyGLCreate, "VideoGLCreate", tracy::Color::Gold, true);
		texture->width = info.width;
		texture->height = info.height;
		glGenTextures(1, &texture->id);
		glBindTexture(GL_TEXTURE_2D, texture->id);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		const std::vector<std::uint8_t> black(static_cast<std::size_t>(info.width) * info.height * 4, 0);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, info.width, info.height, 0, GL_BGRA, GL_UNSIGNED_BYTE, black.data());
		glBindTexture(GL_TEXTURE_2D, previousTexture);
	}

	if (texture->id == 0 || texture->lastUploadFrame == globalRendering->drawFrame)
		return texture->id;
	texture->lastUploadFrame = globalRendering->drawFrame;

	video::VideoFrame frame;
	if (!texture->player->TakeDueFrame(frame))
		return texture->id;
	if (frame.width != texture->width || frame.height != texture->height) {
		LOG_L(L_ERROR, "[LuaVideoTextures] rejected a frame with changed dimensions");
		return texture->id;
	}

	ZoneNamedNC(tracyGLUpload, "VideoGLUpload", tracy::Color::DarkTurquoise, true);
	glBindTexture(GL_TEXTURE_2D, texture->id);
	if (PBO::IsSupported(GL_PIXEL_UNPACK_BUFFER)) {
		GLint previousPBO = 0;
		glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &previousPBO);
		auto& uploadBufferPtr = texture->uploadBuffers[texture->nextUploadBuffer++ % texture->uploadBuffers.size()];
		if (uploadBufferPtr == nullptr)
			uploadBufferPtr = std::make_unique<PBO>();
		PBO& uploadBuffer = *uploadBufferPtr;
		uploadBuffer.Bind();
		uploadBuffer.New(frame.bgra.size(), GL_STREAM_DRAW, frame.bgra.data());
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, frame.width, frame.height, GL_BGRA, GL_UNSIGNED_BYTE, uploadBuffer.GetPtr());
		uploadBuffer.Unbind();
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, previousPBO);
	} else {
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, frame.width, frame.height, GL_BGRA, GL_UNSIGNED_BYTE, frame.bgra.data());
	}
	glBindTexture(GL_TEXTURE_2D, previousTexture);
	return texture->id;
}

std::tuple<int, int, int> LuaVideoTextures::GetSize(std::uint64_t handle) const
{
	const auto* texture = Get(handle);
	if (texture == nullptr)
		return {0, 0, 0};
	const video::VideoInfo info = texture->player->GetInfo();
	return {info.width, info.height, 0};
}
