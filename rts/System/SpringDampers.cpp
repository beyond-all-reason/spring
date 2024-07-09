/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html

	Spring Damper Functions are MIT Licensed Copyright 2021 Daniel Holden
	https://github.com/orangeduck/Spring-It-On
	https://theorangeduck.com/page/spring-roll-call
*/

#include "System/SpringDampers.h"

float halflife_to_damping(float halflife, float eps)
{
	return (4.0f * 0.69314718056f) / (halflife + eps);
}

float fast_negexp(float x)
{
	return 1.0f / (1.0f + x + 0.48f*x*x + 0.235f*x*x*x);
}

float spring_damper_damping(float halflife)
{
	return halflife_to_damping(halflife) / 2.0f;
}

float spring_damper_eydt(float y, float dt)
{
	return fast_negexp(y*dt);
}

void simple_spring_damper_exact(
	float& x,
	float& v,
	float x_goal,
	float damping,
	float eydt,
	float dt)
{
	float j0 = x - x_goal;
	float j1 = v + j0*damping;

	x = eydt*(j0 + j1*dt) + x_goal;
	v = eydt*(v - j1*damping*dt);
}

void simple_spring_damper_exact_vector_part(
	float& cur,
	float& v,
	float goal,
	float damping,
	float eydt,
	float dt)
{
	float xj0 = cur - goal;
	float xj1 = v + xj0*damping;

	cur = eydt*(xj0 + xj1*dt) + goal;
	v = eydt*(v - xj1*damping*dt);
}

void simple_spring_damper_exact_vector(
	float3& cur,
	float3& v,
	float3 goal,
	float damping,
	float eydt,
	float dt)
{
	simple_spring_damper_exact_vector_part(cur.x, v.x, goal.x, damping, eydt, dt);
	simple_spring_damper_exact_vector_part(cur.y, v.y, goal.y, damping, eydt, dt);
	simple_spring_damper_exact_vector_part(cur.z, v.z, goal.z, damping, eydt, dt);
}

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
	float apprehension)
{
	float min_time = t_goal > dt ? t_goal : dt;
	float t_goal_future = dt + apprehension * halflife;

	float xv_goal = (goal.x - xi.x) / min_time;
	float x_goal_future = t_goal_future < t_goal ?
			xi.x + xv_goal * t_goal_future : goal.x;
	simple_spring_damper_exact(x.x, v.x, x_goal_future, damping, eydt, dt);
	xi.x += xv_goal * dt;

	float yv_goal = (goal.y - xi.y) / min_time;
	float y_goal_future = t_goal_future < t_goal ?
			xi.y + yv_goal * t_goal_future : goal.y;
	simple_spring_damper_exact(x.y, v.y, y_goal_future, damping, eydt, dt);
	xi.y += yv_goal * dt;

	float zv_goal = (goal.z - xi.z) / min_time;
	float z_goal_future = t_goal_future < t_goal ?
			xi.z + zv_goal * t_goal_future : goal.z;
	simple_spring_damper_exact(x.z, v.z, z_goal_future, damping, eydt, dt);
	xi.z += zv_goal * dt;
}