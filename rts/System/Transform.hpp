#pragma once

#include "Quaternion.h"
#include "float4.h"
#include "creg/creg_cond.h"

struct Transform {
	CR_DECLARE_STRUCT(Transform)
	CQuaternion r;
	float4 t;
	float4 s;
	static const Transform& Zero() {
		static Transform zero {
			.r = CQuaternion(0.0f, 0.0f, 0.0f, 0.0f),
			.t = float4(0.0f, 0.0f, 0.0f, 0.0f),
			.s = float4(0.0f, 0.0f, 0.0f, 0.0f)
		};
		return zero;
	}
};