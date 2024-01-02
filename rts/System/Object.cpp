/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <algorithm>

#include "System/Object.h"
#include "System/ContainerUtil.h"
#include "System/creg/STL_Map.h"
#include "System/Log/ILog.h"
#include "System/Platform/CrashHandler.h"

CR_BIND(CObject, )

CR_REG_METADATA(CObject, (
	CR_MEMBER(sync_id),

	CR_MEMBER(detached),

	CR_MEMBER(listening),
	CR_MEMBER(listeners),
	CR_MEMBER(listenersDepTbl),
	CR_MEMBER(listeningDepTbl)
))

std::atomic<std::int64_t> CObject::cur_sync_id(0);



static bool VectorInsertSorted(std::vector<CObject*>& v, CObject* o)
{
	return (spring::VectorInsertUniqueSorted(v, o, [](const CObject* a, const CObject* b) { return (a->GetSyncID() < b->GetSyncID()); }));
}

static bool VectorEraseSorted(std::vector<CObject*>& v, CObject* o)
{
	return (spring::VectorEraseUniqueSorted(v, o, [](const CObject* a, const CObject* b) { return (a->GetSyncID() < b->GetSyncID()); }));
}



CObject::CObject() : detached(false)
{
	// Note1: this static var is shared between all different types of classes synced & unsynced (CUnit, CFeature, CProjectile, ...)
	//  Still it doesn't break syncness even when synced objects have different sync_ids between clients as long as the sync_id is
	//  creation time dependent and monotonously increasing, so the _order_ remains between clients.

	// Use atomic fetch-and-add, so threads don't read half written data nor write old (= smaller) numbers
	sync_id = ++cur_sync_id;

	assert((sync_id + 1) > sync_id); // check for overflow

	std::fill(listenersDepTbl.begin(), listenersDepTbl.end(), INVALID_DEP_INDX);
	std::fill(listeningDepTbl.begin(), listeningDepTbl.end(), INVALID_DEP_INDX);
}


CObject::~CObject()
{
	assert(!detached);
	detached = true;

	for (size_t i = 0; i < listenersDepTbl.size(); ++i) {
		const size_t& idx1 = listenersDepTbl[i];

		if (idx1 == INVALID_DEP_INDX)
			continue;

		assert(idx1 < listeners.size());

		for (CObject* obj: listeners[idx1]) {
			obj->DependentDied(this);

			const size_t& idx2 = obj->listeningDepTbl[i];

			if (idx2 == INVALID_DEP_INDX)
				continue;

			VectorEraseSorted(obj->listening[idx2], this);
		}
	}

	for (size_t i = 0; i < listeningDepTbl.size(); ++i) {
		const size_t& idx1 = listeningDepTbl[i];

		if (idx1 == INVALID_DEP_INDX)
			continue;

		assert(idx1 < listening.size());

		for (CObject* obj: listening[idx1]) {
			const size_t& idx2 = obj->listenersDepTbl[i];

			if (idx2 == INVALID_DEP_INDX)
				continue;

			VectorEraseSorted(obj->listeners[idx2], this);
		}
	}
}


// NOTE:
//   we can be listening to a single object from several different places
//   objects are responsible for not adding the same dependence more than
//   once, and preferably try to delete the dependence ASAP in order not
//   to waste memory
void CObject::AddDeathDependence(CObject* obj, DependenceType dep)
{
	assert(!detached && !obj->detached);

	// check this explicitly
	if (detached || obj->detached)
		return;

	VectorInsertSorted(const_cast<TSyncSafeSet&>(     GetListening(dep)),  obj);
	VectorInsertSorted(const_cast<TSyncSafeSet&>(obj->GetListeners(dep)), this);
}


void CObject::DeleteDeathDependence(CObject* obj, DependenceType dep)
{
	assert(dep >= DependenceType::DEPENDENCE_ATTACKER && dep < DependenceType::DEPENDENCE_COUNT);
	assert(!detached);

	if (detached || obj->detached)
		return;

	if (const size_t& idx =      listeningDepTbl[dep]; idx != INVALID_DEP_INDX) VectorEraseSorted(     listening[idx],  obj);
	if (const size_t& idx = obj->listenersDepTbl[dep]; idx != INVALID_DEP_INDX) VectorEraseSorted(obj->listeners[idx], this);
}

