#version 130

in vec2 vertexPos;
in vec2 texCoords;

out vec2 vTexCoords;

void main()
{
	gl_Position = gl_ModelViewProjectionMatrix * vec4(vertexPos, 0.0, 1.0);
	vTexCoords = texCoords;
}
