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
	template<auto FuncPtr, class... FallbackSignature>
	struct FuncPtrSignatureTpl {
		using type = FuncSignature<std::remove_pointer_t<std::remove_pointer_t<decltype(FuncPtr)>>>;
	};
	template<class... FallbackSignature>
	struct FuncPtrSignatureTpl<nullptr, FallbackSignature...> {
		using type = std::tuple<FallbackSignature...>;
	};
}

// This particular helper accepts a nullptr, in which case it falls back to a specified default signature
template<auto FuncPtr, class... FallbackSignature>
using FuncPtrSignature = typename Impl::FuncPtrSignatureTpl<FuncPtr, FallbackSignature...>::type;


namespace Impl
{
	template<class TupleType, class Type>
	struct TupleContainsType;

	template<class Type, class... TupleElementTypes>
	struct TupleContainsType<std::tuple<TupleElementTypes...>, Type> : std::disjunction<std::is_same<Type, TupleElementTypes>...> {};
}

template<class TupleType, class Type>
constexpr inline bool TupleContainsType = Impl::TupleContainsType<TupleType, Type>::value;
