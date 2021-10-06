#include "ModelShaders.h"

#include "ModelDrawerHelpers.h"
#include "System/Config/ConfigHandler.h"
#include "Sim/Misc/TeamHandler.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/Env/SunLighting.h"
#include "Rendering/Env/ISky.h"


template<ModelDrawerTypes T>
CModelShader<T>::CModelShader()
{
	//TODO: move to static Init()?
	alphaValues.x = std::max(0.11f, std::min(1.0f, 1.0f - configHandler->GetFloat("UnitTransparency")));
	alphaValues.y = std::min(1.0f, alphaValues.x + 0.1f);
	alphaValues.z = std::min(1.0f, alphaValues.x + 0.2f);
	alphaValues.w = std::min(1.0f, alphaValues.x + 0.4f);
}

template<ModelDrawerTypes T>
bool CModelShader<T>::SetTeamColor(int team, const float2 alpha) const
{
	// need this because we can be called by no-team projectiles
	if (!teamHandler.IsValidTeam(team))
		return false;

	// should be an assert, but projectiles (+FlyingPiece) would trigger it
	if (shadowHandler.InShadowPass())
		return false;

	return true;
}

/**
 * Set up the texture environment in texture unit 0
 * to give an S3O texture its team-colour.
 *
 * Also:
 * - call SetBasicTeamColour to set the team colour to transform to.
 * - Replace the output alpha channel. If not, only the team-coloured bits will show, if that. Or something.
 */
void CModelShaderFFP::SetupBasicS3OTexture0()
{
	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);

	// RGB = Texture * (1 - Alpha) + Teamcolor * Alpha
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_INTERPOLATE_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_CONSTANT_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_RGB_ARB, GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_RGB_ARB, GL_ONE_MINUS_SRC_ALPHA);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);

	// ALPHA = Ignore
}

/**
 * This sets the first texture unit to GL_MODULATE the colours from the
 * first texture unit with the current glColor.
 *
 * Normal S3O drawing sets the color to full white; translucencies
 * use this setup to 'tint' the drawn model.
 *
 * - Leaves glActivateTextureARB at the first unit.
 * - This doesn't tinker with the output alpha, either.
 */
void CModelShaderFFP::SetupBasicS3OTexture1()
{
	glActiveTexture(GL_TEXTURE1);
	glEnable(GL_TEXTURE_2D);

	// RGB = Primary Color * Previous
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PRIMARY_COLOR_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PREVIOUS_ARB);

	// ALPHA = Current alpha * Alpha mask
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_ARB, GL_PRIMARY_COLOR_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA_ARB, GL_SRC_ALPHA);
}

void CModelShaderFFP::CleanupBasicS3OTexture1()
{
	// reset texture1 state
	glActiveTexture(GL_TEXTURE1);
	glDisable(GL_TEXTURE_2D);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_ARB, GL_PREVIOUS_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
}

void CModelShaderFFP::CleanupBasicS3OTexture0()
{
	// reset texture0 state
	glActiveTexture(GL_TEXTURE0);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PREVIOUS_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_RGB_ARB, GL_CONSTANT_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_RGB_ARB, GL_SRC_ALPHA);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
}

bool CModelShaderFFP::SetTeamColor(int team, const float2 alpha = float2(1.0f, 0.0f)) const
{
	if (!CModelShader::SetTeamColor(team, alpha))
		return false;

	// non-shader case via texture combiners
	const float4 m = { 1.0f, 1.0f, 1.0f, alpha.x };

	glActiveTexture(GL_TEXTURE0);
	glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, std::move(CModelDrawerHelper::GetTeamColor(team, alpha.x)));
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, &m.x);

	return true;
}

void CModelShaderFFP::SetNanoColor(const float4& color) const
{
	if (color.a > 0.0f) {
		DisableTextures();
		glColorf4(color);
	}
	else {
		EnableTextures();
		glColorf3(OnesVector);
	}
}

void CModelShaderFFP::Enable(bool deferredPass, bool alphaPass) const
{
	glEnable(GL_LIGHTING);
	// only for the advshading=0 case
	glLightfv(GL_LIGHT1, GL_POSITION, sky->GetLight()->GetLightDir());
	glLightfv(GL_LIGHT1, GL_AMBIENT, sunLighting->modelAmbientColor);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, sunLighting->modelDiffuseColor);
	glLightfv(GL_LIGHT1, GL_SPECULAR, sunLighting->modelSpecularColor);
	glEnable(GL_LIGHT1);

	CModelShaderFFP::SetupBasicS3OTexture1();
	CModelShaderFFP::SetupBasicS3OTexture0();

	const float4 color = { 1.0f, 1.0f, 1.0, mix(1.0f, alphaValues.x, (1.0f * alphaPass)) };

	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, &color.x);
	glColor4fv(&color.x);

	CModelDrawerHelper::PushTransform(camera);
}

void CModelShaderFFP::Disable(bool deferredPass) const
{
	CModelDrawerHelper::PopTransform();

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	glDisable(GL_LIGHTING);
	glDisable(GL_LIGHT1);

	CModelShaderFFP::CleanupBasicS3OTexture1();
	CModelShaderFFP::CleanupBasicS3OTexture0();
}

void CModelShaderFFP::EnableTextures() const
{
	glEnable(GL_LIGHTING);
	glColor3f(1.0f, 1.0f, 1.0f);
	glEnable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE1);
	glEnable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE0);
}

void CModelShaderFFP::DisableTextures() const
{
	glDisable(GL_LIGHTING);
	glDisable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE1);
	glDisable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE0);
	glDisable(GL_TEXTURE_2D);
}
