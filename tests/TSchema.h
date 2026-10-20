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

TEST(Schema, BytesThatTheComponentRejectsAreNotValid)
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

	static_assert(lcs::save::IdsAreUnique(MySchema::entries));
	static_assert(lcs::save::IdsAreUnique(std::array<lcs::save::SchemaEntry, 0>{}));
	static_assert(!lcs::save::IdsAreUnique(std::array<lcs::save::SchemaEntry, 2>{ lcs::save::SchemaEntry{ 7, "a", 4, false }, lcs::save::SchemaEntry{ 7, "b", 4, false } }),
		"a repeated id is caught");

	static_assert(lcs::IsOneOf<int>(lcs::TypeList<char, int>{}));
	static_assert(!lcs::IsOneOf<long>(lcs::TypeList<char, int>{}));
	static_assert(lcs::EveryTypeIsIn(lcs::TypeList<int>{}, lcs::TypeList<char, int>{}));
	static_assert(!lcs::EveryTypeIsIn(lcs::TypeList<int, long>{}, lcs::TypeList<char, int>{}));
}

/*
 *  What a component says about itself, and what it need not say
 */

TEST(SaveMembers, ASaveIdAndNameAreReadFromTheComponent)
{
	ASSERT_TRUE(lcs::save::SaveIdOf<TSave::Leash>() == 4);
	ASSERT_TRUE(std::string_view(lcs::save::SaveNameOf<TSave::Leash>()) == "Leash");
}

TEST(SaveMembers, ABytesCheckIsOptionalAndTheDefaultIsYes)
{
	const TSave::Position position{ 1, 2 };

	ASSERT_TRUE(lcs::save::SaveBytesAreValid<TSave::Position>(TSchema::BytesOf(position)));
	ASSERT_TRUE(lcs::save::SaveBytesAreValid<TSave::Enemy>(TSchema::BytesOf(TSave::Enemy{ 1 })));
	ASSERT_TRUE(!lcs::save::SaveBytesAreValid<TSave::Enemy>(TSchema::BytesOf(TSave::Enemy{ -1 })));
}

TEST(SaveMembers, EntityIdsAreOptionalAndTheDefaultIsNone)
{
	TSave::Position position{ 1, 2 };
	TSave::Follower follower{ TSave::EntityID::CreateNew(3) };
	int visited = 0;

	lcs::save::ForEachEntityRef(position, [&](TSave::EntityID&) { visited++; });
	ASSERT_TRUE(visited == 0);

	lcs::save::ForEachEntityRef(follower, [&](TSave::EntityID& ref) { visited++; ref = TSave::EntityID{}; });
	ASSERT_TRUE(visited == 1);
	ASSERT_TRUE(!follower.target.IsValid());
}

TEST(SaveMembers, ATransientTypeNeedsNoIdOrName)
{
	static_assert(!TSave::Intent::saved && !TSave::Pending::saved);
	ASSERT_TRUE(TSchema::MySchema::Find(1) != nullptr);
}
