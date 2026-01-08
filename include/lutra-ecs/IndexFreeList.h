#pragma once
#include <vector>
#include <cinttypes>
#include <assert.h>

using u32 = uint32_t;

namespace lcs
{
	class IndexFreeList
	{
	public:
		inline IndexFreeList() {};
		inline u32 GetNextIndex();
		inline void FreeIndex(u32 index);
		inline void Clear();

		inline bool IsFree(u32 index) const;
		inline u32 MaxIndex() const;
		inline u32 UsedIndexCount() const;

		class Iterator
		{
		public:
			using iterator_category = std::bidirectional_iterator_tag;
			using value_type = u32;
			using difference_type = std::ptrdiff_t;
			using pointer = u32*;
			using reference = u32&;

			Iterator(const IndexFreeList& owner, u32 it)
				: owner(owner), base_iterator(it)
			{
				if (base_iterator != u32(owner.indices.size()) && owner.IsFree(base_iterator)) (*this)++;
			};

			inline u32 operator*() const { return base_iterator; }
			inline u32* operator->() { return &base_iterator; }

			/* Prefix increment */
			inline Iterator& operator++()
			{
				base_iterator++;
				while (base_iterator != u32(owner.indices.size()) && owner.IsFree(base_iterator))
				{
					base_iterator++;
				}
				return *this;
			}

			inline Iterator& operator--()
			{
				base_iterator--;
				while (base_iterator != u32(-1) && owner.IsFree(base_iterator))
				{
					base_iterator--;
				}
				return *this;
			}

			/* Postfix increment */
			inline Iterator operator++(int) { Iterator tmp = *this; ++(*this); return tmp; }

			friend bool operator== (const Iterator& a, const Iterator& b) { return a.base_iterator == b.base_iterator; };
			friend bool operator!= (const Iterator& a, const Iterator& b) { return a.base_iterator != b.base_iterator; };

		private:
			u32 base_iterator{};
			const IndexFreeList& owner;
		};

		using RIterator = std::reverse_iterator<Iterator>;

		Iterator begin() const
		{
			return Iterator(*this, 0);
		}
		Iterator end() const
		{
			return Iterator(*this, u32(indices.size()));
		}
		RIterator rbegin() const
		{
			return RIterator(Iterator(*this, u32(indices.size())));
		}
		RIterator rend() const
		{
			return RIterator(Iterator(*this, 0));
		}

	private:
		static constexpr u32 is_occupied_value{ ~u32(0) };
		u32 next_free{ 0 };
		u32 used_index_count{ 0 };
		std::vector<u32> indices;
	};

	inline u32 IndexFreeList::GetNextIndex()
	{
		u32 next_index = next_free;
		if (next_index == (u32)indices.size())
		{
			indices.push_back(is_occupied_value);
			next_free++;
		}
		else
		{
			next_free = indices[next_free];
			indices[next_index] = is_occupied_value;
		}
		used_index_count++;
		return next_index;
	}

	inline void IndexFreeList::FreeIndex(u32 index)
	{
		assert(indices[index] == is_occupied_value);
		if (index < next_free)
		{
			indices[index] = next_free;
			next_free = index;
		}
		else
		{
			indices[index] = indices[next_free];
			indices[next_free] = index;
		}
		used_index_count--;
	}

	inline void IndexFreeList::Clear()
	{
		indices.clear();
		next_free = 0;
		used_index_count = 0;
	}

	inline bool IndexFreeList::IsFree(u32 index) const
	{
		if (index < MaxIndex())
		{
			return indices[index] != is_occupied_value;
		}
		return true;
	}

	inline u32 IndexFreeList::MaxIndex() const
	{
		return u32(indices.size());
	}


	inline u32 IndexFreeList::UsedIndexCount() const
	{
		return used_index_count;
	}
}