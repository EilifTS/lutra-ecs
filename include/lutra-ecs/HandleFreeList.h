#pragma once
#include <lutra-ecs/Handle.h>

#include <span>
#include <vector>
#include <assert.h>

namespace lcs
{
	template <typename handle_t>
	class HandleFreeList
	{
	public:
		using data_t = handle_t::data_t;
		using handle_container_t = std::vector<handle_t>;

		inline HandleFreeList() {};
		inline handle_t GetNextHandle()
		{
			used_index_count++;

			const data_t handle_index = next_free_index;
			if (next_free_index == data_t(handles.size()))
			{
				next_free_index++;
				handles.push_back(handle_t::CreateNew(is_occupied_index));
				return handle_t::CreateNew(handle_index);
			}

			const data_t old_handle_validation_id = handles[handle_index].GetValidationID();
			next_free_index = handles[handle_index].GetIndex();
			handles[handle_index] = handle_t::CreateNext(old_handle_validation_id, is_occupied_index);
			return handle_t::CreateNext(old_handle_validation_id, handle_index);

		}
		inline void FreeHandle(handle_t handle)
		{
			const data_t handle_index = handle.GetIndex();
			assert(IsOccupied(handle_index));
			assert(handles[handle_index].GetValidationID() == handle.GetValidationID());
			handles[handle_index] = handle_t::Create(handle.GetValidationID(), next_free_index);
			next_free_index = handle_index;
			used_index_count--;
		}
		inline void Clear()
		{
			handles.clear();
			next_free_index = 0;
			used_index_count = 0;
		}

		/* Makes `ids` exactly the occupied handles, validation ids included, so a
		 * handle that was saved compares equal to the one restored. For loading
		 * only: the list must be empty (new, or Clear()ed), and `ids` must be
		 * valid and strictly ascending by index.
		 *
		 * Every index below the last one that is not in `ids` becomes a free slot,
		 * chained in ascending order, so GetNextHandle fills the gaps before it
		 * grows. A gap starts over at validation id 0: nothing refers to what used
		 * to be there.
		 *
		 * This takes the whole set at once because a free slot's link to "the next
		 * fresh index" is the list's own size. Adding slots one by one would move
		 * that index under the chain. */
		inline void Restore(std::span<const handle_t> ids)
		{
			assert(handles.empty() && "Restore needs an empty list");
			for (size_t n = 0; n < ids.size(); n++)
			{
				assert(ids[n].IsValid());
				assert(ids[n].GetIndex() < is_occupied_index);
				assert(n == 0 || ids[n - 1].GetIndex() < ids[n].GetIndex());
			}

			const data_t size = ids.empty() ? data_t(0) : ids.back().GetIndex() + 1;
			handles.resize(size);

			/* From the back, so each free slot can link to the next free one. */
			data_t next_free = size;
			auto id = ids.rbegin();
			for (data_t index = size; index-- > 0; )
			{
				if (id != ids.rend() && id->GetIndex() == index)
				{
					handles[index] = handle_t::Create(id->GetValidationID(), is_occupied_index);
					++id;
				}
				else
				{
					handles[index] = handle_t::Create(0, next_free);
					next_free = index;
				}
			}

			next_free_index = next_free;
			used_index_count = data_t(ids.size());
		}

		inline bool IsOccupied(data_t handle_index) const
		{
			assert(handle_index < MaxIndex());
			return handles[handle_index].GetIndex() == is_occupied_index;
		}

		/* Whether this exact handle is in use: its index is occupied and it carries the
		 * validation id the index has now, so the id of an entity that was destroyed (and
		 * of one whose index has been reused since) is not alive. The invalid handle never is. */
		[[nodiscard]] inline bool IsAlive(handle_t handle) const
		{
			if (!handle.IsValid() || handle.GetIndex() >= MaxIndex())
			{
				return false;
			}
			return IsOccupied(handle.GetIndex()) && handles[handle.GetIndex()].GetValidationID() == handle.GetValidationID();
		}

		inline data_t MaxIndex() const
		{
			return data_t(handles.size());
		}
		inline data_t UsedIndexCount() const
		{
			return used_index_count;
		}

		class Iterator
		{
		public:
			using iterator_category = std::bidirectional_iterator_tag;
			using value_type = handle_t;
			using difference_type = std::ptrdiff_t;
			using pointer = handle_t*;
			using reference = handle_t&;

			Iterator(const HandleFreeList& owner, handle_container_t::const_iterator it)
				: owner(owner), base_iterator(it)
			{
				if ((base_iterator != std::end(owner.handles)) && (it->GetIndex() != is_occupied_index)) ++(*this);
			};

			inline handle_t operator*() const {
				const data_t handle_index = data_t(std::distance(std::begin(owner.handles), base_iterator));
				return handle_t::Create(base_iterator->GetValidationID(), handle_index);
			}

			inline Iterator& operator++()
			{
				++base_iterator;
				while ((base_iterator != std::end(owner.handles)) && (base_iterator->GetIndex() != is_occupied_index))
				{
					++base_iterator;
				}
				return *this;
			}

			inline Iterator& operator--()
			{
				--base_iterator;
				while ((base_iterator != std::begin(owner.handles)) && (base_iterator->GetIndex() != is_occupied_index))
				{
					--base_iterator;
				}
				return *this;
			}

			inline Iterator operator++(int) { Iterator tmp = *this; ++(*this); return tmp; }
			inline Iterator operator--(int) { Iterator tmp = *this; --(*this); return tmp; }

			friend bool operator== (const Iterator& a, const Iterator& b) { return a.base_iterator == b.base_iterator; };
			friend bool operator!= (const Iterator& a, const Iterator& b) { return a.base_iterator != b.base_iterator; };

		private:
			handle_container_t::const_iterator base_iterator{};
			const HandleFreeList& owner;
		};

		using RIterator = std::reverse_iterator<Iterator>;

		Iterator begin() const
		{
			return Iterator(*this, std::begin(handles));
		}
		Iterator end() const
		{
			return Iterator(*this, std::end(handles));
		}
		RIterator rbegin() const
		{
			return RIterator(Iterator(*this, std::end(handles)));
		}
		RIterator rend() const
		{
			return RIterator(Iterator(*this, std::begin(handles)));
		}

	private:
		static constexpr data_t is_occupied_index = handle_t().GetIndex();
		data_t next_free_index{ 0 };
		data_t used_index_count{ 0 };
		std::vector<handle_t> handles{};
	};
}