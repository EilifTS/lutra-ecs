#pragma once
#include <gtest/gtest.h>
#include <lutra-ecs/IndexFreeList.h>
#include <ranges>

TEST(IndexFreeList, GetIndices)
{
	lcs::IndexFreeList list{};
	u32 i1 = list.GetNextIndex();
	u32 i2 = list.GetNextIndex();
	u32 i3 = list.GetNextIndex();

	ASSERT_TRUE(i1 == 0);
	ASSERT_TRUE(i2 == 1);
	ASSERT_TRUE(i3 == 2);
}

TEST(IndexFreeList, GetAndFreeIndices)
{
	lcs::IndexFreeList list{};
	u32 i1 = list.GetNextIndex();
	u32 i2_before = list.GetNextIndex();
	u32 i3 = list.GetNextIndex();

	list.FreeIndex(i2_before);
	u32 i2_after = list.GetNextIndex();
	ASSERT_TRUE(i2_before == i2_after);
}

TEST(IndexFreeList, GetAndFreeIndices2)
{
	lcs::IndexFreeList list{};
	u32 i1_before = list.GetNextIndex();
	u32 i2_before = list.GetNextIndex();
	u32 i3_before = list.GetNextIndex();
	u32 i4_before = list.GetNextIndex();
	u32 i5_before = list.GetNextIndex();

	list.FreeIndex(i3_before);
	list.FreeIndex(i4_before);
	list.FreeIndex(i2_before);
	u32 i2_after = list.GetNextIndex();
	u32 i3_after = list.GetNextIndex();
	u32 i4_after = list.GetNextIndex();
	ASSERT_TRUE(i2_before == i2_after);
	ASSERT_TRUE(i3_before == i3_after);
	ASSERT_TRUE(i4_before == i4_after);
}

TEST(IndexFreeList, Clear)
{
	lcs::IndexFreeList list{};
	u32 i1_before = list.GetNextIndex();
	u32 i2_before = list.GetNextIndex();
	u32 i3_before = list.GetNextIndex();
	list.Clear();

	u32 i1_after = list.GetNextIndex();
	u32 i2_after = list.GetNextIndex();
	u32 i3_after = list.GetNextIndex();

	ASSERT_TRUE(i1_before == i1_after);
	ASSERT_TRUE(i2_before == i2_after);
	ASSERT_TRUE(i3_before == i3_after);

	list.FreeIndex(i2_after);
	list.Clear();

	u32 i1_after2 = list.GetNextIndex();
	u32 i2_after2 = list.GetNextIndex();
	u32 i3_after2 = list.GetNextIndex();

	ASSERT_TRUE(i1_after == i1_after2);
	ASSERT_TRUE(i2_after == i2_after2);
	ASSERT_TRUE(i3_after == i3_after2);
}

TEST(IndexFreeList, MaxIndex)
{
	lcs::IndexFreeList list{};
	const u32 i1 = list.GetNextIndex();
	const u32 i2 = list.GetNextIndex();
	const u32 i3 = list.GetNextIndex();
	const u32 max_size1 = list.MaxIndex();

	list.FreeIndex(i1);
	const u32 max_size2 = list.MaxIndex();

	list.FreeIndex(i2);
	const u32 max_size3 = list.MaxIndex();

	ASSERT_TRUE(max_size1 == max_size2);
	ASSERT_TRUE(max_size1 == max_size3);
}

TEST(IndexFreeList, IsFree)
{
	lcs::IndexFreeList list{};
	const u32 i1 = list.GetNextIndex();
	const u32 i2 = list.GetNextIndex();
	const u32 i3 = list.GetNextIndex();
	ASSERT_TRUE(!list.IsFree(i1));
	ASSERT_TRUE(!list.IsFree(i2));
	ASSERT_TRUE(!list.IsFree(i3));

	list.FreeIndex(i1);
	ASSERT_TRUE(list.IsFree(i1));
	ASSERT_TRUE(!list.IsFree(i2));
	ASSERT_TRUE(!list.IsFree(i3));

	list.FreeIndex(i3);
	ASSERT_TRUE(list.IsFree(i1));
	ASSERT_TRUE(!list.IsFree(i2));
	ASSERT_TRUE(list.IsFree(i3));

	list.FreeIndex(i2);
	ASSERT_TRUE(list.IsFree(i1));
	ASSERT_TRUE(list.IsFree(i2));
	ASSERT_TRUE(list.IsFree(i3));
}

TEST(IndexFreeList, OccupiedIterator)
{
	lcs::IndexFreeList list{};
	const u32 i1 = list.GetNextIndex();
	const u32 i2 = list.GetNextIndex();
	const u32 i3 = list.GetNextIndex();

	std::vector<u32> occs1{};
	for (const auto& i : list)
	{
		occs1.push_back(i);
	}
	ASSERT_TRUE(occs1.size() == 3);
	ASSERT_TRUE(occs1[0] == i1);
	ASSERT_TRUE(occs1[1] == i2);
	ASSERT_TRUE(occs1[2] == i3);

	list.FreeIndex(i2);
	std::vector<u32> occs2{};
	for (auto i : list)
	{
		occs2.push_back(i);
	}
	ASSERT_TRUE(occs2.size() == 2);
	ASSERT_TRUE(occs2[0] == i1);
	ASSERT_TRUE(occs2[1] == i3);
}

TEST(IndexFreeList, OccupiedIteratorReverse)
{
	lcs::IndexFreeList list{};
	const u32 i1 = list.GetNextIndex();
	const u32 i2 = list.GetNextIndex();
	const u32 i3 = list.GetNextIndex();

	std::vector<u32> occs1{};
	for (auto it = std::rbegin(list); it != std::rend(list); ++it)
	{
		auto i = *it;
		occs1.push_back(i);
	}
	ASSERT_TRUE(occs1.size() == 3);
	ASSERT_TRUE(occs1[2] == i1);
	ASSERT_TRUE(occs1[1] == i2);
	ASSERT_TRUE(occs1[0] == i3);

	list.FreeIndex(i2);
	std::vector<u32> occs2{};
	for (auto it = std::rbegin(list); it != std::rend(list); ++it)
	{
		auto i = *it;
		occs2.push_back(i);
	}
	ASSERT_TRUE(occs2.size() == 2);
	ASSERT_TRUE(occs2[1] == i1);
	ASSERT_TRUE(occs2[0] == i3);
}

TEST(IndexFreeList, Empty)
{
	lcs::IndexFreeList list{};
	ASSERT_TRUE(list.IsFree(0));
	ASSERT_TRUE(list.MaxIndex() == 0);
	for (auto i : list) {}
}