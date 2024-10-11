#version 150 compatibility

// VS input attributes
in vec3 pos;
in vec3 uvw;
in vec4 color;

//uniform mat4 transformMatrix = mat4(1.0);

// VS output attributes
out vec3 vuvw;
out vec4 vColor;

void main() {
	vuvw = uvw;
	vColor = color;
	gl_Position = gl_ModelViewProjectionMatrix * vec4(pos, 1.0);
}