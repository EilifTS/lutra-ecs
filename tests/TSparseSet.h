#include <gtest/gtest.h>

#include <lutra-ecs/SparseSet.h>

TEST(SparseSet, TestInsertHasGet)
{
	lcs::SparseSet<uint64_t> set{};
	set.Add(100, 1);
	ASSERT_TRUE(set.Size() == 1);
	set.Add(102, 2);
	ASSERT_TRUE(set.Size() == 2);
	set.Add(50, 3);
	ASSERT_TRUE(set.Size() == 3);

	ASSERT_TRUE(!set.Has(0));
	ASSERT_TRUE(!set.Has(10));
	ASSERT_TRUE(!set.Has(1000));
	ASSERT_TRUE(!set.Has(1234557));

	ASSERT_TRUE(set.Has(100));
	ASSERT_TRUE(set.Has(102));
	ASSERT_TRUE(set.Has(50));

	ASSERT_TRUE(set.Get(100) == 1);
	ASSERT_TRUE(set.Get(102) == 2);
	ASSERT_TRUE(set.Get(50) == 3);
}

TEST(SparseSet, TestRemove)
{
	lcs::SparseSet<uint64_t> set{};
	set.Add(33, 1);
	ASSERT_TRUE(set.Size() == 1);
	set.Add(44, 2);
	ASSERT_TRUE(set.Size() == 2);
	set.Add(55, 3);
	ASSERT_TRUE(set.Size() == 3);

	set.Remove(44);
	ASSERT_TRUE(set.Size() == 2);
	ASSERT_TRUE(!set.Has(43));
	ASSERT_TRUE(!set.Has(44));
	ASSERT_TRUE(!set.Has(45));

	ASSERT_TRUE(set.Has(33));
	ASSERT_TRUE(set.Has(55));
}

TEST(SparseSet, TestInsertRemoveInsert)
{
	lcs::SparseSet<uint64_t> set{};
	set.Add(33, 1);
	set.Add(44, 2);
	set.Add(55, 3);
	set.Remove(44);
	set.Remove(55);
	set.Add(55, 10);
	set.Add(56, 11);
	set.Add(57, 12);
	set.Remove(33);
	set.Remove(56);
	set.Remove(57);
	set.Add(33, 100);

	ASSERT_TRUE(set.Size() == 2);
	ASSERT_TRUE(set.Has(55));
	ASSERT_TRUE(set.Has(33));
	ASSERT_TRUE(set.Get(33) == 100);
	ASSERT_TRUE(set.Get(55) == 10);

	ASSERT_TRUE(!set.Has(44));
	ASSERT_TRUE(!set.Has(56));
	ASSERT_TRUE(!set.Has(57));
}

TEST(SparseSet, TestInsertRemoveInsert2)
{
	lcs::SparseSet<uint64_t> set{};
	set.Add(0, 0);
	set.Add(1, 1);
	set.Remove(1);
	set.Add(2, 2);
	set.Add(1, 3);

	ASSERT_TRUE(set.Has(0));
	ASSERT_TRUE(set.Has(1));
	ASSERT_TRUE(set.Has(2));
	ASSERT_TRUE(set.Get(0) == 0);
	ASSERT_TRUE(set.Get(1) == 3);
	ASSERT_TRUE(set.Get(2) == 2);
}

TEST(SparseSet, TestIteration)
{
	lcs::SparseSet<uint64_t> set{};
	set.Add(5, 1);
	set.Add(10, 2);
	set.Add(15, 3);

	uint32_t index_sum = 0;
	uint64_t data_sum = 0;
	for (auto it = set.begin(); it != set.end(); ++it)
	{
		index_sum += it.GetOwner();
		data_sum += *it;
	}
	ASSERT_TRUE(index_sum == 30);
	ASSERT_TRUE(data_sum == 6);
}

TEST(SparseSet, TestIterationReverse)
{
	lcs::SparseSet<uint64_t> set{};
	set.Add(5, 1);
	set.Add(10, 2);
	set.Add(15, 3);

	std::vector<uint64_t> values{};
	for (auto it = set.rbegin(); it != set.rend(); ++it)
	{
		values.push_back(*it);
	}
	ASSERT_TRUE(values[0] == 3);
	ASSERT_TRUE(values[1] == 2);
	ASSERT_TRUE(values[2] == 1);
}

TEST(SparseSet, TestClearSparse)
{
	lcs::SparseSet<uint64_t> set{};
	set.Add(5, 1);
	set.Add(10, 2);
	set.Add(15, 3);

	set.Clear();
	ASSERT_TRUE(!set.Has(5));
	ASSERT_TRUE(!set.Has(10));
	ASSERT_TRUE(!set.Has(15));
	ASSERT_TRUE(set.Size() == 0);
}