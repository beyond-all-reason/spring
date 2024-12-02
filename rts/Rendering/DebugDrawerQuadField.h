/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef DEBUG_DRAWER_QUAD_FIELD
#define DEBUG_DRAWER_QUAD_FIELD

#include "System/EventClient.h"

#include "System/float4.h"

class DebugDrawerQuadField : public CEventClient
{
public:
	// CEventClient interface
	virtual void DrawInMiniMapBackground() override;
	virtual void DrawWorldPreUnit() override;

	static void SetEnabled(bool enable);
	static bool IsEnabled() { return (instance != nullptr); }

private:
	DebugDrawerQuadField();

	float3 TraceToMaxHeight(float3 start, float3 point, float length) const;
	void DrawQuad(unsigned i, float4 color) const;
	void DrawRect(float3 pos, float widthX, float widthZ, float4 color) const;
	void DrawSelectionQuads() const;
	void DrawMouseRayQuads() const;
	void DrawCamera() const;
	void DrawAll() const;

	static DebugDrawerQuadField* instance;
};

#endif // DEBUG_DRAWER_QUAD_FIELD
