#pragma once
#include <lutra-ecs/SparseSet.h>
#include <lutra-ecs/HandleFreeList.h>
#include <lutra-ecs/SparseTagSet.h>
#include <lutra-ecs/Views.h>
#include <algorithm>
#include <array>
#include <type_traits>
#include <vector>
#include <tuple>

namespace lcs
{
	enum class ComponentType
	{
		Component, Tag
	};

	namespace internal_ecs
	{
		template <typename EntityID, typename T, ComponentType type>
		struct GetComponentContainer;

		template <typename EntityID, typename T>
		struct GetComponentContainer<EntityID, T, ComponentType::Component>
		{
			using Container = SparseSet<EntityID, T>;
		};

		template <typename EntityID, typename T>
		struct GetComponentContainer<EntityID, T, ComponentType::Tag>
		{
			using Container = SparseTagSetT<EntityID, T>;
		};
	}

	template <typename handle_t, typename... Ts>
	class ECSManager
	{
	public:
		using EntityID = handle_t;

		ECSManager() { reserveComponentStorage(reserved_component_count); };

		inline EntityID CreateEntity();
		inline void DestroyEntity(EntityID entity);
		inline const HandleFreeList<EntityID>& Entities() { return entity_id_generator; };
		//inline const IndexFreeList::OccupiedIndicesContainer<true> EntitiesReverse() { return entity_id_generator.OccupiedIndicesReverse(); };
		inline EntityID::data_t GetEntityCount();

		/* Component */
		template <typename T> inline bool HasComponent(EntityID id);
		template <typename T> inline T& GetComponent(EntityID id);
		template <typename T> inline std::remove_reference<T>::type& AddComponent(EntityID id, T&& component);
		template <typename T> inline void RemoveComponent(EntityID id);
		template <typename T> inline EntityID::data_t GetComponentCount();
		template <typename T> inline ComponentView<EntityID, T> CView();

		/* Tag */
		template <typename T> inline bool HasTag(EntityID id);
		template <typename T> inline void AddTag(EntityID id);
		template <typename T> inline void RemoveTag(EntityID id);
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
	inline void ECSManager<EntityID, Ts...>::DestroyEntity(EntityID id)
	{
		/* Remove components */
		( std::get<typename internal_ecs::GetComponentContainer<EntityID, Ts, Ts::component_type>::Container>(component_sets).RemoveIfPresent(id), ...);

		entity_id_generator.FreeHandle(id);
	}

	template <typename EntityID, typename... Ts>
	inline EntityID::data_t ECSManager<EntityID, Ts...>::GetEntityCount()
	{
		return entity_id_generator.UsedHandleCount();
	}

	template <typename EntityID, typename... Ts> template <typename T>
	inline bool ECSManager<EntityID, Ts...>::HasComponent(EntityID id)
	{
		SparseSet<EntityID, T>& set = std::get<SparseSet<EntityID, T>>(component_sets);
		return set.Has(id);
	}

	template <typename EntityID, typename... Ts> template <typename T>
	inline T& ECSManager<EntityID, Ts...>::GetComponent(EntityID id)
	{
		SparseSet<EntityID, T>& set = std::get<SparseSet<EntityID, T>>(component_sets);
		return set.Get(id);
	}

	template <typename EntityID, typename... Ts> template <typename T>
	inline std::remove_reference<T>::type& ECSManager<EntityID, Ts...>::AddComponent(EntityID id, T&& component)
	{
		using Tp = std::remove_reference<T>::type;
		SparseSet<EntityID, Tp>& set = std::get<SparseSet<EntityID, Tp>>(component_sets);
		set.Add(id, std::forward<T>(component));
		return GetComponent<Tp>(id);
	}

	template <typename EntityID, typename... Ts> template <typename T>
	inline void ECSManager<EntityID, Ts...>::RemoveComponent(EntityID id)
	{
		assert(HasComponent<T>(id));
		SparseSet<EntityID, T>& set = std::get<SparseSet<EntityID, T>>(component_sets);
		return set.Remove(id);
	}

	template <typename EntityID, typename... Ts> template <typename T>
	inline EntityID::data_t ECSManager<EntityID, Ts...>::GetComponentCount()
	{
		SparseSet<EntityID, T>& set = std::get<SparseSet<EntityID, T>>(component_sets);
		return set.Size();
	}

	template <typename EntityID, typename... Ts> template <typename T>
	inline ComponentView<EntityID, T> ECSManager<EntityID, Ts...>::CView()
	{
		SparseSet<EntityID, T>& set = std::get<SparseSet<EntityID, T>>(component_sets);
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