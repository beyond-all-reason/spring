#pragma once

class float3;

class CModelDrawerHelper {
public:
	virtual void PushRenderState() const = 0;
	virtual void PopRenderState() const = 0;
public:
	// Auxilary
	static bool ObjectVisibleReflection(const float3& objPos, const float3& camPos, float maxRadius);
};