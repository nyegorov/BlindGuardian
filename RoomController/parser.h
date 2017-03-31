// NScript - lightweight, single-pass parser for executing c-style value_tessions. 
// Supports integer, double, wstring and date types. Has limited set of flow 
// control statements (if/then/else, loops). Fully extensible by user-defined 
// objects with IObject interface.

#pragma once

#include "sensor.h"
#include <string>
#include <vector>
#include <map>
#include <set>
#include <functional>
#include <cctype>

using std::wstring;

namespace roomctrl	{
// Container for storing named objects and variables

inline bool icomp_pred(wchar_t a, wchar_t b) { return std::tolower(a) < std::tolower(b); }
struct lessi {
	bool operator () (const wstring& s1, const wstring &s2) const {
		return /*s1.length() == s2.length() && */std::lexicographical_compare(s1.begin(), s1.end(), s2.begin(), s2.end(), icomp_pred);
	}
};

class Context
{
public:
	Context(const Context *base);
	void Push()		{_locals.push_back(vars_t());}
	void Pop()		{_locals.pop_back();}
	value_t& Get(const wstring& name, bool local = false);
	bool Get(const wstring& name, value_t& result) const;
	void Set(const wstring& name, const value_t& value)		{_locals.front()[name] = value;}
private:
	struct vars_t : public std::map<wstring, value_t, lessi> {};
	static vars_t		_globals;
	std::vector<vars_t>	_locals;
};

// Parser of input stream to a list of tokens
class Parser	{
public:
	typedef std::vector<wstring>	ArgList;
	typedef size_t	State;
	enum Token	{end,mod,assign,ge,gt,le,lt,nequ,name,value,land,lor,lnot,stmt,err,dot,newobj,minus,lpar,rpar,lcurly,rcurly,equ,plus,lsquare,rsquare,multiply,divide,idiv,and,or,not,pwr,comma,unaryplus,unaryminus,forloop,ifop,iffunc,ifelse,func,object,plusset, minusset, mulset, divset, idivset, setvar, my};

	Parser();
	void Init(const wstring& value_t){_content = value_t;SetState(0);}
	Token GetToken()			{return _token;}
	const value_t& GetValue()	{return _value;}
	const wstring& GetName()	{return _name;}
	const ArgList& GetArgs()	{return _args;}
	State GetState()			{return _lastpos;}
	void SetState(State state)	{_pos = state;_token=end;Next();}
	wstring GetContent(State begin, State end)	{return _content.substr(begin, end-begin);}
	void CheckPairedToken(Token token);
	Token Next();
private:
	static char	_decpt;

	typedef std::map<wstring, Token, lessi> Keywords;
	static Keywords	_keywords;
	Token			_token;
	wstring			_content;
	State			_pos;
	State			_lastpos;
	value_t			_value;
	wstring			_name;
	ArgList			_args;
	wstring			_temp;

	char Peek()			{return _pos >= _content.length() ? 0 : _content[_pos];}
	char Read()			{char c = Peek();_pos++;return c;}
	void Back()				{_pos--;}
	void ReadNumber(char c);
	void ReadTime(char c);
	void ReadArgList();
	void ReadName(char c);
};

// Main class for executing scripts
class NScript
{
public:
	NScript(const Context *pcontext = NULL) : _context(pcontext)	{}
	~NScript(void)						{};
	value_t eval(wstring script);
	using callfun_t = std::function<value_t(const params_t&)>;
	using getvar_t  = std::function<value_t(void)>;
	void set(wstring name, value_t value)	{ _context.Set(name, value); }
	void set(wstring name, callfun_t func)	{ _callbacks.emplace(name, func); }
	void set(wstring name, getvar_t func)	{ _callbacks.emplace(name, [func](const params_t&) {return func(); }); }

protected:
	enum Precedence	{Script, Statement, Conditional, Logic, Equality, Relation, Addition,Multiplication,Unary,Primary,Term};
	void Parse(Precedence level, value_t& result, bool skip);
	void ParseVar(value_t& result, bool skip);
	void ParseIf(value_t& result, bool skip);

	Parser				_parser;
	Context				_context;
	
	typedef void OpFunc(value_t& op1, value_t& op2, value_t& result);
	struct OpInfo { Parser::Token token; OpFunc* op; };
	static OpInfo _operators[Term][10];

	std::map<wstring, callfun_t, lessi>	_callbacks;
};

}
