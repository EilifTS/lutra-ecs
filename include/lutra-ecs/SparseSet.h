#pragma once
#include <lutra-ecs/Entity.h>
#include <algorithm>
#include <vector>

namespace lcs
{
	template <typename T>
	class SparseSet
	{
	public:
		SparseSet() {};

		inline void Add(EntityID id, T&& data);

		inline T& Get(EntityID id);

		inline const T& Get(EntityID id) const { return Get(id); };

		inline void Remove(EntityID id);
		inline void RemoveIfPresent(EntityID id);
		inline bool Has(EntityID id) const;

		inline uint32_t Size() const { return denseSize(); };

		inline void Clear();

		template <bool reverse>
		class IteratorInternal
		{
		public:
			using IteratorType = T;

			inline IteratorType& operator*() const { return owner.dense_data[dense_index]; }
			inline IteratorType* operator->() { return &(owner.dense_data[dense_index]); }

			inline EntityID GetOwner() const { return owner.inverse_list[dense_index]; };

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
			inline IteratorInternal(SparseSet& owner, uint32_t dense_index) : dense_index(dense_index), owner(owner) {}
			uint32_t dense_index;
			SparseSet& owner;

			friend class SparseSet;
		};

		using Iterator = IteratorInternal<false>;
		using RIterator = IteratorInternal<true>;

		inline Iterator begin() { return Iterator(*this, 0); };

		inline Iterator end() { return Iterator(*this, Size()); };

		inline RIterator rbegin() { return RIterator(*this, Size() - 1); };

		inline RIterator rend() { return RIterator(*this, -1); };

	private:
		constexpr static uint32_t invalid_index{ uint32_t(-1) };

		inline uint32_t sparseSize() const { return uint32_t(sparse_indices.size()); };
		inline uint32_t denseSize() const { return uint32_t(dense_data.size()); };
		inline bool isValidInputIndex(EntityID id) const;
		inline void inflateSparseSet(size_t new_size);

		std::vector<uint32_t> sparse_indices;
		std::vector<EntityID> inverse_list;
		std::vector<T> dense_data;
	};

	template <typename T>
	void SparseSet<T>::Add(EntityID id, T&& data)
	{
		if (id >= sparseSize())
		{
			inflateSparseSet(id + 1);
		}
		assert(sparse_indices[id] == invalid_index);

		dense_data.push_back(data);
		inverse_list.push_back(id);
		sparse_indices[id] = denseSize() - 1;
	}

	template <typename T>
	T& SparseSet<T>::Get(EntityID id)
	{
		assert(isValidInputIndex(id));
		return dense_data[sparse_indices[id]];
	}

	template <typename T>
	void SparseSet<T>::Remove(EntityID id)
	{
		assert(isValidInputIndex(id));
		uint32_t back_index = inverse_list.back();
		uint32_t dense_index = sparse_indices[id];
		uint32_t dense_back_index = sparse_indices[back_index];

		if (dense_back_index != dense_index)
		{
			std::iter_swap(dense_data.begin() + dense_index, dense_data.begin() + dense_back_index);
			std::iter_swap(inverse_list.begin() + dense_index, inverse_list.begin() + dense_back_index);
		}
		dense_data.pop_back();
		inverse_list.pop_back();

		sparse_indices[back_index] = dense_index;
		sparse_indices[id] = invalid_index;
	}

	template <typename T>
	void SparseSet<T>::RemoveIfPresent(EntityID id)
	{
		if (Has(id)) Remove(id);
	}

	template <typename T>
	bool SparseSet<T>::Has(EntityID id) const
	{
		if (id >= sparseSize()) return false;
		if (sparse_indices[id] == invalid_index) return false;
		return true;
	}

	template <typename T>
	inline void SparseSet<T>::Clear()
	{
		sparse_indices.clear();
		inverse_list.clear();
		dense_data.clear();
	}

	template <typename T>
	bool SparseSet<T>::isValidInputIndex(EntityID id) const
	{
		if (id >= sparseSize()) return false;
		if (sparse_indices[id] == invalid_index) return false;
		if (sparse_indices[id] >= denseSize()) return false;
		return true;
	}

	template <typename T>
	void SparseSet<T>::inflateSparseSet(size_t new_size)
	{
		assert(new_size > sparseSize());
		while (sparseSize() < new_size)
		{
			sparse_indices.push_back(invalid_index);
		}
	}

}