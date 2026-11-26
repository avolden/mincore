#pragma once

#include <stdint.h>

#include "alloc.hh"
#include "concepts.hh"
#include "type_traits.hh"
#include "utility.hh"

namespace mc
{
	template <typename T>
	class function;

	template <typename R, typename... Args>
	class function<R(Args...)>
	{
	public:
		function() = default;
		function(function const&) = delete;
		function(function&&);
		template <typename F>
		function(F&& fn)
			requires(!same_as<decay_t<F>, function<R(Args...)>>);
		~function();

		function& operator=(function const&) = delete;
		function& operator=(function&&);

		bool valid() const;

		R operator()(Args... args);

	private:
		struct vtable
		{
			R (*invoke)(void*, Args&&...) {nullptr};
			void (*destruct)(void*) {nullptr};
			void (*move)(void*, void*) {nullptr};
		};

		using fn_ptr = R (*)(Args...);
		static constexpr vtable fn_ptr_funcs {
			[](void* storage, Args&&... args)
			{ return (*reinterpret_cast<fn_ptr*>(storage))(forward<Args>(args)...); },
			[]([[maybe_unused]] void* storage) {},
			[](void* from, void* to)
			{ *reinterpret_cast<fn_ptr*>(to) = *reinterpret_cast<fn_ptr*>(from); },
		};

		static constexpr uint8_t storage_size {sizeof(void*) * 4};
		static constexpr uint8_t storage_align {alignof(void*)};

		alignas(storage_align) uint8_t storage_[storage_size];
		vtable const* vtable_ {nullptr};
	};
}

namespace mc
{
	template <typename R, typename... Args>
	template <typename F>
	function<R(Args...)>::function(F&& fn)
		requires(!same_as<decay_t<F>, function<R(Args...)>>)
	{
		if constexpr (convertible_to<F, fn_ptr>)
		{
			// TODO C function pointer, or lambda without capture
			vtable_ = &fn_ptr_funcs;
			*reinterpret_cast<fn_ptr*>(storage_) = fn;
		}
		else
		{
			using fn_type = decay_t<F>;
			static_assert(sizeof(fn_type) < storage_size,
			              "lambda is too big for mc::function storage");

			static vtable const funcs {
				[](void* storage, Args&&... args)
				{
					return reinterpret_cast<fn_type*>(storage)->operator()(
						forward<Args>(args)...);
				},
				[](void* storage) { reinterpret_cast<fn_type*>(storage)->~fn_type(); },
				[](void* from, void* to)
				{
					new (to) fn_type(
						static_cast<fn_type&&>(*reinterpret_cast<fn_type*>(from)));
				},
			};

			vtable_ = &funcs;
			new (storage_) fn_type(forward<F>(fn));
		}
	}

	template <typename R, typename... Args>
	function<R(Args...)>::~function()
	{
		if (valid())
			vtable_->destruct(storage_);
	}

	template <typename R, typename... Args>
	function<R(Args...)>::function(function&& other)
	{
		if (other.valid())
		{
			other.vtable_->move(other.storage_, storage_);
			vtable_ = other.vtable_;

			other.vtable_->destruct(other.storage_);
			other.vtable_ = nullptr;
		}
	}

	template <typename R, typename... Args>
	function<R(Args...)>& function<R(Args...)>::operator=(function&& other)
	{
		if (valid())
		{
			vtable_->destruct(other.storage_);
			vtable_ = nullptr;
		}

		if (other.valid())
		{
			other.vtable_->move(other.storage_, storage_);
			vtable_ = other.vtable_;

			other.vtable_->destruct(other.storage_);
			other.vtable_ = nullptr;
		}

		return *this;
	}

	template <typename R, typename... Args>
	bool function<R(Args...)>::valid() const
	{
		return vtable_ != nullptr;
	}

	template <typename R, typename... Args>
	R function<R(Args...)>::operator()(Args... args)
	{
		return vtable_->invoke(storage_, forward<Args>(args)...);
	}
}