#pragma once
#include <gtest/gtest.h>

#include <iterator>
#include <span>
#include <vector>

#include <lutra-ecs/save/FileIO.h>

#include "ScratchDir.h"

/**
 *  Putting the bytes of a save on disk and getting them back.
 */
namespace TFileIO
{
	inline std::vector<std::byte> ReadAll(const std::filesystem::path& path)
	{
		std::ifstream file(path, std::ios::binary);
		const std::vector<char> chars((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		const auto bytes = std::as_bytes(std::span(chars));
		return { bytes.begin(), bytes.end() };
	}

	inline std::vector<std::byte> SomeBytes(const uint8_t fill, const size_t size)
	{
		return std::vector<std::byte>(size, std::byte{ fill });
	}
}

TEST(WriteFile, WritesExactlyTheBytes)
{
	ScratchDir dir("write-exact");
	const std::vector<std::byte> bytes = TFileIO::SomeBytes(0xAB, 1000);

	const auto written = lcs::save::WriteFile(dir.root / "quick.sav", bytes);

	ASSERT_TRUE(written.has_value()) << written.error().message;
	EXPECT_EQ(TFileIO::ReadAll(dir.root / "quick.sav"), bytes);
}

TEST(WriteFile, CreatesTheDirectoryIfItIsMissing)
{
	ScratchDir dir("write-mkdir");

	const auto written = lcs::save::WriteFile(dir.root / "saves" / "slot1" / "quick.sav", TFileIO::SomeBytes(1, 10));

	ASSERT_TRUE(written.has_value()) << written.error().message;
	EXPECT_TRUE(std::filesystem::exists(dir.root / "saves" / "slot1" / "quick.sav"));
}

TEST(WriteFile, ReplacesAnEarlierFileAndLeavesNoTemporaryFile)
{
	ScratchDir dir("write-replace");
	const std::filesystem::path path = dir.root / "quick.sav";
	ASSERT_TRUE(lcs::save::WriteFile(path, TFileIO::SomeBytes(1, 500)).has_value());

	const auto second = lcs::save::WriteFile(path, TFileIO::SomeBytes(2, 20));

	ASSERT_TRUE(second.has_value()) << second.error().message;
	EXPECT_EQ(TFileIO::ReadAll(path), TFileIO::SomeBytes(2, 20));
	EXPECT_FALSE(std::filesystem::exists(dir.root / "quick.sav.tmp"));
}

/* A file the OS cannot put in place is an error, and the temporary file does not stay behind. */
TEST(WriteFile, FailingToReplaceTheTargetIsAnErrorAndCleansUp)
{
	ScratchDir dir("write-blocked");
	std::filesystem::create_directories(dir.root / "quick.sav"); /* a directory where the file should go */

	const auto written = lcs::save::WriteFile(dir.root / "quick.sav", TFileIO::SomeBytes(1, 10));

	ASSERT_FALSE(written.has_value());
	EXPECT_EQ(written.error().kind, lcs::save::ErrorKind::Io);
	EXPECT_FALSE(std::filesystem::exists(dir.root / "quick.sav.tmp"));
	EXPECT_TRUE(std::filesystem::is_directory(dir.root / "quick.sav"));
}

TEST(WriteFile, ADirectoryThatCannotBeMadeIsAnError)
{
	ScratchDir dir("write-nodir");
	dir.Write("saves", "this is a file"); /* so "saves" cannot be a directory */

	const auto written = lcs::save::WriteFile(dir.root / "saves" / "quick.sav", TFileIO::SomeBytes(1, 10));

	ASSERT_FALSE(written.has_value());
	EXPECT_EQ(written.error().kind, lcs::save::ErrorKind::Io);
}

TEST(ReadFile, ReturnsTheBytesThatWereWritten)
{
	ScratchDir dir("read-back");
	const std::vector<std::byte> bytes = TFileIO::SomeBytes(0x5A, 300);
	ASSERT_TRUE(lcs::save::WriteFile(dir.root / "a.sav", bytes).has_value());

	const auto read = lcs::save::ReadFile(dir.root / "a.sav");

	ASSERT_TRUE(read.has_value()) << read.error().message;
	EXPECT_EQ(*read, bytes);
}

TEST(ReadFile, AnEmptyFileIsEmptyBytes)
{
	ScratchDir dir("read-empty");
	dir.Write("empty.sav", "");

	const auto read = lcs::save::ReadFile(dir.root / "empty.sav");

	ASSERT_TRUE(read.has_value());
	EXPECT_TRUE(read->empty());
}

TEST(ReadFile, AMissingFileIsAnIoError)
{
	ScratchDir dir("read-missing");

	const auto read = lcs::save::ReadFile(dir.root / "nope.sav");

	ASSERT_FALSE(read.has_value());
	EXPECT_EQ(read.error().kind, lcs::save::ErrorKind::Io);
}

TEST(ReadFile, AFileLargerThanTheLimitIsAnIoErrorAndIsNotRead)
{
	ScratchDir dir("read-large");
	ASSERT_TRUE(lcs::save::WriteFile(dir.root / "big.sav", TFileIO::SomeBytes(1, 100)).has_value());

	const auto read = lcs::save::ReadFile(dir.root / "big.sav", 99);

	ASSERT_FALSE(read.has_value());
	EXPECT_EQ(read.error().kind, lcs::save::ErrorKind::Io);
}
