#pragma once
#include <lutra-ecs/Entity.h>
#include <vector>
#include <assert.h>

namespace lcs
{
	class EntityIDGenerator
	{
	public:
		inline EntityIDGenerator() {};
		inline EntityID GetNextEntityID();
		inline void FreeEntityID(EntityID id);
		inline void Clear();

		inline bool IsFree(EntityID id) const;
		inline uint32_t MaxIndex() const;
		inline size_t UsedIndexCount() const;

		template <bool reverse>
		class OccupiedIndicesContainer
		{
		public:
			OccupiedIndicesContainer(const EntityIDGenerator& owner) : owner(owner) {};

			class const_iterator
			{
			public:
				const_iterator(const EntityIDGenerator& owner, EntityID it)
					: owner(owner), base_iterator(it)
				{
					if constexpr (!reverse)
					{
						if (base_iterator != EntityID(owner.indices.size()) && owner.IsFree(base_iterator)) (*this)++;
					}
					else
					{
						if (base_iterator != EntityID(-1) && owner.IsFree(base_iterator)) (*this)++;
					}
				};

				inline uint32_t operator*() const { return base_iterator; }
				//inline std::vector<u32>::const_iterator operator->() { return base_iterator; }

				/* Prefix increment */
				inline const_iterator& operator++()
				{
					if constexpr (!reverse)
					{
						base_iterator++;
						while (base_iterator != EntityID(owner.indices.size()) && owner.IsFree(base_iterator))
						{
							base_iterator++;
						}
						return *this;
					}
					else
					{
						base_iterator--;
						while (base_iterator != EntityID(-1) && owner.IsFree(base_iterator))
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
				EntityID base_iterator{};
				const EntityIDGenerator& owner;
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
					return const_iterator(owner, EntityID(owner.indices.size()) - 1);
				}
			}
			const_iterator end() const
			{
				if constexpr (!reverse)
				{
					return const_iterator(owner, EntityID(owner.indices.size()));
				}
				else
				{
					return const_iterator(owner, EntityID(-1));
				}
			}

		private:
			const EntityIDGenerator& owner;
		};

		inline const OccupiedIndicesContainer<false> OccupiedIndices() const { return OccupiedIndicesContainer<false>(*this); };
		inline const OccupiedIndicesContainer<true> OccupiedIndicesReverse() const { return OccupiedIndicesContainer<true>(*this); };

	private:
		static constexpr uint32_t is_occupied_value{ ~uint32_t(0) };
		uint32_t next_free{ 0 };
		uint32_t used_index_count{ 0 };
		std::vector<EntityID> indices;
	};

	inline EntityID EntityIDGenerator::GetNextEntityID()
	{
		uint32_t next_index = next_free;
		if (next_index == uint32_t(indices.size()))
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

	inline void EntityIDGenerator::FreeEntityID(EntityID id)
	{
		assert(indices[id] == is_occupied_value);
		if (id < next_free)
		{
			indices[id] = next_free;
			next_free = id;
		}
		else
		{
			indices[id] = indices[next_free];
			indices[next_free] = id;
		}
		used_index_count--;
	}

	inline void EntityIDGenerator::Clear()
	{
		indices.clear();
		next_free = 0;
		used_index_count = 0;
	}

	inline bool EntityIDGenerator::IsFree(EntityID id) const
	{
		if (id < MaxIndex())
		{
			return indices[id] != is_occupied_value;
		}
		return true;
	}

	inline uint32_t EntityIDGenerator::MaxIndex() const
	{
		return uint32_t(indices.size());
	}


	inline size_t EntityIDGenerator::UsedIndexCount() const
	{
		return used_index_count;
	}
}