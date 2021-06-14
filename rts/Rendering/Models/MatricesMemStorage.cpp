#include "MatricesMemStorage.h"

MatricesMemStorage::MatricesMemStorage()
{
	spa = std::make_unique<StablePosAllocator<CMatrix44f>>(INIT_NUM_ELEMS);
	AllocateDummy();
}

void MatricesMemStorage::AllocateDummy()
{
	size_t pos = spa->Allocate(1, false); //allocate dummy matrix at 0th position
	assert(pos == 0u);
	(*spa)[pos] = CMatrix44f::Zero();
}

void MatricesMemStorage::DeallocateDummy()
{
	size_t pos = 0u;
	spa->Free(pos, 1); //deallocate dummy matrix at 0th position
}