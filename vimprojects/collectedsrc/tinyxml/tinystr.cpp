/*
www.sourceforge.net/projects/tinyxml
Original file by Yves Berquin.

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must
not claim that you wrote the original software. If you use this
software in a product, an acknowledgment in the product documentation
would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source
distribution.
*/

/*
 * THIS FILE WAS ALTERED BY Tyge L鴙set, 7. April 2005.
 */


#ifndef TIXML_USE_STL

#include "tinystr.h"

// Error value for find primitive
const TiXmlString::size_type TiXmlString::npos = static_cast< TiXmlString::size_type >(-1);


// Null rep.
TiXmlString::Rep TiXmlString::nullrep_ = { 0, 0, { '\0' } };


/**
 * 相当与扩充容量到cap。
 * 同样使用临时变量tmp来达到释放以前的内存的目的。
 */
void TiXmlString::reserve (size_type cap)
{
	if (cap > capacity())
	{
		TiXmlString tmp;
		tmp.init(length(), cap);
		memcpy(tmp.start(), data(), length());
		swap(tmp);
	}
}


TiXmlString& TiXmlString::assign(const char* str, size_type len)
{
	size_type cap = capacity();
	/**
	 * 如果str的长度大于当前的容量，那么直接生成一个临时的TiXmlString对象tmp，并赋值str，
	 * 然后将tmp的数据和本对象的交换。
	 * 这样在此函数中返回时，以前的数据空间将在销毁tmp时同时销毁，可防止内存泄漏。
	 *
	 * 第二个判断条件是当当前容量过大时也这么做，有助于节省空间。
	 *
	 * 这里面tmp对象很有意思，可以帮助销毁以前的空间，防止内存泄漏！
	 */
	if (len > cap || cap > 3*(len + 8))
	{
		TiXmlString tmp;
		tmp.init(len);
		memcpy(tmp.start(), str, len);
		swap(tmp);
	}
	else
	{
		memmove(start(), str, len);
		set_size(len);
	}
	return *this;
}


TiXmlString& TiXmlString::append(const char* str, size_type len)
{
	size_type newsize = length() + len;

	/**
	 * 扩充容量
	 */
	if (newsize > capacity())
	{
		reserve (newsize + capacity());
	}

	memmove(finish(), str, len);
	set_size(newsize);
	return *this;
}


TiXmlString operator + (const TiXmlString & a, const TiXmlString & b)
{
	TiXmlString tmp;
	tmp.reserve(a.length() + b.length());
	tmp += a;
	tmp += b;
	return tmp;
}

TiXmlString operator + (const TiXmlString & a, const char* b)
{
	TiXmlString tmp;
	TiXmlString::size_type b_len = static_cast<TiXmlString::size_type>( strlen(b) );
	tmp.reserve(a.length() + b_len);
	tmp += a;
	tmp.append(b, b_len);
	return tmp;
}

TiXmlString operator + (const char* a, const TiXmlString & b)
{
	TiXmlString tmp;
	TiXmlString::size_type a_len = static_cast<TiXmlString::size_type>( strlen(a) );
	tmp.reserve(a_len + b.length());
	tmp.append(a, a_len);
	tmp += b;
	return tmp;
}


#endif	// TIXML_USE_STL
