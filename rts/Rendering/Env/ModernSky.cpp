#include "ModernSky.h"

#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/Shaders/Shader.h"
#include "Rendering/Shaders/ShaderHandler.h"
#include "Rendering/Env/DebugCubeMapTexture.h"
#include "Rendering/Env/WaterRendering.h"
#include "Game/Camera.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Map/MapInfo.h"

CModernSky::CModernSky()
{
#ifndef HEADLESS
	skyShader  = shaderHandler->CreateProgramObject("[ModernSky]", "Sky");
	skyShader->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/ModernSkyVS.glsl", "", GL_VERTEX_SHADER));
	skyShader->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/ModernSkyFS.glsl", "", GL_FRAGMENT_SHADER));
	skyShader->Link();
	skyShader->Enable();
	skyShader->Disable();
	skyShader->Validate();
#endif
}

CModernSky::~CModernSky()
{
#ifndef HEADLESS
	shaderHandler->ReleaseProgramObject("[ModernSky]", "Sky");
#endif
}

void CModernSky::Draw()
{
#ifndef HEADLESS
	if (!globalRendering->drawSky)
		return;

	// FFP
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_BLEND);
	glDepthFunc(GL_LEQUAL);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	CMatrix44f view = camera->GetViewMatrix();
	view.SetPos(float3());
	glLoadMatrixf(view);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadMatrixf(camera->GetProjectionMatrix());

	vao.Bind();
	//assert(skyShader->IsValid());
	skyShader->Enable();

	const float4& sunDir = skyLight->GetLightDir();
	skyShader->SetUniform("sunDir", sunDir.x, sunDir.y, sunDir.z);

	const float3& skyDir = mapInfo->atmosphere.skyDir;
	skyShader->SetUniform("skyDir", skyDir.x, skyDir.y, skyDir.z);

	const float3& cloudColor = mapInfo->atmosphere.cloudColor;
	skyShader->SetUniform("cloudInfo", cloudColor.x, cloudColor.y, cloudColor.z, mapInfo->atmosphere.cloudDensity);

	const float3& scatterInfo = mapInfo->atmosphere.scatterInfo;
	skyShader->SetUniform("scatterInfo", scatterInfo.x, scatterInfo.y, scatterInfo.z);
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