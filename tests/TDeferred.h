#pragma once
#include <gtest/gtest.h>

#include <vector>

#include "TECS.h"

/**
 *  Deferred adds: what is deferred must not exist until FlushDeferred, and must
 *  exist after it.
 */
namespace TDeferred
{
	using TECS::EntityID;
	using TECS::Position;
	using TECS::Velocity;
	using TECS::Enemy;
	using TECS::Weapon;
	using TECS::IsWet;
	using TECS::ECS;

	/* An id that has been through a destroy and a reuse, so its validation bits are not 0. */
	inline EntityID ReusedID()
	{
		ECS scratch;
		scratch.DestroyEntity(scratch.CreateEntity());
		return scratch.CreateEntity();
	}

	inline std::vector<EntityID> EntitiesOf(ECS& ecs)
	{
		std::vector<EntityID> entities{};
		for (const EntityID entity : ecs.Entities())
		{
			entities.push_back(entity);
		}
		return entities;
	}
}

TEST(Deferred, ADeferredComponentIsNotThereUntilFlush)
{
	TDeferred::ECS ecs;

	const TDeferred::EntityID entity = ecs.CreateEntity();
	ecs.AddDeferred(entity, TDeferred::Enemy{ 5 });

	ASSERT_TRUE(entity.IsValid());
	ASSERT_TRUE(!ecs.HasComponent<TDeferred::Enemy>(entity));
}

TEST(Deferred, FlushAddsDeferredComponentsAndTags)
{
	TDeferred::ECS ecs;

	const TDeferred::EntityID entity = ecs.CreateEntity();
	ecs.AddDeferred(entity, TDeferred::Enemy{ 8 });
	ecs.AddDeferred(entity, TDeferred::IsWet{});

	ecs.FlushDeferred();

	ASSERT_TRUE(ecs.HasComponent<TDeferred::Enemy>(entity));
	ASSERT_TRUE(ecs.GetComponent<TDeferred::Enemy>(entity).strength == 8);
	ASSERT_TRUE(ecs.HasTag<TDeferred::IsWet>(entity));
}

/* A tag has no value to give. */
TEST(Deferred, ATagCanBeDeferredByTypeAlone)
{
	TDeferred::ECS ecs;

	const TDeferred::EntityID entity = ecs.CreateEntity();
	ecs.AddDeferred<TDeferred::IsWet>(entity);
	ASSERT_TRUE(!ecs.HasTag<TDeferred::IsWet>(entity));

	ecs.FlushDeferred();

	ASSERT_TRUE(ecs.HasTag<TDeferred::IsWet>(entity));
}

TEST(Deferred, FlushKeepsEachEntitysOwnValues)
{
	TDeferred::ECS ecs;

	const TDeferred::EntityID first = ecs.CreateEntity();
	const TDeferred::EntityID second = ecs.CreateEntity();
	ecs.AddDeferred(first, TDeferred::Enemy{ 1 });
	ecs.AddDeferred(second, TDeferred::Enemy{ 2 });

	ecs.FlushDeferred();

	ASSERT_TRUE(ecs.GetComponent<TDeferred::Enemy>(first).strength == 1);
	ASSERT_TRUE(ecs.GetComponent<TDeferred::Enemy>(second).strength == 2);
}

/* There is no list to keep: whatever the ECS has can be deferred, in whatever order. */
TEST(Deferred, EveryTypeTheECSHasCanBeDeferredInAnyOrder)
{
	TDeferred::ECS ecs;

	const TDeferred::EntityID entity = ecs.CreateEntity();
	ecs.AddDeferred(entity, TDeferred::IsWet{});
	ecs.AddDeferred(entity, TDeferred::Velocity{ 3, 4 });
	ecs.AddDeferred(entity, TDeferred::Enemy{ 7 });
	ecs.AddDeferred(entity, TDeferred::Position{ 1, 2 });
	ecs.FlushDeferred();

	ASSERT_TRUE(ecs.HasTag<TDeferred::IsWet>(entity));
	ASSERT_TRUE(ecs.GetComponent<TDeferred::Velocity>(entity).y == 4);
	ASSERT_TRUE(ecs.GetComponent<TDeferred::Enemy>(entity).strength == 7);
	ASSERT_TRUE(ecs.GetComponent<TDeferred::Position>(entity).x == 1);
}

TEST(Deferred, FlushAppliesEachDeferredItemOnce)
{
	TDeferred::ECS ecs;

	const TDeferred::EntityID entity = ecs.CreateEntity();
	ecs.AddDeferred(entity, TDeferred::Enemy{ 5 });
	ecs.FlushDeferred();

	ecs.GetComponent<TDeferred::Enemy>(entity).strength = 1;
	ecs.FlushDeferred();

	ASSERT_TRUE(ecs.GetComponent<TDeferred::Enemy>(entity).strength == 1);
}

TEST(Deferred, ClearDeferredDropsEverythingDeferred)
{
	TDeferred::ECS ecs;

	const TDeferred::EntityID entity = ecs.CreateEntity();
	ecs.AddDeferred(entity, TDeferred::Enemy{ 5 });
	ecs.AddDeferred(entity, TDeferred::IsWet{});

	ecs.ClearDeferred();
	ecs.FlushDeferred();

	ASSERT_TRUE(!ecs.HasComponent<TDeferred::Enemy>(entity));
	ASSERT_TRUE(!ecs.HasTag<TDeferred::IsWet>(entity));
}

TEST(Deferred, AChunkedComponentIsDeferredLikeAnyOther)
{
	TECSRestore::ECS ecs;

	const TECSRestore::EntityID entity = ecs.CreateEntity();
	ecs.AddDeferred(entity, TECSRestore::Crate{ 9 });
	ecs.AddDeferred(entity, TECSRestore::IsWet{});
	ecs.FlushDeferred();

	ASSERT_TRUE(ecs.GetComponent<TECSRestore::Crate>(entity).contents == 9);
	ASSERT_TRUE(ecs.HasTag<TECSRestore::IsWet>(entity));
}

/* Creating an entity defers nothing. */
TEST(Deferred, NothingIsDeferredUntilSomethingIsAdded)
{
	TDeferred::ECS ecs;

	ASSERT_TRUE(!ecs.HasDeferred());

	const TDeferred::EntityID entity = ecs.CreateEntity();
	ASSERT_TRUE(!ecs.HasDeferred());

	ecs.AddDeferred(entity, TDeferred::IsWet{});
	ASSERT_TRUE(ecs.HasDeferred());
}

TEST(Deferred, AnythingDeferredInAnyTypeCounts)
{
	TDeferred::ECS ecs;

	ecs.AddDeferred(ecs.CreateEntity(), TDeferred::Weapon{ 2 });

	ASSERT_TRUE(ecs.HasDeferred());
}

TEST(Deferred, NothingIsDeferredAfterFlushAndAfterClearDeferred)
{
	TDeferred::ECS ecs;
	const TDeferred::EntityID entity = ecs.CreateEntity();

	ecs.AddDeferred(entity, TDeferred::Enemy{ 1 });
	ecs.FlushDeferred();
	ASSERT_TRUE(!ecs.HasDeferred());

	ecs.AddDeferred(entity, TDeferred::IsWet{});
	ecs.ClearDeferred();
	ASSERT_TRUE(!ecs.HasDeferred());
}

/* The ids a deferred add holds mean nothing once the handle free list is reset. */
TEST(Deferred, ClearDropsWhatWasDeferred)
{
	TDeferred::ECS ecs;
	ecs.AddDeferred(ecs.CreateEntity(), TDeferred::IsWet{});

	ecs.Clear();

	ASSERT_TRUE(!ecs.HasDeferred());
	const TDeferred::EntityID fresh = ecs.CreateEntity();
	ecs.FlushDeferred();
	ASSERT_TRUE(!ecs.HasTag<TDeferred::IsWet>(fresh));
}

TEST(Deferred, ComponentsDeferredByIdAfterCreateEntitiesFromIDsAreAttachedByFlush)
{
	TDeferred::ECS ecs;
	const std::vector<TDeferred::EntityID> ids{ TDeferred::EntityID::CreateNew(1), TDeferred::EntityID::CreateNew(4) };
	ecs.CreateEntitiesFromIDs(ids);

	ecs.AddDeferred(ids[0], TDeferred::Enemy{ 9 });
	ecs.AddDeferred(ids[1], TDeferred::IsWet{});
	ASSERT_TRUE(!ecs.HasComponent<TDeferred::Enemy>(ids[0]));
	ecs.FlushDeferred();

	ASSERT_TRUE(ecs.GetComponent<TDeferred::Enemy>(ids[0]).strength == 9);
	ASSERT_TRUE(!ecs.HasComponent<TDeferred::Enemy>(ids[1]));
	ASSERT_TRUE(ecs.HasTag<TDeferred::IsWet>(ids[1]));
}

TEST(Deferred, FlushWorksOnRestoredEntitiesWithReusedValidationIds)
{
	TDeferred::ECS ecs;
	const TDeferred::EntityID reused = TDeferred::ReusedID();
	ASSERT_TRUE(reused.GetValidationID() != 0);
	const std::vector<TDeferred::EntityID> ids{ TDeferred::EntityID::CreateNew(0), TDeferred::EntityID::Create(reused.GetValidationID(), 2) };
	ecs.CreateEntitiesFromIDs(ids);

	ecs.AddDeferred(ids[1], TDeferred::Enemy{ 4 });
	ecs.FlushDeferred();

	ASSERT_TRUE(TDeferred::EntitiesOf(ecs) == ids);
	ASSERT_TRUE(ecs.GetComponent<TDeferred::Enemy>(ids[1]).strength == 4);
}
