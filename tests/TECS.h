#include <gtest/gtest.h>
#include <lutra-ecs/ECSManager.h>

namespace TECS
{
	struct Position
	{
		static constexpr ef::ComponentType component_type = ef::ComponentType::Component;
		int x, y;
	};
	struct Velocity
	{
		static constexpr ef::ComponentType component_type = ef::ComponentType::Component;
		int x, y;
	};
	struct Player
	{
		static constexpr ef::ComponentType component_type = ef::ComponentType::Component;
		bool is_happy;
		bool is_hungry;
		bool is_here;
		bool is_there;
	};
	struct Enemy
	{
		static constexpr ef::ComponentType component_type = ef::ComponentType::Component;
		int strength;
	};
	struct Weapon
	{
		static constexpr ef::ComponentType component_type = ef::ComponentType::Component;
		int length;
	};
	struct IsWet
	{
		static constexpr ef::ComponentType component_type = ef::ComponentType::Tag;
	};

	using ECS = ef::ECSManager<Position, Velocity, Player, Enemy, Weapon, IsWet>;

	inline ef::Entity CreatePlayer(ECS& ecs, int x, int y)
	{
		auto entity = ecs.CreateEntity();
		ecs.AddComponent<Position>(entity, { x, y });
		ecs.AddComponent<Velocity>(entity, { 0, 1 });
		ecs.AddComponent<Player>(entity, { true, true, true, true });
		return entity;
	}

	inline ef::Entity CreateEnemy(ECS& ecs, int x, int y)
	{
		auto entity = ecs.CreateEntity();
		ecs.AddComponent<Position>(entity, { x, y });
		ecs.AddComponent<Velocity>(entity, { 0, 0 });
		ecs.AddComponent<Enemy>(entity, { 10 });
		return entity;
	}

	static void UpdateSystem(ECS& ecs)
	{
		for (auto e : ecs.GetAllEntitiesWithComponent<Position>())
		{
			if (ecs.HasComponent<Velocity>(e))
			{
				Position& p = ecs.GetComponent<Position>(e);
				Velocity& v = ecs.GetComponent<Velocity>(e);
				p.x += v.x;
				p.y += v.y;
			}
		}
	}
}

TEST(ECS, TestCreation1)
{
	TECS::ECS ecs{ };

	constexpr int player_pos_x{ 2 };
	constexpr int player_pos_y{ 1 };

	ef::Entity e = TECS::CreatePlayer(ecs, player_pos_x, player_pos_y);
	ASSERT_TRUE(ecs.HasComponent<TECS::Position>(e));
	ASSERT_TRUE(ecs.HasComponent<TECS::Velocity>(e));
	ASSERT_TRUE(ecs.HasComponent<TECS::Player>(e));
	ASSERT_TRUE(!ecs.HasComponent<TECS::Enemy>(e));
	ASSERT_TRUE(!ecs.HasComponent<TECS::Weapon>(e));

	ASSERT_TRUE(ecs.GetComponent<TECS::Position>(e).x == player_pos_x);
	ASSERT_TRUE(ecs.GetComponent<TECS::Position>(e).y == player_pos_y);

	ecs.Clear();
}

TEST(ECS, TestCreation2)
{
	TECS::ECS ecs{ };

	constexpr int player_pos_x{ 2 };
	constexpr int player_pos_y{ 1 };
	constexpr int enemy_pos_x{ 1 };
	constexpr int enemy_pos_y{ 1 };

	ef::Entity e1 = TECS::CreatePlayer(ecs, player_pos_x, player_pos_y);
	ef::Entity e2 = TECS::CreateEnemy(ecs, enemy_pos_x, enemy_pos_y);

	ASSERT_TRUE(ecs.HasComponent<TECS::Position>(e1));
	ASSERT_TRUE(ecs.HasComponent<TECS::Velocity>(e1));
	ASSERT_TRUE(ecs.HasComponent<TECS::Player>(e1));
	ASSERT_TRUE(!ecs.HasComponent<TECS::Enemy>(e1));
	ASSERT_TRUE(!ecs.HasComponent<TECS::Weapon>(e1));

	ASSERT_TRUE(ecs.HasComponent<TECS::Position>(e2));
	ASSERT_TRUE(ecs.HasComponent<TECS::Velocity>(e2));
	ASSERT_TRUE(!ecs.HasComponent<TECS::Player>(e2));
	ASSERT_TRUE(ecs.HasComponent<TECS::Enemy>(e2));
	ASSERT_TRUE(!ecs.HasComponent<TECS::Weapon>(e2));

	ASSERT_TRUE(ecs.GetComponent<TECS::Position>(e1).x == player_pos_x);
	ASSERT_TRUE(ecs.GetComponent<TECS::Position>(e1).y == player_pos_y);
	ASSERT_TRUE(ecs.GetComponent<TECS::Position>(e2).x == enemy_pos_x);
	ASSERT_TRUE(ecs.GetComponent<TECS::Position>(e2).y == enemy_pos_y);

	ecs.Clear();
}

TEST(ECS, TestCreationDestruction)
{
	TECS::ECS ecs{ };

	constexpr int player_pos_x{ 2 };
	constexpr int player_pos_y{ 1 };
	constexpr int enemy_pos_x1{ 3 };
	constexpr int enemy_pos_y1{ 2 };
	constexpr int enemy_pos_x2{ 5 };
	constexpr int enemy_pos_y2{ 6 };

	ef::Entity e1 = TECS::CreatePlayer(ecs, player_pos_x, player_pos_y);
	ef::Entity e2 = TECS::CreateEnemy(ecs, enemy_pos_x1, enemy_pos_y1);
	ecs.DestroyEntity(e2);
	ef::Entity e3 = TECS::CreateEnemy(ecs, enemy_pos_x2, enemy_pos_y2);

	ASSERT_TRUE(ecs.HasComponent<TECS::Position>(e1));
	ASSERT_TRUE(ecs.HasComponent<TECS::Velocity>(e1));
	ASSERT_TRUE(ecs.HasComponent<TECS::Player>(e1));
	ASSERT_TRUE(!ecs.HasComponent<TECS::Enemy>(e1));
	ASSERT_TRUE(!ecs.HasComponent<TECS::Weapon>(e1));

	ASSERT_TRUE(ecs.HasComponent<TECS::Position>(e3));
	ASSERT_TRUE(ecs.HasComponent<TECS::Velocity>(e3));
	ASSERT_TRUE(!ecs.HasComponent<TECS::Player>(e3));
	ASSERT_TRUE(ecs.HasComponent<TECS::Enemy>(e3));
	ASSERT_TRUE(!ecs.HasComponent<TECS::Weapon>(e3));

	ASSERT_TRUE(ecs.GetComponent<TECS::Position>(e1).x == player_pos_x);
	ASSERT_TRUE(ecs.GetComponent<TECS::Position>(e1).y == player_pos_y);
	ASSERT_TRUE(ecs.GetComponent<TECS::Position>(e3).x == enemy_pos_x2);
	ASSERT_TRUE(ecs.GetComponent<TECS::Position>(e3).y == enemy_pos_y2);

	ecs.Clear();
}

TEST(ECS, TestComponentRemoval)
{
	TECS::ECS ecs{ };

	constexpr int player_pos_x{ 2 };
	constexpr int player_pos_y{ 1 };

	ef::Entity e = TECS::CreatePlayer(ecs, player_pos_x, player_pos_y);
	ecs.RemoveComponent<TECS::Velocity>(e);
	ASSERT_TRUE(ecs.HasComponent<TECS::Position>(e));
	ASSERT_TRUE(!ecs.HasComponent<TECS::Velocity>(e));
	ASSERT_TRUE(ecs.HasComponent<TECS::Player>(e));
	ASSERT_TRUE(!ecs.HasComponent<TECS::Enemy>(e));
	ASSERT_TRUE(!ecs.HasComponent<TECS::Weapon>(e));

	ASSERT_TRUE(ecs.GetComponent<TECS::Position>(e).x == player_pos_x);
	ASSERT_TRUE(ecs.GetComponent<TECS::Position>(e).y == player_pos_y);

	ecs.Clear();
}

TEST(ECS, TestSparseIteration)
{
	TECS::ECS ecs{ };

	constexpr int player_pos_x{ 2 };
	constexpr int player_pos_y{ 1 };

	std::vector<ef::Entity> entities;
	for (int i = 0; i < 100; i++)
	{
		entities.push_back(TECS::CreatePlayer(ecs, player_pos_x + i, player_pos_y));
	}
	for (ef::Entity e : ecs.GetAllEntitiesWithComponent<TECS::Position>())
	{
		ecs.GetComponent<TECS::Position>(e).x++;
	}

	for (int i = 0; i < 100; i++)
	{
		ASSERT_TRUE(ecs.GetComponent<TECS::Position>(entities[i]).x == player_pos_x + i + 1);
	}
	ecs.Clear();
}

TEST(ECS, TestSparseSystem)
{
	TECS::ECS ecs{ };

	constexpr int player_pos_x{ 2 };
	constexpr int player_pos_y{ 1 };

	auto e = TECS::CreatePlayer(ecs, player_pos_x, player_pos_y);
	TECS::UpdateSystem(ecs);

	ASSERT_TRUE(ecs.GetComponent<TECS::Position>(e).x == player_pos_x);
	ASSERT_TRUE(ecs.GetComponent<TECS::Position>(e).y == player_pos_y + 1);
	ecs.Clear();
}

TEST(ECS, TestTagCreation)
{
	TECS::ECS ecs{ };

	ef::Entity e1 = ecs.CreateEntity();
	ef::Entity e2 = ecs.CreateEntity();
	ef::Entity e3 = ecs.CreateEntity();
	ef::Entity e4 = ecs.CreateEntity();

	ASSERT_TRUE(!ecs.HasTag<TECS::IsWet>(e1));
	ASSERT_TRUE(!ecs.HasTag<TECS::IsWet>(e2));
	ASSERT_TRUE(!ecs.HasTag<TECS::IsWet>(e3));
	ASSERT_TRUE(!ecs.HasTag<TECS::IsWet>(e4));

	ecs.AddTag<TECS::IsWet>(e2);

	ASSERT_TRUE(!ecs.HasTag<TECS::IsWet>(e1));
	ASSERT_TRUE(ecs.HasTag<TECS::IsWet>(e2));
	ASSERT_TRUE(!ecs.HasTag<TECS::IsWet>(e3));
	ASSERT_TRUE(!ecs.HasTag<TECS::IsWet>(e4));

	ecs.AddTag<TECS::IsWet>(e3);

	ASSERT_TRUE(!ecs.HasTag<TECS::IsWet>(e1));
	ASSERT_TRUE(ecs.HasTag<TECS::IsWet>(e2));
	ASSERT_TRUE(ecs.HasTag<TECS::IsWet>(e3));
	ASSERT_TRUE(!ecs.HasTag<TECS::IsWet>(e4));

	ecs.RemoveTag<TECS::IsWet>(e2);

	ASSERT_TRUE(!ecs.HasTag<TECS::IsWet>(e1));
	ASSERT_TRUE(!ecs.HasTag<TECS::IsWet>(e2));
	ASSERT_TRUE(ecs.HasTag<TECS::IsWet>(e3));
	ASSERT_TRUE(!ecs.HasTag<TECS::IsWet>(e4));

	ecs.AddTag<TECS::IsWet>(e4);

	std::vector<ef::Entity> tagged_entities{};
	for (auto e : ecs.GetAllEntitiesWithTag<TECS::IsWet>())
	{
		tagged_entities.push_back(e);
	}
	ASSERT_TRUE(tagged_entities[0].GetID() == e3.GetID());
	ASSERT_TRUE(tagged_entities[1].GetID() == e4.GetID());

	ecs.Clear();
}

TEST(ECS, TestDualManager)
{
	TECS::ECS ecs1{ };
	TECS::ECS ecs2{ };

	ef::Entity e1_ecs1 = ecs1.CreateEntity();
	ef::Entity e1_ecs2 = ecs2.CreateEntity();
	ef::Entity e2_ecs2 = ecs2.CreateEntity();
	ef::Entity e2_ecs1 = ecs1.CreateEntity();

	ecs1.AddComponent<TECS::Position>(e1_ecs1, { 1, 1 });
	ecs1.AddComponent<TECS::Position>(e2_ecs1, { 2, 1 });
	ecs2.AddComponent<TECS::Position>(e1_ecs2, { 1, 2 });
	ecs2.AddComponent<TECS::Position>(e2_ecs2, { 2, 2 });

	ASSERT_TRUE(ecs1.GetComponent<TECS::Position>(e1_ecs1).x == 1 && ecs1.GetComponent<TECS::Position>(e1_ecs1).y == 1);
	ASSERT_TRUE(ecs1.GetComponent<TECS::Position>(e2_ecs1).x == 2 && ecs1.GetComponent<TECS::Position>(e2_ecs1).y == 1);
	ASSERT_TRUE(ecs2.GetComponent<TECS::Position>(e1_ecs2).x == 1 && ecs2.GetComponent<TECS::Position>(e1_ecs2).y == 2);
	ASSERT_TRUE(ecs2.GetComponent<TECS::Position>(e2_ecs2).x == 2 && ecs2.GetComponent<TECS::Position>(e2_ecs2).y == 2);

	ecs2.RemoveComponent<TECS::Position>(e1_ecs2);

	ASSERT_TRUE(ecs1.GetComponent<TECS::Position>(e1_ecs1).x == 1 && ecs1.GetComponent<TECS::Position>(e1_ecs1).y == 1);
	ASSERT_TRUE(ecs1.GetComponent<TECS::Position>(e2_ecs1).x == 2 && ecs1.GetComponent<TECS::Position>(e2_ecs1).y == 1);
	ASSERT_TRUE(!ecs2.HasComponent<TECS::Position>(e1_ecs2));
	ASSERT_TRUE(ecs2.GetComponent<TECS::Position>(e2_ecs2).x == 2 && ecs2.GetComponent<TECS::Position>(e2_ecs2).y == 2);
}