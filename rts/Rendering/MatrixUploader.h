#pragma once

#include <string>
#include <cstdint>
#include <vector>
#include <functional>

#include "System/Matrix44f.h"
#include "System/SpringMath.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VBO.h"


class CSolidObject;
struct SolidObjectDef;

class MatrixUploader {
public:
	static constexpr bool enabled = true;
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
	std::size_t GetUnitDefElemOffset(int32_t unitDefID) const;
	std::size_t GetFeatureDefElemOffset(int32_t featureDefID) const;

	std::size_t GetUnitElemOffset(int32_t unitID) const;
	std::size_t GetFeatureElemOffset(int32_t featureID) const;
private:
	template<typename TObj>
	static bool IsObjectVisible(const TObj* obj);
private:
	std::size_t GetDefElemOffsetImpl(int32_t defID, const SolidObjectDef* def, const char* defType) const;
	std::size_t GetElemOffsetImpl(uint32_t id, const CSolidObject* so, const char* objType) const;
private:
	void KillVBO();
	void InitVBO(const uint32_t newElemCount);
	uint32_t GetMatrixElemCount() const;
private:
	static constexpr uint32_t MATRIX_SSBO_BINDING_IDX = 0;
	static constexpr uint32_t elemCount0 = 1u << 13;
	static constexpr uint32_t elemIncreaseBy = 1u << 12;
private:
	VBO matrixSSBO;
};