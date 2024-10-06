#pragma once

#include <string>
#include <string_view>
#include <array>   // std::array
#include <utility> // std::index_sequence

// https://bitwizeshift.github.io/posts/2021/03/09/getting-an-unmangled-type-name-at-compile-time/

namespace spring {
	template <unsigned N>
	struct CharN {
		char str[N];
	};

	namespace impl {
		template <std::size_t...Idxs>
		constexpr auto substring_as_char_n(std::string_view str, std::index_sequence<Idxs...>) -> CharN<sizeof...(Idxs) + 1>
		{
			constexpr unsigned N = sizeof...(Idxs);
			CharN<N + 1> res = {};
			res.str[N] = '\0';
			for (unsigned i = 0; i < N; ++i) {
				res.str[i] = str[i];
			}
			return res;
		}

		template <typename T>
		constexpr auto type_name_array()
		{
		#if defined(__clang__)
			constexpr auto prefix = std::string_view{ "[T = " };
			constexpr auto suffix = std::string_view{ "]" };
			constexpr auto function = std::string_view{ __PRETTY_FUNCTION__ };
		#elif defined(__GNUC__)
			constexpr auto prefix = std::string_view{ "with T = " };
			constexpr auto suffix = std::string_view{ "]" };
			constexpr auto function = std::string_view{ __PRETTY_FUNCTION__ };
		#elif defined(_MSC_VER)
			constexpr auto space = std::string_view{ " " };
			constexpr auto prefix = std::string_view{ "type_name_array<" };
			constexpr auto suffix = std::string_view{ ">(void)" };
			constexpr auto function = std::string_view{ __FUNCSIG__ };
		#else
			#error Unsupported compiler
		#endif

			constexpr auto start = function.find(prefix) + prefix.size();
			constexpr auto end = function.rfind(suffix);

			static_assert(start < end);

			constexpr auto name = function.substr(start, (end - start));
		#if defined(_MSC_VER)
			constexpr auto begSp = name.find(space) + space.size();

			static_assert(begSp < name.size());

			constexpr auto cleanName = name.substr(begSp); //remove "class "/"struct ", etc
			return substring_as_char_n(cleanName, std::make_index_sequence<cleanName.size()>{});
		#else
			return substring_as_char_n(name, std::make_index_sequence<name.size()>{});
		#endif
		}

		template <typename T>
		struct type_name_holder {
			static inline constexpr auto value = type_name_array<T>();
		};
		template <>
		struct type_name_holder<std::string> {
			static constexpr std::string_view tn = std::string_view{ "std::string" };
			static inline constexpr auto value = substring_as_char_n(tn, std::make_index_sequence<tn.size()>{});
		};
	}

	template<unsigned ...Len>
	constexpr auto Concat(const char (&...strings)[Len])
	{
		constexpr unsigned N = (... + Len) - sizeof...(Len);
		CharN<N + 1> res = {};
		res.str[N] = '\0';
		unsigned i = 0;
		for (const char* src : {strings...}) {
			for (; *src != '\0'; src++) {
				res.str[i++] = *src;
			}
		}
		return res;
	}

	template <typename T>
	constexpr auto TypeToCharN()
	{
		return impl::type_name_holder<T>::value;
	}

	template <typename T>
	constexpr auto TypeToStr() -> std::string_view
	{
		return std::string_view{impl::type_name_holder<T>::value.str};
	}

	template <typename T>
	constexpr auto TypeToCStr() -> const char*
	{
		return impl::type_name_holder<T>::value.str;
	}
}
