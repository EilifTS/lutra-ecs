#pragma once
#include <gtest/gtest.h>
#include <lutra-ecs/EntityIDGenerator.h>

TEST(EntityIDGenerator, GetIndices)
{
	lcs::EntityIDGenerator list{};
	lcs::EntityID i1 = list.GetNextEntityID();
	lcs::EntityID i2 = list.GetNextEntityID();
	lcs::EntityID i3 = list.GetNextEntityID();

	ASSERT_TRUE(i1 == 0);
	ASSERT_TRUE(i2 == 1);
	ASSERT_TRUE(i3 == 2);
}

TEST(EntityIDGenerator, GetAndFreeIndices)
{
	lcs::EntityIDGenerator list{};
	lcs::EntityID i1 = list.GetNextEntityID();
	lcs::EntityID i2_before = list.GetNextEntityID();
	lcs::EntityID i3 = list.GetNextEntityID();

	list.FreeEntityID(i2_before);
	lcs::EntityID i2_after = list.GetNextEntityID();
	ASSERT_TRUE(i2_before == i2_after);
}

TEST(EntityIDGenerator, GetAndFreeIndices2)
{
	lcs::EntityIDGenerator list{};
	lcs::EntityID i1_before = list.GetNextEntityID();
	lcs::EntityID i2_before = list.GetNextEntityID();
	lcs::EntityID i3_before = list.GetNextEntityID();
	lcs::EntityID i4_before = list.GetNextEntityID();
	lcs::EntityID i5_before = list.GetNextEntityID();

	list.FreeEntityID(i3_before);
	list.FreeEntityID(i4_before);
	list.FreeEntityID(i2_before);
	lcs::EntityID i2_after = list.GetNextEntityID();
	lcs::EntityID i3_after = list.GetNextEntityID();
	lcs::EntityID i4_after = list.GetNextEntityID();
	ASSERT_TRUE(i2_before == i2_after);
	ASSERT_TRUE(i3_before == i3_after);
	ASSERT_TRUE(i4_before == i4_after);
}

TEST(EntityIDGenerator, Clear)
{
	lcs::EntityIDGenerator list{};
	lcs::EntityID i1_before = list.GetNextEntityID();
	lcs::EntityID i2_before = list.GetNextEntityID();
	lcs::EntityID i3_before = list.GetNextEntityID();
	list.Clear();

	lcs::EntityID i1_after = list.GetNextEntityID();
	lcs::EntityID i2_after = list.GetNextEntityID();
	lcs::EntityID i3_after = list.GetNextEntityID();

	ASSERT_TRUE(i1_before == i1_after);
	ASSERT_TRUE(i2_before == i2_after);
	ASSERT_TRUE(i3_before == i3_after);

	list.FreeEntityID(i2_after);
	list.Clear();

	lcs::EntityID i1_after2 = list.GetNextEntityID();
	lcs::EntityID i2_after2 = list.GetNextEntityID();
	lcs::EntityID i3_after2 = list.GetNextEntityID();

	ASSERT_TRUE(i1_after == i1_after2);
	ASSERT_TRUE(i2_after == i2_after2);
	ASSERT_TRUE(i3_after == i3_after2);
}

TEST(EntityIDGenerator, MaxIndex)
{
	lcs::EntityIDGenerator list{};
	const lcs::EntityID i1 = list.GetNextEntityID();
	const lcs::EntityID i2 = list.GetNextEntityID();
	const lcs::EntityID i3 = list.GetNextEntityID();
	const uint32_t max_size1 = list.MaxIndex();

	list.FreeEntityID(i1);
	const uint32_t max_size2 = list.MaxIndex();

	list.FreeEntityID(i2);
	const uint32_t max_size3 = list.MaxIndex();

	ASSERT_TRUE(max_size1 == max_size2);
	ASSERT_TRUE(max_size1 == max_size3);
}

TEST(EntityIDGenerator, IsFree)
{
	lcs::EntityIDGenerator list{};
	const lcs::EntityID i1 = list.GetNextEntityID();
	const lcs::EntityID i2 = list.GetNextEntityID();
	const lcs::EntityID i3 = list.GetNextEntityID();
	ASSERT_TRUE(!list.IsFree(i1));
	ASSERT_TRUE(!list.IsFree(i2));
	ASSERT_TRUE(!list.IsFree(i3));

	list.FreeEntityID(i1);
	ASSERT_TRUE(list.IsFree(i1));
	ASSERT_TRUE(!list.IsFree(i2));
	ASSERT_TRUE(!list.IsFree(i3));

	list.FreeEntityID(i3);
	ASSERT_TRUE(list.IsFree(i1));
	ASSERT_TRUE(!list.IsFree(i2));
	ASSERT_TRUE(list.IsFree(i3));

	list.FreeEntityID(i2);
	ASSERT_TRUE(list.IsFree(i1));
	ASSERT_TRUE(list.IsFree(i2));
	ASSERT_TRUE(list.IsFree(i3));
}

TEST(EntityIDGenerator, OccupiedIterator)
{
	lcs::EntityIDGenerator list{};
	const lcs::EntityID i1 = list.GetNextEntityID();
	const lcs::EntityID i2 = list.GetNextEntityID();
	const lcs::EntityID i3 = list.GetNextEntityID();

	std::vector<lcs::EntityID> occs1{};
	for (const auto& i : list.OccupiedIndices())
	{
		occs1.push_back(i);
	}
	ASSERT_TRUE(occs1.size() == 3);
	ASSERT_TRUE(occs1[0] == i1);
	ASSERT_TRUE(occs1[1] == i2);
	ASSERT_TRUE(occs1[2] == i3);

	list.FreeEntityID(i2);
	std::vector<lcs::EntityID> occs2{};
	for (const auto& i : list.OccupiedIndices())
	{
		occs2.push_back(i);
	}
	ASSERT_TRUE(occs2.size() == 2);
	ASSERT_TRUE(occs2[0] == i1);
	ASSERT_TRUE(occs2[1] == i3);
}

TEST(EntityIDGenerator, OccupiedIteratorReverse)
{
	lcs::EntityIDGenerator list{};
	const lcs::EntityID i1 = list.GetNextEntityID();
	const lcs::EntityID i2 = list.GetNextEntityID();
	const lcs::EntityID i3 = list.GetNextEntityID();

	std::vector<lcs::EntityID> occs1{};
	for (const auto& i : list.OccupiedIndicesReverse())
	{
		occs1.push_back(i);
	}
	ASSERT_TRUE(occs1.size() == 3);
	ASSERT_TRUE(occs1[2] == i1);
	ASSERT_TRUE(occs1[1] == i2);
	ASSERT_TRUE(occs1[0] == i3);

	list.FreeEntityID(i2);
	std::vector<lcs::EntityID> occs2{};
	for (const auto& i : list.OccupiedIndicesReverse())
	{
		occs2.push_back(i);
	}
	ASSERT_TRUE(occs2.size() == 2);
	ASSERT_TRUE(occs2[1] == i1);
	ASSERT_TRUE(occs2[0] == i3);
}

TEST(EntityIDGenerator, Empty)
{
	lcs::EntityIDGenerator list{};
	ASSERT_TRUE(list.IsFree(0));
	ASSERT_TRUE(list.MaxIndex() == 0);
	for (auto i : list.OccupiedIndices()) {}
}