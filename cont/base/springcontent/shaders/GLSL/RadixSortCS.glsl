/**
* Updated slightly by @lhog
* Based on VkRadixSort written by Mirco Werner: https://github.com/MircoWerner/VkRadixSort which is
* Based on implementation of Intel's Embree: https://github.com/embree/embree/blob/v4.0.0-ploc/kernels/rthwif/builder/gpu/sort.h
*/

#version 430
#extension GL_KHR_shader_subgroup_basic: require
#extension GL_KHR_shader_subgroup_arithmetic: require
//#extension GL_KHR_shader_subgroup_ballot: require
#extension GL_KHR_shader_subgroup_shuffle: require

// assert WORKGROUP_SIZE >= HIST_BIN_SIZE
#define HIST_BIN_SIZE (1 << BIN_BIT_SIZE)


layout(std430, binding = HIST_SSBO_BINDING_IDX) restrict readonly buffer HIST
{
    // [histogram_of_workgroup_0 | histogram_of_workgroup_1 | ... ]
    uint hist []; // |hist| = HIST_BIN_SIZE * #WORKGROUPS = HIST_BIN_SIZE * numWorkGroups
};

layout(std430, binding = KEYI_SSBO_BINDING_IDX) restrict readonly buffer KEYSIN
{
    uint keysIn[];
};

layout(std430, binding = VALI_SSBO_BINDING_IDX) restrict readonly buffer VALSIN
{
    uint valsIn[];
};

layout(std430, binding = KEYO_SSBO_BINDING_IDX) restrict writeonly buffer KEYSOUT
{
    uint keysOut[];
};

layout(std430, binding = VALO_SSBO_BINDING_IDX) restrict writeonly buffer VALSOUT
{
    uint valsOut[];
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

uniform int numWorkGroups;
uniform int passNum;
uniform int numElemsPerThread;

uint GetBinFlagIndex(uint binID, uint flagIdx)
{
    return (WORKGROUP_SIZE / 32) * binID + flagIdx;
}

shared uint sums[HIST_BIN_SIZE / SUBGROUP_SIZE] ; // subgroup reductions
shared uint globalOffsets[HIST_BIN_SIZE] ; // global exclusive scan (prefix sum)

// [binaryFlags_0 | binaryFlags_1 | ... binaryFlags_(HIST_BIN_SIZE-1) ]
shared uint binaryFlags[HIST_BIN_SIZE * WORKGROUP_SIZE / 32] ;

layout(local_size_x = WORKGROUP_SIZE) in;
void main()
{
    uint gID = gl_GlobalInvocationID.x;
    uint lID = gl_LocalInvocationID.x;
    uint wID = gl_WorkGroupID.x;
    uint sID = gl_SubgroupID;
    uint lsID = gl_SubgroupInvocationID;
	
	#ifdef INDIRECT_EXECUTION
	if (lID == 0u) {
		numElements = GET_NUM_ELEMS;
	}
	#endif

    uint localHist = 0;
    uint prefSum = 0;
    uint histCount = 0;

    if (lID < HIST_BIN_SIZE) {
        uint count = 0;
        for (uint j = 0; j < numWorkGroups; j++) {
            uint t = hist[HIST_BIN_SIZE * j + lID];
            localHist = (j == wID) ? count : localHist;
            count += t;
        }
        histCount = count;
        uint sum = subgroupAdd(histCount);
        prefSum = subgroupExclusiveAdd(histCount);
        if (subgroupElect()) {
            // one thread inside the warp/subgroup enters this section
            sums[sID] = sum;
        }
    }
    barrier();
    memoryBarrierShared();

    if (lID < HIST_BIN_SIZE ) {
        uint thisSum = (lsID < HIST_BIN_SIZE / SUBGROUP_SIZE) ? sums[lsID] : 0u;
        uint sumsPrefixSum = (sID < SUBGROUP_SIZE) ? subgroupShuffle(subgroupExclusiveAdd(thisSum), sID) : 0u;
        uint globalHistogram = sumsPrefixSum + prefSum;
        globalOffsets[lID] = globalHistogram + localHist;
    }

    //     ==== scatter keys according to global offsets =====
    uint flagsBin = lID / 32;
    uint flagsBit = 1 << (lID % 32);

    for (uint index = 0; index < numElemsPerThread; index++) {
        uint elementIdx = wID * numElemsPerThread * WORKGROUP_SIZE + index * WORKGROUP_SIZE + lID;

        // initialize bin flags
        if (lID < HIST_BIN_SIZE) {
            for (int i = 0; i < WORKGROUP_SIZE / 32; i++) {
                binaryFlags[GetBinFlagIndex(lID, i)] = 0u; // init all bin flags to 0
            }
        }
        barrier();
        memoryBarrierShared();

        uint keyIn = 0;
        uint valIn = 0;
        uint binID = 0;
        uint binOffset = 0;
        if (elementIdx < numElements) {
            keyIn = keysIn[elementIdx];
            valIn = valsIn[elementIdx];
            binID = uint(keyIn >> (passNum * BIN_BIT_SIZE)) & uint(HIST_BIN_SIZE - 1);
            // offset for group
            binOffset = globalOffsets[binID];
            // add bit to flag
            atomicAdd(binaryFlags[GetBinFlagIndex(binID, flagsBin)], flagsBit);
        }
        barrier();
        memoryBarrierShared();

        if (elementIdx < numElements)
        {
            // calculate output index of element
            uint prefix = 0;
            uint count = 0;
            for (uint i = 0; i < WORKGROUP_SIZE / 32; i++) {
                uint bits = binaryFlags[GetBinFlagIndex(binID, i)];
                uint fullCount = bitCount(bits);
                uint partCount = bitCount(bits & (flagsBit - 1));
                prefix += (i < flagsBin) ? fullCount : 0U;
                prefix += (i == flagsBin) ? partCount : 0U;
                count += fullCount;
            }
            keysOut[binOffset + prefix] = keyIn;
            valsOut[binOffset + prefix] = valIn;
            if (prefix == count - 1) {
                atomicAdd(globalOffsets[binID], count);
            }
        }

        barrier();
        memoryBarrierShared();
    }
}