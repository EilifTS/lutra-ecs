#include <gtest/gtest.h>

#include <lutra-ecs/SparseTagSet.h>

using TestHandle = lcs::Handle<uint32_t, 16>;

TEST(SparseTagSet, InsertHasGet)
{
	lcs::SparseTagSet<TestHandle> set{};
	set.ReserveSparseSize(103);

	set.Add(TestHandle::CreateNew(100));
	ASSERT_TRUE(set.DenseSize() == 1);
	set.Add(TestHandle::CreateNew(102));
	ASSERT_TRUE(set.DenseSize() == 2);
	set.Add(TestHandle::CreateNew(50));
	ASSERT_TRUE(set.DenseSize() == 3);

	ASSERT_TRUE(!set.Has(TestHandle::CreateNew(0)));
	ASSERT_TRUE(!set.Has(TestHandle::CreateNew(10)));
	ASSERT_TRUE(!set.Has(TestHandle::CreateNew(77)));
	ASSERT_TRUE(!set.Has(TestHandle::CreateNew(44)));

	ASSERT_TRUE(set.Has(TestHandle::CreateNew(100)));
	ASSERT_TRUE(set.Has(TestHandle::CreateNew(102)));
	ASSERT_TRUE(set.Has(TestHandle::CreateNew(50)));
}

TEST(SparseTagSet, Remove)
{
	lcs::SparseTagSet<TestHandle> set{ };
	set.ReserveSparseSize(56);

	set.Add(TestHandle::CreateNew(33));
	ASSERT_TRUE(set.DenseSize() == 1);
	set.Add(TestHandle::Create(12 << TestHandle::validation_bits, 44));
	ASSERT_TRUE(set.DenseSize() == 2);
	set.Add(TestHandle::CreateNew(55));
	ASSERT_TRUE(set.DenseSize() == 3);

	set.Remove(TestHandle::Create(12 << TestHandle::validation_bits, 44));
	ASSERT_TRUE(set.DenseSize() == 2);
	ASSERT_TRUE(!set.Has(TestHandle::CreateNew(43)));
	ASSERT_TRUE(!set.Has(TestHandle::Create(12 << TestHandle::validation_bits, 44)));
	ASSERT_TRUE(!set.Has(TestHandle::CreateNew(45)));

	ASSERT_TRUE(set.Has(TestHandle::CreateNew(33)));
	ASSERT_TRUE(set.Has(TestHandle::CreateNew(55)));
}

TEST(SparseTagSet, InsertRemoveInsert)
{
	lcs::SparseTagSet<TestHandle> set{ };
	set.ReserveSparseSize(58);

	set.Add(TestHandle::CreateNew(33));
	set.Add(TestHandle::CreateNew(44));
	set.Add(TestHandle::CreateNew(55));
	set.Remove(TestHandle::CreateNew(44));
	set.Remove(TestHandle::CreateNew(55));
	set.Add(TestHandle::CreateNew(55));
	set.Add(TestHandle::CreateNew(56));
	set.Add(TestHandle::CreateNew(57));
	set.Remove(TestHandle::CreateNew(33));
	set.Remove(TestHandle::CreateNew(56));
	set.Remove(TestHandle::CreateNew(57));
	set.Add(TestHandle::CreateNew(33));

	ASSERT_TRUE(set.DenseSize() == 2);
	ASSERT_TRUE(set.Has(TestHandle::CreateNew(55)));
	ASSERT_TRUE(set.Has(TestHandle::CreateNew(33)));

	ASSERT_TRUE(!set.Has(TestHandle::CreateNew(44)));
	ASSERT_TRUE(!set.Has(TestHandle::CreateNew(56)));
	ASSERT_TRUE(!set.Has(TestHandle::CreateNew(57)));
}

TEST(SparseTagSet, InsertRemoveInsert2)
{
	lcs::SparseTagSet<TestHandle> set{ };
	set.ReserveSparseSize(3);

	set.Add(TestHandle::CreateNew(0));
	set.Add(TestHandle::CreateNew(1));
	set.Remove(TestHandle::CreateNew(1));
	set.Add(TestHandle::CreateNew(2));
	set.Add(TestHandle::CreateNew(1));

	ASSERT_TRUE(set.Has(TestHandle::CreateNew(0)));
	ASSERT_TRUE(set.Has(TestHandle::CreateNew(1)));
	ASSERT_TRUE(set.Has(TestHandle::CreateNew(2)));
}

TEST(SparseTagSet, Iteration)
{
	lcs::SparseTagSet<TestHandle> set{ };
	set.ReserveSparseSize(16);

	set.Add(TestHandle::CreateNew(5));
	set.Add(TestHandle::CreateNew(10));
	set.Add(TestHandle::CreateNew(15));

	uint32_t index_sum = 0;
	for (auto it = set.begin(); it != set.end(); ++it)
	{
		index_sum += it->GetIndex();
	}
	ASSERT_TRUE(index_sum == 30);
}

TEST(SparseTagSet, ClearSparse)
{
	lcs::SparseTagSet<TestHandle> set{ };
	set.ReserveSparseSize(16);

	set.Add(TestHandle::CreateNew(5));
	set.Add(TestHandle::CreateNew(10));
	set.Add(TestHandle::CreateNew(15));

	set.Clear();
	ASSERT_TRUE(set.DenseSize() == 0);
}
