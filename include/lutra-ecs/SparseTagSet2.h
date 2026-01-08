#pragma once
#include <lutra-ecs/Handle.h>
#include <vector>
#include <algorithm>
#include <assert.h>

namespace lcs
{
	template <typename handle_t>
	class SparseTagSet2
	{
	public:
		using data_t = handle_t::data_t;

		inline SparseTagSet2() {};

		inline void Add(handle_t handle);

		inline void Remove(handle_t handle);
		inline void RemoveIfPresent(handle_t handle);
		inline bool Has(handle_t handle) const;

		inline data_t SparseSize() const { return (u32)sparse_indices.size(); };
		inline data_t DenseSize() const { return (u32)inverse_list.size(); };

		inline void ReserveSparseSize(data_t new_size);
		inline void Clear();

		using Iterator = std::vector<handle_t>::iterator;
		
		inline Iterator begin() { return inverse_list.begin(); };

		inline Iterator end() { return inverse_list.end(); };

	private:
		constexpr static data_t invalid_index{ data_t(-1) };

		inline void assertValidInputHandle(handle_t handle) const;

		std::vector<data_t> sparse_indices;
		std::vector<handle_t> inverse_list;
	};

	/* Templated version for specific tags */
	template <typename T, typename handle_t>
	class SparseTagSet2T : public SparseTagSet2<handle_t>
	{

	};

	template <typename handle_t>
	void SparseTagSet2<handle_t>::Add(handle_t handle)
	{
		const auto handle_index = handle.GetIndex();
		assert(handle_index < SparseSize()); /* Invalid index or missing reservation */
		assert(sparse_indices[handle_index] == invalid_index);

		inverse_list.push_back(handle);
		sparse_indices[handle_index] = DenseSize() - 1;
	}

	template <typename handle_t>
	void SparseTagSet2<handle_t>::Remove(handle_t handle)
	{
		assertValidInputHandle(handle);
		const auto handle_index = handle.GetIndex();
		const auto back_index = inverse_list.back().GetIndex();
		const auto dense_index = sparse_indices[handle_index];
		const auto dense_back_index = sparse_indices[back_index];

		if (dense_back_index != dense_index)
		{
			std::iter_swap(inverse_list.begin() + dense_index, inverse_list.begin() + dense_back_index);
		}
		inverse_list.pop_back();

		sparse_indices[back_index] = dense_index;
		sparse_indices[handle_index] = invalid_index;
	}

	template <typename handle_t>
	void SparseTagSet2<handle_t>::RemoveIfPresent(handle_t handle)
	{
		if (Has(handle)) Remove(handle);
	}

	template <typename handle_t>
	bool SparseTagSet2<handle_t>::Has(handle_t handle) const
	{
		const auto handle_index = handle.GetIndex();
		assert(handle_index < SparseSize());
		if (sparse_indices[handle_index] == invalid_index) return false;
		assert(inverse_list[sparse_indices[handle_index]].GetValidationID() == handle.GetValidationID()); /* Check for stale handle */
		return true;
	}

	template <typename handle_t>
	inline void SparseTagSet2<handle_t>::ReserveSparseSize(handle_t::data_t new_size)
	{
		assert(new_size > SparseSize());
		sparse_indices.resize(size_t(new_size), invalid_index);
	}

	template <typename handle_t>
	inline void SparseTagSet2<handle_t>::Clear()
	{
		sparse_indices.clear();
		inverse_list.clear();
	}

	template <typename handle_t>
	void SparseTagSet2<handle_t>::assertValidInputHandle(handle_t handle) const
	{
		const auto handle_index = handle.GetIndex();
		assert(handle_index < SparseSize());
		assert(sparse_indices[handle_index] != invalid_index);
		assert(sparse_indices[handle_index] < DenseSize());
		assert(inverse_list[sparse_indices[handle_index]].GetValidationID() == handle.GetValidationID()); /* Check for stale handle */
	}
}