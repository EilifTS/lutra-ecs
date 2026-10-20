#pragma once
#include <functional>
#include <string>
#include <string_view>
#include <utility>

namespace lcs::save
{
	enum class Severity
	{
		Warning,
		Error,
	};

	/* Where capture and decode say what they left out and why. `subject` is the
	 * component's name (or "save"); a save that loses a component loses it quietly
	 * otherwise. An empty Reporter is allowed and says nothing. */
	using Reporter = std::function<void(Severity severity, std::string_view subject, std::string_view message)>;

	inline void Report(const Reporter& reporter, const Severity severity, const std::string_view subject, const std::string_view message)
	{
		if (reporter)
		{
			reporter(severity, subject, message);
		}
	}

	/* What a policy decides about one component. */
	struct Verdict
	{
		enum class Kind { Keep, Drop, Fail };

		Kind kind{ Kind::Keep };
		std::string why{};

		/* The component goes through as it is (or as the policy changed it). */
		static Verdict Keep() { return Verdict{}; }

		/* The component is left out and reported; the rest goes on. */
		static Verdict Drop(std::string why) { return Verdict{ Kind::Drop, std::move(why) }; }

		/* On decode, the file is not usable: the whole decode fails. On capture
		 * there is no file to fail, so it is a Drop. */
		static Verdict Fail(std::string why) { return Verdict{ Kind::Fail, std::move(why) }; }
	};

	/* A policy is how a game takes part in Capture and Decode: any type, with
	 * whichever of the hooks it needs (`Accept` for Capture, `Component` for Decode;
	 * see each). A hook it does not have is a component let through as it is, so a
	 * policy that only cares about one of them, or about one component type, says
	 * only that. NoPolicy has none, and is what a call without a policy gets. */
	struct NoPolicy {};
}
