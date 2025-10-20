#include "string.hh"

#include "alloc.hh"

#include <string.h>

namespace mc
{
#define SIZE(str) ((str).len_ & ~is_large_flag)
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
	: len_ {count & ~is_large_flag}
	{
		if (len_ < small_size)
		{
			memset(small_.str_, c, len_);
			small_.str_[len_] = '\0';
		}
		else
		{
			large_.str_ = reinterpret_cast<char*>(alloc(len_ + 1, alignof(char)));
			large_.cap_ = len_;
			memset(large_.str_, c, len_);
			large_.str_[len_] = '\0';
			len_ |= is_large_flag;
		}
	}

	string::string(char const* str, uint32_t count)
	{
		if (count == UINT32_MAX)
			len_ = strlen(str);
		else
			len_ = count;

		len_ &= ~is_large_flag;

		if (len_ < small_size)
		{
			memcpy(small_.str_, str, len_);
			small_.str_[len_] = '\0';
		}
		else
		{
			large_.str_ = reinterpret_cast<char*>(alloc(len_ + 1, alignof(char)));
			large_.cap_ = len_;

			memcpy(large_.str_, str, len_);
			large_.str_[len_] = '\0';
			len_ |= is_large_flag;
		}
	}

	string::string(string_view str, uint32_t pos, uint32_t count)
	{
		if (count == UINT32_MAX)
			len_ = str.size() - pos;
		else
			len_ = count;

		len_ &= ~is_large_flag;

		if (len_ < small_size)
		{
			memcpy(small_.str_, str.data() + pos, len_);
			small_.str_[len_] = '\0';
		}
		else
		{
			large_.str_ = reinterpret_cast<char*>(alloc(len_ + 1, alignof(char)));
			large_.cap_ = len_;

			memcpy(large_.str_, str.data() + pos, len_);
			large_.str_[len_] = '\0';
			len_ |= is_large_flag;
		}
	}

	string::string(string const& str, uint32_t pos, uint32_t count)
	{
		if (count == UINT32_MAX)
			len_ = SIZE(str) - pos;
		else
			len_ = count;

		len_ &= ~is_large_flag;

		if (IS_LARGE(str))
		{
			if (len_ < small_size)
			{
				memcpy(small_.str_, str.large_.str_ + pos, len_);
				small_.str_[len_] = '\0';
			}
			else
			{
				large_.str_ = reinterpret_cast<char*>(alloc(len_ + 1, alignof(char)));
				large_.cap_ = len_;

				memcpy(large_.str_, str.large_.str_ + pos, len_);
				large_.str_[len_] = '\0';
				len_ |= is_large_flag;
			}
		}
		else
		{
			memcpy(small_.str_, str.small_.str_, len_);
			small_.str_[len_] = '\0';
		}
	}

	string::string(string&& str, uint32_t pos, uint32_t count)
	{
		if (count == UINT32_MAX)
			len_ = SIZE(str) - pos;
		else
			len_ = count;

		len_ = SIZE(*this) | (str.len_ & is_large_flag);
		if (IS_LARGE(*this))
		{
			large_ = str.large_;
			memmove(large_.str_, large_.str_ + pos, SIZE(*this));
			large_.str_[SIZE(*this)] = '\0';
			str.large_.str_ = nullptr;
			str.large_.cap_ = 0;
		}
		else
		{
			small_ = str.small_;
			memmove(small_.str_, small_.str_ + pos, SIZE(*this));
			small_.str_[len_] = '\0';
		}

		str.len_ = 0;
	}

	string::string(std::initializer_list<char> ilist)
	: len_ {static_cast<uint32_t>(ilist.size())}
	{
		if (len_ < small_size)
		{
			memcpy(small_.str_, ilist.begin(), len_);
			small_.str_[len_] = '\0';
		}
		else
		{
			large_.str_ = reinterpret_cast<char*>(alloc(len_ + 1, alignof(char)));
			large_.cap_ = len_;

			memcpy(large_.str_, ilist.begin(), len_);
			large_.str_[len_] = '\0';
			len_ |= is_large_flag;
		}
	}

	string::~string()
	{
		if (IS_LARGE(*this))
			free(large_.str_, large_.cap_ + 1, alignof(char));
	}

	bool string::empty() const
	{
		return SIZE(*this) == 0;
	}

	uint32_t string::size() const
	{
		return SIZE(*this);
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
		return DATA(*this)[SIZE(*this) - 1];
	}

	char const& string::back() const&
	{
		return DATA(*this)[SIZE(*this) - 1];
	}

	void string::clear()
	{
		if (IS_LARGE(*this))
			large_.str_[0] = '\0';
		else
			small_.str_[0] = '\0';

		len_ = 0 | (len_ & is_large_flag);
	}

	void string::reserve(uint32_t cap)
	{
		if (cap > small_size)
		{
			if (IS_LARGE(*this))
			{
				char* new_str = reinterpret_cast<char*>(alloc(cap + 1, alignof(char)));
				memcpy(new_str, large_.str_, SIZE(*this) + 1);
				free(large_.str_, large_.cap_ + 1, alignof(char));

				large_.str_ = new_str;
				large_.cap_ = cap;
			}
			else
			{
				char* new_str = reinterpret_cast<char*>(alloc(cap + 1, alignof(char)));
				memcpy(new_str, small_.str_, SIZE(*this) + 1);
				free(large_.str_, large_.cap_ + 1, alignof(char));

				large_.str_ = new_str;
				large_.cap_ = cap;

				len_ |= is_large_flag;
			}
		}
	}

	void string::fit()
	{
		if (IS_LARGE(*this))
		{
			uint32_t len = SIZE(*this);
			if (len != large_.cap_)
			{
				char* new_str = reinterpret_cast<char*>(alloc(len + 1, alignof(char)));
				memcpy(new_str, large_.str_, SIZE(*this) + 1);
				free(large_.str_, large_.cap_ + 1, alignof(char));

				large_.str_ = new_str;
				large_.cap_ = len;
			}
		}
	}

	void string::resize(uint32_t size, char c)
	{
		size &= ~is_large_flag;
		if (size > SIZE(*this))
		{
			if (IS_LARGE(*this))
			{
				if (size > large_.cap_)
				{
					uint32_t new_cap = large_.cap_;
					while (new_cap < size)
						new_cap *= 2;

					char* new_str =
						reinterpret_cast<char*>(alloc(new_cap + 1, alignof(char)));

					memcpy(new_str, large_.str_, SIZE(*this));
					memset(new_str + SIZE(*this), c, size - SIZE(*this));
					new_str[size] = '\0';

					free(large_.str_, large_.cap_ + 1, alignof(char));

					large_.str_ = new_str;
					large_.cap_ = new_cap;
				}
				else
				{
					memset(large_.str_ + SIZE(*this), c, size - SIZE(*this));
					large_.str_[size] = '\0';
				}

				len_ = size | is_large_flag;
			}
			else if (size >= small_size)
			{
				char* new_str = reinterpret_cast<char*>(alloc(size + 1, alignof(char)));

				memcpy(new_str, small_.str_, SIZE(*this));
				memset(new_str + SIZE(*this), c, size - SIZE(*this));
				new_str[size] = '\0';

				large_.str_ = new_str;
				large_.cap_ = size;

				len_ = size | is_large_flag;
			}
			else
			{
				memset(small_.str_ + SIZE(*this), c, size - SIZE(*this));
				small_.str_[size] = '\0';

				len_ = size;
			}
		}
		else if (IS_LARGE(*this))
		{
			large_.str_[size] = '\0';

			len_ = size | (len_ & is_large_flag);
		}
		else
		{
			small_.str_[size] = '\0';
			len_ = size;
		}
	}

	void string::assign(uint32_t count, char c)
	{
		count &= ~is_large_flag;

		if (IS_LARGE(*this))
		{
			if (count >= large_.cap_)
			{
				uint32_t new_cap = large_.cap_;
				while (new_cap < count)
					new_cap *= 2;

				char* new_str =
					reinterpret_cast<char*>(alloc(new_cap + 1, alignof(char)));

				free(large_.str_, large_.cap_, alignof(char));
				large_.cap_ = new_cap;
				large_.str_ = new_str;
			}
			memset(large_.str_, c, count);
			large_.str_[count] = '\0';
			len_ = count | is_large_flag;
		}
		else if (count >= small_size)
		{
			char* new_str = reinterpret_cast<char*>(alloc(count + 1, alignof(char)));

			large_.cap_ = count;
			large_.str_ = new_str;
			memset(large_.str_, c, count);
			large_.str_[count] = '\0';
			len_ = count | is_large_flag;
		}
		else
		{
			memset(small_.str_, c, count);
			small_.str_[count] = '\0';
			len_ = count;
		}
	}

	void string::assign(char const* str, uint32_t count)
	{
		if (count == UINT32_MAX)
			count = strlen(str);

		count &= ~is_large_flag;

		if (IS_LARGE(*this))
		{
			if (count >= large_.cap_)
			{
				uint32_t new_cap = large_.cap_;
				while (new_cap < count)
					new_cap *= 2;

				char* new_str =
					reinterpret_cast<char*>(alloc(new_cap + 1, alignof(char)));

				free(large_.str_, large_.cap_, alignof(char));
				large_.cap_ = new_cap;
				large_.str_ = new_str;
			}
			memcpy(large_.str_, str, count);
			large_.str_[count] = '\0';
			len_ = count | is_large_flag;
		}
		else if (count >= small_size)
		{
			char* new_str = reinterpret_cast<char*>(alloc(count + 1, alignof(char)));

			large_.cap_ = count;
			large_.str_ = new_str;
			memcpy(large_.str_, str, count);
			large_.str_[count] = '\0';
			len_ = count | is_large_flag;
		}
		else
		{
			memcpy(small_.str_, str, count);
			small_.str_[count] = '\0';
			len_ = count;
		}
	}

	void string::assign(string_view str, uint32_t pos, uint32_t count)
	{
		if (count == UINT32_MAX)
			count = str.size() - pos;

		count &= ~is_large_flag;

		if (IS_LARGE(*this))
		{
			if (count >= large_.cap_)
			{
				uint32_t new_cap = large_.cap_;
				while (new_cap < count)
					new_cap *= 2;

				char* new_str =
					reinterpret_cast<char*>(alloc(new_cap + 1, alignof(char)));

				free(large_.str_, large_.cap_, alignof(char));
				large_.cap_ = new_cap;
				large_.str_ = new_str;
			}
			memcpy(large_.str_, str.data() + pos, count);
			large_.str_[count] = '\0';
			len_ = count | is_large_flag;
		}
		else if (count >= small_size)
		{
			char* new_str = reinterpret_cast<char*>(alloc(count + 1, alignof(char)));

			large_.cap_ = count;
			large_.str_ = new_str;
			memcpy(large_.str_, str.data() + pos, count);
			large_.str_[count] = '\0';
			len_ = count | is_large_flag;
		}
		else
		{
			memcpy(small_.str_, str.data() + pos, count);
			small_.str_[count] = '\0';
			len_ = count;
		}
	}

	void string::assign(string const& str, uint32_t pos, uint32_t count)
	{
		if (count == UINT32_MAX)
			count = SIZE(str) - pos;

		count &= ~is_large_flag;

		if (IS_LARGE(*this))
		{
			if (count >= large_.cap_)
			{
				uint32_t new_cap = large_.cap_;
				while (new_cap < count)
					new_cap *= 2;

				char* new_str =
					reinterpret_cast<char*>(alloc(new_cap + 1, alignof(char)));

				free(large_.str_, large_.cap_, alignof(char));
				large_.cap_ = new_cap;
				large_.str_ = new_str;
			}
			memcpy(large_.str_, DATA(str) + pos, count);
			large_.str_[count] = '\0';
			len_ = count | is_large_flag;
		}
		else if (count >= small_size)
		{
			char* new_str = reinterpret_cast<char*>(alloc(count + 1, alignof(char)));

			large_.cap_ = count;
			large_.str_ = new_str;
			memcpy(large_.str_, DATA(str) + pos, count);
			large_.str_[count] = '\0';
			len_ = count | is_large_flag;
		}
		else
		{
			memcpy(small_.str_, DATA(str) + pos, count);
			small_.str_[count] = '\0';
			len_ = count;
		}
	}

	void string::assign(std::initializer_list<char> ilist)
	{
		uint32_t str_len = static_cast<uint32_t>(ilist.size()) & ~is_large_flag;

		if (IS_LARGE(*this))
		{
			if (str_len >= large_.cap_)
			{
				uint32_t new_cap = large_.cap_;
				while (new_cap < str_len)
					new_cap *= 2;

				char* new_str =
					reinterpret_cast<char*>(alloc(new_cap + 1, alignof(char)));

				free(large_.str_, large_.cap_, alignof(char));
				large_.cap_ = new_cap;
				large_.str_ = new_str;
			}
			memcpy(large_.str_, ilist.begin(), str_len);
			large_.str_[str_len] = '\0';
			len_ = str_len | is_large_flag;
		}
		else if (str_len >= small_size)
		{
			char* new_str = reinterpret_cast<char*>(alloc(str_len + 1, alignof(char)));

			large_.cap_ = str_len;
			large_.str_ = new_str;
			memcpy(large_.str_, ilist.begin(), str_len);
			large_.str_[str_len] = '\0';
			len_ = str_len | is_large_flag;
		}
		else
		{
			memcpy(small_.str_, ilist.begin(), str_len);
			small_.str_[str_len] = '\0';
			len_ = str_len;
		}
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

			uint32_t new_len = SIZE(*this) + count;
			if (IS_LARGE(*this))
			{
				if (new_len > large_.cap_)
				{
					uint32_t new_cap = large_.cap_;
					while (new_cap < new_len)
						new_cap *= 2;

					char* new_str =
						reinterpret_cast<char*>(alloc(new_cap + 1, alignof(char)));

					memcpy(new_str, large_.str_, idx);
					memmove(new_str + idx + count, large_.str_ + idx,
					        SIZE(*this) - idx + 1);
					memset(new_str + idx, c, count);

					free(large_.str_, large_.cap_ + 1, alignof(char));

					large_.str_ = new_str;
					large_.cap_ = new_cap;
				}
				else
				{
					memmove(large_.str_ + idx + count, large_.str_ + idx,
					        SIZE(*this) - idx + 1);
					memset(large_.str_ + idx, c, count);
				}

				len_ = new_len | is_large_flag;
			}
			else if (new_len >= small_size)
			{
				char* new_str =
					reinterpret_cast<char*>(alloc(new_len + 1, alignof(char)));

				memcpy(new_str, small_.str_, idx);
				memmove(new_str + idx + count, small_.str_ + idx, SIZE(*this) - idx + 1);
				memset(new_str + idx, c, count);

				large_.str_ = new_str;
				large_.cap_ = new_len;

				len_ = new_len | is_large_flag;
			}
			else
			{
				memmove(small_.str_ + idx + count, small_.str_ + idx,
				        SIZE(*this) - idx + 1);
				memset(small_.str_ + idx, c, count);

				len_ = new_len;
			}
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

			uint32_t new_len = SIZE(*this) + count;
			if (IS_LARGE(*this))
			{
				if (new_len > large_.cap_)
				{
					uint32_t new_cap = large_.cap_;
					while (new_cap < new_len)
						new_cap *= 2;

					char* new_str =
						reinterpret_cast<char*>(alloc(new_cap + 1, alignof(char)));

					memcpy(new_str, large_.str_, idx);
					memmove(new_str + idx + count, large_.str_ + idx,
					        SIZE(*this) - idx + 1);
					memcpy(new_str + idx, str, count);

					free(large_.str_, large_.cap_ + 1, alignof(char));

					large_.str_ = new_str;
					large_.cap_ = new_cap;
				}
				else
				{
					memmove(large_.str_ + idx + count, large_.str_ + idx,
					        SIZE(*this) - idx + 1);
					memcpy(large_.str_ + idx, str, count);
				}

				len_ = new_len | is_large_flag;
			}
			else if (new_len >= small_size)
			{
				char* new_str =
					reinterpret_cast<char*>(alloc(new_len + 1, alignof(char)));

				memcpy(new_str, small_.str_, idx);
				memmove(new_str + idx + count, small_.str_ + idx, SIZE(*this) - idx + 1);
				memcpy(new_str + idx, str, count);

				large_.str_ = new_str;
				large_.cap_ = new_len;

				len_ = new_len | is_large_flag;
			}
			else
			{
				memmove(small_.str_ + idx + count, small_.str_ + idx,
				        SIZE(*this) - idx + 1);
				memcpy(small_.str_ + idx, str, count);

				len_ = new_len;
			}
		}
	}

	void string::insert(uint32_t idx, string const& str, uint32_t pos, uint32_t count)
	{
		if (idx == len_)
		{
			append(str, pos, count);
		}
		else
		{
			if (count == UINT32_MAX)
				count = SIZE(str) - pos;

			count &= ~is_large_flag;

			uint32_t new_len = SIZE(*this) + count;
			if (IS_LARGE(*this))
			{
				if (new_len > large_.cap_)
				{
					uint32_t new_cap = large_.cap_;
					while (new_cap < new_len)
						new_cap *= 2;

					char* new_str =
						reinterpret_cast<char*>(alloc(new_cap + 1, alignof(char)));

					memcpy(new_str, large_.str_, idx);
					memmove(new_str + idx + count, large_.str_ + idx,
					        SIZE(*this) - idx + 1);
					memcpy(new_str + idx, DATA(str) + pos, count);

					free(large_.str_, large_.cap_ + 1, alignof(char));

					large_.str_ = new_str;
					large_.cap_ = new_cap;
				}
				else
				{
					memmove(large_.str_ + idx + count, large_.str_ + idx,
					        SIZE(*this) - idx + 1);
					memcpy(large_.str_ + idx, DATA(str) + pos, count);
				}

				len_ = new_len | is_large_flag;
			}
			else if (new_len >= small_size)
			{
				char* new_str =
					reinterpret_cast<char*>(alloc(new_len + 1, alignof(char)));

				memcpy(new_str, small_.str_, idx);
				memmove(new_str + idx + count, small_.str_ + idx, SIZE(*this) - idx + 1);
				memcpy(new_str + idx, DATA(str) + pos, count);

				large_.str_ = new_str;
				large_.cap_ = new_len;

				len_ = new_len | is_large_flag;
			}
			else
			{
				memmove(small_.str_ + idx + count, small_.str_ + idx,
				        SIZE(*this) - idx + 1);
				memcpy(small_.str_ + idx, DATA(str) + pos, count);

				len_ = new_len;
			}
		}
	}

	void string::insert(uint32_t idx, string_view const& str, uint32_t pos,
	                    uint32_t count)
	{
		if (idx == len_)
		{
			append(str, pos, count);
		}
		else
		{
			if (count == UINT32_MAX)
				count = str.size() - pos;

			count &= ~is_large_flag;

			uint32_t new_len = SIZE(*this) + count;
			if (IS_LARGE(*this))
			{
				if (new_len > large_.cap_)
				{
					uint32_t new_cap = large_.cap_;
					while (new_cap < new_len)
						new_cap *= 2;

					char* new_str =
						reinterpret_cast<char*>(alloc(new_cap + 1, alignof(char)));

					memcpy(new_str, large_.str_, idx);
					memmove(new_str + idx + count, large_.str_ + idx,
					        SIZE(*this) - idx + 1);
					memcpy(new_str + idx, str.data() + pos, count);

					free(large_.str_, large_.cap_ + 1, alignof(char));

					large_.str_ = new_str;
					large_.cap_ = new_cap;
				}
				else
				{
					memmove(large_.str_ + idx + count, large_.str_ + idx,
					        SIZE(*this) - idx + 1);
					memcpy(large_.str_ + idx, str.data() + pos, count);
				}

				len_ = new_len | is_large_flag;
			}
			else if (new_len >= small_size)
			{
				char* new_str =
					reinterpret_cast<char*>(alloc(new_len + 1, alignof(char)));

				memcpy(new_str, small_.str_, idx);
				memmove(new_str + idx + count, small_.str_ + idx, SIZE(*this) - idx + 1);
				memcpy(new_str + idx, str.data() + pos, count);

				large_.str_ = new_str;
				large_.cap_ = new_len;

				len_ = new_len | is_large_flag;
			}
			else
			{
				memmove(small_.str_ + idx + count, small_.str_ + idx,
				        SIZE(*this) - idx + 1);
				memcpy(small_.str_ + idx, str.data() + pos, count);

				len_ = new_len;
			}
		}
	}

	void string::insert(uint32_t idx, std::initializer_list<char> ilist)
	{
		if (idx == len_)
		{
			append(ilist);
		}
		else
		{
			uint32_t count = ilist.size() & ~is_large_flag;

			uint32_t new_len = SIZE(*this) + count;
			if (IS_LARGE(*this))
			{
				if (new_len > large_.cap_)
				{
					uint32_t new_cap = large_.cap_;
					while (new_cap < new_len)
						new_cap *= 2;

					char* new_str =
						reinterpret_cast<char*>(alloc(new_cap + 1, alignof(char)));

					memcpy(new_str, large_.str_, idx);
					memmove(new_str + idx + count, large_.str_ + idx,
					        SIZE(*this) - idx + 1);
					memcpy(new_str + idx, ilist.begin(), count);

					free(large_.str_, large_.cap_ + 1, alignof(char));

					large_.str_ = new_str;
					large_.cap_ = new_cap;
				}
				else
				{
					memmove(large_.str_ + idx + count, large_.str_ + idx,
					        SIZE(*this) - idx + 1);
					memcpy(large_.str_ + idx, ilist.begin(), count);
				}

				len_ = new_len | is_large_flag;
			}
			else if (new_len >= small_size)
			{
				char* new_str =
					reinterpret_cast<char*>(alloc(new_len + 1, alignof(char)));

				memcpy(new_str, small_.str_, idx);
				memmove(new_str + idx + count, small_.str_ + idx, SIZE(*this) - idx + 1);
				memcpy(new_str + idx, ilist.begin(), count);

				large_.str_ = new_str;
				large_.cap_ = new_len;

				len_ = new_len | is_large_flag;
			}
			else
			{
				memmove(small_.str_ + idx + count, small_.str_ + idx,
				        SIZE(*this) - idx + 1);
				memcpy(small_.str_ + idx, ilist.begin(), count);

				len_ = new_len;
			}
		}
	}

	void string::append(uint32_t count, char c)
	{
		count &= ~is_large_flag;

		uint32_t new_len = SIZE(*this) + count;
		if (IS_LARGE(*this))
		{
			if (new_len > large_.cap_)
			{
				uint32_t new_cap = large_.cap_;
				while (new_cap < new_len)
					new_cap *= 2;

				char* new_str =
					reinterpret_cast<char*>(alloc(new_cap + 1, alignof(char)));

				memcpy(new_str, large_.str_, SIZE(*this));
				memset(new_str + SIZE(*this), c, count);
				new_str[new_len] = '\0';

				free(large_.str_, large_.cap_ + 1, alignof(char));

				large_.str_ = new_str;
				large_.cap_ = new_cap;
			}
			else
			{
				memset(large_.str_ + SIZE(*this), c, count);
				large_.str_[new_len] = '\0';
			}

			len_ = new_len | is_large_flag;
		}
		else if (new_len >= small_size)
		{
			char* new_str = reinterpret_cast<char*>(alloc(new_len + 1, alignof(char)));

			memcpy(new_str, small_.str_, SIZE(*this));
			memset(new_str + SIZE(*this), c, count);
			new_str[new_len] = '\0';

			large_.str_ = new_str;
			large_.cap_ = new_len;

			len_ = new_len | is_large_flag;
		}
		else
		{
			memset(small_.str_ + SIZE(*this), c, count);
			small_.str_[new_len] = '\0';

			len_ = new_len;
		}
	}

	void string::append(char const* str, uint32_t count)
	{
		if (count == UINT32_MAX)
			count = strlen(str);

		count &= ~is_large_flag;

		uint32_t new_len = SIZE(*this) + count;
		if (IS_LARGE(*this))
		{
			if (new_len > large_.cap_)
			{
				uint32_t new_cap = large_.cap_;
				while (new_cap < new_len)
					new_cap *= 2;

				char* new_str =
					reinterpret_cast<char*>(alloc(new_cap + 1, alignof(char)));

				memcpy(new_str, large_.str_, SIZE(*this));
				memcpy(new_str + SIZE(*this), str, count);
				new_str[new_len] = '\0';

				free(large_.str_, large_.cap_ + 1, alignof(char));

				large_.str_ = new_str;
				large_.cap_ = new_cap;
			}
			else
			{
				memcpy(large_.str_ + SIZE(*this), str, count);
				large_.str_[new_len] = '\0';
			}

			len_ = new_len | is_large_flag;
		}
		else if (new_len >= small_size)
		{
			char* new_str = reinterpret_cast<char*>(alloc(new_len + 1, alignof(char)));

			memcpy(new_str, small_.str_, SIZE(*this));
			memcpy(new_str + SIZE(*this), str, count);
			new_str[new_len] = '\0';

			large_.str_ = new_str;
			large_.cap_ = new_len;

			len_ = new_len | is_large_flag;
		}
		else
		{
			memcpy(small_.str_ + SIZE(*this), str, count);
			small_.str_[new_len] = '\0';

			len_ = new_len;
		}
	}

	void string::append(string const& str, uint32_t pos, uint32_t count)
	{
		if (count == UINT32_MAX)
			count = SIZE(str) - pos;

		count &= ~is_large_flag;

		uint32_t new_len = SIZE(*this) + count;
		if (IS_LARGE(*this))
		{
			if (new_len > large_.cap_)
			{
				uint32_t new_cap = large_.cap_;
				while (new_cap < new_len)
					new_cap *= 2;

				char* new_str =
					reinterpret_cast<char*>(alloc(new_cap + 1, alignof(char)));

				memcpy(new_str, large_.str_, SIZE(*this));
				memcpy(new_str + SIZE(*this), DATA(str) + pos, count);
				new_str[new_len] = '\0';

				free(large_.str_, large_.cap_ + 1, alignof(char));

				large_.str_ = new_str;
				large_.cap_ = new_cap;
			}
			else
			{
				memcpy(large_.str_ + SIZE(*this), DATA(str) + pos, count);
				large_.str_[new_len] = '\0';
			}

			len_ = new_len | is_large_flag;
		}
		else if (new_len >= small_size)
		{
			char* new_str = reinterpret_cast<char*>(alloc(new_len + 1, alignof(char)));

			memcpy(new_str, small_.str_, SIZE(*this));
			memcpy(new_str + SIZE(*this), DATA(str) + pos, count);
			new_str[new_len] = '\0';

			large_.str_ = new_str;
			large_.cap_ = new_len;

			len_ = new_len | is_large_flag;
		}
		else
		{
			memcpy(small_.str_ + SIZE(*this), DATA(str) + pos, count);
			small_.str_[new_len] = '\0';

			len_ = new_len;
		}
	}

	void string::append(string_view const& str, uint32_t pos, uint32_t count)
	{
		if (count == UINT32_MAX)
			count = str.size() - pos;

		count &= ~is_large_flag;

		uint32_t new_len = SIZE(*this) + count;
		if (IS_LARGE(*this))
		{
			if (new_len > large_.cap_)
			{
				uint32_t new_cap = large_.cap_;
				while (new_cap < new_len)
					new_cap *= 2;

				char* new_str =
					reinterpret_cast<char*>(alloc(new_cap + 1, alignof(char)));

				memcpy(new_str, large_.str_, SIZE(*this));
				memcpy(new_str + SIZE(*this), str.data() + pos, count);
				new_str[new_len] = '\0';

				free(large_.str_, large_.cap_ + 1, alignof(char));

				large_.str_ = new_str;
				large_.cap_ = new_cap;
			}
			else
			{
				memcpy(large_.str_ + SIZE(*this), str.data() + pos, count);
				large_.str_[new_len] = '\0';
			}

			len_ = new_len | is_large_flag;
		}
		else if (new_len >= small_size)
		{
			char* new_str = reinterpret_cast<char*>(alloc(new_len + 1, alignof(char)));

			memcpy(new_str, small_.str_, SIZE(*this));
			memcpy(new_str + SIZE(*this), str.data() + pos, count);
			new_str[new_len] = '\0';

			large_.str_ = new_str;
			large_.cap_ = new_len;

			len_ = new_len | is_large_flag;
		}
		else
		{
			memcpy(small_.str_ + SIZE(*this), str.data() + pos, count);
			small_.str_[new_len] = '\0';

			len_ = new_len;
		}
	}

	void string::append(std::initializer_list<char> ilist)
	{
		uint32_t count = ilist.size() & ~is_large_flag;

		uint32_t new_len = SIZE(*this) + count;
		if (IS_LARGE(*this))
		{
			if (new_len > large_.cap_)
			{
				uint32_t new_cap = large_.cap_;
				while (new_cap < new_len)
					new_cap *= 2;

				char* new_str =
					reinterpret_cast<char*>(alloc(new_cap + 1, alignof(char)));

				memcpy(new_str, large_.str_, SIZE(*this));
				memcpy(new_str + SIZE(*this), ilist.begin(), count);
				new_str[new_len] = '\0';

				free(large_.str_, large_.cap_ + 1, alignof(char));

				large_.str_ = new_str;
				large_.cap_ = new_cap;
			}
			else
			{
				memcpy(large_.str_ + SIZE(*this), ilist.begin(), count);
				large_.str_[new_len] = '\0';
			}

			len_ = new_len | is_large_flag;
		}
		else if (new_len >= small_size)
		{
			char* new_str = reinterpret_cast<char*>(alloc(new_len + 1, alignof(char)));

			memcpy(new_str, small_.str_, SIZE(*this));
			memcpy(new_str + SIZE(*this), ilist.begin(), count);
			new_str[new_len] = '\0';

			large_.str_ = new_str;
			large_.cap_ = new_len;

			len_ = new_len | is_large_flag;
		}
		else
		{
			memcpy(small_.str_ + SIZE(*this), ilist.begin(), count);
			small_.str_[new_len] = '\0';

			len_ = new_len;
		}
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
		count = SIZE(*this) - idx < count ? SIZE(*this) - idx : count;

		if (count == 0)
		{
			insert(idx, count2, c);
		}
		else
		{
			count &= ~is_large_flag;
			count2 &= ~is_large_flag;

			uint32_t new_len = SIZE(*this) - count + count2;
			if (IS_LARGE(*this))
			{
				if (new_len > large_.cap_)
				{
					uint32_t new_cap = large_.cap_;
					while (new_cap < new_len)
						new_cap *= 2;

					char* new_str =
						reinterpret_cast<char*>(alloc(new_cap + 1, alignof(char)));

					memcpy(new_str, large_.str_, idx);
					memmove(new_str + idx + count2, large_.str_ + idx + count,
					        SIZE(*this) - idx - count + 1);
					memset(new_str + idx, c, count2);

					free(large_.str_, large_.cap_ + 1, alignof(char));

					large_.str_ = new_str;
					large_.cap_ = new_cap;
				}
				else
				{
					memmove(large_.str_ + idx + count2, large_.str_ + idx + count,
					        SIZE(*this) - idx - count + 1);
					memset(large_.str_ + idx, c, count2);
				}

				len_ = new_len | is_large_flag;
			}
			else if (new_len >= small_size)
			{
				char* new_str =
					reinterpret_cast<char*>(alloc(new_len + 1, alignof(char)));

				memcpy(new_str, small_.str_, idx);
				memmove(new_str + idx + count2, small_.str_ + idx + count,
				        SIZE(*this) - idx - count + 1);
				memset(new_str + idx, c, count2);

				large_.str_ = new_str;
				large_.cap_ = new_len;

				len_ = new_len | is_large_flag;
			}
			else
			{
				memmove(small_.str_ + idx + count2, small_.str_ + idx + count,
				        SIZE(*this) - idx - count + 1);
				memset(small_.str_ + idx, c, count2);

				len_ = new_len;
			}
		}
	}

	void string::replace(uint32_t idx, uint32_t count, char const* str, uint32_t count2)
	{
		count = SIZE(*this) - idx < count ? SIZE(*this) - idx : count;

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

			uint32_t new_len = SIZE(*this) - count + count2;
			if (IS_LARGE(*this))
			{
				if (new_len > large_.cap_)
				{
					uint32_t new_cap = large_.cap_;
					while (new_cap < new_len)
						new_cap *= 2;

					char* new_str =
						reinterpret_cast<char*>(alloc(new_cap + 1, alignof(char)));

					memcpy(new_str, large_.str_, idx);
					memmove(new_str + idx + count2, large_.str_ + idx + count,
					        SIZE(*this) - idx - count + 1);
					memcpy(new_str + idx, str, count2);

					free(large_.str_, large_.cap_ + 1, alignof(char));

					large_.str_ = new_str;
					large_.cap_ = new_cap;
				}
				else
				{
					memmove(large_.str_ + idx + count2, large_.str_ + idx + count,
					        SIZE(*this) - idx - count + 1);
					memcpy(large_.str_ + idx, str, count2);
				}

				len_ = new_len | is_large_flag;
			}
			else if (new_len >= small_size)
			{
				char* new_str =
					reinterpret_cast<char*>(alloc(new_len + 1, alignof(char)));

				memcpy(new_str, small_.str_, idx);
				memmove(new_str + idx + count2, small_.str_ + idx + count,
				        SIZE(*this) - idx - count + 1);
				memcpy(new_str + idx, str, count2);

				large_.str_ = new_str;
				large_.cap_ = new_len;

				len_ = new_len | is_large_flag;
			}
			else
			{
				memmove(small_.str_ + idx + count2, small_.str_ + idx + count,
				        SIZE(*this) - idx - count + 1);
				memcpy(small_.str_ + idx, str, count2);

				len_ = new_len;
			}
		}
	}

	void string::replace(uint32_t idx, uint32_t count, string const& str, uint32_t pos,
	                     uint32_t count2)
	{
		count = SIZE(*this) - idx < count ? SIZE(*this) - idx : count;

		if (count == 0)
		{
			insert(idx, str, pos, count2);
		}
		else
		{
			if (count2 == UINT32_MAX)
				count2 = SIZE(str) - pos;

			count &= ~is_large_flag;
			count2 &= ~is_large_flag;

			uint32_t new_len = SIZE(*this) - count + count2;
			if (IS_LARGE(*this))
			{
				if (new_len > large_.cap_)
				{
					uint32_t new_cap = large_.cap_;
					while (new_cap < new_len)
						new_cap *= 2;

					char* new_str =
						reinterpret_cast<char*>(alloc(new_cap + 1, alignof(char)));

					memcpy(new_str, large_.str_, idx);
					memmove(new_str + idx + count2, large_.str_ + idx + count,
					        SIZE(*this) - idx - count + 1);
					memcpy(new_str + idx, DATA(str) + pos, count2);

					free(large_.str_, large_.cap_ + 1, alignof(char));

					large_.str_ = new_str;
					large_.cap_ = new_cap;
				}
				else
				{
					memmove(large_.str_ + idx + count2, large_.str_ + idx + count,
					        SIZE(*this) - idx - count + 1);
					memcpy(large_.str_ + idx, DATA(str) + pos, count2);
				}

				len_ = new_len | is_large_flag;
			}
			else if (new_len >= small_size)
			{
				char* new_str =
					reinterpret_cast<char*>(alloc(new_len + 1, alignof(char)));

				memcpy(new_str, small_.str_, idx);
				memmove(new_str + idx + count2, small_.str_ + idx + count,
				        SIZE(*this) - idx - count + 1);
				memcpy(new_str + idx, DATA(str) + pos, count2);

				large_.str_ = new_str;
				large_.cap_ = new_len;

				len_ = new_len | is_large_flag;
			}
			else
			{
				memmove(small_.str_ + idx + count2, small_.str_ + idx + count,
				        SIZE(*this) - idx - count + 1);
				memcpy(small_.str_ + idx, DATA(str) + pos, count2);

				len_ = new_len;
			}
		}
	}

	void string::replace(uint32_t idx, uint32_t count, string_view const& str,
	                     uint32_t pos, uint32_t count2)
	{
		count = SIZE(*this) - idx < count ? SIZE(*this) - idx : count;

		if (count == 0)
		{
			insert(idx, str, pos, count2);
		}
		else
		{
			if (count2 == UINT32_MAX)
				count2 = str.size() - pos;

			count &= ~is_large_flag;
			count2 &= ~is_large_flag;

			uint32_t new_len = SIZE(*this) - count + count2;
			if (IS_LARGE(*this))
			{
				if (new_len > large_.cap_)
				{
					uint32_t new_cap = large_.cap_;
					while (new_cap < new_len)
						new_cap *= 2;

					char* new_str =
						reinterpret_cast<char*>(alloc(new_cap + 1, alignof(char)));

					memcpy(new_str, large_.str_, idx);
					memmove(new_str + idx + count2, large_.str_ + idx + count,
					        SIZE(*this) - idx - count + 1);
					memcpy(new_str + idx, str.data() + pos, count2);

					free(large_.str_, large_.cap_ + 1, alignof(char));

					large_.str_ = new_str;
					large_.cap_ = new_cap;
				}
				else
				{
					memmove(large_.str_ + idx + count2, large_.str_ + idx + count,
					        SIZE(*this) - idx - count + 1);
					memcpy(large_.str_ + idx, str.data() + pos, count2);
				}

				len_ = new_len | is_large_flag;
			}
			else if (new_len >= small_size)
			{
				char* new_str =
					reinterpret_cast<char*>(alloc(new_len + 1, alignof(char)));

				memcpy(new_str, small_.str_, idx);
				memmove(new_str + idx + count2, small_.str_ + idx + count,
				        SIZE(*this) - idx - count + 1);
				memcpy(new_str + idx, str.data() + pos, count2);

				large_.str_ = new_str;
				large_.cap_ = new_len;

				len_ = new_len | is_large_flag;
			}
			else
			{
				memmove(small_.str_ + idx + count2, small_.str_ + idx + count,
				        SIZE(*this) - idx - count + 1);
				memcpy(small_.str_ + idx, str.data() + pos, count2);

				len_ = new_len;
			}
		}
	}

	void string::replace(uint32_t idx, uint32_t count, std::initializer_list<char> ilist)
	{
		count = SIZE(*this) - idx < count ? SIZE(*this) - idx : count;

		if (count == 0)
		{
			insert(idx, ilist);
		}
		else
		{
			count &= ~is_large_flag;
			uint32_t count2 = ilist.size() & ~is_large_flag;

			uint32_t new_len = SIZE(*this) - count + count2;
			if (IS_LARGE(*this))
			{
				if (new_len > large_.cap_)
				{
					uint32_t new_cap = large_.cap_;
					while (new_cap < new_len)
						new_cap *= 2;

					char* new_str =
						reinterpret_cast<char*>(alloc(new_cap + 1, alignof(char)));

					memcpy(new_str, large_.str_, idx);
					memmove(new_str + idx + count2, large_.str_ + idx + count,
					        SIZE(*this) - idx - count + 1);
					memcpy(new_str + idx, ilist.begin(), count2);

					free(large_.str_, large_.cap_ + 1, alignof(char));

					large_.str_ = new_str;
					large_.cap_ = new_cap;
				}
				else
				{
					memmove(large_.str_ + idx + count2, large_.str_ + idx + count,
					        SIZE(*this) - idx - count + 1);
					memcpy(large_.str_ + idx, ilist.begin(), count2);
				}

				len_ = new_len | is_large_flag;
			}
			else if (new_len >= small_size)
			{
				char* new_str =
					reinterpret_cast<char*>(alloc(new_len + 1, alignof(char)));

				memcpy(new_str, small_.str_, idx);
				memmove(new_str + idx + count2, small_.str_ + idx + count,
				        SIZE(*this) - idx - count + 1);
				memcpy(new_str + idx, ilist.begin(), count2);

				large_.str_ = new_str;
				large_.cap_ = new_len;

				len_ = new_len | is_large_flag;
			}
			else
			{
				memmove(small_.str_ + idx + count2, small_.str_ + idx + count,
				        SIZE(*this) - idx - count + 1);
				memcpy(small_.str_ + idx, ilist.begin(), count2);

				len_ = new_len;
			}
		}
	}

	void string::erase(uint32_t idx, uint32_t count)
	{
		count = SIZE(*this) - idx < count ? SIZE(*this) - idx : count;

		if (count)
		{
			memmove(DATA(*this) + idx, DATA(*this) + idx + count,
			        SIZE(*this) - idx - count + 1);

			len_ = SIZE(*this) - count | (len_ & is_large_flag);
		}
	}

	void string::push_back(char c)
	{
		uint32_t new_len = SIZE(*this) + 1;
		if (IS_LARGE(*this))
		{
			if (new_len > large_.cap_)
			{
				uint32_t new_cap = large_.cap_;
				while (new_cap < new_len)
					new_cap *= 2;

				char* new_str =
					reinterpret_cast<char*>(alloc(new_cap + 1, alignof(char)));

				memcpy(new_str, large_.str_, SIZE(*this));
				new_str[SIZE(*this)] = c;
				new_str[new_len] = '\0';

				free(large_.str_, large_.cap_ + 1, alignof(char));

				large_.str_ = new_str;
				large_.cap_ = new_cap;
			}
			else
			{
				memset(large_.str_ + SIZE(*this), c, 1);
				large_.str_[new_len] = '\0';
			}

			len_ = new_len | is_large_flag;
		}
		else if (new_len >= small_size)
		{
			char* new_str = reinterpret_cast<char*>(alloc(new_len + 1, alignof(char)));

			memcpy(new_str, small_.str_, SIZE(*this));
			new_str[SIZE(*this)] = c;
			new_str[new_len] = '\0';

			large_.str_ = new_str;
			large_.cap_ = new_len;

			len_ = new_len | is_large_flag;
		}
		else
		{
			small_.str_[SIZE(*this)] = c;
			small_.str_[new_len] = '\0';

			len_ = new_len;
		}
	}

	void string::pop_back()
	{
		DATA(*this)[SIZE(*this) - 1] = '\0';
		len_ = SIZE(*this) - 1 | (len_ & is_large_flag);
	}

	string string::substr(uint32_t pos, uint32_t size) const&
	{
		return {*this, pos, size};
	}

	string string::substr(uint32_t pos, uint32_t size) &&
	{
		return {static_cast<string&&>(*this), pos, size};
	}

	int32_t string::compare(string const& str) const
	{
		uint32_t len = SIZE(*this) < SIZE(str) ? SIZE(*this) : SIZE(str);
		int32_t  res = strncmp(DATA(*this), DATA(str), len);

		if (res == 0)
		{
			if (SIZE(*this) < SIZE(str))
				return -1;
			else if (SIZE(*this) > SIZE(str))
				return 1;
		}

		return res;
	}

	int32_t string::compare(string_view str) const
	{
		uint32_t len = SIZE(*this) < str.size() ? SIZE(*this) : str.size();
		int32_t  res = strncmp(DATA(*this), str.data(), len);

		if (res == 0)
		{
			if (SIZE(*this) < str.size())
				return -1;
			else if (SIZE(*this) > str.size())
				return 1;
		}

		return res;
	}

	int32_t string::compare(char const* str) const
	{
		uint32_t str_len = static_cast<uint32_t>(strlen(str));
		uint32_t len = SIZE(*this) < str_len ? SIZE(*this) : str_len;
		int32_t  res = strncmp(DATA(*this), str, len);

		if (res == 0)
		{
			if (SIZE(*this) < str_len)
				return -1;
			else if (SIZE(*this) > str_len)
				return 1;
		}

		return res;
	}

	bool string::starts_with(string const& str) const
	{
		return strncmp(DATA(*this), DATA(str), SIZE(str)) == 0;
	}

	bool string::starts_with(string_view str) const
	{
		return strncmp(DATA(*this), str.data(), str.size()) == 0;
	}

	bool string::starts_with(char const* str) const
	{
		return strncmp(DATA(*this), str, strlen(str)) == 0;
	}

	bool string::starts_with(char c) const
	{
		return DATA(*this)[0] == c;
	}

	bool string::ends_with(string const& str) const
	{
		return strncmp(DATA(*this) + SIZE(*this) - SIZE(str), DATA(str), SIZE(str)) == 0;
	}

	bool string::ends_with(string_view str) const
	{
		return strncmp(DATA(*this) + SIZE(*this) - str.size(), str.data(), str.size()) ==
		       0;
	}

	bool string::ends_with(char const* str) const
	{
		uint32_t len = strlen(str);
		return strncmp(DATA(*this) + SIZE(*this) - len, str, len) == 0;
	}

	bool string::ends_with(char c) const
	{
		return DATA(*this)[SIZE(*this) - 1] == c;
	}

	bool string::contains(string const& str) const
	{
		if (SIZE(str) > SIZE(*this) || SIZE(str) == 0)
			return false;

		for (uint32_t i {0}, j {0}; i < len_; ++i)
		{
			if (len_ - i < SIZE(str) - j)
				return false;

			if (DATA(*this)[i] == DATA(str)[j])
			{
				++j;
				if (j == SIZE(str))
					return true;
			}
			else
				j = 0;
		}
		return false;
	}

	bool string::contains(string_view str) const
	{
		if (str.size() > SIZE(*this) || str.size() == 0)
			return false;

		for (uint32_t i {0}, j {0}; i < SIZE(*this); ++i)
		{
			if (SIZE(*this) - i < str.size() - j)
				return false;

			if (DATA(*this)[i] == str.data()[j])
			{
				++j;
				if (j == str.size())
					return true;
			}
			else
				j = 0;
		}
		return false;
	}

	bool string::contains(char const* str) const
	{
		uint32_t len = strlen(str);
		if (len > SIZE(*this) || len == 0)
			return false;

		for (uint32_t i {0}, j {0}; i < SIZE(*this); ++i)
		{
			if (SIZE(*this) - i < len - j)
				return false;

			if (DATA(*this)[i] == str[j])
			{
				++j;
				if (j == len)
					return true;
			}
			else
				j = 0;
		}
		return false;
	}

	bool string::contains(char c) const
	{
		for (uint32_t i {0}; i < SIZE(*this); ++i)
			if (DATA(*this)[i] == c)
				return true;
		return false;
	}

	uint32_t string::find(string const& str, uint32_t pos) const
	{
		char const* data {DATA(*this)};
		uint32_t    size {SIZE(*this)};

		char const* data2 {DATA(str)};
		uint32_t    len {SIZE(str)};

		if (len > size - pos || len == 0)
			return UINT32_MAX;

		uint32_t max_len {size - len + 1};
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

				if (k == pos + len)
					return pos;

				if (k == size)
					return UINT32_MAX;
			}

			++pos;
		}

		return UINT32_MAX;
	}

	uint32_t string::find(string_view str, uint32_t pos) const
	{
		char const* data {DATA(*this)};
		uint32_t    size {SIZE(*this)};

		char const* data2 {str.data()};
		uint32_t    len {str.size()};

		if (len > size - pos || len == 0)
			return UINT32_MAX;

		uint32_t max_len {size - len + 1};
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

				if (k == pos + len)
					return pos;

				if (k == size)
					return UINT32_MAX;
			}

			++pos;
		}

		return UINT32_MAX;
	}

	uint32_t string::find(char const* str, uint32_t pos) const
	{
		char const* data {DATA(*this)};
		uint32_t    size {SIZE(*this)};

		char const* data2 {str};
		uint32_t    len {static_cast<uint32_t>(strlen(str))};

		if (len > size - pos || len == 0)
			return UINT32_MAX;

		uint32_t max_len {size - len + 1};
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

				if (k == pos + len)
					return pos;

				if (k == size)
					return UINT32_MAX;
			}

			++pos;
		}

		return UINT32_MAX;
	}

	uint32_t string::find(char c, uint32_t pos) const
	{
		for (uint32_t i {pos}; i < SIZE(*this); ++i)
			if (DATA(*this)[i] == c)
				return i;
		return UINT32_MAX;
	}

	uint32_t string::rfind(string const& str, uint32_t pos) const
	{
		char const* data {DATA(*this)};
		uint32_t    size {SIZE(*this)};

		char const* data2 {DATA(str)};
		uint32_t    len {SIZE(str)};

		if (pos == UINT32_MAX)
			pos = size;

		if (len + 1 > pos || len == 0)
			return UINT32_MAX;

		char first_char {data2[len - 1]};

		while (pos > 0)
		{
			while ((pos > 0) && (data[pos - 1] != first_char))
				--pos;

			if (pos == 0)
				return UINT32_MAX;

			uint32_t j {len - 1}, k {pos - 1};
			while (data[k - 1] == data2[j - 1])
			{
				--j;
				--k;

				if (k == pos - len)
					return k;

				if (k == 0)
					return UINT32_MAX;
			}

			--pos;
		}
		return UINT32_MAX;
	}

	uint32_t string::rfind(string_view str, uint32_t pos) const
	{
		char const* data {DATA(*this)};
		uint32_t    size {SIZE(*this)};

		char const* data2 {str.data()};
		uint32_t    len {str.size()};

		if (pos == UINT32_MAX)
			pos = size;

		if (len + 1 > pos || len == 0)
			return UINT32_MAX;

		char first_char {data2[len - 1]};

		while (pos > 0)
		{
			while ((pos > 0) && (data[pos - 1] != first_char))
				--pos;

			if (pos == 0)
				return UINT32_MAX;

			uint32_t j {len - 1}, k {pos - 1};
			while (data[k - 1] == data2[j - 1])
			{
				--j;
				--k;

				if (k == pos - len)
					return k;

				if (k == 0)
					return UINT32_MAX;
			}

			--pos;
		}
		return UINT32_MAX;
	}

	uint32_t string::rfind(char const* str, uint32_t pos) const
	{
		char const* data {DATA(*this)};
		uint32_t    size {SIZE(*this)};

		char const* data2 {str};
		uint32_t    len {static_cast<uint32_t>(strlen(str))};

		if (pos == UINT32_MAX)
			pos = size;

		if (len + 1 > pos || len == 0)
			return UINT32_MAX;

		char first_char {data2[len - 1]};

		while (pos > 0)
		{
			while ((pos > 0) && (data[pos - 1] != first_char))
				--pos;

			if (pos == 0)
				return UINT32_MAX;

			uint32_t j {len - 1}, k {pos - 1};
			while (data[k - 1] == data2[j - 1])
			{
				--j;
				--k;

				if (k == pos - len)
					return k;

				if (k == 0)
					return UINT32_MAX;
			}

			--pos;
		}
		return UINT32_MAX;
	}

	uint32_t string::rfind(char c, uint32_t pos) const
	{
		if (pos == UINT32_MAX)
			pos = SIZE(*this) - 1;

		for (uint32_t i {pos + 1}; i > 0; --i)
			if (DATA(*this)[i - 1] == c)
				return i - 1;
		return UINT32_MAX;
	}

	bool string::operator==(string const& str) const
	{
		if (SIZE(*this) != SIZE(str))
			return false;
		return strncmp(DATA(*this), DATA(str), SIZE(*this)) == 0;
	}

	bool string::operator==(string_view str) const
	{
		if (SIZE(*this) != str.size())
			return false;
		return strncmp(DATA(*this), str.data(), SIZE(*this)) == 0;
	}

	bool string::operator==(char const* str) const
	{
		if (SIZE(*this) != strlen(str))
			return false;
		return strncmp(DATA(*this), str, SIZE(*this)) == 0;
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
} // namespace mc
