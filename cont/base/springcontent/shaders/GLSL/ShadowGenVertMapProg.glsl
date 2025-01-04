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

mat4 calculateLookAtMatrix(vec3 eyePos, vec3 cntPos, vec3 up)
{
    vec3 zaxis = normalize(eyePos - cntPos); // CE vector
    vec3 xaxis = normalize(cross(up, zaxis));
    vec3 yaxis = cross(zaxis, xaxis);

    mat4 lookAtMatrix;
    
    lookAtMatrix[0] = vec4(xaxis.x, yaxis.x, zaxis.x, 0.0);
    lookAtMatrix[1] = vec4(xaxis.y, yaxis.y, zaxis.y, 0.0);
    lookAtMatrix[2] = vec4(xaxis.z, yaxis.z, zaxis.z, 0.0);
    lookAtMatrix[3] = vec4(-dot(xaxis, eyePos), -dot(yaxis, eyePos), -dot(zaxis, eyePos), 1.0);

    return lookAtMatrix;
}

mat4 calculateLookAtMatrix(vec3 toEyeDir, vec3 up)
{
    return calculateLookAtMatrix(toEyeDir, vec3(0.0), up);
}

mat4 calculateLookAtMatrix(vec3 toEyeDir, float roll)
{
    return calculateLookAtMatrix(toEyeDir, vec3(sin(roll), cos(roll), 0.0));
}


mat4 calculateLookAtMatrix(vec3 eye, vec3 center, float roll)
{
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


//const vec3 lightDir = vec3(0.485069722, 0.727604508, -0.485069722);
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

	ccMat[3][2] = 0.5; // bias
	ccMat[2][2] = 0.5; // z - scale

	//ccMat[3][2] = 0.25; // bias
	//ccMat[2][2] = 0.25; // z - scale

	vec3 mapMiddle = vec3(4096, 0, 4096);
	vec3 eyePos = vec3(4096, 250, 4096);

	vec3 lightDir = -vec3(0.485069722, 0.727604508, -0.485069722); // positive y makes no sense, apparently the vector is inverted, so invert again

	//vec3 lightDir = -vec3(0, 1, 0); 

	//mat4 viewMat = calculateLookAtMatrix(mapMiddle - lightDirUp, mapMiddle, vec3(1.0, 0.0, 0.0));
	//mat4 viewMat = calculateLookAtMatrix(projMidPoint - lightDir, projMidPoint, 0.6);

	//mat4 viewMat = calculateLookAtMatrix(-lightDir, 0.6);

	mat4 viewMat = mat4(1);
	{
		vec3 zaxis = normalize(-lightDir);

/*
		vec4 q = vec4(zaxis.z, 0.0, -zaxis.x, 1.0 + zaxis.y); // or invert all signs. Quaterion UpVec --> zaxis
		q /= length(q);
		
		vec3 xaxis = vec3(0, 0, 1);
		xaxis = 2.0 * dot(q.xyz, xaxis) * q.xyz + (q.w * q.w - dot(q.xyz, q.xyz)) * xaxis + 2.0 * q.w * cross(q.xyz, xaxis);
*/
		//vec3 yaxis = vec3(0, 0, 1);
		//yaxis = 2.0 * dot(q.xyz, yaxis) * q.xyz + (q.w * q.w - dot(q.xyz, q.xyz)) * yaxis + 2.0 * q.w * cross(q.xyz, yaxis);

		vec3 xaxis = vec3(1, 0, 0);
		xaxis = normalize(xaxis - dot(xaxis, zaxis) * zaxis);

		vec3 yaxis = cross(zaxis, xaxis);


		viewMat[0] = vec4(xaxis.x, yaxis.x, zaxis.x, 0.0);
		viewMat[1] = vec4(xaxis.y, yaxis.y, zaxis.y, 0.0);
		viewMat[2] = vec4(xaxis.z, yaxis.z, zaxis.z, 0.0);
		viewMat[3] = vec4(0,0,0,1);
		//viewMat[3] = vec4(dot(xaxis, -eyePos), dot(yaxis, -eyePos), dot(zaxis, -eyePos), 1.0);
	}

	//viewMat = calculateLookAtMatrix(-lightDir, 0.6);

	vec3 mins = vec3( 1000000000);
	vec3 maxs = vec3(-1000000000);

	for (int i = 0; i < 8; ++i) {
		vec3 fpVS = (viewMat * vec4(FrustumPointsWS[i], 1.0)).xyz;
		mins = min(mins, fpVS);
		maxs = max(maxs, fpVS);
	}

/*
	//const vec3 NpVector = vec3(0.33, 0.33, 0.33);
	const vec3 NpVector = vec3(1, 0, 0);

	viewMat = mat4(1.0);
	vec3 zDir = normalize(eyePos - mapMiddle);
	viewMat[2] = vec4(normalize(zDir), 0);
	viewMat[0] = vec4(normalize(cross(NpVector, viewMat[2].xyz)), 0);
	viewMat[1] = vec4(cross(viewMat[2].xyz, viewMat[0].xyz), 0);
	
	mat4 viewMatT = viewMat;
	viewMat = transpose(viewMat);
	viewMat[3] = vec4(dot(viewMatT[0].xyz, -eyePos), dot(viewMatT[1].xyz, -eyePos), dot(viewMatT[2].xyz, -eyePos), 1.0);
	//viewMat[3] = vec4(-4096, -4096, -100, 1);
*/
	//viewMat = calculateLookAtMatrix(eyePos, mapMiddle, vec3(1.0, 0.0, 0.0));

	// only correct if looking top down, adjust relative to viewMat[3][2]
	//   float mapH =  200.0 + viewMat[3][2];
	//   float mapL =  0.0 + viewMat[3][2];
	// if changed like that viewMat[3][2] value makes no difference

	float mapH =  8000.0;
	float mapL =  -8000.0;

	//mat4 projMat = createOrthographicMatrix(-0.5 * bounds, 0.5 * bounds, -0.5 * bounds, 0.5 * bounds, -mapH, -mapL);

	mat4 projMat = createOrthographicMatrix(mins.x, maxs.x, mins.y, maxs.y, -maxs.z, -mins.z);

	//vec4 lightVertexPos = ccMat * projMat * viewMat * vertexWorldPos;

	vec4 lightVertexPos = gl_ModelViewMatrix * vertexWorldPos;

	//vec4 lightVertexPos = gl_ModelViewMatrix * vertexWorldPos;

	//lightVertexPos.xy += vec2(0.5);
	//lightVertexPos.z  -= 2e-3; // glEnable(GL_POLYGON_OFFSET_FILL); is in use

	gl_Position = gl_ProjectionMatrix * lightVertexPos;

	gl_ClipVertex  = vertexWorldPos;
	gl_TexCoord[0] = gl_MultiTexCoord0;
}