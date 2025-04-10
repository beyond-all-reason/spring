#version 130

uniform samplerCube skybox;
uniform vec4 planeColor; // .w signals if enabled

in vec3 uvw;
out vec4 fragColor;

void main()
{
    vec4 texColor = texture(skybox, normalize(uvw));
	fragColor = mix(texColor, vec4(planeColor.rgb, 1.0), smoothstep(0.0, 0.5, uvw.y) * planeColor.w);
}
