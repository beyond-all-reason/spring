#version 430 core

uniform ivec3 arraySizes;
uniform vec2 frameData; //prev, this
uniform mat3 camView;
uniform vec3 camPos;

struct InputData {
	vec4 posRad;
	vec4 speedGrav;
	vec4 texCoord;
	vec4 info; // color, intensity, creatFrame, destrFrame
	// anim params?	
};

struct TriangleData {
	vec4 pos; // .w unused
	vec4 uvw; // .w unused
	vec4 uvInfo;
	vec4 apAndCol;
};

layout(local_size_x = WORKGROUP_SIZE, local_size_y = 1, local_size_z = 1) in;

layout(std430, binding = 0) readonly buffer IN
{
    InputData dataIn[];
};

layout(std430, binding = 1) buffer OUT1
{
    TriangleData triangleData[];
};

layout(std430, binding = 2) buffer OUT2
{
    uint indicesData[];
};

void main() {
	
}