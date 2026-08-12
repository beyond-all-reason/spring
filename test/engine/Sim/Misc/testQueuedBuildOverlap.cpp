/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <catch_amalgamated.hpp>

#include <string_view>
#include <vector>

#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Units/BuildInfo.h"
#include "Sim/Units/QueuedBuildOverlap.h"
#include "Sim/Units/UnitDef.h"

// Keep this focused test independent from the full UnitDef parsing and model
// dependencies while exercising the production BuildInfo overlap interface.
SolidObjectDecalDef::SolidObjectDecalDef() = default;
SolidObjectDef::SolidObjectDef() = default;
UnitDef::UnitDef() = default;
BuildInfo::BuildInfo() = default;

int BuildInfo::GetXSize() const
{
	return ((buildFacing & 1) == 0) ? def->xsize : def->zsize;
}

int BuildInfo::GetZSize() const
{
	return ((buildFacing & 1) == 1) ? def->xsize : def->zsize;
}

namespace {

std::vector<YardMapStatus> ExpandLowResolutionYardMap(const std::vector<std::string_view>& rows)
{
	std::vector<YardMapStatus> yardMap;
	yardMap.reserve(rows.size() * rows.front().size() * 4);

	for (const std::string_view row: rows) {
		for (int z = 0; z < 2; ++z) {
			for (const char cell: row) {
				for (int x = 0; x < 2; ++x)
					yardMap.emplace_back(cell == 'y' ? YARDMAP_OPEN : YARDMAP_BLOCKED);
			}
		}
	}

	return yardMap;
}

BuildInfo MakeBuildInfo(
	const int2& mins,
	const int2& yardMapSize,
	int facing,
	UnitDef* unitDef,
	const std::vector<YardMapStatus>& yardMap
) {
	const int2 size = ((facing & 1) == 0) ? yardMapSize : int2{yardMapSize.y, yardMapSize.x};
	unitDef->xsize = yardMapSize.x;
	unitDef->zsize = yardMapSize.y;
	unitDef->yardmap = yardMap;

	BuildInfo buildInfo;
	buildInfo.def = unitDef;
	buildInfo.pos = {
		float((mins.x + (size.x >> 1)) * SQUARE_SIZE),
		0.0f,
		float((mins.y + (size.y >> 1)) * SQUARE_SIZE),
	};
	buildInfo.buildFacing = facing;
	return buildInfo;
}

} // namespace

TEST_CASE("Queued build overlap follows yardmaps like successive construction")
{
	const auto solarYardMap = ExpandLowResolutionYardMap({
		"yyoyy",
		"yoooy",
		"ooooo",
		"yoooy",
		"yyoyy",
	});
	UnitDef firstSolarDef;
	const auto firstSolar = MakeBuildInfo({0, 0}, {10, 10}, FACING_SOUTH, &firstSolarDef, solarYardMap);

	SECTION("diagonally interlocking solars are compatible") {
		UnitDef secondSolarDef;
		const auto secondSolar = MakeBuildInfo({6, 6}, {10, 10}, FACING_SOUTH, &secondSolarDef, solarYardMap);
		CHECK(QueuedBuildOverlap::Test(firstSolar, secondSolar) == QueuedBuildOverlap::Result::NONE);
		CHECK(QueuedBuildOverlap::Test(secondSolar, firstSolar) == QueuedBuildOverlap::Result::NONE);
		CHECK(QueuedBuildOverlap::Test(firstSolar, secondSolar, false) == QueuedBuildOverlap::Result::OVERLAP);
		CHECK(QueuedBuildOverlap::Test(secondSolar, firstSolar, false) == QueuedBuildOverlap::Result::OVERLAP);
	}

	SECTION("occupied yardmap cells still conflict") {
		UnitDef samePositionDef;
		const auto samePosition = MakeBuildInfo({0, 0}, {10, 10}, FACING_SOUTH, &samePositionDef, solarYardMap);
		CHECK(QueuedBuildOverlap::Test(firstSolar, samePosition) == QueuedBuildOverlap::Result::CANCEL);
	}

	SECTION("outer occupied-cell conflicts overlap without cancelling") {
		UnitDef outerOverlapDef;
		const auto outerOverlap = MakeBuildInfo({6, 0}, {10, 10}, FACING_SOUTH, &outerOverlapDef, solarYardMap);
		CHECK(QueuedBuildOverlap::Test(firstSolar, outerOverlap) == QueuedBuildOverlap::Result::OVERLAP);
	}

	SECTION("non-overlapping footprints are compatible") {
		UnitDef distantSolarDef;
		const auto distantSolar = MakeBuildInfo({10, 10}, {10, 10}, FACING_SOUTH, &distantSolarDef, solarYardMap);
		CHECK(QueuedBuildOverlap::Test(firstSolar, distantSolar) == QueuedBuildOverlap::Result::NONE);
	}

	SECTION("missing blocker yardmap keeps rectangular fallback") {
		UnitDef noYardMapDef;
		const auto noYardMap = MakeBuildInfo({0, 0}, {10, 10}, FACING_SOUTH, &noYardMapDef, {});
		CHECK(QueuedBuildOverlap::Test(noYardMap, firstSolar) == QueuedBuildOverlap::Result::CANCEL);
	}
}

TEST_CASE("Queued build overlap is directional")
{
	const std::vector<YardMapStatus> buildOnly = {YARDMAP_BUILDONLY};
	const std::vector<YardMapStatus> blocked = {YARDMAP_BLOCKED};
	UnitDef buildOnlyDef;
	UnitDef blockedDef;
	const auto buildOnlyFootprint = MakeBuildInfo({0, 0}, {1, 1}, FACING_SOUTH, &buildOnlyDef, buildOnly);
	const auto blockedFootprint = MakeBuildInfo({0, 0}, {1, 1}, FACING_SOUTH, &blockedDef, blocked);

	CHECK(QueuedBuildOverlap::Test(buildOnlyFootprint, blockedFootprint) == QueuedBuildOverlap::Result::NONE);
	CHECK(QueuedBuildOverlap::Test(blockedFootprint, buildOnlyFootprint) == QueuedBuildOverlap::Result::CANCEL);
}

TEST_CASE("Queued build yardmap index handles every facing")
{
	const int2 southXRange = {0, 2};
	const int2 southZRange = {0, 3};
	const int2 eastXRange = {0, 3};
	const int2 eastZRange = {0, 2};

	CHECK(QueuedBuildOverlap::GetYardMapIndex(FACING_SOUTH, {0, 0}, southXRange, southZRange) == 0);
	CHECK(QueuedBuildOverlap::GetYardMapIndex(FACING_SOUTH, {1, 2}, southXRange, southZRange) == 5);
	CHECK(QueuedBuildOverlap::GetYardMapIndex(FACING_NORTH, {0, 0}, southXRange, southZRange) == 5);
	CHECK(QueuedBuildOverlap::GetYardMapIndex(FACING_NORTH, {1, 2}, southXRange, southZRange) == 0);
	CHECK(QueuedBuildOverlap::GetYardMapIndex(FACING_EAST, {0, 0}, eastXRange, eastZRange) == 1);
	CHECK(QueuedBuildOverlap::GetYardMapIndex(FACING_EAST, {2, 1}, eastXRange, eastZRange) == 4);
	CHECK(QueuedBuildOverlap::GetYardMapIndex(FACING_WEST, {0, 0}, eastXRange, eastZRange) == 4);
	CHECK(QueuedBuildOverlap::GetYardMapIndex(FACING_WEST, {2, 1}, eastXRange, eastZRange) == 1);
}
