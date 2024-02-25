/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "ResourceHandler.h"
#include "ResourceMapAnalyzer.h"

#include <algorithm>
#include <cfloat>


CR_BIND(CResourceHandler, )
CR_REG_METADATA(CResourceHandler, (
	CR_IGNORED(resourceDescriptions),
	CR_IGNORED(resourceMapAnalyzer),
	CR_MEMBER(metalResourceId),
	CR_MEMBER(energyResourceId),

	CR_POSTLOAD(PostLoad)
))


static CResourceHandler instance;


CResourceHandler* CResourceHandler::GetInstance() { return &instance; }

void CResourceHandler::CreateInstance() { instance.Init(); }
void CResourceHandler::FreeInstance() { instance.Kill(); }


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

void CResourceHandler::AddResources() {
	resourceDescriptions.clear();
	resourceDescriptions.reserve(SResourcePack::MAX_RESOURCES);
	resourceMapAnalyzer.reset();

	CResourceDescription rMetal;
	rMetal.name = "Metal";
	rMetal.optimum = FLT_MAX;
	metalResourceId = AddResource(rMetal);

	CResourceDescription rEnergy;
	rEnergy.name = "Energy";
	rEnergy.optimum = FLT_MAX;
	energyResourceId = AddResource(rEnergy);
}

int CResourceHandler::AddResource(const CResourceDescription& resource)
{
	// GetResourceMapAnalyzer returns a pointer, no resizing allowed
	assert(resourceDescriptions.size() < SResourcePack::MAX_RESOURCES);

	resourceDescriptions.push_back(resource);

	/* I'm not sure whether this needs to wait until a resource description exists.
	 * Perhaps it could be created unconditionally earlier, since the map should
	 * contain everything needed regardless. Keep in mind object lifetime issues
	 * when dealing with things like save/load, or reloading a different map tho. */
	if (!resourceMapAnalyzer)
		resourceMapAnalyzer.emplace();

	return (resourceDescriptions.size() - 1);
}


const CResourceDescription* CResourceHandler::GetResource(int resourceId) const
{
	if (IsValidId(resourceId))
		return &resourceDescriptions[resourceId];

	return nullptr;
}

const CResourceDescription* CResourceHandler::GetResourceByName(const std::string& resourceName) const
{
	return GetResource(GetResourceId(resourceName));
}

int CResourceHandler::GetResourceId(const std::string& resourceName) const
{
	const auto pred = [&](const CResourceDescription& rd) { return (resourceName == rd.name); };
	const auto iter = std::find_if(resourceDescriptions.cbegin(), resourceDescriptions.cend(), pred);
	return ((iter == resourceDescriptions.end())? -1: (iter - resourceDescriptions.cbegin()));
}

const CResourceMapAnalyzer* CResourceHandler::GetResourceMapAnalyzer()
{
	CResourceMapAnalyzer* rma = &resourceMapAnalyzer.value();

	if (rma->GetNumSpots() < 0)
		rma->Init();

	return rma;
}

