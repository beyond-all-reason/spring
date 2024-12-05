#include "Transform.hpp"

CR_BIND(Transform, )
CR_REG_METADATA(Transform, (
	CR_MEMBER(r),
	CR_MEMBER(t),
	CR_MEMBER(s)
))

static_assert(sizeof(Transform) == 3 * 4 * sizeof(float));
static_assert(alignof(Transform) == alignof(decltype(Transform::r)));