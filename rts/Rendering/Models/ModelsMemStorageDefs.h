#pragma once

#include "System/float4.h"
#include "Sim/Misc/GlobalConstants.h"

class alignas(16) ModelUniformData {
public:
	CR_DECLARE_STRUCT(ModelUniformData)
	static constexpr int MAX_MODEL_UD_UNIFORMS = 16;

	union {
		uint32_t composite;
		struct {
			uint8_t drawFlag;
			uint8_t unused1;
			uint16_t id;
		};
	};

	uint32_t unused2;
	uint32_t unused3;
	uint32_t unused4;

	float maxHealth;
	float health;
	float unused5;
	float unused6;

	float4 drawPos;
	float4 speed;

	float userDefined[MAX_MODEL_UD_UNIFORMS];
public:
	static void Init() { SetGLSLDefinition(1); }
	static const std::string& GetGLSLDefinition() { return glslDefinition; }
private:
	static void SetGLSLDefinition(int binding);
	static inline std::string glslDefinition;
};

static_assert(sizeof(ModelUniformData) == 128, "");
static_assert(alignof(ModelUniformData) == 16, "");
static_assert(sizeof(ModelUniformData::userDefined) % 4 == 0, ""); //due to GLSL std140 userDefined must be a multiple of 4
