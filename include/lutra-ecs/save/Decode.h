#pragma once
#include <concepts>
#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

#include <lutra-ecs/save/Container.h>
#include <lutra-ecs/save/Report.h>
#include <lutra-ecs/save/Schema.h>

/**
 *  A Container into components the game can use, and those into an ECS.
 *
 *  Decode checks every component and makes the save fit the game as it is now,
 *  without touching the ECS or anything else of the game's. Everything that can
 *  be wrong with the file has been found by the time a Decoded exists, so
 *  applying it (Restore) cannot fail halfway and cost the world its entities.
 *  That order -- decode, then clear the world, then restore -- is the caller's.
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

		/* Components left out because the policy refused them, or because they
		 * cannot work without an entity that was left out. Each was reported. */
		size_t dropped_components{ 0 };

		template <typename T> Entries<EntityID, T>& Of() { return std::get<Entries<EntityID, T>>(components); }
		template <typename T> const Entries<EntityID, T>& Of() const { return std::get<Entries<EntityID, T>>(components); }
	};

	namespace detail
	{
		template <typename P, typename EntityID, typename T>
		concept DecidesOnComponent = requires (P& policy, EntityID entity, T& component) { { policy.Component(entity, component) } -> std::same_as<Verdict>; };

		template <typename Schema, typename Policy>
		class Decoder
		{
		public:
			using EntityID = typename Schema::EcsType::EntityID;

			Decoder(const Container<EntityID>& data, Policy& policy, const Reporter& reporter, Decoded<Schema>& out)
			: data(data), policy(policy), reporter(reporter), out(out)
			{
				for (const EntityID entity : data.entities)
				{
					listed.insert(entity.handle);
				}
			}

			[[nodiscard]] bool Lists(const EntityID entity) const { return listed.contains(entity.handle); }
			[[nodiscard]] const std::optional<Error>& Failure() const { return failure; }

			template <typename T>
			void operator()(std::type_identity<T>)
			{
				if (failure) return;

				const ComponentChunk<EntityID>* chunk = FindChunk(data, SaveNameOf<T>());
				if (chunk == nullptr) return;

				for (size_t i = 0; i < chunk->Count() && !failure; i++)
				{
					const EntityID entity = chunk->entities[i];

					if constexpr (T::component_type == ComponentType::Tag)
					{
						out.template Of<T>().emplace_back(entity, T{});
					}
					else
					{
						decodeComponent<T>(*chunk, i, entity);
					}
				}
			}

		private:
			template <typename T>
			void fail(const EntityID entity, const std::string& why)
			{
				failure = MakeError(ErrorKind::Malformed, std::string(SaveNameOf<T>()) + " of entity " + std::to_string(entity.GetIndex()) + ": " + why);
			}

			template <typename T>
			void drop(const EntityID entity, const std::string& why)
			{
				Report(reporter, Severity::Error, SaveNameOf<T>(), "entity " + std::to_string(entity.GetIndex()) + " loaded without it: " + why);
				out.dropped_components++;
			}

			template <typename T>
			void decodeComponent(const ComponentChunk<EntityID>& chunk, const size_t index, const EntityID entity)
			{
				if (!SaveBytesAreValid<T>(chunk.Component(index)))
				{
					fail<T>(entity, "its values cannot be used (a number that is not a number, a value out of range, or a name it needs and does not have)");
					return;
				}

				T component = ReadComponent<T>(chunk, index);

				if constexpr (DecidesOnComponent<Policy, EntityID, T>)
				{
					const Verdict verdict = policy.Component(entity, component);
					if (verdict.kind == Verdict::Kind::Fail)
					{
						fail<T>(entity, verdict.why);
						return;
					}
					if (verdict.kind == Verdict::Kind::Drop)
					{
						drop<T>(entity, verdict.why);
						return;
					}
				}

				out.template Of<T>().emplace_back(entity, component);
			}

			const Container<EntityID>& data;
			Policy& policy;
			const Reporter& reporter;
			Decoded<Schema>& out;

			std::unordered_set<uint32_t> listed{};
			std::optional<Error> failure{};
		};
	}

	/* A component of an entity the save does not list is an error, as is anything
	 * the component or the policy fail. A chunk of a component this build does not know
	 * (see Container::skipped_component_names) is reported and skipped.
	 *
	 * A decode policy may have
	 *
	 *     Verdict Component(EntityID entity, T& component);   // a template, or for chosen types only
	 *
	 * which is called with each plain component of a type it accepts (it is a
	 * template, or overloaded, or constrained), after the component has been read and
	 * found valid. It may change the component (a handle into a library, replaced
	 * with the library's own) and says Keep, Drop(why) -- the component is left out
	 * and reported -- or Fail(why): the file cannot be used, and the whole decode is
	 * an error. A type it has no Component for is decoded as it is, and it is not
	 * called for tags.
	 *
	 * Entity ids a component holds are values like any other and are not looked at
	 * (see the note on Container). */
	template <typename Schema, typename Policy = NoPolicy>
	[[nodiscard]] Result<Decoded<Schema>> Decode(const Container<typename Schema::EcsType::EntityID>& data, const Reporter& reporter, Policy&& policy = {})
	{
		using EntityID = typename Schema::EcsType::EntityID;

		for (const std::string& name : data.skipped_component_names)
		{
			Report(reporter, Severity::Warning, "save", "a component named '" + name + "' is not one this build knows, skipped");
		}

		Decoded<Schema> out;
		out.entities = data.entities;

		detail::Decoder<Schema, std::remove_reference_t<Policy>> decoder(data, policy, reporter, out);

		for (const ComponentChunk<EntityID>& chunk : data.chunks)
		{
			for (const EntityID entity : chunk.entities)
			{
				if (!decoder.Lists(entity))
				{
					return std::unexpected(MakeError(ErrorKind::Malformed, "a component belongs to entity " + std::to_string(entity.GetIndex()) + ", which the save does not list"));
				}
			}
		}

		Schema::ForEachSaved([&decoder](auto type) { decoder(type); });
		if (decoder.Failure())
		{
			return std::unexpected(*decoder.Failure());
		}

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

	/* Restore, deferring every component as it is. */
	template <typename Schema>
	void Restore(typename Schema::EcsType& ecs, const Decoded<Schema>& decoded)
	{
		Restore(ecs, decoded, [&ecs](const auto entity, const auto& component) { ecs.AddDeferred(entity, component); });
	}
}
