#include "RenderBuffers.h"

#include "System/Log/ILog.h"

#include "System/Misc/TracyDefs.h"

std::array<std::unique_ptr<RenderBuffer>, 13> RenderBuffer::typedRenderBuffers;

void RenderBuffer::InitStatic()
{
	RECOIL_DETAILED_TRACY_ZONE;
	RenderBuffer::typedRenderBuffers = {
		std::make_unique<TypedRenderBuffer<VA_TYPE_0   >>(1 << 16, 1 << 17),
		std::make_unique<TypedRenderBuffer<VA_TYPE_C   >>(1 << 20, 1 << 21),
		std::make_unique<TypedRenderBuffer<VA_TYPE_N   >>(1 << 10, 1 << 11),
		std::make_unique<TypedRenderBuffer<VA_TYPE_T   >>(1 << 20, 1 << 21),
		std::make_unique<TypedRenderBuffer<VA_TYPE_T4  >>(1 << 16, 1 << 18),
		std::make_unique<TypedRenderBuffer<VA_TYPE_TN  >>(1 << 16, 1 << 17),
		std::make_unique<TypedRenderBuffer<VA_TYPE_TC  >>(1 << 20, 1 << 21),
		std::make_unique<TypedRenderBuffer<VA_TYPE_PROJ>>(1 << 20, 1 << 21),
		std::make_unique<TypedRenderBuffer<VA_TYPE_TNT >>(0      , 0      ),
		std::make_unique<TypedRenderBuffer<VA_TYPE_2D0 >>(1 << 16, 1 << 17),
		std::make_unique<TypedRenderBuffer<VA_TYPE_2DC >>(1 << 16, 1 << 17),
		std::make_unique<TypedRenderBuffer<VA_TYPE_2DT >>(1 << 20, 1 << 21),
		std::make_unique<TypedRenderBuffer<VA_TYPE_2DTC>>(1 << 20, 1 << 21)
	};
}

void RenderBuffer::KillStatic()
{
	RECOIL_DETAILED_TRACY_ZONE;
	for (const auto& trb : typedRenderBuffers) {
		if (!trb)
			continue;

		const auto Cx = trb->GetBuffersCapacity();
		const auto C0 = trb->GetInitialCapacity();
		const auto S  = trb->GetMaxSize();
		LOG_L(L_INFO, "[RenderBuffer::%s] Type = %s max size/init. capacity/curr. capacity: VBO = {%u, %u, %u}, EBO = {%u, %u, %u}", __func__, trb->GetBufferName(),
			static_cast<uint32_t>( S[0]),
			static_cast<uint32_t>(C0[0]),
			static_cast<uint32_t>(Cx[0]),
			static_cast<uint32_t>( S[1]),
			static_cast<uint32_t>(C0[1]),
			static_cast<uint32_t>(Cx[1])
		);
	}

	RenderBuffer::typedRenderBuffers = {};
}