#pragma once
#include <lutra-ecs/EntityID.h>
#include <lutra-ecs/SparseSet.h>
#include <lutra-ecs/SparseTagSet.h>

#include <utility>

namespace lcs
{
	/* Forward declatations */
	template <typename... Ts> class ECSManager;

	namespace internal_ecs
	{
		/* Forward declatations */
		class EntityIterationHelperIteratorArrowHelper;
		template <typename T> class EntityComponentIterationHelper;
		template <typename T> class EntityComponentIterationHelperReverse;
		template <typename T> class EntityComponentIterationHelperIterator;
		template <typename T> class EntityTagIterationHelper;
		template <typename T> class EntityTagIterationHelperIterator;

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

	template <typename T>
	class ComponentView
	{
	public:
		ComponentView(SparseSet<T>& set) : set(set) {};

		class Iterator
		{
		public:
			/* Constructor */
			Iterator(SparseSet<T>::Iterator it) : set_iterator(it) {};

			/* Class for enabling arrow operator on temporary return object */
			class ArrowHelper
			{
			public:
				inline ArrowHelper(EntityID id, T& c) : p(std::pair<EntityID, T&>(id, c)) {};
				inline std::pair<EntityID, T&>* operator->() { return &p; };
			private:
				std::pair<EntityID, T&> p;
			};

			/* Accessors */
			inline std::pair<EntityID, T&> operator*(){ return std::pair<EntityID, T&>(EntityID(set_iterator.GetOwner()), *set_iterator); }
			inline ArrowHelper operator->() { return ArrowHelper(set_iterator.GetOwner(), *set_iterator); }

			/* Prefix increment */
			inline Iterator& operator++() { set_iterator++; return *this; }

			/* Postfix increment */
			inline Iterator operator++(int) { Iterator tmp = *this; ++(*this); return tmp; }

			friend bool operator== (const Iterator& a, const Iterator& b) { return a.set_iterator == b.set_iterator; };
			friend bool operator!= (const Iterator& a, const Iterator& b) { return a.set_iterator != b.set_iterator; };

		private:
			SparseSet<T>::Iterator set_iterator;
		};

		inline Iterator begin();
		inline Iterator end();

	private:
		SparseSet<T>& set;
	};

	template <typename T>
	inline ComponentView<T>::Iterator ComponentView<T>::begin() { return ComponentView<T>::Iterator(set.begin()); };

	template <typename T>
	inline ComponentView<T>::Iterator ComponentView<T>::end() { return ComponentView<T>::Iterator(set.end()); };

	template <typename T>
	class TagView
	{
	public:
		TagView(SparseTagSetT<T>& set) : set(set) {};

		class Iterator
		{
		public:
			/* Constructor */
			Iterator(SparseTagSetT<T>::Iterator it) : set_iterator(it) {};

			/* Class for enabling arrow operator on temporary return object */
			class ArrowHelper
			{
			public:
				inline ArrowHelper(EntityID id) : id(id) {};
				inline EntityID* operator->() { return &id; };
			private:
				EntityID id;
			};

			/* Accessors */
			inline EntityID operator*() { return *set_iterator; }
			inline ArrowHelper operator->() { return ArrowHelper(*set_iterator); }

			/* Prefix increment */
			inline Iterator& operator++() { set_iterator++; return *this; }

			/* Postfix increment */
			inline Iterator operator++(int) { Iterator tmp = *this; ++(*this); return tmp; }

			friend bool operator== (const Iterator& a, const Iterator& b) { return a.set_iterator == b.set_iterator; };
			friend bool operator!= (const Iterator& a, const Iterator& b) { return a.set_iterator != b.set_iterator; };

		private:
			SparseTagSetT<T>::Iterator set_iterator;
		};

		inline Iterator begin();
		inline Iterator end();

	private:
		SparseTagSetT<T>& set;
	};

	template <typename T>
	inline TagView<T>::Iterator TagView<T>::begin() { return TagView<T>::Iterator(set.begin()); };

	template <typename T>
	inline TagView<T>::Iterator TagView<T>::end() { return TagView<T>::Iterator(set.end()); };
}