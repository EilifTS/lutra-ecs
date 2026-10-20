#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>

#include <lutra-ecs/ECSManager.h>
#include <lutra-ecs/TypeList.h>
#include <lutra-ecs/save/Schema.h>

/**
 *  A small ECS to save: some plain components, two that hold entity ids (one that
 *  survives losing its target, one that does not), a value that can be out of
 *  range, a saved tag, and two types that are never saved.
 */
namespace TSave
{
	using EntityID = lcs::Handle<uint32_t, 16>;

	struct Position
	{
		static constexpr lcs::ComponentType component_type = lcs::ComponentType::Component;
		int x, y;
	};

	/* strength must not be negative: what Valid is for. */
	struct Enemy
	{
		static constexpr lcs::ComponentType component_type = lcs::ComponentType::Component;
		int strength;
	};

	/* Can live without its target, if whoever saves it clears a dead one (ClearDeadRefs). */
	struct Follower
	{
		static constexpr lcs::ComponentType component_type = lcs::ComponentType::Component;
		EntityID target;
	};

	/* Means nothing without its anchor, so nothing clears a dead one: it is dropped on load. */
	struct Leash
	{
		static constexpr lcs::ComponentType component_type = lcs::ComponentType::Component;
		EntityID anchor;
		int length;
	};

	struct Wet
	{
		static constexpr lcs::ComponentType component_type = lcs::ComponentType::Tag;
	};

	/* Never saved: gone by the time a save runs. */
	struct Intent
	{
		static constexpr lcs::ComponentType component_type = lcs::ComponentType::Component;
		int direction;
	};

	struct Pending
	{
		static constexpr lcs::ComponentType component_type = lcs::ComponentType::Tag;
	};

	using ECS = lcs::ECSManager<EntityID, Position, Enemy, Follower, Leash, Wet, Intent, Pending>;

	template <typename T>
	T Load(const std::span<const std::byte> raw)
	{
		T value;
		std::memcpy(&value, raw.data(), sizeof(T));
		return value;
	}

	template <typename T> struct Traits;

	template <> struct Traits<Position> : lcs::save::NoRules
	{
		static constexpr uint32_t id = 1;
		static constexpr const char* name = "Position";
	};

	template <> struct Traits<Enemy> : lcs::save::NoRules
	{
		static constexpr uint32_t id = 2;
		static constexpr const char* name = "Enemy";
		static bool Valid(const std::span<const std::byte> raw) { return Load<Enemy>(raw).strength >= 0; }
	};

	template <> struct Traits<Follower> : lcs::save::NoRules
	{
		static constexpr uint32_t id = 3;
		static constexpr const char* name = "Follower";
		template <typename C, typename V> static void ForEachEntityRef(C& c, V&& visit) { visit(c.target); }
	};

	template <> struct Traits<Leash> : lcs::save::NoRules
	{
		static constexpr uint32_t id = 4;
		static constexpr const char* name = "Leash";
		template <typename C, typename V> static void ForEachEntityRef(C& c, V&& visit) { visit(c.anchor); }
	};

	template <> struct Traits<Wet> : lcs::save::NoRules
	{
		static constexpr uint32_t id = 100;
		static constexpr const char* name = "Wet";
	};

	using SavedTypes = lcs::TypeList<Position, Enemy, Follower, Leash, Wet>;
	using TransientTypes = lcs::TypeList<Intent, Pending>;
	using MySchema = lcs::save::Schema<ECS, Traits, SavedTypes, TransientTypes>;
}
