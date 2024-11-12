/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "System/Color.h"
#include "System/float3.h"
#include "System/float4.h"
#include "System/type2.h"
#include "myGL.h"

#include <array>
#include <string>

// Workaround
// warning: 'offsetof' within non-standard-layout type 'VA_TYPE_**' is conditionally-supported [-Winvalid-offsetof]
#define VA_TYPE_OFFSET(T, m) (reinterpret_cast<const void*>( &(reinterpret_cast<const T*>(0)->m) ))

struct AttributeDef {
	AttributeDef(uint32_t index_, uint32_t count_, uint32_t type_, uint32_t stride_, const void* data_, bool normalize_ = false, std::string name_ = "")
		: name{ std::move(name_) }
		, index{ index_ }
		, count{ count_ }
		, type{ type_ }
		, stride{ stride_ }
		, data{ data_ }
		, normalize{ static_cast<uint8_t>(normalize_) }
	{}

	const std::string name;

	uint32_t index;
	uint32_t count;   // in number of elements of type
	uint32_t type;    // GL_FLOAT, etc
	uint32_t stride;  // in bytes

	const void* data; // offset

	uint8_t normalize;
};

struct VA_TYPE_0 {
	using MY_VA_TYPE = VA_TYPE_0;
	float3 pos;

	auto operator+(MY_VA_TYPE const& o) const {
		auto v = *this;
		v.pos += o.pos;
		return v;
	}
	auto operator-(MY_VA_TYPE const& o) const {
		auto v = *this;
		v.pos -= o.pos;
		return v;
	}
	auto operator* (float t) const {
		auto v = *this;
		v.pos *= t;
		return v;
	}

	static std::array<AttributeDef, 1> attributeDefs;
};
struct VA_TYPE_N {
	using MY_VA_TYPE = VA_TYPE_N;
	float3 pos;
	float3 n;

	auto operator+(MY_VA_TYPE const& o) const {
		auto v = *this;
		v.pos += o.pos;
		v.n += o.n; v.n.ANormalize();
		return v;
	}
	auto operator-(MY_VA_TYPE const& o) const {
		auto v = *this;
		v.pos -= o.pos;
		v.n -= o.n; v.n.ANormalize();
		return v;
	}
	auto operator* (float t) const {
		auto v = *this;
		v.pos *= t;
		v.n *= t; v.n.ANormalize();

		return v;
	}

	static std::array<AttributeDef, 2> attributeDefs;
};
struct VA_TYPE_C {
	using MY_VA_TYPE = VA_TYPE_C;
	float3 pos;
	SColor c;

	auto operator+(MY_VA_TYPE const& o) const {
		auto v = *this;
		v.pos += o.pos;
		v.c += o.c;
		return v;
	}
	auto operator-(MY_VA_TYPE const& o) const {
		auto v = *this;
		v.pos -= o.pos;
		v.c -= o.c;
		return v;
	}
	auto operator* (float t) const {
		auto v = *this;
		v.pos *= t;
		v.c *= t;
		return v;
	}

	static std::array<AttributeDef, 2> attributeDefs;
};
struct VA_TYPE_T {
	using MY_VA_TYPE = VA_TYPE_T;
	float3 pos;
	float  s, t;

	auto operator+(MY_VA_TYPE const& o) const {
		auto v = *this;
		v.pos += o.pos;
		v.s += o.s;
		v.t += o.t;
		return v;
	}
	auto operator-(MY_VA_TYPE const& o) const {
		auto v = *this;
		v.pos -= o.pos;
		v.s -= o.s;
		v.t -= o.t;
		return v;
	}
	auto operator* (float t) const {
		auto v = *this;
		v.pos *= t;
		v.s *= t;
		v.t *= t;
		return v;
	}

	static std::array<AttributeDef, 2> attributeDefs;
};
struct VA_TYPE_T4 {
	using MY_VA_TYPE = VA_TYPE_T4;
	float3 pos;
	float4 uv;

	auto operator+(MY_VA_TYPE const& o) const {
		auto v = *this;
		v.pos += o.pos;
		v.uv += o.uv;		
		return v;
	}
	auto operator-(MY_VA_TYPE const& o) const {
		auto v = *this;
		v.pos -= o.pos;
		v.uv -= o.uv;
		return v;
	}
	auto operator* (float t) const {
		auto v = *this;
		v.pos *= t;
		v.uv *= t;
		return v;
	}

	static std::array<AttributeDef, 2> attributeDefs;
};
struct VA_TYPE_TN {
	using MY_VA_TYPE = VA_TYPE_TN;
	float3 pos;
	float  s, t;
	float3 n;

	auto operator+(MY_VA_TYPE const& o) const {
		auto v = *this;
		v.pos += o.pos;
		v.s += o.s;
		v.t += o.t;
		v.n += o.n; v.n.ANormalize();
		return v;
	}
	auto operator-(MY_VA_TYPE const& o) const {
		auto v = *this;
		v.pos -= o.pos;
		v.s -= o.s;
		v.t -= o.t;
		v.n -= o.n; v.n.ANormalize();
		return v;
	}
	auto operator* (float t) const {
		auto v = *this;
		v.pos *= t;
		v.s *= t;
		v.t *= t;
		v.n *= t; v.n.ANormalize();
		return v;
	}

	static std::array<AttributeDef, 3> attributeDefs;
};
struct VA_TYPE_TC {
	using MY_VA_TYPE = VA_TYPE_TC;
	float3 pos;
	float  s, t;
	SColor c;

	auto operator+(MY_VA_TYPE const& o) const {
		auto v = *this;
		v.pos += o.pos;
		v.s += o.s;
		v.t += o.t;
		v.c += o.c;
		return v;
	}
	auto operator-(MY_VA_TYPE const& o) const {
		auto v = *this;
		v.pos -= o.pos;
		v.s -= o.s;
		v.t -= o.t;
		v.c -= o.c;
		return v;
	}
	auto operator* (float t) const {
		auto v = *this;
		v.pos *= t;
		v.s *= t;
		v.t *= t;
		v.c *= t;
		return v;
	}

	static std::array<AttributeDef, 3> attributeDefs;
};
struct VA_TYPE_TC3 {
	using MY_VA_TYPE = VA_TYPE_TC3;
	float3 pos;
	float  s, t, u;
	SColor c;

	auto operator+(MY_VA_TYPE const& o) const {
		auto v = *this;
		v.pos += o.pos;
		v.s += o.s;
		v.t += o.t;
		v.u += o.u;
		v.c += o.c;
		return v;
	}
	auto operator-(MY_VA_TYPE const& o) const {
		auto v = *this;
		v.pos -= o.pos;
		v.s -= o.s;
		v.t -= o.t;
		v.u -= o.u;
		v.c -= o.c;
		return v;
	}
	auto operator* (float t) const {
		auto v = *this;
		v.pos *= t;
		v.s *= t;
		v.t *= t;
		v.u *= t;
		v.c *= t;
		return v;
	}

	static std::array<AttributeDef, 3> attributeDefs;
};
struct VA_TYPE_PROJ {
	using MY_VA_TYPE = VA_TYPE_PROJ;
	float3 pos;
	float3 uvw;
	float4 uvInfo;
	float3 aparams;
	SColor c;

	auto operator+(MY_VA_TYPE const& o) const {
		auto v = *this;
		v.pos += o.pos;
		v.uvw += o.uvw;
		v.uvInfo += o.uvInfo;
		v.aparams += o.aparams;
		v.c += o.c;
		return v;
	}
	auto operator-(MY_VA_TYPE const& o) const {
		auto v = *this;
		v.pos -= o.pos;
		v.uvw -= o.uvw;
		v.uvInfo -= o.uvInfo;
		v.aparams -= o.aparams;
		v.c -= o.c;
		return v;
	}
	auto operator* (float t) const {
		auto v = *this;
		v.pos *= t;
		v.uvw *= t;
		v.uvInfo *= t;
		v.aparams *= t;
		v.c *= t;
		return v;
	}

	static std::array<AttributeDef, 5> attributeDefs;
};
struct VA_TYPE_TNT {
	using MY_VA_TYPE = VA_TYPE_TNT;
	float3 pos;
	float  s, t;
	float3 n;
	float3 uv1;
	float3 uv2;

	auto operator+(MY_VA_TYPE const& o) const {
		auto v = *this;
		v.pos += o.pos;
		v.s += o.s;
		v.t += o.t;
		v.n += o.n; v.n.ANormalize();
		v.uv1 += o.uv1;
		v.uv2 += o.uv2;
		return v;
	}
	auto operator-(MY_VA_TYPE const& o) const {
		auto v = *this;
		v.pos -= o.pos;
		v.s -= o.s;
		v.t -= o.t;
		v.n -= o.n; v.n.ANormalize();
		v.uv1 -= o.uv1;
		v.uv2 -= o.uv2;
		return v;
	}
	auto operator* (float t) const {
		auto v = *this;
		v.pos *= t;
		v.s *= t;
		v.t *= t;
		v.n *= t; v.n.ANormalize();
		v.uv1 *= t;
		v.uv2 *= t;
		return v;
	}

	static std::array<AttributeDef, 5> attributeDefs;
};
struct VA_TYPE_2D0 {
	using MY_VA_TYPE = VA_TYPE_2D0;
	float x, y;

	auto operator+(MY_VA_TYPE const& o) const {
		auto v = *this;
		v.x += o.x;
		v.y += o.y;
		return v;
	}
	auto operator-(MY_VA_TYPE const& o) const {
		auto v = *this;
		v.x -= o.x;
		v.y -= o.y;
		return v;
	}
	auto operator* (float t) const {
		auto v = *this;
		v.x *= t;
		v.y *= t;
		return v;
	}

	static std::array<AttributeDef, 1> attributeDefs;
};
struct VA_TYPE_2DC {
	using MY_VA_TYPE = VA_TYPE_2DC;
	float x, y;
	SColor c;

	auto operator+(MY_VA_TYPE const& o) const {
		auto v = *this;
		v.x += o.x;
		v.y += o.y;
		v.c += o.c;
		return v;
	}
	auto operator-(MY_VA_TYPE const& o) const {
		auto v = *this;
		v.x -= o.x;
		v.y -= o.y;
		v.c -= o.c;
		return v;
	}
	auto operator* (float t) const {
		auto v = *this;
		v.x *= t;
		v.y *= t;
		v.c *= t;
		return v;
	}

	static std::array<AttributeDef, 2> attributeDefs;
};
struct VA_TYPE_2DT {
	using MY_VA_TYPE = VA_TYPE_2DT;
	float x, y;
	float s, t;

	auto operator+(MY_VA_TYPE const& o) const {
		auto v = *this;
		v.x += o.x;
		v.y += o.y;
		v.s += o.s;
		v.t += o.t;
		return v;
	}
	auto operator-(MY_VA_TYPE const& o) const {
		auto v = *this;
		v.x -= o.x;
		v.y -= o.y;
		v.s -= o.s;
		v.t -= o.t;
		return v;
	}
	auto operator* (float t) const {
		auto v = *this;
		v.x *= t;
		v.y *= t;
		v.s *= t;
		v.t *= t;
		return v;
	}

	static std::array<AttributeDef, 2> attributeDefs;
};
struct VA_TYPE_2DTC {
	using MY_VA_TYPE = VA_TYPE_2DTC;
	float  x, y;
	float  s, t;
	SColor c;

	auto operator+(MY_VA_TYPE const& o) const {
		auto v = *this;
		v.x += o.x;
		v.y += o.y;
		v.s += o.s;
		v.t += o.t;
		v.c += o.c;
		return v;
	}
	auto operator-(MY_VA_TYPE const& o) const {
		auto v = *this;
		v.x -= o.x;
		v.y -= o.y;
		v.s -= o.s;
		v.t -= o.t;
		v.c -= o.c;
		return v;
	}
	auto operator* (float t) const {
		auto v = *this;
		v.x *= t;
		v.y *= t;
		v.s *= t;
		v.t *= t;
		v.c *= t;
		return v;
	}

	static std::array<AttributeDef, 3> attributeDefs;
};

struct VA_TYPE_2DTC3 {
	using MY_VA_TYPE = VA_TYPE_2DTC3;
	float  x, y;
	float  s, t, u;
	SColor c;

	auto operator+(MY_VA_TYPE const& o) const {
		auto v = *this;
		v.x += o.x;
		v.y += o.y;
		v.s += o.s;
		v.t += o.t;
		v.u += o.u;
		v.c += o.c;
		return v;
	}
	auto operator-(MY_VA_TYPE const& o) const {
		auto v = *this;
		v.x -= o.x;
		v.y -= o.y;
		v.s -= o.s;
		v.t -= o.t;
		v.u -= o.u;
		v.c -= o.c;
		return v;
	}
	auto operator* (float t) const {
		auto v = *this;
		v.x *= t;
		v.y *= t;
		v.s *= t;
		v.t *= t;
		v.u *= u;
		v.c *= t;
		return v;
	}

	static std::array<AttributeDef, 3> attributeDefs;
};

// number of elements (bytes / sizeof(float)) per vertex
constexpr size_t VA_SIZE_0     = (sizeof(VA_TYPE_0) / sizeof(float));
constexpr size_t VA_SIZE_C     = (sizeof(VA_TYPE_C) / sizeof(float));
constexpr size_t VA_SIZE_N     = (sizeof(VA_TYPE_N) / sizeof(float));
constexpr size_t VA_SIZE_T     = (sizeof(VA_TYPE_T) / sizeof(float));
constexpr size_t VA_SIZE_TN    = (sizeof(VA_TYPE_TN) / sizeof(float));
constexpr size_t VA_SIZE_TC    = (sizeof(VA_TYPE_TC) / sizeof(float));
constexpr size_t VA_SIZE_T4C   = (sizeof(VA_TYPE_PROJ) / sizeof(float));
constexpr size_t VA_SIZE_TNT   = (sizeof(VA_TYPE_TNT) / sizeof(float));
constexpr size_t VA_SIZE_2D0   = (sizeof(VA_TYPE_2D0) / sizeof(float));
constexpr size_t VA_SIZE_2DT   = (sizeof(VA_TYPE_2DT) / sizeof(float));
constexpr size_t VA_SIZE_2DTC  = (sizeof(VA_TYPE_2DTC) / sizeof(float));