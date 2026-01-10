#pragma once
#include <cassert>
#include <cinttypes>
#include <bit>

namespace lcs
{
	template <typename T>
	struct BitMask
	{
		static constexpr uint8_t bit_count = sizeof(T) * 8;

		inline void SetBit(uint8_t bit)
		{
			assert(bit < bit_count);
			mask |= T(1) << T(bit);
		}

		inline void ClearBit(uint8_t bit)
		{
			assert(bit < bit_count);
			mask &= ~(T(1) << T(bit));
		}

		class Iterator
		{
		public:
			inline Iterator(BitMask mask) : mask(mask)
			{
				current_index = uint8_t(std::countr_zero(mask.mask));
			}

			inline uint8_t operator*() const { return current_index; }
			inline uint8_t* operator->() { return &current_index; }

			inline Iterator& operator++() {
				mask.ClearBit(current_index);
				current_index = std::countr_zero(mask.mask);
				return *this;
			}

			inline Iterator operator++(int)
			{
				Iterator tmp = *this; ++(*this); return tmp;
			}

			friend bool operator== (const Iterator& a, const Iterator& b) { return a.mask.mask == b.mask.mask; };
			friend bool operator!= (const Iterator& a, const Iterator& b) { return a.mask.mask != b.mask.mask; };
		private:
			uint8_t current_index{};
			BitMask mask{};
		};

		inline Iterator begin() { return Iterator(*this); };
		inline Iterator end() { return Iterator(BitMask(T(0))); };

		T mask{};
	};
}