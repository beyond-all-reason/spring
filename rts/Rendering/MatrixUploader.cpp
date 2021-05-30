#include "MatrixUploader.h"

#include <limits>
#include <cassert>

#include "System/float4.h"
#include "System/Matrix44f.h"
#include "System/Log/ILog.h"
#include "System/SpringMath.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Objects/SolidObject.h"
#include "Sim/Projectiles/Projectile.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Objects/SolidObjectDef.h"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Features/FeatureDef.h"
#include "Sim/Features/FeatureDefHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Rendering/Models/3DModel.h"
#include "Rendering/Models/MatricesMemStorage.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Game/GlobalUnsynced.h"

//#include "System/TimeProfiler.h" //KILL ME

void MatrixUploader::InitVBO(const uint32_t newElemCount)
{
	matrixSSBO = VBO(GL_SHADER_STORAGE_BUFFER, false, false);
	matrixSSBO.Bind(GL_SHADER_STORAGE_BUFFER);
	matrixSSBO.New(newElemCount * sizeof(CMatrix44f), GL_STREAM_DRAW);
	matrixSSBO.Unbind();

	matrixSSBO.BindBufferRange(GL_SHADER_STORAGE_BUFFER, MATRIX_SSBO_BINDING_IDX, 0, matrixSSBO.GetSize());
}

void MatrixUploader::Init()
{
	if (!MatrixUploader::Supported())
		return;

	InitVBO(elemCount0);
}

void MatrixUploader::KillVBO()
{
	if (matrixSSBO.GetIdRaw() > 0u) {
		if (matrixSSBO.bound)
			matrixSSBO.Unbind();

		matrixSSBO.UnbindBufferRange(GL_SHADER_STORAGE_BUFFER, MATRIX_SSBO_BINDING_IDX, 0, matrixSSBO.GetSize());
	}

	matrixSSBO = {};
}

void MatrixUploader::Kill()
{
	if (!MatrixUploader::Supported())
		return;

	KillVBO();
}

uint32_t MatrixUploader::GetMatrixElemCount() const
{
	return matrixSSBO.GetSize() / sizeof(CMatrix44f);
}

void MatrixUploader::Update()
{
	if (!MatrixUploader::Supported())
		return;

		//resize
		const uint32_t matrixElemCount = GetMatrixElemCount();
		const uint32_t matricesMemStorageCount = matricesMemStorage.GetSize();
		if (matricesMemStorageCount > matrixElemCount) {
			const uint32_t newElemCount = AlignUp(matricesMemStorageCount, elemIncreaseBy);
			LOG_L(L_INFO, "MatrixUploader::%s sizing matrixSSBO %s. New elements count = %u, matrixElemCount = %u, matricesMemStorageCount = %u", __func__, "up", newElemCount, matrixElemCount, matricesMemStorageCount);
			matrixSSBO.UnbindBufferRange(MATRIX_SSBO_BINDING_IDX);
			matrixSSBO.Bind();
			matrixSSBO.Resize(newElemCount * sizeof(CMatrix44f), GL_STREAM_DRAW);
			matrixSSBO.Unbind();
			matrixSSBO.BindBufferRange(MATRIX_SSBO_BINDING_IDX);
		}

		//update on the GPU
		matrixSSBO.Bind();
#if 0 //unexpectedly expensive on idle run (NVidia & Windows). Needs triple buffering to perform
		auto* buff = matrixSSBO.MapBuffer(matricesMemStorage.GetData(), GL_WRITE_ONLY); //matrices.size() always has the correct size no matter neededElemByteOffset
		memcpy(buff, matricesMemStorage.GetData().data(), matricesMemStorage.GetSize() * sizeof(CMatrix44f));
		matrixSSBO.UnmapBuffer();
#else
		matrixSSBO.SetBufferSubData(matricesMemStorage.GetData());
#endif
		matrixSSBO.Unbind();
}

template<typename TObj>
bool MatrixUploader::IsObjectVisible(const TObj* obj)
{
	if (losHandler->GetGlobalLOS(gu->myAllyTeam))
		return true;

	if constexpr (std::is_same<TObj, CProjectile>::value) //CProjectile has no IsInLosForAllyTeam()
		return losHandler->InLos(obj, gu->myAllyTeam);
	else //else is needed. Otherwise won't compile
		return obj->IsInLosForAllyTeam(gu->myAllyTeam);
}

std::size_t MatrixUploader::GetDefElemOffsetImpl(int32_t defID, const SolidObjectDef* def, const char* defType)  const
{
	if (def == nullptr) {
		LOG_L(L_ERROR, "[MatrixUploader::%s] Supplied invalid %s %d", __func__, defType, defID);
		return MatricesMemStorage::INVALID_INDEX;
	}

	const S3DModel* mdl = def->LoadModel();
	if (mdl == nullptr) {
		LOG_L(L_ERROR, "[MatrixUploader::%s] Failed to load model for %s %d", __func__, defType, defID);
		return MatricesMemStorage::INVALID_INDEX;
	}

	return mdl->GetMatAlloc().GetOffset();
}

std::size_t MatrixUploader::GetUnitDefElemOffset(int32_t unitDefID)  const
{
	return GetDefElemOffsetImpl(unitDefID, unitDefHandler->GetUnitDefByID(unitDefID), "UnitDefID");
}

std::size_t MatrixUploader::GetFeatureDefElemOffset(int32_t featureDefID)  const
{
	return GetDefElemOffsetImpl(featureDefID, featureDefHandler->GetFeatureDefByID(featureDefID), "FeatureDefID");
}


std::size_t MatrixUploader::GetElemOffsetImpl(uint32_t id, const CSolidObject* so, const char* objType)  const
{
	if (so == nullptr) {
		LOG_L(L_ERROR, "[MatrixUploader::%s] Supplied invalid %s %d", __func__, objType, id);
		return MatricesMemStorage::INVALID_INDEX;
	}

	if (std::size_t offset = so->localModel.GetMatAlloc().GetOffset(); offset != MatricesMemStorage::INVALID_INDEX) {
		return offset;
	}

	LOG_L(L_ERROR, "[MatrixUploader::%s] Supplied invalid %s %d", __func__, objType, id);
	return MatricesMemStorage::INVALID_INDEX;
};


std::size_t MatrixUploader::GetUnitElemOffset(int32_t unitID)  const
{
	return GetElemOffsetImpl(unitID, unitHandler.GetUnit(unitID), "UnitID");
}


std::size_t MatrixUploader::GetFeatureElemOffset(int32_t featureID)  const
{
	return GetElemOffsetImpl(featureID, featureHandler.GetFeature(featureID), "FeatureID");
}