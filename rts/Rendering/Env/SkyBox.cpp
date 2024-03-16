/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <vector>
#include <algorithm>

#include "SkyBox.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/FBO.h"
#include "Rendering/Shaders/Shader.h"
#include "Rendering/Shaders/ShaderHandler.h"
#include "Rendering/Textures/Bitmap.h"
#include "Rendering/Env/DebugCubeMapTexture.h"
#include "Rendering/Env/WaterRendering.h"
#include "Game/Game.h"
#include "Game/Camera.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "System/Exceptions.h"
#include "System/ScopedResource.h"
#include "System/float3.h"
#include "System/type2.h"
#include "System/Color.h"
#include "System/Config/ConfigHandler.h"
#include "System/Log/ILog.h"

#include <tracy/Tracy.hpp>

#define LOG_SECTION_SKY_BOX "SkyBox"
LOG_REGISTER_SECTION_GLOBAL(LOG_SECTION_SKY_BOX)

// use the specific section for all LOG*() calls in this source file
#ifdef LOG_SECTION_CURRENT
	#undef LOG_SECTION_CURRENT
#endif
#define LOG_SECTION_CURRENT LOG_SECTION_SKY_BOX

void CSkyBox::Init(uint32_t textureID, uint32_t xsize, uint32_t ysize, bool convertToCM)
{
	//ZoneScoped;
	shader = nullptr;
#ifndef HEADLESS
	if (textureID == 0)
		return;

	if (convertToCM) {
		auto generateMipMaps = configHandler->GetBool("CubeTexGenerateMipMaps");
		// here textureID represents 2D texture

		auto cubeTexID = spring::ScopedResource(
			[]() { uint32_t tempID = 0; glGenTextures(1, &tempID); return tempID; }(),
			[](uint32_t texID) { if (texID > 0) glDeleteTextures(1, &texID); }
		);

		glEnable(GL_TEXTURE_CUBE_MAP);

		glBindTexture(GL_TEXTURE_CUBE_MAP, cubeTexID);

		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, generateMipMaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);


		for (GLenum glFace = GL_TEXTURE_CUBE_MAP_POSITIVE_X; glFace <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z; ++glFace) {
			glTexImage2D(glFace, 0, GL_RGBA8, ysize, ysize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		}

		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

		FBO fbo;
		fbo.Init(false);

		if (!fbo.IsValid())
			return;

		fbo.Bind();

		auto* ercShader = shaderHandler->CreateProgramObject("[SkyBox]", "EquiRectConverter");
		ercShader->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/CubeMapVS.glsl", "", GL_VERTEX_SHADER));
		ercShader->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/EquiRectConverterFS.glsl", "", GL_FRAGMENT_SHADER));
		ercShader->Link();
		ercShader->Enable();
		ercShader->SetUniform("tex", 0);
		ercShader->SetUniform("uvFlip", 1.0f, 1.0f, 1.0f);
		ercShader->Disable();

		if (!ercShader->Validate()) {
			fbo.DetachAll();
			FBO::Unbind();
			cubeTexID = nullptr;
			shaderHandler->ReleaseProgramObject("[SkyBox]", "EquiRectConverter");
			return;
		}

		valid = true;
		{
			glPushAttrib(GL_ENABLE_BIT | GL_VIEWPORT_BIT);

			glViewport(0, 0, ysize, ysize);

			glDisable(GL_DEPTH_TEST);
			glDisable(GL_BLEND);

			glMatrixMode(GL_PROJECTION);
			glPushMatrix();
			glLoadIdentity();

			glMatrixMode(GL_MODELVIEW);
			glPushMatrix();

			static constexpr std::array viewMatParams = {
				std::pair{ float3( 1.0f,  0.0f,  0.0f), float3(0.0f, -1.0f,  0.0f) }, // GL_TEXTURE_CUBE_MAP_POSITIVE_X
				std::pair{ float3(-1.0f,  0.0f,  0.0f), float3(0.0f, -1.0f,  0.0f) }, // GL_TEXTURE_CUBE_MAP_NEGATIVE_X
				std::pair{ float3( 0.0f,  1.0f,  0.0f), float3(0.0f,  0.0f,  1.0f) }, // GL_TEXTURE_CUBE_MAP_POSITIVE_Y
				std::pair{ float3( 0.0f, -1.0f,  0.0f), float3(0.0f,  0.0f, -1.0f) }, // GL_TEXTURE_CUBE_MAP_NEGATIVE_Y
				std::pair{ float3( 0.0f,  0.0f,  1.0f), float3(0.0f, -1.0f,  0.0f) }, // GL_TEXTURE_CUBE_MAP_POSITIVE_Z
				std::pair{ float3( 0.0f,  0.0f, -1.0f), float3(0.0f, -1.0f,  0.0f) }  // GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
			};

			VAO vao;
			vao.Bind();
			ercShader->Enable();
			for (int side = 0; side < 6; ++side) {
				fbo.AttachTexture(cubeTexID, GL_TEXTURE_CUBE_MAP_POSITIVE_X + side, GL_COLOR_ATTACHMENT0);
				valid &= fbo.CheckStatus("SKYBOX-EQUIRECT-CONVERT");
				if (!valid)
					continue;

				CMatrix44f viewMat = CMatrix44f::LookAtView(
					float3(),
					viewMatParams[side].first,
					viewMatParams[side].second
				);
				glLoadMatrixf(viewMat);

				glDrawBuffer(GL_COLOR_ATTACHMENT0);

				glDrawArrays(GL_TRIANGLES, side * 6, 6);
			}
			ercShader->Disable();
			vao.Unbind();


			glMatrixMode(GL_PROJECTION);
			glPopMatrix();
			glMatrixMode(GL_MODELVIEW);
			glPopMatrix();

			glPopAttrib();

			FBO::Unbind();

			if (!valid) {
				fbo = {};
				cubeTexID = nullptr;
				return;
			}

			if (generateMipMaps) {
				glBindTexture(GL_TEXTURE_CUBE_MAP, cubeTexID);
				glGenerateMipmapEXT(GL_TEXTURE_CUBE_MAP);
				glBindTexture(GL_TEXTURE_CUBE_MAP,         0);
			}
		}

		glDisable(GL_TEXTURE_CUBE_MAP);

		glDeleteTextures(1, &textureID); // release 2D texture

		skyTex.SetRawTexID(cubeTexID.Release());
		skyTex.SetRawSize(int2(ysize, ysize));
	}
	else {
		valid = true;

		skyTex.SetRawTexID(textureID);
		skyTex.SetRawSize(int2(xsize, ysize));
	}

	glEnable(GL_TEXTURE_CUBE_MAP);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skyTex.GetID());
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	glDisable(GL_TEXTURE_CUBE_MAP);

	shader = shaderHandler->CreateProgramObject("[SkyBox]", "SkyBox");
	shader->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/CubeMapVS.glsl", "", GL_VERTEX_SHADER));
	shader->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/CubeMapFS.glsl", "", GL_FRAGMENT_SHADER));
	shader->Link();
	shader->Enable();
	shader->SetUniform("uvFlip", 1.0f, -1.0f, 1.0f);
	shader->SetUniform("skybox", 0);
	shader->Disable();

	valid &= shader->Validate();
#else
	valid = true;
#endif
	globalRendering->drawFog = (fogStart <= 0.99f);
}

CSkyBox::CSkyBox(const std::string& texture)
{
	CBitmap btex;
#ifndef HEADLESS
	if (!btex.Load(texture) || !(btex.textype == GL_TEXTURE_CUBE_MAP || btex.textype == GL_TEXTURE_2D)) {
		LOG_L(L_WARNING, "could not load skybox texture from file %s", texture.c_str());
		valid = false;
	}
	Init(btex.CreateTexture(), btex.xsize, btex.ysize, btex.textype == GL_TEXTURE_2D);
#else
	Init(btex.CreateTexture(), btex.xsize, btex.ysize,                         false);
#endif
}


CSkyBox::~CSkyBox()
{
	//ZoneScoped;
#ifndef HEADLESS
	if (shader)
		shaderHandler->ReleaseProgramObject("[SkyBox]", "SkyBox");
#endif
}

void CSkyBox::Draw()
{
	//ZoneScoped;
#ifndef HEADLESS
	if (!globalRendering->drawSky)
		return;

	if (!valid)
		return;

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

	glEnable(GL_TEXTURE_CUBE_MAP);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skyTex.GetID());

	skyVAO.Bind();
	assert(shader->IsValid());
	shader->Enable();

	shader->SetUniform("planeColor",
		waterRendering->planeColor.x,
		waterRendering->planeColor.y,
		waterRendering->planeColor.z,
		static_cast<float>(waterRendering->hasWaterPlane && !globalRendering->drawDebugCubeMap)
	);

	glDrawArrays(GL_TRIANGLES, 0, 36);

	shader->Disable();
	skyVAO.Unbind();

	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	glDisable(GL_TEXTURE_CUBE_MAP);

	// glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	sky->SetupFog();
#endif
}