#version 130

in vec2 mapUV;
out vec4 outColor;

uniform sampler2D normalsTex;
uniform sampler2D heightMapTex;

uniform vec4 mapSize;
uniform vec4 groundAmbientColor;
uniform vec4 groundDiffuseColor;
uniform vec4 lightDir;
uniform vec3 waterBaseColor;
uniform vec3 waterAbsorb;
uniform vec3 waterMinColor;
uniform float waterLevel;

const float SMF_INTENSITY_MULT = 210.0 / 255.0;

vec3 GetFragmentNormal(vec2 uv) {
	vec3 normal;
	normal.xz = textureLod(normalsTex, uv, 0.0).rg;
	normal.y  = sqrt(1.0 - dot(normal.xz, normal.xz));
	return normal;
}

float EncodeHeight(float relH) {
	return clamp((255 - 10.0 * relH) / 255, 0.0, 1.0);
}

vec3 GetWaterHeightColor(float relH) {
	relH = clamp(relH, 0.0, 1024.0);
	vec3 absorbColor = waterBaseColor - waterAbsorb * relH;
	return clamp(absorbColor, waterMinColor, vec3(1));
}

void main() {
	// get central heightmap
	#if 0
		float height =
			texelFetch(heightMapTex, ivec2(hmCoord.x + 0, hmCoord.y + 0), 0).x +
			texelFetch(heightMapTex, ivec2(hmCoord.x + 0, hmCoord.y + 1), 0).x +
			texelFetch(heightMapTex, ivec2(hmCoord.x + 1, hmCoord.y + 0), 0).x +
			texelFetch(heightMapTex, ivec2(hmCoord.x + 1, hmCoord.y + 1), 0).x;
		height *= 0.25;
	#else
		vec2 halfPixel = 0.5 / (mapSize.xy + vec2(1));
		ivec2 hmCoord = ivec2(mapUV * mapSize.xy);
		float height = textureLod(heightMapTex, mapUV + halfPixel, 0.0).x;
	#endif

	vec3 terrainNormal = GetFragmentNormal(mapUV);
	float posNdotL = max(dot(terrainNormal, lightDir), 0.0);

	vec3 lightVal = min((groundAmbientColor.rgb + groundDiffuseColor.rgb * posNdotL) * SMF_INTENSITY_MULT, vec3(1));

	if (height < waterLevel) {
		float relH = (waterLevel - height);
		float lightIntensity = min((posNdotL + 0.2) * 2.0, 1.0);
		vec3 waterHeightColor = GetWaterHeightColor(relH);
		if (height > waterLevel - 10.0) {
			float wc = relH * 0.1f;
			vec3 lightColor = lightVal * (1.0 - wc);
			lightIntensity *= wc;

			outColor.rgb = waterHeightColor * lightIntensity + lightColor;
		} else {
			outColor.rgb = waterHeightColor * lightIntensity;
		}
		outColor.a = EncodeHeight(relH);
	} else {
		outColor = vec4(lightVal, 1.0);
	}
}