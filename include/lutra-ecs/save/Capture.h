#pragma once
#include <cassert>
#include <cstddef>
#include <span>
#include <string>

#include <lutra-ecs/save/Container.h>
#include <lutra-ecs/save/Report.h>
#include <lutra-ecs/save/Schema.h>

/**
 *  The ECS into a Container: every entity, and every component of every saved
 *  type, as bytes.
 *
 *  The ECS must be at rest: nothing deferred (a deferred entity is a bare id),
 *  and none of the Schema's transient types present. A debug build asserts both.
 *  Capture reads; it changes nothing.
 *
 *  A component is left out, and reported, when loading it back would fail:
 *  bytes the component calls invalid (a number that is not a number), or a policy
 *  that vetoes it. The game's own parts of a save -- name tables, sections --
 *  are the caller's to put in the Container afterwards.
 */
namespace lcs::save
{
	template <typename Ecs>
	struct CaptureResult
	{
		Container<typename Ecs::EntityID> data{};

		/* Components left out because loading them back would fail. Each was reported. */
		size_t dropped_components{ 0 };
	};

	/* A policy is any type with
	 *
	 *     template <typename T> Verdict Accept(EntityID entity, T& copy);
	 *
	 * called with a copy of each plain component that passed Valid, before it is
	 * written. It may look at the copy (is every handle in it nameable?) and it
	 * may remember it (which handles are used), and says Keep or Drop(why). It is
	 * not called for tags, which are only an entity id. */
	struct AcceptEverything
	{
		template <typename EntityID, typename T>
		Verdict Accept(EntityID, T&) { return Verdict::Keep(); }
	};

	template <typename Schema, typename Policy>
	[[nodiscard]] CaptureResult<typename Schema::EcsType> Capture(typename Schema::EcsType& ecs, Policy& policy, const Reporter& reporter)
	{
		using Ecs = typename Schema::EcsType;
		using EntityID = typename Ecs::EntityID;

		assert(Schema::NothingTransientIsPresent(ecs) && "a save must not run while a transient component is in the world");
		assert(!ecs.HasDeferred() && "a save must not run while something is waiting to be added");

		CaptureResult<Ecs> result{};

		for (const EntityID entity : ecs.Entities())
		{
			result.data.entities.push_back(entity);
		}

		const auto drop = [&](const char* name, const EntityID entity, const std::string& why) {
			Report(reporter, Severity::Error, name, "entity " + std::to_string(entity.GetIndex()) + " not saved: " + why);
			result.dropped_components++;
		};

		Schema::ForEachSaved([&](auto type) {
			using T = typename decltype(type)::type;
			constexpr const char* name = SaveNameOf<T>();

			if constexpr (T::component_type == ComponentType::Tag)
			{
				for (const EntityID entity : ecs.template TView<T>())
				{
					AddTag<Schema, T>(result.data, entity);
				}
			}
			else
			{
				for (auto [entity, component] : ecs.template CView<T>())
				{
					T copy = component;

					if (!SaveBytesAreValid<T>(std::as_bytes(std::span(&copy, 1))))
					{
						drop(name, entity, "its values are not ones a save can load back (a number that is not a number, or a value out of range)");
						continue;
					}

					const Verdict verdict = policy.Accept(entity, copy);
					if (verdict.kind != Verdict::Kind::Keep)
					{
						drop(name, entity, verdict.why);
						continue;
					}

					AddComponent<Schema>(result.data, entity, copy);
				}
			}
		});

		return result;
	}

	template <typename Schema>
	[[nodiscard]] CaptureResult<typename Schema::EcsType> Capture(typename Schema::EcsType& ecs, const Reporter& reporter)
	{
		AcceptEverything policy;
		return Capture<Schema>(ecs, policy, reporter);
	}
}
