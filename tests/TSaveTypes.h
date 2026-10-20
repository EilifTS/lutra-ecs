#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>

#include <lutra-ecs/ECSManager.h>
#include <lutra-ecs/TypeList.h>
#include <lutra-ecs/save/Schema.h>

/**
 *  A small ECS to save: some plain components, two that hold entity ids, a value
 *  that can be out of range, a saved tag, and two types that are never saved. Each
 *  says for itself what a save needs to know.
 */
namespace TSave
{
	using EntityID = lcs::Handle<uint32_t, 16>;

	/* Nothing to say but its id. */
	struct Position
	{
		static constexpr lcs::ComponentType component_type = lcs::ComponentType::Component;
		static constexpr bool saved = true;
		static constexpr uint32_t save_id = 1;
		static constexpr const char* save_name = "Position";

		int x, y;
	};

	/* strength must not be negative: what SaveBytesValid is for. */
	struct Enemy
	{
		static constexpr lcs::ComponentType component_type = lcs::ComponentType::Component;
		static constexpr bool saved = true;
		static constexpr uint32_t save_id = 2;
		static constexpr const char* save_name = "Enemy";

		int strength;

		static bool SaveBytesValid(const std::span<const std::byte> raw)
		{
			Enemy value;
			std::memcpy(&value, raw.data(), sizeof(Enemy));
			return value.strength >= 0;
		}
	};

	/* Can live without its target, if whoever saves it clears a dead one (ClearDeadRefs). */
	struct Follower
	{
		static constexpr lcs::ComponentType component_type = lcs::ComponentType::Component;
		static constexpr bool saved = true;
		static constexpr uint32_t save_id = 3;
		static constexpr const char* save_name = "Follower";

		EntityID target;

		template <typename V> void ForEachEntityRef(V&& visit) { visit(target); }
	};

	/* Means nothing without its anchor, so nothing clears a dead one: it is dropped on load. */
	struct Leash
	{
		static constexpr lcs::ComponentType component_type = lcs::ComponentType::Component;
		static constexpr bool saved = true;
		static constexpr uint32_t save_id = 4;
		static constexpr const char* save_name = "Leash";

		EntityID anchor;
		int length;

		template <typename V> void ForEachEntityRef(V&& visit) { visit(anchor); }
	};

	struct Wet
	{
		static constexpr lcs::ComponentType component_type = lcs::ComponentType::Tag;
		static constexpr bool saved = true;
		static constexpr uint32_t save_id = 100;
		static constexpr const char* save_name = "Wet";
	};

	/* Never saved: gone by the time a save runs. */
	struct Intent
	{
		static constexpr lcs::ComponentType component_type = lcs::ComponentType::Component;
		static constexpr bool saved = false;

		int direction;
	};

	struct Pending
	{
		static constexpr lcs::ComponentType component_type = lcs::ComponentType::Tag;
		static constexpr bool saved = false;
	};

	using ECS = lcs::ECSManager<EntityID, Position, Enemy, Follower, Leash, Wet, Intent, Pending>;

	using MySchema = lcs::save::Schema<ECS>;
}
