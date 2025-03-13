#include "UpdateList.h"

CR_BIND_TEMPLATE_1TYPED(UpdateListTemplate, ValueT, )
CR_REG_METADATA_TEMPLATE_1TYPED(UpdateListTemplate, ValueT, (
	CR_MEMBER(updateList),
	CR_MEMBER(changed)
))

CR_BIND(UpdateList, )
CR_REG_METADATA(UpdateList, )

CR_BIND(UpdateListMT, )
CR_REG_METADATA(UpdateListMT, )