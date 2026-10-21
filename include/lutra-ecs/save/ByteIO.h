#pragma once
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <expected>
#include <limits>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include <lutra-ecs/save/Error.h>

/**
 *  Bytes in and out of a save file. Everything is little-endian and fixed width, and
 *  the file is only ever written and read on little-endian machines, which every
 *  platform this library is used on is; the assert makes a port say so.
 */
static_assert(std::endian::native == std::endian::little, "the save format is little-endian only");

namespace lcs::save
{
	class ByteWriter
	{
	public:
		void U32(const uint32_t value) { Raw(value); }

		/* Length-prefixed, no terminator. */
		void String(const std::string_view text)
		{
			U32(uint32_t(text.size()));
			Bytes(std::as_bytes(std::span(text.data(), text.size())));
		}

		void Bytes(const std::span<const std::byte> bytes)
		{
			buffer.insert(buffer.end(), bytes.begin(), bytes.end());
		}

		/* The object's bytes as they are in memory. */
		template <typename T> requires std::is_trivially_copyable_v<T>
		void Raw(const T& value)
		{
			Bytes(std::as_bytes(std::span(&value, 1)));
		}

		template <typename T> requires std::is_trivially_copyable_v<T>
		void RawArray(const std::span<const T> values)
		{
			Bytes(std::as_bytes(values));
		}

		[[nodiscard]] size_t Size() const { return buffer.size(); }
		[[nodiscard]] std::span<const std::byte> View() const { return buffer; }
		[[nodiscard]] std::vector<std::byte> Take() { return std::move(buffer); }

	private:
		std::vector<std::byte> buffer{};
	};

	/* Reads from bytes nobody has vouched for. Every read is checked against what
	 * is left and returns an error, never asserts and never reads past the end;
	 * a count is checked against the bytes left before anyone allocates for it. */
	class ByteReader
	{
	public:
		explicit ByteReader(const std::span<const std::byte> bytes) : data(bytes) {}

		[[nodiscard]] size_t Remaining() const { return data.size() - position; }
		[[nodiscard]] bool AtEnd() const { return position == data.size(); }

		[[nodiscard]] Result<uint32_t> U32() { return Raw<uint32_t>(); }

		/* A view into the buffer, which must outlive it. */
		[[nodiscard]] Result<std::span<const std::byte>> Bytes(const size_t count)
		{
			if (count > Remaining())
			{
				return std::unexpected(MakeError(ErrorKind::Truncated, "the file ends inside a value"));
			}

			const std::span<const std::byte> view = data.subspan(position, count);
			position += count;
			return view;
		}

		template <typename T> requires std::is_trivially_copyable_v<T>
		[[nodiscard]] Result<T> Raw()
		{
			const Result<std::span<const std::byte>> bytes = Bytes(sizeof(T));
			if (!bytes) return std::unexpected(bytes.error());

			T value;
			std::memcpy(&value, bytes->data(), sizeof(T));
			return value;
		}

		/* A length-prefixed name of at most `max_length` bytes. */
		[[nodiscard]] Result<std::string> String(const size_t max_length)
		{
			const Result<uint32_t> length = U32();
			if (!length) return std::unexpected(length.error());
			if (*length > max_length)
			{
				return std::unexpected(MakeError(ErrorKind::TooLong, "a name is longer than " + std::to_string(max_length) + " bytes"));
			}

			const Result<std::span<const std::byte>> bytes = Bytes(*length);
			if (!bytes) return std::unexpected(bytes.error());

			return std::string(reinterpret_cast<const char*>(bytes->data()), bytes->size());
		}

		/* A count of elements that are each at least `element_bytes` long, which
		 * must fit in what is left. This is what stops a corrupt count of four
		 * billion from becoming a four billion element allocation. */
		[[nodiscard]] Result<uint32_t> Count(const size_t element_bytes)
		{
			const Result<uint32_t> count = U32();
			if (!count) return std::unexpected(count.error());

			if (element_bytes != 0 && *count > Remaining() / element_bytes)
			{
				return std::unexpected(MakeError(ErrorKind::BadCount, "a count of " + std::to_string(*count) + " does not fit in the rest of the file"));
			}

			return count;
		}

	private:
		std::span<const std::byte> data;
		size_t position{ 0 };
	};
}
