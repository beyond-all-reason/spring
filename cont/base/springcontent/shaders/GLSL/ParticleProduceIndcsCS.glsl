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
shared uint localNumOutOfB;

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
		localNumOutOfB = 0u;
	}

	barrier();
    memoryBarrierShared();

	uint elemIdx = gl_GlobalInvocationID.x;
	if (elemIdx >= totalNumElems)
		return;

	#ifdef PROCESS_TRIANGLES
	uint triIndex = 4u * (valsIn[elemIdx] >> 1u);
	uint idxIndex = 3u * elemIdx;

/*
	if (idxIndex + 3u >= indcsArraySize) {
		atomicAdd(localNumOutOfB, 1u);
		return;
	}
*/
	if (elemIdx % 2u == 0u) {
		indicesData[idxIndex++] = triIndex + 3u;
		indicesData[idxIndex++] = triIndex + 0u;
		indicesData[idxIndex  ] = triIndex + 1u;
	} else {
		indicesData[idxIndex++] = triIndex + 3u;
		indicesData[idxIndex++] = triIndex + 1u;
		indicesData[idxIndex  ] = triIndex + 2u;
	}
	#else
	uint triIndex = 4u * valsIn[elemIdx];
	uint idxIndex = 3u * elemIdx;
/*
	if (idxIndex + 6u >= indcsArraySize) {
		atomicAdd(localNumOutOfB, 1u);
		return;
	}
*/
	indicesData[idxIndex++] = triIndex + 3u;
	indicesData[idxIndex++] = triIndex + 0u;
	indicesData[idxIndex++] = triIndex + 1u;

	indicesData[idxIndex++] = triIndex + 3u;
	indicesData[idxIndex++] = triIndex + 1u;
	indicesData[idxIndex  ] = triIndex + 2u;
	#endif
	
	barrier();
    memoryBarrierShared();
	
	atomicAdd(atomicCounters[SIZE_SSBO_OOBC_IDX], localNumOutOfB);
}
