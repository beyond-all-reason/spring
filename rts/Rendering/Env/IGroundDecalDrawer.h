/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "Decals/GroundDecal.h"

class CSolidObject;
class GhostSolidObject;

class IGroundDecalDrawer
{
public:
	static bool GetDrawDecals() { return (decalLevel > 0); }
	static void SetDrawDecals(bool v);

	static void Init();
	static void FreeInstance();
	static IGroundDecalDrawer* singleton;

public:
	virtual void ReloadTextures() = 0;
	virtual void DumpAtlasTextures() = 0;

	virtual void Draw() = 0;

	virtual size_t CreateLuaDecal() = 0;
	virtual GroundDecal* GetDecalByIdx(size_t idx) = 0;

	const std::vector<GroundDecal>& GetPermanentDecals() const { return permanentDecals; }
	const std::vector<GroundDecal>& GetTemporaryDecals() const { return temporaryDecals; }

	virtual void AddSolidObject(const CSolidObject* object) = 0;
	virtual void ForceRemoveSolidObject(const CSolidObject* object) = 0;

	//FIXME move to eventhandler?
	virtual void GhostDestroyed(const GhostSolidObject* gb) = 0;
	virtual void GhostCreated(const CSolidObject* object, const GhostSolidObject* gb) = 0;

public:
	virtual ~IGroundDecalDrawer() {}

protected:
	virtual void OnDecalLevelChanged() = 0;

protected:
	std::vector<GroundDecal> permanentDecals;
	std::vector<GroundDecal> temporaryDecals;
	static int decalLevel;
};



class NullGroundDecalDrawer: public IGroundDecalDrawer
{
public:
	void ReloadTextures() override {}
	void DumpAtlasTextures() override {}

	void Draw() override {}

	void AddSolidObject(const CSolidObject* object) override {}
	void ForceRemoveSolidObject(const CSolidObject* object) override {}

	void GhostDestroyed(const GhostSolidObject* gb) override {}
	void GhostCreated(const CSolidObject* object, const GhostSolidObject* gb) override {}

	void OnDecalLevelChanged() override {}

	size_t CreateLuaDecal() override { return 0; };
	GroundDecal* GetDecalByIdx(size_t idx) override { return nullptr; }
};


#define groundDecals IGroundDecalDrawer::singleton