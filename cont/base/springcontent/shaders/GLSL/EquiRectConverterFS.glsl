#version 130

uniform sampler2D tex;

in vec3 uvw;

out vec4 fragColor;

vec4 SampleEquiRect(vec3 rayDirection)
{
    const float PI = 3.1415926535897932384626433832795;
    const float PIm2 = 2.0 * PI;
    return textureLod(tex, vec2((atan(rayDirection.z, rayDirection.x) / PIm2) + 0.5, acos(-rayDirection.y) / PI), 0.0);
}

void main()
{
    fragColor = SampleEquiRect(normalize(uvw));
}
