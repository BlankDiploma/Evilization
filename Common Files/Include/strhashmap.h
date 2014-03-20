#pragma once

#include "stdafx.h"
#include <hash_map>

struct stringHasher : public stdext::hash_compare <const wchar_t*>
{
	size_t operator() (const wchar_t* s) const
	{
		size_t h = 0;
		unsigned int size = wcslen(s);
		for(unsigned int i = 0; i < size; i++)
		{
			if (s[i] >= 'A' && s[i] <= 'Z')
				h = 31 * h + s[i] - 'A' + 'a';
			else
				h = 31 * h + s[i];
		}
		return h;
	}
	bool operator()(const wchar_t* s1, const wchar_t* s2) const
	{
		return _wcsicmp(s1, s2) < 0;
	}
};

struct pointerHasher : public stdext::hash_compare <const void*>
{
	size_t operator() (const void* s) const
	{
		return (size_t)s >> 2;
	}
	bool operator()(const void* s1, const void* s2) const
	{
		return s1 < s2;
	}
};

struct intHasher : public stdext::hash_compare <const int>
{
	size_t operator() (const int i) const
	{
		return (size_t)i >> 2;
	}
	bool operator()(int i1, int i2) const
	{
		return i1 < i2;
	}
};

typedef stdext::hash_map<int, const wchar_t*, intHasher> IntStringHash;

typedef stdext::hash_map<const wchar_t*, int, stringHasher> StringIntHash;
typedef stdext::hash_map<const wchar_t*, void*, stringHasher> StringPointerHash;
typedef stdext::hash_map<const void*, void*, pointerHasher> PointerPointerHash;