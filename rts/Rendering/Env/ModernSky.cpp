#include "ModernSky.h"

#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/Shaders/Shader.h"
#include "Rendering/Shaders/ShaderHandler.h"
#include "Rendering/Env/DebugCubeMapTexture.h"
#include "Rendering/Env/WaterRendering.h"
#include "System/StringUtil.h"
#include "Game/Game.h"
#include "Game/Camera.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"

#include <tracy/Tracy.hpp>

CModernSky::CModernSky()
{
	valid = true;
#ifndef HEADLESS
	for (size_t i = 0; i < 2; ++i) {
		skyShaders[i] = shaderHandler->CreateProgramObject("[ModernSky]", "Sky-" + IntToString(i));
		skyShaders[i]->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/ModernSkyVS.glsl", "", GL_VERTEX_SHADER));
		skyShaders[i]->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/ModernSkyFS.glsl", "", GL_FRAGMENT_SHADER));
		skyShaders[i]->SetFlag("SIMPLIFIED_RENDERING", static_cast<int>(i == 1));
		skyShaders[i]->Link();
		skyShaders[i]->Enable();
		skyShaders[i]->Disable();
		valid &= skyShaders[i]->Validate();
	}
#endif
}

CModernSky::~CModernSky()
{
#ifndef HEADLESS
	shaderHandler->ReleaseProgramObjects("[ModernSky]");
#endif
}

void CModernSky::Draw()
{
	//ZoneScoped;
#ifndef HEADLESS
	if (!globalRendering->drawSky)
		return;

	if (!valid)
		return;

	// FFP
	glDisable(GL_ALPHA_TEST);
	//glDisable(GL_BLEND);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthFunc(GL_LEQUAL);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	const CMatrix44f& view = camera->GetViewMatrix();
	//view.SetPos(float3());
	glLoadMatrixf(view);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadMatrixf(camera->GetProjectionMatrix());

	vao.Bind();
	//assert(skyShader->IsValid());

	auto* skyShader = skyShaders[game->GetDrawMode() != CGame::GameDrawMode::gameNormalDraw];

	skyShader->Enable();

	const float3 midMap{ static_cast<float>(SQUARE_SIZE * mapDims.mapx >> 1), 0.0f, static_cast<float>(SQUARE_SIZE * mapDims.mapy >> 1) };
	skyShader->SetUniform("midMap", midMap.x, midMap.y, midMap.z);

	const float4& sunDir = skyLight->GetLightDir();
	skyShader->SetUniform("sunDir", sunDir.x, sunDir.y, sunDir.z);

	const float3& cloudColor = mapInfo->atmosphere.cloudColor;
	skyShader->SetUniform("cloudInfo", cloudColor.x, cloudColor.y, cloudColor.z, mapInfo->atmosphere.cloudDensity);

	const float3& skyColor = mapInfo->atmosphere.skyColor;
	skyShader->SetUniform("skyColor", skyColor.x, skyColor.y, skyColor.z);

	const float3& fogColor = mapInfo->atmosphere.fogColor;
	skyShader->SetUniform("fogColor", fogColor.x, fogColor.y, fogColor.z);

	skyShader->SetUniform("planeColor",
		waterRendering->planeColor.x,
		waterRendering->planeColor.y,
		waterRendering->planeColor.z,
		static_cast<float>(waterRendering->hasWaterPlane && !globalRendering->drawDebugCubeMap)
	);

	skyShader->SetUniform("time", (static_cast<float>(gs->frameNum) + globalRendering->timeOffset) * 0.005f);

	glDrawArrays(GL_TRIANGLES, 0, 36);

	skyShader->Disable();
	vao.Unbind();

	// FFP
	// glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	sky->SetupFog();
#endif
}