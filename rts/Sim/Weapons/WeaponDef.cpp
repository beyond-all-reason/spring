/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#include "WeaponDef.h"

#include "Game/TraceRay.h"
#include "Rendering/Models/IModelParser.h"
#include "Rendering/Textures/ColorMap.h"
#include "Sim/Misc/DamageArrayHandler.h"
#include "Sim/Misc/DefinitionTag.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Misc/ModInfo.h"
#include "Sim/Projectiles/ExplosionGenerator.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectileTypes.h"
#include "Sim/Units/Scripts/CobInstance.h" // TAANG2RAD
#include "System/SpringMath.h"
#include "System/Log/ILog.h"
#include "System/StringHash.h"
#include "System/StringUtil.h"

#include "System/Misc/TracyDefs.h"


static DefType WeaponDefs("WeaponDefs");

#define WEAPONTAG(T, name, ...) DEFTAG(WeaponDefs, WeaponDef, T, name, ##__VA_ARGS__)
#define WEAPONDUMMYTAG(T, name) DUMMYTAG(WeaponDefs, T, name)

// General
WEAPONTAG(std::string, description).externalName("name").defaultValue("Weapon").description("The descriptive name of the weapon, for GUI purposes.");
WEAPONTAG(std::string, type).externalName("weaponType").defaultValue("Cannon")
	.description("Sets weapon type, which is a bundle of behaviours and visuals (check other tags). "
	"Available types (sorted from the most general):\n"
	"Cannon - ballistic projectile, defaults to 'plasma ball' visuals\n"
	"LaserCannon - non-ballistic projectile, defaults to 'slow laser' visuals (think stormtroopers)\n"
	"BeamLaser - hitscan weapon, laser visuals\n"
	"MissileLauncher - potentially homing projectile, leaves a smoke trail\n"
	"TorpedoLauncher - missile that can't exit water by default\n"
	"StarburstLauncher - missile with a vertical ascent phase at the beginning\n"
	"AircraftBomb - resolves into Torpedo or Cannon, but has support for being dropped by a bomber plane\n"
	"Flame - non-ballistic projectile, has a sprite which expands\n"
	"LightningCannon - hitscan weapon, lightning visuals\n"
	"Melee - just applies damage. No frontswing though\n"
	"DGun - deprecated, a fiery ball projectile. Has a lot of hardcoded visuals but does NOT convey 'dgun' mechanics. Prefer Cannon instead\n"
	"EmgCannon - deprecated, a version of Laser or Flame with crappy visuals\n"
	"Rifle - deprecated, more or less equivalent to invisible Lightning\n"
);
WEAPONDUMMYTAG(table, customParams).description("A table of arbitrary string key-value pairs, for use by Lua gadgets (no engine meaning)");

// Collision & Avoidance
WEAPONTAG(bool, avoidFriendly).defaultValue(true).description("Does the weapon avoid shooting if there's allies in the way? Note that an ally can run into the projectile and it will still explode - use `collideFriendly` to avoid that");
WEAPONTAG(bool, avoidFeature).defaultValue(true).description("Does the weapon avoid shooting if there's features in the way? See remarks at `avoidFriendly`");
WEAPONTAG(bool, avoidNeutral).defaultValue(false).description("Does the weapon avoid shooting if there's neutrals in the way? Note this does not mean Gaia! See also remarks at `avoidFriendly`");
WEAPONTAG(bool, avoidGround).defaultValue(true).description("Does the weapon avoid shooting if terrain would block it? See remarks at `avoidFriendly`");
WEAPONTAG(bool, avoidCloaked).defaultValue(false).description("Does the weapon avoid shooting if there's cloaked (incl. revealed, but not decloaked) units in the way? See remarks at `avoidFriendly`");
WEAPONDUMMYTAG(bool, collideEnemy).defaultValue(true).description("Does the projectile collide with enemies? Use to make sure it hits the ground, or for buffs targeting allies. Note that targeting will always target enemies anyway, and never allies. Also note there is no corresponding `avoidEnemy`");
WEAPONDUMMYTAG(bool, collideFriendly).defaultValue(true).description("Does the projectile collide with allies? Note that the unit will still shoot if there are allies in the way, which is controlled separately via `avoidFriendly`.");
WEAPONDUMMYTAG(bool, collideFeature).defaultValue(true).description("Does the projectile collide with features? See remarks at `collideFriendly`");
WEAPONDUMMYTAG(bool, collideNeutral).defaultValue(true).description("Does the projectile collide with neutrals? Note this does not mean Gaia! See also remarks at `collideFriendly`");
WEAPONDUMMYTAG(bool, collideFireBase).defaultValue(false).description("Does the projectile collide with its firebase, i.e. a transport holding the unit? Put it on marines' weapons to let them shoot out of a bunker while remaining in its colvol. There is no corresponding `avoidFirebase`.");
WEAPONDUMMYTAG(bool, collideNonTarget).defaultValue(true).description("Does the projectile ghost through everything that isn't its target (incl. other enemies)? Combine with `tracks` and `impactOnly` for 'starcraft' style weapons that are largely just graphics. There is no corresponding `avoidNonTarget`.");
WEAPONDUMMYTAG(bool, collideGround).defaultValue(true).description("Does the projectile collide with terrain? See remarks at `collideFriendly`");
WEAPONDUMMYTAG(bool, collideCloaked).defaultValue(true).description("Does the projectile collide with cloaked (includes revealed but not decloaked) units? See remarks at `collideFriendly`");

// Damaging
WEAPONDUMMYTAG(table, damage).description("Damage table, indexed by armor class name");
WEAPONDUMMYTAG(float, damage.default).defaultValue(1.0f).description("The default damage used in absence of explicit per-armorclass value");
WEAPONDUMMYTAG(float, explosionSpeed).description("How fast does the shockwave propagate? Note that units cannot actually dodge the shockwave (they are tagged immediately and just damaged after a delay)");
WEAPONTAG(bool, impactOnly).defaultValue(false).description("Does the projectile only damage a single thing it hits? Mostly equivalent to having 0 AoE without the issues with 0. Also removes cratering.");
WEAPONTAG(bool, noSelfDamage).defaultValue(false).description("Is the unit unable to damage itself with the weapon?");
WEAPONTAG(bool, noExplode).defaultValue(false).description("The projectile will not be removed when exploding and instead continue on. It will keep exploding every sim frame while inside a collision volume, massively multiplying nominal damage.");
WEAPONTAG(bool, selfExplode).externalName("burnblow").defaultValue(false).description("For LaserCannon, expire when reaching the target (at max range otherwise). For Cannon, explode when reaching the target (keep falling otherwise). For Missile/Starburst/TorpedoLauncher, explode when running out of fuel (fall down otherwise).");
WEAPONTAG(float, damageAreaOfEffect, damages.damageAreaOfEffect).fallbackName("areaOfEffect").defaultValue(8.0f).scaleValue(0.5f).description("The diameter (not radius!) for damage. Cratering controlled separately. Also the collision radius for projectile-based interceptors.");
WEAPONTAG(float, edgeEffectiveness, damages.edgeEffectiveness).defaultValue(0.0f).maximumValue(1.0f).description("Exponent for a magic formula describing splash damage falloff. The damage always drops down to 0, this tag just controls how large the 'core' is. Can be negative for a very core-centric explosion. 0 is linear falloff with radius. 1 is no falloff.");
WEAPONTAG(float, collisionSize).defaultValue(0.05f).description("Width for hitscan interceptors. Supposed to be collision radius for others but it's broken at the moment");

// Projectile Properties
WEAPONTAG(float, projectilespeed).externalName("weaponVelocity").fallbackName("maxVelocity").defaultValue(0.0f).minimumValue(0.01f).scaleValue(INV_GAME_SPEED).description("Maximum speed in elmo/s (won't accelerate further on its own)");
WEAPONTAG(float, startvelocity).defaultValue(0.0f).minimumValue(0.01f).scaleValue(INV_GAME_SPEED).description("Initial projectile speed in elmo/s");
WEAPONTAG(float, weaponacceleration).fallbackName("acceleration").defaultValue(0.0f).scaleValue(INV_GAME_SPEED * INV_GAME_SPEED).description("Acceleration in elmo/s^2");
WEAPONTAG(float, reload).externalName("reloadTime").defaultValue(1.0f).description("Reload time between bursts, in seconds. Note that reloadTime starts to count down from the first round fired, not the last, so if (reloadTime < burst * burstRate) the weapon will fire continuously.");
WEAPONDUMMYTAG(float, salvoWindup).externalName("windup").description("Delay between firing and the first shot");
WEAPONTAG(float, salvodelay).externalName("burstRate").defaultValue(0.1f).description("Delay between shots within a burst, in seconds");
WEAPONTAG(int, salvosize).externalName("burst").defaultValue(1).description("Shots per burst. Cannot be used by #BeamLaser unless `beamburst` is used which comes with caveats.");
WEAPONTAG(int, projectilespershot).externalName("projectiles").defaultValue(1).description("Projectiles per shot. Best used in conjunction with `sprayAngle` or changing the firing piece in script using ShotX as otherwise they'll all be clumped up in one blob.");

// Bounce
WEAPONTAG(bool, waterBounce).defaultValue(false).description("Bounces when hitting the water surface?");
WEAPONTAG(bool, groundBounce).defaultValue(false).description("Bounces when hitting terrain?");
WEAPONTAG(float, bounceSlip).defaultValue(1.0f).description("Horizontal velocity multiplier on bounce");
WEAPONTAG(float, bounceRebound).defaultValue(1.0f).description("Vertical velocity multiplier on bounce");
WEAPONTAG(int, numBounce).defaultValue(-1).description("How many bounces can the weapon do? Explodes on impact when cannot bounce anymore");

// Crater & Impulse
WEAPONTAG(float, impulseFactor, damages.impulseFactor).defaultValue(1.0f).description("A multiplier to base impulse (knockback). For most weapons, base impulse is equal to applied damage. For #Melee weapons the base impulse is the hitting unit's mass.");
WEAPONTAG(float, impulseBoost, damages.impulseBoost).defaultValue(0.0f).description("A flat bonus to impulse.");
WEAPONTAG(float, craterMult, damages.craterMult).fallbackName("impulseFactor").defaultValue(1.0f).description("A multiplier to cratering strength. Applies after all other modifiers.");
WEAPONTAG(float, craterBoost, damages.craterBoost).defaultValue(0.0f).description("A flat modifier to cratering strength, applies second-last (after reduction due to altitude).");
WEAPONTAG(float, craterAreaOfEffect, damages.craterAreaOfEffect).fallbackName("areaOfEffect").defaultValue(8.0f).scaleValue(0.5f).description("Diameter of terrain deformation. Damage to units controlled separately. Keep in mind about the inner half of this is the hole, and the outer half is the raised (!) rim");

// Water
WEAPONTAG(bool, waterweapon).defaultValue(false).description("Can the projectile travel underwater? Ability to fire underwater controlled separately via `fireSubmersed`");
WEAPONTAG(bool, submissile).defaultValue(false).description("Torpedo only. Lets torpedoes exit the water and be a missile. Lets underwater launchers shoot out-of-water targets (out-of-water launchers still cannot - use Missile instead of Torpedo for that).");
WEAPONTAG(bool, fireSubmersed).fallbackName("waterweapon").defaultValue(false).description("Can the weapon fire underwater? Requires `waterweapon`.");

// Targeting
WEAPONTAG(bool, manualfire).externalName("commandfire").defaultValue(false).description("Does the weapon respond to the manual fire command instead of regular attack?");
WEAPONTAG(float, range).defaultValue(10.0f).description("Maximum targeting range. Ballistic weapons can resolve lower due to physics. Some weapons can also fly past that range if they miss.");
WEAPONTAG(float, heightmod).defaultValue(0.2f).description("Multiplies height difference to target, for targeting purposes. When lower than 1, the targeting volume becomes elongated vertically and the unit can target further high than normal (useful to make terrain and aircraft less punishing). At 0, the height difference component becomes completely ignored.");
WEAPONTAG(float, targetBorder).defaultValue(0.0f).minimumValue(-1.0f).maximumValue(1.0f).description("1/-1 will target the close/far edge of the colvol (instead of center). Matters for huge colvols and/or small ranges");
WEAPONTAG(float, cylinderTargeting).fallbackName("cylinderTargetting").defaultValue(0.0f).minimumValue(0.0f).maximumValue(128.0f).description("Makes targeting happen in a cylinder. The height is range times this value. Zero means use the usual range (spherical or ballistic)");
WEAPONTAG(bool, turret).defaultValue(false).description("Does the unit aim within an arc (up-to and including full 360° turret traverse) or always aim along the owner's heading?");
WEAPONTAG(bool, fixedLauncher).defaultValue(false).description("Missile/Torpedo/Starburst only. The projectile will start aimed at the direction of the aiming piece, which is not necessarily towards the target");
WEAPONTAG(float, maxAngle).externalName("tolerance").defaultValue(3000.0f).scaleValue(TAANG2RAD).description("For `turret = false` only. Firing cone width, in the 16-bit legacy angular unit.");
WEAPONDUMMYTAG(float, maxFireAngle).externalName("firetolerance").defaultValue(3640.0f).scaleValue(TAANG2RAD).description("Angle above which reaim (script `AimWeapon`) is forced outside the usual time-based reaim. In the legacy 16-bit angular units."); // default value is 20degree
WEAPONTAG(int, highTrajectory).defaultValue(2).description("0: low trajectory, 1: high trajectory, 2: the unit will have a state toggle for the player to pick");
WEAPONTAG(float, trajectoryHeight).defaultValue(0.0f).description("Missile/Torpedo only. Causes the missile to fly in an arc. The value is the fraction of target distance as extra arc height (e.g. at 1.0 the arc is as tall as it is long).");
WEAPONTAG(bool, tracks).defaultValue(false).description("Missile/Torpedo/Starburst only. Does the projectile track its target (i.e. homing)? Requires a positive `turnRate`.");
WEAPONTAG(float, wobble).defaultValue(0.0f).scaleValue(TAANG2RAD * INV_GAME_SPEED).description("Missile only. Missiles will turn towards random directions (new direction rolled every 16 sim frames). In legacy angular units per second.");
WEAPONTAG(float, dance).defaultValue(0.0f).scaleValue(1.0f / GAME_SPEED).description("Missile only. Missiles will randomly shift up to this many elmos, perpendicular to their movement direction. Movement period is hardcoded to 8 sim frames");
WEAPONTAG(bool, gravityAffected).defaultValue(false).description("#DGun weapon type only. Is the dgun projectile affected by gravity? Aiming won't take this into account.");
WEAPONTAG(float, myGravity).defaultValue(0.0f).description("Overrides the map gravity for ballistic weapons and missiles. Missiles only affected once flightTime expired. The default of 0.0 disables the tag in favour of map gravity.");
WEAPONTAG(bool, canAttackGround).defaultValue(true).description("Can the unit target ground? Only units otherwise. Note, features are not directly targetable either way.");
WEAPONTAG(float, uptime).externalName("weaponTimer").defaultValue(0.0f).description("StarburstLauncher only. Seconds of vertical ascent");
WEAPONDUMMYTAG(float, flighttime).defaultValue(0).scaleValue(GAME_SPEED).description("Lifetime of the projectile, in seconds. Missile/Torpedo/Starburst projectiles 'lose fuel' and fall down; Cannons explode; others fade away"); // needs to be written as int and read as float
WEAPONTAG(float, turnrate).defaultValue(0.0f).scaleValue(TAANG2RAD * INV_GAME_SPEED).description("For projectiles with `tracks`, in COB angular units (65536 is tau) per second. Also the turn rate for Starburst when they stop ascending and turn towards target (regardless of homing).");
WEAPONTAG(float, heightBoostFactor).defaultValue(-1.0f).description("#Cannon weapon type only. Controls the ballistic range gain/loss for height difference; larger means higher effect of range difference. -1 is derived some magic formula. Hard to tell how this stacks with `heightMod`.");
WEAPONTAG(float, proximityPriority).defaultValue(1.0f).description("Importance of distance when picking targets. Higher means closer units are preferred more; negative values make weapons prefer distant targets.");
WEAPONTAG(bool, allowNonBlockingAim).defaultValue(false).description("When false, the weapon is blocked from firing until AimWeapon() returns.");

// Target Error
TAGFUNCTION(AccuracyToSin, float, math::sin(x * math::PI / 0xafff)) // should really be tan but TA seem to cap it somehow, should also be 7fff or ffff theoretically but neither seems good
WEAPONTAG(float, accuracy).defaultValue(0.0f).tagFunction(AccuracyToSin).description("How INaccurate are entire bursts? Larger is worse, 0 is perfect accuracy. This is an angle, so long range shots are always less likely to hit than close range given same target and inaccuracy. Can improve with unit XP.");
WEAPONTAG(float, sprayAngle).defaultValue(0.0f).tagFunction(AccuracyToSin).description("How inaccurate are individual projectiles in a burst?");
WEAPONTAG(float, movingAccuracy).fallbackName("accuracy").defaultValue(0.0f).tagFunction(AccuracyToSin).description("Same as `accuracy` but applies when the unit is moving.");
WEAPONTAG(float, targetMoveError).defaultValue(0.0f).description("Fraction of target speed per second added as a random error. E.g. if target moves at 50 elmo/s and targetMoveError is 0.5 then a random vector of length up to 25 will be added to the target position");
WEAPONTAG(float, leadLimit).defaultValue(-1.0f).description("Maximum distance in elmos the unit will lead a moving target. Less than zero is unlimited.");
WEAPONTAG(float, leadBonus).defaultValue(0.0f).description("If `leadLimit` is not unlimited, add this value multiplied by raw unit XP (not limXP) to the limit.");
WEAPONTAG(float, predictBoost).defaultValue(0.0f).description("How well the unit leads its targets. Between 0 and 1. At pb=0 it will over- or under-estimate target speed by between 0-2x its actual value. At pb=1 it will estimate speed perfectly. Keep in mind `leadLimit` can still make it undershoot.");
WEAPONDUMMYTAG(float, ownerExpAccWeight).description("How much does accuracy (but not sprayAngle!) improve with unit limXP? Multiplier for limXP which is then subtracted as a fraction (for example at limXP=0.4 and ownerExpAccWeight=2, the weapon only has 1-(0.4*2) = 20% of original inaccuracy.");

// Laser Stuff
WEAPONTAG(float, minIntensity).defaultValue(0.0f).description("BeamLaser only. The minimum percentage the weapon's damage can fall-off to over its range. Setting to 1.0 will disable fall off entirely. Unrelated to the visual-only `intensity`. Largely a duplicate of `dynDamageExp`.");
WEAPONTAG(float, duration).defaultValue(0.05f).description("#LaserCannon only. The visual-only length of the projectile as a fraction of the per-second projectile speed.");
WEAPONTAG(float, beamtime).defaultValue(1.0f).description("#BeamLaser only. The laser maintains it beam for this many seconds, spreading its damage over that time.");
WEAPONTAG(bool, beamburst).defaultValue(false).description("#BeamLaser only. Lets a laser use burst mechanics, but sets `beamtime` to the duration of 1 sim frame.");
WEAPONTAG(int, beamLaserTTL).externalName("beamTTL").defaultValue(0).description("BeamLaser and LightningCannon only. Linger time of the visual sprite, in sim frames.");
WEAPONTAG(bool, sweepFire).defaultValue(false).description("Makes BeamLasers continue firing while aiming for a new target, 'sweeping' across the terrain.");
WEAPONTAG(bool, largeBeamLaser).defaultValue(false).description("BeamLaser only. Enables some extra fancy texturing (NOT size). Check other Beamlaser tags for 'large'.");

// FLAMETHROWER
WEAPONTAG(float, sizeGrowth).defaultValue(0.5f).description("#Flamethrower only. Visual-only radius growth in elmos per sim frame.");
WEAPONDUMMYTAG(float, flameGfxTime).description("#Flamethrower only. Multiplier of the total range for visuals purposes. For example at 1.2, the visual will extend 20% further than max range. Should be >= 1. Exposed back to WeaponDefs as `duration` (same as the unrelated #LaserCannon tag)");

// Eco
WEAPONDUMMYTAG(float,  metalPerShot).description( "Metal cost per shot. For stockpile weapons this is consumed over time, immediately on shot otherwise.");
WEAPONDUMMYTAG(float, energyPerShot).description("Energy cost per shot. For stockpile weapons this is consumed over time, immediately on shot otherwise.");

// Other Properties
WEAPONTAG(float, fireStarter).defaultValue(0.0f).minimumValue(0.0f).scaleValue(0.01f) // max value that makes engine sense is 100%, but Lua gadgets may make use of higher ones
	.description("The percentage chance of the weapon setting fire to static map features on impact.");
WEAPONTAG(bool, paralyzer).defaultValue(false).description("Is the weapon a paralyzer? If true the weapon only stuns enemy units and does not cause damage in the form of lost hit-points.");
WEAPONTAG(int, paralyzeTime,  damages.paralyzeDamageTime).defaultValue(10).minimumValue(0).description("Determines the maximum length of time in seconds that the target will be paralyzed. The timer is restarted every time the target is hit by the weapon. Cannot be less than 0.");
WEAPONTAG(bool, stockpile).defaultValue(false).description("Does each round of the weapon have to be built and stockpiled by the player? Will only correctly function for the first of each stockpiled weapons a unit has.");
WEAPONTAG(float, stockpileTime).fallbackName("reload").defaultValue(1.0f).scaleValue(GAME_SPEED).description("The time in seconds taken to stockpile one round of the weapon.");

// Interceptor
WEAPONTAG(int, targetable).defaultValue(0).description("Bitmask representing the types of weapon that can intercept this weapon. Each digit of binary that is set to one means that a weapon with the corresponding digit in its interceptor tag will intercept this weapon. Instant-hitting weapons such as [#BeamLaser], [#LightningCannon] and [#Rifle] cannot be targeted.");
WEAPONTAG(int, interceptor).defaultValue(0).description("Bitmask representing the types of weapons that this weapon can intercept. Each digit of binary that is set to one means that a weapon with the corresponding digit in its targetable tag will be intercepted by this weapon.");
WEAPONDUMMYTAG(unsigned, interceptedByShieldType).description("Bitmask representing the types of shields that this weapon can intercept. Each digit of binary that is set to one means that a shield with the corresponding digit in its shieldInterceptType tag will be hit by this weapon.");
WEAPONTAG(float, coverageRange).externalName("coverage").defaultValue(0.0f).description("The radius in elmos within which an interceptor weapon will fire on targetable weapons.");
WEAPONTAG(bool, interceptSolo).defaultValue(true).description("If true no other interceptors may target the same projectile.");

// Dynamic Damage
WEAPONTAG(bool, dynDamageInverted, damages.dynDamageInverted).defaultValue(false).description("If true the damage curve is inverted i.e. the weapon does more damage at greater ranges as opposed to less.");
WEAPONTAG(float, dynDamageExp, damages.dynDamageExp).defaultValue(0.0f).description("Exponent of the range-dependent damage formula, the default of 0.0 disables dynamic damage, 1.0 means linear scaling, 2.0 quadratic and so on.");
WEAPONTAG(float, dynDamageMin, damages.dynDamageMin).defaultValue(0.0f).description("The minimum floor value that range-dependent damage can drop to.");
WEAPONTAG(float, dynDamageRange, damages.dynDamageRange).defaultValue(0.0f).description("If set to non-zero values the weapon will use this value in the range-dependant damage formula instead of the actual range.");

// Shield
WEAPONTAG(bool, shieldRepulser).externalName("shield.repulser").fallbackName("shieldRepulser")
	.defaultValue(false).description("Does the shield repulse (deflect) projectiles or absorb them?");
WEAPONTAG(bool, smartShield).externalName("shield.smart").fallbackName("smartShield")
	.defaultValue(false).description("Determines whether or not projectiles fired by allied units can pass through the shield (true) or are intercepted as enemy weapons are (false).");
WEAPONTAG(bool, exteriorShield).externalName("shield.exterior").fallbackName("exteriorShield")
	.defaultValue(false).description("Determines whether or not projectiles fired within the shield's radius can pass through the shield (true) or are intercepted (false).");

WEAPONTAG(float, shieldMaxSpeed).externalName("shield.maxSpeed").fallbackName("shieldMaxSpeed")
	.defaultValue(0.0f).description("The maximum speed the repulsor will impart to deflected projectiles.");
WEAPONTAG(float, shieldForce).externalName("shield.force").fallbackName("shieldForce")
	.defaultValue(0.0f).description("The force applied by the repulsor to the weapon - higher values will deflect weapons away at higher velocities.");
WEAPONTAG(float, shieldRadius).externalName("shield.radius").fallbackName("shieldRadius")
	.defaultValue(0.0f).description("The radius of the circular area the shield covers.");
WEAPONTAG(float, shieldPower).externalName("shield.power").fallbackName("shieldPower")
	.defaultValue(0.0f).description("Essentially the maximum allowed hit-points of the shield - reduced by the damage of a weapon upon impact.");
WEAPONTAG(float, shieldStartingPower).externalName("shield.startingPower").fallbackName("shieldStartingPower")
	.defaultValue(0.0f).description("How many hit-points the shield starts with - otherwise the shield must regenerate from 0 until it reaches maximum power.");
WEAPONTAG(float, shieldPowerRegen).externalName("shield.powerRegen").fallbackName("shieldPowerRegen")
	.defaultValue(0.0f).description("How many hit-points the shield regenerates each second.");
WEAPONTAG(float, shieldPowerRegenEnergy).externalName("shield.powerRegenEnergy").fallbackName("shieldPowerRegenEnergy")
	.defaultValue(0.0f).description("How much energy resource is consumed to regenerate each hit-point.");
WEAPONTAG(float, shieldEnergyUse).externalName("shield.energyUse").fallbackName("shieldEnergyUse")
	.defaultValue(0.0f).description("The amount of the energy resource consumed by the shield to absorb or repulse weapons, continually drained by a repulsor as long as the projectile is in range.");
WEAPONDUMMYTAG(float, shieldRechargeDelay).externalName("rechargeDelay").fallbackName("shieldRechargeDelay").defaultValue(0)
	.scaleValue(GAME_SPEED).description("The delay in seconds before a shield begins to regenerate after it is hit."); // must be read as float
WEAPONTAG(unsigned int, shieldInterceptType).externalName("shield.interceptType").fallbackName("shieldInterceptType")
	.defaultValue(0).description("Bitmask representing the types of weapons that this shield can intercept. Each digit of binary that is set to one means that a weapon with the corresponding digit in its interceptedByShieldType will be intercepted by this shield (See [[Shield Interception Tag]] Use).");

WEAPONTAG(bool, visibleShield).externalName("shield.visible").fallbackName("visibleShield")
	.defaultValue(false).description("Is the shield visible or not?");
WEAPONTAG(bool, visibleShieldRepulse).externalName("shield.visibleRepulse").fallbackName("visibleShieldRepulse")
	.defaultValue(false).description("Is the (hard-coded) repulse effect rendered or not?");
WEAPONTAG(int, visibleShieldHitFrames).externalName("shield.visibleHitFrames").fallbackName("visibleShieldHitFrames")
	.defaultValue(0).description("The number of frames a shield becomes visible for when hit.");
WEAPONTAG(float4, shieldBadColor).externalName("shield.badColor").fallbackName("shieldBadColor")
	.defaultValue(float4(1.0f, 0.5f, 0.5f, 1.0f)).description("The RGBA colour the shield transitions to as its hit-points are reduced towards 0.");
WEAPONTAG(float4, shieldGoodColor).externalName("shield.goodColor").fallbackName("shieldGoodColor")
	.defaultValue(float4(0.5f, 0.5f, 1.0f, 1.0f)).description("The RGBA colour the shield transitions to as its hit-points are regenerated towards its maximum power.");
WEAPONTAG(float, shieldAlpha).externalName("shield.alpha").fallbackName("shieldAlpha")
	.defaultValue(0.2f).description("The alpha transparency of the shield whilst it is visible.");
WEAPONTAG(std::string, shieldArmorTypeName).externalName("shield.armorType").fallbackName("shieldArmorType")
	.defaultValue("default").description("Specifies the armorclass of the shield; you can input either an armorclass name OR a unitdef name to share that unit's armorclass");

// Unsynced (= Visuals)
WEAPONTAG(std::string, model, visuals.modelName).defaultValue("").description("Name of a 3D model. Otherwise uses 2D sprites");
WEAPONDUMMYTAG(float, size).description("Size of the 2D visual sprite, if the weapon has no 'model' set.");
WEAPONTAG(std::string, scarGlowColorMap, visuals.scarGlowColorMapStr).defaultValue("").description("A colormap (set of RGBA tuples) for the scar decal. Will smoothly fade between these colours over its lifetime.");
WEAPONDUMMYTAG(table, scarIndices).description("A table of indices to the scar table in resources.lua");
WEAPONTAG(bool, explosionScar, visuals.explosionScar).defaultValue(true).description("Does the explosion leave a scar decal on the ground?");
WEAPONTAG(float, scarDiameter, visuals.scarDiameter).defaultValue(-1.0f).description("Diameter of the scar decal");
WEAPONTAG(float, scarAlpha, visuals.scarAlpha).defaultValue(0.0f).description("Initial opacity of the scar decal (0-1)");
WEAPONTAG(float, scarGlow, visuals.scarGlow).defaultValue(0.0f).description("Initial glow intensity of the scar decal (0-1)");
WEAPONTAG(float, scarTtl, visuals.scarTtl).defaultValue(0.0f).description("Duration of the scar decal, in seconds");
WEAPONTAG(float, scarGlowTtl, visuals.scarGlowTtl).defaultValue(0.0f).description("Duration of the glow of the scar, in seconds");
WEAPONTAG(float, scarDotElimination, visuals.scarDotElimination).defaultValue(0.0f).description("Specifies the exponent of dot product of projection vector and current terrain normal vector, this is used to remove to remove scar projection from surfaces that usually should not receive the ground scar effect, the exponent controls the degree of elimination. The default 0.0 means there will be no elimination at all.");
WEAPONTAG(float4, scarProjVector, visuals.scarProjVector).defaultValue(float4{0.0f}).description("Forced direction of a ground scar projection (you can think of this technique as if the scar image was projected onto the ground from the cinema projector at certain world-space vector). If all zeroes, it will use ground normals. Realistically this should either be left at default, or set to {0, 1, 0} for orbital-type weapons");
WEAPONTAG(float4, scarColorTint, visuals.scarColorTint).defaultValue(float4{0.5f, 0.5f, 0.5f, 0.5f }).description("Color tint for explosion scar decal. Scaled so that 0.5 is no change, 1.0 is twice as bright.");
WEAPONTAG(bool, alwaysVisible, visuals.alwaysVisible).defaultValue(false).description("Is the projectile visible regardless of sight?");
WEAPONTAG(float, cameraShake).fallbackName("damage.default").defaultValue(0.0f).minimumValue(0.0f).description("Passed to the wupget:ShockFront callin as the first argument, intended for shaking the camera on particularly strong hits. Same scale as damage.");
WEAPONTAG(float3, animParams1, visuals.animParams[0]).fallbackName("animParams").defaultValue(float3{ 1.0f, 1.0f, 30.0f }).description("Used to do flipbook style animation of texture1");
WEAPONTAG(float3, animParams2, visuals.animParams[1]).fallbackName("animParams").defaultValue(float3{ 1.0f, 1.0f, 30.0f }).description("Used to do flipbook style animation of texture2");
WEAPONTAG(float3, animParams3, visuals.animParams[2]).fallbackName("animParams").defaultValue(float3{ 1.0f, 1.0f, 30.0f }).description("Used to do flipbook style animation of texture3");
WEAPONTAG(float3, animParams4, visuals.animParams[3]).fallbackName("animParams").defaultValue(float3{ 1.0f, 1.0f, 30.0f }).description("Used to do flipbook style animation of texture4");

// Missile
WEAPONTAG(bool, smokeTrail, visuals.smokeTrail).defaultValue(false).description("MissileLauncher only. Does it leave a smoke trail ribbon?");
WEAPONTAG(bool, smokeTrailCastShadow, visuals.smokeTrailCastShadow).defaultValue(true).description("Does the smoke trail cast shadow?");
WEAPONTAG(int, smokePeriod, visuals.smokePeriod).defaultValue(8).description("Smoke trail update rate - the trail ribbon will make a new vertex this many sim frames. Use for high turn-rate homing missiles to prevent jagged edges. Smaller is smoother but more performance-heavy.");
WEAPONTAG(int, smokeTime, visuals.smokeTime).defaultValue(2 * GAME_SPEED).description("Smoke trail linger duration, in sim frames");
WEAPONTAG(float, smokeSize, visuals.smokeSize).defaultValue(7.0f).description("Smoke trail size multiplier");
WEAPONTAG(float, smokeColor, visuals.smokeColor).defaultValue(0.65f).description("Smoke trail brightness multiplier");
WEAPONTAG(bool, castShadow, visuals.castShadow).defaultValue(true).description("Does the projectile itself cast shadow?"); //TODO move out of missile block?

// Cannon
WEAPONTAG(float, sizeDecay, visuals.sizeDecay).defaultValue(0.0f).description("#Cannon only. See `stages`. Size reduction per stage, as a fraction of the first stage");
WEAPONTAG(float, alphaDecay, visuals.alphaDecay).defaultValue(1.0f).description("#Cannon only. See `stages`. Alpha reduction for the last stage (interpolated linearly for the others).");
WEAPONTAG(float, separation, visuals.separation).defaultValue(1.0f).description("#Cannon only. See `stages`. Multiplier for the default distance between stages");
WEAPONTAG(bool, noGap, visuals.noGap).defaultValue(true).description("#Cannon only. Makes `separation` adjust to take `sizeDecay` into account so `stages` stay adjacent.");
WEAPONTAG(int, stages, visuals.stages).defaultValue(5).description("#Cannon only. If `model` is not set then draw this many 2D sprites to simulate a sort of motion blur.");

// Laser
WEAPONTAG(int, lodDistance, visuals.lodDistance).defaultValue(1000).description("LaserCannon only. Distance at which rendering is simplified, without the rounded ends");
WEAPONTAG(float, thickness, visuals.thickness).defaultValue(2.0f).description("LaserCannon, BeamLaser and Lightning only. How thicc is the laser?");
WEAPONTAG(float, coreThickness, visuals.corethickness).defaultValue(0.25f).description("BeamLaser and LaserCannon only. Thickness of the inner core as a fraction of full thickness (0-1). Just to get a secondary color via `rgbColor2`.");
WEAPONTAG(float, laserFlareSize, visuals.laserflaresize).defaultValue(15.0f).description("BeamLaser only. Size of the flare visual effect at emit point, in elmos");
WEAPONTAG(float, tileLength, visuals.tilelength).defaultValue(200.0f).description("'Large' BeamLaser only. Length in elmos of a repeated texture tile for the beam. Regular BeamLaser just has a single stretched tile.");
WEAPONTAG(float, scrollSpeed, visuals.scrollspeed).defaultValue(5.0f).description("'Large' BeamLaser only. Speed in elmo/s at which the tiled beam texture shifts.");
WEAPONTAG(float, pulseSpeed, visuals.pulseSpeed).defaultValue(1.0f).description("'Large' BeamLaser only. Frequency of beam pulsation (fade to zero alpha and back) in hertz");
WEAPONTAG(float, beamDecay, visuals.beamdecay).defaultValue(1.0f).description("BeamLaser only. Controls the fadeout (multiplier for alpha per sim frame)");
WEAPONTAG(float, falloffRate).defaultValue(0.5f).description("LaserCannon with `hardStop = false` only. How much, as a fraction, the laser fades per sim frame beyond max range. Capped to be 0.2 or above (ie. will never take more than 5 sim frames to fade completely)");
WEAPONTAG(bool, laserHardStop).externalName("hardstop").defaultValue(false).description("LaserCannon only. If true, lasers get 'eaten up' at max range. Otherwise they continue on and fade away according to `intensityFalloff` (but can't collide anymore).");

// Color
WEAPONTAG(float3, rgbColor, visuals.color).defaultValue(float3(1.0f, 0.5f, 0.0f)).description("Color of the sprite, when not using a model. #EmgCannon has a different default of 0.9/0.9/0.2.");
WEAPONTAG(float3, rgbColor2, visuals.color2).defaultValue(float3(1.0f, 1.0f, 1.0f)).description("BeamLaser and LaserCannon only. The color of the inner core of the sprite, see `coreThickness`.");
WEAPONTAG(float, intensity).defaultValue(0.9f).description("Alpha transparency for non-model projectiles. Lower values are more opaque, but 0.0 will cause the projectile to disappear entirely.");
WEAPONTAG(std::string, colormap, visuals.colorMapStr).defaultValue("").description("A series of RGBA tuples. If the projectile is a sprite then it will shift over these colours over its lifetime");

WEAPONTAG(std::string, textures1, visuals.texNames[0]).externalName("textures.1").fallbackName("texture1").defaultValue("").description("When not using a model, sprite texture for AircraftBomb, Cannon, EMG, Flame; main beam texture for LaserCannon, BeamLaser, Lightning; flare texture for Missile, Starburst; dome for Shield. Note that DGun has a hardcoded texture.");
WEAPONTAG(std::string, textures2, visuals.texNames[1]).externalName("textures.2").fallbackName("texture2").defaultValue("").description("The end-of-beam texture for LaserCannon and BeamLaser (half of texture for each end); smoketrail for Missile and Starburst. Note that Torpedo has a hardcoded trail.");
WEAPONTAG(std::string, textures3, visuals.texNames[2]).externalName("textures.3").fallbackName("texture3").defaultValue("").description("Flare for non-'large' BeamLaser, or directional muzzle exhaust for 'large'; flame exhaust for Starburst.");
WEAPONTAG(std::string, textures4, visuals.texNames[3]).externalName("textures.4").fallbackName("texture4").defaultValue("").description("Flare for 'large' BeamLaser.");

WEAPONTAG(std::string, cegTag,                   visuals.ptrailExpGenTag).defaultValue("").description("The name, without prefixes, of a CEG to be emitted by the projectile each frame.");
WEAPONTAG(std::string, explosionGenerator,       visuals.impactExpGenTag).defaultValue("").description("The name, with prefix, of a CEG to be emitted on impact");
WEAPONTAG(std::string, bounceExplosionGenerator, visuals.bounceExpGenTag).defaultValue("").description("The name, with prefix, of a CEG to be emitted when bouncing");

// Sound
WEAPONDUMMYTAG(bool, soundTrigger).description("Does the weapon produce the shooting sound only once for the whole burst? If false, for each shot in a burst.");
WEAPONDUMMYTAG(std::string, soundStart).defaultValue("").description("The sound emitted when shooting the weapon.");
WEAPONDUMMYTAG(std::string, soundHitDry).fallbackName("soundHit").defaultValue("").description("The sound emitted when hitting outside water. Note that a #BeamLaser will play this once per sim frame.");
WEAPONDUMMYTAG(std::string, soundHitWet).fallbackName("soundHit").defaultValue("").description("The sound emitted when hitting in water. Note that a #BeamLaser will play this once per sim frame.");
WEAPONDUMMYTAG(float, soundStartVolume).defaultValue(-1.0f).description("Sound volume of the shot. -1 means autogenerated from damage.");
WEAPONDUMMYTAG(float, soundHitDryVolume).fallbackName("soundHitVolume").defaultValue(-1.0f).description("Sound volume of the impact, outside water. -1 means autogenerate from damage.");
WEAPONDUMMYTAG(float, soundHitWetVolume).fallbackName("soundHitVolume").defaultValue(-1.0f).description("Sound volume of the impact, inside water. -1 means autogenerate from damage.");


WeaponDef::WeaponDef()
{
	id = 0;
	projectileType = WEAPON_BASE_PROJECTILE;
	collisionFlags = 0;

	// set later by ProjectileDrawer
	ptrailExplosionGeneratorID = CExplosionGeneratorHandler::EXPGEN_ID_INVALID;
	impactExplosionGeneratorID = CExplosionGeneratorHandler::EXPGEN_ID_STANDARD;
	bounceExplosionGeneratorID = CExplosionGeneratorHandler::EXPGEN_ID_INVALID;

	isNulled = false;
	isShield = false;
	noAutoTarget = false;
	onlyForward = false;

	damages.fromDef = true;

	WeaponDefs.Load(this, {});
}

WeaponDef::WeaponDef(const LuaTable& wdTable, const std::string& name_, int id_)
	: name(name_)

	, ptrailExplosionGeneratorID(CExplosionGeneratorHandler::EXPGEN_ID_INVALID)
	, impactExplosionGeneratorID(CExplosionGeneratorHandler::EXPGEN_ID_STANDARD)
	, bounceExplosionGeneratorID(CExplosionGeneratorHandler::EXPGEN_ID_INVALID)

	, id(id_)
	, projectileType(WEAPON_BASE_PROJECTILE)
	, collisionFlags(0)
{
	{
		WeaponDefs.Load(this, wdTable);
		WeaponDefs.ReportUnknownTags(name, wdTable);
	}


	if (wdTable.KeyExists("cylinderTargetting"))
		LOG_L(L_WARNING, "WeaponDef (%s) cylinderTargetting is deprecated and will be removed in the next release (use cylinderTargeting).", name.c_str());

	if (wdTable.KeyExists("color1") || wdTable.KeyExists("color2"))
		LOG_L(L_WARNING, "WeaponDef (%s) color1 & color2 (= hue & sat) are removed. Use rgbColor instead!", name.c_str());

	if (wdTable.KeyExists("isShield"))
		LOG_L(L_WARNING, "WeaponDef (%s) The \"isShield\" tag has been removed. Use the weaponType=\"Shield\" tag instead!", name.c_str());

	shieldRechargeDelay = int(wdTable.GetFloat("rechargeDelay", 0) * GAME_SPEED);
	shieldArmorType = damageArrayHandler.GetTypeFromName(shieldArmorTypeName);
	flighttime = int(wdTable.GetFloat("flighttime", 0.0f) * GAME_SPEED);
	maxFireAngle = math::cos(wdTable.GetFloat("firetolerance", 3640.0f) * TAANG2RAD);
	salvoWindup = int(wdTable.GetFloat("windup", 0.0f) * GAME_SPEED);

	collisionFlags = 0;
	collisionFlags |= (Collision::NOENEMIES    * (!wdTable.GetBool("collideEnemy",      true)));
	collisionFlags |= (Collision::NOFRIENDLIES * (!wdTable.GetBool("collideFriendly",   true)));
	collisionFlags |= (Collision::NOFEATURES   * (!wdTable.GetBool("collideFeature",    true)));
	collisionFlags |= (Collision::NONEUTRALS   * (!wdTable.GetBool("collideNeutral",    true)));
	collisionFlags |= (Collision::NOFIREBASES  * (!wdTable.GetBool("collideFireBase",  false)));
	collisionFlags |= (Collision::NONONTARGETS * (!wdTable.GetBool("collideNonTarget",  true)));
	collisionFlags |= (Collision::NOGROUND     * (!wdTable.GetBool("collideGround",     true)));
	collisionFlags |= (Collision::NOCLOAKED    * (!wdTable.GetBool("collideCloaked",    true)));

	cost =
		{ wdTable.GetFloat( "metalPerShot", 0.0f)
		, wdTable.GetFloat("energyPerShot", 0.0f)
	};

	//FIXME defaults depend on other tags
	{
		if (paralyzer)
			cameraShake = wdTable.GetFloat("cameraShake", 0.0f);

		if (selfExplode)
			predictBoost = wdTable.GetFloat("predictBoost", 0.5f);

		switch (hashString(type.c_str())) {
			case hashString("Melee"): {
				targetBorder = std::clamp(wdTable.GetFloat("targetBorder", 1.0f), -1.0f, 1.0f);
				cylinderTargeting = std::clamp(wdTable.GetFloat("cylinderTargeting", wdTable.GetFloat("cylinderTargetting", 1.0f)), 0.0f, 128.0f);
			} break;

			//TODO move to lua (for all other weapons this tag is named `duration` and has a different default)
			case hashString("Flame"): {
				duration = wdTable.GetFloat("flameGfxTime", 1.2f);
			} break;

			case hashString("Cannon"): {
				heightmod = wdTable.GetFloat("heightMod", 0.8f);
			} break;
			case hashString("BeamLaser"):
			case hashString("LightningCannon"): {
				heightmod = wdTable.GetFloat("heightMod", 1.0f);
			} break;

			case hashString("LaserCannon"): {
				// for lasers we want this to be true by default: it sets
				// projectile ttl values to the minimum required to hit a
				// target which prevents them overshooting (lasers travel
				// many elmos per frame and ttl's are rounded) at maximum
				// range
				selfExplode = wdTable.GetBool("burnblow", true);
			} break;

			default: {
			} break;
		}
	}

	// setup the default damages
	{
		const LuaTable dmgTable = wdTable.SubTable("damage");
		float defDamage = dmgTable.GetFloat("default", 1.0f);

		damages.SetDefaultDamage(defDamage);
		damages.fromDef = true;

		if (!paralyzer)
			damages.paralyzeDamageTime = 0;


		static std::vector<std::pair<std::string, float>> dmgs;

		dmgs.clear();
		dmgs.reserve(32);
		dmgTable.GetPairs(dmgs);

		for (const auto& [armorTypeName, damage] : dmgs) {
			const int armorType = damageArrayHandler.GetTypeFromName(armorTypeName);
			if (armorType == 0)
				continue;
			damages.Set(armorType, damage);
		}

		const float tempsize = 2.0f + std::min(defDamage * 0.0025f, damages.damageAreaOfEffect * 0.1f);
		const float gd = std::max(30.0f, defDamage / 20.0f);
		const float defExpSpeed = (8.0f + (gd * 2.5f)) / (9.0f + (math::sqrt(gd) * 0.7f)) * 0.5f;

		size = wdTable.GetFloat("size", tempsize);
		damages.explosionSpeed = wdTable.GetFloat("explosionSpeed", defExpSpeed);
		if (damages.dynDamageRange <= 0.0f)
			damages.dynDamageRange = range;
	}

	{
		// 0.78.2.1 backwards compatibility: non-burst beamlasers play one
		// sample per shot, not for each individual beam making up the shot
		const bool singleSampleShot = (type == "BeamLaser" && !beamburst);
		const bool singleShotWeapon = (type == "Melee" || type == "Rifle");

		soundTrigger = wdTable.GetBool("soundTrigger", singleSampleShot || singleShotWeapon);
	}

	{
		// get some weapon specific defaults
		defInterceptType = 0;

		switch (hashString(type.c_str())) {
			case hashString("Cannon"): {
				// CExplosiveProjectile
				defInterceptType = 1;
				projectileType = WEAPON_EXPLOSIVE_PROJECTILE;

				intensity = wdTable.GetFloat("intensity", 0.2f);
			} break;
			case hashString("Rifle"): {
				// no projectile or intercept type
				defInterceptType = 128;
			} break;
			case hashString("Melee"): {
				// no projectile or intercept type
				defInterceptType = 256;
			} break;
			case hashString("Flame"): {
				// CFlameProjectile
				projectileType = WEAPON_FLAME_PROJECTILE;
				defInterceptType = 16;

				collisionSize = wdTable.GetFloat("collisionSize", 0.5f);
			} break;
			case hashString("MissileLauncher"): {
				// CMissileProjectile
				projectileType = WEAPON_MISSILE_PROJECTILE;
				defInterceptType = 4;
			} break;
			case hashString("LaserCannon"): {
				// CLaserProjectile
				projectileType = WEAPON_LASER_PROJECTILE;
				defInterceptType = 2;

				collisionSize = wdTable.GetFloat("collisionSize", 0.5f);
			} break;
			case hashString("BeamLaser"): {
				projectileType = largeBeamLaser? WEAPON_LARGEBEAMLASER_PROJECTILE: WEAPON_BEAMLASER_PROJECTILE;
				defInterceptType = 2;
			} break;
			case hashString("LightningCannon"): {
				projectileType = WEAPON_LIGHTNING_PROJECTILE;
				defInterceptType = 64;
			} break;
			case hashString("EmgCannon"): {
				// CEmgProjectile
				projectileType = WEAPON_EMG_PROJECTILE;
				defInterceptType = 1;

				size = wdTable.GetFloat("size", 3.0f);
			} break;
			case hashString("TorpedoLauncher"): {
				// WeaponLoader will create either BombDropper with dropTorpedoes = true
				// (owner->unitDef->canfly && !weaponDef->submissile) or TorpedoLauncher
				// (both types of weapons will spawn TorpedoProjectile's)
				projectileType = WEAPON_TORPEDO_PROJECTILE;
				defInterceptType = 32;

				waterweapon = true;
			} break;
			case hashString("DGun"): {
				// CFireBallProjectile
				projectileType = WEAPON_FIREBALL_PROJECTILE;

				collisionSize = wdTable.GetFloat("collisionSize", 10.0f);
				leadLimit = wdTable.GetFloat("leadLimit", 0.0f);
			} break;
			case hashString("StarburstLauncher"): {
				// CStarburstProjectile
				projectileType = WEAPON_STARBURST_PROJECTILE;
				defInterceptType = 4;
			} break;
			case hashString("AircraftBomb"): {
				// WeaponLoader will create BombDropper with dropTorpedoes = false
				// BombDropper with dropTorpedoes=false spawns ExplosiveProjectile's
				//
				projectileType = WEAPON_EXPLOSIVE_PROJECTILE;
				defInterceptType = 8;
			} break;
			default: {
			} break;
		}

		if ((ownerExpAccWeight = wdTable.GetFloat("ownerExpAccWeight", 0.0f)) < 0.0f)
			LOG_L(L_ERROR, "[%s] weaponDef %s has negative ownerExpAccWeight %f", __func__, name.c_str(), ownerExpAccWeight);

		interceptedByShieldType = wdTable.GetInt("interceptedByShieldType", defInterceptType);
	}

	const auto siTbl = wdTable.SubTable("scarIndices");
	const int siTblSize = siTbl.GetLength();
	for (int i = 1; i <= siTblSize; ++i) {
		const auto si = siTbl.GetInt(i, 0);
		if (si > 0)
			visuals.scarIdcs.emplace_back(si);
	}

	visuals.scarProjVector.w = visuals.scarProjVector.LengthNormalize();

	if (!visuals.scarGlowColorMapStr.empty())
		visuals.scarGlowColorMap = CColorMap::LoadFromDefString(visuals.scarGlowColorMapStr);

	ParseWeaponSounds(wdTable);

	// custom parameters table
	wdTable.SubTable("customParams").GetMap(customParams);

	// internal only
	isNulled = (STRCASECMP(name.c_str(), "noweapon") == 0);
	isShield = (type == "Shield");
	noAutoTarget = (manualfire || interceptor || isShield);
	onlyForward = !turret && (projectileType != WEAPON_STARBURST_PROJECTILE);
}



void WeaponDef::ParseWeaponSounds(const LuaTable& wdTable) {
	RECOIL_DETAILED_TRACY_ZONE;
	LoadSound(wdTable, "soundStart" , fireSound);
	LoadSound(wdTable, "soundHitDry",  hitSound);
	LoadSound(wdTable, "soundHitWet",  hitSound);
}



void WeaponDef::LoadSound(
	const LuaTable& wdTable,
	const std::string& soundKey,
	GuiSoundSet& soundData
) {
	RECOIL_DETAILED_TRACY_ZONE;
	switch (hashString(soundKey.c_str())) {
		case hashString("soundStart"): {
			CommonDefHandler::AddSoundSetData(soundData, wdTable.GetString(soundKey, ""), wdTable.GetFloat(soundKey + "Volume", 1.0f));
		} break;

		case hashString("soundHitDry"): {
			CommonDefHandler::AddSoundSetData(soundData, wdTable.GetString(soundKey, wdTable.GetString("soundHit", "")), wdTable.GetFloat(soundKey + "Volume", wdTable.GetFloat("soundHitVolume", 1.0f)));
		} break;
		case hashString("soundHitWet"): {
			CommonDefHandler::AddSoundSetData(soundData, wdTable.GetString(soundKey, wdTable.GetString("soundHit", "")), wdTable.GetFloat(soundKey + "Volume", wdTable.GetFloat("soundHitVolume", 1.0f)));
		} break;

		default: {
		} break;
	}
}


S3DModel* WeaponDef::LoadModel()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (visuals.model != nullptr)
		return visuals.model;

	if (!visuals.modelName.empty())
		return (visuals.model = modelLoader.LoadModel(visuals.modelName));

	// not useful, too much spam
	// LOG_L(L_WARNING, "[WeaponDef::%s] weapon \"%s\" has no model defined", __FUNCTION__, name.c_str());
	return nullptr;
}

S3DModel* WeaponDef::LoadModel() const {
	RECOIL_DETAILED_TRACY_ZONE;
	//not very sweet, but still better than replacing "const WeaponDef" _everywhere_
	return const_cast<WeaponDef*>(this)->LoadModel();
}

void WeaponDef::PreloadModel() {
	RECOIL_DETAILED_TRACY_ZONE;
	if (visuals.model != nullptr)
		return;
	if (visuals.modelName.empty())
		return;

	modelLoader.PreloadModel(visuals.modelName);
}

void WeaponDef::PreloadModel() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	//not very sweet, but still better than replacing "const WeaponDef" _everywhere_
	const_cast<WeaponDef*>(this)->PreloadModel();
}
