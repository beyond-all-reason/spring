#pragma once

#include "System/Rectangle.h"
#include "System/float3.h"
#include "System/type2.h"
#include "System/Object.h"
#include "Sim/Misc/Resource.h"

class CUnit;
class TerraformTaskToken;

class TerraformTask {
public:
	friend class TerraformTaskToken;
	CR_DECLARE(TerraformTask)


	TerraformTask() = default; //creg only
	TerraformTask(const float3& center, const float2& dims, CUnit* buildee, const SResourcePack& cost = {});
	TerraformTask(SRectangle&& bounds, CUnit* buildee, const SResourcePack& cost = {});


	void Update();
	void AddTerraformSpeed(float addSpeed) { speed += addSpeed; }

	bool TerraformComplete() const { return amount <= 0.0f; }
	float3 GetCenterPos() const;
	float GetRadius() const;
	uint32_t GetRefCnt() const { return refCnt; }

	uint32_t id;
	SRectangle bounds;
	SResourcePack cost;
	float amount;
	float speed;
	CUnit* buildee;

	static constexpr int BORDER_SIZE = 3;
private:
	uint32_t refCnt;

	void SmoothBorders(float scale) const;
};

class TerraformTaskHandler {
public:
	CR_DECLARE(TerraformTaskHandler)
	void Init();
	void Kill();
	void Update();

	TerraformTaskToken AddTerraformTask(SRectangle&& bounds, CUnit* buildee, const SResourcePack& cost = {});
	TerraformTaskToken AddTerraformTask(const float3& center, const float2& dims, CUnit* buildee, const SResourcePack& cost = {});

	TerraformTask* GetById(uint32_t id);

	uint32_t GetNextId() {
		nextId = (nextId % ID_WRAPAROUND) + 1; return nextId;
	}
	uint32_t nextId = 0; // 0 in fact is reserved and never used
	std::vector<TerraformTask> terraformTasks;
private:
	static constexpr uint32_t ID_WRAPAROUND = 1 << 16;
};

extern TerraformTaskHandler terraformTaskHandler;


class TerraformTaskToken {
public:
	CR_DECLARE(TerraformTaskToken)
	TerraformTaskToken() = default; //creg
	TerraformTaskToken(uint32_t id_)
		:id(id_)
	{
		if (auto* tt = terraformTaskHandler.GetById(id); tt)
			tt->refCnt++;
	}
	~TerraformTaskToken() {
		if (auto* tt = terraformTaskHandler.GetById(id); tt)
			tt->refCnt--;
	}
	TerraformTaskToken(TerraformTaskToken&& ttt) noexcept { *this = std::move(ttt); }
	TerraformTaskToken(const TerraformTaskToken& ttt) noexcept { *this = ttt; }
	TerraformTaskToken(std::nullptr_t) { *this = nullptr; }

	bool operator ==(std::nullptr_t) const noexcept { return id == 0; }
	bool operator !=(std::nullptr_t) const noexcept { return id != 0; }
	bool operator ==(const TerraformTaskToken& ttt) const noexcept { return id == ttt.id; }
	bool operator !=(const TerraformTaskToken& ttt) const noexcept { return id != ttt.id; }

	TerraformTaskToken& operator=(TerraformTaskToken&& ttt) noexcept {
		if (&ttt == this)
			return *this;

		std::swap(id, ttt.id);
		return *this;
	}
	TerraformTaskToken& operator=(const TerraformTaskToken& ttt) {
		if (&ttt == this)
			return *this;

		id = ttt.id;
		if (auto* tt = terraformTaskHandler.GetById(id); tt)
			tt->refCnt++;

		return *this;
	}
	TerraformTaskToken& operator=(std::nullptr_t) noexcept {
		if (auto* tt = terraformTaskHandler.GetById(id); tt)
			tt->refCnt--;

		id = 0;

		return *this;
	}
	TerraformTask* GetTask() const { return terraformTaskHandler.GetById(id); }
private:
	uint32_t id;
};
