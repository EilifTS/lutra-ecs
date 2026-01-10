#pragma once
#include <lutra-ecs/Handle.h>
#include <lutra-ecs/BitMask.h>
#include <algorithm>
#include <cassert>
#include <utility>
#include <array>
#include <vector>

namespace lcs
{
	template <typename handle_t, typename T>
	class SparseSetChunked
	{
	public:
		using data_t = handle_t::data_t;
		using InverseHandlesChunk = std::array<handle_t, 64>;
		using Chunk = std::array<T, 64>;

		SparseSetChunked() {};

		inline void Add(handle_t handle, T&& data);
		inline T& Get(handle_t handle);
		inline const T& Get(handle_t handle) const { return Get(handle); };
		inline void Remove(handle_t handle);
		inline void RemoveIfPresent(handle_t handle);
		inline bool Has(handle_t handle) const;

		inline data_t SparseSize() const { return data_t(chunk_indices.size() * 64); };
		inline data_t DenseSize() const { return data_t(chunks.size()) * 64; };

		inline void ReserveSparseSize(data_t new_size);
		inline void Clear();

		class Iterator
		{
		public:
			inline T& operator*() const { return owner.chunks[chunk_index][*occ_it]; }
			inline T* operator->() { return &(owner.chunks[chunk_index][*occ_it]); }

			inline handle_t GetOwner() const { return owner.inverse_handle_chunks[chunk_index][*occ_it]; };

			inline Iterator& operator++() 
			{
				++occ_it;
				if (occ_it.IsZero())
				{
					chunk_index++;
					if (chunk_index < owner.chunks.size())
					{
						occ_it = BitMask<uint64_t>::Iterator::Create(owner.occupancy_masks[chunk_index]);
					}
				}
				return *this;
			}

			inline Iterator operator++(int)
			{
				Iterator tmp = *this; ++(*this); return tmp;
			}

			friend bool operator== (const Iterator& a, const Iterator& b)
			{
				return (a.chunk_index == b.chunk_index) && (a.occ_it == b.occ_it);
			};
			friend bool operator!= (const Iterator& a, const Iterator& b)
			{
				return (a.chunk_index != b.chunk_index) || (a.occ_it != b.occ_it);
			};
		private:
			inline Iterator(SparseSetChunked& owner, data_t chunk_index) 
				: chunk_index(chunk_index), occ_it(occ_it), owner(owner)
			{
				if (owner.occupancy_masks.size() > 0 && chunk_index < owner.occupancy_masks.size())
				{
					occ_it = BitMask<uint64_t>::Iterator::Create(owner.occupancy_masks[chunk_index]);
				}
				else
				{
					occ_it = BitMask<uint64_t>::Iterator::Create({});
				}
			}
			data_t chunk_index{};
			BitMask<uint64_t>::Iterator occ_it{};
			SparseSetChunked& owner;

			friend class SparseSetChunked;
		};
		inline Iterator begin() { return Iterator(*this, 0); };
		inline Iterator end() { return Iterator(*this, chunks.size()); };


	private:
		constexpr static data_t invalid_index{ data_t(-1) };

		std::vector<data_t> chunk_indices{};
		std::vector<BitMask<uint64_t>> occupancy_masks{};
		std::vector<InverseHandlesChunk> inverse_handle_chunks{};
		std::vector<Chunk> chunks{};
	};

	template <typename handle_t, typename T>
	void SparseSetChunked<handle_t, T>::Add(handle_t handle, T&& data)
	{
		assert(!Has(handle));
		const auto handle_index = handle.GetIndex();

		auto chunk_index = chunk_indices[handle_index / 64];
		if (chunk_index == invalid_index)
		{
			/* Add new chunk */
			chunk_index = chunks.size();
			chunk_indices[handle_index / 64] = chunk_index;
			occupancy_masks.push_back({ 0 });
			inverse_handle_chunks.push_back({});
			chunks.push_back({});
		}

		BitMask<uint64_t>& occupancy_mask = occupancy_masks[chunk_index];
		InverseHandlesChunk& inverse_handle_chunk = inverse_handle_chunks[chunk_index];
		Chunk& chunk = chunks[chunk_index];

		const auto data_index = handle.GetIndex() % 64;
		occupancy_mask.SetBit(uint8_t(data_index));
		inverse_handle_chunk[data_index] = handle;
		chunk[data_index] = data;
	}

	template <typename handle_t, typename T>
	T& SparseSetChunked<handle_t, T>::Get(handle_t handle)
	{
		assert(Has(handle));

		const auto handle_index = handle.GetIndex();
		const auto chunk_index = chunk_indices[handle_index / 64];
		const auto data_index = handle_index % 64;
		return chunks[chunk_index][data_index];
	}

	template <typename handle_t, typename T>
	void SparseSetChunked<handle_t, T>::Remove(handle_t handle)
	{
		assert(Has(handle));

		const auto handle_index = handle.GetIndex();
		const auto chunk_index = chunk_indices[handle_index / 64];
		const auto data_index = handle_index % 64;


		BitMask<uint64_t>& occupancy_mask = occupancy_masks[chunk_index];
		occupancy_mask.ClearBit(uint8_t(data_index));

		if (occupancy_mask.IsZero())
		{
			/* Remove the chunk */
			assert(chunks.size() > 0);
			const auto back_chunk_index = typename handle_t::data_t(chunks.size()) - 1;
			const auto back_index = inverse_handle_chunks[back_chunk_index][data_index].GetIndex();

			if (back_chunk_index != chunk_index)
			{
				std::iter_swap(occupancy_masks.begin() + chunk_index, occupancy_masks.begin() + back_chunk_index);
				std::iter_swap(inverse_handle_chunks.begin() + chunk_index, inverse_handle_chunks.begin() + back_chunk_index);
				std::iter_swap(chunks.begin() + chunk_index, chunks.begin() + back_chunk_index);
			}
			occupancy_masks.pop_back();
			inverse_handle_chunks.pop_back();
			chunks.pop_back();

			chunk_indices[back_index] = chunk_index;
			chunk_indices[handle_index] = invalid_index;
		}
	}

	template <typename handle_t, typename T>
	void SparseSetChunked<handle_t, T>::RemoveIfPresent(handle_t handle)
	{
		if (Has(handle)) Remove(handle);
	}

	template <typename handle_t, typename T>
	bool SparseSetChunked<handle_t, T>::Has(handle_t handle) const
	{
		const auto handle_index = handle.GetIndex();
		assert(handle_index / 64 < chunk_indices.size());

		const auto chunk_index = chunk_indices[handle_index / 64];
		if (chunk_index == invalid_index) return false;

		const auto data_index = handle_index % 64;
		const auto occupancy_mask = occupancy_masks[chunk_index];
		if (!occupancy_mask.IsBitSet(uint8_t(data_index))) return false;

		const InverseHandlesChunk& inverse_handle_chunk = inverse_handle_chunks[chunk_index];
		assert(inverse_handle_chunk[data_index].GetValidationID() == handle.GetValidationID()); /* Check for stale handle */
		return true;
	}

	template <typename handle_t, typename T>
	inline void SparseSetChunked<handle_t, T>::ReserveSparseSize(handle_t::data_t new_size)
	{
		assert(new_size > SparseSize());
		const auto new_size_aligned = (new_size / 64) + 1;
		chunk_indices.resize(size_t(new_size_aligned), invalid_index);
	}

	template <typename handle_t, typename T>
	inline void SparseSetChunked<handle_t, T>::Clear()
	{
		chunk_indices.clear();
		occupancy_masks.clear();
		inverse_handle_chunks.clear();
		chunks.clear();
	}
}