--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    api_compute_indirect_draw.lua
--  brief:   Compute-culled sphere grid, drawn via indirect draw
--  author:  RecoilEngine contributors
--
--  Demonstrates:
--    - A static world-space grid of spheres, frustum-culled by a compute shader
--    - The compute shader appends surviving instances into a compacted SSBO
--      with an atomic counter, and writes that count into the indirect
--      draw command, so only the surviving instances are actually drawn
--    - Applying shader-storage and command memory barriers between compute
--      and the indirect draw
--
--  License: GNU GPL, v2 or later
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function widget:GetInfo()
	return {
		name = "Compute Frustum Culling Grid",
		desc = "Sphere grid culled and compacted by a compute-shader view frustum test",
		author = "RecoilEngine contributors",
		date = "2026",
		license = "GNU GPL, v2 or later",
		layer = 0,
		enabled = false,
	}
end

local spGetGroundHeight = Spring.GetGroundHeight

local glCreateShader = gl.CreateShader
local glDeleteShader = gl.DeleteShader
local glDispatchCompute = gl.DispatchCompute
local glGetShaderLog = gl.GetShaderLog
local glGetVAO = gl.GetVAO
local glGetVBO = gl.GetVBO
local glUseShader = gl.UseShader
local glCulling = gl.Culling
local glDepthTest = gl.DepthTest
local glDepthMask = gl.DepthMask
local glBlending = gl.Blending

local GL_SHADER_STORAGE_BARRIER_BIT = GL.SHADER_STORAGE_BARRIER_BIT
local GL_COMMAND_BARRIER_BIT = GL.COMMAND_BARRIER_BIT
local GL_SHADER_STORAGE_BUFFER = GL.SHADER_STORAGE_BUFFER
local GL_TRIANGLES = GL.TRIANGLES
local GL_UNSIGNED_INT_VEC4 = GL.UNSIGNED_INT_VEC4

-- Grid layout: spheres are spaced along the X and Z axes, centered on the map
local GRID_RADIUS = 8 -- instances extend this many steps outward on each side
local GRID_SIDE = GRID_RADIUS * 2 + 1
local INSTANCE_COUNT = GRID_SIDE * GRID_SIDE
local GRID_SPACING = 220 -- elmos between adjacent sphere centers
local SPHERE_RADIUS = 50
local SPHERE_HEIGHT_OFFSET = 80 -- elmos above the ground at each sphere's XZ

local INSTANCE_BUFFER_BINDING = 4 -- static full grid, readonly
local VISIBLE_BUFFER_BINDING = 5 -- compacted survivors, written by the compute shader
local COMMAND_BUFFER_BINDING = 6 -- indirect draw command, instanceCount is an atomic counter

local COMPUTE_LOCAL_SIZE = 64
local computeGroups = math.ceil(INSTANCE_COUNT / COMPUTE_LOCAL_SIZE)

local renderShader = nil
local computeShader = nil
local sphereVBO = nil
local sphereIndexVBO = nil
local sphereNumIndices = nil
local posRadiusVBO = nil
local visibleVBO = nil
local commandVBO = nil
local vao = nil

local renderVertexSource = [[
#version 430 core

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;
layout(location = 2) in vec2 vertexUV;

layout(std430, binding = 5) readonly buffer VisibleBuffer {
	vec4 visibleInstances[]; // xyz = world center, w = radius; compacted by the compute shader
};

//__ENGINEUNIFORMBUFFERDEFS__

out vec3 worldNormal;

void main()
{
	vec4 posRadius = visibleInstances[gl_InstanceID];
	vec3 worldPos = vertexPosition * posRadius.w + posRadius.xyz;

	gl_Position = cameraViewProj * vec4(worldPos, 1.0);
	worldNormal = vertexNormal;
}
]]

local renderFragmentSource = [[
#version 430 core

in vec3 worldNormal;

out vec4 fragmentColor;

void main()
{
	// constant tint: seeing red confirms this instance survived the compute shader's culling pass
	const vec3 keptColor = vec3(1.0, 0.15, 0.1);
	float shading = clamp(dot(normalize(worldNormal), normalize(vec3(0.4, 0.8, 0.4))), 0.35, 1.0);
	fragmentColor = vec4(keptColor * shading, 1.0);
}
]]

local computeSource = [[
#version 430 core

layout(local_size_x = ]] .. COMPUTE_LOCAL_SIZE .. [[, local_size_y = 1, local_size_z = 1) in;

layout(std430, binding = 4) readonly buffer InstanceBuffer {
	vec4 instancePosRadius[]; // xyz = world center, w = radius
};

layout(std430, binding = 5) writeonly buffer VisibleBuffer {
	vec4 visibleInstances[];
};

layout(std430, binding = 6) buffer CommandBuffer {
	uint count;
	uint instanceCount; // reset to 0 by Lua each frame, then used as an atomic append counter
	uint firstIndex;
	int baseVertex;
	uint baseInstance;
};

//__ENGINEUNIFORMBUFFERDEFS__

const uint instanceCountTotal = ]] .. INSTANCE_COUNT .. [[u;

shared vec4 sharedPlanes[6];

// Gribb/Hartmann plane extraction from the combined view-projection matrix
void ExtractFrustumPlanes(out vec4 planes[6])
{
	mat4 m = cameraViewProj;
	planes[0] = vec4(m[0][3] + m[0][0], m[1][3] + m[1][0], m[2][3] + m[2][0], m[3][3] + m[3][0]); // left
	planes[1] = vec4(m[0][3] - m[0][0], m[1][3] - m[1][0], m[2][3] - m[2][0], m[3][3] - m[3][0]); // right
	planes[2] = vec4(m[0][3] + m[0][1], m[1][3] + m[1][1], m[2][3] + m[2][1], m[3][3] + m[3][1]); // bottom
	planes[3] = vec4(m[0][3] - m[0][1], m[1][3] - m[1][1], m[2][3] - m[2][1], m[3][3] - m[3][1]); // top
	planes[4] = vec4(m[0][3] + m[0][2], m[1][3] + m[1][2], m[2][3] + m[2][2], m[3][3] + m[3][2]); // near
	planes[5] = vec4(m[0][3] - m[0][2], m[1][3] - m[1][2], m[2][3] - m[2][2], m[3][3] - m[3][2]); // far

	for (int i = 0; i < 6; i++) {
		planes[i] /= length(planes[i].xyz);
	}
}

void main()
{
	if (gl_LocalInvocationIndex == 0u) {
		vec4 planes[6];
		ExtractFrustumPlanes(planes);
		for (int i = 0; i < 6; i++) {
			sharedPlanes[i] = planes[i];
		}
	}
	barrier();

	uint index = gl_GlobalInvocationID.x;
	if (index >= instanceCountTotal) {
		return;
	}

	vec4 posRadius = instancePosRadius[index];

	bool visible = true;
	for (int i = 0; i < 6; i++) {
		float distance = dot(sharedPlanes[i].xyz, posRadius.xyz) + sharedPlanes[i].w;
		if (distance < -posRadius.w) {
			visible = false;
			break;
		}
	}

	// culled instances are simply never appended, so only survivors get drawn
	if (visible) {
		uint slot = atomicAdd(instanceCount, 1u);
		visibleInstances[slot] = posRadius;
		visibleInstances[slot].w = posRadius.w * fract(float(slot) * 135.021);
	}
}
]]

local function freeResources()
	if vao then
		vao:Delete()
		vao = nil
	end

	if commandVBO then
		commandVBO:Delete()
	end
	if visibleVBO then
		visibleVBO:Delete()
	end
	if posRadiusVBO then
		posRadiusVBO:Delete()
	end
	if sphereIndexVBO then
		sphereIndexVBO:Delete()
	end
	if sphereVBO then
		sphereVBO:Delete()
	end
	commandVBO = nil
	visibleVBO = nil
	posRadiusVBO = nil
	sphereIndexVBO = nil
	sphereVBO = nil

	if computeShader then
		glDeleteShader(computeShader)
		computeShader = nil
	end
	if renderShader then
		glDeleteShader(renderShader)
		renderShader = nil
	end
end

local function fail(reason)
	Spring.Echo("Compute Frustum Culling Grid: " .. reason)
	freeResources()
	widgetHandler:RemoveWidget()
end

local function buildInstanceGrid()
	local mapCenterX = Game.mapSizeX * 0.5
	local mapCenterZ = Game.mapSizeZ * 0.5
	local data = {}
	local n = 0
	for gz = -GRID_RADIUS, GRID_RADIUS do
		for gx = -GRID_RADIUS, GRID_RADIUS do
			local worldX = mapCenterX + gx * GRID_SPACING
			local worldZ = mapCenterZ + gz * GRID_SPACING
			local worldY = spGetGroundHeight(worldX, worldZ) + SPHERE_HEIGHT_OFFSET
			data[n + 1] = worldX
			data[n + 2] = worldY
			data[n + 3] = worldZ
			data[n + 4] = SPHERE_RADIUS
			n = n + 4
		end
	end
	return data
end

local function initResources()
	local engineUniformBufferDefs = gl.LuaShader.GetEngineUniformBufferDefs()
	local vertexSource = renderVertexSource:gsub("//__ENGINEUNIFORMBUFFERDEFS__", engineUniformBufferDefs)
	local fragmentSource = renderFragmentSource
	local finalComputeSource = computeSource:gsub("//__ENGINEUNIFORMBUFFERDEFS__", engineUniformBufferDefs)

	renderShader = glCreateShader({
		vertex = vertexSource,
		fragment = fragmentSource,
	})
	if not renderShader then
		return false, "render shader failed: " .. (glGetShaderLog() or "unknown error")
	end

	computeShader = glCreateShader({ compute = finalComputeSource })
	if not computeShader then
		return false, "compute shader failed: " .. (glGetShaderLog() or "unknown error")
	end

	sphereVBO, _, sphereIndexVBO, sphereNumIndices = gl.InstanceVBOTable.makeSphereVBO(16, 12, 1)
	posRadiusVBO = glGetVBO(GL_SHADER_STORAGE_BUFFER, true)
	visibleVBO = glGetVBO(GL_SHADER_STORAGE_BUFFER, true)
	commandVBO = glGetVBO(GL_SHADER_STORAGE_BUFFER, true)
	vao = glGetVAO()

	if not (sphereVBO and sphereIndexVBO and posRadiusVBO and visibleVBO and commandVBO and vao) then
		return false, "required GL4 VAO/VBO functionality is unavailable"
	end

	posRadiusVBO:Define(INSTANCE_COUNT, {
		{ id = 0, name = "instancePosRadius", size = 1, type = GL.FLOAT_VEC4 },
	})
	posRadiusVBO:Upload(buildInstanceGrid())

	-- capacity for the worst case where every instance survives culling
	visibleVBO:Define(INSTANCE_COUNT, {
		{ id = 0, name = "visibleInstances", size = 1, type = GL.FLOAT_VEC4 },
	})

	-- Two uvec4 elements provide 32 bytes. The command occupies the first 20.
	commandVBO:Define(2, {
		{ id = 0, name = "indirectCommand", size = 1, type = GL_UNSIGNED_INT_VEC4 },
	})
	commandVBO:Upload({
		sphereNumIndices, -- count
		0, -- instanceCount, filled in every frame by the compute shader
		0, -- firstIndex
		0, -- baseVertex
		0, -- baseInstance
		0,
		0,
		0,
	})

	vao:AttachVertexBuffer(sphereVBO)
	vao:AttachIndexBuffer(sphereIndexVBO)

	return true
end

function widget:Initialize()
	if not (glCreateShader and glDispatchCompute and glGetVAO and glGetVBO and gl.InstanceVBOTable) then
		fail("required GL4 API is unavailable")
		return
	end

	local success, reason = initResources()
	if not success then
		fail(reason)
	end
end

function widget:Shutdown()
	freeResources()
end

function widget:DrawWorld()
	-- reset the atomic append counter but keep the fixed mesh index count
	commandVBO:Upload({ sphereNumIndices, 0, 0, 0, 0, 0, 0, 0 })

	posRadiusVBO:BindBufferRange(INSTANCE_BUFFER_BINDING)
	visibleVBO:BindBufferRange(VISIBLE_BUFFER_BINDING)
	commandVBO:BindBufferRange(COMMAND_BUFFER_BINDING)

	glUseShader(computeShader)
	glDispatchCompute(computeGroups, 1, 1, GL_SHADER_STORAGE_BARRIER_BIT + GL_COMMAND_BARRIER_BIT)
	glUseShader(0)

	glDepthTest(true)
	glDepthMask(true)
	glBlending(false)
	glCulling(GL.BACK)

	glUseShader(renderShader)
	vao:DrawElementsIndirect(GL_TRIANGLES, commandVBO)
	glUseShader(0)

	glCulling(false)
	glDepthMask(false)
	glDepthTest(false)

	commandVBO:UnbindBufferRange(COMMAND_BUFFER_BINDING)
	visibleVBO:UnbindBufferRange(VISIBLE_BUFFER_BINDING)
	posRadiusVBO:UnbindBufferRange(INSTANCE_BUFFER_BINDING)
end
