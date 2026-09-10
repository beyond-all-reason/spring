--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    gui_buildsquare_gl4.lua
--  brief:   Example widget demonstrating DrawBuildSquare callin with GL4 rendering
--  author:  RecoilEngine contributors
--
--  Demonstrates:
--    - Using the DrawBuildSquare callin to receive per-cell build placement data
--    - Using Spring.SetEngineBuildSquareRendering(false) to replace engine rendering
--    - Using GL4 VAO/VBO/shader via raw engine API (gl.CreateShader, gl.GetVBO, gl.GetVAO)
--
--  License: GNU GPL, v2 or later
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function widget:GetInfo()
	return {
		name = "BuildSquare GL4 Example",
		desc = "GL4 custom build placement grid rendering via DrawBuildSquare callin",
		author = "RecoilEngine contributors",
		date = "2026",
		license = "GNU GPL, v2 or later",
		layer = 0,
		enabled = false,
	}
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
-- Localized API

local glCreateShader = gl.CreateShader
local glDeleteShader = gl.DeleteShader
local glUseShader = gl.UseShader
local glGetShaderLog = gl.GetShaderLog
local glGetUniformLocation = gl.GetUniformLocation
local glUniform = gl.Uniform
local glGetVBO = gl.GetVBO
local glGetVAO = gl.GetVAO
local glGetEngineUniformBufferDef = gl.GetEngineUniformBufferDef
local spGetGameFrame = Spring.GetGameFrame
local spSetEngineBuildSquareRendering = Spring.SetEngineBuildSquareRendering
		or function() end
local GL_ARRAY_BUFFER = GL.ARRAY_BUFFER
local GL_ELEMENT_ARRAY_BUFFER = GL.ELEMENT_ARRAY_BUFFER
local GL_TRIANGLES = GL.TRIANGLES
local GL_FLOAT = GL.FLOAT
local GL_UNSIGNED_SHORT = GL.UNSIGNED_SHORT

local SQUARE_SIZE = 8

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
-- Configuration

local STATUS_BLOCKED = 0
local STATUS_OCCUPIED = 1
local STATUS_RECLAIMABLE = 2
local STATUS_OPEN = 3

local CELL_INSET = 1.5

local STATUS_COLORS = {
	[STATUS_BLOCKED]     = { 1.0, 0.1, 0.3, 0.7 },
	[STATUS_OCCUPIED]    = { 1.0, 0.6, 0.0, 0.7 },
	[STATUS_RECLAIMABLE] = { 0.0, 0.7, 1.0, 0.7 },
	[STATUS_OPEN]        = { 0.0, 1.0, 0.6, 0.6 },
}

local HEIGHT_OFFSET = 0.5

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
-- State

local shaderProgram = nil
local shaderObj = nil
local quadVBO = nil
local quadIndexVBO = nil
local instanceVBO = nil
local vao = nil
local heightmapTexLoc = nil

local MAX_CELLS = 4096

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
-- Shader sources

local vsSrc = [[
#version 420
#extension GL_ARB_uniform_buffer_object : require
#extension GL_ARB_shading_language_420pack: require

layout (location = 0) in vec2 a_cornerPos;
layout (location = 1) in vec4 a_cellData;
layout (location = 2) in vec4 a_color;

//__ENGINEUNIFORMBUFFERDEFS__

out vec4 v_color;
flat out float v_status;

uniform sampler2D heightmapTex;
uniform float heightOffset;
uniform float cellInset;

vec2 heightmapUVatWorldPos(vec2 worldpos) {
	vec2 inverseMapSize = vec2(1.0) / mapSize.xy;
	vec2 heightmaptexel = vec2(8.0, 8.0);
	worldpos += vec2(-8.0, -8.0) * (worldpos * inverseMapSize) + vec2(4.0, 4.0);
	vec2 uvhm = clamp(worldpos, heightmaptexel, mapSize.xy - heightmaptexel);
	return uvhm * inverseMapSize;
}

void main() {
	vec2 insetCorner = a_cornerPos + vec2(cellInset);
	float wx = a_cellData.x + insetCorner.x;
	float wz = a_cellData.y + insetCorner.y;
	vec2 uvhm = heightmapUVatWorldPos(vec2(wx, wz));
	float wy = textureLod(heightmapTex, uvhm, 0.0).x + heightOffset;

	v_color = a_color;
	v_status = a_cellData.z;
	gl_Position = cameraViewProj * vec4(wx, wy, wz, 1.0);
}
]]

local fsSrc = [[
#version 420
#extension GL_ARB_uniform_buffer_object : require
#extension GL_ARB_shading_language_420pack: require

//__ENGINEUNIFORMBUFFERDEFS__

in vec4 v_color;
flat in float v_status;
out vec4 fragColor;

void main() {
	fragColor = v_color;
}
]]

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
-- Initialization / cleanup

local function goodbye(reason)
	Spring.Echo("BuildSquare GL4 Example widget exiting: " .. reason)
	widgetHandler:RemoveWidget()
end

local function initGL4Resources()
	local uboMatDefs = glGetEngineUniformBufferDef(0)
	local uboParamDefs = glGetEngineUniformBufferDef(1)
	local uboDefs = uboMatDefs .. "\n" .. uboParamDefs

	local vsProcessed = vsSrc:gsub("//__ENGINEUNIFORMBUFFERDEFS__", uboDefs)
	local fsProcessed = fsSrc:gsub("//__ENGINEUNIFORMBUFFERDEFS__", uboDefs)

	local shaderID, programID = glCreateShader({
		vertex = vsProcessed,
		fragment = fsProcessed,
		uniformInt = {
			heightmapTex = 0,
		},
		uniformFloat = {
			heightOffset = HEIGHT_OFFSET,
			cellInset = CELL_INSET,
		},
	})

	if not shaderID then
		goodbye("Failed to compile shader: " .. (glGetShaderLog() or "unknown error"))
		return false
	end

	shaderProgram = shaderID
	shaderObj = programID
	heightmapTexLoc = glGetUniformLocation(shaderID, "heightmapTex")

	local quadVerts = {
		0.0, 0.0,
		SQUARE_SIZE, 0.0,
		SQUARE_SIZE, SQUARE_SIZE,
		0.0, SQUARE_SIZE,
	}

	quadVBO = glGetVBO(GL_ARRAY_BUFFER, false)
	quadVBO:Define(4, {
		{ id = 0, name = "a_cornerPos", size = 2 },
	})
	quadVBO:Upload(quadVerts)

	local quadIndices = { 0, 1, 2, 0, 2, 3 }
	quadIndexVBO = glGetVBO(GL_ELEMENT_ARRAY_BUFFER, false)
	quadIndexVBO:Define(6, GL_UNSIGNED_SHORT)
	quadIndexVBO:Upload(quadIndices)

	instanceVBO = glGetVBO(GL_ARRAY_BUFFER, true)
	instanceVBO:Define(MAX_CELLS, {
		{ id = 1, name = "a_cellData", size = 4 },
		{ id = 2, name = "a_color",    size = 4 },
	})

	vao = glGetVAO()
	vao:AttachVertexBuffer(quadVBO)
	vao:AttachInstanceBuffer(instanceVBO)
	vao:AttachIndexBuffer(quadIndexVBO)

	return true
end

local function freeGL4Resources()
	if vao then
		vao:Delete()
		vao = nil
	end
	if instanceVBO then
		instanceVBO:Delete()
		instanceVBO = nil
	end
	if quadIndexVBO then
		quadIndexVBO:Delete()
		quadIndexVBO = nil
	end
	if quadVBO then
		quadVBO:Delete()
		quadVBO = nil
	end
	if shaderProgram then
		glDeleteShader(shaderProgram)
		shaderProgram = nil
		shaderObj = nil
	end
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
-- Widget callbacks

function widget:Initialize()
	if not glCreateShader then
		goodbye("Shaders not supported")
		return
	end

	if not initGL4Resources() then
		return
	end

	spSetEngineBuildSquareRendering(false)
end

function widget:Shutdown()
	spSetEngineBuildSquareRendering(true)
	freeGL4Resources()
end

function widget:DrawBuildSquare(unitDefID, x, z, facing, statuses)
	local ud = UnitDefs[unitDefID]
	if not ud then
		return
	end

	local xsize = ((facing % 2) == 0) and ud.xsize or ud.zsize
	local zsize = ((facing % 2) == 1) and ud.xsize or ud.zsize

	local numCells = xsize * zsize
	if numCells <= 0 or numCells > MAX_CELLS then
		return
	end

	local sx = math.floor(x / SQUARE_SIZE) - math.floor(xsize / 2)
	local sz = math.floor(z / SQUARE_SIZE) - math.floor(zsize / 2)

	local instanceData = {}
	for zi = 0, zsize - 1 do
		for xi = 0, xsize - 1 do
			local cellIdx = zi * xsize + xi
			local status = statuses[cellIdx + 1] or 0
			local color = STATUS_COLORS[status] or STATUS_COLORS[STATUS_BLOCKED]

			local wx = (sx + xi) * SQUARE_SIZE
			local wz = (sz + zi) * SQUARE_SIZE

			instanceData[#instanceData + 1] = wx
			instanceData[#instanceData + 1] = wz
			instanceData[#instanceData + 1] = status
			instanceData[#instanceData + 1] = 0.0

			instanceData[#instanceData + 1] = color[1]
			instanceData[#instanceData + 1] = color[2]
			instanceData[#instanceData + 1] = color[3]
			instanceData[#instanceData + 1] = color[4]
		end
	end

	instanceVBO:Upload(instanceData)

	gl.Texture(0, "$heightmap")
	gl.DepthTest(false)
	gl.Blending(true)

	glUseShader(shaderProgram)
	vao:DrawElements(GL_TRIANGLES, 6, 0, numCells)
	glUseShader(0)

	gl.Texture(0, false)
	gl.DepthTest(false)
	gl.Blending(false)
end
