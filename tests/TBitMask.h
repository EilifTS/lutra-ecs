#pragma once
#include <gtest/gtest.h>
#include <lutra-ecs/BitMask.h>

TEST(BitMask, SetBit)
{
	lcs::BitMask<uint64_t> mask{};

	mask.SetBit(0);
	ASSERT_EQ(mask.mask, 0x0000000000000001);

	mask.SetBit(1);
	ASSERT_EQ(mask.mask, 0x0000000000000003);

	mask.SetBit(7);
	ASSERT_EQ(mask.mask, 0x0000000000000083);

	mask.SetBit(15);
	ASSERT_EQ(mask.mask, 0x0000000000008083);

	mask.SetBit(31);
	ASSERT_EQ(mask.mask, 0x0000000080008083);

	mask.SetBit(63);
	ASSERT_EQ(mask.mask, 0x8000000080008083);
}

TEST(BitMask, ClearBit)
{
	lcs::BitMask<uint64_t> mask{ 0x8000000080008083 };

	mask.ClearBit(0);
	ASSERT_EQ(mask.mask, 0x8000000080008082);

	mask.ClearBit(1);
	ASSERT_EQ(mask.mask, 0x8000000080008080);

	mask.ClearBit(7);
	ASSERT_EQ(mask.mask, 0x8000000080008000);

	mask.ClearBit(15);
	ASSERT_EQ(mask.mask, 0x8000000080000000);

	mask.ClearBit(31);
	ASSERT_EQ(mask.mask, 0x8000000000000000);

	mask.ClearBit(63);
	ASSERT_EQ(mask.mask, 0x0000000000000000);
}

TEST(BitMask, Iterate)
{
	auto extract_bits = [](lcs::BitMask<uint64_t> mask) -> std::vector<uint8_t> {
		std::vector<uint8_t> bits{};
		for (uint8_t bit : mask)
		{
			bits.push_back(bit);
		}
		return bits;
	};

	const auto bits0 = extract_bits({ 0x0000000000000000 });
	ASSERT_EQ(bits0.size(), 0);

	const auto bits1 = extract_bits({ 0x0000000000000001 });
	ASSERT_EQ(bits1.size(), 1);
	ASSERT_EQ(bits1[0], 0);

	const auto bits2 = extract_bits({ 0x8000000000000000 });
	ASSERT_EQ(bits2.size(), 1);
	ASSERT_EQ(bits2[0], 63);

	const auto bits3 = extract_bits({ 0x8000000080008083 });
	ASSERT_EQ(bits3.size(), 6);
	ASSERT_EQ(bits3[0], 0);
	ASSERT_EQ(bits3[1], 1);
	ASSERT_EQ(bits3[2], 7);
	ASSERT_EQ(bits3[3], 15);
	ASSERT_EQ(bits3[4], 31);
	ASSERT_EQ(bits3[5], 63);
}

TEST(BitMask, IterateReverse)
{
	auto extract_bits = [](lcs::BitMask<uint64_t> mask) -> std::vector<uint8_t> {
		std::vector<uint8_t> bits{};
		for (auto it = mask.rbegin(); it != mask.rend(); ++it)
		{
			bits.push_back(*it);
		}
		return bits;
	};

	const auto bits0 = extract_bits({ 0x0000000000000000 });
	ASSERT_EQ(bits0.size(), 0);

	const auto bits1 = extract_bits({ 0x0000000000000001 });
	ASSERT_EQ(bits1.size(), 1);
	ASSERT_EQ(bits1[0], 0);

	const auto bits2 = extract_bits({ 0x8000000000000000 });
	ASSERT_EQ(bits2.size(), 1);
	ASSERT_EQ(bits2[0], 63);

	const auto bits3 = extract_bits({ 0x8000000080008083 });
	ASSERT_EQ(bits3.size(), 6);
	ASSERT_EQ(bits3[0], 63);
	ASSERT_EQ(bits3[1], 31);
	ASSERT_EQ(bits3[2], 15);
	ASSERT_EQ(bits3[3], 7);
	ASSERT_EQ(bits3[4], 1);
	ASSERT_EQ(bits3[5], 0);
}