/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <catch_amalgamated.hpp>
#include <string_view>
#include <type_traits>
#include "Sim/Misc/TargetCheckResult.h"

TEST_CASE("Target diagnostics are query-owned values")
{
	using Result = TargetCheckResult;
	static_assert(!std::is_convertible_v<Result, bool>);
	static_assert(std::is_trivially_copyable_v<Result>);
	const Result blocked{Result::Friendly, Result::ObjectType::Unit, 42};
	const auto saved = blocked;
	for (const auto reason: {Result::Clear, Result::NotChecked, Result::InvalidTarget, Result::Range, Result::Terrain, Result::Blocked}) {
		const Result result = reason;
		CHECK(result == reason);
		CHECK(result.objectType == Result::ObjectType::None);
		CHECK(result.objectID == -1);
		CHECK(saved.objectID == 42);
		CHECK(saved == Result::Friendly);
	}
	CHECK(std::string_view(TargetCheckResultName(saved)) == "friendly");
	const Result feature{Result::Feature, Result::ObjectType::Feature, 42};
	CHECK(feature.objectType != saved.objectType);
	CHECK(feature != Result::Clear);
	CHECK(std::string_view(TargetCheckResultName(feature)) == "feature");
}
