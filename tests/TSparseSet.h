#include <gtest/gtest.h>

#include <lutra-ecs/SparseSet.h>
#include <lutra-ecs/SparseSetChunked.h>

using TestHandle = lcs::Handle<uint32_t, 16>;
using u64 = uint64_t;

TestHandle ch(uint32_t validation_id, uint32_t value) { return TestHandle::Create(validation_id << TestHandle::index_bits, value); }
TestHandle cnh(uint32_t value) { return TestHandle::CreateNew(value); }

template <typename SetType>
void TestInsertHasGet()
{
	SetType set{};
	set.ReserveSparseSize(103);

	set.Add(cnh(100), 1);
	ASSERT_TRUE(set.DenseSize() >= 1);
	set.Add(cnh(102), 2);
	ASSERT_TRUE(set.DenseSize() >= 2);
	set.Add(cnh(50), 3);
	ASSERT_TRUE(set.DenseSize() >= 3);

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

template <typename SetType>
void TestRemove()
{
	lcs::SparseSet<TestHandle, u64> set{};
	set.ReserveSparseSize(56);

	set.Add(cnh(33), 1);
	ASSERT_TRUE(set.DenseSize() >= 1);
	set.Add(cnh(44), 2);
	ASSERT_TRUE(set.DenseSize() >= 2);
	set.Add(cnh(55), 3);
	ASSERT_TRUE(set.DenseSize() >= 3);

	set.Remove(cnh(44));
	ASSERT_TRUE(set.DenseSize() >= 2);
	ASSERT_TRUE(!set.Has(cnh(43)));
	ASSERT_TRUE(!set.Has(cnh(44)));
	ASSERT_TRUE(!set.Has(cnh(45)));

	ASSERT_TRUE(set.Has(cnh(33)));
	ASSERT_TRUE(set.Has(cnh(55)));
}

template <typename SetType>
void TestInsertRemoveInsert()
{
	lcs::SparseSet<TestHandle, u64> set{};
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

	ASSERT_TRUE(set.DenseSize() >= 2);
	ASSERT_TRUE(set.Has(cnh(55)));
	ASSERT_TRUE(set.Has(cnh(33)));
	ASSERT_TRUE(set.Get(cnh(33)) == 100);
	ASSERT_TRUE(set.Get(cnh(55)) == 10);

	ASSERT_TRUE(!set.Has(cnh(44)));
	ASSERT_TRUE(!set.Has(cnh(56)));
	ASSERT_TRUE(!set.Has(cnh(57)));
}

template <typename SetType>
void TestInsertRemoveInsert2()
{
	lcs::SparseSet<TestHandle, u64> set{};
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

template <typename SetType>
void TestIteration()
{
	lcs::SparseSet<TestHandle, u64> set{};
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

template <typename SetType>
void TestClearSparse()
{
	lcs::SparseSet<TestHandle, u64> set{};
	set.ReserveSparseSize(16);

	set.Add(cnh(5), 1);
	set.Add(cnh(10), 2);
	set.Add(cnh(15), 3);

	set.Clear();
	ASSERT_TRUE(set.DenseSize() == 0);
}

TEST(SparseSet, TestInsertHasGet) { TestInsertHasGet<lcs::SparseSet<TestHandle, u64>>(); }
TEST(SparseSet, TestRemove) { TestRemove<lcs::SparseSet<TestHandle, u64>>(); }
TEST(SparseSet, TestInsertRemoveInsert) { TestInsertRemoveInsert<lcs::SparseSet<TestHandle, u64>>(); }
TEST(SparseSet, TestInsertRemoveInsert2) { TestInsertRemoveInsert2<lcs::SparseSet<TestHandle, u64>>(); }
TEST(SparseSet, TestIteration) { TestIteration<lcs::SparseSet<TestHandle, u64>>(); }
TEST(SparseSet, TestClearSparse) { TestClearSparse<lcs::SparseSet<TestHandle, u64>>(); }

TEST(SparseSetChunked, TestInsertHasGet) { TestInsertHasGet<lcs::SparseSetChunked<TestHandle, u64>>(); }
TEST(SparseSetChunked, TestRemove) { TestRemove<lcs::SparseSetChunked<TestHandle, u64>>(); }
TEST(SparseSetChunked, TestInsertRemoveInsert) { TestInsertRemoveInsert<lcs::SparseSetChunked<TestHandle, u64>>(); }
TEST(SparseSetChunked, TestInsertRemoveInsert2) { TestInsertRemoveInsert2<lcs::SparseSetChunked<TestHandle, u64>>(); }
TEST(SparseSetChunked, TestIteration) { TestIteration<lcs::SparseSetChunked<TestHandle, u64>>(); }
TEST(SparseSetChunked, TestClearSparse) { TestClearSparse<lcs::SparseSetChunked<TestHandle, u64>>(); }