#pragma once
#include <gtest/gtest.h>
#include <lutra-ecs/ECSManager.h>

namespace TECS
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
	struct Enemy
	{
		static constexpr lcs::ComponentType component_type = lcs::ComponentType::Component;
		int strength;
	};
	struct Weapon
	{
		static constexpr lcs::ComponentType component_type = lcs::ComponentType::Component;
		int length;
	};
	struct IsWet
	{
		static constexpr lcs::ComponentType component_type = lcs::ComponentType::Tag;
	};

	using ECS = lcs::ECSManager<EntityID, Position, Velocity, Player, Enemy, Weapon, IsWet>;

	inline EntityID CreatePlayer(ECS& ecs, int x, int y)
	{
		auto entity = ecs.CreateEntity();
		ecs.AddComponent<Position>(entity, { x, y });
		ecs.AddComponent<Velocity>(entity, { 0, 1 });
		ecs.AddComponent<Player>(entity, { true, true, true, true });
		return entity;
	}

	inline EntityID CreateEnemy(ECS& ecs, int x, int y)
	{
		auto entity = ecs.CreateEntity();
		ecs.AddComponent<Position>(entity, { x, y });
		ecs.AddComponent<Velocity>(entity, { 0, 0 });
		ecs.AddComponent<Enemy>(entity, { 10 });
		return entity;
	}

	static void UpdateSystem(ECS& ecs)
	{
		for (auto [e, p] : ecs.CView<Position>())
		{
			if (ecs.HasComponent<Velocity>(e))
			{
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

	TECS::EntityID e = TECS::CreatePlayer(ecs, player_pos_x, player_pos_y);
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

	TECS::EntityID e1 = TECS::CreatePlayer(ecs, player_pos_x, player_pos_y);
	TECS::EntityID e2 = TECS::CreateEnemy(ecs, enemy_pos_x, enemy_pos_y);

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

	TECS::EntityID e1 = TECS::CreatePlayer(ecs, player_pos_x, player_pos_y);
	TECS::EntityID e2 = TECS::CreateEnemy(ecs, enemy_pos_x1, enemy_pos_y1);
	ecs.DestroyEntity(e2);
	TECS::EntityID e3 = TECS::CreateEnemy(ecs, enemy_pos_x2, enemy_pos_y2);

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

	TECS::EntityID e = TECS::CreatePlayer(ecs, player_pos_x, player_pos_y);
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

	std::vector<TECS::EntityID> entities;
	for (int i = 0; i < 100; i++)
	{
		entities.push_back(TECS::CreatePlayer(ecs, player_pos_x + i, player_pos_y));
	}
	for (auto [e, p] : ecs.CView<TECS::Position>())
	{
		p.x++;
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

TEST(ECS, TestGetComponentCount)
{
	TECS::ECS ecs{ };

	ASSERT_TRUE(ecs.GetComponentCount<TECS::Position>() == 0);
	ASSERT_TRUE(ecs.GetComponentCount<TECS::Enemy>() == 0);

	TECS::EntityID e1 = TECS::CreatePlayer(ecs, 2, 1);
	ASSERT_TRUE(ecs.GetComponentCount<TECS::Position>() == 1);
	ASSERT_TRUE(ecs.GetComponentCount<TECS::Velocity>() == 1);
	ASSERT_TRUE(ecs.GetComponentCount<TECS::Player>() == 1);
	ASSERT_TRUE(ecs.GetComponentCount<TECS::Enemy>() == 0);

	TECS::EntityID e2 = TECS::CreateEnemy(ecs, 1, 1);
	ASSERT_TRUE(ecs.GetComponentCount<TECS::Position>() == 2);
	ASSERT_TRUE(ecs.GetComponentCount<TECS::Velocity>() == 2);
	ASSERT_TRUE(ecs.GetComponentCount<TECS::Player>() == 1);
	ASSERT_TRUE(ecs.GetComponentCount<TECS::Enemy>() == 1);

	ecs.RemoveComponent<TECS::Velocity>(e1);
	ASSERT_TRUE(ecs.GetComponentCount<TECS::Velocity>() == 1);

	ecs.DestroyEntity(e2);
	ASSERT_TRUE(ecs.GetComponentCount<TECS::Position>() == 1);
	ASSERT_TRUE(ecs.GetComponentCount<TECS::Velocity>() == 0);
	ASSERT_TRUE(ecs.GetComponentCount<TECS::Enemy>() == 0);

	ecs.Clear();
	ASSERT_TRUE(ecs.GetComponentCount<TECS::Position>() == 0);
}

TEST(ECS, TestGetComponentData)
{
	TECS::ECS ecs{ };

	TECS::EntityID e1 = TECS::CreatePlayer(ecs, 2, 1);
	TECS::EntityID e2 = TECS::CreateEnemy(ecs, 1, 1);

	std::span<TECS::Position> data = ecs.GetComponentData<TECS::Position>();
	ASSERT_TRUE(data.data() != nullptr);
	ASSERT_TRUE(data.size() == 2);
	ASSERT_TRUE(ecs.GetComponentCount<TECS::Position>() == 2);

	/* Data() points at the same dense storage backing Get() */
	ASSERT_TRUE(&data[0] == &ecs.GetComponent<TECS::Position>(e1));
	ASSERT_TRUE(&data[1] == &ecs.GetComponent<TECS::Position>(e2));
	ASSERT_TRUE(data[0].x == 2 && data[0].y == 1);
	ASSERT_TRUE(data[1].x == 1 && data[1].y == 1);

	ecs.Clear();
}

TEST(ECS, TestClearComponent)
{
	TECS::ECS ecs{ };

	TECS::EntityID e1 = TECS::CreatePlayer(ecs, 2, 1);
	TECS::EntityID e2 = TECS::CreateEnemy(ecs, 1, 1);
	ecs.AddTag<TECS::IsWet>(e1);

	ASSERT_TRUE(ecs.GetComponentCount<TECS::Position>() == 2);
	ASSERT_TRUE(ecs.GetComponentCount<TECS::Velocity>() == 2);

	ecs.ClearComponent<TECS::Position>();

	ASSERT_TRUE(ecs.GetComponentCount<TECS::Position>() == 0);
	ASSERT_TRUE(!ecs.HasComponent<TECS::Position>(e1));
	ASSERT_TRUE(!ecs.HasComponent<TECS::Position>(e2));

	/* Unrelated components/tags are untouched */
	ASSERT_TRUE(ecs.GetComponentCount<TECS::Velocity>() == 2);
	ASSERT_TRUE(ecs.HasComponent<TECS::Velocity>(e1));
	ASSERT_TRUE(ecs.HasTag<TECS::IsWet>(e1));

	/* Capacity for existing entities is preserved, so re-adding still works */
	ecs.AddComponent<TECS::Position>(e1, { 5, 5 });
	ASSERT_TRUE(ecs.HasComponent<TECS::Position>(e1));
	ASSERT_TRUE(ecs.GetComponent<TECS::Position>(e1).x == 5);
	ASSERT_TRUE(ecs.GetComponentCount<TECS::Position>() == 1);

	ecs.Clear();
}

TEST(ECS, TestClearTag)
{
	TECS::ECS ecs{ };

	TECS::EntityID e1 = TECS::CreatePlayer(ecs, 2, 1);
	TECS::EntityID e2 = TECS::CreateEnemy(ecs, 1, 1);
	ecs.AddTag<TECS::IsWet>(e1);
	ecs.AddTag<TECS::IsWet>(e2);

	ASSERT_TRUE(ecs.HasTag<TECS::IsWet>(e1));
	ASSERT_TRUE(ecs.HasTag<TECS::IsWet>(e2));

	ecs.ClearTag<TECS::IsWet>();

	ASSERT_TRUE(!ecs.HasTag<TECS::IsWet>(e1));
	ASSERT_TRUE(!ecs.HasTag<TECS::IsWet>(e2));

	/* Unrelated components are untouched */
	ASSERT_TRUE(ecs.HasComponent<TECS::Position>(e1));
	ASSERT_TRUE(ecs.HasComponent<TECS::Position>(e2));

	/* Capacity for existing entities is preserved, so re-adding still works */
	ecs.AddTag<TECS::IsWet>(e1);
	ASSERT_TRUE(ecs.HasTag<TECS::IsWet>(e1));
	ASSERT_TRUE(!ecs.HasTag<TECS::IsWet>(e2));

	ecs.Clear();
}

TEST(ECS, TestTagCreation)
{
	TECS::ECS ecs{ };

	TECS::EntityID e1 = ecs.CreateEntity();
	TECS::EntityID e2 = ecs.CreateEntity();
	TECS::EntityID e3 = ecs.CreateEntity();
	TECS::EntityID e4 = ecs.CreateEntity();

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

	std::vector<TECS::EntityID> tagged_entities{};
	for (auto e : ecs.TView<TECS::IsWet>())
	{
		tagged_entities.push_back(e);
	}
	ASSERT_TRUE(tagged_entities[0] == e3);
	ASSERT_TRUE(tagged_entities[1] == e4);

	ecs.Clear();
}

TEST(ECS, TestDualManager)
{
	TECS::ECS ecs1{ };
	TECS::ECS ecs2{ };

	TECS::EntityID e1_ecs1 = ecs1.CreateEntity();
	TECS::EntityID e1_ecs2 = ecs2.CreateEntity();
	TECS::EntityID e2_ecs2 = ecs2.CreateEntity();
	TECS::EntityID e2_ecs1 = ecs1.CreateEntity();

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

TEST(ECS, TestUseAfterClear)
{
	TECS::ECS ecs{ };
	ecs.Clear();

	constexpr int player_pos_x{ 2 };
	constexpr int player_pos_y{ 1 };

	TECS::EntityID e = TECS::CreatePlayer(ecs, player_pos_x, player_pos_y);
	ASSERT_TRUE(ecs.HasComponent<TECS::Position>(e));
	ASSERT_TRUE(ecs.HasComponent<TECS::Velocity>(e));
	ASSERT_TRUE(ecs.HasComponent<TECS::Player>(e));
	ASSERT_TRUE(!ecs.HasComponent<TECS::Enemy>(e));
	ASSERT_TRUE(!ecs.HasComponent<TECS::Weapon>(e));

	ASSERT_TRUE(ecs.GetComponent<TECS::Position>(e).x == player_pos_x);
	ASSERT_TRUE(ecs.GetComponent<TECS::Position>(e).y == player_pos_y);

}

namespace TECSRestore
{
	using TECS::EntityID;
	using TECS::Position;
	using TECS::IsWet;

	struct Crate
	{
		static constexpr lcs::ComponentType component_type = lcs::ComponentType::ComponentChunked;
		int contents;
	};

	using ECS = lcs::ECSManager<EntityID, Position, Crate, IsWet>;

	inline std::vector<EntityID> Entities(ECS& ecs)
	{
		std::vector<EntityID> entities{};
		for (const EntityID e : ecs.Entities())
		{
			entities.push_back(e);
		}
		return entities;
	}

	/* An id that has been through a destroy and a reuse, so its validation bits are not 0. */
	inline EntityID ReusedID()
	{
		ECS ecs{};
		const EntityID first = ecs.CreateEntity();
		ecs.DestroyEntity(first);
		return ecs.CreateEntity();
	}
}

TEST(ECSRestore, TheGivenIdsExistAndHoldComponentsAndTags)
{
	TECSRestore::ECS ecs{};
	const std::vector<TECSRestore::EntityID> ids{ TECS::EntityID::CreateNew(0), TECS::EntityID::CreateNew(2), TECS::EntityID::CreateNew(5) };

	ecs.CreateEntitiesFromIDs(ids);

	ASSERT_TRUE(ecs.GetEntityCount() == 3);
	ASSERT_TRUE(TECSRestore::Entities(ecs) == ids);
	ecs.AddComponent<TECSRestore::Position>(ids[1], { 3, 4 });
	ecs.AddComponent<TECSRestore::Crate>(ids[2], { 9 });
	ecs.AddTag<TECSRestore::IsWet>(ids[0]);
	ASSERT_TRUE(ecs.HasComponent<TECSRestore::Position>(ids[1]));
	ASSERT_TRUE(ecs.GetComponent<TECSRestore::Position>(ids[1]).x == 3);
	ASSERT_TRUE(ecs.GetComponent<TECSRestore::Position>(ids[1]).y == 4);
	ASSERT_TRUE(ecs.GetComponent<TECSRestore::Crate>(ids[2]).contents == 9);
	ASSERT_TRUE(!ecs.HasComponent<TECSRestore::Position>(ids[0]));
	ASSERT_TRUE(ecs.HasTag<TECSRestore::IsWet>(ids[0]));
	ASSERT_TRUE(!ecs.HasTag<TECSRestore::IsWet>(ids[2]));
}

/* The sparse arrays start at 8 and only grow with CreateEntity. */
TEST(ECSRestore, IdsFarBeyondTheInitialReservationCanHoldComponents)
{
	TECSRestore::ECS ecs{};
	const std::vector<TECSRestore::EntityID> ids{ TECS::EntityID::CreateNew(1), TECS::EntityID::CreateNew(40), TECS::EntityID::CreateNew(1000) };

	ecs.CreateEntitiesFromIDs(ids);

	for (const TECSRestore::EntityID id : ids)
	{
		ecs.AddComponent<TECSRestore::Position>(id, { int(id.GetIndex()), 0 });
		ecs.AddComponent<TECSRestore::Crate>(id, { int(id.GetIndex()) });
		ecs.AddTag<TECSRestore::IsWet>(id);
	}
	for (const TECSRestore::EntityID id : ids)
	{
		ASSERT_TRUE(ecs.GetComponent<TECSRestore::Position>(id).x == int(id.GetIndex()));
		ASSERT_TRUE(ecs.GetComponent<TECSRestore::Crate>(id).contents == int(id.GetIndex()));
		ASSERT_TRUE(ecs.HasTag<TECSRestore::IsWet>(id));
	}
	ASSERT_TRUE(ecs.GetComponentCount<TECSRestore::Position>() == 3);
}

TEST(ECSRestore, CreateEntityFillsTheGapsThenGrowsAndTheNewEntitiesWork)
{
	TECSRestore::ECS ecs{};
	ecs.CreateEntitiesFromIDs(std::vector<TECSRestore::EntityID>{ TECS::EntityID::CreateNew(0), TECS::EntityID::CreateNew(3) });

	std::vector<uint32_t> indices{};
	for (int n = 0; n < 40; n++)
	{
		const TECSRestore::EntityID e = ecs.CreateEntity();
		ecs.AddComponent<TECSRestore::Position>(e, { n, n });
		indices.push_back(e.GetIndex());
	}

	ASSERT_TRUE(indices[0] == 1);
	ASSERT_TRUE(indices[1] == 2);
	ASSERT_TRUE(indices[2] == 4);
	ASSERT_TRUE(indices[39] == 41);
	ASSERT_TRUE(ecs.GetEntityCount() == 42);
}

/* The restore fills the reservation exactly, so the next CreateEntity has to grow the arrays itself. */
TEST(ECSRestore, GrowthAfterARestoreThatExactlyFilledTheReservation)
{
	TECSRestore::ECS ecs{};
	std::vector<TECSRestore::EntityID> ids{};
	for (uint32_t i = 0; i < 20; i++)
	{
		ids.push_back(TECS::EntityID::CreateNew(i));
	}
	ecs.CreateEntitiesFromIDs(ids);

	for (int n = 0; n < 50; n++)
	{
		const TECSRestore::EntityID e = ecs.CreateEntity();
		ecs.AddComponent<TECSRestore::Position>(e, { n, 0 });
		ecs.AddTag<TECSRestore::IsWet>(e);
	}

	ASSERT_TRUE(ecs.GetEntityCount() == 70);
}

TEST(ECSRestore, ValidationBitsAreKeptSoARestoredIdEqualsTheSavedOne)
{
	const TECSRestore::EntityID saved = TECSRestore::ReusedID();
	ASSERT_TRUE(saved.GetValidationID() != 0);

	TECSRestore::ECS ecs{};
	ecs.CreateEntitiesFromIDs(std::vector<TECSRestore::EntityID>{ saved });
	ecs.AddComponent<TECSRestore::Position>(saved, { 1, 2 });

	ASSERT_TRUE(TECSRestore::Entities(ecs)[0] == saved);
	ASSERT_TRUE(ecs.GetComponent<TECSRestore::Position>(saved).y == 2);

	ecs.DestroyEntity(saved);
	const TECSRestore::EntityID after = ecs.CreateEntity();
	ASSERT_TRUE(after.GetIndex() == saved.GetIndex());
	ASSERT_TRUE(after != saved);
}

TEST(ECSRestore, ARestoredEntityCanBeDestroyedAndItsComponentsGo)
{
	TECSRestore::ECS ecs{};
	const std::vector<TECSRestore::EntityID> ids{ TECS::EntityID::CreateNew(0), TECS::EntityID::CreateNew(4) };
	ecs.CreateEntitiesFromIDs(ids);
	ecs.AddComponent<TECSRestore::Position>(ids[1], { 1, 1 });
	ecs.AddTag<TECSRestore::IsWet>(ids[1]);

	ecs.DestroyEntity(ids[1]);

	ASSERT_TRUE(ecs.GetEntityCount() == 1);
	ASSERT_TRUE(ecs.GetComponentCount<TECSRestore::Position>() == 0);
	/* The slot just freed comes back first (last freed, first reused), then the original gaps. */
	const TECSRestore::EntityID reused = ecs.CreateEntity();
	ASSERT_TRUE(reused.GetIndex() == ids[1].GetIndex());
	ASSERT_TRUE(reused != ids[1]);
	ASSERT_TRUE(!ecs.HasComponent<TECSRestore::Position>(reused));
	ASSERT_TRUE(!ecs.HasTag<TECSRestore::IsWet>(reused));
	ASSERT_TRUE(ecs.CreateEntity().GetIndex() == 1);
}

TEST(ECSRestore, AWorldCanBeRestoredIntoAnotherECSWithTheSameIds)
{
	TECSRestore::ECS source{};
	std::vector<TECSRestore::EntityID> created{};
	for (int n = 0; n < 12; n++)
	{
		created.push_back(source.CreateEntity());
		source.AddComponent<TECSRestore::Position>(created.back(), { n, -n });
	}
	source.DestroyEntity(created[3]);
	source.DestroyEntity(created[8]);
	const TECSRestore::EntityID refilled = source.CreateEntity(); /* index 8, a new validation id */
	source.AddComponent<TECSRestore::Position>(refilled, { 100, 100 });

	TECSRestore::ECS restored{};
	restored.CreateEntitiesFromIDs(TECSRestore::Entities(source));

	ASSERT_TRUE(TECSRestore::Entities(restored) == TECSRestore::Entities(source));
	ASSERT_TRUE(restored.GetEntityCount() == source.GetEntityCount());
	for (const TECSRestore::EntityID e : TECSRestore::Entities(source))
	{
		restored.AddComponent<TECSRestore::Position>(e, TECSRestore::Position(source.GetComponent<TECSRestore::Position>(e)));
		ASSERT_TRUE(restored.GetComponent<TECSRestore::Position>(e).x == source.GetComponent<TECSRestore::Position>(e).x);
	}
}

TEST(ECSRestore, RestoreWorksAgainAfterClear)
{
	TECSRestore::ECS ecs{};
	ecs.CreateEntity();
	ecs.CreateEntity();
	ecs.Clear();

	const std::vector<TECSRestore::EntityID> ids{ TECS::EntityID::CreateNew(2), TECS::EntityID::CreateNew(30) };
	ecs.CreateEntitiesFromIDs(ids);
	ecs.AddComponent<TECSRestore::Position>(ids[1], { 5, 5 });

	ASSERT_TRUE(TECSRestore::Entities(ecs) == ids);
	ASSERT_TRUE(ecs.GetComponent<TECSRestore::Position>(ids[1]).x == 5);
}

TEST(ECSRestore, RestoringNothingLeavesAWorkingEmptyECS)
{
	TECSRestore::ECS ecs{};

	ecs.CreateEntitiesFromIDs({});

	ASSERT_TRUE(ecs.GetEntityCount() == 0);
	const TECSRestore::EntityID e = ecs.CreateEntity();
	ASSERT_TRUE(e.GetIndex() == 0);
	ecs.AddComponent<TECSRestore::Position>(e, { 1, 1 });
}
