/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _BUILDING_H
#define _BUILDING_H

#include "Sim/Units/Unit.h"
#include "System/float3.h"

class CBuilding : public CUnit
{
public:
	CR_DECLARE(CBuilding)

	CBuilding(): CUnit() { immobile = true; }

	// Unblock is required here because the blockMap is not available during ~CUnit()
	virtual ~CBuilding() { UnBlock(); };

	void PostLoad();
	void PreInit(const UnitLoadParams& params) override;
	void PostInit(const CUnit* builder) override;
	void ForcedMove(const float3& newPos) override;

	const YardMapStatus* GetBlockMap() const override { return blockMap; }

protected:
	// current unrotated blockmap/yardmap of this object;
	// null means no active yardmap (all squares blocked)
	const YardMapStatus* blockMap = nullptr;
};

#endif // _BUILDING_H
