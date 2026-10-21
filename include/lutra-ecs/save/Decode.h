#pragma once
#include <cstddef>
#include <string>
#include <tuple>
#include <unordered_set>
#include <utility>
#include <vector>

#include <lutra-ecs/save/Container.h>
#include <lutra-ecs/save/Report.h>
#include <lutra-ecs/save/Schema.h>

/**
 *  A Container into components the game can use, and those into an ECS.
 *
 *  Decode reads every component as the bytes it was and makes nothing of what it
 *  holds, without touching the ECS or anything else of the game's. What the game
 *  has to do to the components before they are used -- replace the handles in them
 *  with its own, leave out ones that name something gone -- it does to the Decoded
 *  it gets back, before anything is put in the world. So everything that can be
 *  wrong with a file has been found by the time Restore is called, and applying it
 *  cannot fail halfway and cost the world its entities. That order -- decode,
 *  fix up, then clear the world, then restore -- is the caller's.
 */
namespace lcs::save
{
	template <typename EntityID, typename T>
	using Entries = std::vector<std::pair<EntityID, T>>;

	namespace detail
	{
		template <typename EntityID, typename... Ts>
		std::tuple<Entries<EntityID, Ts>...> EntriesTuple(TypeList<Ts...>);
	}

	/* Every saved component, typed, by entity: in the shape of the ECS's own deferred
	 * storage. */
	template <typename Schema>
	struct Decoded
	{
		using EntityID = typename Schema::EcsType::EntityID;
		using Components = decltype(detail::EntriesTuple<EntityID>(typename Schema::SavedTypes{}));

		std::vector<EntityID> entities{}; /* Ascending by index. */
		Components components{};

		template <typename T> Entries<EntityID, T>& Of() { return std::get<Entries<EntityID, T>>(components); }
		template <typename T> const Entries<EntityID, T>& Of() const { return std::get<Entries<EntityID, T>>(components); }
	};

	/* A component of an entity the save does not list is an error. A chunk of a
	 * component this build does not know (see Container::skipped_component_names) is
	 * reported and skipped.
	 *
	 * What a component holds is not looked at -- an entity id, a float, an enum are
	 * values like any other (see the note on Container). */
	template <typename Schema>
	[[nodiscard]] Result<Decoded<Schema>> Decode(const Container<typename Schema::EcsType::EntityID>& data, const Reporter& reporter)
	{
		using EntityID = typename Schema::EcsType::EntityID;

		for (const std::string& name : data.skipped_component_names)
		{
			Report(reporter, Severity::Warning, "save", "a component named '" + name + "' is not one this build knows, skipped");
		}

		std::unordered_set<uint32_t> listed;
		for (const EntityID entity : data.entities)
		{
			listed.insert(entity.handle);
		}

		for (const ComponentChunk<EntityID>& chunk : data.chunks)
		{
			for (const EntityID entity : chunk.entities)
			{
				if (!listed.contains(entity.handle))
				{
					return std::unexpected(MakeError(ErrorKind::Malformed, "a component belongs to entity " + std::to_string(entity.GetIndex()) + ", which the save does not list"));
				}
			}
		}

		Decoded<Schema> out;
		out.entities = data.entities;

		Schema::ForEachSaved([&](auto type) {
			using T = typename decltype(type)::type;

			const ComponentChunk<EntityID>* chunk = FindChunk(data, SaveNameOf<T>());
			if (chunk == nullptr) return;

			Entries<EntityID, T>& entries = out.template Of<T>();
			entries.reserve(chunk->Count());
			for (size_t index = 0; index < chunk->Count(); index++)
			{
				if constexpr (T::component_type == ComponentType::Tag)
				{
					entries.emplace_back(chunk->entities[index], T{});
				}
				else
				{
					entries.emplace_back(chunk->entities[index], ReadComponent<T>(*chunk, index));
				}
			}
		});

		return out;
	}

	/* Creates the decoded entities in the ECS, with exactly their ids, and calls
	 * visit(entity, component) for every decoded component in the Schema's order --
	 * which is where the caller defers it (ecs.AddDeferred), after changing it or
	 * making whatever else it needs. Nothing is flushed. The ECS must be as new:
	 * cleared, with nothing deferred. */
	template <typename Schema, typename Visitor>
	void Restore(typename Schema::EcsType& ecs, const Decoded<Schema>& decoded, Visitor&& visit)
	{
		ecs.CreateEntitiesFromIDs(decoded.entities);

		Schema::ForEachSaved([&](auto type) {
			using T = typename decltype(type)::type;
			for (const auto& [entity, component] : decoded.template Of<T>())
			{
				visit(entity, component);
			}
		});
	}
}
