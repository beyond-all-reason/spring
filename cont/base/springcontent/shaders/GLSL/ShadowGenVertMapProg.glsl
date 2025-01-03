#if (GL_FRAGMENT_PRECISION_HIGH == 1)
// ancient GL3 ATI drivers confuse GLSL for GLSL-ES and require this
precision highp float;
#else
precision mediump float;
#endif

in vec3 vertexPos;

uniform sampler2D heightMapTex;
uniform float borderMinHeight;
uniform ivec2 texSquare;
uniform vec4 mapSize; // mapSize, 1.0/mapSize

const float SMF_TEXSQR_SIZE = 1024.0;

float HeightAtWorldPos(vec2 wxz){
	// Some texel magic to make the heightmap tex perfectly align:
	const vec2 HM_TEXEL = vec2(8.0, 8.0);
	wxz +=  -HM_TEXEL * (wxz * mapSize.zw) + 0.5 * HM_TEXEL;

	vec2 uvhm = clamp(wxz, HM_TEXEL, mapSize.xy - HM_TEXEL);
	uvhm *= mapSize.zw;

	return textureLod(heightMapTex, uvhm, 0.0).x;
}

mat4 calculateLookAtMatrix(vec3 eye, vec3 center, vec3 up) {

    vec3 zaxis = normalize(center - eye); //from center towards eye vector
    vec3 xaxis = normalize(cross(zaxis, up));
    vec3 yaxis = cross(xaxis, zaxis);
    
    mat4 lookAtMatrix;
    
    lookAtMatrix[0] = vec4(xaxis.x, yaxis.x, -zaxis.x, 0.0);
    lookAtMatrix[1] = vec4(xaxis.y, yaxis.y, -zaxis.y, 0.0);
    lookAtMatrix[2] = vec4(xaxis.z, yaxis.z, -zaxis.z, 0.0);
    lookAtMatrix[3] = vec4(dot(xaxis, -eye), dot(yaxis, -eye), dot(zaxis, eye), 1.0);

    return lookAtMatrix;
}

mat4 calculateLookAtMatrix(vec3 toEyeDir, vec3 up) {
    return calculateLookAtMatrix(toEyeDir, vec3(0.0), up);
}


mat4 calculateLookAtMatrix(vec3 eye, vec3 center, float roll) {
    return calculateLookAtMatrix(eye, center, vec3(sin(roll), cos(roll), 0.0));
}


mat4 createOrthographicMatrix(float l, float r, float b, float t, float n, float f) {
    mat4 ortho = mat4(1.0);

	float tx = -((r + l) / (r - l));
	float ty = -((t + b) / (t - b));
	float tz = -((f + n) / (f - n));
    
    ortho[0][0] =  2.0 / (r - l);
    ortho[1][1] =  2.0 / (t - b);
    ortho[2][2] = -2.0 / (f - n);
    ortho[3][0] = tx;
    ortho[3][1] = ty;
    ortho[3][2] = tz;
    
    return ortho;
}


const vec3 lightDir = vec3(0.485069722, 0.727604508, -0.485069722);
const vec3 projMidPoint = vec3(2003.51465, 302.398773, 4904.83691);
const vec3 FrustumPointsWS[8] = vec3[8](
	vec3(763.520508, 719.709961, 7475.94531),
	vec3(2105.97266, 719.708984, 7475.94531),
	vec3(4006.02637, 719.709473, 4440.43750),
	vec3(-200.00000, 719.709473, 4440.43555),
	vec3(64.6748047, -114.911926, 6949.44238),
	vec3(2804.81738, -114.911926, 6949.44287),
	vec3(6683.10693, -114.911926, 753.524902),
	vec3(-200.00000, -114.911926, 753.521973)
);

const float bounds = 8192.0;

void main() {
	vec4 vertexWorldPos = vec4(vertexPos, 1.0);
	vertexWorldPos.xz += vec2(texSquare) * SMF_TEXSQR_SIZE;
	vertexWorldPos.y = mix(borderMinHeight, HeightAtWorldPos(vertexWorldPos.xz), float(vertexWorldPos.y == 0.0));
	/*
	if (vertexWorldPos.y == 0.0)
		vertexWorldPos.y = HeightAtWorldPos(vertexWorldPos.xz);
	else
		vertexWorldPos.y = borderMinHeight;
	*/

	mat4 ccMat = mat4(1);
	//ccMat[3][2] = 0.5;
	//ccMat[2][2] = 0.5;

	vec3 mapMiddle = vec3(4096, 100, 4096);
	vec3 eyePos = vec3(4096, 150, 4096);

	mat4 viewMat = calculateLookAtMatrix(eyePos, mapMiddle, vec3(1.0, 0.0, 0.0));

	//const vec3 NpVector = vec3(0.33, 0.33, 0.33);
	const vec3 NpVector = vec3(1, 0, 0);

	viewMat = mat4(1.0);
	vec3 zDir = normalize(eyePos - mapMiddle);
	viewMat[2] = vec4(normalize(zDir), 0);
	viewMat[0] = vec4(normalize(cross(NpVector, viewMat[2].xyz)), 0);
	viewMat[1] = vec4(cross(viewMat[2].xyz, viewMat[0].xyz), 0);

	//viewMat[2] = vec4(0, 0, 1, 0);
	//viewMat[0] = vec4(1, 0, 0, 0);
	//viewMat[2] = vec4(cross(viewMat[0].xyz, viewMat[1].xyz), 0);
	
	mat4 viewMatT = viewMat;
	viewMat = transpose(viewMat);
	//viewMat[3] = viewMat * vec4(4096, 1000, 4096, 1);
	//viewMat[3] = vec4(-4096, -4096, 0, 1);
	viewMat[3] = vec4(dot(viewMatT[0].xyz, -eyePos), dot(viewMatT[1].xyz, -eyePos), dot(viewMatT[2].xyz, eyePos), 1.0);
	viewMat[3] = vec4(-4096, -4096, 0, 1);

	//viewMat = calculateLookAtMatrix(eyePos, mapMiddle, vec3(1.0, 0.0, 0.0));

	mat4 projMat = createOrthographicMatrix(-0.5 * bounds, 0.5 * bounds, -0.5 * bounds, 0.5 * bounds, -5000.0, 0);

	vec4 lightVertexPos = ccMat * projMat * viewMat * vertexWorldPos;

	//vec4 lightVertexPos = gl_ModelViewMatrix * vertexWorldPos;

	//lightVertexPos.xy += vec2(0.5);
	//lightVertexPos.z  -= 2e-3; // glEnable(GL_POLYGON_OFFSET_FILL); is in use

	//gl_Position = gl_ProjectionMatrix * lightVertexPos;
	//gl_Position = vec4(0,0,1,1);

	gl_Position = lightVertexPos;

	gl_ClipVertex  = vertexWorldPos;
	gl_TexCoord[0] = gl_MultiTexCoord0;
}