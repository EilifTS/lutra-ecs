#pragma once
#include <vector>
#include <cinttypes>
#include <assert.h>

using u32 = uint32_t;

namespace ef
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

		template <bool reverse>
		class OccupiedIndicesContainer
		{
		public:
			OccupiedIndicesContainer(const IndexFreeList& owner) : owner(owner) {};

			class const_iterator
			{
			public:
				const_iterator(const IndexFreeList& owner, u32 it)
					: owner(owner), base_iterator(it)
				{
					if constexpr (!reverse)
					{
						if (base_iterator != u32(owner.indices.size()) && owner.IsFree(base_iterator)) (*this)++;
					}
					else
					{
						if (base_iterator != u32(-1) && owner.IsFree(base_iterator)) (*this)++;
					}
				};

				inline u32 operator*() const { return base_iterator; }
				//inline std::vector<u32>::const_iterator operator->() { return base_iterator; }

				/* Prefix increment */
				inline const_iterator& operator++()
				{
					if constexpr (!reverse)
					{
						base_iterator++;
						while (base_iterator != u32(owner.indices.size()) && owner.IsFree(base_iterator))
						{
							base_iterator++;
						}
						return *this;
					}
					else
					{
						base_iterator--;
						while (base_iterator != u32(-1) && owner.IsFree(base_iterator))
						{
							base_iterator--;
						}
						return *this;
					}
				}

				/* Postfix increment */
				inline const_iterator operator++(int) { const_iterator tmp = *this; ++(*this); return tmp; }

				friend bool operator== (const const_iterator& a, const const_iterator& b) { return a.base_iterator == b.base_iterator; };
				friend bool operator!= (const const_iterator& a, const const_iterator& b) { return a.base_iterator != b.base_iterator; };

			private:
				u32 base_iterator{};
				const IndexFreeList& owner;
			};

			using const_iterator = const_iterator;

			const_iterator begin() const
			{
				if constexpr (!reverse)
				{
					return const_iterator(owner, 0);
				}
				else
				{
					return const_iterator(owner, u32(owner.indices.size()) - 1);
				}
			}
			const_iterator end() const
			{
				if constexpr (!reverse)
				{
					return const_iterator(owner, u32(owner.indices.size()));
				}
				else
				{
					return const_iterator(owner, u32(-1));
				}
			}

		private:
			const IndexFreeList& owner;
		};

		inline const OccupiedIndicesContainer<false> OccupiedIndices() const { return OccupiedIndicesContainer<false>(*this); };
		inline const OccupiedIndicesContainer<true> OccupiedIndicesReverse() const { return OccupiedIndicesContainer<true>(*this); };

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