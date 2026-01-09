#pragma once
#include <lutra-ecs/Handle.h>
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

	private:
		constexpr static data_t invalid_index{ data_t(-1) };

		std::vector<data_t> chunk_indices{};
		std::vector<uint64_t> occupancy_masks{};
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
			occupancy_masks.push_back(0);
			inverse_handle_chunks.push_back({});
			chunks.push_back({});
		}

		uint64_t& occupancy_mask = occupancy_masks[chunk_index];
		InverseHandlesChunk& inverse_handle_chunk = inverse_handle_chunks[chunk_index];
		Chunk& chunk = chunks[chunk_index];

		const auto data_index = handle.GetIndex() % 64;
		occupancy_mask |= (uint64_t(1) << data_index);
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


		uint64_t& occupancy_mask = occupancy_masks[chunk_index];
		occupancy_mask &= ~(uint64_t(1) << data_index);

		if (occupancy_mask == 0)
		{
			/* Remove the chunk */
			assert(chunks.size() > 0);
			const auto back_chunk_index = handle_t::data_t(chunks.size()) - 1;
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
		if ((occupancy_mask & (uint64_t(1) << data_index)) == 0) return false;

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