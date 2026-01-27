#pragma once

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define GROUP(Name)                                                                      \
	namespace namesp_group_##Name                                                        \
	{                                                                                    \
		struct group_##Name : public group_base                                          \
		{                                                                                \
			group_##Name()                                                               \
			: group_base(#Name)                                                          \
			{}                                                                           \
		};                                                                               \
		group_##Name group_instance;                                                     \
	}                                                                                    \
	namespace namesp_group_##Name

#define TEST(Name)                                                                       \
	struct test_##Name : public test_base                                                \
	{                                                                                    \
		test_##Name(group_base* group)                                                   \
		: test_base(#Name, group)                                                        \
		{}                                                                               \
		void run() override;                                                             \
	};                                                                                   \
	test_##Name test_##Name##_instance(&group_instance);                                 \
	void        test_##Name::run()

#define CHECK_EQ(a, b)                                                                   \
	if ((a) != (b))                                                                      \
	{                                                                                    \
		++checks_fail_cnt;                                                               \
		++checks_cnt;                                                                    \
		print_check_error(a, b, #a, #b, "==", __LINE__, __FILE__);                       \
	}                                                                                    \
	else                                                                                 \
	{                                                                                    \
		++checks_cnt;                                                                    \
	}

#define CHECK_NE(a, b)                                                                   \
	if ((a) == (b))                                                                      \
	{                                                                                    \
		++checks_fail_cnt;                                                               \
		++checks_cnt;                                                                    \
		print_check_error(a, b, #a, #b, "!=", __LINE__, __FILE__);                       \
	}                                                                                    \
	else                                                                                 \
	{                                                                                    \
		++checks_cnt;                                                                    \
	}

#define CHECK_GT(a, b)                                                                   \
	if ((a) <= (b))                                                                      \
	{                                                                                    \
		++checks_fail_cnt;                                                               \
		++checks_cnt;                                                                    \
		print_check_error(a, b, #a, #b, ">", __LINE__, __FILE__);                        \
	}                                                                                    \
	else                                                                                 \
	{                                                                                    \
		++checks_cnt;                                                                    \
	}

#define CHECK_GE(a, b)                                                                   \
	if ((a) < (b))                                                                       \
	{                                                                                    \
		++checks_fail_cnt;                                                               \
		++checks_cnt;                                                                    \
		print_check_error(a, b, #a, #b, ">=", __LINE__, __FILE__);                       \
	}                                                                                    \
	else                                                                                 \
	{                                                                                    \
		++checks_cnt;                                                                    \
	}

#define CHECK_LT(a, b)                                                                   \
	if ((a) >= (b))                                                                      \
	{                                                                                    \
		++checks_fail_cnt;                                                               \
		++checks_cnt;                                                                    \
		print_check_error(a, b, #a, #b, "<", __LINE__, __FILE__);                        \
	}                                                                                    \
	else                                                                                 \
	{                                                                                    \
		++checks_cnt;                                                                    \
	}

#define CHECK_LE(a, b)                                                                   \
	if ((a) > (b))                                                                       \
	{                                                                                    \
		++checks_fail_cnt;                                                               \
		++checks_cnt;                                                                    \
		print_check_error(a, b, #a, #b, "<=", __LINE__, __FILE__);                       \
	}                                                                                    \
	else                                                                                 \
	{                                                                                    \
		++checks_cnt;                                                                    \
	}

struct group_base;
struct test_base;

struct unit_state
{
	static unit_state& instance();

	~unit_state();

	int run_tests();

	void add_group(group_base* group);

	group_base** groups {nullptr};
	uint32_t     group_size {0};
	uint32_t     group_cap {0};
};

struct group_base
{
	group_base(char const* name);
	virtual ~group_base();

	void run();

	void add_test(test_base* test);

	char const* name;
	test_base** tests {nullptr};
	uint32_t    test_size {0};
	uint32_t    test_cap {0};

	uint32_t tests_fail_cnt {0};
};

struct test_base
{
	test_base(char const* name, group_base* group)
	: name {name}
	{
		group->add_test(this);
	}

	virtual void run() = 0;

	char const* name;
	uint32_t    checks_cnt {0};
	uint32_t    checks_fail_cnt {0};
};

template <typename, typename>
constexpr bool is_same_v = false;

template <typename T>
constexpr bool is_same_v<T, T> = true;

template <typename>
constexpr bool is_pointer_v = false;

template <typename T>
constexpr bool is_pointer_v<T*> = true;

template <typename T>
bool print_value(char* buf, char const* val_str, T const& val)
{
	if constexpr (is_same_v<T, int8_t> || is_same_v<T, int16_t> || is_same_v<T, int32_t>)
	{
		snprintf(buf, 64, "%i", val);
		return strcmp(buf, val_str) != 0;
	}
	else if constexpr (is_same_v<T, int64_t>)
	{
		snprintf(buf, 64, "%" PRIi64, val);
		return strcmp(buf, val_str) != 0;
	}
	else if constexpr (is_same_v<T, uint8_t> || is_same_v<T, uint16_t> ||
	                   is_same_v<T, uint32_t>)
	{
		snprintf(buf, 64, "%u", val);
		return strcmp(buf, val_str) != 0;
	}
	else if constexpr (is_same_v<T, uint64_t>)
	{
		snprintf(buf, 64, "%" PRIu64, val);
		return strcmp(buf, val_str) != 0;
	}
	else if constexpr (is_same_v<T, float> || is_same_v<T, double>)
	{
		snprintf(buf, 64, "%f", val);
		return strcmp(buf, val_str) != 0;
	}
	else if constexpr (is_same_v<T, char>)
	{
		snprintf(buf, 64, "'%c'", val);
		return strcmp(buf, val_str) != 0;
	}
	else if constexpr (is_same_v<T, char const*> || is_same_v<T, char*>)
	{
		snprintf(buf, 64, "\"%s\"", val);
		return strcmp(buf, val_str) != 0;
	}
	else if constexpr (is_same_v<T, bool>)
	{
		strcpy(buf, val ? "true" : "false");
		return strcmp(buf, val_str) != 0;
	}
	else if constexpr (is_pointer_v<T>)
	{
		bool simple_ptr = print_value(buf, val_str, *val);
		if (!simple_ptr)
			snprintf(buf, 64, "%p", val);

		return true;
	}
	else
	{
		// Non-trivial type, no simple way to parse it
		return false;
	}
}

#define RED "\x1b[31m"
#define GREEN "\x1b[32m"
#define DEFAULT "\x1b[0m"

template <typename T1, typename T2>
void print_check_error(T1 const& a, T2 const& b, char const* a_str, char const* b_str,
                       char const* op, int line, char const* file)
{
	char buf1[64], buf2[64];
	bool need_print_a = print_value(buf1, a_str, a);
	bool need_print_b = print_value(buf2, b_str, b);
	printf("||-[" RED "fail" DEFAULT "]: ");
	if (need_print_a)
		printf("%s(%s) ", a_str, buf1);
	else
		printf("%s ", a_str);

	printf("%s ", op);
	if (need_print_b)
		printf("%s(%s) ", b_str, buf2);
	else
		printf("%s ", b_str);

	printf("(%s:%d)\n", file, line);
}