#pragma once

#include "type_traits.hh"

// TODO this file is named after the std <utility> header. I currently don't have another
// idea to organize 'utility stuff'.
// This file will need to be reevaluated from time to time to not make it awfully big.
namespace mc
{
	template <typename T>
	constexpr remove_reference_t<T>&& move(T&& t)

	{
		return static_cast<remove_reference_t<T>&&>(t);
	}

	template <typename T>
	constexpr T&& forward(remove_reference_t<T>& t) noexcept
	{
		return static_cast<T&&>(t);
	}

	template <typename T>
	constexpr T&& forward(remove_reference_t<T>&& t) noexcept
	{
		// static_assert(!is_lvalue_reference_v<T>);
		return static_cast<T&&>(t);
	}

}