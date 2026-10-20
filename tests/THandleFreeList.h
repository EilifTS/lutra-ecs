#pragma once
#include <gtest/gtest.h>
#include <lutra-ecs/HandleFreeList.h>
#include <ranges>

using TestHandle = lcs::Handle<uint32_t, 16>;

TEST(HandleFreeList, GetIndices)
{
	lcs::HandleFreeList<TestHandle> list{};
	const TestHandle i1 = list.GetNextHandle();
	const TestHandle i2 = list.GetNextHandle();
	const TestHandle i3 = list.GetNextHandle();

	ASSERT_TRUE(i1.GetIndex() == 0);
	ASSERT_TRUE(i2.GetIndex() == 1);
	ASSERT_TRUE(i3.GetIndex() == 2);

	ASSERT_TRUE(i1.GetValidationID() == 0);
	ASSERT_TRUE(i2.GetValidationID() == 0);
	ASSERT_TRUE(i3.GetValidationID() == 0);
}

TEST(HandleFreeList, GetAndFreeIndices)
{
	lcs::HandleFreeList<TestHandle> list{};
	const TestHandle i1 = list.GetNextHandle();
	const TestHandle i2_before = list.GetNextHandle();
	const TestHandle i3 = list.GetNextHandle();

	list.FreeHandle(i2_before);
	const TestHandle i2_after = list.GetNextHandle();
	ASSERT_TRUE(i2_before.GetIndex() == i2_after.GetIndex());
	ASSERT_TRUE(i2_before.GetValidationID() != i2_after.GetValidationID());
}

TEST(HandleFreeList, GetAndFreeIndices2)
{
	lcs::HandleFreeList<TestHandle> list{};
	const TestHandle i1_before = list.GetNextHandle();
	const TestHandle i2_before = list.GetNextHandle();
	const TestHandle i3_before = list.GetNextHandle();
	const TestHandle i4_before = list.GetNextHandle();
	const TestHandle i5_before = list.GetNextHandle();

	list.FreeHandle(i3_before);
	list.FreeHandle(i4_before);
	list.FreeHandle(i2_before);
	const TestHandle i2_after = list.GetNextHandle();
	const TestHandle i4_after = list.GetNextHandle();
	const TestHandle i3_after = list.GetNextHandle();

	ASSERT_TRUE(i2_before.GetIndex() == i2_after.GetIndex());
	ASSERT_TRUE(i3_before.GetIndex() == i3_after.GetIndex());
	ASSERT_TRUE(i4_before.GetIndex() == i4_after.GetIndex());

	ASSERT_TRUE(i2_before.GetValidationID() != i2_after.GetValidationID());
	ASSERT_TRUE(i3_before.GetValidationID() != i3_after.GetValidationID());
	ASSERT_TRUE(i4_before.GetValidationID() != i4_after.GetValidationID());
}

TEST(HandleFreeList, Clear)
{
	lcs::HandleFreeList<TestHandle> list{};
	const TestHandle i1_before = list.GetNextHandle();
	const TestHandle i2_before = list.GetNextHandle();
	const TestHandle i3_before = list.GetNextHandle();
	list.Clear();

	const TestHandle i1_after = list.GetNextHandle();
	const TestHandle i2_after = list.GetNextHandle();
	const TestHandle i3_after = list.GetNextHandle();

	ASSERT_TRUE(i1_before == i1_after);
	ASSERT_TRUE(i2_before == i2_after);
	ASSERT_TRUE(i3_before == i3_after);

	list.FreeHandle(i2_after);
	list.Clear();

	const TestHandle i1_after2 = list.GetNextHandle();
	const TestHandle i2_after2 = list.GetNextHandle();
	const TestHandle i3_after2 = list.GetNextHandle();

	ASSERT_TRUE(i1_after == i1_after2);
	ASSERT_TRUE(i2_after == i2_after2);
	ASSERT_TRUE(i3_after == i3_after2);
}

TEST(HandleFreeList, MaxIndex)
{
	lcs::HandleFreeList<TestHandle> list{};
	const TestHandle i1 = list.GetNextHandle();
	const TestHandle i2 = list.GetNextHandle();
	const TestHandle i3 = list.GetNextHandle();
	const uint32_t max_size1 = list.MaxIndex();

	list.FreeHandle(i1);
	const uint32_t max_size2 = list.MaxIndex();

	list.FreeHandle(i2);
	const uint32_t max_size3 = list.MaxIndex();

	ASSERT_TRUE(max_size1 == max_size2);
	ASSERT_TRUE(max_size1 == max_size3);
}

TEST(HandleFreeList, IsFree)
{
	lcs::HandleFreeList<TestHandle> list{};
	const TestHandle i1 = list.GetNextHandle();
	const TestHandle i2 = list.GetNextHandle();
	const TestHandle i3 = list.GetNextHandle();
	ASSERT_TRUE(list.IsOccupied(i1.GetIndex()));
	ASSERT_TRUE(list.IsOccupied(i2.GetIndex()));
	ASSERT_TRUE(list.IsOccupied(i3.GetIndex()));

	list.FreeHandle(i1);
	ASSERT_TRUE(!list.IsOccupied(i1.GetIndex()));
	ASSERT_TRUE(list.IsOccupied(i2.GetIndex()));
	ASSERT_TRUE(list.IsOccupied(i3.GetIndex()));

	list.FreeHandle(i3);
	ASSERT_TRUE(!list.IsOccupied(i1.GetIndex()));
	ASSERT_TRUE(list.IsOccupied(i2.GetIndex()));
	ASSERT_TRUE(!list.IsOccupied(i3.GetIndex()));

	list.FreeHandle(i2);
	ASSERT_TRUE(!list.IsOccupied(i1.GetIndex()));
	ASSERT_TRUE(!list.IsOccupied(i2.GetIndex()));
	ASSERT_TRUE(!list.IsOccupied(i3.GetIndex()));
}

TEST(HandleFreeList, OccupiedIterator)
{
	lcs::HandleFreeList<TestHandle> list{};
	const TestHandle i1 = list.GetNextHandle();
	const TestHandle i2 = list.GetNextHandle();
	const TestHandle i3 = list.GetNextHandle();

	std::vector<TestHandle> occs1{};
	for (const auto& i : list)
	{
		occs1.push_back(i);
	}
	ASSERT_TRUE(occs1.size() == 3);
	ASSERT_TRUE(occs1[0] == i1);
	ASSERT_TRUE(occs1[1] == i2);
	ASSERT_TRUE(occs1[2] == i3);

	list.FreeHandle(i2);
	std::vector<TestHandle> occs2{};
	for (auto i : list)
	{
		occs2.push_back(i);
	}
	ASSERT_TRUE(occs2.size() == 2);
	ASSERT_TRUE(occs2[0] == i1);
	ASSERT_TRUE(occs2[1] == i3);
}

TEST(HandleFreeList, OccupiedIteratorReverse)
{
	lcs::HandleFreeList<TestHandle> list{};
	const TestHandle i1 = list.GetNextHandle();
	const TestHandle i2 = list.GetNextHandle();
	const TestHandle i3 = list.GetNextHandle();

	std::vector<TestHandle> occs1{};
	for (auto it = std::rbegin(list); it != std::rend(list); ++it)
	{
		auto i = *it;
		occs1.push_back(i);
	}
	ASSERT_TRUE(occs1.size() == 3);
	ASSERT_TRUE(occs1[2] == i1);
	ASSERT_TRUE(occs1[1] == i2);
	ASSERT_TRUE(occs1[0] == i3);

	list.FreeHandle(i2);
	std::vector<TestHandle> occs2{};
	for (auto it = std::rbegin(list); it != std::rend(list); ++it)
	{
		auto i = *it;
		occs2.push_back(i);
	}
	ASSERT_TRUE(occs2.size() == 2);
	ASSERT_TRUE(occs2[1] == i1);
	ASSERT_TRUE(occs2[0] == i3);
}

TEST(HandleFreeList, ReusedHandleIsOccupied)
{
	lcs::HandleFreeList<TestHandle> list{};
	const TestHandle before = list.GetNextHandle();
	list.FreeHandle(before);

	const TestHandle after = list.GetNextHandle();

	ASSERT_TRUE(list.IsOccupied(after.GetIndex()));
}

TEST(HandleFreeList, ReusedHandleCanBeFreed)
{
	lcs::HandleFreeList<TestHandle> list{};
	const TestHandle before = list.GetNextHandle();
	list.FreeHandle(before);
	const TestHandle after = list.GetNextHandle();

	list.FreeHandle(after);

	ASSERT_TRUE(!list.IsOccupied(after.GetIndex()));
	ASSERT_TRUE(list.UsedIndexCount() == 0);
}

TEST(HandleFreeList, OccupiedIteratorIncludesReusedHandles)
{
	lcs::HandleFreeList<TestHandle> list{};
	const TestHandle i1 = list.GetNextHandle();
	const TestHandle i2_before = list.GetNextHandle();
	list.FreeHandle(i2_before);
	const TestHandle i2_after = list.GetNextHandle();

	std::vector<TestHandle> occupied{};
	for (const auto& i : list)
	{
		occupied.push_back(i);
	}

	ASSERT_TRUE(occupied.size() == 2);
	ASSERT_TRUE(occupied[0] == i1);
	ASSERT_TRUE(occupied[1] == i2_after);
}

TEST(HandleFreeList, Empty)
{
	lcs::HandleFreeList<TestHandle> list{};
	ASSERT_TRUE(list.MaxIndex() == 0);
	for (auto i : list) {}
}

/* A handle that has been through a free and a reuse, so its validation id is not 0. */
inline TestHandle ReusedHandle(lcs::HandleFreeList<TestHandle>& list)
{
	const TestHandle first = list.GetNextHandle();
	list.FreeHandle(first);
	return list.GetNextHandle();
}

inline std::vector<TestHandle> Occupied(lcs::HandleFreeList<TestHandle>& list)
{
	std::vector<TestHandle> occupied{};
	for (const auto& i : list)
	{
		occupied.push_back(i);
	}
	return occupied;
}

TEST(HandleFreeList, RestoreOccupiesExactlyTheGivenHandles)
{
	lcs::HandleFreeList<TestHandle> source{};
	const TestHandle reused = ReusedHandle(source);
	ASSERT_TRUE(reused.GetValidationID() != 0);

	const std::vector<TestHandle> ids{ TestHandle::CreateNew(0), TestHandle::Create(reused.GetValidationID(), 2), TestHandle::CreateNew(5) };
	lcs::HandleFreeList<TestHandle> list{};
	list.Restore(ids);

	ASSERT_TRUE(list.MaxIndex() == 6);
	ASSERT_TRUE(list.UsedIndexCount() == 3);
	ASSERT_TRUE(list.IsOccupied(0));
	ASSERT_TRUE(!list.IsOccupied(1));
	ASSERT_TRUE(list.IsOccupied(2));
	ASSERT_TRUE(!list.IsOccupied(3));
	ASSERT_TRUE(!list.IsOccupied(4));
	ASSERT_TRUE(list.IsOccupied(5));
	ASSERT_TRUE(Occupied(list) == ids);
}

TEST(HandleFreeList, RestoredGapsAreReusedLowestFirstThenTheListGrows)
{
	lcs::HandleFreeList<TestHandle> list{};
	const std::vector<TestHandle> ids{ TestHandle::CreateNew(0), TestHandle::CreateNew(2), TestHandle::CreateNew(5) };
	list.Restore(ids);

	ASSERT_TRUE(list.GetNextHandle().GetIndex() == 1);
	ASSERT_TRUE(list.GetNextHandle().GetIndex() == 3);
	ASSERT_TRUE(list.GetNextHandle().GetIndex() == 4);
	ASSERT_TRUE(list.GetNextHandle().GetIndex() == 6);
	ASSERT_TRUE(list.UsedIndexCount() == 7);
	ASSERT_TRUE(list.MaxIndex() == 7);
}

TEST(HandleFreeList, RestoredHandleKeepsItsValidationIdAndCanBeFreedAndReused)
{
	lcs::HandleFreeList<TestHandle> source{};
	const TestHandle reused = ReusedHandle(source);

	lcs::HandleFreeList<TestHandle> list{};
	list.Restore(std::vector<TestHandle>{ reused });
	ASSERT_TRUE(Occupied(list).size() == 1);
	ASSERT_TRUE(Occupied(list)[0] == reused);

	list.FreeHandle(reused);
	const TestHandle after = list.GetNextHandle();

	ASSERT_TRUE(after.GetIndex() == reused.GetIndex());
	ASSERT_TRUE(after.GetValidationID() != reused.GetValidationID());
}

TEST(HandleFreeList, RestoreWithNoGapsGrowsFromTheEnd)
{
	lcs::HandleFreeList<TestHandle> list{};
	list.Restore(std::vector<TestHandle>{ TestHandle::CreateNew(0), TestHandle::CreateNew(1), TestHandle::CreateNew(2) });

	ASSERT_TRUE(list.MaxIndex() == 3);
	ASSERT_TRUE(list.GetNextHandle().GetIndex() == 3);
}

TEST(HandleFreeList, RestoreStartingAboveZeroFreesTheIndicesBelow)
{
	lcs::HandleFreeList<TestHandle> list{};
	list.Restore(std::vector<TestHandle>{ TestHandle::CreateNew(3) });

	ASSERT_TRUE(list.UsedIndexCount() == 1);
	ASSERT_TRUE(list.GetNextHandle().GetIndex() == 0);
	ASSERT_TRUE(list.GetNextHandle().GetIndex() == 1);
	ASSERT_TRUE(list.GetNextHandle().GetIndex() == 2);
	ASSERT_TRUE(list.GetNextHandle().GetIndex() == 4);
}

TEST(HandleFreeList, RestoreOfNothingLeavesAnEmptyList)
{
	lcs::HandleFreeList<TestHandle> list{};
	list.Restore({});

	ASSERT_TRUE(list.MaxIndex() == 0);
	ASSERT_TRUE(list.UsedIndexCount() == 0);
	ASSERT_TRUE(Occupied(list).empty());
	ASSERT_TRUE(list.GetNextHandle().GetIndex() == 0);
}

TEST(HandleFreeList, RestoreWorksAgainAfterClear)
{
	lcs::HandleFreeList<TestHandle> list{};
	list.GetNextHandle();
	list.GetNextHandle();
	list.Clear();

	const std::vector<TestHandle> ids{ TestHandle::CreateNew(1), TestHandle::CreateNew(4) };
	list.Restore(ids);

	ASSERT_TRUE(Occupied(list) == ids);
	ASSERT_TRUE(list.GetNextHandle().GetIndex() == 0);
}

TEST(HandleFreeList, ARestoredListFreesAndReusesLikeAnyOther)
{
	lcs::HandleFreeList<TestHandle> list{};
	const std::vector<TestHandle> ids{ TestHandle::CreateNew(0), TestHandle::CreateNew(2) };
	list.Restore(ids);

	list.FreeHandle(ids[1]);
	list.FreeHandle(ids[0]);

	/* The last one freed, the one freed before it, the original gap, then growth. */
	ASSERT_TRUE(list.GetNextHandle().GetIndex() == 0);
	ASSERT_TRUE(list.GetNextHandle().GetIndex() == 2);
	ASSERT_TRUE(list.GetNextHandle().GetIndex() == 1);
	ASSERT_TRUE(list.GetNextHandle().GetIndex() == 3);
}

TEST(HandleFreeList, CanBeIteratedThroughAConstReference)
{
	lcs::HandleFreeList<TestHandle> list{};
	const TestHandle i1 = list.GetNextHandle();
	const TestHandle i2 = list.GetNextHandle();
	const TestHandle i3 = list.GetNextHandle();
	list.FreeHandle(i2);

	const lcs::HandleFreeList<TestHandle>& const_list = list;
	std::vector<TestHandle> forward{};
	for (const TestHandle i : const_list)
	{
		forward.push_back(i);
	}
	std::vector<TestHandle> backward{};
	for (auto it = const_list.rbegin(); it != const_list.rend(); ++it)
	{
		backward.push_back(*it);
	}

	ASSERT_TRUE((forward == std::vector<TestHandle>{ i1, i3 }));
	ASSERT_TRUE((backward == std::vector<TestHandle>{ i3, i1 }));
}
