#version 430 core


layout(std430, binding = VALI_SSBO_BINDING_IDX) restrict readonly buffer VALSIN
{
    uint valsIn[];
};

layout(std430, binding = SIZE_SSBO_BINDING_IDX) restrict readonly buffer SIZE
{
    uint atomicCounters[];
};

layout(std430, binding = IDCS_SSBO_BINDING_IDX) writeonly restrict buffer OUT2
{
    uint indicesData[];
};

shared uint indcsArraySize;
shared uint totalNumElems;
//shared uint localNumOutOfB;

layout(local_size_x = WORKGROUP_SIZE) in;
void main()
{
	if (gl_LocalInvocationID.x == 0u) {
		#ifdef PROCESS_TRIANGLES
			indcsArraySize = 3u * GET_NUM_ELEMS;
		#else
			indcsArraySize = 6u * GET_NUM_ELEMS;
		#endif
		totalNumElems = atomicCounters[SIZE_SSBO_NUM_ELEM];
		//localNumOutOfB = 0u;
	}

	barrier();
    memoryBarrierShared();

	uint elemIdx = gl_GlobalInvocationID.x;
	if (elemIdx >= totalNumElems)
		return;

	#ifdef PROCESS_TRIANGLES
	uint triaNum = valsIn[elemIdx];
	uint quadNum = (triaNum >> 1u);
	uint vrtIndex = 4u * quadNum;
	uint idxIndex = 3u * elemIdx;

/*
	if (idxIndex + 3u >= indcsArraySize) {
		atomicAdd(localNumOutOfB, 1u);
		return;
	}
*/
	if (triaNum % 2u == 1u) {
		indicesData[idxIndex++] = vrtIndex + 3u;
		indicesData[idxIndex++] = vrtIndex + 0u;
		indicesData[idxIndex  ] = vrtIndex + 1u;
	} else {
		indicesData[idxIndex++] = vrtIndex + 3u;
		indicesData[idxIndex++] = vrtIndex + 1u;
		indicesData[idxIndex  ] = vrtIndex + 2u;
	}
	#else
	uint quadNum = valsIn[elemIdx];
	uint vrtIndex = 4u * quadNum;
	uint idxIndex = 6u * elemIdx;
/*
	if (idxIndex + 6u >= indcsArraySize) {
		atomicAdd(localNumOutOfB, 1u);
		return;
	}
*/
	indicesData[idxIndex++] = vrtIndex + 3u;
	indicesData[idxIndex++] = vrtIndex + 0u;
	indicesData[idxIndex++] = vrtIndex + 1u;

	indicesData[idxIndex++] = vrtIndex + 3u;
	indicesData[idxIndex++] = vrtIndex + 1u;
	indicesData[idxIndex  ] = vrtIndex + 2u;
	#endif
	
	barrier();
    memoryBarrierShared();
	
	//atomicAdd(atomicCounters[SIZE_SSBO_OOBC_IDX], localNumOutOfB);
}
