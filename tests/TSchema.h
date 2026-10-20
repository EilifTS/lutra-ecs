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
	ASSERT_TRUE(entries[0].name == "Position");
	ASSERT_TRUE(entries[1].name == "Enemy");
	ASSERT_TRUE(entries[4].name == "Wet");
}

TEST(Schema, EntriesSayHowBigAComponentIsAndWhetherItIsATag)
{
	const auto& entries = TSchema::MySchema::entries;

	ASSERT_TRUE(entries[0].size == sizeof(TSave::Position) && !entries[0].is_tag);
	ASSERT_TRUE(entries[4].size == 0 && entries[4].is_tag);
}

TEST(Schema, ForEachSavedVisitsEverySavedTypeInOrder)
{
	std::vector<std::string_view> names;

	TSchema::MySchema::ForEachSaved([&](auto type) {
		using T = typename decltype(type)::type;
		names.push_back(T::save_name);
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

TEST(Schema, SavedTypesAreTheOnesThatSaySoInTheECSsOrder)
{
	static_assert(std::is_same_v<TSchema::MySchema::SavedTypes, lcs::TypeList<TSave::Position, TSave::Enemy, TSave::Follower, TSave::Leash, TSave::Wet>>);
	static_assert(std::is_same_v<TSchema::MySchema::TransientTypes, lcs::TypeList<TSave::Intent, TSave::Pending>>);
	ASSERT_TRUE(TSchema::MySchema::entries.size() == 5);
}

/*
 *  Checked when this compiles
 */

namespace TSchema
{
	static_assert(std::is_same_v<TSave::ECS::Types, lcs::TypeList<TSave::Position, TSave::Enemy, TSave::Follower, TSave::Leash, TSave::Wet, TSave::Intent, TSave::Pending>>);

	static_assert(lcs::save::NamesAreUnique(MySchema::entries));
	static_assert(lcs::save::NamesAreUnique(std::array<lcs::save::SchemaEntry, 0>{}));
	static_assert(!lcs::save::NamesAreUnique(std::array<lcs::save::SchemaEntry, 2>{ lcs::save::SchemaEntry{ "a", 4, false }, lcs::save::SchemaEntry{ "a", 8, false } }),
		"a repeated name is caught");

	static_assert(lcs::IsOneOf<int>(lcs::TypeList<char, int>{}));
	static_assert(!lcs::IsOneOf<long>(lcs::TypeList<char, int>{}));
}

/*
 *  What a component says about itself, and what it need not say
 */

TEST(SaveMembers, ASaveNameIsReadFromTheComponent)
{
	ASSERT_TRUE(std::string_view(lcs::save::SaveNameOf<TSave::Leash>()) == "Leash");
}

TEST(SaveMembers, ATransientTypeNeedsNoName)
{
	static_assert(!TSave::Intent::saved && !TSave::Pending::saved);
	ASSERT_TRUE(TSchema::MySchema::entries.size() == 5);
}
