#pragma once
#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <type_traits>

#include <lutra-ecs/ECSManager.h>
#include <lutra-ecs/TypeList.h>

/**
 *  What gets saved, and what each saved component says about itself.
 *
 *  A component is written as its bytes, so a saved type must be trivially
 *  copyable, and is read back as the bytes it was: a value is what the world held,
 *  and a load does not second-guess it. What a component says about itself is
 *  members beside its component_type:
 *
 *      struct Position
 *      {
 *          static constexpr lcs::ComponentType component_type = lcs::ComponentType::Component;
 *          static constexpr bool saved = true;
 *          static constexpr const char* save_name = "Position"; // what the file calls it
 *
 *          float x, y;
 *      };
 *
 *      struct Follower
 *      {
 *          ...
 *          EntityID target;  // an entity id is a value like any other: see Container.h
 *      };
 *
 *      struct Intent
 *      {
 *          static constexpr lcs::ComponentType component_type = lcs::ComponentType::Component;
 *          static constexpr bool saved = false;  // transient: cannot exist when a save runs
 *      };
 *
 *      using MySchema = lcs::save::Schema<Ecs>;
 *
 *  Every component and tag of the ECS says whether it is saved. One that does not
 *  breaks the build, so none goes missing from every save unnoticed; one that is
 *  saved without a save_name does too. The saved types are visited in
 *  the order the ECS lists its types. A game's own hooks (the policies of Capture
 *  and Decode) can look for members of their own in the same way.
 */
namespace lcs::save
{
	namespace detail
	{
		template <typename T>
		concept SaysWhetherItIsSaved = requires { { T::saved } -> std::convertible_to<bool>; };

		template <typename T>
		concept HasSaveName = requires { { T::save_name } -> std::convertible_to<const char*>; };
	}

	/* The name a saved type is written under: the identity of the component in a file,
	 * so it is never changed (a renamed component is a component the file does not
	 * know). */
	template <typename T>
	constexpr const char* SaveNameOf()
	{
		return T::save_name;
	}

	/* One saved type, as data: for reading a file that names a component. */
	struct SchemaEntry
	{
		std::string_view name;
		uint32_t size; /* Bytes of one component; 0 for a tag. */
		bool is_tag;
	};

	constexpr bool NamesAreUnique(const std::span<const SchemaEntry> entries)
	{
		for (size_t a = 0; a < entries.size(); a++)
		{
			for (size_t b = a + 1; b < entries.size(); b++)
			{
				if (entries[a].name == entries[b].name) return false;
			}
		}
		return true;
	}

	namespace detail
	{
		/* Instantiated once per ECS type, so the compiler names the one that has not said. */
		template <typename T>
		struct Decision
		{
			static_assert(SaysWhetherItIsSaved<T>,
				"a component of the ECS does not say whether it is saved: give it `static constexpr bool saved = false;` if it is transient, or `= true` with a save_name that is never changed");

			static constexpr bool saved = [] {
				if constexpr (SaysWhetherItIsSaved<T>) return bool(T::saved);
				else return false;
			}();
		};

		template <typename T>
		constexpr SchemaEntry MakeEntry()
		{
			/* Raw bytes only work for a type that can be copied as bytes. */
			static_assert(std::is_trivially_copyable_v<T>, "a saved component is written as its bytes");
			static_assert(HasSaveName<T>, "a saved component needs `static constexpr const char* save_name`, which is what a file calls it and is never changed");

			constexpr bool is_tag = T::component_type == ComponentType::Tag;
			return SchemaEntry{ SaveNameOf<T>(), is_tag ? 0u : uint32_t(sizeof(T)), is_tag };
		}

		template <typename... Ts>
		constexpr auto MakeEntries(TypeList<Ts...>)
		{
			return std::array<SchemaEntry, sizeof...(Ts)>{ MakeEntry<Ts>()... };
		}

		/* The ECS's types that are saved, in the ECS's order. */
		template <typename... Ts>
		constexpr auto SavedOf(TypeList<Ts...>)
		{
			return (TypeList<>{} + ... + std::conditional_t<Decision<Ts>::saved, TypeList<Ts>, TypeList<>>{});
		}

		template <typename... Ts>
		constexpr auto TransientOf(TypeList<Ts...>)
		{
			return (TypeList<>{} + ... + std::conditional_t<Decision<Ts>::saved, TypeList<>, TypeList<Ts>>{});
		}

		template <typename Ecs, typename... Ts>
		bool NothingPresent(Ecs& ecs, TypeList<Ts...>)
		{
			return (ecs.template IsEmpty<Ts>() && ...);
		}
	}

	template <typename Ecs>
	struct Schema
	{
		using EcsType = Ecs;
		using SavedTypes = decltype(detail::SavedOf(typename Ecs::Types{}));
		using TransientTypes = decltype(detail::TransientOf(typename Ecs::Types{}));

		static constexpr auto entries = detail::MakeEntries(SavedTypes{});

		static_assert(NamesAreUnique(entries), "two saved components share a save_name");

		template <typename T>
		static constexpr SchemaEntry EntryOf()
		{
			static_assert(IsOneOf<T>(SavedTypes{}), "this type is not one of the Schema's saved types");
			return detail::MakeEntry<T>();
		}

		/* Calls visit(std::type_identity<T>{}) for each saved type, in the ECS's order. */
		template <typename Visitor>
		static void ForEachSaved(Visitor&& visit)
		{
			ForEachType(SavedTypes{}, visit);
		}

		/* The transient types are the ones a save must never find: the systems before
		 * it have consumed or cleared them. A save asserts this in a debug build. */
		static bool NothingTransientIsPresent(Ecs& ecs)
		{
			return detail::NothingPresent(ecs, TransientTypes{});
		}
	};
}
