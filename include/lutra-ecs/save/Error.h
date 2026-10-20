#pragma once
#include <expected>
#include <string>
#include <utility>

namespace lcs::save
{
	enum class ErrorKind
	{
		Truncated,          /* Fewer bytes than the file says it holds. */
		BadMagic,           /* Not a save. */
		UnsupportedVersion, /* A save, but from a layout this build does not read. */
		BadChecksum,        /* The bytes changed after they were written. */
		BadCount,           /* A count that cannot fit in what is left of the file. */
		TooLong,            /* A name longer than any content uses. */
		SizeMismatch,       /* A component or record with another layout than this build's. */
		Malformed,          /* Anything else that is wrong with the structure or the values. */
		Io,                 /* The file could not be written or read. */
		Other,              /* A failure of the caller's own, carried through the same result. */
	};

	struct Error
	{
		ErrorKind kind;
		std::string message;
	};

	inline Error MakeError(const ErrorKind kind, std::string message)
	{
		return Error{ kind, std::move(message) };
	}

	template <typename T>
	using Result = std::expected<T, Error>;
}
