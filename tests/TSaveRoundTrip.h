#pragma once
#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <tuple>
#include <vector>

#include <lutra-ecs/save/Capture.h>
#include <lutra-ecs/save/Container.h>
#include <lutra-ecs/save/Decode.h>

#include "TSaveTypes.h"

/**
 *  An ECS into a Container and back: what capture leaves out and says so, what
 *  decode refuses, makes fit or drops, and that restoring puts every entity back
 *  under its own id.
 */
namespace TRoundTrip
{
	using lcs::save::ErrorKind;
	using lcs::save::Severity;
	using lcs::save::Verdict;
	using TSave::ECS;
	using TSave::EntityID;
	using TSave::MySchema;

	using Data = lcs::save::Container<EntityID>;
	using Decoded = lcs::save::Decoded<MySchema>;

	constexpr lcs::save::Format format{ .magic = 0x54455354u, .version = 1, .name_table_count = 0, .max_name_length = 64 };

	/* What was reported, in order. */
	struct Log
	{
		struct Line
		{
			Severity severity;
			std::string subject;
			std::string message;
		};

		std::vector<Line> lines{};

		lcs::save::Reporter Reporter()
		{
			return [this](const Severity severity, const std::string_view subject, const std::string_view message) {
				lines.push_back(Line{ severity, std::string(subject), std::string(message) });
			};
		}

		bool Has(const Severity severity, const std::string& subject, const std::string& message_part) const
		{
			return std::ranges::any_of(lines, [&](const Line& line) {
				return line.severity == severity && line.subject == subject && line.message.find(message_part) != std::string::npos;
			});
		}
	};

	/* Remembers what it was asked about, and refuses what it is told to. */
	struct RecordingPolicy
	{
		int accepted_calls{ 0 };
		int component_calls{ 0 };
		bool refuse_enemies_over{ false };
		int refuse_strength_over{ 0 };
		bool fail_on_follower{ false };
		int add_to_position_x{ 0 };

		template <typename T>
		Verdict Accept(const EntityID, T& copy)
		{
			accepted_calls++;
			if constexpr (std::is_same_v<T, TSave::Enemy>)
			{
				if (refuse_enemies_over && copy.strength > refuse_strength_over) return Verdict::Drop("too strong to keep");
			}
			return Verdict::Keep();
		}

		template <typename T>
		Verdict Component(const EntityID, T& component)
		{
			component_calls++;
			if constexpr (std::is_same_v<T, TSave::Enemy>)
			{
				if (refuse_enemies_over && component.strength > refuse_strength_over) return Verdict::Drop("too strong to load");
			}
			if constexpr (std::is_same_v<T, TSave::Follower>)
			{
				if (fail_on_follower) return Verdict::Fail("it follows nobody the game knows");
			}
			if constexpr (std::is_same_v<T, TSave::Position>)
			{
				component.x += add_to_position_x;
			}
			return Verdict::Keep();
		}
	};

	inline Data AContainer(const std::vector<EntityID>& entities)
	{
		Data data;
		data.entities = entities;
		return data;
	}

	inline std::vector<EntityID> EntitiesOf(ECS& ecs)
	{
		std::vector<EntityID> entities;
		for (const EntityID entity : ecs.Entities())
		{
			entities.push_back(entity);
		}
		return entities;
	}

	inline Decoded MustDecode(const Data& data, Log& log)
	{
		auto decoded = lcs::save::Decode<MySchema>(data, log.Reporter());
		EXPECT_TRUE(decoded.has_value()) << (decoded.has_value() ? "" : decoded.error().message);
		return decoded.has_value() ? std::move(*decoded) : Decoded{};
	}

	/* A small world with a bit of everything. Entity 1 is gone, so ids are not dense. */
	struct World
	{
		ECS ecs;
		EntityID a, gone, b, c;

		World()
		{
			a = ecs.CreateEntity();
			gone = ecs.CreateEntity();
			b = ecs.CreateEntity();
			c = ecs.CreateEntity();
			ecs.DestroyEntity(gone);

			ecs.AddComponent<TSave::Position>(a, { 1, 2 });
			ecs.AddComponent<TSave::Enemy>(a, { 30 });
			ecs.AddTag<TSave::Wet>(a);
			ecs.AddComponent<TSave::Position>(b, { 3, 4 });
			ecs.AddComponent<TSave::Follower>(b, { a });
			ecs.AddComponent<TSave::Leash>(c, { a, 7 });
		}
	};
}

/*
 *  Capture
 */

TEST(Capture, ListsTheEntitiesThatExist)
{
	TRoundTrip::World world;
	TRoundTrip::Log log;

	const auto captured = lcs::save::Capture<TSave::MySchema>(world.ecs, log.Reporter());

	ASSERT_TRUE((captured.data.entities == std::vector<TSave::EntityID>{ world.a, world.b, world.c }));
	ASSERT_TRUE(captured.dropped_components == 0);
	ASSERT_TRUE(log.lines.empty());
}

TEST(Capture, WritesEachComponentOfEachSavedTypeUnderItsEntity)
{
	TRoundTrip::World world;
	TRoundTrip::Log log;

	const auto captured = lcs::save::Capture<TSave::MySchema>(world.ecs, log.Reporter());

	const auto* positions = lcs::save::FindChunk(captured.data, 1);
	ASSERT_TRUE(positions != nullptr && positions->Count() == 2);
	ASSERT_TRUE(lcs::save::ReadComponent<TSave::Position>(*positions, 0).y == 2 || lcs::save::ReadComponent<TSave::Position>(*positions, 1).y == 2);

	const auto* wet = lcs::save::FindChunk(captured.data, 100);
	ASSERT_TRUE(wet != nullptr && wet->Count() == 1 && wet->entities[0] == world.a);

	const auto* leash = lcs::save::FindChunk(captured.data, 4);
	ASSERT_TRUE(leash != nullptr && leash->entities[0] == world.c);
	ASSERT_TRUE(lcs::save::ReadComponent<TSave::Leash>(*leash, 0).anchor == world.a);
}

TEST(Capture, LeavesOutATypeNobodyHas)
{
	TRoundTrip::World world;
	TRoundTrip::Log log;
	world.ecs.RemoveComponent<TSave::Enemy>(world.a);

	const auto captured = lcs::save::Capture<TSave::MySchema>(world.ecs, log.Reporter());

	ASSERT_TRUE(lcs::save::FindChunk(captured.data, 2) == nullptr);
}

TEST(Capture, ChangesNothingInTheECS)
{
	TRoundTrip::World world;
	TRoundTrip::Log log;

	(void)lcs::save::Capture<TSave::MySchema>(world.ecs, log.Reporter());

	ASSERT_TRUE(world.ecs.GetComponent<TSave::Enemy>(world.a).strength == 30);
	ASSERT_TRUE((TRoundTrip::EntitiesOf(world.ecs) == std::vector<TSave::EntityID>{ world.a, world.b, world.c }));
	ASSERT_TRUE(!world.ecs.HasDeferred());
}

TEST(Capture, AnEmptyECSIsAnEmptyContainer)
{
	TSave::ECS ecs;
	TRoundTrip::Log log;

	const auto captured = lcs::save::Capture<TSave::MySchema>(ecs, log.Reporter());

	ASSERT_TRUE(captured.data.entities.empty() && captured.data.chunks.empty());
}

TEST(Capture, AComponentWhoseValuesCannotBeLoadedBackIsLeftOutAndSaid)
{
	TRoundTrip::World world;
	TRoundTrip::Log log;
	world.ecs.GetComponent<TSave::Enemy>(world.a).strength = -4;

	const auto captured = lcs::save::Capture<TSave::MySchema>(world.ecs, log.Reporter());

	ASSERT_TRUE(captured.dropped_components == 1);
	ASSERT_TRUE(lcs::save::FindChunk(captured.data, 2) == nullptr);
	ASSERT_TRUE(log.Has(lcs::save::Severity::Error, "Enemy", "entity 0 not saved"));
	ASSERT_TRUE(world.ecs.HasComponent<TSave::Position>(world.a)); /* the rest of the entity is saved */
	ASSERT_TRUE(lcs::save::FindChunk(captured.data, 1) != nullptr);
}

TEST(Capture, ThePolicyCanRefuseAComponentWithAReason)
{
	TRoundTrip::World world;
	TRoundTrip::Log log;
	TRoundTrip::RecordingPolicy policy{ .refuse_enemies_over = true, .refuse_strength_over = 10 };

	const auto captured = lcs::save::Capture<TSave::MySchema>(world.ecs, policy, log.Reporter());

	ASSERT_TRUE(captured.dropped_components == 1);
	ASSERT_TRUE(lcs::save::FindChunk(captured.data, 2) == nullptr);
	ASSERT_TRUE(log.Has(lcs::save::Severity::Error, "Enemy", "too strong to keep"));
}

/* Tags are only an entity id: there is nothing to vet. */
TEST(Capture, ThePolicyIsAskedAboutPlainComponentsAndNotAboutTags)
{
	TRoundTrip::World world;
	TRoundTrip::Log log;
	TRoundTrip::RecordingPolicy policy{};

	(void)lcs::save::Capture<TSave::MySchema>(world.ecs, policy, log.Reporter());

	ASSERT_TRUE(policy.accepted_calls == 5); /* 2 positions, enemy, follower, leash: not the one tag */
}

/*
 *  Decode
 */

TEST(Decode, ComponentsComeBackTypedUnderTheirEntities)
{
	TRoundTrip::World world;
	TRoundTrip::Log log;
	const auto captured = lcs::save::Capture<TSave::MySchema>(world.ecs, log.Reporter());

	const TRoundTrip::Decoded decoded = TRoundTrip::MustDecode(captured.data, log);

	ASSERT_TRUE((decoded.entities == std::vector<TSave::EntityID>{ world.a, world.b, world.c }));
	ASSERT_TRUE(decoded.Of<TSave::Position>().size() == 2);
	ASSERT_TRUE(decoded.Of<TSave::Enemy>().size() == 1 && decoded.Of<TSave::Enemy>()[0].first == world.a);
	ASSERT_TRUE(decoded.Of<TSave::Enemy>()[0].second.strength == 30);
	ASSERT_TRUE(decoded.Of<TSave::Wet>().size() == 1);
	ASSERT_TRUE(decoded.dropped_components == 0);
	ASSERT_TRUE(log.lines.empty());
}

TEST(Decode, ThroughTheBytesOfAFileToo)
{
	TRoundTrip::World world;
	TRoundTrip::Log log;
	const auto captured = lcs::save::Capture<TSave::MySchema>(world.ecs, log.Reporter());
	const std::vector<std::byte> bytes = lcs::save::Encode(captured.data, TRoundTrip::format);

	const auto parsed = lcs::save::Parse<TSave::EntityID>(bytes, TRoundTrip::format, TSave::MySchema::entries);
	ASSERT_TRUE(parsed.has_value()) << parsed.error().message;
	const TRoundTrip::Decoded decoded = TRoundTrip::MustDecode(*parsed, log);

	ASSERT_TRUE(decoded.Of<TSave::Leash>().size() == 1 && decoded.Of<TSave::Leash>()[0].second.anchor == world.a);
}

TEST(Decode, AComponentOfAnEntityTheSaveDoesNotListIsAnError)
{
	TRoundTrip::Data data = TRoundTrip::AContainer({ TSave::EntityID::CreateNew(0) });
	lcs::save::AddComponent<TSave::MySchema>(data, TSave::EntityID::CreateNew(5), TSave::Position{ 1, 1 });
	TRoundTrip::Log log;

	const auto decoded = lcs::save::Decode<TSave::MySchema>(data, log.Reporter());

	ASSERT_TRUE(!decoded.has_value());
	ASSERT_TRUE(decoded.error().kind == lcs::save::ErrorKind::Malformed);
	ASSERT_TRUE(decoded.error().message.find("entity 5") != std::string::npos);
}

TEST(Decode, ValuesTheTraitsRejectAreAnError)
{
	TRoundTrip::Data data = TRoundTrip::AContainer({ TSave::EntityID::CreateNew(3) });
	lcs::save::AddComponent<TSave::MySchema>(data, data.entities[0], TSave::Enemy{ -1 });
	TRoundTrip::Log log;

	const auto decoded = lcs::save::Decode<TSave::MySchema>(data, log.Reporter());

	ASSERT_TRUE(!decoded.has_value());
	ASSERT_TRUE(decoded.error().kind == lcs::save::ErrorKind::Malformed);
	ASSERT_TRUE(decoded.error().message.find("Enemy of entity 3") != std::string::npos);
}

TEST(Decode, ThePolicyCanChangeAComponentOnTheWayIn)
{
	TRoundTrip::World world;
	TRoundTrip::Log log;
	const auto captured = lcs::save::Capture<TSave::MySchema>(world.ecs, log.Reporter());
	TRoundTrip::RecordingPolicy policy{ .add_to_position_x = 100 };

	const auto decoded = lcs::save::Decode<TSave::MySchema>(captured.data, policy, log.Reporter());

	ASSERT_TRUE(decoded.has_value());
	for (const auto& [entity, position] : decoded->Of<TSave::Position>())
	{
		ASSERT_TRUE(position.x >= 100);
	}
}

TEST(Decode, ThePolicyCanDropAComponentWithAReason)
{
	TRoundTrip::World world;
	TRoundTrip::Log log;
	const auto captured = lcs::save::Capture<TSave::MySchema>(world.ecs, log.Reporter());
	TRoundTrip::RecordingPolicy policy{ .refuse_enemies_over = true, .refuse_strength_over = 10 };

	const auto decoded = lcs::save::Decode<TSave::MySchema>(captured.data, policy, log.Reporter());

	ASSERT_TRUE(decoded.has_value());
	ASSERT_TRUE(decoded->Of<TSave::Enemy>().empty());
	ASSERT_TRUE(decoded->dropped_components == 1);
	ASSERT_TRUE(log.Has(lcs::save::Severity::Error, "Enemy", "entity 0 loaded without it: too strong to load"));
	ASSERT_TRUE(decoded->Of<TSave::Position>().size() == 2); /* the rest loads */
}

TEST(Decode, ThePolicyCanFailTheWholeDecode)
{
	TRoundTrip::World world;
	TRoundTrip::Log log;
	const auto captured = lcs::save::Capture<TSave::MySchema>(world.ecs, log.Reporter());
	TRoundTrip::RecordingPolicy policy{ .fail_on_follower = true };

	const auto decoded = lcs::save::Decode<TSave::MySchema>(captured.data, policy, log.Reporter());

	ASSERT_TRUE(!decoded.has_value());
	ASSERT_TRUE(decoded.error().kind == lcs::save::ErrorKind::Malformed);
	ASSERT_TRUE(decoded.error().message.find("Follower of entity 2: it follows nobody the game knows") != std::string::npos);
}

TEST(Decode, ThePolicyIsNotAskedAboutTags)
{
	TRoundTrip::Data data = TRoundTrip::AContainer({ TSave::EntityID::CreateNew(0) });
	lcs::save::AddTag<TSave::MySchema, TSave::Wet>(data, data.entities[0]);
	TRoundTrip::Log log;
	TRoundTrip::RecordingPolicy policy{};

	const auto decoded = lcs::save::Decode<TSave::MySchema>(data, policy, log.Reporter());

	ASSERT_TRUE(decoded.has_value());
	ASSERT_TRUE(policy.component_calls == 0);
	ASSERT_TRUE(decoded->Of<TSave::Wet>().size() == 1);
}

/* The target is not in the save: the follower goes on following nobody. */
TEST(Decode, AnEntityIdTheSaveDoesNotListIsClearedWhereTheComponentSurvivesWithoutIt)
{
	TRoundTrip::Data data = TRoundTrip::AContainer({ TSave::EntityID::CreateNew(0) });
	lcs::save::AddComponent<TSave::MySchema>(data, data.entities[0], TSave::Follower{ TSave::EntityID::CreateNew(9) });
	TRoundTrip::Log log;

	const TRoundTrip::Decoded decoded = TRoundTrip::MustDecode(data, log);

	ASSERT_TRUE(decoded.Of<TSave::Follower>().size() == 1);
	ASSERT_TRUE(!decoded.Of<TSave::Follower>()[0].second.target.IsValid());
	ASSERT_TRUE(decoded.dropped_components == 0);
}

TEST(Decode, AnEntityIdTheSaveListsIsLeftAlone)
{
	TRoundTrip::Data data = TRoundTrip::AContainer({ TSave::EntityID::CreateNew(0), TSave::EntityID::CreateNew(2) });
	lcs::save::AddComponent<TSave::MySchema>(data, data.entities[0], TSave::Follower{ data.entities[1] });
	TRoundTrip::Log log;

	const TRoundTrip::Decoded decoded = TRoundTrip::MustDecode(data, log);

	ASSERT_TRUE(decoded.Of<TSave::Follower>()[0].second.target == data.entities[1]);
}

TEST(Decode, AnEntityIdThatWasNeverSetStaysUnset)
{
	TRoundTrip::Data data = TRoundTrip::AContainer({ TSave::EntityID::CreateNew(0) });
	lcs::save::AddComponent<TSave::MySchema>(data, data.entities[0], TSave::Follower{ TSave::EntityID{} });
	lcs::save::AddComponent<TSave::MySchema>(data, data.entities[0], TSave::Leash{ TSave::EntityID{}, 3 });
	TRoundTrip::Log log;

	const TRoundTrip::Decoded decoded = TRoundTrip::MustDecode(data, log);

	ASSERT_TRUE(decoded.Of<TSave::Follower>().size() == 1);
	ASSERT_TRUE(decoded.Of<TSave::Leash>().size() == 1); /* it names nothing, so it is not missing anything */
	ASSERT_TRUE(decoded.dropped_components == 0);
}

/* The anchor is not in the save: a leash with no anchor means nothing. */
TEST(Decode, AnEntityIdTheSaveDoesNotListDropsAComponentThatNeedsIt)
{
	TRoundTrip::Data data = TRoundTrip::AContainer({ TSave::EntityID::CreateNew(0) });
	lcs::save::AddComponent<TSave::MySchema>(data, data.entities[0], TSave::Leash{ TSave::EntityID::CreateNew(9), 3 });
	TRoundTrip::Log log;

	const TRoundTrip::Decoded decoded = TRoundTrip::MustDecode(data, log);

	ASSERT_TRUE(decoded.Of<TSave::Leash>().empty());
	ASSERT_TRUE(decoded.dropped_components == 1);
	ASSERT_TRUE(log.Has(lcs::save::Severity::Error, "Leash", "the entity it refers to is not in the save"));
}

TEST(Decode, AComponentThisBuildDoesNotKnowIsReportedAsAWarningAndTheRestLoads)
{
	TRoundTrip::Data data = TRoundTrip::AContainer({ TSave::EntityID::CreateNew(0) });
	lcs::save::AddComponent<TSave::MySchema>(data, data.entities[0], TSave::Position{ 1, 1 });
	data.skipped_component_ids.push_back(9999);
	TRoundTrip::Log log;

	const TRoundTrip::Decoded decoded = TRoundTrip::MustDecode(data, log);

	ASSERT_TRUE(log.Has(lcs::save::Severity::Warning, "save", "9999"));
	ASSERT_TRUE(decoded.Of<TSave::Position>().size() == 1);
}

TEST(Decode, DecodingTouchesNoECS)
{
	TRoundTrip::World world;
	TRoundTrip::Log log;
	const auto captured = lcs::save::Capture<TSave::MySchema>(world.ecs, log.Reporter());

	(void)TRoundTrip::MustDecode(captured.data, log);

	ASSERT_TRUE(world.ecs.GetComponent<TSave::Position>(world.a).x == 1);
	ASSERT_TRUE(!world.ecs.HasDeferred());
}

/*
 *  Restore
 */

TEST(Restore, PutsEveryEntityBackUnderItsOwnId)
{
	TRoundTrip::World world;
	TRoundTrip::Log log;
	const auto captured = lcs::save::Capture<TSave::MySchema>(world.ecs, log.Reporter());
	const TRoundTrip::Decoded decoded = TRoundTrip::MustDecode(captured.data, log);
	TSave::ECS target;

	lcs::save::Restore(target, decoded);
	target.FlushDeferred();

	ASSERT_TRUE(TRoundTrip::EntitiesOf(target) == TRoundTrip::EntitiesOf(world.ecs));
	ASSERT_TRUE(target.GetComponent<TSave::Position>(world.b).x == 3);
	ASSERT_TRUE(target.GetComponent<TSave::Enemy>(world.a).strength == 30);
	ASSERT_TRUE(target.HasTag<TSave::Wet>(world.a));
	ASSERT_TRUE(target.GetComponent<TSave::Follower>(world.b).target == world.a);
	ASSERT_TRUE(target.GetComponent<TSave::Leash>(world.c).anchor == world.a);
	ASSERT_TRUE(!target.HasComponent<TSave::Position>(world.c));
}

/* Nothing is flushed: the caller decides when the components arrive. */
TEST(Restore, DefersTheComponentsAndFlushesNothing)
{
	TRoundTrip::World world;
	TRoundTrip::Log log;
	const TRoundTrip::Decoded decoded = TRoundTrip::MustDecode(lcs::save::Capture<TSave::MySchema>(world.ecs, log.Reporter()).data, log);
	TSave::ECS target;

	lcs::save::Restore(target, decoded);

	ASSERT_TRUE(target.HasDeferred());
	ASSERT_TRUE(!target.HasComponent<TSave::Position>(world.a));
	ASSERT_TRUE(TRoundTrip::EntitiesOf(target).size() == 3);
}

TEST(Restore, AnIdThatHasBeenReusedKeepsItsValidationBits)
{
	TSave::ECS source;
	source.DestroyEntity(source.CreateEntity());
	const TSave::EntityID reused = source.CreateEntity();
	ASSERT_TRUE(reused.GetValidationID() != 0);
	source.AddComponent<TSave::Position>(reused, { 8, 9 });
	TRoundTrip::Log log;
	const TRoundTrip::Decoded decoded = TRoundTrip::MustDecode(lcs::save::Capture<TSave::MySchema>(source, log.Reporter()).data, log);
	TSave::ECS target;

	lcs::save::Restore(target, decoded);
	target.FlushDeferred();

	ASSERT_TRUE(target.HasComponent<TSave::Position>(reused));
	ASSERT_TRUE(target.GetComponent<TSave::Position>(reused).y == 9);
}

TEST(Restore, TheCallerChoosesWhatIsDeferred)
{
	TRoundTrip::World world;
	TRoundTrip::Log log;
	const TRoundTrip::Decoded decoded = TRoundTrip::MustDecode(lcs::save::Capture<TSave::MySchema>(world.ecs, log.Reporter()).data, log);
	TSave::ECS target;
	int visited = 0;

	lcs::save::Restore(target, decoded, [&](const TSave::EntityID entity, const auto& component) {
		visited++;
		using T = std::remove_cvref_t<decltype(component)>;
		if constexpr (std::is_same_v<T, TSave::Enemy>)
		{
			target.AddDeferred(entity, TSave::Enemy{ component.strength * 2 });
		}
		else if constexpr (!std::is_same_v<T, TSave::Leash>)
		{
			target.AddDeferred(entity, component);
		}
	});
	target.FlushDeferred();

	ASSERT_TRUE(visited == 6); /* 2 positions, enemy, follower, leash, wet */
	ASSERT_TRUE(target.GetComponent<TSave::Enemy>(world.a).strength == 60);
	ASSERT_TRUE(!target.HasComponent<TSave::Leash>(world.c));
	ASSERT_TRUE(target.HasComponent<TSave::Follower>(world.b));
}

TEST(Restore, VisitsInTheSchemasOrder)
{
	TRoundTrip::World world;
	TRoundTrip::Log log;
	const TRoundTrip::Decoded decoded = TRoundTrip::MustDecode(lcs::save::Capture<TSave::MySchema>(world.ecs, log.Reporter()).data, log);
	TSave::ECS target;
	std::vector<std::string> order;

	lcs::save::Restore(target, decoded, [&](const TSave::EntityID, const auto& component) {
		order.push_back(TSave::Traits<std::remove_cvref_t<decltype(component)>>::name);
	});

	ASSERT_TRUE((order == std::vector<std::string>{ "Position", "Position", "Enemy", "Follower", "Leash", "Wet" }));
}

/* What a world reload leaves: a cleared ECS. */
TEST(Restore, WorksAgainOnceTheECSIsCleared)
{
	TRoundTrip::World world;
	TRoundTrip::Log log;
	const TRoundTrip::Decoded decoded = TRoundTrip::MustDecode(lcs::save::Capture<TSave::MySchema>(world.ecs, log.Reporter()).data, log);
	TSave::ECS target;
	lcs::save::Restore(target, decoded);
	target.FlushDeferred();

	target.Clear();
	lcs::save::Restore(target, decoded);
	target.FlushDeferred();

	ASSERT_TRUE(TRoundTrip::EntitiesOf(target) == TRoundTrip::EntitiesOf(world.ecs));
	ASSERT_TRUE(target.GetComponent<TSave::Position>(world.a).y == 2);
}

/* The whole trip: an ECS, to bytes, to a different ECS that holds the same thing. */
TEST(RoundTrip, AWorldThroughTheBytesOfAFileComesBackTheSame)
{
	TRoundTrip::World world;
	TRoundTrip::Log log;
	auto captured = lcs::save::Capture<TSave::MySchema>(world.ecs, log.Reporter());
	const std::vector<std::byte> bytes = lcs::save::Encode(captured.data, TRoundTrip::format);

	const auto parsed = lcs::save::Parse<TSave::EntityID>(bytes, TRoundTrip::format, TSave::MySchema::entries);
	ASSERT_TRUE(parsed.has_value()) << parsed.error().message;
	const TRoundTrip::Decoded decoded = TRoundTrip::MustDecode(*parsed, log);
	TSave::ECS target;
	lcs::save::Restore(target, decoded);
	target.FlushDeferred();

	ASSERT_TRUE(TRoundTrip::EntitiesOf(target) == TRoundTrip::EntitiesOf(world.ecs));
	ASSERT_TRUE(target.GetComponent<TSave::Position>(world.a).x == 1 && target.GetComponent<TSave::Position>(world.b).y == 4);
	ASSERT_TRUE(target.GetComponent<TSave::Enemy>(world.a).strength == 30);
	ASSERT_TRUE(target.HasTag<TSave::Wet>(world.a) && !target.HasTag<TSave::Wet>(world.b));
	ASSERT_TRUE(target.GetComponent<TSave::Follower>(world.b).target == world.a);
	ASSERT_TRUE(log.lines.empty());

	/* and a second capture of the copy is the same bytes */
	TRoundTrip::Log second_log;
	auto again = lcs::save::Capture<TSave::MySchema>(target, second_log.Reporter());
	ASSERT_TRUE(lcs::save::Encode(again.data, TRoundTrip::format).size() == bytes.size());
}
