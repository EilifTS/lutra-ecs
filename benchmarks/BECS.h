#pragma once
#include <benchmark/benchmark.h>

#include <lutra-ecs/ECSManager.h>

#include <vector>

namespace becs
{
	using EntityID = lcs::Handle<uint32_t, 16>;

	struct Position
	{
		static constexpr lcs::ComponentType component_type = lcs::ComponentType::Component;
		int x, y;
	};
	struct Velocity
	{
		static constexpr lcs::ComponentType component_type = lcs::ComponentType::Component;
		int x, y;
	};
	struct Player
	{
		static constexpr lcs::ComponentType component_type = lcs::ComponentType::Component;
		bool is_happy;
		bool is_hungry;
		bool is_here;
		bool is_there;
	};

	using ECS = lcs::ECSManager<EntityID, Position, Velocity, Player>;

	inline EntityID CreatePlayer(ECS& ecs, int x, int y)
	{
		auto entity = ecs.CreateEntity();
		ecs.AddComponent<Position>(entity, { x, y });
		ecs.AddComponent<Velocity>(entity, { 0, 1 });
		ecs.AddComponent<Player>(entity, { true, true, true, true });
		return entity;
	}
}

static void BenchmarkECS(benchmark::State& state)
{
	const uint32_t player_count = state.range(0);

	becs::ECS ecs{};

	std::vector<becs::EntityID> players;
	std::vector<becs::Position> positions(player_count);
	std::vector<becs::Velocity> velocities(player_count);

	for (uint32_t i = 0; i < player_count; i++)
	{
		players.push_back(becs::CreatePlayer(ecs, 1, i));
		positions[i] = { 1, (int)i };
		velocities[i] = { 0, 1 };
	}

	for (auto _ : state)
	{
		for (auto [e, p] : ecs.CView<becs::Position>())
		{
			if (ecs.HasComponent<becs::Velocity>(e))
			{
				becs::Velocity& v = ecs.GetComponent<becs::Velocity>(e);
				p.x += v.x;
				p.y += v.y;
			}
		}
	}

	ecs.Clear();
}

static void BenchmarkECSSTD(benchmark::State& state)
{
	const uint32_t player_count = state.range(0);

	becs::ECS ecs{};

	std::vector<uint32_t> entity_id_to_position_idx;
	std::vector<uint32_t> entity_id_to_velocity_idx;
	std::vector<uint32_t> position_idx_to_entity_id;
	std::vector<uint32_t> velocity_idx_to_entity_id;
	std::vector<becs::Position> positions(player_count);
	std::vector<becs::Velocity> velocities(player_count);

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
				becs::Position& p = positions[entity_id_to_position_idx[entity_id]];
				becs::Velocity& v = velocities[entity_id_to_velocity_idx[entity_id]];
				p.x += v.x;
				p.y += v.y;
			}
		}
	}
	ecs.Clear();
}