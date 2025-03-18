#pragma once

#include <memory>
#include <functional>
#include <tuple>
#include <type_traits>

namespace spring {
	template<bool...> struct bool_pack;
	template<bool... bs>
	using all_true = std::is_same<bool_pack<bs..., true>, bool_pack<true, bs...>>;

	// check if parameters pack is constructible
	template<class... Ts>
	using are_all_constructible = all_true<std::is_constructible_v<std::decay_t<Ts>>...>;

	// https://stackoverflow.com/questions/38067106/c-verify-callable-signature-of-template-type
	template<typename, typename, typename = void>
	struct is_signature : std::false_type {};

	// use static_assert(is_signature<SomeTemplateF, int(int)>::value);
	template<typename TFunc, typename Ret, typename... Args>
	struct is_signature<TFunc, Ret(Args...),
			typename std::enable_if_t<
				std::is_convertible<TFunc, std::function<Ret(Args...)>>::value
			>
		> : public std::true_type
	{};

	// use static_assert(is_signature<std::function<int(int)>, decltype(someFunction)>::value);
	template<typename TFunc1, typename TFunc2>
	struct is_signature<TFunc1, TFunc2,
			typename std::enable_if_t<
				std::is_convertible<TFunc1, TFunc2>::value
			>
		> : public std::true_type
	{};
	
	template<typename... Args>
	inline constexpr bool is_signature_v = is_signature<Args...>::value;

	// cpp20 compat
	template< class T >
	struct remove_cvref {
		typedef std::remove_cv_t<std::remove_reference_t<T>> type;
	};

	// https://stackoverflow.com/questions/8194227/how-to-get-the-i-th-element-from-an-stdtuple-when-i-isnt-know-at-compile-time
	template<std::size_t I = 0, typename FuncT, typename... Tp>
	inline typename std::enable_if<I == sizeof...(Tp), void>::type
		tuple_exec_at(int, std::tuple<Tp...>&, FuncT)
	{}

	template<std::size_t I = 0, typename FuncT, typename... Tp>
	inline typename std::enable_if < I < sizeof...(Tp), void>::type
		tuple_exec_at(int index, std::tuple<Tp...>& t, FuncT f)
	{
		if (index == 0)
			f(std::get<I>(t));

		tuple_exec_at<I + 1, FuncT, Tp...>(index - 1, t, f);
	}

	// https://blog.tartanllama.xyz/exploding-tuples-fold-expressions/
	template <std::size_t... Idx>
	auto make_index_dispatcher(std::index_sequence<Idx...>) {
		return [](auto&& f) { (f(std::integral_constant<std::size_t, Idx>{}), ...); };
	}

	template <std::size_t N>
	auto make_index_dispatcher() {
		return make_index_dispatcher(std::make_index_sequence<N>{});
	}

	template<typename T>
	struct return_type { using type = T; };

	template<typename R, typename... Ts>
	struct return_type<std::function<R(Ts...)>> { using type = R; };

	template<typename R, typename... Ts>
	struct return_type<std::function<R(Ts...)> const> { using type = R; };

	template<typename R, typename T, typename... Ts>
	struct return_type<std::function<R(Ts...)> T::*> { using type = R; };

	template<typename R, typename T, typename... Ts>
	struct return_type<std::function<R(Ts...)> const T::*> { using type = R; };

	template<typename R, typename T, typename... Ts>
	struct return_type<std::function<R(Ts...)> T::* const&> { using type = R; };

	template<typename R, typename T, typename... Ts>
	struct return_type<std::function<R(Ts...)> const T::* const> { using type = R; };

	template<typename R, typename... Ts>
	struct return_type<R(*)(Ts...)> { using type = R; };

	template<typename R, typename... Ts>
	struct return_type<R& (*)(Ts...)> { using type = R; };

	template<typename R, typename T>
	struct return_type<R(T::*)() const> { using type = R; };

	template<typename R, typename T>
	struct return_type<R& (T::*)() const> { using type = R; };

	template<typename R, typename T>
	struct return_type<std::shared_ptr<R>(T::*)() const> { using type = R; };

	template<typename R, typename T>
	struct return_type<std::shared_ptr<R>& (T::*)() const> { using type = R; };

	template<typename R, typename T>
	struct return_type<R(T::* const)() const> { using type = R; };

	template<typename R, typename T>
	struct return_type<R& (T::* const)() const> { using type = R; };

	template<typename R, typename T>
	struct return_type<std::shared_ptr<R>(T::* const)() const> { using type = R; };

	template<typename R, typename T>
	struct return_type<std::shared_ptr<R>& (T::* const)() const> { using type = R; };

	template<typename R, typename T>
	struct return_type<R(T::* const&)() const> { using type = R; };

	template<typename R, typename T>
	struct return_type<R& (T::* const&)() const> { using type = R; };

	template<typename R, typename T>
	struct return_type<std::shared_ptr<R>(T::* const&)() const> { using type = R; };

	template<typename R, typename T>
	struct return_type<std::shared_ptr<R>& (T::* const&)() const> { using type = R; };

	template<typename T>
	using return_type_t = typename return_type<T>::type;
	

	template<typename TupleType, typename Type>
	struct tuple_contains_type;

	template<typename Type, typename... TupleElementTypes>
	struct tuple_contains_type<std::tuple<TupleElementTypes...>, Type> : std::disjunction<std::is_same<Type, TupleElementTypes>...> {};

	template<typename TupleType, typename Type>
	constexpr inline bool tuple_contains_type_v = tuple_contains_type<TupleType, Type>::value;


	template <typename T, typename Tuple>
	struct tuple_type_index;

	template <typename T, typename... Types>
	struct tuple_type_index<T, std::tuple<T, Types...>> {
		static const std::size_t value = 0;
	};

	template <typename T, typename U, typename... Types>
	struct tuple_type_index<T, std::tuple<U, Types...>> {
		static const std::size_t value = 1 + tuple_type_index<T, std::tuple<Types...>>::value;
	};
	template <typename T, typename Tuple>
	constexpr size_t tuple_type_index_v = tuple_type_index<T, Tuple>::value;


	template<typename FuncType>
	struct func_signature;

	template<typename ReturnType, typename... ArgTypes>
	struct func_signature<ReturnType(ArgTypes...)> {
		using type = std::tuple<ArgTypes...>;
		using ret = ReturnType;
	};

	template<typename FuncType>
	using func_signature_t = typename func_signature<FuncType>::type;

	template<
		typename F,
		std::enable_if_t<std::is_function<F>::value, bool> = true
	>
	auto arg_types_tuple_t(const F&) -> typename func_signature<F>::type;
	template<
		typename F,
		std::enable_if_t<std::is_function<F>::value, bool> = true
	>
	auto arg_types_tuple_t(const F*) -> typename func_signature<F>::type;


	// This particular helper accepts a nullptr, in which case it falls back to a specified default signature, or just an empty tuple
	template<auto FuncPtr, typename... FallbackSignature>
	struct func_ptr_signature {
		using type = func_signature_t<std::remove_pointer_t<std::remove_pointer_t<decltype(FuncPtr)>>>;
	};
	template<typename... FallbackSignature>
	struct func_ptr_signature<nullptr, FallbackSignature...> {
		using type = std::tuple<FallbackSignature...>;
	};

	template<auto FuncPtr, typename... FallbackSignature>
	using func_ptr_signature_t = typename func_ptr_signature<FuncPtr, FallbackSignature...>::type;
};