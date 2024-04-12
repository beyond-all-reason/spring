/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "MeleeWeapon.h"
#include "WeaponDef.h"
#include "Sim/Units/Unit.h"

#include "System/Misc/TracyDefs.h"

CR_BIND_DERIVED(CMeleeWeapon, CWeapon, )
CR_REG_METADATA(CMeleeWeapon, )

void CMeleeWeapon::FireImpl(const bool scriptCall)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (currentTarget.type != Target_Unit)
		return;

	// the heavier the unit, the more impulse it does
	currentTarget.unit->DoDamage(*damages, wantedDir * owner->mass * damages->impulseFactor, owner, weaponDef->id, -1);
}
