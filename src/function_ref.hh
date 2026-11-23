#pragma once

#include "concepts.hh"
#include "type_traits.hh"
#include "utility.hh"

namespace mc
{
	template <typename T>
	class function_ref;

	template <typename R, typename... Args>
	class function_ref<R(Args...)>
	{
	public:
		function_ref() = default;
		template <typename F>
		function_ref(F&& fn)
			requires(!same_as<decay_t<F>, function_ref<R(Args...)>>);

		bool valid() const;

		R operator()(Args... args);

	private:
		void* ptr_ {nullptr};
		R (*invoke_)(void*, Args&&...) {nullptr};

		using fn_ptr = R (*)(Args...);
	};
}

namespace mc
{
	template <typename R, typename... Args>
	template <typename F>
	function_ref<R(Args...)>::function_ref(F&& fn)
		requires(!same_as<decay_t<F>, function_ref<R(Args...)>>)
	{
		if constexpr (convertible_to<F, fn_ptr>)
		{
			ptr_ = reinterpret_cast<void*>(static_cast<fn_ptr>(fn));
		}
		else
		{
			ptr_ = reinterpret_cast<void*>(&fn);
			invoke_ = [](void* ptr, Args&&... args)
			{ return (*reinterpret_cast<decay_t<F>*>(ptr))(forward<Args>(args)...); };
		}
	}

	template <typename R, typename... Args>
	bool function_ref<R(Args...)>::valid() const
	{
		return ptr_ != nullptr;
	}

	template <typename R, typename... Args>
	R function_ref<R(Args...)>::operator()(Args... args)
	{
		if (invoke_ != nullptr)
			return invoke_(ptr_, forward<Args>(args)...);
		else
			return reinterpret_cast<fn_ptr>(ptr_)(forward<Args>(args)...);
	}
}