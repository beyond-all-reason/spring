/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

// The first failed native targeting test, not a prediction of a future shot.
enum class TargetCheckResult {
	Clear = 0,
	NotChecked,
	InvalidTarget,
	Range,
	Terrain,
	Friendly,
	Neutral,
	Feature,
	Blocked,
};

inline const char* TargetCheckResultName(TargetCheckResult result)
{
	switch (result) {
		case TargetCheckResult::Clear: return "clear";
		case TargetCheckResult::NotChecked: return "notChecked";
		case TargetCheckResult::InvalidTarget: return "invalidTarget";
		case TargetCheckResult::Range: return "range";
		case TargetCheckResult::Terrain: return "terrain";
		case TargetCheckResult::Friendly: return "friendly";
		case TargetCheckResult::Neutral: return "neutral";
		case TargetCheckResult::Feature: return "feature";
		case TargetCheckResult::Blocked: return "blocked";
	}
	return "blocked";
}
