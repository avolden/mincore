#include "string.hh"

#include "alloc.hh"

#include <cstring>
#include <string.h>

namespace mc
{
#define LEN(str) ((str).len_ & ~is_large_flag)
#define SET_LEN(str, new_len) ((str).len_ = (new_len) | ((str).len_ & is_large_flag))
#define DATA(str)                                                                        \
	(((str).len_ & is_large_flag) == 0 ? (str).small_.str_ : (str).large_.str_)
#define IS_LARGE(str) (((str).len_ & is_large_flag) != 0)

	// The user-defined ctor is needed because clang and gcc (< 13) misunderstand the
	// union not being trivially constructible.
	// See https://github.com/llvm/llvm-project/pull/82407 for the clang fix.
	string::string()
	: small_ {0}
	{}

	string::string(uint32_t count, char c)
	{
		count = count & ~is_large_flag;

		realloc(count, true);
		char* data = DATA(*this);

		memset(data, c, count);
		data[count] = '\0';
		SET_LEN(*this, count);
	}

	string::string(char const* str, uint32_t count)
	{
		if (count == UINT32_MAX)
			count = strlen(str);
		count &= ~is_large_flag;

		realloc(count, true);
		char* data = DATA(*this);

		memcpy(data, str, count);
		data[count] = '\0';
		SET_LEN(*this, count);
	}

	string::string(string_view str, uint32_t pos, uint32_t count)
	{
		if (count == UINT32_MAX)
			count = str.size() - pos;
		count &= ~is_large_flag;

		realloc(count, true);
		char* data = DATA(*this);

		memcpy(data, str.data(), count);
		data[count] = '\0';
		SET_LEN(*this, count);
	}

	string::string(string const& str, uint32_t pos, uint32_t count)
	{
		if (count == UINT32_MAX)
			count = LEN(str) - pos;
		count &= ~is_large_flag;

		realloc(count, true);
		char* data = DATA(*this);

		memcpy(data, DATA(str) + pos, count);
		data[count] = '\0';
		SET_LEN(*this, count);
	}

	string::string(string&& str, uint32_t pos, uint32_t count)
	{
		if (count == UINT32_MAX)
			count = LEN(str) - pos;
		count &= ~is_large_flag;

		if (count > small_size)
		{
			large_ = str.large_;
			memmove(large_.str_, large_.str_ + pos, count);
			large_.str_[count] = '\0';
			str.large_.str_ = nullptr;
			str.large_.cap_ = 0;

			len_ = count | is_large_flag;
		}
		else
		{
			small_ = str.small_;
			memmove(small_.str_, small_.str_ + pos, count);
			small_.str_[count] = '\0';

			len_ = count;
		}

		str.len_ = 0;
	}

	string::string(std::initializer_list<char> ilist)
	: len_ {static_cast<uint32_t>(ilist.size())}
	{
		uint32_t count = ilist.size() & ~is_large_flag;
		realloc(count, true);
		char* data = DATA(*this);

		memcpy(data, ilist.begin(), count);
		data[count] = '\0';
		SET_LEN(*this, count);
	}

	string::~string()
	{
		if (IS_LARGE(*this))
			free(large_.str_, large_.cap_ + 1, alignof(char));
	}

	bool string::empty() const
	{
		return LEN(*this) == 0;
	}

	uint32_t string::size() const
	{
		return LEN(*this);
	}

	uint32_t string::capacity() const
	{
		return IS_LARGE(*this) ? large_.cap_ : small_size - 1;
	}

	char* string::data() &
	{
		return DATA(*this);
	}

	char const* string::data() const&
	{
		return DATA(*this);
	}

	string::operator string_view() const&
	{
		return string_view(DATA(*this), size());
	}

	char& string::operator[](uint32_t pos) &
	{
		return DATA(*this)[pos];
	}

	char const& string::operator[](uint32_t pos) const&
	{
		return DATA(*this)[pos];
	}

	char& string::front() &
	{
		return DATA(*this)[0];
	}

	char const& string::front() const&
	{
		return DATA(*this)[0];
	}

	char& string::back() &
	{
		return DATA(*this)[LEN(*this) - 1];
	}

	char const& string::back() const&
	{
		return DATA(*this)[LEN(*this) - 1];
	}

	void string::clear()
	{
		DATA(*this)[0] = '\0';
		SET_LEN(*this, 0);
	}

	void string::reserve(uint32_t cap)
	{
		realloc(cap, true);
	}

	void string::fit()
	{
		if (IS_LARGE(*this))
		{
			uint32_t len = LEN(*this);
			if (len != large_.cap_)
			{
				char* new_str = reinterpret_cast<char*>(alloc(len + 1, alignof(char)));
				memcpy(new_str, large_.str_, LEN(*this) + 1);
				free(large_.str_, large_.cap_ + 1, alignof(char));

				large_.str_ = new_str;
				large_.cap_ = len;
			}
		}
	}

	void string::resize(uint32_t size, char c)
	{
		size &= ~is_large_flag;
		if (size > LEN(*this))
		{
			realloc(size, false);
			char* data = DATA(*this);

			memset(data + LEN(*this), c, size - LEN(*this));
			data[size] = '\0';

			len_ = size | (len_ & is_large_flag);
		}
		else
		{
			DATA(*this)[size] = '\0';

			len_ = size | (len_ & is_large_flag);
		}
	}

	void string::assign(uint32_t count, char c)
	{
		count &= ~is_large_flag;
		realloc(count, true);
		char* data = DATA(*this);

		memset(data, c, count);
		data[count] = '\0';

		SET_LEN(*this, count);
	}

	void string::assign(char const* str, uint32_t count)
	{
		if (count == UINT32_MAX)
			count = strlen(str);
		count &= ~is_large_flag;

		realloc(count, true);
		char* data = DATA(*this);

		memcpy(data, str, count);
		data[count] = '\0';

		SET_LEN(*this, count);
	}

	void string::assign(string_view str, uint32_t pos, uint32_t count)
	{
		if (count == UINT32_MAX)
			count = str.size() - pos;
		count &= ~is_large_flag;

		assign(str.data() + pos, count);
	}

	void string::assign(string const& str, uint32_t pos, uint32_t count)
	{
		if (count == UINT32_MAX)
			count = LEN(str) - pos;
		count &= ~is_large_flag;

		assign(str.data() + pos, count);
	}

	void string::assign(std::initializer_list<char> ilist)
	{
		uint32_t count = static_cast<uint32_t>(ilist.size()) & ~is_large_flag;

		assign(ilist.begin(), count);
	}

	void string::assign(string&& str)
	{
		if (IS_LARGE(*this))
			free(large_.str_, large_.cap_ + 1, alignof(char));

		len_ = str.len_;
		if (IS_LARGE(*this))
		{
			large_.str_ = str.large_.str_;
			large_.cap_ = str.large_.cap_;
		}
		else
		{
			memcpy(small_.str_, str.small_.str_, len_ + 1);
		}

		str.len_ = 0;
	}

	string& string::operator=(char const* str)
	{
		assign(str);
		return *this;
	}

	string& string::operator=(string_view str)
	{
		assign(str);
		return *this;
	}

	string& string::operator=(string const& str)
	{
		assign(str);
		return *this;
	}

	string& string::operator=(string&& str)
	{
		assign(static_cast<string&&>(str));
		return *this;
	}

	string& string::operator=(std::initializer_list<char> ilist)
	{
		assign(ilist);

		return *this;
	}

	void string::insert(uint32_t idx, uint32_t count, char c)
	{
		if (idx == len_)
		{
			append(count, c);
		}
		else
		{
			count &= ~is_large_flag;

			uint32_t new_len = LEN(*this) + count;

			realloc_sparse(new_len, false, idx, idx + count, idx);
			memset(DATA(*this) + idx, c, count);
			SET_LEN(*this, new_len);
		}
	}

	void string::insert(uint32_t idx, char const* str, uint32_t count)
	{
		if (idx == len_)
		{
			append(str, count);
		}
		else
		{
			if (count == UINT32_MAX)
				count = strlen(str);
			count &= ~is_large_flag;

			uint32_t new_len = LEN(*this) + count;

			realloc_sparse(new_len, false, idx, idx + count, idx);
			memcpy(DATA(*this) + idx, str, count);
			SET_LEN(*this, new_len);
		}
	}

	void string::insert(uint32_t idx, string const& str, uint32_t pos, uint32_t count)
	{
		if (count == UINT32_MAX)
			count = LEN(str) - pos;

		count &= ~is_large_flag;

		insert(idx, str.data() + pos, count);
	}

	void string::insert(uint32_t idx, string_view const& str, uint32_t pos,
	                    uint32_t count)
	{
		if (count == UINT32_MAX)
			count = str.size() - pos;

		count &= ~is_large_flag;

		insert(idx, str.data() + pos, count);
	}

	void string::insert(uint32_t idx, std::initializer_list<char> ilist)
	{
		uint32_t count = ilist.size() & ~is_large_flag;

		insert(idx, ilist.begin(), count);
	}

	void string::append(uint32_t count, char c)
	{
		count &= ~is_large_flag;

		uint32_t len = LEN(*this);
		uint32_t new_len = len + count;
		realloc(new_len, false);

		char* data = DATA(*this);
		memset(data + len, c, count);
		data[new_len] = '\0';

		SET_LEN(*this, new_len);
	}

	void string::append(char const* str, uint32_t count)
	{
		if (count == UINT32_MAX)
			count = strlen(str);

		count &= ~is_large_flag;

		uint32_t len = LEN(*this);
		uint32_t new_len = len + count;
		realloc(new_len, false);

		char* data = DATA(*this);
		memcpy(data + len, str, count);
		data[new_len] = '\0';

		SET_LEN(*this, new_len);
	}

	void string::append(string const& str, uint32_t pos, uint32_t count)
	{
		if (count == UINT32_MAX)
			count = LEN(str) - pos;

		count &= ~is_large_flag;

		append(str.data() + pos, count);
	}

	void string::append(string_view const& str, uint32_t pos, uint32_t count)
	{
		if (count == UINT32_MAX)
			count = str.size() - pos;

		count &= ~is_large_flag;

		append(str.data() + pos, count);
	}

	void string::append(std::initializer_list<char> ilist)
	{
		uint32_t count = ilist.size() & ~is_large_flag;

		append(ilist.begin(), count);
	}

	string& string::operator+=(char c)
	{
		append(1, c);
		return *this;
	}

	string& string::operator+=(char const* str)
	{
		append(str);
		return *this;
	}

	string& string::operator+=(string_view str)
	{
		append(str);
		return *this;
	}

	string& string::operator+=(string const& str)
	{
		append(str);
		return *this;
	}

	string& string::operator+=(std::initializer_list<char> ilist)
	{
		append(ilist);
		return *this;
	}

	void string::replace(uint32_t idx, uint32_t count, uint32_t count2, char c)
	{
		uint32_t len = LEN(*this);
		count = len - idx < count ? len - idx : count;

		if (count == 0)
		{
			insert(idx, count2, c);
		}
		else
		{
			count &= ~is_large_flag;
			count2 &= ~is_large_flag;

			uint32_t new_len = len - count + count2;
			realloc_sparse(new_len, false, idx, idx + count2, idx + count);

			char* data = DATA(*this);
			memset(data + idx, c, count2);
			SET_LEN(*this, new_len);
		}
	}

	void string::replace(uint32_t idx, uint32_t count, char const* str, uint32_t count2)
	{
		uint32_t len = LEN(*this);
		count = len - idx < count ? len - idx : count;

		if (count == 0)
		{
			insert(idx, str, count2);
		}
		else
		{
			if (count2 == UINT32_MAX)
				count2 = strlen(str);

			count &= ~is_large_flag;
			count2 &= ~is_large_flag;

			uint32_t new_len = len - count + count2;
			realloc_sparse(new_len, false, idx, idx + count2, idx + count);

			char* data = DATA(*this);
			memcpy(data + idx, str, count2);
			SET_LEN(*this, new_len);
		}
	}

	void string::replace(uint32_t idx, uint32_t count, string const& str, uint32_t pos,
	                     uint32_t count2)
	{
		count = LEN(*this) - idx < count ? LEN(*this) - idx : count;

		replace(idx, count, str.data() + pos, count2);
	}

	void string::replace(uint32_t idx, uint32_t count, string_view const& str,
	                     uint32_t pos, uint32_t count2)
	{
		count = LEN(*this) - idx < count ? LEN(*this) - idx : count;

		replace(idx, count, str.data() + pos, count2);
	}

	void string::replace(uint32_t idx, uint32_t count, std::initializer_list<char> ilist)
	{
		count = LEN(*this) - idx < count ? LEN(*this) - idx : count;
		uint32_t count2 = ilist.size() & ~is_large_flag;

		replace(idx, count, ilist.begin(), count2);
	}

	void string::erase(uint32_t idx, uint32_t count)
	{
		uint32_t len = LEN(*this);
		count = len - idx < count ? len - idx : count;

		if (count)
		{
			memmove(DATA(*this) + idx, DATA(*this) + idx + count, len - idx - count + 1);

			SET_LEN(*this, len - count);
		}
	}

	void string::push_back(char c)
	{
		uint32_t new_len = LEN(*this) + 1;
		realloc(new_len, false);

		char* data = DATA(*this);
		data[new_len - 1] = c;
		data[new_len] = '\0';

		SET_LEN(*this, new_len);
	}

	void string::pop_back()
	{
		DATA(*this)[LEN(*this) - 1] = '\0';
		len_ = LEN(*this) - 1 | (len_ & is_large_flag);
	}

	string string::substr(uint32_t pos, uint32_t size) const&
	{
		return {*this, pos, size};
	}

	string string::substr(uint32_t pos, uint32_t size) &&
	{
		return {static_cast<string&&>(*this), pos, size};
	}

	int32_t string::compare(char const* str, uint32_t count) const
	{
		if (count == UINT32_MAX)
			count = strlen(str);
		count = count & ~is_large_flag;

		uint32_t len = LEN(*this) < count ? LEN(*this) : count;
		int32_t  res = strncmp(DATA(*this), str, len);

		if (res == 0)
		{
			if (LEN(*this) < count)
				return -1;
			else if (LEN(*this) > count)
				return 1;
		}

		return res;
	}

	int32_t string::compare(string const& str) const
	{
		return compare(DATA(str), LEN(str));
	}

	int32_t string::compare(string_view str) const
	{
		return compare(str.data(), str.size());
	}

	bool string::starts_with(char c, uint32_t count) const
	{
		char const* data = DATA(*this);
		for (uint32_t i {0}; i < count; ++i)
			if (data[i] != c)
				return false;
		return true;
	}

	bool string::starts_with(char const* str, uint32_t count) const
	{
		if (count == UINT32_MAX)
			count = strlen(str);
		count = count & ~is_large_flag;

		return strncmp(DATA(*this), str, count) == 0;
	}

	bool string::starts_with(string const& str) const
	{
		return starts_with(DATA(str), LEN(str));
	}

	bool string::starts_with(string_view str) const
	{
		return starts_with(str.data(), str.size());
	}

	bool string::ends_with(char c, uint32_t count) const
	{
		uint32_t    len = LEN(*this);
		char const* data = DATA(*this);
		for (uint32_t i {LEN(*this)}; i > len - count; --i)
			if (data[i - 1] != c)
				return false;
		return true;
	}

	bool string::ends_with(char const* str, uint32_t count) const
	{
		if (count == UINT32_MAX)
			count = strlen(str);
		count = count & ~is_large_flag;
		return strncmp(DATA(*this) + LEN(*this) - count, str, count) == 0;
	}

	bool string::ends_with(string const& str) const
	{
		return ends_with(DATA(str), LEN(str));
	}

	bool string::ends_with(string_view str) const
	{
		return ends_with(str.data(), str.size());
	}

	bool string::contains(char c, uint32_t count) const
	{
		uint32_t crt_count {0};
		for (uint32_t i {0}; i < LEN(*this); ++i)
		{
			if (DATA(*this)[i] == c)
			{
				++crt_count;
				if (crt_count == count)
					return true;
			}
			else
				crt_count = 0;
		}
		return false;
	}

	bool string::contains(char const* str, uint32_t count) const
	{
		if (count == UINT32_MAX)
			count = strlen(str);
		count = count & ~is_large_flag;

		if (count > LEN(*this) || count == 0)
			return false;

		for (uint32_t i {0}, j {0}; i < LEN(*this); ++i)
		{
			if (LEN(*this) - i < count - j)
				return false;

			if (DATA(*this)[i] == str[j])
			{
				++j;
				if (j == count)
					return true;
			}
			else
				j = 0;
		}
		return false;
	}

	bool string::contains(string const& str) const
	{
		return contains(DATA(str), LEN(str));
	}

	bool string::contains(string_view str) const
	{
		return contains(str.data(), str.size());
	}

	uint32_t string::find(char c, uint32_t pos, uint32_t count) const
	{
		uint32_t crt_count {0};
		for (uint32_t i {pos}; i < LEN(*this); ++i)
		{
			if (DATA(*this)[i] == c)
			{
				++crt_count;
				if (crt_count == count)
					return i - (crt_count - 1);
			}
		}
		return UINT32_MAX;
	}

	uint32_t string::find(char const* str, uint32_t pos, uint32_t count) const
	{
		if (count == UINT32_MAX)
			count = strlen(str);
		count = count & ~is_large_flag;

		char const* data {DATA(*this)};
		uint32_t    size {LEN(*this)};

		char const* data2 {str};

		if (count > size - pos || count == 0)
			return UINT32_MAX;

		uint32_t max_len {size - count + 1};
		char     first_char {data2[0]};

		while (pos < max_len)
		{
			while ((data[pos] != first_char) && (pos < max_len))
				++pos;

			uint32_t j {1}, k {pos + 1};
			while (data[k] == data2[j])
			{
				++j;
				++k;

				if (k == pos + count)
					return pos;

				if (k == size)
					return UINT32_MAX;
			}

			++pos;
		}

		return UINT32_MAX;
	}

	uint32_t string::find(string const& str, uint32_t pos) const
	{
		return find(DATA(str), pos, LEN(str));
	}

	uint32_t string::find(string_view str, uint32_t pos) const
	{
		return find(str.data(), pos, str.size());
	}

	uint32_t string::rfind(char c, uint32_t pos, uint32_t count) const
	{
		if (pos == UINT32_MAX)
			pos = LEN(*this) - 1;

		uint32_t crt_count {0};
		for (uint32_t i {pos + 1}; i > 0; --i)
		{
			if (DATA(*this)[i - 1] == c)
			{
				++crt_count;
				if (crt_count == count)
					return i - crt_count;
			}
		}
		return UINT32_MAX;
	}

	uint32_t string::rfind(char const* str, uint32_t pos, uint32_t count) const
	{
		if (count == UINT32_MAX)
			count = strlen(str);
		count = count & ~is_large_flag;

		char const* data {DATA(*this)};
		uint32_t    size {LEN(*this)};

		char const* data2 {str};

		if (pos == UINT32_MAX)
			pos = size;

		if (count + 1 > pos || count == 0)
			return UINT32_MAX;

		char first_char {data2[count - 1]};

		while (pos > 0)
		{
			while ((pos > 0) && (data[pos - 1] != first_char))
				--pos;

			if (pos == 0)
				return UINT32_MAX;

			uint32_t j {count - 1}, k {pos - 1};
			while (data[k - 1] == data2[j - 1])
			{
				--j;
				--k;

				if (k == pos - count)
					return k;

				if (k == 0)
					return UINT32_MAX;
			}

			--pos;
		}
		return UINT32_MAX;
	}

	uint32_t string::rfind(string const& str, uint32_t pos) const
	{
		return rfind(DATA(str), pos, LEN(str));
	}

	uint32_t string::rfind(string_view str, uint32_t pos) const
	{
		return rfind(str.data(), pos, str.size());
	}

	bool string::operator==(string const& str) const
	{
		if (LEN(*this) != LEN(str))
			return false;
		return strncmp(DATA(*this), DATA(str), LEN(*this)) == 0;
	}

	bool string::operator==(string_view str) const
	{
		if (LEN(*this) != str.size())
			return false;
		return strncmp(DATA(*this), str.data(), LEN(*this)) == 0;
	}

	bool string::operator==(char const* str) const
	{
		if (LEN(*this) != strlen(str))
			return false;
		return strncmp(DATA(*this), str, LEN(*this)) == 0;
	}

	std::strong_ordering string::operator<=>(string const& str) const
	{
		if (this == &str)
			return std::strong_ordering::equal;

		int32_t res = compare(str);

		if (res > 0)
			return std::strong_ordering::greater;
		else if (res < 0)
			return std::strong_ordering::less;
		else
			return std::strong_ordering::equivalent;
	}

	std::strong_ordering string::operator<=>(string_view str) const
	{
		int32_t res = compare(str);

		if (res > 0)
			return std::strong_ordering::greater;
		else if (res < 0)
			return std::strong_ordering::less;
		else
			return std::strong_ordering::equivalent;
	}

	std::strong_ordering string::operator<=>(char const* str) const
	{
		int32_t res = compare(str);

		if (res > 0)
			return std::strong_ordering::greater;
		else if (res < 0)
			return std::strong_ordering::less;
		else
			return std::strong_ordering::equivalent;
	}

	void string::realloc(uint32_t cap, bool precise)
	{
		if (!precise)
		{
			uint32_t crt_cap = capacity();
			while (crt_cap < cap)
				crt_cap *= 2;

			cap = crt_cap;
		}

		if (cap > small_size)
		{
			if (IS_LARGE(*this))
			{
				char* new_str = reinterpret_cast<char*>(alloc(cap + 1, alignof(char)));
				memcpy(new_str, large_.str_, LEN(*this) + 1);
				free(large_.str_, large_.cap_ + 1, alignof(char));

				large_.str_ = new_str;
				large_.cap_ = cap;
			}
			else
			{
				char* new_str = reinterpret_cast<char*>(alloc(cap + 1, alignof(char)));
				memcpy(new_str, small_.str_, LEN(*this) + 1);

				large_.str_ = new_str;
				large_.cap_ = cap;

				len_ |= is_large_flag;
			}
		}
	}

	void string::realloc_sparse(uint32_t cap, bool precise, uint32_t cut1_end,
	                            uint32_t cut2_start, uint32_t cut2_pos)
	{
		if (!precise)
		{
			uint32_t crt_cap = capacity();
			while (crt_cap < cap)
				crt_cap *= 2;

			cap = crt_cap;
		}

		if (cap > small_size)
		{
			if (IS_LARGE(*this))
			{
				char* new_str = reinterpret_cast<char*>(alloc(cap + 1, alignof(char)));
				memcpy(new_str, large_.str_, cut1_end);
				memcpy(new_str + cut2_start, large_.str_ + cut2_pos,
				       LEN(*this) - cut2_pos + 1);
				free(large_.str_, large_.cap_ + 1, alignof(char));

				large_.str_ = new_str;
				large_.cap_ = cap;
			}
			else
			{
				char* new_str = reinterpret_cast<char*>(alloc(cap + 1, alignof(char)));
				memcpy(new_str, small_.str_, cut1_end);
				memcpy(new_str + cut2_start, small_.str_ + cut2_pos,
				       LEN(*this) - cut2_pos + 1);

				large_.str_ = new_str;
				large_.cap_ = cap;
				len_ |= is_large_flag;
			}
		}
		else
		{
			memmove(small_.str_ + cut2_start, small_.str_ + cut2_pos,
			        LEN(*this) - cut2_pos);
		}
	}
} // namespace mc
