#pragma once
#include <lutra-ecs/SparseSet.h>
#include <lutra-ecs/SparseSetChunked.h>
#include <lutra-ecs/HandleFreeList.h>
#include <lutra-ecs/SparseTagSet.h>
#include <lutra-ecs/TypeList.h>
#include <lutra-ecs/Views.h>
#include <algorithm>
#include <array>
#include <cassert>
#include <span>
#include <type_traits>
#include <utility>
#include <vector>
#include <tuple>

namespace lcs
{
	template <typename handle_t, typename... Ts>
	class ECSManager
	{
	public:
		using EntityID = handle_t;

		/* Every component and tag the ECS holds, for code that has to visit them all. */
		using Types = TypeList<Ts...>;

		ECSManager() { reserveComponentStorage(reserved_component_count); };

		inline EntityID CreateEntity();

		/* Creates entities with exactly these ids, validation bits included. For
		 * restoring a saved world only: nothing may have been created since the
		 * ECS was constructed or Clear()ed, and `ids` must be valid and strictly
		 * ascending by index. Indices below the last one that are not in `ids`
		 * are free, and CreateEntity fills them (lowest first) before it grows. */
		inline void CreateEntitiesFromIDs(std::span<const EntityID> ids);

		inline void DestroyEntity(EntityID entity);
		inline const HandleFreeList<EntityID>& Entities() { return entity_id_generator; };
		//inline const IndexFreeList::OccupiedIndicesContainer<true> EntitiesReverse() { return entity_id_generator.OccupiedIndicesReverse(); };
		inline EntityID::data_t GetEntityCount();

		/* Whether this exact id belongs to an entity that exists: not destroyed, and not
		 * an index that has been reused since. */
		[[nodiscard]] inline bool IsAlive(EntityID id) const { return entity_id_generator.IsAlive(id); }

		/* Component */
		template <typename T> inline bool HasComponent(EntityID id);
		template <typename T> inline T& GetComponent(EntityID id);
		template <typename T> inline std::remove_reference<T>::type& AddComponent(EntityID id, T&& component);
		template <typename T> inline void RemoveComponent(EntityID id);
		template <typename T> inline void ClearComponent();
		template <typename T> inline EntityID::data_t GetComponentCount();

		/* No entity has a T. For a component or a tag alike. */
		template <typename T> [[nodiscard]] inline bool IsEmpty();
		template <typename T> inline std::span<T> GetComponentData();
		template <typename T> inline ComponentView<EntityID, T> CView();

		/* Tag */
		template <typename T> inline bool HasTag(EntityID id);
		template <typename T> inline void AddTag(EntityID id);
		template <typename T> inline void RemoveTag(EntityID id);
		template <typename T> inline void ClearTag();
		template <typename T> inline TagView<EntityID, T> TView();
		/* template <typename T> inline u32 GetTagCount(); */

		/* Deferred adds
		 *
		 * AddDeferred() takes the component or tag now and puts it on the entity at
		 * the next FlushDeferred(), which belongs at one known point of the frame and
		 * never inside a view iteration: adding a component while a view over the
		 * same type is being walked would invalidate it. That lets an entity be made
		 * and described anywhere (a parser, a loader) with the ids already in hand --
		 * stored in another component, say -- while the storage stays untouched.
		 *
		 * Between CreateEntity() and the flush the entity is valid and bare: every
		 * HasComponent() is false. Don't read through it.
		 *
		 * Any component or tag of the ECS can be deferred, at most one of a type per
		 * entity. Flush adds them type by type in the order the ECS lists its types;
		 * an entity's components are independent of each other, so the order matters
		 * to nothing. Destroying an entity does not drop what is deferred for it.
		 *
		 *     ecs.AddDeferred(entity, Position{ .x = 1 });
		 *     ecs.AddDeferred<IsPlayer>(entity);
		 *     ecs.FlushDeferred();  // the entity has a Position and the IsPlayer tag */
		template <typename T> inline void AddDeferred(EntityID id, T value = {});
		inline void FlushDeferred();

		/* True when anything is deferred, in any type. Creating an entity defers nothing. */
		[[nodiscard]] inline bool HasDeferred() const;

		/* Drops everything deferred. Clear() does it too: clearing resets the entity
		 * handle free list, so a deferred id would collide with a freshly issued one. */
		inline void ClearDeferred();

		inline void Clear();

	private:
		inline void reserveComponentStorage(EntityID::data_t new_size);
		inline void growComponentStorageIfNecessary();

		template <typename T>
		using DeferredEntries = std::vector<std::pair<EntityID, T>>;

	private:
		HandleFreeList<EntityID> entity_id_generator{};
		std::tuple<typename internal_ecs::GetComponentContainer<EntityID, Ts, Ts::component_type>::Container... > component_sets{};
		std::tuple<DeferredEntries<Ts>...> deferred{};
		EntityID::data_t reserved_component_count{ 8 };
		static constexpr uint32_t component_grow_factor = 2;
	};

	template <typename EntityID, typename... Ts>
	inline EntityID ECSManager<EntityID, Ts...>::CreateEntity()
	{
		growComponentStorageIfNecessary();
		return entity_id_generator.GetNextHandle();
	}

	template <typename EntityID, typename... Ts>
	inline void ECSManager<EntityID, Ts...>::CreateEntitiesFromIDs(std::span<const EntityID> ids)
	{
		assert(entity_id_generator.MaxIndex() == 0 && "CreateEntitiesFromIDs needs an ECS with no entities ever created since Clear()");
		assert(!HasDeferred() && "CreateEntitiesFromIDs needs nothing deferred");

		entity_id_generator.Restore(ids);

		/* The sparse arrays must reach every index in use. CreateEntity grows them
		 * when they are exactly full, so only the size has to be at least the
		 * list's. Sizes only ever grow (ReserveSparseSize asserts on a no-op). */
		const EntityID::data_t needed = entity_id_generator.MaxIndex();
		if (needed > reserved_component_count)
		{
			reserved_component_count = needed;
			reserveComponentStorage(reserved_component_count);
		}
	}

	template <typename EntityID, typename... Ts>
	inline void ECSManager<EntityID, Ts...>::DestroyEntity(EntityID id)
	{
		/* Remove components */
		( std::get<typename internal_ecs::GetComponentContainer<EntityID, Ts, Ts::component_type>::Container>(component_sets).RemoveIfPresent(id), ...);

		entity_id_generator.FreeHandle(id);
	}

	template <typename EntityID, typename... Ts>
	inline EntityID::data_t ECSManager<EntityID, Ts...>::GetEntityCount()
	{
		return entity_id_generator.UsedIndexCount();
	}

	template <typename EntityID, typename... Ts> template <typename T>
	inline bool ECSManager<EntityID, Ts...>::HasComponent(EntityID id)
	{
		using SetType = typename internal_ecs::GetComponentContainer<EntityID, T, T::component_type>::Container;
		SetType& set = std::get<SetType>(component_sets);
		return set.Has(id);
	}

	template <typename EntityID, typename... Ts> template <typename T>
	inline T& ECSManager<EntityID, Ts...>::GetComponent(EntityID id)
	{
		using SetType = typename internal_ecs::GetComponentContainer<EntityID, T, T::component_type>::Container;
		SetType& set = std::get<SetType>(component_sets);
		return set.Get(id);
	}

	template <typename EntityID, typename... Ts> template <typename T>
	inline std::remove_reference<T>::type& ECSManager<EntityID, Ts...>::AddComponent(EntityID id, T&& component)
	{
		using Tp = std::remove_reference<T>::type;
		using SetType = typename internal_ecs::GetComponentContainer<EntityID, T, T::component_type>::Container;
		SetType& set = std::get<SetType>(component_sets);
		set.Add(id, std::forward<T>(component));
		return GetComponent<Tp>(id);
	}

	template <typename EntityID, typename... Ts> template <typename T>
	inline void ECSManager<EntityID, Ts...>::RemoveComponent(EntityID id)
	{
		assert(HasComponent<T>(id));
		using SetType = typename internal_ecs::GetComponentContainer<EntityID, T, T::component_type>::Container;
		SetType& set = std::get<SetType>(component_sets);
		return set.Remove(id);
	}

	template <typename EntityID, typename... Ts> template <typename T>
	inline void ECSManager<EntityID, Ts...>::ClearComponent()
	{
		static_assert(T::component_type != ComponentType::Tag, "ClearComponent is not supported for tag types, use ClearTag instead");
		using SetType = typename internal_ecs::GetComponentContainer<EntityID, T, T::component_type>::Container;
		SetType& set = std::get<SetType>(component_sets);
		set.ClearDense();
	}

	template <typename EntityID, typename... Ts> template <typename T>
	inline EntityID::data_t ECSManager<EntityID, Ts...>::GetComponentCount()
	{
		using SetType = typename internal_ecs::GetComponentContainer<EntityID, T, T::component_type>::Container;
		SetType& set = std::get<SetType>(component_sets);
		return set.DenseSize();
	}

	template <typename EntityID, typename... Ts> template <typename T>
	inline bool ECSManager<EntityID, Ts...>::IsEmpty()
	{
		if constexpr (T::component_type == ComponentType::Tag)
		{
			TagView<EntityID, T> view = TView<T>();
			return view.begin() == view.end();
		}
		else
		{
			return GetComponentCount<T>() == 0;
		}
	}

	template <typename EntityID, typename... Ts> template <typename T>
	inline std::span<T> ECSManager<EntityID, Ts...>::GetComponentData()
	{
		static_assert(T::component_type == ComponentType::Component, "GetComponentData is not supported for chunked or tag component types, since their dense storage is not one contiguous array");
		using SetType = typename internal_ecs::GetComponentContainer<EntityID, T, T::component_type>::Container;
		SetType& set = std::get<SetType>(component_sets);
		return set.Data();
	}

	template <typename EntityID, typename... Ts> template <typename T>
	inline ComponentView<EntityID, T> ECSManager<EntityID, Ts...>::CView()
	{
		using SetType = typename internal_ecs::GetComponentContainer<EntityID, T, T::component_type>::Container;
		SetType& set = std::get<SetType>(component_sets);
		return ComponentView<EntityID, T>(set);
	}

	template <typename EntityID, typename... Ts> template <typename T>
	bool ECSManager<EntityID, Ts...>::HasTag(EntityID id)
	{
		SparseTagSetT<EntityID, T>& set = std::get<SparseTagSetT<EntityID, T>>(component_sets);
		return set.Has(id);
	}

	template <typename EntityID, typename... Ts> template <typename T>
	void ECSManager<EntityID, Ts...>::AddTag(EntityID id)
	{
		SparseTagSetT<EntityID, T>& set = std::get<SparseTagSetT<EntityID, T>>(component_sets);
		return set.Add(id);
	}

	template <typename EntityID, typename... Ts> template <typename T>
	void ECSManager<EntityID, Ts...>::RemoveTag(EntityID id)
	{
		SparseTagSetT<EntityID, T>& set = std::get<SparseTagSetT<EntityID, T>>(component_sets);
		return set.Remove(id);
	}

	template <typename EntityID, typename... Ts> template <typename T>
	void ECSManager<EntityID, Ts...>::ClearTag()
	{
		SparseTagSetT<EntityID, T>& set = std::get<SparseTagSetT<EntityID, T>>(component_sets);
		set.ClearDense();
	}

	template <typename EntityID, typename... Ts> template <typename T>
	inline TagView<EntityID, T> ECSManager<EntityID, Ts...>::TView()
	{
		SparseTagSetT<EntityID, T>& set = std::get<SparseTagSetT<EntityID, T>>(component_sets);
		return TagView<EntityID, T>(set);
	}

	/*template <typename... Ts, typename T>
	inline u32 ECSManager<Ts...>::GetTagCount()
	{
		SparseTagSetT<T>& set = std::get<SparseTagSetT<T>>(component_sets);
		return set.Size();
	}*/

	template <typename EntityID, typename... Ts> template <typename T>
	inline void ECSManager<EntityID, Ts...>::AddDeferred(EntityID id, T value)
	{
		static_assert((std::is_same_v<T, Ts> || ...), "this type is not one of the ECS's components or tags");

		std::get<DeferredEntries<T>>(deferred).emplace_back(id, std::move(value));
	}

	template <typename EntityID, typename... Ts>
	inline void ECSManager<EntityID, Ts...>::FlushDeferred()
	{
		([&]
		{
			auto& entries = std::get<DeferredEntries<Ts>>(deferred);
			for (auto& [id, value] : entries)
			{
				if constexpr (Ts::component_type == ComponentType::Tag)  AddTag<Ts>(id);
				else                                                     AddComponent<Ts>(id, std::move(value));
			}
			entries.clear();
		}(), ...);
	}

	template <typename EntityID, typename... Ts>
	inline bool ECSManager<EntityID, Ts...>::HasDeferred() const
	{
		return (!std::get<DeferredEntries<Ts>>(deferred).empty() || ...);
	}

	template <typename EntityID, typename... Ts>
	inline void ECSManager<EntityID, Ts...>::ClearDeferred()
	{
		(std::get<DeferredEntries<Ts>>(deferred).clear(), ...);
	}

	template <typename EntityID, typename... Ts>
	void ECSManager<EntityID, Ts...>::Clear()
	{
		(std::get<typename internal_ecs::GetComponentContainer<EntityID, Ts, Ts::component_type>::Container>(component_sets).Clear(), ...);
		ClearDeferred();
		entity_id_generator.Clear();

		reserved_component_count = 8;
		reserveComponentStorage(reserved_component_count);
	}

	template <typename EntityID, typename... Ts>
	void ECSManager<EntityID, Ts...>::reserveComponentStorage(EntityID::data_t new_size)
	{
		(std::get<typename internal_ecs::GetComponentContainer<EntityID, Ts, Ts::component_type>::Container>(component_sets).ReserveSparseSize(new_size), ...);
	}

	template <typename EntityID, typename... Ts>
	void ECSManager<EntityID, Ts...>::growComponentStorageIfNecessary()
	{
		if (reserved_component_count == entity_id_generator.UsedIndexCount())
		{
			reserved_component_count *= component_grow_factor;
			reserveComponentStorage(reserved_component_count);
		}
	}
}