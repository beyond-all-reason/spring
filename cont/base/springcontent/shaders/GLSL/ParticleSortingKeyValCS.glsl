#version 430 core

struct TriangleData
{
    vec4 pos;
    vec4 uvw;
    vec4 uvInfo;
	vec4 apAndCol;
};

layout(std430, binding = VERT_SSBO_BINDING_IDX) restrict readonly buffer OUT1
{
    TriangleData triangleData[];
};

layout(std430, binding = SIZE_SSBO_BINDING_IDX) restrict readonly buffer SIZE
{
    uint atomicCounters[];
};

layout(std430, binding = KEYO_SSBO_BINDING_IDX) restrict writeonly buffer KEYS
{
    uint keysOut[];
};

layout(std430, binding = VALO_SSBO_BINDING_IDX) restrict writeonly buffer VALS
{
    uint valsOut[];
};

uniform vec3 cameraPos;
uniform vec3 cameraFwd;
uniform vec2 cameraNearFar;

shared uint totalNumQuads;

layout(local_size_x = WORKGROUP_SIZE) in;
void main()
{
	if (gl_LocalInvocationID.x == 0u)
		totalNumQuads = atomicCounters[SIZE_SSBO_QUAD_IDX];
	
	barrier();
	memoryBarrierShared();
	
	uint quadIdx = gl_GlobalInvocationID.x;

	if (quadIdx >= totalNumQuads)
		return;

	// microoptimization to localize the data
	vec4 vertexPositions[4u];
	for (uint j = 0u; j < 4u; ++j) {
		vertexPositions[j] = triangleData[4u * quadIdx + j].pos;
	}

	uint drawOrder = uint(clamp(floatBitsToInt(vertexPositions[0u].w), 0, 255));
#ifdef PROCESS_TRIANGLES
	vec3 trCenter1 = (vertexPositions[3u].xyz + vertexPositions[0u].xyz + vertexPositions[1u].xyz) * 0.333333333;
	vec3 trCenter2 = (vertexPositions[3u].xyz + vertexPositions[1u].xyz + vertexPositions[2u].xyz) * 0.333333333;

#ifdef USE_PROJECTED_DISTANCE
	vec2 dist = vec2(
		dot(cameraFwd, trCenter1 - cameraPos),
		dot(cameraFwd, trCenter2 - cameraPos)
	);
#else
	vec2 dist = vec2(
		distance(cameraPos, trCenter1),
		distance(cameraPos, trCenter2)
	);
#endif

	dist = clamp((dist - cameraNearFar.xx) / (cameraNearFar.yy - cameraNearFar.xx), vec2(0), vec2(1));
	
	uvec2 key = uvec2(0xFFFFFFu * dist); // save in 24 bit depth format
	key |= uvec2(drawOrder << 24u); // add drawOrder as MSB of UINT
	
	keysOut[2u * quadIdx + 0u] = 0xFFFFFFFFu - key[0u];
	keysOut[2u * quadIdx + 1u] = 0xFFFFFFFFu - key[1u];
	valsOut[2u * quadIdx + 0u] = 2u * quadIdx + 0u;
	valsOut[2u * quadIdx + 1u] = 2u * quadIdx + 1u;
#else
	vec3 quadCenter = (vertexPositions[0u].xyz + vertexPositions[1u].xyz + vertexPositions[2u].xyz + vertexPositions[3u].xyz) * 0.25;

#ifdef USE_PROJECTED_DISTANCE
	float dist = dot(cameraFwd, quadCenter - cameraPos);
#else
	float dist = distance(cameraPos, quadCenter);
#endif

	dist = clamp((distance - cameraNearFar.x) / (cameraNearFar.y - cameraNearFar.x), 0.0, 1.0);
	
	uint key = uint(0xFFFFFFu * dist); // save in 24 bit depth format
	key |= uint(drawOrder << 24u); // add drawOrder as MSB of UINT
	keysOut[quadIdx] = 0xFFFFFFFFu - key;
	valsOut[quadIdx] = quadIdx;
#endif
}