/**
* Updated slightly by @lhog
* Based on VkRadixSort written by Mirco Werner: https://github.com/MircoWerner/VkRadixSort which is
* Based on implementation of Intel's Embree: https://github.com/embree/embree/blob/v4.0.0-ploc/kernels/rthwif/builder/gpu/sort.h
*/

#version 430 core

//#define WORKGROUP_SIZE 512 // assert WORKGROUP_SIZE >= HIST_BIN_SIZE
//#define BIN_BIT_SIZE 8
#define HIST_BIN_SIZE (1 << BIN_BIT_SIZE)

layout(std430, binding = KEYI_SSBO_BINDING_IDX) restrict readonly buffer KEYS
{
    uint keyIn[];
};

layout(std430, binding = HIST_SSBO_BINDING_IDX) restrict writeonly buffer HIST
{
    // [wgHist_of_workgroup_0 | wgHist_of_workgroup_1 | ... ]
    uint hist[]; // |hist| = HIST_BIN_SIZE * #WORKGROUPS
};

#ifdef INDIRECT_EXECUTION
layout(std430, binding = SIZE_SSBO_BINDING_IDX) restrict readonly buffer NUMKEYS
{
    uint atomicCounters[];
};
shared uint numElements;
#else
uniform int numElements;
#endif

uniform int passNum;
uniform int numElemsPerThread;

shared uint[HIST_BIN_SIZE] wgHist;

layout(local_size_x = WORKGROUP_SIZE) in;
void main() {
    uint gID = gl_GlobalInvocationID.x;
    uint lID = gl_LocalInvocationID.x;
    uint wID = gl_WorkGroupID.x;
	
	#ifdef INDIRECT_EXECUTION
	if (lID == 0u) {
		numElements = GET_NUM_ELEMS;
	}
	#endif

    // initialize wgHist
    if (lID < HIST_BIN_SIZE) {
        wgHist[lID] = 0U;
    }
    barrier();
    memoryBarrierShared();

    for (uint index = 0; index < numElemsPerThread; index++) {
        uint elementIdx = wID * numElemsPerThread * WORKGROUP_SIZE + index * WORKGROUP_SIZE + lID;
        if (elementIdx < numElements) {
            // determine the bin
            const uint binID = uint(keyIn[elementIdx] >> (passNum * BIN_BIT_SIZE)) & uint(HIST_BIN_SIZE - 1);
            // increment the wgHist
            atomicAdd(wgHist[binID], 1U);
        }
    }
    barrier();
    memoryBarrierShared();

    if (lID < HIST_BIN_SIZE) {
        hist[HIST_BIN_SIZE * wID + lID] = wgHist[lID];
    }
}