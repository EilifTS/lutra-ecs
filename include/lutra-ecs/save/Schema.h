#pragma once
#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>

#include <lutra-ecs/ECSManager.h>
#include <lutra-ecs/TypeList.h>

/**
 *  What gets saved, and what each saved component says about itself.
 *
 *  A component is written as its bytes, so a saved type must be trivially
 *  copyable. Whatever the bytes do not say, the component says itself, with
 *  members beside its component_type:
 *
 *      struct Position
 *      {
 *          static constexpr lcs::ComponentType component_type = lcs::ComponentType::Component;
 *          static constexpr bool saved = true;
 *          static constexpr uint32_t save_id = 1;              // never changed or reused
 *          static constexpr const char* save_name = "Position"; // for messages
 *
 *          float x, y;
 *
 *          // Optional: are these bytes a value a system can use? A number that is not a
 *          // number, an enum past its last value, a flag that is not 0 or 1 would
 *          // otherwise reach a system and index out of bounds. `raw` is exactly
 *          // sizeof(Position) bytes, and is looked at before it is made into a Position.
 *          static bool SaveBytesValid(std::span<const std::byte> raw) { ... }
 *      };
 *
 *      struct Follower
 *      {
 *          ...
 *          EntityID target;
 *
 *          // Optional: the entity ids it holds, which are checked against the
 *          // file's entities.
 *          template <typename V> void ForEachEntityRef(V&& visit) { visit(target); }
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
 *  saved without a save_id or a save_name does too. The saved types are visited in
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
		concept HasSaveIdAndName = requires {
			{ static_cast<uint32_t>(T::save_id) };
			{ T::save_name } -> std::convertible_to<const char*>;
		};

		template <typename T>
		concept ChecksItsBytes = requires (std::span<const std::byte> raw) { { T::SaveBytesValid(raw) } -> std::convertible_to<bool>; };

		template <typename T>
		concept HoldsEntityIds = requires (T& component) { component.ForEachEntityRef([](auto&) {}); };
	}

	/* The id a saved type is written under. */
	template <typename T>
	constexpr uint32_t SaveIdOf()
	{
		return static_cast<uint32_t>(T::save_id);
	}

	template <typename T>
	constexpr const char* SaveNameOf()
	{
		return T::save_name;
	}

	/* Whether the bytes of one component of type T are safe to use: what its
	 * SaveBytesValid says, or yes if it has none. */
	template <typename T>
	bool SaveBytesAreValid(const std::span<const std::byte> raw)
	{
		if constexpr (detail::ChecksItsBytes<T>)
		{
			return T::SaveBytesValid(raw);
		}
		else
		{
			return true;
		}
	}

	/* Calls visit(EntityID&) for each entity id the component holds: what its
	 * ForEachEntityRef does, or nothing if it has none. */
	template <typename T, typename Visitor>
	void ForEachEntityRef(T& component, Visitor&& visit)
	{
		if constexpr (detail::HoldsEntityIds<T>)
		{
			component.ForEachEntityRef(visit);
		}
	}

	/* One saved type, as data: for reading a file that names a component by id. */
	struct SchemaEntry
	{
		uint32_t id;
		const char* name;
		uint32_t size; /* Bytes of one component; 0 for a tag. */
		bool is_tag;
	};

	constexpr bool IdsAreUnique(const std::span<const SchemaEntry> entries)
	{
		for (size_t a = 0; a < entries.size(); a++)
		{
			for (size_t b = a + 1; b < entries.size(); b++)
			{
				if (entries[a].id == entries[b].id) return false;
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
				"a component of the ECS does not say whether it is saved: give it `static constexpr bool saved = false;` if it is transient, or `= true` with a save_id that is never reused and a save_name");

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
			static_assert(HasSaveIdAndName<T>, "a saved component needs `static constexpr uint32_t save_id` (never reused) and `static constexpr const char* save_name`");

			constexpr bool is_tag = T::component_type == ComponentType::Tag;
			return SchemaEntry{ SaveIdOf<T>(), SaveNameOf<T>(), is_tag ? 0u : uint32_t(sizeof(T)), is_tag };
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

		static_assert(IdsAreUnique(entries), "two saved components share an id");

		template <typename T>
		static constexpr SchemaEntry EntryOf()
		{
			static_assert(IsOneOf<T>(SavedTypes{}), "this type is not one of the Schema's saved types");
			return detail::MakeEntry<T>();
		}

		/* Null for an id this build does not know. */
		static const SchemaEntry* Find(const uint32_t id)
		{
			for (const SchemaEntry& entry : entries)
			{
				if (entry.id == id) return &entry;
			}
			return nullptr;
		}

		/* Whether the bytes of one component, of the type with this id, are safe to
		 * use. False for an id this build does not know, and for `raw` that is not
		 * exactly the component's size. */
		static bool BytesAreValid(const uint32_t id, const std::span<const std::byte> raw)
		{
			const SchemaEntry* entry = Find(id);
			if (entry == nullptr || raw.size() != entry->size)
			{
				return false;
			}

			bool valid = true;
			ForEachSaved([&](auto type) {
				using T = typename decltype(type)::type;
				if (SaveIdOf<T>() == id)
				{
					valid = SaveBytesAreValid<T>(raw);
				}
			});
			return valid;
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
