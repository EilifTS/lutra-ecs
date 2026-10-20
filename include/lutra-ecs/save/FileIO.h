#pragma once
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <span>
#include <string>
#include <system_error>
#include <vector>

#include <lutra-ecs/save/Error.h>

namespace lcs::save
{
	/* Writes the bytes to `path`, creating its directory if need be. The bytes go
	 * to a temporary file beside it first and replace the target in one rename, so
	 * a write that fails halfway (a full disk) leaves an earlier save as it was,
	 * and no temporary file behind. */
	[[nodiscard]] inline Result<void> WriteFile(const std::filesystem::path& path, const std::span<const std::byte> bytes)
	{
		std::error_code error;

		if (path.has_parent_path())
		{
			std::filesystem::create_directories(path.parent_path(), error);
			if (error)
			{
				return std::unexpected(MakeError(ErrorKind::Io, "cannot create " + path.parent_path().string() + ": " + error.message()));
			}
		}

		std::filesystem::path temporary = path;
		temporary += ".tmp";

		{
			std::ofstream file(temporary, std::ios::binary | std::ios::trunc);
			if (!file)
			{
				return std::unexpected(MakeError(ErrorKind::Io, "cannot open " + temporary.string() + " for writing"));
			}

			file.write(reinterpret_cast<const char*>(bytes.data()), std::streamsize(bytes.size()));
			file.flush();
			if (!file)
			{
				file.close();
				std::filesystem::remove(temporary, error);
				return std::unexpected(MakeError(ErrorKind::Io, "cannot write " + temporary.string() + " (is the disk full?)"));
			}
		}

		std::filesystem::rename(temporary, path, error);
		if (error)
		{
			std::error_code ignored;
			std::filesystem::remove(temporary, ignored);
			return std::unexpected(MakeError(ErrorKind::Io, "cannot replace " + path.string() + ": " + error.message()));
		}

		return {};
	}

	/* The file's bytes, or an error for a file that is missing, unreadable, or
	 * larger than `max_bytes`: a save is kilobytes to a few megabytes, and this is
	 * the line past which a file is not one. */
	[[nodiscard]] inline Result<std::vector<std::byte>> ReadFile(const std::filesystem::path& path, const uintmax_t max_bytes = 256ull * 1024 * 1024)
	{
		std::error_code error;
		const uintmax_t size = std::filesystem::file_size(path, error);
		if (error)
		{
			return std::unexpected(MakeError(ErrorKind::Io, "cannot read " + path.string() + ": " + error.message()));
		}
		if (size > max_bytes)
		{
			return std::unexpected(MakeError(ErrorKind::Io, path.string() + " is too large to be a save"));
		}

		std::ifstream file(path, std::ios::binary);
		std::vector<std::byte> bytes(static_cast<size_t>(size));
		if (!file || (size > 0 && !file.read(reinterpret_cast<char*>(bytes.data()), std::streamsize(size))))
		{
			return std::unexpected(MakeError(ErrorKind::Io, "cannot read " + path.string()));
		}

		return bytes;
	}
}
