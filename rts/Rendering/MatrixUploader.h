#ifndef MATRIX_UPLOADER_H
#define MATRIX_UPLOADER_H

#include <string>
#include <cstdint>
#include <vector>
#include <unordered_map>
#include <map>

#include "System/Matrix44f.h"
#include "System/SpringMath.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VBO.h"
#include "System/EventClient.h"

class MatrixUploader : public CEventClient {
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
	MatrixUploader() : CEventClient("[MatrixUploader]", 313374, false) {}
	// CEventClient interface
	virtual bool WantsEvent(const std::string& eventName) override {
		return (
			   eventName == "RenderUnitCreated"
			|| eventName == "RenderUnitDestroyed"
			|| eventName == "RenderFeatureCreated"
			|| eventName == "RenderFeatureDestroyed"
			|| eventName == "RenderProjectileCreated"
			|| eventName == "RenderProjectileDestroyed"
		);

	}
	virtual bool GetFullRead() const override { return true; }
	virtual int GetReadAllyTeam() const override { return AllAccessTeam; }

	virtual void RenderUnitCreated(const CUnit* unit, int cloaked) override {};
	virtual void RenderUnitDestroyed(const CUnit* unit) override {};
	virtual void RenderFeatureCreated(const CFeature* feature) override {};
	virtual void RenderFeatureDestroyed(const CFeature* feature) override {};
	virtual void RenderProjectileCreated(const CProjectile* projectile) override {};
	virtual void RenderProjectileDestroyed(const CProjectile* projectile) override {};
public:
	void Init();
	void Kill();
	void Update();
public:
	uint32_t GetUnitDefElemOffset(int32_t unitDefID);
	uint32_t GetFeatureDefElemOffset(int32_t featureDefID);
	uint32_t GetUnitElemOffset(int32_t unitID);
	uint32_t GetFeatureElemOffset(int32_t featureID);
private:
	template<typename TObj>
	bool IsObjectVisible(const TObj* obj);

	template<typename TObj>
	void GetVisibleObjects(std::map<int, const TObj*>& visibleObjects);
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

	std::unordered_map<int32_t, std::string> unitDefToModel;
	std::unordered_map<int32_t, std::string> featureDefToModel;
	std::unordered_map<std::string, uint32_t> modelToOffsetMap;

	std::unordered_map<int32_t, uint32_t> unitIDToOffsetMap;
	std::unordered_map<int32_t, uint32_t> featureIDToOffsetMap;
	std::unordered_map<int32_t, uint32_t> weaponIDToOffsetMap;

	std::vector<CMatrix44f> matrices;

	VBO* matrixSSBO = nullptr;
};

#endif //MATRIX_UPLOADER_H