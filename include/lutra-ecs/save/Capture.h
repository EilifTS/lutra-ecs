#pragma once
#include <cassert>

#include <lutra-ecs/save/Container.h>
#include <lutra-ecs/save/Schema.h>

/**
 *  The ECS into a Container: every entity, and every component of every saved
 *  type, as bytes, exactly as the world holds them.
 *
 *  The ECS must be at rest: nothing deferred (a deferred entity is a bare id),
 *  and none of the Schema's transient types present. A debug build asserts both.
 *  Capture reads; it changes nothing.
 *
 *  The game's own parts of a save -- name tables, sections, and anything it needs
 *  to look at in the components, such as the handles in them -- are the caller's to
 *  work out from, and put into, the Container afterwards.
 */
namespace lcs::save
{
	template <typename Schema>
	[[nodiscard]] Container<typename Schema::EcsType::EntityID> Capture(typename Schema::EcsType& ecs)
	{
		using Ecs = typename Schema::EcsType;
		using EntityID = typename Ecs::EntityID;

		assert(Schema::NothingTransientIsPresent(ecs) && "a save must not run while a transient component is in the world");
		assert(!ecs.HasDeferred() && "a save must not run while something is waiting to be added");

		Container<EntityID> data{};

		for (const EntityID entity : ecs.Entities())
		{
			data.entities.push_back(entity);
		}

		Schema::ForEachSaved([&](auto type) {
			using T = typename decltype(type)::type;

			if constexpr (T::component_type == ComponentType::Tag)
			{
				for (const EntityID entity : ecs.template TView<T>())
				{
					AddTag<Schema, T>(data, entity);
				}
			}
			else
			{
				for (auto [entity, component] : ecs.template CView<T>())
				{
					AddComponent<Schema>(data, entity, component);
				}
			}
		});

		return data;
	}
}
