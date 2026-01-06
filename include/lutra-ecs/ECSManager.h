#pragma once
#include <lutra-ecs/SparseSet.h>
#include <lutra-ecs/IndexFreeList.h>
#include <lutra-ecs/SparseTagSet.h>
#include <algorithm>
#include <array>
#include <type_traits>
#include <vector>
#include <cinttypes>

namespace lcs
{
	using u32 = uint32_t;

	template <typename... Ts> class ECSManager;
	class Entity;
	using EntityID = u32;

	enum class ComponentType
	{
		Component, Tag
	};

	namespace internal_ecs
	{
		class EntityIterationHelperIteratorArrowHelper;

		template <typename T> class EntityComponentIterationHelper;
		template <typename T> class EntityComponentIterationHelperReverse;
		template <typename T> class EntityComponentIterationHelperIterator;
		template <typename T> class EntityTagIterationHelper;
		template <typename T> class EntityTagIterationHelperIterator;

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
		ECSManager() {};

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

		/* Tag */
		template <typename T> inline internal_ecs::EntityTagIterationHelper<T> GetAllEntitiesWithTag();
		template <typename T> inline bool HasTag(EntityID id);
		template <typename T> inline void AddTag(EntityID id);
		template <typename T> inline void RemoveTag(EntityID id);
		/* template <typename T> inline u32 GetTagCount(); */

		inline void Clear();

	private:
		IndexFreeList entity_id_generator{};
		std::tuple<typename internal_ecs::GetComponentContainer<Ts, Ts::component_type>::Container... > component_sets{};

		friend class Entity;
	};

	namespace internal_ecs
	{
		class EntityIterationHelperIteratorArrowHelper
		{
		public:
			inline EntityID* operator->() { return &entity; };
		private:
			inline EntityIterationHelperIteratorArrowHelper(EntityID id) : entity(id) {};
			EntityID entity;

			template <typename T>
			friend class EntityComponentIterationHelperIterator;
		};

		template <typename T>
		class EntityComponentIterationHelperIterator
		{
		public:
			inline EntityID operator*() const { return set_iterator.GetOwner(); }
			inline EntityIterationHelperIteratorArrowHelper operator->() { return EntityIterationHelperIteratorArrowHelper(set_iterator.GetOwner()); }

			/* Prefix increment */
			inline EntityComponentIterationHelperIterator& operator++() { set_iterator++; return *this; }

			/* Postfix increment */
			inline EntityComponentIterationHelperIterator operator++(int) { EntityComponentIterationHelperIterator tmp = *this; ++(*this); return tmp; }

			friend bool operator== (const EntityComponentIterationHelperIterator& a, const EntityComponentIterationHelperIterator& b) { return a.set_iterator == b.set_iterator; };
			friend bool operator!= (const EntityComponentIterationHelperIterator& a, const EntityComponentIterationHelperIterator& b) { return a.set_iterator != b.set_iterator; };

		private:
			EntityComponentIterationHelperIterator(const T& creator) : set_iterator(creator) {};
			T set_iterator;
			friend EntityComponentIterationHelper<typename T::IteratorType>;
			friend EntityComponentIterationHelperReverse<typename T::IteratorType>;
		};

		template <typename T>
		class EntityComponentIterationHelper
		{
		public:
			inline EntityComponentIterationHelperIterator<typename SparseSet<T>::Iterator> begin() { return EntityComponentIterationHelperIterator<typename SparseSet<T>::Iterator>(set.begin()); };

			inline EntityComponentIterationHelperIterator<typename SparseSet<T>::Iterator> end() { return EntityComponentIterationHelperIterator<typename SparseSet<T>::Iterator>(set.end()); };

		private:
			EntityComponentIterationHelper(SparseSet<T>& set) : set(set) {};
			SparseSet<T>& set;
			template <typename... Ts> friend class lcs::ECSManager;
		};

		template <typename T>
		class EntityComponentIterationHelperReverse
		{
		public:
			inline EntityComponentIterationHelperIterator<typename SparseSet<T>::RIterator> begin() { return EntityComponentIterationHelperIterator<typename SparseSet<T>::RIterator>(set.rbegin()); };

			inline EntityComponentIterationHelperIterator<typename SparseSet<T>::RIterator> end() { return EntityComponentIterationHelperIterator<typename SparseSet<T>::RIterator>(set.rend()); };

		private:
			EntityComponentIterationHelperReverse(SparseSet<T>& set) : set(set) {};
			SparseSet<T>& set;
			template <typename... Ts> friend class lcs::ECSManager;
		};

		template <typename T>
		class EntityTagIterationHelperIterator
		{
		public:
			inline EntityID operator*() const { return *set_iterator; }
			inline EntityIterationHelperIteratorArrowHelper operator->() { return EntityIterationHelperIteratorArrowHelper(*set_iterator); }

			/* Prefix increment */
			inline EntityTagIterationHelperIterator& operator++() { set_iterator++; return *this; }

			/* Postfix increment */
			inline EntityTagIterationHelperIterator operator++(int) { EntityTagIterationHelperIterator tmp = *this; ++(*this); return tmp; }

			friend bool operator== (const EntityTagIterationHelperIterator& a, const EntityTagIterationHelperIterator& b) { return a.set_iterator == b.set_iterator; };
			friend bool operator!= (const EntityTagIterationHelperIterator& a, const EntityTagIterationHelperIterator& b) { return a.set_iterator != b.set_iterator; };

		private:
			EntityTagIterationHelperIterator(const SparseTagSetT<T>::Iterator& creator) : set_iterator(creator) {};
			SparseTagSetT<T>::Iterator set_iterator;
			friend EntityTagIterationHelper<T>;
		};

		template <typename T>
		class EntityTagIterationHelper
		{
		public:
			inline EntityTagIterationHelperIterator<T> begin() { return EntityTagIterationHelperIterator<T>(set.begin()); };

			inline EntityTagIterationHelperIterator<T> end() { return EntityTagIterationHelperIterator<T>(set.end()); };

		private:
			EntityTagIterationHelper(SparseTagSetT<T>& set) : set(set) {};
			SparseTagSetT<T>& set;
			template <typename... Ts> friend class lcs::ECSManager;
		};
	}

	template <typename... Ts>
	inline EntityID ECSManager<Ts...>::CreateEntity()
	{
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
}