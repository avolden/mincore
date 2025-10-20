#pragma once

#include <stddef.h>
#include <stdint.h>

#include "config.hh"

namespace mc
{
	using alloc_func = void*(uint64_t size, uint32_t alignment, void* ud);
	using free_func = void(void* ptr, uint64_t size, uint32_t alignment, void* ud);

	struct alloc_funcs
	{
		alloc_func* alloc {nullptr};
		free_func*  free {nullptr};
		void*       ud {nullptr};
	};

	extern alloc_funcs g_alloc;

	void set_alloc_funcs(alloc_func* alloc, free_func* free, void* ud);

	void* alloc(uint64_t size, uint32_t alignment);
	void  free(void* ptr, uint64_t size, uint32_t alignment);
}

#ifdef NEW_USE_STD
#include <new>
#else
// Internal functions declarations for placement new, to prevent including <new>
#ifdef _WIN32
#ifndef __PLACEMENT_NEW_INLINE
#define __PLACEMENT_NEW_INLINE

inline void* operator new([[maybe_unused]] size_t count, void* ptr) noexcept
{
	return ptr;
}
#endif

#ifndef __PLACEMENT_VEC_NEW_INLINE
#define __PLACEMENT_VEC_NEW_INLINE

inline void* operator new[]([[maybe_unused]] size_t count, void* ptr) noexcept
{
	return ptr;
}
#endif
#else
extern void* operator new(size_t count, void* ptr) noexcept;
extern void* operator new[](size_t count, void* ptr) noexcept;
#endif
#endif