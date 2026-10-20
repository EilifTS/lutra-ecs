#pragma once
#include <cstddef>
#include <optional>
#include <span>
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

	/* A policy is any type with
	 *
	 *     template <typename T> Verdict Component(EntityID entity, T& component);
	 *
	 * called with each plain component after it has been read and found valid, and
	 * before its entity ids are checked. It may change the component (a handle into
	 * a library, replaced with the library's own) and says Keep, Drop(why) -- the
	 * component is left out and reported -- or Fail(why): the file cannot be used,
	 * and the whole decode is an error. It is not called for tags.
	 *
	 * Entity ids are the library's: an id a component holds that the save does not
	 * list is cleared or drops the component, as the component's traits say. */
	struct KeepEverything
	{
		template <typename EntityID, typename T>
		Verdict Component(EntityID, T&) { return Verdict::Keep(); }
	};

	namespace detail
	{
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

				const ComponentChunk<EntityID>* chunk = FindChunk(data, static_cast<uint32_t>(Schema::template TraitsOf<T>::id));
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
				failure = MakeError(ErrorKind::Malformed, std::string(Schema::template TraitsOf<T>::name) + " of entity " + std::to_string(entity.GetIndex()) + ": " + why);
			}

			template <typename T>
			void drop(const EntityID entity, const std::string& why)
			{
				Report(reporter, Severity::Error, Schema::template TraitsOf<T>::name, "entity " + std::to_string(entity.GetIndex()) + " loaded without it: " + why);
				out.dropped_components++;
			}

			template <typename T>
			void decodeComponent(const ComponentChunk<EntityID>& chunk, const size_t index, const EntityID entity)
			{
				using Traits = typename Schema::template TraitsOf<T>;

				if (!Traits::Valid(chunk.Component(index)))
				{
					fail<T>(entity, "its values cannot be used (a number that is not a number, a value out of range, or a name it needs and does not have)");
					return;
				}

				T component = ReadComponent<T>(chunk, index);

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

				bool dangling_drop = false;
				Traits::ForEachEntityRef(component, [&](EntityID& ref) {
					/* An id that was never set names nothing and stays that way. */
					if (!ref.IsValid() || listed.contains(ref.handle))
					{
						return;
					}

					if (Traits::dangling == OnDanglingRef::ClearTheRef)
					{
						ref = EntityID{};
					}
					else
					{
						dangling_drop = true;
					}
				});
				if (dangling_drop)
				{
					drop<T>(entity, "the entity it refers to is not in the save");
					return;
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
	 * the traits or the policy fail. A chunk of a component this build does not know
	 * (see Container::skipped_component_ids) is reported and skipped. */
	template <typename Schema, typename Policy>
	[[nodiscard]] Result<Decoded<Schema>> Decode(const Container<typename Schema::EcsType::EntityID>& data, Policy& policy, const Reporter& reporter)
	{
		using EntityID = typename Schema::EcsType::EntityID;

		for (const uint32_t id : data.skipped_component_ids)
		{
			Report(reporter, Severity::Warning, "save", "a component with id " + std::to_string(id) + " is not one this build knows, skipped");
		}

		Decoded<Schema> out;
		out.entities = data.entities;

		detail::Decoder<Schema, Policy> decoder(data, policy, reporter, out);

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

	template <typename Schema>
	[[nodiscard]] Result<Decoded<Schema>> Decode(const Container<typename Schema::EcsType::EntityID>& data, const Reporter& reporter)
	{
		KeepEverything policy;
		return Decode<Schema>(data, policy, reporter);
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
