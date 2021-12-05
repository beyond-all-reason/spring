#include "RenderBuffers.h"

std::array<std::unique_ptr<RenderBuffer>, 11> RenderBuffer::typedRenderBuffers = {
	std::make_unique<TypedRenderBuffer<VA_TYPE_0   >>(1 << 16, 1 << 16),
	std::make_unique<TypedRenderBuffer<VA_TYPE_C   >>(1 << 20, 1 << 20),
	std::make_unique<TypedRenderBuffer<VA_TYPE_N   >>(1 << 10, 1 << 10),
	std::make_unique<TypedRenderBuffer<VA_TYPE_T   >>(1 << 20, 1 << 20),
	std::make_unique<TypedRenderBuffer<VA_TYPE_TN  >>(1 << 16, 1 << 16),
	std::make_unique<TypedRenderBuffer<VA_TYPE_TC  >>(1 << 20, 1 << 20),
	std::make_unique<TypedRenderBuffer<VA_TYPE_TNT >>(0      , 0      ),
	std::make_unique<TypedRenderBuffer<VA_TYPE_2d0 >>(1 << 16, 1 << 16),
	std::make_unique<TypedRenderBuffer<VA_TYPE_2dC >>(1 << 16, 1 << 16),
	std::make_unique<TypedRenderBuffer<VA_TYPE_2dT >>(1 << 20, 1 << 20),
	std::make_unique<TypedRenderBuffer<VA_TYPE_2dTC>>(1 << 20, 1 << 20)
};

template<typename T>
void TypedRenderBuffer<T>::InitVAO() const
{
	if (vbo == nullptr)
		return;

	if (ebo == nullptr)
		return;

	assert(vbo);
	assert(ebo);

	vao.Bind(); //will instantiate

	vbo->Bind();
	ebo->Bind();

	for (const AttributeDef& ad : T::attributeDefs) {
		glEnableVertexAttribArray(ad.index);
		glVertexAttribDivisor(ad.index, 0);

		//assume only float or float convertible values
		glVertexAttribPointer(ad.index, ad.count, ad.type, ad.normalize, ad.stride, ad.data);
	}

	vao.Unbind();

	vbo->Unbind();
	ebo->Unbind();

	//restore default state
	for (const AttributeDef& ad : T::attributeDefs) {
		glDisableVertexAttribArray(ad.index);
	}
}