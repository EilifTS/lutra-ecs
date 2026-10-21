#pragma once
#include <gtest/gtest.h>

#include <algorithm>
#include <cstring>
#include <functional>
#include <string>
#include <vector>

#include <lutra-ecs/save/ByteIO.h>
#include <lutra-ecs/save/Container.h>
#include <lutra-ecs/save/Crc32.h>

#include "TSaveTypes.h"

/**
 *  The file format: a save written and read back, and what happens to bytes that
 *  are not one. The corrupt cases come in two kinds. A damaged file fails its
 *  checksum. A forged one has a correct checksum, because a checksum is not a
 *  defence, and so it is the structure checks that have to hold; those are built
 *  by hand with Seal.
 */
namespace TContainer
{
	using lcs::save::ByteWriter;
	using lcs::save::ErrorKind;
	using TSave::EntityID;
	using TSave::MySchema;

	using Data = lcs::save::Container<EntityID>;
	using Chunk = lcs::save::ComponentChunk<EntityID>;

	constexpr const char* position_name = "Position";
	constexpr const char* enemy_name = "Enemy";
	constexpr const char* wet_name = "Wet";

	constexpr lcs::save::Format format{ .magic = 0x5354554Cu, .version = 3, .max_name_length = 1024 };

	/* Something a game keeps beside its components: fixed size, an id of its own. */
	struct Body
	{
		uint32_t id;
		float mass;
	};

	Data ASave()
	{
		Data data;
		data.entities = { EntityID::CreateNew(0), EntityID::CreateNew(2), EntityID::CreateNew(5) };

		const Body bodies[] = { { 7, 1.5f }, { 9, 2.5f } };
		data.sections.push_back(lcs::save::MakeRecordSection<Body>(40, bodies));
		data.sections.push_back(lcs::save::Section{ .id = 41, .bytes = { std::byte{ 1 }, std::byte{ 2 }, std::byte{ 3 } } });

		lcs::save::AddComponent<MySchema>(data, data.entities[0], TSave::Position{ 1, 2 });
		lcs::save::AddComponent<MySchema>(data, data.entities[1], TSave::Position{ 4, 5 });
		lcs::save::AddComponent<MySchema>(data, data.entities[1], TSave::Enemy{ 9 });
		lcs::save::AddTag<MySchema, TSave::Wet>(data, data.entities[0]);
		lcs::save::AddTag<MySchema, TSave::Wet>(data, data.entities[2]);
		return data;
	}

	std::vector<std::byte> Seal(ByteWriter& writer)
	{
		writer.U32(lcs::save::Crc32(writer.View()));
		return writer.Take();
	}

	/* The start of a valid file. The pieces a case wants to forge are written by
	 * hand after or instead of these. */
	void Header(ByteWriter& w) { w.U32(format.magic); w.U32(format.version); }
	void NoSections(ByteWriter& w) { w.U32(0); }

	/* A valid file around the `chunks` the case writes. */
	std::vector<std::byte> FileWithChunks(const std::function<void(ByteWriter&)>& write_chunks)
	{
		ByteWriter w;
		Header(w);
		w.U32(0);
		NoSections(w);
		write_chunks(w);
		return Seal(w);
	}

	lcs::save::Result<Data> Parse(const std::span<const std::byte> bytes)
	{
		return lcs::save::Parse<EntityID>(bytes, format, MySchema::entries);
	}

	std::vector<std::byte> Encode(const Data& data)
	{
		return lcs::save::Encode(data, format);
	}

	ErrorKind ErrorOf(const std::vector<std::byte>& bytes)
	{
		const auto result = Parse(bytes);
		EXPECT_FALSE(result.has_value());
		return result.has_value() ? ErrorKind::Other : result.error().kind;
	}
}

/*
 *  A save and back
 */

TEST(Container, ASaveComesBackAsItWasWritten)
{
	const TContainer::Data saved = TContainer::ASave();

	const auto parsed = TContainer::Parse(TContainer::Encode(saved));

	ASSERT_TRUE(parsed.has_value()) << parsed.error().message;
	EXPECT_EQ(parsed->entities, saved.entities);
	ASSERT_EQ(parsed->sections.size(), saved.sections.size());
	for (size_t i = 0; i < saved.sections.size(); i++)
	{
		EXPECT_EQ(parsed->sections[i].id, saved.sections[i].id);
		EXPECT_EQ(parsed->sections[i].bytes, saved.sections[i].bytes);
	}
	ASSERT_EQ(parsed->chunks.size(), saved.chunks.size());
	for (size_t i = 0; i < saved.chunks.size(); i++)
	{
		EXPECT_EQ(parsed->chunks[i].name, saved.chunks[i].name);
		EXPECT_EQ(parsed->chunks[i].size, saved.chunks[i].size);
		EXPECT_EQ(parsed->chunks[i].entities, saved.chunks[i].entities);
		EXPECT_EQ(parsed->chunks[i].bytes, saved.chunks[i].bytes);
	}
	EXPECT_TRUE(parsed->skipped_component_names.empty());
}

TEST(Container, TheComponentsInAChunkAreReadBackByTypeAndIndex)
{
	const auto parsed = TContainer::Parse(TContainer::Encode(TContainer::ASave()));
	ASSERT_TRUE(parsed.has_value());

	const TContainer::Chunk* positions = lcs::save::FindChunk(*parsed, TContainer::position_name);
	ASSERT_NE(positions, nullptr);
	ASSERT_EQ(positions->Count(), 2u);
	EXPECT_EQ(positions->entities[1], TSave::EntityID::CreateNew(2));
	EXPECT_EQ(lcs::save::ReadComponent<TSave::Position>(*positions, 1).y, 5);

	const TContainer::Chunk* tags = lcs::save::FindChunk(*parsed, TContainer::wet_name);
	ASSERT_NE(tags, nullptr);
	EXPECT_EQ(tags->Count(), 2u);
	EXPECT_EQ(tags->size, 0u);
	EXPECT_TRUE(tags->bytes.empty());
	EXPECT_EQ(lcs::save::FindChunk(*parsed, "Follower"), nullptr);
}

TEST(Container, AnEmptySaveIsASave)
{
	const auto parsed = TContainer::Parse(TContainer::Encode(TContainer::Data{}));

	ASSERT_TRUE(parsed.has_value()) << parsed.error().message;
	EXPECT_TRUE(parsed->entities.empty());
	EXPECT_TRUE(parsed->chunks.empty());
	EXPECT_TRUE(parsed->sections.empty());
}

TEST(Container, TheFileStartsWithItsMagicAndVersion)
{
	const std::vector<std::byte> bytes = TContainer::Encode(TContainer::ASave());

	EXPECT_EQ(char(bytes[0]), 'L');
	EXPECT_EQ(char(bytes[1]), 'U');
	EXPECT_EQ(char(bytes[2]), 'T');
	EXPECT_EQ(char(bytes[3]), 'S');
	EXPECT_EQ(uint8_t(bytes[4]), TContainer::format.version);
}

TEST(Container, ASaveWithNoSectionsRoundTrips)
{
	TContainer::Data data;
	data.entities = { TSave::EntityID::CreateNew(1) };
	lcs::save::AddComponent<TContainer::MySchema>(data, data.entities[0], TSave::Enemy{ 2 });

	const auto parsed = TContainer::Parse(TContainer::Encode(data));

	ASSERT_TRUE(parsed.has_value()) << parsed.error().message;
	EXPECT_EQ(parsed->entities.size(), 1u);
}

/*
 *  Sections
 */

TEST(Container, ASectionIsFoundByIdAndAnotherIsNot)
{
	const TContainer::Data data = TContainer::ASave();

	const lcs::save::Section* section = lcs::save::FindSection(data, 41);
	ASSERT_NE(section, nullptr);
	EXPECT_EQ(section->bytes.size(), 3u);
	EXPECT_EQ(lcs::save::FindSection(data, 99), nullptr);
}

TEST(Container, RecordsComeBackFromTheirSection)
{
	const auto parsed = TContainer::Parse(TContainer::Encode(TContainer::ASave()));
	ASSERT_TRUE(parsed.has_value());
	const lcs::save::Section* section = lcs::save::FindSection(*parsed, 40);
	ASSERT_NE(section, nullptr);

	const auto records = lcs::save::ReadRecordSection<TContainer::Body>(*section, "body");

	ASSERT_TRUE(records.has_value()) << records.error().message;
	ASSERT_EQ(records->size(), 2u);
	EXPECT_EQ((*records)[1].id, 9u);
	EXPECT_FLOAT_EQ((*records)[1].mass, 2.5f);
}

TEST(Container, NoRecordsIsAnEmptySection)
{
	const lcs::save::Section section = lcs::save::MakeRecordSection<TContainer::Body>(40, {});

	const auto records = lcs::save::ReadRecordSection<TContainer::Body>(section, "body");

	ASSERT_TRUE(records.has_value());
	EXPECT_TRUE(records->empty());
}

TEST(Container, RecordsOfAnotherSizeAreRejectedAndNamed)
{
	lcs::save::ByteWriter w;
	w.U32(uint32_t(sizeof(TContainer::Body) + 4));
	w.U32(0);
	const lcs::save::Section section{ .id = 40, .bytes = w.Take() };

	const auto records = lcs::save::ReadRecordSection<TContainer::Body>(section, "body");

	ASSERT_FALSE(records.has_value());
	EXPECT_EQ(records.error().kind, lcs::save::ErrorKind::SizeMismatch);
	EXPECT_NE(records.error().message.find("body"), std::string::npos);
}

TEST(Container, ARecordCountThatDoesNotFitItsSectionIsRejected)
{
	lcs::save::ByteWriter w;
	w.U32(uint32_t(sizeof(TContainer::Body)));
	w.U32(0xFFFFFFFFu);
	const lcs::save::Section section{ .id = 40, .bytes = w.Take() };

	const auto records = lcs::save::ReadRecordSection<TContainer::Body>(section, "body");

	ASSERT_FALSE(records.has_value());
	EXPECT_EQ(records.error().kind, lcs::save::ErrorKind::BadCount);
}

TEST(Container, BytesAfterTheLastRecordAreRejected)
{
	const TContainer::Body bodies[] = { { 1, 1.0f } };
	lcs::save::Section section = lcs::save::MakeRecordSection<TContainer::Body>(40, bodies);
	section.bytes.push_back(std::byte{ 0 });

	const auto records = lcs::save::ReadRecordSection<TContainer::Body>(section, "body");

	ASSERT_FALSE(records.has_value());
	EXPECT_EQ(records.error().kind, lcs::save::ErrorKind::Malformed);
}

/* A game that no longer looks for a section still reads the file. */
TEST(Container, ASectionNobodyLooksForIsCarriedAlongAndIgnored)
{
	const auto parsed = TContainer::Parse(TContainer::Encode(TContainer::ASave()));

	ASSERT_TRUE(parsed.has_value());
	EXPECT_NE(lcs::save::FindSection(*parsed, 41), nullptr);
}

TEST(Container, ASectionThatIsInTheFileTwiceIsRejected)
{
	lcs::save::ByteWriter w;
	TContainer::Header(w);
	w.U32(0);
	w.U32(2);
	for (int i = 0; i < 2; i++)
	{
		w.U32(40);
		w.U32(0);
	}
	w.U32(0);

	EXPECT_EQ(TContainer::ErrorOf(TContainer::Seal(w)), lcs::save::ErrorKind::Malformed);
}

TEST(Container, ASectionLongerThanTheFileIsRejected)
{
	lcs::save::ByteWriter w;
	TContainer::Header(w);
	w.U32(0);
	w.U32(1);
	w.U32(40);
	w.U32(1000);
	w.U32(0);

	EXPECT_EQ(TContainer::ErrorOf(TContainer::Seal(w)), lcs::save::ErrorKind::Truncated);
}

TEST(Container, ACountOfFourBillionSectionsIsRejected)
{
	lcs::save::ByteWriter w;
	TContainer::Header(w);
	w.U32(0);
	w.U32(0xFFFFFFFFu);

	EXPECT_EQ(TContainer::ErrorOf(TContainer::Seal(w)), lcs::save::ErrorKind::BadCount);
}

/*
 *  Damage
 */

/* A file cut short at any length, empty included, is an error and never a crash. */
TEST(Container, ATruncatedFileIsAnErrorAtEveryLength)
{
	const std::vector<std::byte> bytes = TContainer::Encode(TContainer::ASave());

	for (size_t length = 0; length < bytes.size(); length++)
	{
		const auto result = TContainer::Parse(std::span<const std::byte>(bytes).first(length));
		EXPECT_FALSE(result.has_value()) << "length " << length;
	}
}

TEST(Container, ChangingAnySingleByteIsAnError)
{
	std::vector<std::byte> bytes = TContainer::Encode(TContainer::ASave());

	for (size_t i = 0; i < bytes.size(); i++)
	{
		const std::byte original = bytes[i];
		bytes[i] ^= std::byte{ 0xFF };
		EXPECT_FALSE(TContainer::Parse(bytes).has_value()) << "byte " << i;
		bytes[i] = original;
	}
	EXPECT_TRUE(TContainer::Parse(bytes).has_value());
}

TEST(Container, ABitFlippedInAComponentFailsTheChecksum)
{
	const TContainer::Data data = TContainer::ASave();
	std::vector<std::byte> bytes = TContainer::Encode(data);
	const TContainer::Chunk* chunk = lcs::save::FindChunk(data, TContainer::position_name);
	ASSERT_NE(chunk, nullptr);
	const auto where = std::search(bytes.begin(), bytes.end(), chunk->bytes.begin(), chunk->bytes.end());
	ASSERT_NE(where, bytes.end());

	*where ^= std::byte{ 0x01 };

	EXPECT_EQ(TContainer::ErrorOf(bytes), lcs::save::ErrorKind::BadChecksum);
}

TEST(Container, AFileThatIsNotASaveIsSaidSo)
{
	std::vector<std::byte> bytes = TContainer::Encode(TContainer::ASave());
	bytes[0] = std::byte{ 'X' };

	EXPECT_EQ(TContainer::ErrorOf(bytes), lcs::save::ErrorKind::BadMagic);
}

TEST(Container, ASaveFromAnotherVersionIsSaidSo)
{
	std::vector<std::byte> bytes = TContainer::Encode(TContainer::ASave());
	bytes[4] = std::byte{ uint8_t(TContainer::format.version + 1) };

	const auto result = TContainer::Parse(bytes);

	ASSERT_FALSE(result.has_value());
	EXPECT_EQ(result.error().kind, lcs::save::ErrorKind::UnsupportedVersion);
	EXPECT_NE(result.error().message.find("version"), std::string::npos);
}

/*
 *  Forged: a correct checksum around something that is not a save
 */

TEST(Container, ACountOfFourBillionEntitiesIsRejected)
{
	lcs::save::ByteWriter w;
	TContainer::Header(w);
	w.U32(0xFFFFFFFFu);

	EXPECT_EQ(TContainer::ErrorOf(TContainer::Seal(w)), lcs::save::ErrorKind::BadCount);
}

TEST(Container, ACountOfFourBillionChunksIsRejected)
{
	EXPECT_EQ(TContainer::ErrorOf(TContainer::FileWithChunks([](lcs::save::ByteWriter& w) { w.U32(0xFFFFFFFFu); })), lcs::save::ErrorKind::BadCount);
}

TEST(Container, AChunkThatHoldsMoreThanTheFileIsRejected)
{
	const auto bytes = TContainer::FileWithChunks([](lcs::save::ByteWriter& w) {
		w.U32(1);
		w.String(TContainer::position_name);
		w.U32(uint32_t(sizeof(TSave::Position)));
		w.U32(0x7FFFFFFFu);
	});

	EXPECT_EQ(TContainer::ErrorOf(bytes), lcs::save::ErrorKind::BadCount);
}

/* count * (4 + size) wraps 32 bits to something small; it must not be believed. */
TEST(Container, AChunkWhoseSizeTimesCountWrapsIsRejected)
{
	const auto bytes = TContainer::FileWithChunks([](lcs::save::ByteWriter& w) {
		w.U32(1);
		w.String("Mystery"); /* a name no one knows, so it would be skipped */
		w.U32(0xFFFFFFF0u);
		w.U32(0x00020000u);
	});

	EXPECT_EQ(TContainer::ErrorOf(bytes), lcs::save::ErrorKind::BadCount);
}

TEST(Container, EntitiesOutOfOrderOrRepeatedAreRejected)
{
	for (const std::vector<uint32_t>& handles : { std::vector<uint32_t>{ 3, 1 }, std::vector<uint32_t>{ 2, 2 } })
	{
		lcs::save::ByteWriter w;
		TContainer::Header(w);
		w.U32(uint32_t(handles.size()));
		for (const uint32_t handle : handles) w.U32(handle);

		EXPECT_EQ(TContainer::ErrorOf(TContainer::Seal(w)), lcs::save::ErrorKind::Malformed);
	}
}

TEST(Container, AnEntityIdThatIsNoIdIsRejected)
{
	for (const uint32_t handle : { 0xFFFFFFFFu, TSave::EntityID::max_index })
	{
		lcs::save::ByteWriter w;
		TContainer::Header(w);
		w.U32(1);
		w.U32(handle);

		EXPECT_EQ(TContainer::ErrorOf(TContainer::Seal(w)), lcs::save::ErrorKind::Malformed) << handle;
	}
}

/*
 *  Chunks
 */

/* A component this build has never heard of: skipped by its length. */
TEST(Container, AChunkOfAnUnknownComponentIsSkippedAndTheRestIsRead)
{
	const auto bytes = TContainer::FileWithChunks([](lcs::save::ByteWriter& w) {
		w.U32(2);

		w.String("Mystery"); /* unknown, 12 bytes of component for each of 2 entities */
		w.U32(12);
		w.U32(2);
		w.U32(3); w.U32(4);
		for (int i = 0; i < 24; i++) w.Raw(uint8_t(0xAB));

		w.String(TContainer::enemy_name);
		w.U32(uint32_t(sizeof(TSave::Enemy)));
		w.U32(1);
		w.U32(6);
		w.Raw(TSave::Enemy{ 25 });
	});

	const auto parsed = TContainer::Parse(bytes);

	ASSERT_TRUE(parsed.has_value()) << parsed.error().message;
	ASSERT_EQ(parsed->skipped_component_names.size(), 1u);
	EXPECT_EQ(parsed->skipped_component_names[0], "Mystery");
	ASSERT_EQ(parsed->chunks.size(), 1u);
	EXPECT_EQ(lcs::save::ReadComponent<TSave::Enemy>(parsed->chunks[0], 0).strength, 25);
}

/* The layout of a component it does know changed: the bytes cannot be reinterpreted. */
TEST(Container, AKnownComponentWithAnotherSizeIsRejectedAndNamed)
{
	const auto bytes = TContainer::FileWithChunks([](lcs::save::ByteWriter& w) {
		w.U32(1);
		w.String(TContainer::position_name);
		w.U32(uint32_t(sizeof(TSave::Position) + 4));
		w.U32(0);
	});

	const auto result = TContainer::Parse(bytes);

	ASSERT_FALSE(result.has_value());
	EXPECT_EQ(result.error().kind, lcs::save::ErrorKind::SizeMismatch);
	EXPECT_NE(result.error().message.find("Position"), std::string::npos);
}

TEST(Container, ATagWithBytesIsRejected)
{
	const auto bytes = TContainer::FileWithChunks([](lcs::save::ByteWriter& w) {
		w.U32(1);
		w.String(TContainer::wet_name);
		w.U32(4);
		w.U32(0);
	});

	EXPECT_EQ(TContainer::ErrorOf(bytes), lcs::save::ErrorKind::SizeMismatch);
}

TEST(Container, TwoChunksOfTheSameComponentAreRejected)
{
	const auto bytes = TContainer::FileWithChunks([](lcs::save::ByteWriter& w) {
		w.U32(2);
		for (int i = 0; i < 2; i++)
		{
			w.String(TContainer::enemy_name);
			w.U32(uint32_t(sizeof(TSave::Enemy)));
			w.U32(1);
			w.U32(uint32_t(i));
			w.Raw(TSave::Enemy{});
		}
	});

	EXPECT_EQ(TContainer::ErrorOf(bytes), lcs::save::ErrorKind::Malformed);
}

TEST(Container, AnEntityWithTheSameComponentTwiceIsRejected)
{
	const auto bytes = TContainer::FileWithChunks([](lcs::save::ByteWriter& w) {
		w.U32(1);
		w.String(TContainer::enemy_name);
		w.U32(uint32_t(sizeof(TSave::Enemy)));
		w.U32(2);
		w.U32(3); w.U32(3);
		w.Raw(TSave::Enemy{});
		w.Raw(TSave::Enemy{});
	});

	EXPECT_EQ(TContainer::ErrorOf(bytes), lcs::save::ErrorKind::Malformed);
}

TEST(Container, AComponentOnAnEntityIdThatIsNoIdIsRejected)
{
	const auto bytes = TContainer::FileWithChunks([](lcs::save::ByteWriter& w) {
		w.U32(1);
		w.String(TContainer::enemy_name);
		w.U32(uint32_t(sizeof(TSave::Enemy)));
		w.U32(1);
		w.U32(0xFFFFFFFFu);
		w.Raw(TSave::Enemy{});
	});

	EXPECT_EQ(TContainer::ErrorOf(bytes), lcs::save::ErrorKind::Malformed);
}

TEST(Container, BytesAfterTheLastChunkAreRejected)
{
	const auto bytes = TContainer::FileWithChunks([](lcs::save::ByteWriter& w) {
		w.U32(0);
		w.U32(0xDEADBEEFu);
	});

	EXPECT_EQ(TContainer::ErrorOf(bytes), lcs::save::ErrorKind::Malformed);
}

/*
 *  Chunks are named for their component
 */

TEST(Container, AChunkCarriesTheNameOfItsComponentInTheFile)
{
	const std::vector<std::byte> bytes = TContainer::Encode(TContainer::ASave());
	const std::string_view name = "Position";

	const auto where = std::search(bytes.begin(), bytes.end(), std::as_bytes(std::span(name)).begin(), std::as_bytes(std::span(name)).end());

	EXPECT_NE(where, bytes.end());
}

/* The same name is the same component, wherever it sits in the ECS's type list. */
TEST(Container, ComponentsAreFoundByNameAndNotByPosition)
{
	const auto parsed = TContainer::Parse(TContainer::Encode(TContainer::ASave()));
	ASSERT_TRUE(parsed.has_value());

	const TContainer::Chunk* enemy = lcs::save::FindChunk(*parsed, "Enemy");

	ASSERT_NE(enemy, nullptr);
	EXPECT_EQ(enemy->name, "Enemy");
	EXPECT_EQ(lcs::save::ReadComponent<TSave::Enemy>(*enemy, 0).strength, 9);
}

/* A component renamed since the save is a component this build does not know. */
TEST(Container, ASavedComponentWithAnotherNameIsSkippedAndNamed)
{
	const auto bytes = TContainer::FileWithChunks([](lcs::save::ByteWriter& w) {
		w.U32(1);
		w.String("enemy"); /* the case is not the same */
		w.U32(uint32_t(sizeof(TSave::Enemy)));
		w.U32(1);
		w.U32(2);
		w.Raw(TSave::Enemy{ 3 });
	});

	const auto parsed = TContainer::Parse(bytes);

	ASSERT_TRUE(parsed.has_value()) << parsed.error().message;
	EXPECT_TRUE(parsed->chunks.empty());
	ASSERT_EQ(parsed->skipped_component_names.size(), 1u);
	EXPECT_EQ(parsed->skipped_component_names[0], "enemy");
}

TEST(Container, AChunkWithNoNameIsRejected)
{
	const auto bytes = TContainer::FileWithChunks([](lcs::save::ByteWriter& w) {
		w.U32(1);
		w.String("");
		w.U32(0);
		w.U32(0);
	});

	EXPECT_EQ(TContainer::ErrorOf(bytes), lcs::save::ErrorKind::Malformed);
}

TEST(Container, AChunkNameLongerThanTheFormatAllowsIsRejected)
{
	const auto bytes = TContainer::FileWithChunks([](lcs::save::ByteWriter& w) {
		w.U32(1);
		w.String(std::string(TContainer::format.max_name_length + 1, 'a'));
		w.U32(0);
		w.U32(0);
	});

	EXPECT_EQ(TContainer::ErrorOf(bytes), lcs::save::ErrorKind::TooLong);
}

TEST(Container, AChunkNameThatClaimsMoreThanTheFileHoldsIsTruncated)
{
	const auto bytes = TContainer::FileWithChunks([](lcs::save::ByteWriter& w) {
		w.U32(1);
		w.U32(1000); /* a name length, and a header that fits, but no name after it */
		w.U32(0);
		w.U32(0);
	});

	EXPECT_EQ(TContainer::ErrorOf(bytes), lcs::save::ErrorKind::Truncated);
}
