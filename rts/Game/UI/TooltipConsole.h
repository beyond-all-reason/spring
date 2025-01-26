/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef TOOL_TIP_CONSOLE_H
#define TOOL_TIP_CONSOLE_H

#include "Sim/Misc/Resource.h"
#include "InputReceiver.h"
#include <string>

class CUnit;
class CFeature;
class float3;

struct SUnitStats {
	SUnitStats();

	void AddUnit(const CUnit* u, bool enemy);

public:
	float health, maxHealth;
	float experience;
	float cost;
	float maxRange;
	SResourcePack resourceMake;
	SResourcePack resourceUse;
	SResourcePack resourceHarvest;
	SResourcePack resourceHarvestMax;

	int count;
};


class CTooltipConsole : public CInputReceiver {
	public:
		CTooltipConsole();
		~CTooltipConsole();

		void Draw();
		bool IsAbove(int x, int y);

		// helpers
		static std::string MakeUnitString(const CUnit* unit);
		static std::string MakeFeatureString(const CFeature* feature);
		static std::string MakeGroundString(const float3& pos);
		static std::string MakeUnitStatsString(const SUnitStats& stats);

		bool enabled;

	protected:
		float x, y, w, h;
		bool outFont;
};

extern CTooltipConsole* tooltip;

#endif /* TOOL_TIP_CONSOLE_H */
