#include "unit.hh"

#include <stdint.h>

#include <vector.hh>

GROUP(vector)
{
	class vec_test
	{
	public:
		int32_t val_ {-1};
	};

	class vec_test_copy
	{
	public:
		vec_test_copy(int32_t val)
		: val_ {val}
		{}

		vec_test_copy(vec_test_copy const& other)
		: val_ {other.val_}
		{}

		vec_test_copy(vec_test_copy&&) = delete;

		~vec_test_copy()
		{
			val_ = 0;
		}

		vec_test_copy& operator=(vec_test_copy const& other)
		{
			val_ = other.val_;
			return *this;
		}

		vec_test_copy& operator=(vec_test_copy&&) = delete;

		int32_t val_ {-1};
	};

	class vec_test_move
	{
	public:
		vec_test_move(int32_t val)
		: val_ {val}
		{}

		vec_test_move(vec_test_move const&) = delete;

		vec_test_move(vec_test_move&& other)
		: val_ {other.val_}
		{
			other.val_ = -2;
		}

		~vec_test_move()
		{
			val_ = 0;
		}

		vec_test_move& operator=(vec_test_move const&) = delete;

		vec_test_move& operator=(vec_test_move&& other)
		{
			val_ = other.val_;

			other.val_ = -2;
			return *this;
		}

		int32_t val_ {-1};
	};

	TEST(ctor)
	{
		mc::vector<int32_t> vec0;
		CHECK_EQ(vec0.size(), 0)
		CHECK_EQ(vec0.capacity(), 0)

		mc::vector<int32_t> vec1(5);
		CHECK_EQ(vec1.size(), 5)
		CHECK_EQ(vec1.capacity(), 5)

		mc::vector<int32_t> vec2(5, 42);
		CHECK_EQ(vec2.size(), 5)
		CHECK_EQ(vec2.capacity(), 5)
		for (int32_t i {0}; i < static_cast<int32_t>(vec2.size()); ++i)
			CHECK_EQ(vec2[i], 42)

		mc::vector<int32_t> vec3 {0, 1, 2, 3, 4};
		CHECK_EQ(vec3.size(), 5)
		CHECK_EQ(vec3.capacity(), 5)
		for (int32_t i {0}; i < static_cast<int32_t>(vec3.size()); ++i)
			CHECK_EQ(vec3[i], i)

		mc::vector<int32_t> vec4(vec3);
		CHECK_EQ(vec4.size(), 5)
		CHECK_EQ(vec4.capacity(), 5)
		for (int32_t i {0}; i < static_cast<int32_t>(vec4.size()); ++i)
			CHECK_EQ(vec4[i], i)

		mc::vector<int32_t> vec5(static_cast<mc::vector<int32_t>&&>(vec4));
		CHECK_EQ(vec5.size(), 5)
		CHECK_EQ(vec5.capacity(), 5)
		CHECK_EQ(vec4.size(), 0); // NOLINT(clang-analyzer-cplusplus.Move
		CHECK_EQ(vec4.capacity(), 0)
		for (int32_t i {0}; i < static_cast<int32_t>(vec5.size()); ++i)
			CHECK_EQ(vec5[i], i)

		mc::vector<vec_test_copy> vec6(5, vec_test_copy(42));
		CHECK_EQ(vec6.size(), 5)
		CHECK_EQ(vec6.capacity(), 5)
		for (int32_t i {0}; i < static_cast<int32_t>(vec6.size()); ++i)
			CHECK_EQ(vec6[i].val_, 42)

		mc::vector<vec_test_copy> vec7 {0, 1, 2, 3, 4};
		CHECK_EQ(vec7.size(), 5)
		CHECK_EQ(vec7.capacity(), 5)
		for (int32_t i {0}; i < static_cast<int32_t>(vec7.size()); ++i)
			CHECK_EQ(vec7[i].val_, i)

		mc::vector<vec_test_copy> vec8(vec7);
		CHECK_EQ(vec8.size(), 5)
		CHECK_EQ(vec8.capacity(), 5)
		for (int32_t i {0}; i < static_cast<int32_t>(vec8.size()); ++i)
			CHECK_EQ(vec8[i].val_, i)

		mc::vector<vec_test_copy> vec9(static_cast<mc::vector<vec_test_copy>&&>(vec8));
		CHECK_EQ(vec9.size(), 5)
		CHECK_EQ(vec9.capacity(), 5)
		CHECK_EQ(vec8.size(), 0); // NOLINT(clang-analyzer-cplusplus.Move
		CHECK_EQ(vec8.capacity(), 0)
		for (int32_t i {0}; i < static_cast<int32_t>(vec9.size()); ++i)
			CHECK_EQ(vec9[i].val_, i)
	}

	TEST(resize)
	{
		mc::vector<int32_t> vec0;
		CHECK_EQ(vec0.size(), 0)
		CHECK_EQ(vec0.capacity(), 0)
		vec0.resize(5);
		CHECK_EQ(vec0.size(), 5)
		vec0.resize(8);
		CHECK_EQ(vec0.size(), 8)
		CHECK_EQ(vec0.capacity(), 10)

		mc::vector<vec_test> vec1;
		CHECK_EQ(vec1.size(), 0)
		vec1.resize(5);
		CHECK_EQ(vec1.size(), 5)
		for (uint32_t i {0}; i < vec1.size(); ++i)
			CHECK_EQ(vec1[i].val_, -1)

		mc::vector<int32_t> vec2;
		CHECK_EQ(vec2.size(), 0)
		CHECK_EQ(vec2.capacity(), 0)
		vec2.resize(5, 42);
		CHECK_EQ(vec2.size(), 5)
		for (uint32_t i {0}; i < vec2.size(); ++i)
			CHECK_EQ(vec2[i], 42)

		mc::vector<vec_test_copy> vec3;
		CHECK_EQ(vec3.size(), 0)
		vec3.resize(5, vec_test_copy(42));
		CHECK_EQ(vec3.size(), 5)
		for (uint32_t i {0}; i < vec3.size(); ++i)
			CHECK_EQ(vec3[i].val_, 42)
	}

	TEST(emplace_back)
	{
		mc::vector<int32_t> vec0;
		CHECK_EQ(vec0.size(), 0)
		CHECK_EQ(vec0.capacity(), 0)

		int32_t& val0 = vec0.emplace_back(0);
		CHECK_EQ(vec0.size(), 1)
		CHECK_EQ(vec0.capacity(), 1)
		CHECK_EQ(val0, 0)

		int32_t val1 = vec0.emplace_back(1);
		CHECK_EQ(vec0.size(), 2)
		CHECK_EQ(vec0.capacity(), 2)
		CHECK_EQ(val1, 1)

		int32_t val2 = vec0.emplace_back(2);
		CHECK_EQ(vec0.size(), 3)
		CHECK_EQ(vec0.capacity(), 4)
		CHECK_EQ(val2, 2)

		for (int32_t i {0}; i < static_cast<int32_t>(vec0.size()); ++i)
			CHECK_EQ(vec0[i], i)

		mc::vector<vec_test> vec1;
		CHECK_EQ(vec1.size(), 0)
		CHECK_EQ(vec1.capacity(), 0)

		vec_test& val3 = vec1.emplace_back(40);
		CHECK_EQ(vec1.size(), 1)
		CHECK_EQ(vec1.capacity(), 1)
		CHECK_EQ(val3.val_, 40)

		vec_test& val4 = vec1.emplace_back(41);
		CHECK_EQ(vec1.size(), 2)
		CHECK_EQ(vec1.capacity(), 2)
		CHECK_EQ(val4.val_, 41)

		vec_test& val5 = vec1.emplace_back(42);
		CHECK_EQ(vec1.size(), 3)
		CHECK_EQ(vec1.capacity(), 4)
		CHECK_EQ(val5.val_, 42)

		for (int32_t i {0}; i < static_cast<int32_t>(vec1.size()); ++i)
			CHECK_EQ(vec1[i].val_, 40 + i)
	}

	TEST(pop_back)
	{
		mc::vector<int32_t> vec0 {0, 1, 2, 3, 4};
		CHECK_EQ(vec0.size(), 5)
		CHECK_EQ(vec0.capacity(), 5)

		vec0.pop_back();
		CHECK_EQ(vec0.size(), 4)
		CHECK_EQ(vec0.capacity(), 5)
		CHECK_EQ(vec0[vec0.size() - 1], 3)

		mc::vector<vec_test_copy> vec1 {1, 2, 3, 4, 5};
		CHECK_EQ(vec1.size(), 5)
		CHECK_EQ(vec1.capacity(), 5)
		vec1.pop_back();
		CHECK_EQ(vec1.size(), 4)
		CHECK_EQ(vec1.capacity(), 5)
		CHECK_EQ(vec1[vec1.size() - 1].val_, 4)
		CHECK_EQ(vec1[vec1.size()].val_, 0)
	}

	TEST(emplace)
	{
		mc::vector<int32_t> vec0 {0, 1, 2, 3, 4};
		CHECK_EQ(vec0.size(), 5)
		CHECK_EQ(vec0.capacity(), 5)

		uint32_t pos0 = vec0.emplace(1, 5);
		CHECK_EQ(vec0.size(), 6)
		CHECK_EQ(pos0, 1)
		CHECK_EQ(vec0[pos0], 5)

		uint32_t pos1 = vec0.emplace(6, 6);
		CHECK_EQ(vec0.size(), 7)
		CHECK_EQ(pos1, 6)
		CHECK_EQ(vec0[pos1], 6)

		mc::vector<vec_test_copy> vec1 {0, 1, 2, 3, 4};
		CHECK_EQ(vec1.size(), 5)
		CHECK_EQ(vec1.capacity(), 5)

		uint32_t pos2 = vec1.emplace(1, 5);
		CHECK_EQ(vec1.size(), 6)
		CHECK_EQ(pos2, 1)
		CHECK_EQ(vec1[pos2].val_, 5)

		uint32_t pos3 = vec1.emplace(6, 6);
		CHECK_EQ(vec1.size(), 7)
		CHECK_EQ(pos3, 6)
		CHECK_EQ(vec1[pos3].val_, 6)
	}

	TEST(insert)
	{
		mc::vector<int32_t> vec0;
		vec0.insert(0, 0, 4);
		CHECK_EQ(vec0.size(), 4)
		CHECK_EQ(vec0.capacity(), 4)
		for (uint32_t i {0}; i < vec0.size(); ++i)
			CHECK_EQ(vec0[i], 0)

		uint32_t pos0 = vec0.insert(1, 1, 4);
		CHECK_EQ(vec0.size(), 8)
		CHECK_EQ(pos0, 1)
		for (uint32_t i {pos0}; i < pos0 + 4; ++i)
			CHECK_EQ(vec0[i], 1)

		uint32_t pos1 = vec0.insert(0, {2, 3, 4});
		CHECK_EQ(vec0.size(), 11)
		CHECK_EQ(pos1, 0)
		for (uint32_t i {pos1}; i < pos1 + 3; ++i)
			CHECK_EQ(vec0[i], static_cast<int32_t>(i + 2))

		mc::vector<vec_test_copy> vec1;
		vec1.insert(0, 0, 4);
		CHECK_EQ(vec1.size(), 4)
		CHECK_EQ(vec1.capacity(), 4)
		for (uint32_t i {0}; i < vec1.size(); ++i)
			CHECK_EQ(vec1[i].val_, 0)

		uint32_t pos2 = vec1.insert(1, 1, 4);
		CHECK_EQ(vec1.size(), 8)
		CHECK_EQ(pos2, 1)
		for (uint32_t i {pos2}; i < pos2 + 4; ++i)
			CHECK_EQ(vec1[i].val_, 1)

		uint32_t pos3 = vec1.insert(0, {2, 3, 4});
		CHECK_EQ(vec1.size(), 11)
		CHECK_EQ(pos3, 0)
		for (uint32_t i {pos3}; i < pos3 + 3; ++i)
			CHECK_EQ(vec1[i].val_, static_cast<int32_t>(i + 2))

		mc::vector<vec_test_move> vec2;
		vec2.insert(0, 40);
		CHECK_EQ(vec2.size(), 1)
		vec2.insert(0, 41);
		CHECK_EQ(vec2.size(), 2)
		vec2.insert(0, 42);
		CHECK_EQ(vec2.size(), 3)
		for (uint32_t i {0}; i < vec2.size(); ++i)
			CHECK_EQ(vec2[i].val_, static_cast<int32_t>(42 - i))
	}

	TEST(erase)
	{
		mc::vector<int32_t> vec0 {0, 1, 2, 3, 4};
		CHECK_EQ(vec0.size(), 5)
		CHECK_EQ(vec0.capacity(), 5)

		uint32_t pos0 = vec0.erase(1, 2);
		CHECK_EQ(vec0.size(), 3)
		CHECK_EQ(pos0, 1)
		CHECK_EQ(vec0[pos0], 3)

		mc::vector<vec_test_move> vec1;
		vec1.emplace_back(0);
		vec1.emplace_back(1);
		vec1.emplace_back(2);
		vec1.emplace_back(3);
		vec1.emplace_back(4);
		CHECK_EQ(vec1.size(), 5)

		uint32_t pos1 = vec1.erase(1, 2);
		CHECK_EQ(vec1.size(), 3)
		CHECK_EQ(pos1, 1)
		CHECK_EQ(vec1[pos1].val_, 3)
	}

	TEST(swap_pop_back)
	{
		mc::vector<int32_t> vec0 {0, 1, 2, 3, 4};
		CHECK_EQ(vec0.size(), 5)
		CHECK_EQ(vec0.capacity(), 5)

		vec0.swap_pop_back(2);
		CHECK_EQ(vec0.size(), 4)
		CHECK_EQ(vec0[2], 4)

		vec0.swap_pop_back(3);
		CHECK_EQ(vec0.size(), 3)
		CHECK_EQ(vec0[2], 4)

		mc::vector<vec_test_move> vec1;
		vec1.emplace_back(0);
		vec1.emplace_back(1);
		vec1.emplace_back(2);
		vec1.emplace_back(3);
		vec1.emplace_back(4);
		CHECK_EQ(vec1.size(), 5)

		vec1.swap_pop_back(2);
		CHECK_EQ(vec1.size(), 4)
		CHECK_EQ(vec1[2].val_, 4)

		vec1.swap_pop_back(3);
		CHECK_EQ(vec1.size(), 3)
		CHECK_EQ(vec1[2].val_, 4)
	}
}
