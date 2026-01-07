#pragma once
#include <lutra-ecs/EntityID.h>
#include <lutra-ecs/SparseSet.h>
#include <lutra-ecs/SparseTagSet.h>

#include <utility>

namespace lcs
{
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