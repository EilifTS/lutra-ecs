#pragma once
#include <lutra-ecs/SparseSet.h>
#include <lutra-ecs/IndexFreeList.h>
#include <lutra-ecs/SparseTagSet.h>
#include <lutra-ecs/IterationHelpers.h>
#include <lutra-ecs/EntityID.h>
#include <algorithm>
#include <array>
#include <type_traits>
#include <vector>
#include <tuple>

namespace lcs
{
	template <typename... Ts> class ECSManager;

	enum class ComponentType
	{
		Component, Tag
	};

	namespace internal_ecs
	{
		template <typename T, ComponentType type>
		struct GetComponentContainer;

		template <typename T>
		struct GetComponentContainer<T, ComponentType::Component>
		{
			using Container = SparseSet<T>;
		};

		template <typename T>
		struct GetComponentContainer<T, ComponentType::Tag>
		{
			using Container = SparseTagSetT<T>;
		};
	}

	template <typename... Ts>
	class ECSManager
	{
	public:
		ECSManager() { reserveComponentStorage(reserved_component_count); };

		inline EntityID CreateEntity();
		inline void DestroyEntity(EntityID entity);
		inline const IndexFreeList::OccupiedIndicesContainer<false> Entities() { return entity_id_generator.OccupiedIndices(); };
		inline const IndexFreeList::OccupiedIndicesContainer<true> EntitiesReverse() { return entity_id_generator.OccupiedIndicesReverse(); };
		inline u32 GetEntityCount();

		/* Component */
		template <typename T> inline bool HasComponent(EntityID id);
		template <typename T> inline T& GetComponent(EntityID id);
		template <typename T> inline std::remove_reference<T>::type& AddComponent(EntityID id, T&& component);
		template <typename T> inline void RemoveComponent(EntityID id);
		template <typename T> inline u32 GetComponentCount();
		template <typename T> inline internal_ecs::EntityComponentIterationHelper<T> GetAllEntitiesWithComponent();
		template <typename T> inline internal_ecs::EntityComponentIterationHelperReverse<T> GetAllEntitiesWithComponentReverse();
		template <typename T> inline ComponentView<T> CView();

		/* Tag */
		template <typename T> inline bool HasTag(EntityID id);
		template <typename T> inline void AddTag(EntityID id);
		template <typename T> inline void RemoveTag(EntityID id);
		template <typename T> inline internal_ecs::EntityTagIterationHelper<T> GetAllEntitiesWithTag();
		template <typename T> inline TagView<T> TView();
		/* template <typename T> inline u32 GetTagCount(); */

		inline void Clear();

	private:
		inline void reserveComponentStorage(u32 new_size);
		inline void growComponentStorageIfNecessary();

	private:
		IndexFreeList entity_id_generator{};
		std::tuple<typename internal_ecs::GetComponentContainer<Ts, Ts::component_type>::Container... > component_sets{};
		u32 reserved_component_count{ 8 };
		static constexpr u32 component_grow_factor = 2;

		friend class Entity;
	};

	template <typename... Ts>
	inline EntityID ECSManager<Ts...>::CreateEntity()
	{
		growComponentStorageIfNecessary();
		return EntityID(entity_id_generator.GetNextIndex());
	}

	template <typename... Ts>
	inline void ECSManager<Ts...>::DestroyEntity(EntityID id)
	{
		/* Remove components */
		( std::get<typename internal_ecs::GetComponentContainer<Ts, Ts::component_type>::Container>(component_sets).RemoveIfPresent(id), ...);

		entity_id_generator.FreeIndex(id);
	}

	template <typename... Ts>
	inline u32 ECSManager<Ts...>::GetEntityCount()
	{
		return entity_id_generator.UsedIndexCount();
	}

	template <typename... Ts> template <typename T>
	inline bool ECSManager<Ts...>::HasComponent(EntityID id)
	{
		SparseSet<T>& set = std::get<SparseSet<T>>(component_sets);
		return set.Has(id);
	}

	template <typename... Ts> template <typename T>
	inline T& ECSManager<Ts...>::GetComponent(EntityID id)
	{
		SparseSet<T>& set = std::get<SparseSet<T>>(component_sets);
		return set.Get(id);
	}

	template <typename... Ts> template <typename T>
	inline std::remove_reference<T>::type& ECSManager<Ts...>::AddComponent(EntityID id, T&& component)
	{
		using Tp = std::remove_reference<T>::type;
		SparseSet<Tp>& set = std::get<SparseSet<Tp>>(component_sets);
		set.Add(id, std::forward<T>(component));
		return GetComponent<Tp>(id);
	}

	template <typename... Ts> template <typename T>
	inline void ECSManager<Ts...>::RemoveComponent(EntityID id)
	{
		assert(HasComponent<T>(id));
		SparseSet<T>& set = std::get<SparseSet<T>>(component_sets);
		return set.Remove(id);
	}

	template <typename... Ts> template <typename T>
	inline u32 ECSManager<Ts...>::GetComponentCount()
	{
		SparseSet<T>& set = std::get<SparseSet<T>>(component_sets);
		return set.Size();
	}

	template <typename... Ts> template <typename T>
	inline internal_ecs::EntityComponentIterationHelper<T> ECSManager<Ts...>::GetAllEntitiesWithComponent()
	{
		SparseSet<T>& set = std::get<SparseSet<T>>(component_sets);
		return internal_ecs::EntityComponentIterationHelper<T>(set);
	}

	template <typename... Ts> template <typename T>
	inline internal_ecs::EntityComponentIterationHelperReverse<T> ECSManager<Ts...>::GetAllEntitiesWithComponentReverse()
	{
		SparseSet<T>& set = std::get<SparseSet<T>>(component_sets);
		return internal_ecs::EntityComponentIterationHelperReverse<T>(set);
	}

	template <typename... Ts> template <typename T>
	inline ComponentView<T> ECSManager<Ts...>::CView()
	{
		SparseSet<T>& set = std::get<SparseSet<T>>(component_sets);
		return ComponentView<T>(set);
	}

	template <typename... Ts> template <typename T>
	bool ECSManager<Ts...>::HasTag(EntityID id)
	{
		SparseTagSetT<T>& set = std::get<SparseTagSetT<T>>(component_sets);
		return set.Has(id);
	}

	template <typename... Ts> template <typename T>
	void ECSManager<Ts...>::AddTag(EntityID id)
	{
		SparseTagSetT<T>& set = std::get<SparseTagSetT<T>>(component_sets);
		return set.Add(id);
	}

	template <typename... Ts> template <typename T>
	void ECSManager<Ts...>::RemoveTag(EntityID id)
	{
		SparseTagSetT<T>& set = std::get<SparseTagSetT<T>>(component_sets);
		return set.Remove(id);
	}

	template <typename... Ts> template <typename T>
	internal_ecs::EntityTagIterationHelper<T> ECSManager<Ts...>::GetAllEntitiesWithTag()
	{
		SparseTagSetT<T>& set = std::get<SparseTagSetT<T>>(component_sets);
		return internal_ecs::EntityTagIterationHelper<T>(set);
	}

	template <typename... Ts> template <typename T>
	inline TagView<T> ECSManager<Ts...>::TView()
	{
		SparseTagSetT<T>& set = std::get<SparseTagSetT<T>>(component_sets);
		return TagView<T>(set);
	}

	/*template <typename... Ts, typename T>
	inline u32 ECSManager<Ts...>::GetTagCount()
	{
		SparseTagSetT<T>& set = std::get<SparseTagSetT<T>>(component_sets);
		return set.Size();
	}*/

	template <typename... Ts>
	void ECSManager<Ts...>::Clear()
	{
		(std::get<typename internal_ecs::GetComponentContainer<Ts, Ts::component_type>::Container>(component_sets).Clear(), ...);
		entity_id_generator.Clear();
	}

	template <typename... Ts>
	void ECSManager<Ts...>::reserveComponentStorage(u32 new_size)
	{
		(std::get<typename internal_ecs::GetComponentContainer<Ts, Ts::component_type>::Container>(component_sets).ReserveSparseSize(new_size), ...);
	}

	template <typename... Ts>
	void ECSManager<Ts...>::growComponentStorageIfNecessary()
	{
		if (reserved_component_count == entity_id_generator.UsedIndexCount())
		{
			reserved_component_count *= component_grow_factor;
			reserveComponentStorage(reserved_component_count);
		}
	}
}