#pragma once
#include <gtest/gtest.h>

#include <cstring>
#include <string>
#include <vector>

#include <lutra-ecs/save/ByteIO.h>
#include <lutra-ecs/save/Crc32.h>

/**
 *  The bytes underneath a save: the writer and reader that agree on a layout, a
 *  reader that is safe against bytes nobody vouched for, and the checksum.
 */
namespace TByteIO
{
	using lcs::save::ByteReader;
	using lcs::save::ByteWriter;
	using lcs::save::ErrorKind;

	struct Mixed
	{
		float x;
		uint32_t y;
		uint8_t z[4];
	};

	std::span<const std::byte> BytesOf(const std::string_view text)
	{
		return std::as_bytes(std::span(text.data(), text.size()));
	}

	TEST(ByteIO, IntegersAreLittleEndian)
	{
		ByteWriter writer;
		writer.U32(0x01020304u);

		ASSERT_EQ(writer.Size(), 4u);
		EXPECT_EQ(uint8_t(writer.View()[0]), 0x04);
		EXPECT_EQ(uint8_t(writer.View()[1]), 0x03);
		EXPECT_EQ(uint8_t(writer.View()[2]), 0x02);
		EXPECT_EQ(uint8_t(writer.View()[3]), 0x01);
	}

	TEST(ByteIO, ValuesComeBackAsTheyWentIn)
	{
		ByteWriter writer;
		writer.U32(7);
		writer.U64(0x1122334455667788ull);
		writer.String("tree");
		writer.String("");
		writer.Raw(Mixed{ 1.5f, 9, { 1, 2, 3, 4 } });

		ByteReader reader(writer.View());
		EXPECT_EQ(*reader.U32(), 7u);
		EXPECT_EQ(*reader.U64(), 0x1122334455667788ull);
		EXPECT_EQ(*reader.String(100), "tree");
		EXPECT_EQ(*reader.String(100), "");
		const auto mixed = reader.Raw<Mixed>();
		ASSERT_TRUE(mixed.has_value());
		EXPECT_FLOAT_EQ(mixed->x, 1.5f);
		EXPECT_EQ(mixed->y, 9u);
		EXPECT_EQ(mixed->z[3], 4);
		EXPECT_TRUE(reader.AtEnd());
	}

	TEST(ByteIO, AnArrayOfRecordsIsWrittenBackToBack)
	{
		const std::vector<uint32_t> values{ 1, 2, 3 };
		ByteWriter writer;
		writer.RawArray<uint32_t>(values);

		EXPECT_EQ(writer.Size(), 12u);
		ByteReader reader(writer.View());
		EXPECT_EQ(*reader.U32(), 1u);
		EXPECT_EQ(*reader.U32(), 2u);
		EXPECT_EQ(*reader.U32(), 3u);
	}

	TEST(ByteIO, ReadingPastTheEndIsAnErrorAtEveryLength)
	{
		ByteWriter writer;
		writer.U32(1);
		writer.U64(2);

		for (size_t length = 0; length < 4; length++)
		{
			ByteReader reader(writer.View().first(length));
			const auto value = reader.U32();
			ASSERT_FALSE(value.has_value());
			EXPECT_EQ(value.error().kind, ErrorKind::Truncated);
		}

		ByteReader reader(writer.View().first(8));
		EXPECT_TRUE(reader.U32().has_value());
		EXPECT_FALSE(reader.U64().has_value());
	}

	TEST(ByteIO, AFailedReadConsumesNothing)
	{
		ByteWriter writer;
		writer.U32(5);
		ByteReader reader(writer.View());

		EXPECT_FALSE(reader.Bytes(5).has_value());

		EXPECT_EQ(reader.Remaining(), 4u);
		EXPECT_EQ(*reader.U32(), 5u);
	}

	TEST(ByteIO, ANameLongerThanTheLimitIsRejected)
	{
		ByteWriter writer;
		writer.String(std::string(50, 'a'));

		ByteReader reader(writer.View());
		const auto name = reader.String(49);

		ASSERT_FALSE(name.has_value());
		EXPECT_EQ(name.error().kind, ErrorKind::TooLong);
	}

	TEST(ByteIO, ANameAtTheLimitIsAccepted)
	{
		ByteWriter writer;
		writer.String(std::string(50, 'a'));

		ByteReader reader(writer.View());

		EXPECT_EQ(reader.String(50)->size(), 50u);
	}

	/* The length says far more than there is; it must not be believed. */
	TEST(ByteIO, ANameThatClaimsMoreThanTheFileHoldsIsTruncated)
	{
		ByteWriter writer;
		writer.U32(1000);
		writer.Bytes(BytesOf("short"));

		ByteReader reader(writer.View());
		const auto name = reader.String(4096);

		ASSERT_FALSE(name.has_value());
		EXPECT_EQ(name.error().kind, ErrorKind::Truncated);
	}

	TEST(ByteIO, ACountThatFitsIsReturned)
	{
		ByteWriter writer;
		writer.U32(3);
		writer.RawArray<uint32_t>(std::vector<uint32_t>{ 1, 2, 3 });

		ByteReader reader(writer.View());

		EXPECT_EQ(*reader.Count(4), 3u);
	}

	TEST(ByteIO, ACountOfFourBillionIsRejectedBeforeAnyoneAllocatesForIt)
	{
		ByteWriter writer;
		writer.U32(0xFFFFFFFFu);
		writer.U32(1);

		ByteReader reader(writer.View());
		const auto count = reader.Count(4);

		ASSERT_FALSE(count.has_value());
		EXPECT_EQ(count.error().kind, ErrorKind::BadCount);
	}

	TEST(ByteIO, ACountOneTooLargeIsRejectedAndTheExactFitIsNot)
	{
		ByteWriter writer;
		writer.U32(4);
		writer.RawArray<uint32_t>(std::vector<uint32_t>{ 1, 2, 3 });
		EXPECT_FALSE(ByteReader(writer.View()).Count(4).has_value());

		ByteWriter exact;
		exact.U32(3);
		exact.RawArray<uint32_t>(std::vector<uint32_t>{ 1, 2, 3 });
		EXPECT_TRUE(ByteReader(exact.View()).Count(4).has_value());
	}

	/* A multiplication that wraps would turn a huge count into a small one. */
	TEST(ByteIO, ACountTimesAHugeElementSizeDoesNotWrap)
	{
		ByteWriter writer;
		writer.U32(0x10000001u);
		writer.U32(1);

		const auto count = ByteReader(writer.View()).Count(16);

		ASSERT_FALSE(count.has_value());
		EXPECT_EQ(count.error().kind, ErrorKind::BadCount);
	}

	TEST(Crc32, MatchesTheStandardCheckValue)
	{
		EXPECT_EQ(lcs::save::Crc32(BytesOf("123456789")), 0xCBF43926u);
	}

	TEST(Crc32, OfNothingIsZero)
	{
		EXPECT_EQ(lcs::save::Crc32({}), 0u);
	}

	TEST(Crc32, ChangesWhenASingleBitDoes)
	{
		std::vector<std::byte> bytes(64, std::byte{ 0x5A });
		const uint32_t before = lcs::save::Crc32(bytes);

		for (size_t i = 0; i < bytes.size(); i++)
		{
			bytes[i] ^= std::byte{ 0x01 };
			EXPECT_NE(lcs::save::Crc32(bytes), before) << "byte " << i;
			bytes[i] ^= std::byte{ 0x01 };
		}
	}
}
