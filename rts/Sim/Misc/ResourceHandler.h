/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _RESOURCEHANDLER_H
#define _RESOURCEHANDLER_H

#include <optional>
#include <vector>

#include "System/Misc/NonCopyable.h"
#include "System/creg/creg_cond.h"
#include "Resource.h"
#include "ResourceMapAnalyzer.h"


class CResourceHandler : public spring::noncopyable
{
	CR_DECLARE_STRUCT(CResourceHandler)

public:
	static CResourceHandler* GetInstance();

	static void CreateInstance();
	static void FreeInstance();

	void Init() { AddResources(); }
	void Kill() {
		resourceDescriptions.clear();
		resourceMapAnalyzer.reset();
	}

	void PostLoad() { AddResources(); }
	void AddResources();

	/**
	 * @brief	add a resource
	 * @param	resource the resource to add
	 * @return	the id of the resource just added
	 *
	 * Adds a CResource to the pool and retun its resourceId.
	 */
	int AddResource(const CResourceDescription& resource);
	/**
	 * @brief	resource
	 * @param	resourceId index to fetch
	 * @return	the searched resource
	 *
	 * Accesses a CResource instance at a given index
	 */
	const CResourceDescription* GetResource(int resourceId) const;
	/**
	 * @brief	resource by name
	 * @param	resourceName name of the resource to fetch
	 * @return	the searched resource
	 *
	 * Accesses a CResource instance by name
	 */
	const CResourceDescription* GetResourceByName(const std::string& resourceName) const;
	/**
	 * @brief	resource index by name
	 * @param	resourceName name of the resource to fetch
	 * @return	index of the searched resource
	 *
	 * Accesses a resource by name
	 */
	int GetResourceId(const std::string& resourceName) const;

	/**
	 * @brief	resource map analyzer
	 * @return	resource map analyzer
	 *
	 * Returns the resource map analyzer.
	 */
	const CResourceMapAnalyzer* GetResourceMapAnalyzer();

	size_t GetNumResources() const { return resourceDescriptions.size(); }

	int GetMetalId() const { return metalResourceId; }
	int GetEnergyId() const { return energyResourceId; }

	bool IsValidId(int resourceId) const { return (static_cast<size_t>(resourceId) < GetNumResources()); }

private:
	std::vector<CResourceDescription> resourceDescriptions;
	std::optional<CResourceMapAnalyzer> resourceMapAnalyzer;

	int metalResourceId = -1;
	int energyResourceId = -1;
};

#define resourceHandler CResourceHandler::GetInstance()

#endif // _RESOURCEHANDLER_H
