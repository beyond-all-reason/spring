#version 430 core

layout(std430, binding = SIZE_SSBO_BINDING_IDX) restrict buffer SIZE
{
    uint atomicCounters[];
};

layout(std430, binding = INDR_SSBO_BINDING_IDX) restrict writeonly buffer INDIRECT
{
    uint indirect[];
};

uniform int sortElemsPerThread;

layout(local_size_x = 1) in;
void main()
{
#ifdef PROCESS_TRIANGLES
	uint numElems = 2u * atomicCounters[SIZE_SSBO_QUAD_IDX];
#else // quads
	uint numElems = 1u * atomicCounters[SIZE_SSBO_QUAD_IDX];
#endif
	atomicCounters[SIZE_SSBO_NUM_ELEM] = numElems;
	
	{
		uint divisor = KEYVAL_SORTING_KEYVAL_WG_SIZE;
		indirect[KVAL_SSBO_INDRCT_X] = (numElems + divisor - 1) / divisor;
		indirect[KVAL_SSBO_INDRCT_Y] = 1u;
		indirect[KVAL_SSBO_INDRCT_Z] = 1u;
	}
	{
		uint divisor = RADIX_SHADER_WG_SIZE * sortElemsPerThread;
		indirect[HIST_SSBO_INDRCT_X] = (numElems + divisor - 1) / divisor;
		indirect[HIST_SSBO_INDRCT_Y] = 1u;
		indirect[HIST_SSBO_INDRCT_Z] = 1u;
	}
}