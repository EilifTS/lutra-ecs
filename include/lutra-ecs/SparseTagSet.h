#pragma once
#include <vector>
#include <algorithm>
#include <cinttypes>
#include <assert.h>

using u32 = uint32_t;

namespace lcs
{
	class SparseTagSet
	{
	public:
		inline SparseTagSet() {};

		inline void Add(u32 index);

		inline void Remove(u32 index);
		inline void RemoveIfPresent(u32 index);
		inline bool Has(u32 index) const;

		inline u32 Size() const { return denseSize(); };

		inline void Clear();

		using Iterator = std::vector<u32>::iterator;
		
		inline Iterator begin() { return inverse_list.begin(); };

		inline Iterator end() { return inverse_list.end(); };

	private:
		constexpr static u32 invalid_index{ u32(-1) };

		inline u32 sparseSize() const { return (u32)sparse_indices.size(); };
		inline u32 denseSize() const { return (u32)inverse_list.size(); };
		inline bool isValidInputIndex(u32 index) const;
		inline void inflateSparseSet(u32 new_size);

		std::vector<u32> sparse_indices;
		std::vector<u32> inverse_list;
	};

	/* Templated version for specific tags */
	template <typename T>
	class SparseTagSetT : public SparseTagSet
	{

	};

	void SparseTagSet::Add(u32 index)
	{
		if (index >= sparseSize())
		{
			inflateSparseSet(index + 1);
		}
		assert(sparse_indices[index] == invalid_index);

		inverse_list.push_back(index);
		sparse_indices[index] = denseSize() - 1;
	}

	void SparseTagSet::Remove(u32 index)
	{
		assert(isValidInputIndex(index));
		u32 back_index = inverse_list.back();
		u32 dense_index = sparse_indices[index];
		u32 dense_back_index = sparse_indices[back_index];

		if (dense_back_index != dense_index)
		{
			std::iter_swap(inverse_list.begin() + dense_index, inverse_list.begin() + dense_back_index);
		}
		inverse_list.pop_back();

		sparse_indices[back_index] = dense_index;
		sparse_indices[index] = invalid_index;
	}

	void SparseTagSet::RemoveIfPresent(u32 index)
	{
		if (Has(index)) Remove(index);
	}

	bool SparseTagSet::Has(u32 index) const
	{
		if (index >= sparseSize()) return false;
		if (sparse_indices[index] == invalid_index) return false;
		return true;
	}

	inline void SparseTagSet::Clear()
	{
		sparse_indices.clear();
		inverse_list.clear();
	}

	bool SparseTagSet::isValidInputIndex(u32 index) const
	{
		if (index >= sparseSize()) return false;
		if (sparse_indices[index] == invalid_index) return false;
		if (sparse_indices[index] >= denseSize()) return false;
		return true;
	}

	void SparseTagSet::inflateSparseSet(u32 new_size)
	{
		assert(new_size > sparseSize());
		while (sparseSize() < new_size)
		{
			sparse_indices.push_back(invalid_index);
		}
	}
}