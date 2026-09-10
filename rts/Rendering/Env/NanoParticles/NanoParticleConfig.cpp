/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "NanoParticleConfig.h"

#include <algorithm>

#include "System/Config/ConfigHandler.h"

/*
 * The player-facing switches. Everything else is a modrule; see
 * NanoParticleConfig.h for why the split is drawn here.
 */
CONFIG(bool, NanoParticlesGL4)
	.defaultValue(false)
	.safemodeValue(false)
	.headlessValue(false)
	.description("Render nano particles as shader-generated 3D shapes instead of textured billboards");
CONFIG(bool, NanoParticlesNoGeometryShader)
	.defaultValue(false)
	.safemodeValue(false)
	.headlessValue(false)
	.description("Force the instanced no-geometry-shader nano particle renderer");
CONFIG(bool, NanoParticlesHoming)
	.defaultValue(true)
	.safemodeValue(false)
	.headlessValue(false)
	.description("Allow nano particles to follow moving unit targets and builder nano pieces");
CONFIG(bool, NanoParticlesGroundClamp)
	.defaultValue(true)
	.safemodeValue(false)
	.headlessValue(false)
	.description("Route nano particles above intervening terrain");
CONFIG(bool, NanoParticlesReclaimBurst)
	.defaultValue(false)
	.safemodeValue(false)
	.headlessValue(false)
	.description("Emit a nano burst when reclaiming a unit finishes");
CONFIG(float, NanoParticlesRate)
	.defaultValue(0.32f)
	.minimumValue(0.0f)
	.maximumValue(1.0f)
	.description("Per-emitter nano emission multiplier; also scales the minimum visual feedback cadence");
CONFIG(bool, NanoParticlesTargetLostFade)
	.defaultValue(true)
	.description("Nano particles fade out and shrink when the unit they were aimed at is destroyed, cancelled, or finished, instead of flying on into nothing");
CONFIG(bool, NanoParticlesUpdateLuaUI)
	.defaultValue(false)
	.safemodeValue(false)
	.headlessValue(false)
	.description("Send batched nano particle lifecycle updates to LuaUI");
CONFIG(float, NanoParticlesUpdateLuaUISampleRate)
	.defaultValue(0.25f)
	.minimumValue(0.0f)
	.maximumValue(1.0f)
	.description("Fraction multiplier for nano particles reported to LuaUI (used for deferred lights)");

namespace NanoParticles {

namespace {
	Config config;

	const std::vector<std::string> observedConfigKeys = {
		"NanoParticlesGL4",
		"NanoParticlesNoGeometryShader",
		"NanoParticlesHoming",
		"NanoParticlesGroundClamp",
		"NanoParticlesReclaimBurst",
		"NanoParticlesRate",
		"NanoParticlesTargetLostFade",
		"NanoParticlesUpdateLuaUI",
		"NanoParticlesUpdateLuaUISampleRate",
	};

} // namespace

const Config& GetConfig() { return config; }

const std::vector<std::string>& GetObservedConfigKeys() { return observedConfigKeys; }

void InitConfig()
{
	config.enabled               = configHandler->GetBool("NanoParticlesGL4");
	config.forceNoGeometryShader = configHandler->GetBool("NanoParticlesNoGeometryShader");
	config.homing                = configHandler->GetBool("NanoParticlesHoming");
	config.groundClamp           = configHandler->GetBool("NanoParticlesGroundClamp");
	config.reclaimBurst          = configHandler->GetBool("NanoParticlesReclaimBurst");
	config.luaUpdates            = configHandler->GetBool("NanoParticlesUpdateLuaUI");
	config.rate                  = std::clamp(configHandler->GetFloat("NanoParticlesRate"), 0.0f, 1.0f);
	config.targetLostFade        = configHandler->GetBool("NanoParticlesTargetLostFade");
	config.luaUpdate.sampleRate  = std::clamp(configHandler->GetFloat("NanoParticlesUpdateLuaUISampleRate"), 0.0f, 1.0f);

	++config.generation;
}

bool ReloadConfigSetting(const std::string& key)
{
	const Config previous = config;

	if (std::find(observedConfigKeys.begin(), observedConfigKeys.end(), key) == observedConfigKeys.end())
		return false;

	InitConfig();

	return previous.enabled != config.enabled
		|| previous.forceNoGeometryShader != config.forceNoGeometryShader
		|| previous.homing != config.homing
		|| previous.groundClamp != config.groundClamp
		|| previous.reclaimBurst != config.reclaimBurst
		|| previous.luaUpdates != config.luaUpdates
		|| previous.rate != config.rate
		|| previous.targetLostFade != config.targetLostFade
		|| previous.luaUpdate.sampleRate != config.luaUpdate.sampleRate;
}

} // namespace NanoParticles
