#pragma once

#include "System/Rectangle.h"
#include "System/float3.h"
#include "System/type2.h"
#include "System/Object.h"
#include "System/RefCnt.h"
#include "Sim/Misc/Resource.h"

class CUnit;
class TerraformTask;
class TerraformTask {
public:
	CR_DECLARE(TerraformTask)
	static void Init();
	static void Kill();

	TerraformTask() = default; //creg only
	TerraformTask(const float3& center, const float2& dims, CUnit* buildee, const SResourcePack& cost = {});
	TerraformTask(SRectangle&& bounds, CUnit* buildee, const SResourcePack& cost = {});

	static void UpdateAll();
	void Update();
	void AddTerraformSpeed(float addSpeed) { speed += addSpeed; }

	static recoil::LightSharedPtr<TerraformTask> AddTerraformTask(SRectangle && bounds, CUnit * buildee, const SResourcePack & cost = {});
	static recoil::LightSharedPtr<TerraformTask> AddTerraformTask(const float3& center, const float2& dims, CUnit* buildee, const SResourcePack& cost = {});

	bool TerraformComplete() const { return amount <= 0.0f; }
	float3 GetCenterPos() const;
	float GetRadius() const;

	SRectangle bounds;
	SResourcePack cost;
	float amount;
	float speed;
	CUnit* buildee;

	static constexpr int BORDER_SIZE = 3;
private:
	void SmoothBorders(float scale) const;

	inline static std::vector<recoil::LightWeakPtr<TerraformTask>> terraformTasks;
};