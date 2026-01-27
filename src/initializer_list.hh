#pragma once

#include "config.hh"

#ifdef INITIALIZER_LIST_USE_STD
#include <initializer_list>
#else
#include <stddef.h>
#include <stdint.h>

namespace std
{
	template <typename T>
	class initializer_list
	{
	public:
		initializer_list() = default;
		initializer_list(T const* first, T const* last);
		T const* begin();
		T const* end();
		uint64_t size();

	private:
		initializer_list(T const* arr, uint64_t size);

		T const* arr_ {nullptr};
		size_t   size_ {0};
	};
}

namespace std
{

	template <typename T>
	initializer_list<T>::initializer_list(T const* first, T const* last)
	: arr_ {first}
	, size_ {last - first}
	{}

	template <typename T>
	T const* initializer_list<T>::begin()
	{
		return arr_;
	}

	template <typename T>
	T const* initializer_list<T>::end()
	{
		return arr_ + size_;
	}

	template <typename T>
	uint64_t initializer_list<T>::size()
	{
		return size_;
	}

	template <typename T>
	initializer_list<T>::initializer_list(T const* arr, uint64_t size)
	: arr_ {arr}
	, size_ {size}
	{}
}
#endif