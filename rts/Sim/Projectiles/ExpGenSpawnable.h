/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef EXP_GEN_SPAWNABLE_H
#define EXP_GEN_SPAWNABLE_H

#include <memory>

#include "Sim/Objects/WorldObject.h"
#include "System/Threading/ThreadPool.h"
#include "Rendering/GL/RenderBuffersFwd.h"

struct SExpGenSpawnableMemberInfo;
class CUnit;

class CExpGenSpawnable : public CWorldObject
{
	CR_DECLARE(CExpGenSpawnable)
public:
	CExpGenSpawnable(const float3& pos, const float3& spd);

	virtual ~CExpGenSpawnable();
	virtual void Init(const CUnit* owner, const float3& offset) = 0;

	static bool GetSpawnableMemberInfo(const std::string& spawnableName, SExpGenSpawnableMemberInfo& memberInfo);
	static int GetSpawnableID(const std::string& spawnableName);

	//Memory handled in projectileHandler
	static CExpGenSpawnable* CreateSpawnable(int spawnableID);
private:
	static std::array<std::unique_ptr<TypedRenderBuffer<VA_TYPE_TC>>, ThreadPool::MAX_THREADS> rbs; //per thread
protected:
	TypedRenderBuffer<VA_TYPE_TC>& GetThreadRenderBuffer();

	CExpGenSpawnable();
	static bool GetMemberInfo(SExpGenSpawnableMemberInfo& memberInfo);
public:
	static TypedRenderBuffer<VA_TYPE_TC>& GetMainThreadRenderBuffer();
	static TypedRenderBuffer<VA_TYPE_TC>& GetJointRenderBuffer();
};

#endif //EXP_GEN_SPAWNABLE_H
