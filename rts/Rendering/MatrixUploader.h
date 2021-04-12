#ifndef MATRIX_UPLOADER_H
#define MATRIX_UPLOADER_H

#include <string>
#include <cstdint>
#include <vector>
#include <unordered_map>
#include <map>

#include "System/UnorderedMap.hpp"
#include "System/Matrix44f.h"
#include "System/SpringMath.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VBO.h"

struct S3DModel;
struct UnitDef;
struct FeatureDef;
struct CUnit;
struct CFeature;

class MatrixUploader {
public:
	static constexpr bool enabled = true;
	static constexpr bool checkInView = false;
	static MatrixUploader& GetInstance() {
		static MatrixUploader instance;
		return instance;
	};
	static bool Supported() {
		static bool supported = enabled && VBO::IsSupported(GL_SHADER_STORAGE_BUFFER) && GLEW_ARB_shading_language_420pack; //UBO && UBO layout(binding=x)
		return supported;
	}
public:
	void Init();
	void Kill();
	void Update();
public:
	uint32_t GetModelElemOffset(const int32_t model);
	uint32_t GetUnitDefElemOffset(const int32_t unitDef);
	uint32_t GetFeatureDefElemOffset(const int32_t featureDef);
	uint32_t GetUnitElemOffset(const int32_t unit);
	uint32_t GetFeatureElemOffset(const int32_t feature);

	uint32_t GetElemOffset(const S3DModel* model);
	uint32_t GetElemOffset(const UnitDef* unitDef);
	uint32_t GetElemOffset(const FeatureDef* featureDef);
	uint32_t GetElemOffset(const CUnit* unit);
	uint32_t GetElemOffset(const CFeature* feature);
private:
	template<typename TObj>
	bool IsObjectVisible(const TObj* obj);

	template<typename TObj>
	bool IsInView(const TObj* obj);

	template<typename TObj>
	void GetVisibleObjects(spring::unordered_map<int, const TObj*>& visibleObjects);
private:
	void KillVBO();
	void InitVBO(const uint32_t newElemCount);
	uint32_t GetMatrixElemCount();

	bool UpdateObjectDefs();

	template<typename TObj>
	void UpdateVisibleObjects();
private:
	static constexpr uint32_t MATRIX_SSBO_BINDING_IDX = 0;
	static constexpr uint32_t elemCount0 = 1u << 13;
	static constexpr uint32_t elemIncreaseBy = 1u << 12;
private:
	uint32_t elemUpdateOffset = 0u; // a index offset separating constant part of the buffer from varying part

	spring::unordered_map<int32_t, int32_t> unitDefToModel;
	spring::unordered_map<int32_t, int32_t> featureDefToModel;
	spring::unordered_map<int32_t, uint32_t> modelToOffsetMap;

	spring::unordered_map<int32_t, uint32_t> unitIDToOffsetMap;
	spring::unordered_map<int32_t, uint32_t> featureIDToOffsetMap;
	spring::unordered_map<int32_t, uint32_t> weaponIDToOffsetMap;

	std::vector<CMatrix44f> matrices;

	VBO* matrixSSBO;
};

#endif //MATRIX_UPLOADER_H