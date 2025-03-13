#include "ModelsDataUploader.h"

#include <limits>
#include <cassert>

#include "System/float4.h"
#include "System/Matrix44f.h"
#include "System/Log/ILog.h"
#include "System/SpringMath.h"
#include "System/TimeProfiler.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Objects/SolidObject.h"
#include "Sim/Projectiles/Projectile.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Features/FeatureDef.h"
#include "Sim/Features/FeatureDefHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Rendering/GL/VBO.h"
#include "Rendering/Models/3DModel.h"
#include "Rendering/Models/ModelsMemStorage.h"
#include "Rendering/Models/ModelsLock.h"
#include "Rendering/Units/UnitDrawer.h"
#include "Rendering/Features/FeatureDrawer.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Game/GlobalUnsynced.h"


////////////////////////////////////////////////////////////////////

namespace Impl {
	static constexpr uint32_t PERSISTENT_MAPPING_BUFFERING = 3u;

	template<
		typename DataType,
		typename SSBO,
		typename Uploader,
		typename MemStorage
	>
	void UpdateCommon(Uploader& uploader, std::unique_ptr<SSBO>& ssbo, MemStorage& memStorage, const char* className, const char* funcName)
	{
		auto& ul = memStorage.GetUpdateList();

		//resize handling
		const uint32_t elemCount = uploader.GetElemsCount();
		const uint32_t storageElemCount = memStorage.GetSize();
		if (storageElemCount > elemCount) {
			ssbo->UnbindBufferRange(uploader.GetBindingIdx());

			const uint32_t newElemCount = AlignUp(storageElemCount, uploader.GetElemCountIncr());
			LOG_L(L_DEBUG, "[%s::%s] sizing SSBO %s. New elements count = %u, elemCount = %u, storageElemCount = %u", className, funcName, "up", newElemCount, elemCount, storageElemCount);
			ssbo->Resize(newElemCount);

			if (!ssbo->IsValid()) {
				LOG_L(L_ERROR, "[%s::%s] Resizing of IStreamBuffer<%s> of type %d, failed. Falling back to SB_BUFFERSUBDATA.", className, __func__, spring::TypeToCStr<DataType>(), static_cast<int>(ssbo->GetBufferImplementation()));
				ssbo = nullptr;

				IStreamBufferConcept::StreamBufferCreationParams p;
				p.target = GL_SHADER_STORAGE_BUFFER;
				p.numElems = newElemCount;
				p.name = std::string(className);
				p.type = IStreamBufferConcept::Types::SB_BUFFERSUBDATA;
				p.resizeAble = true;
				p.coherent = false;
				p.numBuffers = 1;
				p.optimizeForStreaming = true;

				// must match p.numBuffers
				ul.SetTrueValue(1);

				ssbo = std::move(IStreamBuffer<DataType>::CreateInstance(p));
			}

			// ssbo->Resize() doesn't copy the data, force the update
			ul.SetNeedUpdateAll();
		}

		if (!ul.NeedUpdate())
			return;

		// may have been already unbound above, not a big deal
		ssbo->UnbindBufferRange(uploader.GetBindingIdx());

		// get the data
		const auto* clientPtr = memStorage.GetData().data();

		// iterate over contiguous regions of values that need update on the GPU
		for (auto itPair = ul.GetNext(); itPair.has_value(); itPair = ul.GetNext(itPair)) {
			auto [idxOffset, idxSize] = ul.GetOffsetAndSize(itPair.value());

			auto* mappedPtr = ssbo->Map(clientPtr, idxOffset, idxSize);

			if (!ssbo->HasClientPtr())
				std::copy(clientPtr + idxOffset, clientPtr + idxOffset + idxSize, mappedPtr);

			ul.DecrementUpdate(itPair.value());

			ssbo->Unmap();
		}

		ssbo->BindBufferRange(uploader.GetBindingIdx());
		ssbo->SwapBuffer();

		ul.CalcNeedUpdateAll();
	}

	template <
		typename DataType,
		typename SSBO,
		typename MemStorage
	>
	void InitCommon(std::unique_ptr<SSBO>& ssbo, MemStorage& memStorage, uint32_t bindingIdx, uint32_t elemCount0, uint32_t elemCountIncr, IStreamBufferConcept::Types type, bool coherent, uint32_t numBuffers, const char* className)
	{
		if (!globalRendering->haveGL4)
			return;

		assert(bindingIdx < -1u);

		IStreamBufferConcept::StreamBufferCreationParams p;
		p.target = GL_SHADER_STORAGE_BUFFER;
		p.numElems = elemCount0;
		p.name = std::string(className);
		p.type = type;
		p.resizeAble = true;
		p.coherent = coherent;
		p.numBuffers = numBuffers;
		p.optimizeForStreaming = true;

		auto& ul = memStorage.GetUpdateList();

		// must match p.numBuffers
		ul.SetTrueValue(static_cast<uint8_t>(numBuffers));

		ssbo = std::move(IStreamBuffer<DataType>::CreateInstance(p));
		if (!ssbo->IsValid()) {
			LOG_L(L_ERROR, "[%s::%s] Initialization of IStreamBuffer<%s> of type %d, failed. Falling back to SB_BUFFERSUBDATA.", className, __func__, spring::TypeToCStr<DataType>(), static_cast<int>(type));
			ssbo = nullptr;

			p.type = IStreamBufferConcept::Types::SB_BUFFERSUBDATA;
			p.coherent = false;
			p.numBuffers = 1;

			// must match p.numBuffers
			ul.SetTrueValue(1);

			ssbo = std::move(IStreamBuffer<DataType>::CreateInstance(p));
		}

		ssbo->BindBufferRange(bindingIdx);
	}

	template<typename SSBO>
	void KillCommon(std::unique_ptr<SSBO>& ssbo, uint32_t bindingIdx)
	{
		if (!globalRendering->haveGL4)
			return;

		ssbo->UnbindBufferRange(bindingIdx);
		ssbo = nullptr;
	}
}

////////////////////////////////////////////////////////////////////

TransformsUploader transformsUploader;
ModelUniformsUploader modelUniformsUploader;

////////////////////////////////////////////////////////////////////

void TransformsUploader::Init()
{
	const auto sbType = globalRendering->supportPersistentMapping
		? IStreamBufferConcept::Types::SB_PERSISTENTMAP
		: IStreamBufferConcept::Types::SB_BUFFERSUBDATA;

	Impl::InitCommon<MyDataType>(
		ssbo,
		transformsMemStorage,
		MATRIX_SSBO_BINDING_IDX, ELEM_COUNT0, ELEM_COUNTI,
		sbType, true, Impl::PERSISTENT_MAPPING_BUFFERING,
		className
	);
}

void TransformsUploader::Kill()
{
	Impl::KillCommon(ssbo, MATRIX_SSBO_BINDING_IDX);
}

void TransformsUploader::Update()
{
	if (!globalRendering->haveGL4)
		return;

	SCOPED_TIMER("TransformsUploader::Update");

	//auto lock = CModelsLock::GetScopedLock();

	Impl::UpdateCommon<MyDataType>(
		*this,
		ssbo,
		transformsMemStorage,
		className,
		__func__
	);
}

size_t TransformsUploader::GetElemOffset(const UnitDef* def) const
{
	if (def == nullptr) {
		LOG_L(L_ERROR, "[%s::%s] Supplied nullptr UnitDef", className, __func__);
		return TransformsMemStorage::INVALID_INDEX;
	}

	return GetElemOffset(def->LoadModel());
}

size_t TransformsUploader::GetElemOffset(const FeatureDef* def) const
{
	if (def == nullptr) {
		LOG_L(L_ERROR, "[%s::%s] Supplied nullptr FeatureDef", className, __func__);
		return TransformsMemStorage::INVALID_INDEX;
	}

	return GetElemOffset(def->LoadModel());
}

size_t TransformsUploader::GetElemOffset(const S3DModel* model) const
{
	if (model == nullptr) {
		LOG_L(L_ERROR, "[%s::%s] Supplied nullptr S3DModel", className, __func__);
		return TransformsMemStorage::INVALID_INDEX;
	}

	return model->GetMatAlloc().GetOffset(false);
}

size_t TransformsUploader::GetUnitDefElemOffset(int32_t unitDefID) const
{
	return GetElemOffset(unitDefHandler->GetUnitDefByID(unitDefID));
}

size_t TransformsUploader::GetFeatureDefElemOffset(int32_t featureDefID) const
{
	return GetElemOffset(featureDefHandler->GetFeatureDefByID(featureDefID));
}

size_t TransformsUploader::GetElemOffset(const CUnit* unit) const
{
	if (unit == nullptr) {
		LOG_L(L_ERROR, "[%s::%s] Supplied nullptr CUnit", className, __func__);
		return TransformsMemStorage::INVALID_INDEX;
	}

	if (size_t offset = CUnitDrawer::GetTransformMemAlloc(unit).GetOffset(false); offset != TransformsMemStorage::INVALID_INDEX) {
		return offset;
	}

	LOG_L(L_ERROR, "[%s::%s] Supplied invalid CUnit (id:%d)", className, __func__, unit->id);
	return TransformsMemStorage::INVALID_INDEX;
}

size_t TransformsUploader::GetElemOffset(const CFeature* feature) const
{
	if (feature == nullptr) {
		LOG_L(L_ERROR, "[%s::%s] Supplied nullptr CFeature", className, __func__);
		return TransformsMemStorage::INVALID_INDEX;
	}

	if (size_t offset = CFeatureDrawer::GetTransformMemAlloc(feature).GetOffset(false); offset != TransformsMemStorage::INVALID_INDEX) {
		return offset;
	}

	LOG_L(L_ERROR, "[%s::%s] Supplied invalid CFeature (id:%d)", className, __func__, feature->id);
	return TransformsMemStorage::INVALID_INDEX;
}

size_t TransformsUploader::GetElemOffset(const CProjectile* p) const
{
	if (p == nullptr) {
		LOG_L(L_ERROR, "[%s::%s] Supplied nullptr CProjectile", className, __func__);
		return TransformsMemStorage::INVALID_INDEX;
	}

	if (!p->synced) {
		LOG_L(L_ERROR, "[%s::%s] Supplied non-synced CProjectile (id:%d)", className, __func__, p->id);
		return TransformsMemStorage::INVALID_INDEX;
	}

	if (!p->weapon || !p->piece) {
		LOG_L(L_ERROR, "[%s::%s] Supplied non-weapon or non-piece CProjectile (id:%d)", className, __func__, p->id);
		return TransformsMemStorage::INVALID_INDEX;
	}
	/*
	if (size_t offset = p->GetMatAlloc().GetOffset(false); offset != TransformsMemStorage::INVALID_INDEX) {
		return offset;
	}
	*/

	LOG_L(L_ERROR, "[%s::%s] Supplied invalid CProjectile (id:%d)", className, __func__, p->id);
	return TransformsMemStorage::INVALID_INDEX;
}

size_t TransformsUploader::GetUnitElemOffset(int32_t unitID) const
{
	return GetElemOffset(unitHandler.GetUnit(unitID));
}

size_t TransformsUploader::GetFeatureElemOffset(int32_t featureID) const
{
	return GetElemOffset(featureHandler.GetFeature(featureID));
}

size_t TransformsUploader::GetProjectileElemOffset(int32_t syncedProjectileID) const
{
	return GetElemOffset(projectileHandler.GetProjectileBySyncedID(syncedProjectileID));
}

////////////////////////////////////////////////////////////////////

void ModelUniformsUploader::Init()
{
	if (!globalRendering->haveGL4)
		return;

	Impl::InitCommon<MyDataType>(
		ssbo,
		modelUniformsStorage,
		MATUNI_SSBO_BINDING_IDX, ELEM_COUNT0, ELEM_COUNTI,
		IStreamBufferConcept::Types::SB_BUFFERSUBDATA, true, 1,
		className
	);
}

void ModelUniformsUploader::Kill()
{
	Impl::KillCommon(ssbo, MATUNI_SSBO_BINDING_IDX);
}

void ModelUniformsUploader::Update()
{
	SCOPED_TIMER("ModelUniformsUploader::Update");

	Impl::UpdateCommon<MyDataType>(*this, ssbo, modelUniformsStorage, className, __func__);
}

size_t ModelUniformsUploader::GetElemOffset(const UnitDef* def) const
{
	assert(false);
	LOG_L(L_ERROR, "[%s::%s] Invalid call", className, __func__);
	return ModelUniformsStorage::INVALID_INDEX;
}

size_t ModelUniformsUploader::GetElemOffset(const FeatureDef* def) const
{
	assert(false);
	LOG_L(L_ERROR, "[%s::%s] Invalid call", className, __func__);
	return ModelUniformsStorage::INVALID_INDEX;
}

size_t ModelUniformsUploader::GetElemOffset(const S3DModel* model) const
{
	assert(false);
	LOG_L(L_ERROR, "[%s::%s] Invalid call", className, __func__);
	return ModelUniformsStorage::INVALID_INDEX;
}

size_t ModelUniformsUploader::GetUnitDefElemOffset(int32_t unitDefID) const
{
	return GetElemOffset(unitDefHandler->GetUnitDefByID(unitDefID));
}

size_t ModelUniformsUploader::GetFeatureDefElemOffset(int32_t featureDefID) const
{
	return GetElemOffset(featureDefHandler->GetFeatureDefByID(featureDefID));
}

size_t ModelUniformsUploader::GetElemOffset(const CUnit* unit) const
{
	if (unit == nullptr) {
		LOG_L(L_ERROR, "[%s::%s] Supplied nullptr CUnit", className, __func__);
		return ModelUniformsStorage::INVALID_INDEX;
	}

	if (size_t offset = modelUniformsStorage.GetObjOffset(unit); offset != size_t(-1)) {
		return offset;
	}

	LOG_L(L_ERROR, "[%s::%s] Supplied invalid CUnit (id:%d)", className, __func__, unit->id);
	return ModelUniformsStorage::INVALID_INDEX;
}

size_t ModelUniformsUploader::GetElemOffset(const CFeature* feature) const
{
	if (feature == nullptr) {
		LOG_L(L_ERROR, "[%s::%s] Supplied nullptr CFeature", className, __func__);
		return ModelUniformsStorage::INVALID_INDEX;
	}

	if (size_t offset = modelUniformsStorage.GetObjOffset(feature); offset != size_t(-1)) {
		return offset;
	}

	LOG_L(L_ERROR, "[%s::%s] Supplied invalid CFeature (id:%d)", className, __func__, feature->id);
	return ModelUniformsStorage::INVALID_INDEX;
}

size_t ModelUniformsUploader::GetElemOffset(const CProjectile* p) const
{
	if (p == nullptr) {
		LOG_L(L_ERROR, "[%s::%s] Supplied nullptr CProjectile", className, __func__);
		return ModelUniformsStorage::INVALID_INDEX;
	}

	if (size_t offset = modelUniformsStorage.GetObjOffset(p); offset != size_t(-1)) {
		return offset;
	}

	LOG_L(L_ERROR, "[%s::%s] Supplied invalid CProjectile (id:%d)", className, __func__, p->id);
	return ModelUniformsStorage::INVALID_INDEX;
}

size_t ModelUniformsUploader::GetUnitElemOffset(int32_t unitID) const
{
	return GetElemOffset(unitHandler.GetUnit(unitID));
}

size_t ModelUniformsUploader::GetFeatureElemOffset(int32_t featureID) const
{
	return GetElemOffset(featureHandler.GetFeature(featureID));
}

size_t ModelUniformsUploader::GetProjectileElemOffset(int32_t syncedProjectileID) const
{
	return GetElemOffset(projectileHandler.GetProjectileBySyncedID(syncedProjectileID));
}
