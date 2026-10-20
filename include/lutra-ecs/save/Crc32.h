#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace lcs::save
{
	namespace detail
	{
		constexpr std::array<uint32_t, 256> MakeCrc32Table()
		{
			std::array<uint32_t, 256> table{};
			for (uint32_t i = 0; i < 256; i++)
			{
				uint32_t value = i;
				for (int bit = 0; bit < 8; bit++)
				{
					value = (value & 1) ? (value >> 1) ^ 0xEDB88320u : value >> 1;
				}
				table[i] = value;
			}
			return table;
		}

		inline constexpr std::array<uint32_t, 256> crc32_table = MakeCrc32Table();
	}

	/* CRC-32 as zlib and PNG compute it (reflected, polynomial 0xEDB88320). It
	 * catches a flipped bit or a truncated write; it is not a defence against
	 * anyone who wants to forge a save. */
	constexpr uint32_t Crc32(const std::span<const std::byte> bytes)
	{
		uint32_t crc = 0xFFFFFFFFu;
		for (const std::byte b : bytes)
		{
			crc = detail::crc32_table[(crc ^ uint32_t(b)) & 0xFFu] ^ (crc >> 8);
		}
		return crc ^ 0xFFFFFFFFu;
	}
}
