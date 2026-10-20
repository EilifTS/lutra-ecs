#pragma once
#include <lutra-ecs/SparseSet.h>
#include <lutra-ecs/SparseSetChunked.h>
#include <lutra-ecs/HandleFreeList.h>
#include <lutra-ecs/SparseTagSet.h>
#include <lutra-ecs/Views.h>
#include <algorithm>
#include <array>
#include <span>
#include <type_traits>
#include <vector>
#include <tuple>

namespace lcs
{
	template <typename handle_t, typename... Ts>
	class ECSManager
	{
	public:
		using EntityID = handle_t;

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

		/* Component */
		template <typename T> inline bool HasComponent(EntityID id);
		template <typename T> inline T& GetComponent(EntityID id);
		template <typename T> inline std::remove_reference<T>::type& AddComponent(EntityID id, T&& component);
		template <typename T> inline void RemoveComponent(EntityID id);
		template <typename T> inline void ClearComponent();
		template <typename T> inline EntityID::data_t GetComponentCount();
		template <typename T> inline std::span<T> GetComponentData();
		template <typename T> inline ComponentView<EntityID, T> CView();

		/* Tag */
		template <typename T> inline bool HasTag(EntityID id);
		template <typename T> inline void AddTag(EntityID id);
		template <typename T> inline void RemoveTag(EntityID id);
		template <typename T> inline void ClearTag();
		template <typename T> inline TagView<EntityID, T> TView();
		/* template <typename T> inline u32 GetTagCount(); */

		inline void Clear();

	private:
		inline void reserveComponentStorage(EntityID::data_t new_size);
		inline void growComponentStorageIfNecessary();

	private:
		HandleFreeList<EntityID> entity_id_generator{};
		std::tuple<typename internal_ecs::GetComponentContainer<EntityID, Ts, Ts::component_type>::Container... > component_sets{};
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

	template <typename EntityID, typename... Ts>
	void ECSManager<EntityID, Ts...>::Clear()
	{
		(std::get<typename internal_ecs::GetComponentContainer<EntityID, Ts, Ts::component_type>::Container>(component_sets).Clear(), ...);
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