#version 130

// positions
const vec3 CUBE_VERT[36] = vec3[36](
	//RIGHT
	vec3( 1.0f, -1.0f, -1.0f),
	vec3( 1.0f, -1.0f,  1.0f),
	vec3( 1.0f,  1.0f,  1.0f),
	vec3( 1.0f,  1.0f,  1.0f),
	vec3( 1.0f,  1.0f, -1.0f),
	vec3( 1.0f, -1.0f, -1.0f),
	//LEFT
	vec3(-1.0f, -1.0f,  1.0f),
	vec3(-1.0f, -1.0f, -1.0f),
	vec3(-1.0f,  1.0f, -1.0f),
	vec3(-1.0f,  1.0f, -1.0f),
	vec3(-1.0f,  1.0f,  1.0f),
	vec3(-1.0f, -1.0f,  1.0f),
	//TOP
	vec3(-1.0f,  1.0f, -1.0f),
	vec3( 1.0f,  1.0f, -1.0f),
	vec3( 1.0f,  1.0f,  1.0f),
	vec3( 1.0f,  1.0f,  1.0f),
	vec3(-1.0f,  1.0f,  1.0f),
	vec3(-1.0f,  1.0f, -1.0f),
	//BOTTOM
	vec3(-1.0f, -1.0f, -1.0f),
	vec3(-1.0f, -1.0f,  1.0f),
	vec3( 1.0f, -1.0f, -1.0f),
	vec3( 1.0f, -1.0f, -1.0f),
	vec3(-1.0f, -1.0f,  1.0f),
	vec3( 1.0f, -1.0f,  1.0f),
	//FRONT
	vec3(-1.0f, -1.0f,  1.0f),
	vec3(-1.0f,  1.0f,  1.0f),
	vec3( 1.0f,  1.0f,  1.0f),
	vec3( 1.0f,  1.0f,  1.0f),
	vec3( 1.0f, -1.0f,  1.0f),
	vec3(-1.0f, -1.0f,  1.0f),
	//BACK
	vec3(-1.0f,  1.0f, -1.0f),
	vec3(-1.0f, -1.0f, -1.0f),
	vec3( 1.0f, -1.0f, -1.0f),
	vec3( 1.0f, -1.0f, -1.0f),
	vec3( 1.0f,  1.0f, -1.0f),
	vec3(-1.0f,  1.0f, -1.0f)
);

uniform vec3 midMap;

out vec3 dir;

void main()
{
	vec3 camPos = gl_ModelViewMatrixInverse[3].xyz;

	float R = sqrt(16.0 * (midMap.x * midMap.x + midMap.z * midMap.z));

	vec3 pos  = midMap + CUBE_VERT[gl_VertexID] * R;

	dir = pos - camPos.xyz;

	gl_Position = gl_ModelViewProjectionMatrix * vec4(pos, 1.0);
	gl_Position = gl_Position.xyww;
}
