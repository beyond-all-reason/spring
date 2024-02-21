#include "TerraformTask.h"

#include "Map/ReadMap.h"
#include "Map/MapDamage.h"
#include "Map/Ground.h"
#include "System/ContainerUtil.h"
#include "Sim/Units/Unit.h"


CR_BIND(TerraformTask, )
CR_REG_METADATA(TerraformTask, (
	CR_MEMBER(id),
	CR_MEMBER(bounds),
	CR_MEMBER(cost),
	CR_MEMBER(amount),
	CR_MEMBER(speed),
	CR_MEMBER(buildee),
	CR_MEMBER(refCnt)
))

CR_BIND(TerraformTaskHandler, )
CR_REG_METADATA(TerraformTaskHandler, (
	CR_MEMBER(nextId),
	CR_MEMBER(terraformTasks)
))

CR_BIND(TerraformTaskToken, )
CR_REG_METADATA(TerraformTaskToken, (
	CR_MEMBER(id)
))

TerraformTask::TerraformTask(const float3& center, const float2& dims, CUnit* buildee, const SResourcePack& cost)
	: TerraformTask(
		SRectangle(
			static_cast<int>((center.x - dims.x) / SQUARE_SIZE),
			static_cast<int>((center.z - dims.y) / SQUARE_SIZE),
			static_cast<int>((center.x + dims.x) / SQUARE_SIZE),
			static_cast<int>((center.z + dims.y) / SQUARE_SIZE)
		)
		, buildee, cost
	)
{}

TerraformTask::TerraformTask(SRectangle&& bounds_, CUnit* buildee_, const SResourcePack& cost_)
	: id(terraformTaskHandler.GetNextId())
	, refCnt(0)
	, bounds(std::move(bounds_))
	, buildee(buildee_)
	, cost(cost_)
	, speed(0.0f)
{
	bounds.x1 = std::clamp(bounds.x1, 0, mapDims.mapx);
	bounds.x2 = std::clamp(bounds.x2, 0, mapDims.mapx);
	bounds.z1 = std::clamp(bounds.z1, 0, mapDims.mapy);
	bounds.z2 = std::clamp(bounds.z2, 0, mapDims.mapy);

	amount = 0.0f;
	const float* curHeightMap = readMap->GetCornerHeightMapSynced();
	const float* orgHeightMap = readMap->GetOriginalHeightMapSynced();

	if (buildee) {
		for (int z = bounds.z1; z <= bounds.z2; z++) {
			for (int x = bounds.x1; x <= bounds.x2; x++) {
				float delta = buildee->pos.y - curHeightMap[z * mapDims.mapxp1 + x];
				amount += math::fabs(delta);
			}
		}
	}
	else {
		for (int z = bounds.z1; z <= bounds.z2; z++) {
			for (int x = bounds.x1; x <= bounds.x2; x++) {
				float delta = orgHeightMap[z * mapDims.mapxp1 + x] - curHeightMap[z * mapDims.mapxp1 + x];
				amount += math::fabs(delta);
			}
		}
	}

	if (amount > 0.0f)
		cost *= 1.0f / amount; //cost per 1 amount
	else
		cost *= 0.0f;
}

void TerraformTask::Update()
{
	if (speed == 0.0f)
		return;

	assert(!TerraformComplete());

	const float scale = std::clamp(speed / amount, 0.0f, 1.0f);

	// amount -= speed; // dangerous due to floating point inprecision

	//TODO handle resources

	speed = 0.0f;

	const float* curHeightMap = readMap->GetCornerHeightMapSynced();
	const float* orgHeightMap = readMap->GetOriginalHeightMapSynced();

	float terraformed = 0.0;
	// Terraform the building site
	if (buildee) {
		for (int z = bounds.z1; z <= bounds.z2; z++) {
			for (int x = bounds.x1; x <= bounds.x2; x++) {
				const int idx = z * mapDims.mapxp1 + x;
				const float dh = (buildee->pos.y - curHeightMap[idx]) * scale;
				readMap->AddHeight(idx, dh);
				terraformed += math::fabsf(dh);
			}
		}
	}
	// Terraform Restore ground
	else {
		for (int z = bounds.z1; z <= bounds.z2; z++) {
			for (int x = bounds.x1; x <= bounds.x2; x++) {
				const int idx = z * mapDims.mapxp1 + x;
				const float dh = (orgHeightMap[idx] - curHeightMap[idx]) * scale;
				readMap->AddHeight(idx, dh);
				terraformed += math::fabsf(dh);
			}
		}
	}

	if (terraformed > 0.0f) {
		amount -= terraformed;
		SmoothBorders(scale);
		mapDamage->RecalcArea(
			bounds.x1 - BORDER_SIZE,
			bounds.x2 + BORDER_SIZE,
			bounds.z1 - BORDER_SIZE,
			bounds.z2 + BORDER_SIZE
		);
	}
	else {
		amount = 0.0f;
	}
}

float3 TerraformTask::GetCenterPos() const
{
	float x = (bounds.x2 + bounds.x1) * (SQUARE_SIZE >> 1);
	float z = (bounds.z2 + bounds.z1) * (SQUARE_SIZE >> 1);
	float y = CGround::GetApproximateHeight(x, z, true);
	return float3(x, y, z);
}

float TerraformTask::GetRadius() const
{
	// not the real radius, rather maximum dimension
	return std::max(bounds.x2 - bounds.x1, bounds.z2 - bounds.z1) * 0.5f;
}

void TerraformTask::SmoothBorders(float scale) const
{
	const float* heightmap = readMap->GetCornerHeightMapSynced();

	// smooth the x-borders
	for (int z = bounds.z1; z <= bounds.z2; z++) {
		for (int x = 1; x <= BORDER_SIZE; x++) {
			if (bounds.x1 - BORDER_SIZE >= 0) {
				const float chb = heightmap[z * mapDims.mapxp1 + bounds.x1 -           0];
				const float ch  = heightmap[z * mapDims.mapxp1 + bounds.x1 -           x];
				const float che = heightmap[z * mapDims.mapxp1 + bounds.x1 - BORDER_SIZE];
				const float amount = ((chb * (BORDER_SIZE - x) + che * x) / BORDER_SIZE - ch) * scale;

				readMap->AddHeight(z * mapDims.mapxp1 + bounds.x1 - x, amount);
			}
			if (bounds.x2 + BORDER_SIZE < mapDims.mapx) {
				const float chb = heightmap[z * mapDims.mapxp1 + bounds.x2 +           0];
				const float ch  = heightmap[z * mapDims.mapxp1 + bounds.x2 +           x];
				const float che = heightmap[z * mapDims.mapxp1 + bounds.x2 + BORDER_SIZE];
				const float amount = ((chb * (BORDER_SIZE - x) + che * x) / BORDER_SIZE - ch) * scale;

				readMap->AddHeight(z * mapDims.mapxp1 + bounds.x2 + x, amount);
			}
		}
	}

	// smooth the z-borders
	for (int z = 1; z <= BORDER_SIZE; z++) {
		for (int x = bounds.x1; x <= bounds.x2; x++) {
			if ((bounds.z1 - BORDER_SIZE) >= 0) {
				const float chb = heightmap[(bounds.z1           - 0) * mapDims.mapxp1 + x];
				const float ch  = heightmap[(bounds.z1           - z) * mapDims.mapxp1 + x];
				const float che = heightmap[(bounds.z1 - BORDER_SIZE) * mapDims.mapxp1 + x];
				const float adjust = ((chb * (BORDER_SIZE - z) + che * z) / BORDER_SIZE - ch) * scale;

				readMap->AddHeight((bounds.z1 - z) * mapDims.mapxp1 + x, adjust);
			}
			if ((bounds.z2 + BORDER_SIZE) < mapDims.mapy) {
				const float chb = heightmap[(bounds.z2 + 0) * mapDims.mapxp1 + x];
				const float ch  = heightmap[(bounds.z2 + z) * mapDims.mapxp1 + x];
				const float che = heightmap[(bounds.z2 + BORDER_SIZE) * mapDims.mapxp1 + x];
				const float adjust = ((chb * (BORDER_SIZE - z) + che * z) / 3 - ch) * scale;

				readMap->AddHeight((bounds.z2 + z) * mapDims.mapxp1 + x, adjust);
			}
		}
	}

}

TerraformTaskHandler terraformTaskHandler;

void TerraformTaskHandler::Init()
{
	terraformTasks = {};
}

void TerraformTaskHandler::Kill()
{
	spring::VectorEraseAllIf(terraformTasks, [](auto item) { return item.GetRefCnt() == 0; });
	assert(terraformTasks.empty());
}

void TerraformTaskHandler::Update()
{
	for (size_t i = 0; i < terraformTasks.size(); /*NOOP*/) {
		if (terraformTasks[i].GetRefCnt() == 0) {
			terraformTasks[i] = terraformTasks.back();
			terraformTasks.pop_back();
		}
		else {
			terraformTasks[i].Update();
			++i;
		}
	}
}

TerraformTask* TerraformTaskHandler::GetById(uint32_t id)
{
	if (id == 0)
		return nullptr;

	auto it = std::find_if(terraformTasks.begin(), terraformTasks.end(), [id](const TerraformTask& tt) { return tt.id == id; });
	if (it == terraformTasks.end())
		return nullptr;

	return &(*it);
}

TerraformTaskToken TerraformTaskHandler::AddTerraformTask(SRectangle&& bounds, CUnit* buildee, const SResourcePack& cost)
{
	for (auto& tt : terraformTasks) {
		if (tt.GetRefCnt() == 0)
			continue;

		if (tt.buildee == buildee && tt.bounds == bounds)
			return TerraformTaskToken(tt.id);
	}

	return TerraformTaskToken(
		terraformTasks.emplace_back(
		std::move(bounds), buildee, cost
	).id);
}

TerraformTaskToken TerraformTaskHandler::AddTerraformTask(const float3& center, const float2& dims, CUnit* buildee, const SResourcePack& cost)
{
	auto bounds = SRectangle{
		std::clamp(static_cast<int>((center.x - dims.x) / SQUARE_SIZE), 0, mapDims.mapx),
		std::clamp(static_cast<int>((center.z - dims.y) / SQUARE_SIZE), 0, mapDims.mapy),
		std::clamp(static_cast<int>((center.x + dims.x) / SQUARE_SIZE), 0, mapDims.mapx),
		std::clamp(static_cast<int>((center.z + dims.y) / SQUARE_SIZE), 0, mapDims.mapy)
	};
	return AddTerraformTask(std::move(bounds), buildee, cost);
}