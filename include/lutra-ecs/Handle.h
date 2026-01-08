#pragma once
#include <cassert>
#include <cinttypes>

namespace lcs
{
	template <typename T, uint32_t vb>
	struct Handle
	{
		using data_t = T;
		
		static constexpr uint32_t handle_bits = sizeof(data_t) * 8;
		static constexpr uint32_t validation_bits = vb;

		static constexpr uint32_t index_bits = handle_bits - validation_bits;
		static constexpr data_t max_index = (data_t(1) << index_bits) - data_t(1);
		static constexpr data_t index_mask = max_index;

		static constexpr data_t validation_id_increment = data_t(1) << validation_bits;
		static constexpr data_t max_validation_id = (validation_id_increment - data_t(1)) << validation_bits;
		static constexpr data_t validation_id_mask = max_validation_id;

		static constexpr data_t invalid_handle = data_t(-1);

		constexpr data_t GetIndex() const { return handle & index_mask; }
		constexpr data_t GetValidationID() const { return (handle & validation_id_mask); }
		constexpr data_t IsValid() const { return handle != invalid_handle; }

		data_t handle{ invalid_handle };

		static inline Handle CreateNew( data_t index)
		{
			assert(index <= max_index);
			return { index };
		}

		static inline Handle Create(data_t validation_id, data_t index)
		{
			assert((validation_id == 0) || (validation_id > max_index));
			assert(validation_id <= max_validation_id);
			assert(index <= max_index);
			return { validation_id | index };
		}

		static inline Handle CreateNext(data_t validation_id, data_t index)
		{
			assert((validation_id == 0) || (validation_id > max_index));
			assert(validation_id <= max_validation_id);
			assert(index <= max_index);
			const data_t next_validation_id = (validation_id + validation_id_increment) & validation_id_mask;
			return { next_validation_id | index };
		}

		friend inline bool operator== (Handle a, Handle b) { return a.handle == b.handle; };
		friend inline bool operator!= (Handle a, Handle b) { return a.handle != b.handle; };
	};
}