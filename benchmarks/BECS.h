#pragma once
#include <benchmark/benchmark.h>

#include <lutra-ecs/ECSManager.h>

#include <vector>

static constexpr uint32_t entity_count = 1024 * 1024;

namespace becs
{
	using EntityID = lcs::Handle<uint32_t, 8>;

	template <lcs::ComponentType ct>
	struct ECSSetup
	{
		struct Position
		{
			static constexpr lcs::ComponentType component_type = ct;
			int x, y;
		};
		struct Velocity
		{
			static constexpr lcs::ComponentType component_type = ct;
			int x, y;
		};
		struct Player
		{
			static constexpr lcs::ComponentType component_type = ct;
			bool is_happy;
			bool is_hungry;
			bool is_here;
			bool is_there;
		};

		using ECS = lcs::ECSManager<EntityID, Position, Velocity, Player>;

		inline static EntityID CreatePlayer(ECS& ecs, int x, int y)
		{
			auto entity = ecs.CreateEntity();
			ecs.template AddComponent<Position>(entity, { x, y });
			ecs.template AddComponent<Velocity>(entity, { 0, 1 });
			ecs.template AddComponent<Player>(entity, { true, true, true, true });
			return entity;
		}
	};

	void Shuffle(std::vector<EntityID>& list)
	{
		const uint32_t size = uint32_t(list.size());

		for (uint32_t i = 0; i < size - 1; i++)
		{
			const uint32_t back_index = size - i - 1;
			const uint32_t rnd_nr = rand() % (back_index + 1);

			if (rnd_nr != back_index)
			{
				std::iter_swap(list.begin() + rnd_nr, list.begin() + back_index);
			}
		}
	}

	std::vector<EntityID> SelectNRandomEntriesFrom(const std::vector<EntityID>& list, uint32_t N)
	{
		std::vector<EntityID> list_shuffle = list;
		Shuffle(list_shuffle);

		std::vector<EntityID> out(N);

		for (uint32_t i = 0; i < N; i++)
		{
			out[i] = list_shuffle[i];
		}
		return out;
	}

	void Sort(std::vector<EntityID>& list)
	{
		std::sort(std::begin(list), std::end(list), [](EntityID e1, EntityID e2) -> bool { return e1.GetIndex() < e2.GetIndex(); });
	}
}

template <lcs::ComponentType ct>
static void BenchmarkECSIteration(benchmark::State& state, bool use_random_ordering)
{
	using Setup = becs::ECSSetup<ct>;
	using ECS = typename Setup::ECS;
	using EntityID = becs::EntityID;
	using Position = typename Setup::Position;
	using Velocity = typename Setup::Velocity;

	const float component_fraction = float(state.range(0)) / 100.0f;
	const uint32_t component_count = uint32_t(float(entity_count) * component_fraction);

	ECS ecs{};

	std::vector<EntityID> entities(entity_count);
	for (uint32_t i = 0; i < entity_count; i++)
	{
		entities[i] = ecs.CreateEntity();
	}
	std::vector<EntityID> entities_with_component = becs::SelectNRandomEntriesFrom(entities, component_count);

	if (!use_random_ordering)
	{
		becs::Sort(entities_with_component);
	}

	for (uint32_t i = 0; i < component_count; i++)
	{
		const EntityID e = entities_with_component[i];
		ecs.template AddComponent<Position>(e, { 1, (int)i });
		ecs.template AddComponent<Velocity>(e, { 0, 1 });
	}

	if (use_random_ordering)
	{
		becs::Shuffle(entities_with_component);
	}

	for (uint32_t i = 0; i < component_count; i++)
	{
		const EntityID e = entities_with_component[i];
		ecs.template AddComponent<Velocity>(e, { 0, 1 });
	}

	for (auto _ : state)
	{
		for (auto [e, p] : ecs.template CView<Position>())
		{
			if (ecs.template HasComponent<Velocity>(e))
			{
				Velocity& v = ecs.template GetComponent<Velocity>(e);
				p.x += v.x;
				p.y += v.y;
			}
		}
	}

	ecs.Clear();
}

static void ECSIterationNormal(benchmark::State& state) { BenchmarkECSIteration<lcs::ComponentType::Component>(state, false); }
static void ECSIterationChunked(benchmark::State& state) { BenchmarkECSIteration<lcs::ComponentType::ComponentChunked>(state, false); }

static void ECSIterationRndNormal(benchmark::State& state) { BenchmarkECSIteration<lcs::ComponentType::Component>(state, true); }
static void ECSIterationRndChunked(benchmark::State& state) { BenchmarkECSIteration<lcs::ComponentType::ComponentChunked>(state, true); }

static void ECSIterationSTD(benchmark::State& state)
{
	using Setup = becs::ECSSetup<lcs::ComponentType::Component>;

	const uint32_t player_count = entity_count;

	std::vector<uint32_t> entity_id_to_position_idx;
	std::vector<uint32_t> entity_id_to_velocity_idx;
	std::vector<uint32_t> position_idx_to_entity_id;
	std::vector<uint32_t> velocity_idx_to_entity_id;
	std::vector<Setup::Position> positions(player_count);
	std::vector<Setup::Velocity> velocities(player_count);

	for (uint32_t i = 0; i < player_count; i++)
	{
		entity_id_to_position_idx.push_back(i);
		entity_id_to_velocity_idx.push_back(i);
		position_idx_to_entity_id.push_back(i);
		velocity_idx_to_entity_id.push_back(i);
		positions[i] = { 1, (int)i };
		velocities[i] = { 0, 1 };
	}

	for (auto _ : state)
	{
		/* Mimic ECS, but with pure std funcs */
		for (uint32_t entity_id : position_idx_to_entity_id)
		{
			/* Has velocity */
			if (entity_id_to_velocity_idx[entity_id] != ~(uint32_t(0)))
			{
				Setup::Position& p = positions[entity_id_to_position_idx[entity_id]];
				Setup::Velocity& v = velocities[entity_id_to_velocity_idx[entity_id]];
				p.x += v.x;
				p.y += v.y;
			}
		}
	}
}