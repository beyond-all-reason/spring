#version 130

in vec2 mapUV;
out vec4 shadingVal;
out vec2 normalXZ;

uniform sampler2D heightMapTex;

uniform vec4 mapSizeP1;
uniform vec4 groundAmbientColor;
uniform vec4 groundDiffuseColor;
uniform vec4 lightDir;
uniform vec3 waterBaseColor;
uniform vec3 waterAbsorb;
uniform vec3 waterMinColor;
uniform float waterLevel;

const float SMF_INTENSITY_MULT = 210.0 / 255.0;
const float SQUARE_SIZE = 8.0;

vec3 GetVertex(ivec2 xy) {
	xy = clamp(xy, ivec2(0), ivec2(mapSizeP1.xy));
	return vec3(
		xy.x * SQUARE_SIZE,
		texelFetch(heightMapTex, xy, 0).x,
		xy.y * SQUARE_SIZE
	);
}

vec3 CalcFragmentNormal(vec2 uv) {
	ivec2 xy = ivec2(uv * mapSizeP1.xy);

	ivec2 bl = xy + ivec2(-1, -1);
	ivec2 bm = xy + ivec2( 0, -1);
	ivec2 br = xy + ivec2( 1, -1);
	ivec2 ml = xy + ivec2(-1,  0);
	ivec2 mm = xy + ivec2( 0,  0);
	ivec2 mr = xy + ivec2( 1,  0);
	ivec2 tl = xy + ivec2(-1,  1);
	ivec2 tm = xy + ivec2( 0,  1);
	ivec2 tr = xy + ivec2( 1,  1);

	// get vertices
	vec3 vbl = GetVertex(bl);
	vec3 vbm = GetVertex(bm);
	vec3 vbr = GetVertex(br);
	vec3 vml = GetVertex(ml);
	vec3 vmm = GetVertex(mm);
	vec3 vmr = GetVertex(mr);
	vec3 vtl = GetVertex(tl);
	vec3 vtm = GetVertex(tm);
	vec3 vtr = GetVertex(tr);

	// make them vectors
	vbl -= vmm;
	vbm -= vmm;
	vbr -= vmm;
	vml -= vmm;
	vmr -= vmm;
	vtl -= vmm;
	vtm -= vmm;
	vtr -= vmm;

	vec3 normal = vec3(0);

	// go CW, seems to be the right direction
	normal += cross(vtr, vmr);
	normal += cross(vmr, vbr);
	normal += cross(vbr, vbm);
	normal += cross(vbm, vbl);
	normal += cross(vbl, vml);
	normal += cross(vml, vtl);
	normal += cross(vtl, vtm);
	normal += cross(vtm, vtr);

	return normalize(normal);
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
	float height = textureLod(heightMapTex, mapUV, 0.0).x;
	height = texelFetch(heightMapTex, ivec2(mapUV * mapSizeP1.xy), 0).x;

	vec3 terrainNormal = CalcFragmentNormal(mapUV);
	normalXZ = vec2(terrainNormal.x, terrainNormal.z);

	float posNdotL = max(dot(terrainNormal, lightDir.xyz), 0.0);

	vec3 lightVal = min((groundAmbientColor.rgb + groundDiffuseColor.rgb * posNdotL) * SMF_INTENSITY_MULT, vec3(1));

	if (height < waterLevel) {
		float relH = (waterLevel - height);
		float lightIntensity = min((posNdotL + 0.2) * 2.0, 1.0);
		vec3 waterHeightColor = GetWaterHeightColor(relH);
		if (height > waterLevel - 10.0) {
			float wc = relH * 0.1f;
			vec3 lightColor = lightVal * (1.0 - wc);
			lightIntensity *= wc;

			shadingVal.rgb = waterHeightColor * lightIntensity + lightColor;
		} else {
			shadingVal.rgb = waterHeightColor * lightIntensity;
		}
		shadingVal.a = EncodeHeight(relH);
	} else {
		shadingVal = vec4(lightVal, 1.0);
	}
}