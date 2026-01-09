#pragma once
#include <lutra-ecs/Handle.h>
#include <algorithm>
#include <cassert>
#include <utility>
#include <vector>

namespace lcs
{
	template <typename handle_t, typename T>
	class SparseSet2
	{
	public:
		using data_t = handle_t::data_t;

		SparseSet2() {};

		inline void Add(handle_t handle, T&& data);
		inline T& Get(handle_t handle);
		inline const T& Get(handle_t handle) const { return Get(handle); };
		inline void Remove(handle_t handle);
		inline void RemoveIfPresent(handle_t handle);
		inline bool Has(handle_t handle) const;

		inline data_t SparseSize() const { return data_t(sparse_indices.size()); };
		inline data_t DenseSize() const { return data_t(dense_data.size()); };

		inline void ReserveSparseSize(data_t new_size);
		inline void Clear();

		class Iterator
		{
		public:
			using iterator_category = std::bidirectional_iterator_tag;
			using value_type = T;
			using difference_type = std::ptrdiff_t;
			using pointer = T*;
			using reference = T&;

			inline T& operator*() const { return owner.dense_data[dense_index]; }
			inline T* operator->() { return &(owner.dense_data[dense_index]); }

			inline handle_t GetOwner() const { return owner.inverse_list[dense_index]; };

			inline Iterator& operator++() { dense_index++; return *this; }
			inline Iterator& operator--() { dense_index--; return *this; }

			inline Iterator operator++(int)
			{
				Iterator tmp = *this; ++(*this); return tmp;
			}
			inline Iterator operator--(int)
			{
				Iterator tmp = *this; --(*this); return tmp;
			}

			friend bool operator== (const Iterator& a, const Iterator& b) { return a.dense_index == b.dense_index; };
			friend bool operator!= (const Iterator& a, const Iterator& b) { return a.dense_index != b.dense_index; };
		private:
			inline Iterator(SparseSet2& owner, u32 dense_index) : dense_index(dense_index), owner(owner) {}
			data_t dense_index;
			SparseSet2& owner;

			friend class SparseSet2;
		};

		using RIterator = std::reverse_iterator<Iterator>;

		inline Iterator begin() { return Iterator(*this, 0); };

		inline Iterator end() { return Iterator(*this, DenseSize()); };

		inline RIterator rbegin() { return RIterator(Iterator(*this, DenseSize())); };

		inline RIterator rend() { return RIterator(Iterator(*this, 0)); };

	private:
		constexpr static data_t invalid_index{ data_t(-1) };

		inline void assertValidInputHandle(handle_t handle) const;

		std::vector<data_t> sparse_indices;
		std::vector<handle_t> inverse_list;
		std::vector<T> dense_data;
	};

	template <typename handle_t, typename T>
	void SparseSet2<handle_t, T>::Add(handle_t handle, T&& data)
	{
		const auto handle_index = handle.GetIndex();
		assert(handle_index < SparseSize()); /* Invalid index or missing reservation */
		assert(sparse_indices[handle_index] == invalid_index);

		dense_data.emplace_back(data);
		inverse_list.push_back(handle);
		sparse_indices[handle_index] = DenseSize() - 1;
	}

	template <typename handle_t, typename T>
	T& SparseSet2<handle_t, T>::Get(handle_t handle)
	{
		assertValidInputHandle(handle);
		return dense_data[sparse_indices[handle.GetIndex()]];
	}

	template <typename handle_t, typename T>
	void SparseSet2<handle_t, T>::Remove(handle_t handle)
	{
		assertValidInputHandle(handle);
		const auto handle_index = handle.GetIndex();
		const auto back_index = inverse_list.back().GetIndex();
		const auto dense_index = sparse_indices[handle_index];
		const auto dense_back_index = sparse_indices[back_index];

		if (dense_back_index != dense_index)
		{
			std::iter_swap(dense_data.begin() + dense_index, dense_data.begin() + dense_back_index);
			std::iter_swap(inverse_list.begin() + dense_index, inverse_list.begin() + dense_back_index);
		}
		dense_data.pop_back();
		inverse_list.pop_back();

		sparse_indices[back_index] = dense_index;
		sparse_indices[handle_index] = invalid_index;
	}

	template <typename handle_t, typename T>
	void SparseSet2<handle_t, T>::RemoveIfPresent(handle_t handle)
	{
		if (Has(handle)) Remove(handle);
	}

	template <typename handle_t, typename T>
	bool SparseSet2<handle_t, T>::Has(handle_t handle) const
	{
		const auto handle_index = handle.GetIndex();
		assert(handle_index < SparseSize());
		if (sparse_indices[handle_index] == invalid_index) return false;
		assert(inverse_list[sparse_indices[handle_index]].GetValidationID() == handle.GetValidationID()); /* Check for stale handle */
		return true;
	}

	template <typename handle_t, typename T>
	inline void SparseSet2<handle_t, T>::ReserveSparseSize(handle_t::data_t new_size)
	{
		assert(new_size > SparseSize());
		sparse_indices.resize(size_t(new_size), invalid_index);
	}

	template <typename handle_t, typename T>
	inline void SparseSet2<handle_t, T>::Clear()
	{
		sparse_indices.clear();
		inverse_list.clear();
		dense_data.clear();
	}

	template <typename handle_t, typename T>
	inline void SparseSet2<handle_t, T>::assertValidInputHandle(handle_t handle) const
	{
		const auto handle_index = handle.GetIndex();
		assert(handle_index < SparseSize());
		assert(sparse_indices[handle_index] != invalid_index);
		assert(sparse_indices[handle_index] < DenseSize());
		assert(inverse_list[sparse_indices[handle_index]].GetValidationID() == handle.GetValidationID()); /* Check for stale handle */
	}
}