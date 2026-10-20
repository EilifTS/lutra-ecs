#pragma once
#include <array>
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
 *  copyable. Whatever the bytes do not say lives in a traits type per component,
 *  next to the id it is saved under: which of its fields are entity ids (checked
 *  against the file's entities), and whether the bytes mean something that can
 *  be used (a number that is not a number, an enum past its last value, a flag
 *  that is not 0 or 1 would otherwise reach a system and index out of bounds).
 *
 *  Adding a component to the ECS means choosing: saved, with an id that is never
 *  reused, or transient -- one that cannot exist when a save runs. A Schema that
 *  finds a component with no choice made, or two, breaks the build.
 *
 *  The traits are the user's own template, specialised per saved type:
 *
 *      template <typename T> struct SaveTraits;
 *      template <> struct SaveTraits<Position> : lcs::save::NoRules
 *      {
 *          static constexpr uint32_t id = 1;
 *          static constexpr const char* name = "Position";
 *          static bool Valid(std::span<const std::byte> raw) { ... }
 *      };
 *
 *      using MySchema = lcs::save::Schema<Ecs, SaveTraits, lcs::TypeList<Position, ...>, lcs::TypeList<Intent, ...>>;
 *
 *  It may carry more than the library reads (the game's own policy hooks, say);
 *  what the library reads is `id` (anything that converts to uint32_t, an enum
 *  class's value included, via static_cast), `name`, `ForEachEntityRef`,
 *  `dangling` and `Valid`. NoRules supplies the last three.
 */
namespace lcs::save
{
	/* What to do with a saved entity id that the file does not hold. */
	enum class OnDanglingRef
	{
		ClearTheRef,       /* Becomes the invalid id; the component stays. */
		DropTheComponent,  /* The component means nothing without it. */
	};

	/* The base for a traits type: no entity ids, and any bytes will do. */
	struct NoRules
	{
		/* Calls visit(EntityID&) for each entity id the component holds. */
		template <typename C, typename V> static void ForEachEntityRef(C&, V&&) {}
		static constexpr OnDanglingRef dangling = OnDanglingRef::ClearTheRef;

		/* `raw` is exactly sizeof(component) bytes. */
		static bool Valid(const std::span<const std::byte>) { return true; }
	};

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
		template <template <typename> class Traits, typename T>
		constexpr SchemaEntry MakeEntry()
		{
			/* Raw bytes only work for a type that can be copied as bytes. */
			static_assert(std::is_trivially_copyable_v<T>, "a saved component is written as its bytes");

			constexpr bool is_tag = T::component_type == ComponentType::Tag;
			return SchemaEntry{ static_cast<uint32_t>(Traits<T>::id), Traits<T>::name, is_tag ? 0u : uint32_t(sizeof(T)), is_tag };
		}

		/* Instantiated once per ECS type, so the compiler names the one that was not
		 * chosen for. */
		template <typename T, typename Saved, typename Transient>
		struct ChosenFor
		{
			static_assert(IsOneOf<T>(Saved{}) || IsOneOf<T>(Transient{}),
				"a component of the ECS is neither saved nor transient: add it to the Schema's saved types (with a traits type and an id that is never reused) or to its transient types");
			static_assert(!(IsOneOf<T>(Saved{}) && IsOneOf<T>(Transient{})),
				"a component is both saved and transient in the Schema");

			static constexpr bool value = true;
		};

		template <typename Saved, typename Transient, typename... Ts>
		constexpr bool EveryTypeIsChosenFor(TypeList<Ts...>)
		{
			return (ChosenFor<Ts, Saved, Transient>::value && ...);
		}
	}

	template <typename Ecs, template <typename> class Traits, typename Saved, typename Transient>
	struct Schema;

	template <typename Ecs, template <typename> class Traits, typename... Saved, typename... Transient>
	struct Schema<Ecs, Traits, TypeList<Saved...>, TypeList<Transient...>>
	{
		using EcsType = Ecs;
		using SavedTypes = TypeList<Saved...>;
		using TransientTypes = TypeList<Transient...>;

		static constexpr std::array<SchemaEntry, sizeof...(Saved)> entries{ detail::MakeEntry<Traits, Saved>()... };

		static_assert(IdsAreUnique(entries), "two saved components share an id");

		/* A component added to the ECS without a decision about saving breaks the
		 * build, here, instead of silently going missing from every save. */
		static_assert(detail::EveryTypeIsChosenFor<SavedTypes, TransientTypes>(typename Ecs::Types{}));
		static_assert(EveryTypeIsIn(SavedTypes{}, typename Ecs::Types{}), "the Schema's saved types name a type that is not in the ECS");
		static_assert(EveryTypeIsIn(TransientTypes{}, typename Ecs::Types{}), "the Schema's transient types name a type that is not in the ECS");

		/* The traits of one saved type, as the Schema was given them. */
		template <typename T>
		using TraitsOf = Traits<T>;

		template <typename T>
		static constexpr SchemaEntry EntryOf()
		{
			static_assert(IsOneOf<T>(SavedTypes{}), "this type is not one of the Schema's saved types");
			return detail::MakeEntry<Traits, T>();
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
				if (static_cast<uint32_t>(Traits<T>::id) == id)
				{
					valid = Traits<T>::Valid(raw);
				}
			});
			return valid;
		}

		/* Calls visit(std::type_identity<T>{}) for each saved type, in list order. */
		template <typename Visitor>
		static void ForEachSaved(Visitor&& visit)
		{
			ForEachType(SavedTypes{}, visit);
		}

		/* The transient types are the ones a save must never find: the systems before
		 * it have consumed or cleared them. A save asserts this in a debug build. */
		static bool NothingTransientIsPresent(Ecs& ecs)
		{
			return (ecs.template IsEmpty<Transient>() && ...);
		}
	};
}
