/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef OBJECT_DEPENDENCE_TYPES_H
#define OBJECT_DEPENDENCE_TYPES_H

#include <cstdint>

// The reason to have different dependence types is that an object may simultaneously have more than one kind of
// dependence to another object. Without dependence types, the dependencies would therefore need to be duplicated
// (stored as lists or vectors instead of maps or sets). This would also make deleting the death dependence somewhat
// risky since there in this case must be an exact matching number of AddDeathDependence / DeleteDeathDependence calls
// in order not to risk a crash. With dependence types this can never happen, i.e. DeleteDeathDependence(object,
// DEPENDENCE_ATTACKER) can be called a hundred times without any risk of losing some other type of dependence.
// Dependence types also makes it easy to detect deletion of non-existent dependence types, and output a warning for
// debugging purposes.
enum DependenceType : uint8_t {
	DEPENDENCE_ATTACKER,
	DEPENDENCE_BUILD,
	DEPENDENCE_BUILDER,
	DEPENDENCE_CAPTURE,
	DEPENDENCE_COMMANDQUE,
	DEPENDENCE_DECOYTARGET,
	DEPENDENCE_INCOMING,
	DEPENDENCE_INTERCEPT,
	DEPENDENCE_INTERCEPTABLE,
	DEPENDENCE_INTERCEPTTARGET,
	DEPENDENCE_LASTCOLWARN,
	DEPENDENCE_ORDERTARGET,
	DEPENDENCE_RECLAIM,
	DEPENDENCE_RESURRECT,
	DEPENDENCE_TARGET,
	DEPENDENCE_TARGETUNIT,
	DEPENDENCE_TERRAFORM,
	DEPENDENCE_TRANSPORTEE,
	DEPENDENCE_TRANSPORTER,
	DEPENDENCE_WAITCMD,
	DEPENDENCE_WEAPON,
	DEPENDENCE_WEAPONTARGET,
	DEPENDENCE_REPULSE,
	DEPENDENCE_REPULSED,
	DEPENDENCE_SOLIDONTOP,
	// unsynced, keep them last
	DEPENDENCE_LIGHT,
	DEPENDENCE_SELECTED,

	DEPENDENCE_NONE,
	DEPENDENCE_COUNT
};

#endif
