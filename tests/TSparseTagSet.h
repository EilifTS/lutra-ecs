#include <lutra-ecs/SparseTagSet.h>

TEST(SparseTagSet, InsertHasGet)
{
	lcs::SparseTagSet set{};
	set.ReserveSparseSize(103);

	set.Add(100);
	ASSERT_TRUE(set.DenseSize() == 1);
	set.Add(102);
	ASSERT_TRUE(set.DenseSize() == 2);
	set.Add(50);
	ASSERT_TRUE(set.DenseSize() == 3);

	ASSERT_TRUE(!set.Has(0));
	ASSERT_TRUE(!set.Has(10));
	ASSERT_TRUE(!set.Has(1000));
	ASSERT_TRUE(!set.Has(1234557));

	ASSERT_TRUE(set.Has(100));
	ASSERT_TRUE(set.Has(102));
	ASSERT_TRUE(set.Has(50));
}

TEST(SparseTagSet, Remove)
{
	lcs::SparseTagSet set{ };
	set.ReserveSparseSize(56);

	set.Add(33);
	ASSERT_TRUE(set.DenseSize() == 1);
	set.Add(44);
	ASSERT_TRUE(set.DenseSize() == 2);
	set.Add(55);
	ASSERT_TRUE(set.DenseSize() == 3);

	set.Remove(44);
	ASSERT_TRUE(set.DenseSize() == 2);
	ASSERT_TRUE(!set.Has(43));
	ASSERT_TRUE(!set.Has(44));
	ASSERT_TRUE(!set.Has(45));

	ASSERT_TRUE(set.Has(33));
	ASSERT_TRUE(set.Has(55));
}

TEST(SparseTagSet, InsertRemoveInsert)
{
	lcs::SparseTagSet set{ };
	set.ReserveSparseSize(58);

	set.Add(33);
	set.Add(44);
	set.Add(55);
	set.Remove(44);
	set.Remove(55);
	set.Add(55);
	set.Add(56);
	set.Add(57);
	set.Remove(33);
	set.Remove(56);
	set.Remove(57);
	set.Add(33);

	ASSERT_TRUE(set.DenseSize() == 2);
	ASSERT_TRUE(set.Has(55));
	ASSERT_TRUE(set.Has(33));

	ASSERT_TRUE(!set.Has(44));
	ASSERT_TRUE(!set.Has(56));
	ASSERT_TRUE(!set.Has(57));
}

TEST(SparseTagSet, InsertRemoveInsert2)
{
	lcs::SparseTagSet set{ };
	set.ReserveSparseSize(3);

	set.Add(0);
	set.Add(1);
	set.Remove(1);
	set.Add(2);
	set.Add(1);

	ASSERT_TRUE(set.Has(0));
	ASSERT_TRUE(set.Has(1));
	ASSERT_TRUE(set.Has(2));
}

TEST(SparseTagSet, Iteration)
{
	lcs::SparseTagSet set{ };
	set.ReserveSparseSize(16);

	set.Add(5);
	set.Add(10);
	set.Add(15);

	u32 index_sum = 0;
	for (auto it = set.begin(); it != set.end(); ++it)
	{
		index_sum += *it;
	}
	ASSERT_TRUE(index_sum == 30);
}

TEST(SparseTagSet, ClearSparse)
{
	lcs::SparseTagSet set{ };
	set.ReserveSparseSize(16);

	set.Add(5);
	set.Add(10);
	set.Add(15);

	set.Clear();
	ASSERT_TRUE(!set.Has(5));
	ASSERT_TRUE(!set.Has(10));
	ASSERT_TRUE(!set.Has(15));
	ASSERT_TRUE(set.DenseSize() == 0);
}
