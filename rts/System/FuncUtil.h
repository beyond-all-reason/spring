/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <tuple>
#include <type_traits>

namespace Impl
{
	template<class FuncType>
	struct FuncSignatureTpl;
	
	template<class ReturnType, class... ArgTypes>
	struct FuncSignatureTpl<ReturnType(ArgTypes...)> {
		using type = std::tuple<ArgTypes...>;
	};
}

template<class FuncType>
using FuncSignature = typename Impl::FuncSignatureTpl<FuncType>::type;

template<
    typename F,
    std::enable_if_t<std::is_function_v<F>, bool> = true
>
auto FuncArgs(const F&) -> typename FuncSignature<F>::type;
template<
    typename F,
    std::enable_if_t<std::is_function_v<F>, bool> = true
>
auto FuncArgs(const F*) -> typename FuncSignature<F>::type;


namespace Impl
{
	template<auto FuncPtr, class... NullPtrFallbackSignature>
	struct FuncPtrSignatureTpl {
		using type = FuncSignature<std::remove_pointer_t<std::remove_pointer_t<decltype(FuncPtr)>>>;
	};
	template<class... NullPtrFallbackSignature>
	struct FuncPtrSignatureTpl<nullptr, NullPtrFallbackSignature...> {
		using type = std::tuple<NullPtrFallbackSignature...>;
	};
}

template<auto FuncPtr, class... NullPtrFallbackSignature>
using FuncPtrSignature = typename Impl::FuncPtrSignatureTpl<FuncPtr, NullPtrFallbackSignature...>::type;
