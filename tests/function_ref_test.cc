#include "unit.hh"

#include "function_ref.hh"

GROUP(function_ref)
{
	namespace
	{
		int add(int a, int b)
		{
			return a + b;
		}

		constexpr auto add_lambda = [](int a, int b) -> int { return a + b; };
	}

	TEST(ctor)
	{
		int  c = 3;
		auto add_lambda_capture = [c](int a, int b) { return a + b + c; };

		mc::function_ref<int(int, int)> f0;
		CHECK_EQ(f0.valid(), false);

		mc::function_ref<int(int, int)> f1(add);
		CHECK_EQ(f1.valid(), true);

		mc::function_ref<int(int, int)> f2(add_lambda);
		CHECK_EQ(f2.valid(), true);

		mc::function_ref<int(int, int)> f3(add_lambda_capture);
		CHECK_EQ(f3.valid(), true);

		mc::function_ref<int(int, int)> f4(f0);
		CHECK_EQ(f4.valid(), false);

		mc::function_ref<int(int, int)> f5(f1);
		CHECK_EQ(f5.valid(), true);

		mc::function_ref<int(int, int)> f6(f2);
		CHECK_EQ(f6.valid(), true);

		mc::function_ref<int(int, int)> f7(f3);
		CHECK_EQ(f7.valid(), true);
	}

	TEST(invoke)
	{
		int  c = 3;
		auto add_lambda_capture = [c](int a, int b) { return a + b + c; };

		mc::function_ref<int(int, int)> f0(add);

		int res0 = f0(1, 2);
		CHECK_EQ(res0, 3)

		mc::function_ref<int(int, int)> f1(add_lambda);

		int res1 = f1(1, 2);
		CHECK_EQ(res1, 3)

		mc::function_ref<int(int, int)> f2(add_lambda_capture);

		int res2 = f2(1, 2);
		CHECK_EQ(res2, 6)
	}
}