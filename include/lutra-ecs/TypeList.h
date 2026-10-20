#pragma once
#include <type_traits>

namespace lcs
{
	/* A list of types, to be passed around and unpacked. */
	template <typename... Ts>
	struct TypeList {};

	template <typename T, typename... Ts>
	constexpr bool IsOneOf(TypeList<Ts...>)
	{
		return (std::is_same_v<T, Ts> || ...);
	}

	/* Lists joined, in order. */
	template <typename... As, typename... Bs>
	constexpr TypeList<As..., Bs...> operator+(TypeList<As...>, TypeList<Bs...>)
	{
		return {};
	}

	/* Every type of the first list is in the second. */
	template <typename... Ts, typename... Packed>
	constexpr bool EveryTypeIsIn(TypeList<Ts...>, TypeList<Packed...> pack)
	{
		return (IsOneOf<Ts>(pack) && ...);
	}

	/* Calls visit(std::type_identity<T>{}) for each type, in list order. */
	template <typename Visitor, typename... Ts>
	void ForEachType(TypeList<Ts...>, Visitor&& visit)
	{
		(visit(std::type_identity<Ts>{}), ...);
	}
}
