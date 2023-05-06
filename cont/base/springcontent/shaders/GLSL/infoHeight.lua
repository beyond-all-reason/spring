return {
	definitions = {
		Spring.GetConfigInt("HighResInfoTexture") and "#define HIGH_QUALITY" or "",
	},
	vertex = [[
	#version 130
	varying vec2 texCoord;

	void main() {
		texCoord = gl_MultiTexCoord0.st;
		gl_Position = vec4(gl_Vertex.xyz, 1.0);
	}
	]],
	fragment = [[
	#version 130
	#ifdef HIGH_QUALITY
		#extension GL_ARB_texture_query_lod : enable
		#extension GL_EXT_gpu_shader4_1 : enable
	#endif

	uniform sampler2D tex0;
	varying vec2 texCoord;

	#ifdef HIGH_QUALITY
		#if GL_ARB_texture_query_lod == 1
			#define GET_TEXLOD(tex, p) (int(textureQueryLOD(tex, p).x))
		#elif GL_EXT_gpu_shader4_1 == 1
			#define GET_TEXLOD(tex, p) (int(textureQueryLod(tex, p).x))
		#else
			#define GET_TEXLOD(tex, p) (0)
		#endif

		#define HASHSCALE1 443.8975
		float rand(vec2 p) {
			vec3 p3  = fract(vec3(p.xyx) * HASHSCALE1);
			p3 += dot(p3, p3.yzx + 19.19);
			return fract((p3.x + p3.y) * p3.z);
		}

		//! source: http://www.iquilezles.org/www/articles/texture/texture.htm
		vec4 getTexel(sampler2D tex, vec2 p) {
			int lod = GET_TEXLOD(tex, p);
			vec2 texSize = vec2(textureSize(tex, lod));
			p = p * texSize + 0.5;

			vec2 i = floor(p);
			vec2 f = p - i;
			vec2 ff = f * f;
			f = ff * f * ((ff * 6.0 - f * 15.0) + 10.0);
			p = i + f;

			p = (p - 0.5) / texSize;
			return texture2D(tex, p);
		}
	#else
		#define getTexel texture2D
	#endif

	void main() {
		gl_FragColor = getTexel(tex0, texCoord);
		gl_FragColor.a = 0.3;
	}
	]],
	uniformInt = {
		tex0 = 0,
	},
	textures = {
		[0] = "$info:height",
	},
}
