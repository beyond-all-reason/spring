#version 150

in vec3 vertexPos;

uniform mat4 viewProjMat;
uniform mat4 worldMat;

void main()
{
	gl_Position = viewProjMat * worldMat * vec4(vertexPos, 1.0);
}