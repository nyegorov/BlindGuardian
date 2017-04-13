#include "pch.h"
#include <locale>
#include <limits>
#include <algorithm>
#include <functional>
#include <assert.h>
#include "parser.h"

#pragma warning(disable:4503)

namespace roomctrl	{

wchar_t* get_error_msg(const value_t& v) {
	auto it = std::get_if<error_t>(&v);
	if(!it)	return L"ok";
	switch(*it) {
	case error_t::invalid_args:		return L"invalid arguments";
	case error_t::not_implemented:	return L"not implemented";
	case error_t::syntax_error:			return L"syntax error";
	case error_t::name_not_found:	return L"name not found";
	case error_t::type_mismatch:	return L"type mismatch";
	default:						return L"runtime error";
	}
}

// Operators

void OpNull(value_t& op1, value_t& op2, value_t& result) {}
void OpAdd(value_t& op1, value_t& op2, value_t& result) { result = std::get<value_type>(*op1) + std::get<value_type>(*op2); }
void OpNeg(value_t& op1, value_t& op2, value_t& result) { result = -std::get<value_type>(*op2); }
void OpSub(value_t& op1, value_t& op2, value_t& result) { result = std::get<value_type>(*op1) - std::get<value_type>(*op2); }
void OpMul(value_t& op1, value_t& op2, value_t& result) { result = std::get<value_type>(*op1) * std::get<value_type>(*op2); }
void OpDiv(value_t& op1, value_t& op2, value_t& result) { result = std::get<value_type>(*op1) / std::get<value_type>(*op2); }
void OpNot(value_t& op1, value_t& op2, value_t& result) { result = !std::get<value_type>(*op2); }
void OpOr(value_t& op1, value_t& op2, value_t& result)  { result = std::get<value_type>(*op1) != 0 || std::get<value_type>(*op2) != 0; }
void OpAnd(value_t& op1, value_t& op2, value_t& result) { result = std::get<value_type>(*op1) != 0 && std::get<value_type>(*op2) != 0; }
void OpEqu(value_t& op1, value_t& op2, value_t& result) { result = *op1 == *op2; }
void OpNeq(value_t& op1, value_t& op2, value_t& result) { result = *op1 != *op2; }
void OpLT(value_t& op1, value_t& op2, value_t& result)  { result = *op1  < *op2; }
void OpLE(value_t& op1, value_t& op2, value_t& result)  { result = *op1 <= *op2; }
void OpGT(value_t& op1, value_t& op2, value_t& result)  { result = *op1  > *op2; }
void OpGE(value_t& op1, value_t& op2, value_t& result)  { result = *op1 >= *op2; }

Context::vars_t	Context::_globals;
Context::Context(const Context *base) : _locals(1)
{
	if(base)	
		_locals.assign(base->_locals.begin(), base->_locals.end());
		//_locals[0] = base->_locals[0];
	if(_globals.empty())	{
		// Constants
	}
}

value_t& Context::Get(const wstring& name, bool local)
{
	if(!local)	{
		for(int i = (int)_locals.size() - 1; i >= -1; i--)	{
			vars_t& plane = i<0?_globals:_locals[i];
			vars_t::iterator p = plane.find(name);
			if(p != plane.end()) return	p->second;
		}
	}
	//return _locals.back()[name] = value_t{};
	throw error_t::name_not_found;
}

bool Context::Get(const wstring& name, value_t& result) const
{
	for(std::vector<vars_t>::const_reverse_iterator ri = _locals.rbegin(); ri != _locals.rend(); ri++)	{
		vars_t::const_iterator p = ri->find(name);
		if(p != ri->end())	return result = p->second, true;
	}
	return false;
}

// Operator precedence
NScript::OpInfo NScript::_operators[Term][10] = {
	{{Parser::stmt,		&OpNull },	{Parser::end, NULL}},
	{{Parser::comma,	&OpNull},	{Parser::end, NULL}},
	{{Parser::ifop,		&OpNull },	{Parser::end, NULL}},
	{{Parser::land,		&OpAnd },	{Parser::lor, &OpOr},		{Parser::end, NULL}},
	{{Parser::equ,		&OpEqu },	{Parser::nequ,	&OpNeq },	{Parser::end, NULL } },
	{{Parser::gt,		&OpGT },	{Parser::ge,	&OpGE },	{Parser::lt, &OpLT },	{Parser::le, &OpLE},	{ Parser::end, NULL } },
	{{Parser::plus,		&OpAdd},	{Parser::minus,	&OpSub},	{Parser::end, NULL}},
	{{Parser::multiply,	&OpMul},	{Parser::divide,&OpDiv},	{Parser::end, NULL}},
	{{Parser::minus,	&OpNeg},	{Parser::lnot, &OpNot},		{Parser::end, NULL}},
};

value_t NScript::eval(wstring script)
{
	value_t result;
	try	{
		_parser.Init(script);
		Parse(Script, result, false);
		if(_parser.GetToken() != Parser::end)	throw error_t::syntax_error;
	} catch(error_t e) {
		result = e;
	} catch(...) {
		result = error_t::runtime;
	}
	return *result;
}

// Parse "if <cond> <true-part> [else <part>]" statement
void NScript::ParseIf(value_t& result, bool skip) {
	bool cond = skip || result != value_t{0};
	Parse(Statement, result, !cond || skip);
	if(_parser.GetToken() == Parser::ifelse) {
		_parser.Next();
		Parse(Statement, result, cond || skip);
	}
}

// Parse "var[:=]" statement
void NScript::ParseVar(value_t& result, bool skip)
{
	wstring name = _parser.GetName();
	auto pcb = _callbacks.find(name);
	_parser.Next();
	if(_parser.GetToken() == Parser::setvar) {
		_parser.Next();
		Parse(Statement, result, skip);
		if(!skip)	_context.Set(name, result);
	}	else if(_parser.GetToken() == Parser::lpar) {
		params_t params;
		do {
			_parser.Next();
			if(_parser.GetToken() == Parser::rpar)	break;
			Parse(Statement, result, skip);
			params.push_back(result);
		} while(_parser.GetToken() == Parser::comma);
		_parser.CheckPairedToken(Parser::lpar);
		if(!skip) {
			if(pcb == _callbacks.end())	throw error_t::name_not_found;
			result = pcb->second(params);
		}
	}	else {
		if(!skip)	result = pcb != _callbacks.end() ? pcb->second(params_t{}) : _context.Get(name);
	}
}

void NScript::Parse(Precedence level, value_t& result, bool skip)
{
	// main parse loop
	Parser::Token token = _parser.GetToken();
	if(level == Primary)	{
		// primary value_tessions
		switch(token)	{
			case Parser::value:		if(!skip)	result = _parser.GetValue();_parser.Next();break;
			case Parser::name:		ParseVar(result, skip);	break;
			case Parser::iffunc:	_parser.Next(); Parse(Statement, result, skip); ParseIf(result, skip); break;
			case Parser::lpar:
				_parser.Next();
				result = value_t{};
				Parse(Statement, result, skip);
				_parser.CheckPairedToken(token);
				break;
			case Parser::lcurly:
				_parser.Next();
				_context.Push();
				Parse(Script, result, skip);
				_context.Pop();
				_parser.CheckPairedToken(token);
				break;
			case Parser::end:		throw error_t::syntax_error;
			default:				break;
		}
	} else if(level == Statement) {
		// comma operator, non-associative
		Parse((Precedence)((int)level + 1), result, skip);
	}	else	{
		// all other
		bool noop = true, is_unary = (level == Unary || level == Unary);

		// parse left-hand operand (for binary operators)
		if(!is_unary)	Parse((Precedence)((int)level+1), result, skip);
again:
		if(_parser.GetToken() == Parser::end)	return;
		for(OpInfo *pinfo = _operators[level];pinfo->op;pinfo++)	{
			token = pinfo->token;
			if(token == _parser.GetToken())	{
				if(token != Parser::lpar)	_parser.Next();
				if(token == Parser::ifop) {		// ternary "a?b:c" operator
					ParseIf(result, skip);
					goto again;
				}

				value_t left(result), right;
				result = value_t{};

				// parse right-hand operand
				if(is_unary)	Parse(level, right, skip);											// right-associative operators
				else if(token != Parser::unaryplus && token != Parser::unaryminus && !(level == Script && _parser.GetToken() == Parser::rcurly))
					Parse((Precedence)((int)level+1), level == Script ? result : right, skip);		// left-associative operators

				// perform operator's action
				if(is_error(*left))			result = *left;
				else if(is_error(*right))	result = *right;
				else						(*pinfo->op)(left, right, result);
				noop = false;

				if(is_unary)	break;
				goto again;
			}
		}
		// for unary operators, return right-hand value_tession if no operators were found
		if(is_unary && noop)	Parse((Precedence)((int)level+1), result, skip);
	}
}

// Parser

Parser::Keywords Parser::_keywords;
wchar_t Parser::_decpt;

Parser::Parser() {
	_decpt = std::use_facet<std::numpunct<char> >(std::locale()).decimal_point();
	if(_keywords.empty()) {
		_keywords[L"if"] = Parser::iffunc;
		_keywords[L"else"] = Parser::ifelse;
		_decpt = std::use_facet<std::numpunct<char> >(std::locale()).decimal_point();
		srand((unsigned)time(NULL));
	}
	_temp.reserve(32);
	_name.reserve(32);
	_pos = _lastpos = 0;
}

Parser::Token Parser::Next()
{
	_lastpos = _pos;
	wchar_t c;
	while(isspace(c = Read()));
	switch(c)	{
		case '\0':	_token = end;break;
		case '+':	_token = plus; break;
		case '-':	_token = minus; break;
		case '*':	_token = multiply;break;
		case '/':	_token = divide;break;
		case '^':	_token = pwr;break;
		case '~':	_token = not;break;
		case ';':	_token = stmt; while(Peek() == c)	Read(); break;
		case '<':	_token = Peek() == '=' ? Read(), le : lt; break;
		case '>':	_token = Peek() == '=' ? Read(), ge : gt; break;
		case '=':	_token = Peek() == '=' ? Read(), equ : setvar; break;
		case '!':	_token = Peek() == '=' ? Read(), nequ : lnot; break;
		case '&':	_token = Peek() == '&' ? Read(), land : land; break;
		case '|':	_token = Peek() == '|' ? Read(), lor : lor ; break;
		case ',':	_token = comma ; break;
		case '(':	_token = lpar;break;
		case ')':	_token = rpar;break;
		case '{':	_token = lcurly; break;
		case '}':	_token = rcurly; break;
		case '#':	_token = value; ReadTime(c); break;
		case '?':	_token = ifop; break;
		case ':':	_token = Peek() == '=' ? Read(), setvar : ifelse; break;
		default:
			if(isdigit(c))	{
				ReadNumber(c);
			}	else	{
				ReadName(c);
				Keywords::const_iterator p = _keywords.find(_name);
				if(p == _keywords.end()) {
					_token = name;
				}
				else {
					_token = p->second;
				}
			}
			break;
	}
	return _token;
}

// Parse integer value from input stream
void Parser::ReadNumber(wchar_t c)
{
	int base = 10, m = c - '0';
	int e1 = 0, e2 = 0, esign = 1;
	bool overflow = false;

	while(c = Read())	{	
		if(isdigit(c))	{
			char v = c - '0';
			if(m > (LONG_MAX - v) / base)	throw error_t::syntax_error;
			m = m * base + v;
		}	else	break;
	};
	Back();
	_value = m;
	_token = Parser::value;
}

// Parse time from input stream
void Parser::ReadTime(wchar_t c)
{
	ReadNumber(Read());
	auto hours = _value;
	if(Read() != ':')	throw error_t::syntax_error;
	ReadNumber(Read());
	auto mins = _value;
	if(Read() != '#')	throw error_t::syntax_error;
	//Back();
	_value = hours * 60 + mins;
	_token = Parser::value;
}


// Parse comma-separated arguments list from input stream
void Parser::ReadArgList()	{
	_args.clear();
	_temp.clear();
	wchar_t c;
	while(isspace(c = Read()));
	if(c != '(')	{Back(); return;}		// function without parameters

	while(c = Read()) {
		if(isspace(c))	{
		}	else if(c == ',')	{
			_args.push_back(_temp);
			_temp.clear();
		}	else if(c == ')')	{
			if(!_temp.empty()) _args.push_back(_temp);
			break;
		}	else if(!isalnum(c) && c!= '_'){
			throw error_t::syntax_error;
		}	else	_temp += c;
	}
}

// Parse object name from input stream
void Parser::ReadName(wchar_t c)
{
	if(!isalpha(c) && c != '@' && c != '_')	throw error_t::syntax_error;
	_name = c;
	while(isalnum(c = Read()) || c == '_' || c == '.')	_name += c;
	Back();
}

void Parser::CheckPairedToken(Parser::Token token)
{
	if(token == lpar && _token != rpar)			throw error_t::syntax_error;
	if(token == lsquare && _token != rsquare)	throw error_t::syntax_error;
	if(token == lcurly && _token != rcurly)		throw error_t::syntax_error;
	if(token == lpar || token == lsquare || token == lcurly)	Next();
}

}