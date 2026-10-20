#pragma once
#include <gtest/gtest.h>

#include <array>
#include <string_view>
#include <vector>

#include <lutra-ecs/save/Schema.h>

#include "TSaveTypes.h"

/**
 *  The schema is the saved types as data, and the compile-time proof that every
 *  type of the ECS has been chosen for.
 */
namespace TSchema
{
	using TSave::MySchema;

	template <typename T>
	std::span<const std::byte> BytesOf(const T& value)
	{
		return std::as_bytes(std::span(&value, 1));
	}
}

TEST(Schema, HasOneEntryPerSavedTypeInListOrder)
{
	const auto& entries = TSchema::MySchema::entries;

	ASSERT_TRUE(entries.size() == 5);
	ASSERT_TRUE(entries[0].id == 1 && std::string_view(entries[0].name) == "Position");
	ASSERT_TRUE(entries[1].id == 2 && std::string_view(entries[1].name) == "Enemy");
	ASSERT_TRUE(entries[4].id == 100 && std::string_view(entries[4].name) == "Wet");
}

TEST(Schema, EntriesSayHowBigAComponentIsAndWhetherItIsATag)
{
	const lcs::save::SchemaEntry* position = TSchema::MySchema::Find(1);
	const lcs::save::SchemaEntry* wet = TSchema::MySchema::Find(100);

	ASSERT_TRUE(position != nullptr && wet != nullptr);
	ASSERT_TRUE(position->size == sizeof(TSave::Position) && !position->is_tag);
	ASSERT_TRUE(wet->size == 0 && wet->is_tag);
}

TEST(Schema, FindIsNullForAnIdThisBuildDoesNotKnow)
{
	ASSERT_TRUE(TSchema::MySchema::Find(9999) == nullptr);
	ASSERT_TRUE(TSchema::MySchema::Find(0) == nullptr);
}

TEST(Schema, BytesThatAreInRangeAreValid)
{
	ASSERT_TRUE(TSchema::MySchema::BytesAreValid(2, TSchema::BytesOf(TSave::Enemy{ 5 })));
}

TEST(Schema, BytesThatTheTraitsRejectAreNotValid)
{
	ASSERT_TRUE(!TSchema::MySchema::BytesAreValid(2, TSchema::BytesOf(TSave::Enemy{ -1 })));
}

TEST(Schema, BytesOfTheWrongSizeAreNotValid)
{
	const std::array<std::byte, 3> short_bytes{};

	ASSERT_TRUE(!TSchema::MySchema::BytesAreValid(2, short_bytes));
	ASSERT_TRUE(!TSchema::MySchema::BytesAreValid(100, TSchema::BytesOf(TSave::Enemy{ 1 })));
}

TEST(Schema, BytesOfAnUnknownIdAreNotValid)
{
	ASSERT_TRUE(!TSchema::MySchema::BytesAreValid(9999, TSchema::BytesOf(TSave::Enemy{ 1 })));
}

TEST(Schema, AnEmptyTagHasNoBytesAndIsValid)
{
	ASSERT_TRUE(TSchema::MySchema::BytesAreValid(100, {}));
}

TEST(Schema, ForEachSavedVisitsEverySavedTypeInOrder)
{
	std::vector<std::string_view> names;

	TSchema::MySchema::ForEachSaved([&](auto type) {
		using T = typename decltype(type)::type;
		names.push_back(TSave::Traits<T>::name);
	});

	ASSERT_TRUE((names == std::vector<std::string_view>{ "Position", "Enemy", "Follower", "Leash", "Wet" }));
}

TEST(Schema, NoTransientTypeIsPresentInAnECSThatHasOnlySavedOnes)
{
	TSave::ECS ecs;
	ecs.AddComponent<TSave::Position>(ecs.CreateEntity(), { 1, 2 });

	ASSERT_TRUE(TSchema::MySchema::NothingTransientIsPresent(ecs));
}

TEST(Schema, ATransientComponentIsFound)
{
	TSave::ECS ecs;
	ecs.AddComponent<TSave::Intent>(ecs.CreateEntity(), { 1 });

	ASSERT_TRUE(!TSchema::MySchema::NothingTransientIsPresent(ecs));
}

TEST(Schema, ATransientTagIsFound)
{
	TSave::ECS ecs;
	ecs.AddTag<TSave::Pending>(ecs.CreateEntity());

	ASSERT_TRUE(!TSchema::MySchema::NothingTransientIsPresent(ecs));
}

TEST(ECSTypes, IsEmptyIsTrueUntilSomethingIsAddedForComponentsAndTagsAlike)
{
	TSave::ECS ecs;
	const TSave::EntityID entity = ecs.CreateEntity();
	ASSERT_TRUE(ecs.IsEmpty<TSave::Position>() && ecs.IsEmpty<TSave::Wet>());

	ecs.AddComponent<TSave::Position>(entity, { 0, 0 });
	ecs.AddTag<TSave::Wet>(entity);

	ASSERT_TRUE(!ecs.IsEmpty<TSave::Position>() && !ecs.IsEmpty<TSave::Wet>());

	ecs.RemoveComponent<TSave::Position>(entity);
	ecs.RemoveTag<TSave::Wet>(entity);

	ASSERT_TRUE(ecs.IsEmpty<TSave::Position>() && ecs.IsEmpty<TSave::Wet>());
}

/*
 *  Checked when this compiles
 */

namespace TSchema
{
	static_assert(std::is_same_v<TSave::ECS::Types, lcs::TypeList<TSave::Position, TSave::Enemy, TSave::Follower, TSave::Leash, TSave::Wet, TSave::Intent, TSave::Pending>>);

	static_assert(lcs::save::IdsAreUnique(MySchema::entries));
	static_assert(lcs::save::IdsAreUnique(std::array<lcs::save::SchemaEntry, 0>{}));
	static_assert(!lcs::save::IdsAreUnique(std::array<lcs::save::SchemaEntry, 2>{ lcs::save::SchemaEntry{ 7, "a", 4, false }, lcs::save::SchemaEntry{ 7, "b", 4, false } }),
		"a repeated id is caught");

	static_assert(lcs::IsOneOf<int>(lcs::TypeList<char, int>{}));
	static_assert(!lcs::IsOneOf<long>(lcs::TypeList<char, int>{}));
	static_assert(lcs::EveryTypeIsIn(lcs::TypeList<int>{}, lcs::TypeList<char, int>{}));
	static_assert(!lcs::EveryTypeIsIn(lcs::TypeList<int, long>{}, lcs::TypeList<char, int>{}));
}
