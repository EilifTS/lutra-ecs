#pragma once
#include <functional>
#include <string_view>

namespace lcs::save
{
	enum class Severity
	{
		Warning,
		Error,
	};

	/* Where decode says what it skipped, and where a game says what it left out and
	 * why. `subject` is the component's name (or "save"); a save that loses a
	 * component loses it quietly otherwise. An empty Reporter is allowed and says
	 * nothing. */
	using Reporter = std::function<void(Severity severity, std::string_view subject, std::string_view message)>;

	inline void Report(const Reporter& reporter, const Severity severity, const std::string_view subject, const std::string_view message)
	{
		if (reporter)
		{
			reporter(severity, subject, message);
		}
	}
}
