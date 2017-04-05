#pragma once

#include <iostream>
#include <streambuf>

using std::basic_streambuf;
using std::basic_ostream;
using std::char_traits;

template<class C = char> struct debug_writer {
	void put_s(const C* pc) { OutputDebugStringA(pc); }
	void put_c(C c) { C buf[2] = {c,0}; OutputDebugStringA(buf); }
};

template<> struct debug_writer<wchar_t> {
	void put_s(const wchar_t * pc) { OutputDebugStringW(pc); }
	void put_c(wchar_t c) { wchar_t buf[2] = { c,0 }; OutputDebugStringW(buf); }
};

template<class Elem, class Traits> struct basic_debugbuf : public basic_streambuf<Elem, Traits>, debug_writer<Elem>
{
	typedef typename Traits::int_type int_type;
	virtual int_type overflow(int_type elem = Traits::eof())
	{
		return put_c((Elem)elem), elem;
	}
	virtual std::streamsize xsputn(const Elem *pstr, std::streamsize count)
	{
		if (pstr[count] == 0)	return put_s(pstr), count;
		return basic_streambuf::xsputn(pstr, count);
	}
};

template<class Elem, class Traits> class basic_debugstream : public basic_ostream<Elem, Traits>
{
public:
	basic_debugstream() : basic_ostream<Elem, Traits>(&_debug_buffer)	{ }
private:
	basic_debugbuf<Elem, Traits> _debug_buffer;
};

typedef basic_debugstream<char, char_traits<char>> debugstream;
typedef basic_debugstream<wchar_t, char_traits<wchar_t>> wdebugstream;
