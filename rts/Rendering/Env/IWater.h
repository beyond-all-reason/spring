/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef I_WATER_H
#define I_WATER_H

#include "Sim/Projectiles/ExplosionListener.h"
#include "System/float3.h"

#include <array>
class CGame;

class IWater : public IExplosionListener {
public:
	enum WATER_RENDERER {
		WATER_RENDERER_BASIC = 0,
		WATER_RENDERER_REFLECTIVE = 1,
		WATER_RENDERER_DYNAMIC = 2,
		WATER_RENDERER_REFL_REFR = 3,
		WATER_RENDERER_BUMPMAPPED = 4,
		NUM_WATER_RENDERERS = 5,
	};

	IWater();
	virtual ~IWater() = default;
	virtual void InitResources(bool loadShader = true) = 0;
	virtual void FreeResources() = 0;

	virtual void Draw() {}

	virtual void Update() {}

	virtual void UpdateWater(const CGame* game) {}

	virtual void AddExplosion(const float3& pos, float strength, float size) {}

	virtual WATER_RENDERER GetID() const = 0;

	virtual bool CanDrawReflectionPass() const { return false; }

	virtual bool CanDrawRefractionPass() const { return false; }

	void ExplosionOccurred(const CExplosionParams& event) override;

	bool DrawReflectionPass() const { return drawReflection; }

	bool DrawRefractionPass() const { return drawRefraction; }

	bool BlockWakeProjectiles() const { return (GetID() == WATER_RENDERER_DYNAMIC); }

	bool& WireFrameModeRef() { return wireFrameMode; }

	static void SetModelClippingPlane(const double* planeEq);

	static void SetWater(int rendererMode); // not enum on purpose, -1 means cycling

	static auto& GetWater() { return water; }

	static void KillWater() { water = nullptr; }

	static const char* GetWaterName(WATER_RENDERER wr) { return WaterNames[wr]; }

protected:
	void DrawReflections(const double* clipPlaneEqs, bool drawGround, bool drawSky);
	void DrawRefractions(const double* clipPlaneEqs, bool drawGround, bool drawSky);

protected:
	static inline std::unique_ptr<IWater> water = nullptr;

	bool drawReflection;
	bool drawRefraction;
	bool wireFrameMode;

private:
	static constexpr std::array<const char*, NUM_WATER_RENDERERS> WaterNames{
	    "basic", "reflective", "dynamic", "reflective&refractive", "bumpmapped"};
};

#endif // I_WATER_H
