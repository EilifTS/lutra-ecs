#pragma once
#include <lutra-ecs/SparseSet.h>
#include <lutra-ecs/SparseTagSet.h>

#include <utility>

namespace lcs
{
	template <typename EntityID, typename T>
	class ComponentView
	{
	public:
		ComponentView(SparseSet<EntityID, T>& set) : set(set) {};

		class Iterator
		{
		public:
			/* Constructor */
			Iterator(SparseSet<EntityID, T>::Iterator it) : set_iterator(it) {};

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
			inline std::pair<EntityID, T&> operator*(){ return std::pair<EntityID, T&>(set_iterator.GetOwner(), *set_iterator); }
			inline ArrowHelper operator->() { return ArrowHelper(set_iterator.GetOwner(), *set_iterator); }

			/* Prefix increment */
			inline Iterator& operator++() { set_iterator++; return *this; }

			/* Postfix increment */
			inline Iterator operator++(int) { Iterator tmp = *this; ++(*this); return tmp; }

			friend bool operator== (const Iterator& a, const Iterator& b) { return a.set_iterator == b.set_iterator; };
			friend bool operator!= (const Iterator& a, const Iterator& b) { return a.set_iterator != b.set_iterator; };

		private:
			SparseSet<EntityID, T>::Iterator set_iterator;
		};

		inline Iterator begin();
		inline Iterator end();

	private:
		SparseSet<EntityID, T>& set;
	};

	template <typename EntityID, typename T>
	inline ComponentView<EntityID, T>::Iterator ComponentView<EntityID, T>::begin() { return ComponentView<EntityID, T>::Iterator(set.begin()); };

	template <typename EntityID, typename T>
	inline ComponentView<EntityID, T>::Iterator ComponentView<EntityID, T>::end() { return ComponentView<EntityID, T>::Iterator(set.end()); };

	template <typename EntityID, typename T>
	class TagView
	{
	public:
		TagView(SparseTagSetT<EntityID, T>& set) : set(set) {};

		class Iterator
		{
		public:
			/* Constructor */
			Iterator(SparseTagSetT<EntityID, T>::Iterator it) : set_iterator(it) {};

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
			SparseTagSetT<EntityID, T>::Iterator set_iterator;
		};

		inline Iterator begin();
		inline Iterator end();

	private:
		SparseTagSetT<EntityID, T>& set;
	};

	template <typename EntityID, typename T>
	inline TagView<EntityID, T>::Iterator TagView<EntityID, T>::begin() { return TagView<EntityID, T>::Iterator(set.begin()); };

	template <typename EntityID, typename T>
	inline TagView<EntityID, T>::Iterator TagView<EntityID, T>::end() { return TagView<EntityID, T>::Iterator(set.end()); };
}