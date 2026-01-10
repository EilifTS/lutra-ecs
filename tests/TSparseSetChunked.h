#include <gtest/gtest.h>

#include <lutra-ecs/SparseSetChunked.h>

using TestHandle = lcs::Handle<uint32_t, 16>;
using u64 = uint64_t;

TEST(SparseSetChunked, TestInsertHasGet)
{
	lcs::SparseSetChunked<TestHandle, u64> set{};
	set.ReserveSparseSize(103);

	set.Add(cnh(100), 1);
	ASSERT_TRUE(set.DenseSize() == 64);
	set.Add(cnh(102), 2);
	ASSERT_TRUE(set.DenseSize() == 64);
	set.Add(cnh(50), 3);
	ASSERT_TRUE(set.DenseSize() == 128);

	ASSERT_TRUE(!set.Has(cnh(0)));
	ASSERT_TRUE(!set.Has(cnh(10)));
	ASSERT_TRUE(!set.Has(cnh(55)));
	ASSERT_TRUE(!set.Has(cnh(66)));

	ASSERT_TRUE(set.Has(cnh(100)));
	ASSERT_TRUE(set.Has(cnh(102)));
	ASSERT_TRUE(set.Has(cnh(50)));

	ASSERT_TRUE(set.Get(cnh(100)) == 1);
	ASSERT_TRUE(set.Get(cnh(102)) == 2);
	ASSERT_TRUE(set.Get(cnh(50)) == 3);
}

TEST(SparseSetChunked, TestRemove)
{
	lcs::SparseSetChunked<TestHandle, u64> set{};
	set.ReserveSparseSize(56);

	set.Add(cnh(33), 1);
	ASSERT_TRUE(set.DenseSize() == 64);
	set.Add(cnh(44), 2);
	ASSERT_TRUE(set.DenseSize() == 64);
	set.Add(cnh(55), 3);
	ASSERT_TRUE(set.DenseSize() == 64);

	set.Remove(cnh(44));
	ASSERT_TRUE(set.DenseSize() == 64);
	ASSERT_TRUE(!set.Has(cnh(43)));
	ASSERT_TRUE(!set.Has(cnh(44)));
	ASSERT_TRUE(!set.Has(cnh(45)));

	ASSERT_TRUE(set.Has(cnh(33)));
	ASSERT_TRUE(set.Has(cnh(55)));
}

TEST(SparseSetChunked, TestInsertRemoveInsert)
{
	lcs::SparseSetChunked<TestHandle, u64> set{};
	set.ReserveSparseSize(58);

	set.Add(cnh(33), 1);
	set.Add(cnh(44), 2);
	set.Add(cnh(55), 3);
	set.Remove(cnh(44));
	set.Remove(cnh(55));
	set.Add(cnh(55), 10);
	set.Add(cnh(56), 11);
	set.Add(cnh(57), 12);
	set.Remove(cnh(33));
	set.Remove(cnh(56));
	set.Remove(cnh(57));
	set.Add(cnh(33), 100);

	ASSERT_TRUE(set.DenseSize() == 64);
	ASSERT_TRUE(set.Has(cnh(55)));
	ASSERT_TRUE(set.Has(cnh(33)));
	ASSERT_TRUE(set.Get(cnh(33)) == 100);
	ASSERT_TRUE(set.Get(cnh(55)) == 10);

	ASSERT_TRUE(!set.Has(cnh(44)));
	ASSERT_TRUE(!set.Has(cnh(56)));
	ASSERT_TRUE(!set.Has(cnh(57)));
}

TEST(SparseSetChunked, TestInsertRemoveInsert2)
{
	lcs::SparseSetChunked<TestHandle, u64> set{};
	set.ReserveSparseSize(3);

	set.Add(cnh(0), 0);
	set.Add(cnh(1), 1);
	set.Remove(cnh(1));
	set.Add(cnh(2), 2);
	set.Add(cnh(1), 3);

	ASSERT_TRUE(set.Has(cnh(0)));
	ASSERT_TRUE(set.Has(cnh(1)));
	ASSERT_TRUE(set.Has(cnh(2)));
	ASSERT_TRUE(set.Get(cnh(0)) == 0);
	ASSERT_TRUE(set.Get(cnh(1)) == 3);
	ASSERT_TRUE(set.Get(cnh(2)) == 2);
}

TEST(SparseSetChunked, TestIteration)
{
	lcs::SparseSetChunked<TestHandle, u64> set{};
	set.ReserveSparseSize(16);

	set.Add(cnh(5), 1);
	set.Add(cnh(10), 2);
	set.Add(cnh(15), 3);

	uint32_t index_sum = 0;
	u64 data_sum = 0;
	for (auto it = set.begin(); it != set.end(); ++it)
	{
		index_sum += it.GetOwner().GetIndex();
		data_sum += *it;
	}
	ASSERT_TRUE(index_sum == 30);
	ASSERT_TRUE(data_sum == 6);
}

