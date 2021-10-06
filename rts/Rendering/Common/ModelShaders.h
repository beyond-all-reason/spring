#pragma once

#include "ModelDrawer.h"

namespace Shader { struct IProgramObject; }

enum ModelShaderProgram {
	MODEL_SHADER_NOSHADOW_STANDARD = 0, ///< model shader (V+F) without self-shadowing
	MODEL_SHADER_SHADOWED_STANDARD = 1, ///< model shader (V+F) with    self-shadowing
	MODEL_SHADER_NOSHADOW_DEFERRED = 2, ///< deferred version of MODEL_SHADER_NOSHADOW (GLSL-only)
	MODEL_SHADER_SHADOWED_DEFERRED = 3, ///< deferred version of MODEL_SHADER_SHADOW   (GLSL-only)
	MODEL_SHADER_COUNT             = 4,
};

template <ModelDrawerTypes T>
class CModelShader {
public:
	CModelShader();
	virtual ~CModelShader() {}
public:
	virtual bool CanEnable() const = 0;

	virtual bool SetTeamColor(int team, const float2 alpha = float2(1.0f, 0.0f)) const;
	virtual void SetNanoColor(const float4& color) const = 0;

	virtual void Enable(bool deferredPass, bool alphaPass) const = 0;
	virtual void Disable(bool deferredPass) const = 0;

	virtual void EnableTextures() const = 0;
	virtual void DisableTextures() const = 0;

	void SetActiveShader(bool shadowed, bool deferred) const {
		// shadowed=1 --> shader 1 (deferred=0) or 3 (deferred=1)
		// shadowed=0 --> shader 0 (deferred=0) or 2 (deferred=1)
		modelShader = modelShaders[shadowed + deferred * 2];
	}
	Shader::IProgramObject* ActiveShader() { return modelShader; }
protected:
	/// <summary>
	/// .x := regular unit alpha
	/// .y := ghosted unit alpha (out of radar)
	/// .z := ghosted unit alpha (inside radar)
	/// .w := AI-temp unit alpha
	/// </summary>
	inline static float4 alphaValues = {};

	std::array<Shader::IProgramObject*, MODEL_SHADER_COUNT> modelShaders = {};
	mutable Shader::IProgramObject* modelShader = nullptr;
public:
	std::array<CModelShader*, MODEL_DRAWER_CNT> modelShaderTypes;
};

class CModelShaderFFP : public CModelShader<MODEL_DRAWER_FFP> {
public:
	CModelShaderFFP() {}
	virtual ~CModelShaderFFP() {}
public:
	bool CanEnable() const override { return true; }

	// Inherited via CModelShader
	bool SetTeamColor(int team, const float2 alpha = float2(1.0f, 0.0f)) const;
	void SetNanoColor(const float4& color) const override;
	void Enable(bool deferredPass, bool alphaPass) const override;
	void Disable(bool deferredPass) const override;
	void EnableTextures() const override;
	void DisableTextures() const override;
private:
public:
	static void SetupBasicS3OTexture0();
	static void SetupBasicS3OTexture1();
	static void CleanupBasicS3OTexture1();
	static void CleanupBasicS3OTexture0();
};
