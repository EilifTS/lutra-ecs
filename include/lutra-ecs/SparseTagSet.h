#pragma once
#include <lutra-ecs/Entity.h>
#include <vector>
#include <algorithm>
#include <assert.h>

namespace lcs
{
	class SparseTagSet
	{
	public:
		inline SparseTagSet() {};

		inline void Add(EntityID id);

		inline void Remove(EntityID id);
		inline void RemoveIfPresent(EntityID id);
		inline bool Has(EntityID id) const;

		inline uint32_t Size() const { return denseSize(); };

		inline void Clear();

		using Iterator = std::vector<EntityID>::iterator;
		
		inline Iterator begin() { return inverse_list.begin(); };

		inline Iterator end() { return inverse_list.end(); };

	private:
		constexpr static uint32_t invalid_index{ uint32_t(-1) };

		inline uint32_t sparseSize() const { return uint32_t(sparse_indices.size()); };
		inline uint32_t denseSize() const { return uint32_t(inverse_list.size()); };
		inline bool isValidInputIndex(EntityID id) const;
		inline void inflateSparseSet(uint32_t new_size);

		std::vector<uint32_t> sparse_indices;
		std::vector<EntityID> inverse_list;
	};

	/* Templated version for specific tags */
	/* TODO: Should be possible to remove this */
	template <typename T>
	class SparseTagSetT : public SparseTagSet
	{

	};

	void SparseTagSet::Add(EntityID id)
	{
		if (id >= sparseSize())
		{
			inflateSparseSet(id + 1);
		}
		assert(sparse_indices[id] == invalid_index);

		inverse_list.push_back(id);
		sparse_indices[id] = denseSize() - 1;
	}

	void SparseTagSet::Remove(EntityID id)
	{
		assert(isValidInputIndex(id));
		uint32_t back_index = inverse_list.back();
		uint32_t dense_index = sparse_indices[id];
		uint32_t dense_back_index = sparse_indices[back_index];

		if (dense_back_index != dense_index)
		{
			std::iter_swap(inverse_list.begin() + dense_index, inverse_list.begin() + dense_back_index);
		}
		inverse_list.pop_back();

		sparse_indices[back_index] = dense_index;
		sparse_indices[id] = invalid_index;
	}

	void SparseTagSet::RemoveIfPresent(EntityID id)
	{
		if (Has(id)) Remove(id);
	}

	bool SparseTagSet::Has(EntityID id) const
	{
		if (id >= sparseSize()) return false;
		if (sparse_indices[id] == invalid_index) return false;
		return true;
	}

	inline void SparseTagSet::Clear()
	{
		sparse_indices.clear();
		inverse_list.clear();
	}

	bool SparseTagSet::isValidInputIndex(EntityID id) const
	{
		if (id >= sparseSize()) return false;
		if (sparse_indices[id] == invalid_index) return false;
		if (sparse_indices[id] >= denseSize()) return false;
		return true;
	}

	void SparseTagSet::inflateSparseSet(uint32_t new_size)
	{
		assert(new_size > sparseSize());
		while (sparseSize() < new_size)
		{
			sparse_indices.push_back(invalid_index);
		}
	}
}