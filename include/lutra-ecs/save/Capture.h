#pragma once
#include <cassert>
#include <concepts>
#include <cstddef>
#include <span>
#include <string>
#include <type_traits>

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
 *  A component is written as it is, whatever it holds, unless a policy vetoes it:
 *  then it is left out and reported. The game's own parts of a save -- name
 *  tables, sections -- are the caller's to put in the Container afterwards.
 */
namespace lcs::save
{
	template <typename Ecs>
	struct CaptureResult
	{
		Container<typename Ecs::EntityID> data{};

		/* Components a policy left out. Each was reported. */
		size_t dropped_components{ 0 };
	};

	namespace detail
	{
		template <typename P, typename EntityID, typename T>
		concept AcceptsComponent = requires (P& policy, EntityID entity, T& copy) { { policy.Accept(entity, copy) } -> std::same_as<Verdict>; };

		/* The policy's verdict on a component, or Keep if it has no hook for the type.
		 * A function of its own so that looking up `Accept` waits for the policy's type. */
		template <typename P, typename EntityID, typename T>
		Verdict AskPolicy(P& policy, const EntityID entity, T& copy)
		{
			if constexpr (AcceptsComponent<P, EntityID, T>)
			{
				return policy.Accept(entity, copy);
			}
			else
			{
				return Verdict::Keep();
			}
		}
	}

	/* A capture policy may have
	 *
	 *     Verdict Accept(EntityID entity, T& copy);   // a template, or for chosen types only
	 *
	 * which is called with a copy of each plain component of a type it accepts (it is
	 * a template, or overloaded, or constrained), before it is written. It may look
	 * at the copy (is every handle in it nameable?) and it may remember it (which
	 * handles are used), and says Keep or Drop(why). A type it has no Accept for is written as it is,
	 * and it is not called for tags, which are only an entity id. */
	template <typename Schema, typename Policy = NoPolicy>
	[[nodiscard]] CaptureResult<typename Schema::EcsType> Capture(typename Schema::EcsType& ecs, const Reporter& reporter, Policy&& policy = {})
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

					const Verdict verdict = detail::AskPolicy(policy, entity, copy);
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
}
