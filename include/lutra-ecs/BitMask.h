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

		inline bool IsBitSet(uint8_t bit) const
		{
			return (mask & (T(1) << bit)) != 0;
		}

		inline bool IsZero() const { return mask == T(0); };

		class Iterator
		{
		public:
			inline Iterator(BitMask mask, uint8_t first_index) : mask(mask), current_index(first_index) {};

			inline static Iterator Create(BitMask mask)
			{
				return { mask, uint8_t(std::countr_zero(mask.mask)) };
			}
			inline static Iterator CreateReverse(BitMask mask)
			{
				return { mask, uint8_t(bit_count - uint8_t(std::countl_zero(mask.mask)) - 1) };
			}

			inline uint8_t operator*() const { return current_index; }
			inline uint8_t* operator->() { return &current_index; }

			inline Iterator& operator++() {
				mask.ClearBit(current_index);
				current_index = std::countr_zero(mask.mask);
				return *this;
			}
			inline Iterator& operator--() {
				mask.ClearBit(current_index);
				current_index = uint8_t(bit_count - std::countl_zero(mask.mask) - 1);
				return *this;
			}

			inline Iterator operator++(int)
			{
				Iterator tmp = *this; ++(*this); return tmp;
			}
			inline Iterator operator--(int)
			{
				Iterator tmp = *this; --(*this); return tmp;
			}

			friend bool operator== (const Iterator& a, const Iterator& b) { return a.mask.mask == b.mask.mask; };
			friend bool operator!= (const Iterator& a, const Iterator& b) { return a.mask.mask != b.mask.mask; };
		private:
			BitMask mask{};
			uint8_t current_index{};
		};

		class RIterator
		{
		public:
			RIterator(Iterator it) : it(it) {};

			inline uint8_t operator*() const { return *it; }
			inline Iterator operator->() { return it; }

			inline RIterator& operator++() {
				--it;
				return *this;
			}

			inline RIterator operator++(int)
			{
				RIterator tmp = *this; ++(*this); return tmp;
			}

			friend bool operator== (const RIterator& a, const RIterator& b) { return a.it == b.it; };
			friend bool operator!= (const RIterator& a, const RIterator& b) { return a.it != b.it; };
		private:
			Iterator it{};
		};

		inline Iterator begin() { return Iterator::Create(*this); };
		inline Iterator end() { return Iterator::Create(BitMask(T(0))); };
		inline RIterator rbegin() { return RIterator(Iterator::CreateReverse(*this)); };
		inline RIterator rend() { return RIterator(Iterator::CreateReverse(BitMask(T(0)))); };

		T mask{};
	};
}