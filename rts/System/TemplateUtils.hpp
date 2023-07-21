#include <functional>

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
};