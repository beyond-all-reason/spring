#version 130

in vec3 pos;
in vec4 uv;

out vec4 vTexCoord;

void main() {
	vTexCoord = uv;
	gl_Position = gl_ModelViewProjectionMatrix * vec4(pos, 1.0);
}
