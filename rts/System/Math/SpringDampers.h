/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "System/float3.h"

float halflife_to_damping(float halflife, float eps = 1e-5f);

float fast_negexp(float x);

float spring_damper_damping(float halflife);

float spring_damper_eydt(float y, float dt);

void simple_spring_damper_exact(
	float& x,
	float& v,
	float x_goal,
	float damping,
	float eydt,
	float dt);

void simple_spring_damper_exact_vector_part(
	float& cur,
	float& v,
	float goal,
	float damping,
	float eydt,
	float dt);

void simple_spring_damper_exact_vector(
	float3& cur,
	float3& v,
	float3 goal,
	float damping,
	float eydt,
	float dt);

void timed_spring_damper_exact_vector(
	float3& x,
	float3& v,
	float3& xi,
	float3 goal,
	float t_goal,
	float halflife,
	float damping,
	float eydt,
	float dt,
	float apprehension = 2.0f);
