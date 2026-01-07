#pragma once
#include <algorithm>
#include <vector>
#include <cinttypes>
#include <utility>

using u32 = uint32_t;

namespace lcs
{
	template <typename T>
	class SparseSet
	{
	public:
		SparseSet() {};

		inline void Add(u32 index, T&& data);

		inline T& Get(u32 index);

		inline const T& Get(u32 index) const { return Get(index); };

		inline void Remove(u32 index);
		inline void RemoveIfPresent(u32 index);
		inline bool Has(u32 index) const;

		inline u32 SparseSize() const { return u32(sparse_indices.size()); };
		inline u32 DenseSize() const { return u32(dense_data.size()); };

		inline void ReserveSparseSize(u32 new_size);
		inline void Clear();

		template <bool reverse>
		class IteratorInternal
		{
		public:
			using IteratorType = T;

			inline IteratorType& operator*() const { return owner.dense_data[dense_index]; }
			inline IteratorType* operator->() { return &(owner.dense_data[dense_index]); }

			inline u32 GetOwner() const { return owner.inverse_list[dense_index]; };

			inline IteratorInternal& operator++()
			{
				if constexpr (!reverse)
				{
					/* Normal */
					dense_index++; return *this;
				}
				else
				{
					/* Reverse */
					dense_index--; return *this;
				}
			}

			inline IteratorInternal operator++(int)
			{
				IteratorInternal tmp = *this; ++(*this); return tmp;
			}

			friend bool operator== (const IteratorInternal& a, const IteratorInternal& b) { return a.dense_index == b.dense_index; };
			friend bool operator!= (const IteratorInternal& a, const IteratorInternal& b) { return a.dense_index != b.dense_index; };
		private:
			inline IteratorInternal(SparseSet& owner, u32 dense_index) : dense_index(dense_index), owner(owner) {}
			u32 dense_index;
			SparseSet& owner;

			friend class SparseSet;
		};

		using Iterator = IteratorInternal<false>;
		using RIterator = IteratorInternal<true>;

		inline Iterator begin() { return Iterator(*this, 0); };

		inline Iterator end() { return Iterator(*this, DenseSize()); };

		inline RIterator rbegin() { return RIterator(*this, DenseSize() - 1); };

		inline RIterator rend() { return RIterator(*this, -1); };

	private:
		constexpr static u32 invalid_index{ u32(-1) };

		inline bool isValidInputIndex(u32 index) const;

		std::vector<u32> sparse_indices;
		std::vector<u32> inverse_list;
		std::vector<T> dense_data;
	};

	template <typename T>
	void SparseSet<T>::Add(u32 index, T&& data)
	{
		assert(index < SparseSize()); /* Invalid index or missing reservation */
		assert(sparse_indices[index] == invalid_index);

		dense_data.emplace_back(data);
		inverse_list.push_back(index);
		sparse_indices[index] = DenseSize() - 1;
	}

	template <typename T>
	T& SparseSet<T>::Get(u32 index)
	{
		assert(isValidInputIndex(index));
		return dense_data[sparse_indices[index]];
	}

	template <typename T>
	void SparseSet<T>::Remove(u32 index)
	{
		assert(isValidInputIndex(index));
		u32 back_index = inverse_list.back();
		u32 dense_index = sparse_indices[index];
		u32 dense_back_index = sparse_indices[back_index];

		if (dense_back_index != dense_index)
		{
			std::iter_swap(dense_data.begin() + dense_index, dense_data.begin() + dense_back_index);
			std::iter_swap(inverse_list.begin() + dense_index, inverse_list.begin() + dense_back_index);
		}
		dense_data.pop_back();
		inverse_list.pop_back();

		sparse_indices[back_index] = dense_index;
		sparse_indices[index] = invalid_index;
	}

	template <typename T>
	void SparseSet<T>::RemoveIfPresent(u32 index)
	{
		if (Has(index)) Remove(index);
	}

	template <typename T>
	bool SparseSet<T>::Has(u32 index) const
	{
		if (index >= SparseSize()) return false;
		if (sparse_indices[index] == invalid_index) return false;
		return true;
	}

	template <typename T>
	inline void SparseSet<T>::ReserveSparseSize(u32 new_size)
	{
		assert(new_size > SparseSize());
		sparse_indices.resize(size_t(new_size), invalid_index);
	}

	template <typename T>
	inline void SparseSet<T>::Clear()
	{
		sparse_indices.clear();
		inverse_list.clear();
		dense_data.clear();
	}

	template <typename T>
	bool SparseSet<T>::isValidInputIndex(u32 index) const
	{
		if (index >= SparseSize()) return false;
		if (sparse_indices[index] == invalid_index) return false;
		if (sparse_indices[index] >= DenseSize()) return false;
		return true;
	}
}