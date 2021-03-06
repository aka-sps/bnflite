
/*************************************************************************\
*   BNF Lite is a C++ template library for lightweight grammar parsers    *
*   Copyright (c) 2017 by Alexander A. Semjonov.  ALL RIGHTS RESERVED.    *
*                                                                         *
*   This program is free software: you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation, either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
*                                                                         *
*   Recommendations for commercial use:                                   *
*   Commercial application should have dedicated LGPL licensed cpp-file   *
*   which exclusively includes GPL licensed "bnflit.h" file. In fact,     *
*   the law does not care how this cpp-file file is linked to other       *
*   binary applications. Just source code of this cpp-file has to be      *
*   published in accordance with LGPL license.                            *
\*************************************************************************/

#ifndef BNFLITE_H
#define BNFLITE_H

#include <string.h>
#include <string>
#include <list>
#include <vector>
#include <bitset>
#include <algorithm>
#include <typeinfo>

namespace bnf
{
// BNF (Backus-Naur form) is a notation for describing syntax of computer languages
// BNF Lite is the source code template library implementing the way to support BNF specifications
//
// BNF Terms:
// * Production rule is formal BNF expression which is a conjunction of a series
//   of more concrete rules:
//      production_rule ::= <rule_1>...<rule_n> | <rule_n_1>...<rule_m>;
// * e.g.
//      <digit> ::= <0> | <1> | <2> | <3> | <4> | <5> | <6> | <7> | <8> | <9>
//      <number> ::= <digit> | <digit> <number>
//      where the number is just a digit or another number with one more digit;

// BNF Lite Terms:
// * Token is a terminal production;
// * Lexem is a lexical production;
// * Rule is used here as synonym of syntax production
//
// BNF Lite Usage:
// Above BNF example can be presented in C++ friendly notation:
//      Lexem Digit = Token("0") | "1"  | "2" | "4" | "5" | "6" | "7" | "8" | "9";
//      LEXEM(Number) = Digit | Digit + Number;
// These both expressions are executable due to this "bnflite.h" source code library
// which supports "Token", "Lexem" and "Rule" classes with overloaded "+" and "|" operators.
// then, e.g. bnf::Analyze(Number, "532" ) can be called with success.
// BNF Lite library supports two kind of callback:
//  - The first kind of callback is treated as bnf element
//      int MyNumber(const char* number_string, size_t length_of_number) ...
//      Lexem Number = Iterate(1, Digit) + MyNumber;
//  - The second kind of callback is bound to 'Rule' element
//      Interface<UserData> CallBack(std::vector<Interface<UserData>>& res) ...
//      Bind(Rule, CallBack);


enum Limits {   maxCharNum = 256, maxLexemLength = 1024, maxIterate = 0x4096
            };
enum Status {   eNone = 0, eErr = 0, eOk = 1,
                eRet = 0x8, e1st = 0x10, eSkip = 0x20, eCatch = 0x40, eTry = 0x80,
                eRest = 0x0100, eNull = 0x0200, eOver = 0x0400, eEof = 0x0800,
                eBadRule = 0x1000, eBadLexem = 0x2000, eSyntax = 0x4000,
                eError = ((~(unsigned int)0) >> 1) + 1
            };

class _Tie; class _And; class _Or; class _Cycle;

/* context class to support the first kind of callback */
class _Base // base parser class
{
public:
    std::vector<const char*> cntxV;
protected: friend class Token; friend class Lexem; friend class Rule;
           friend class _And; friend class _Or; friend class Action;
    int level;
    virtual int _analyze(_Tie& root, const char* text);
    virtual void _erase(int low, int up = 0)
        {   cntxV.erase(cntxV.begin() + low,  up? cntxV.begin() + up : cntxV.end() ); }
    virtual std::pair<void*, int> _pre_call(void* callback)
        {   return std::make_pair((void*)0, 0); }
    virtual void _post_call(std::pair<void*, int> up)
        {};
    virtual void _do_call(std::pair<void*, int> up,
            void* callback, const char* begin, const char* end,  const char* name)
        {};
    virtual void _stub_call(const char* begin, const char* end,  const char* name)
        {};
public:
    _Base(): level(1)
        {};
    virtual ~_Base()
        {};
    int Get_tail(const char** pstop)
        {   const char* ptr = zero_parse(cntxV.back());
            if (pstop) *pstop = ptr;
            return *ptr? eError|eRest: 0;  }
    // primary interface to start parsing of text against constructed rules
    // there are 3 kins of parsing errors presented together with eError in returned status
    // 1) eBadRule, eBadLexem - means the rules tree is not properly built
    // 2) eEof - not enough text for applied rules
    // 3) *pstop != '\0' - not enough rules(or resources), stopped and pointed to unparsed text
    friend int Analyze(_Tie& root, const char* text, const char** pstop = 0);
    template <class U> friend int Analyze(_Tie& root, const char* text, const char** pstop, U& u);
    template <class P> friend int Analyze(_Tie& root, const char* text, P& parser);
    // default parser procedure to skip special symbols
    virtual  const char* zero_parse(const char* ptr)
        {   for (char cc = *ptr; cc != 0; cc = *++ptr) {
                if (cc != ' ' && cc !='\t' && cc != '\n' && cc != '\r') {
                    break; } }
            return ptr; }
    // attempt to catch syntax errors (examples available in professional set)
    virtual int Catch()
        {   return 0; }
};

#if !defined(_MSC_VER)
#define _NAME_OFF 0
#else
#define _NAME_OFF 6
#endif

/* internal base class to support multiform relationships between different BNF elements */
class _Tie
{
    bool _is_compound();
protected:              friend class _Base;  friend class ExtParser;
    friend class _And;  friend class _Or;   friend class _Cycle;
    friend class Token; friend class Lexem; friend class Rule;
    bool inner;
    mutable std::vector<const _Tie*> use;
    mutable std::list<const _Tie*> usage;
    std::string name;
    template<class T> static void _setname(T* t, const char * name = 0)
        {   static int cnt = 0;
            if (name) { t->name = name; }
            else { t->name = typeid(*t).name() + _NAME_OFF;
                   for( int i = ++cnt; i != 0; i /= 10) {
                       t->name += '0' + i - (i/10)*10; } } }
    void _clone(const _Tie* lnk)
        {   usage.swap(lnk->usage);
            for (std::list<const _Tie*>::const_iterator usg = usage.begin(); usg != usage.end(); ++usg) {
                for (unsigned i = 0; i < (*usg)->use.size(); i++) {
                   if ((*usg)->use[i] == lnk) {
                        (*usg)->use[i] = this; } } }
            use.swap(lnk->use);
            for (unsigned i = 0; i < use.size(); i++) {
                if (!use[i]) continue;
                std::list<const _Tie*>::iterator itr =
                    std::find(use[i]->usage.begin(), use[i]->usage.end(), lnk);
                *itr = this; }
            if(lnk->inner) {
                delete lnk; } }
    _Tie(std::string nm = "") :inner(false), name(nm)
        {};
    explicit _Tie(const _Tie* lnk) : inner(true), name(lnk->name)
        {   _clone(lnk); }
    _Tie(const _Tie& link) : inner(link.inner), name(link.name)
        {   _clone(&link); }
    virtual ~_Tie()
        {   for (unsigned int i = 0; i < use.size(); i++) {
                if (use[i]) {
                    use[i]->usage.remove(this);
                    if ( use[i]->inner && use[i]->usage.size() == 0) {
                        delete use[i]; }
                    use[i] = 0; } } }
    static int call_1st(const _Tie* lnk, _Base* parser)
        {   return lnk->_parse(parser); }
    void _clue(const _Tie& link)
        {   if (!use.size() || _is_compound()) {
                use.push_back(&link);
            } else {
                if (use[0]) {
                    use[0]->usage.remove(this);
                    if (use[0]->inner && use[0]->usage.size() == 0) {
                        delete use[0]; } }
                use[0] = &link; }
            link.usage.push_back(this); }
    template<class T> static T* _safe_delete(T* t)
        {   if (t->usage.size() != 0)  {
                if (!t->inner)    {
                    return new T(t); } }
            return 0; }
    virtual int _parse(_Base* parser) const throw() = 0;
public:
    void setName(const char * name)
        {   this->name = name; }
    _Tie& Name(const char * name)
        {   this->name = name; return *this; }
    const char *getName()
        {   return name.c_str(); }
    _And operator+(const _Tie& link);
    _And operator+(const char* s);
    _And operator+(bool (*f)(const char*, size_t));
    friend _And operator+(const char* s, const _Tie& lnk);
    friend _And operator+(bool (*f)(const char*, size_t),const _Tie& lnk);
    _Or operator|(const _Tie& link);
    _Or operator|(const char* s);
    _Or operator|(bool (*f)(const char*, size_t));
    friend _Or operator|(const char* s, const _Tie& lnk);
    friend _Or operator|(bool (*f)(const char*, size_t), const _Tie& lnk);

    // Support Augmented BNF constructions like "<a>*<b><element>" to implement repetition;
    // In ABNF <a> and <b> imply at least <a> and at most <b> occurrences of the element;
    // e.g *<element> allows any number(from 0 to infinity, 1*<element> requires at least one;
    // 3*3<element> allows exactly 3 and 1*2<element> allows one or two.
    _Cycle operator()(int at_least, int total); // ABNF case <a>.<b>*<element> as element(a,b)
    _Cycle operator*(); // ABNF case *<element> (from 0 to infinity)
    _Cycle operator!(); // ABNF case <0>.<1>*<element> or <1><element> (at least one)
    // Note: more readable to use Series, Iterate, Repeat statements correspondingly
    template <class U>
    friend int Analyze(_Tie& root, const char* text, const char** pstop, U& u);
};

/* implementation of parsing control statements */
template <const int flg, const char cc> class _Ctrl: public _Tie
{
protected:  friend class _Tie;
    virtual int _parse(_Base* parser) const throw()
        {   return flg; }
    explicit _Ctrl(const _Ctrl* ctrl) :_Tie(ctrl)
        {};
    _Ctrl(const _Ctrl& control) :_Tie(control)
        {};
public:
     explicit _Ctrl(): _Tie(std::string(1, cc))
        {};
    ~_Ctrl()
        {   _safe_delete(this); }
};

 /* Null operation, immediate successful return */
typedef _Ctrl<eOk, 'N'> Null;  // stub for some constructions (e.g. "zero-or-one")

/* Force Return, immediate return from conjunction rule to impact disjunction rule */
typedef _Ctrl<eOk|eRet, 'R'> Return;

/* Switch to use "Accept First" strategy for disjunction rule instead "Accept Best" */
typedef _Ctrl<e1st, '1'> AcceptFirst;

/* Try to catch Syntax error in current conjunction rule */
typedef _Ctrl<eOk|eTry, 'T'> Try;

/* Check but do not accept next statement for conjunction rule */
typedef _Ctrl<eOk|eSkip, 'S'> Skip;


/* interface class for tokens */
class Token: public _Tie
{
    Token& operator=(const _Tie&);
    explicit Token(const _Tie&);
    std::bitset<bnf::maxCharNum> match;
 protected: friend class _Tie;
    explicit Token(const Token* tkn) :_Tie(tkn), match(tkn->match)
        {};
    virtual int _parse(_Base* parser) const throw()
        {   const char* cc = parser->cntxV.back();
            if (parser->level)
                cc = parser->zero_parse(cc);
            if (match[*((unsigned char*)cc)]) {
                if (parser->level) {
                    parser->_stub_call(cc, cc + 1, name.c_str());
                    parser->cntxV.push_back(cc); }
                parser->cntxV.push_back(++cc);
                return  *cc ? true : true|eEof; }
            return 0; }
public:
    Token(const char c) :_Tie(std::string(1, c))
        {   Add(c, 0); };    // create single char token
    Token(int fst, int lst) :_Tie(std::string(1, fst).append("-") += lst)
        {   Add(fst, lst); };    // create token by ASCII charactes in range
    Token(const char *s) :_Tie(std::string(s))
        {   Add(s); }; // create token by C string sample
    Token(const Token& token) :_Tie(token), match(token.match)
        {};
    virtual ~Token()
        {   _safe_delete(this); }
    void Add(int fst, int lst = 0)  // add characters in range fst...lst;
        {   switch (lst) { // lst == 0|1: add single | upper&lower case character(s)
            case 1: if (fst >= 'A' && fst <= 'Z') match[fst - 'A' + 'a'] = 1;
                    else if (fst >= 'a' && fst <= 'z') match[fst - 'a' + 'A'] = 1;
            case 0: match[(unsigned char)fst] = 1; break;
            default: for (int i = fst; i <= lst; i++) {
                        match[(unsigned char)i] = 1; } } }
    void Add(const char *sample)
        {   while (*sample) {
                match[(unsigned char)*sample++] = 1; } }
    void Remove(int fst, int lst = 0)
        {   for (int i = fst; i <= (lst?lst:fst); i++) {
                match[(unsigned char)i] = 0; } }
    void Remove(const char *sample)
        {   while (*sample) {
                match[(unsigned char)*sample++] = 0; } }
    int GetSymbol(int next = 0)
        {   for (unsigned i = next; i < match.size(); i++) {
                if (match.test(i)) return i; }
            return 0; }
};
#if __cplusplus > 199711L
inline Token operator""_T(const char* sample, size_t len)
    {   return  Token(std::string(sample, len).c_str());    }
#endif

/* internal standalone callback wrapper class */
class Action: public _Tie
{
    bool (*action)(const char* lexem, size_t len);
    Action(_Tie&);
protected:  friend class _Tie;
    explicit Action(const Action* a) :_Tie(a), action(a->action)
        {};
    int _parse(_Base* parser) const throw()
        {   std::vector<const char*>::reverse_iterator itr = parser->cntxV.rbegin() + 1;
            return (*action)(*itr, parser->cntxV.back() - *itr); }
public:
    Action(bool (*action)(const char* lexem, size_t len), const char *name = "")
        :_Tie(name), action(action) {};
    virtual ~Action()
        {   _safe_delete(this); }
};

/* internal class to support conjunction constructions of BNF elements */
class _And: public _Tie
{
protected: friend class _Tie; friend class Lexem;
    _And(const _Tie& b1, const _Tie& b2):_Tie("")
        {   (name = b1.name).append("+") += b2.name; _clue(b1); _clue(b2); }
    explicit _And(const _And* rl) :_Tie(rl)
        {};
    virtual int _parse(_Base* parser) const throw()
        {   int stat = 0; int save = 0; int size = parser->cntxV.size();
            for (unsigned i = 0; i < use.size(); i++, stat &= ~(eSkip|eRet|eOk)) {
                stat |= use[i]->_parse(parser);
                if ((stat & (eOk|eError)) == eOk) {
                    if(save) {
                        parser->cntxV.resize(save);
                        save = 0; }
                    if (stat & eSkip) {
                        save = parser->cntxV.size(); }
                } else {
                    if (parser->level && (stat & eTry) && !(stat & eError) && !save) {
                        stat |= parser->Catch(); }
                    parser->_erase(size);
                    return (stat & (eEof|eOver)? eError : eErr) | (stat & ~(eTry|eSkip|eOk)); }}
            return (stat & eTry? eRet : eNone) | eOk | (stat & ~(eTry|eSkip));   }
public:
    ~_And()
        {   _safe_delete(this); }
    _And& operator+(const _Tie& rule2)
        {   name.append("+") += rule2.name; _clue(rule2); return *this; }
    _And& operator+(const char* s)
        {   name.append("+") += s; _clue(Token(s)); return *this; }
    _And& operator+(bool (*f)(const char*, size_t))
        {   name += "+()"; _clue(Action(f)); return *this; }
    friend _And operator+(const char* s, const _Tie& link);
    friend _And operator+(bool (*f)(const char*, size_t), const _Tie& link);
};
inline _And _Tie::operator+(const _Tie& rule2)
    {   return _And(*this, rule2); }
inline _And _Tie::operator+(const char* s)
    {   return _And(*this, Token(s)); }
inline _And _Tie::operator+(bool (*f)(const char*, size_t))
    {   return _And(*this, Action(f)); }
inline _And operator+(const char* s, const _Tie& link)
    {   return _And(Token(s), link); }
inline _And operator+(bool (*f)(const char*, size_t), const _Tie& link)
    {   return _And(Action(f), link); }

/* internal class to support disjunction constructions of BNF elements */
class _Or: public _Tie
{
protected: friend class _Tie;
    _Or(const _Tie& b1, const _Tie& b2):_Tie("")
        {   (name = b1.name).append("|") += b2.name; _clue(b1); _clue(b2);}
    explicit _Or(const _Or* rl) :_Tie(rl)
        {};
    virtual int _parse(_Base* parser) const throw()
        {   int stat = 0; int max = 0; int tmp = -1;
            const char* org = parser->cntxV.back();
            unsigned int msize; unsigned int size = parser->cntxV.size();
            for (unsigned i = 0; i < use.size(); i++, stat &= ~(eOk|eRet|eError)) {
                msize = parser->cntxV.size();
                if (msize > size) {
                    parser->cntxV.push_back(org); }
                stat |= use[i]->_parse(parser);
                if (stat & (eOk|eError)) {
                    tmp = parser->cntxV.back() - org;
                    if (  (tmp > max) || (tmp > 0 && (stat & (eRet|e1st|eError)))  )  {
                        max = tmp;
                        if (msize > size) {
                            parser->_erase(size, msize + 1); }
                        if (stat & (eRet|e1st|eError)) {
                            break; }
                        continue;  }  }
                if (parser->cntxV.size() > msize) {
                    parser->_erase(msize); } }
            return (max || tmp >= 0 ? stat | eOk: stat & ~eOk) & ~(e1st|eRet); }
public:
    ~_Or()
        {   _safe_delete(this); }
    _Or& operator|(const _Tie& rule2)
        {   name.append("|") += rule2.name; _clue(rule2); return *this; }
    _Or& operator|(const char* s)
        {   name.append("|") += s; _clue(Token(s)); return *this; }
    _Or& operator|(bool (*f)(const char*, size_t))
        {   name += "|()"; _clue(Action(f)); return *this; }
    friend _Or operator|(const char* s, const _Tie& link);
    friend _Or operator|(bool (*f)(const char*, size_t), const _Tie& link);
};
inline _Or _Tie::operator|(const _Tie& rule2)
    {   return _Or(*this, rule2); }
inline _Or _Tie::operator|(const char* s)
    {   return _Or(*this, Token(s)); }
inline _Or _Tie::operator|(bool (*f)(const char*, size_t))
    {   return _Or(*this, Action(f)); }
inline _Or operator|(const char* s, const _Tie& link)
    {   return _Or(Token(s), link); }
inline _Or operator|(bool (*f)(const char*, size_t), const _Tie& link)
    {   return _Or(Action(f), link); }
inline bool _Tie::_is_compound()
        {return dynamic_cast<_And*>(this) || dynamic_cast<_Or*>(this); }


/* interface class for lexem */
class Lexem: public _Tie
{
    Lexem& operator=(const class Rule&);
    Lexem(const Rule& rule);
protected: friend class _Tie;
    explicit Lexem(Lexem* lxm) :_Tie(lxm)
        {};
    virtual int _parse(_Base* parser) const throw()
        {   if (!use.size())
                return eError|eBadLexem;
            if (!parser->level || dynamic_cast<const Action*>(use[0]))
                return use[0]->_parse(parser);
            int size = parser->cntxV.size();
            const char* org = parser->zero_parse(parser->cntxV.back());
            parser->cntxV.push_back(org);
            parser->level--;
            int  stat = use[0]->_parse(parser);
            parser->level++;
            if ((stat & eOk) && parser->cntxV.size() - size > 1) {
                parser->_stub_call(org, parser->cntxV.back(), name.c_str());
                parser->cntxV[(++size)++] = parser->cntxV.back(); }
            parser->cntxV.resize(size);
            return stat; }
public:
    Lexem(const char *literal, bool cs = 0) :_Tie()
        {   int size = strlen(literal);
            switch (size) {
            case 1: this->operator=(Token(literal[0], cs));
            case 0: break;
            default: {
                _And _and(Token(literal[0], cs), Token(literal[1], cs));
                for (int i = 2; i < size; i++) {
                    _and.operator+((const _Tie&)Token(literal[i], cs)); }
                this->operator=(_and); } }
            _setname(this, literal);  }
    explicit Lexem() :_Tie()
        {   _setname(this); }
    virtual ~Lexem()
        {   _safe_delete(this); }
    Lexem(const _Tie& link) :_Tie()
        {   _setname(this, 0); _clue(link); }
    Lexem& operator=(const Lexem& lexem)
        {   if (&lexem != this) _clue(lexem);
            return *this; }
    Lexem& operator=(const _Tie& link)
        {   _clue(link); return *this; }
};

/* interface class for BNF rules */
class Rule : public _Tie
{
    void* callback;
protected:  friend class _Tie; friend class _And;
    explicit Rule(const Rule* rl) :_Tie(rl), callback(rl->callback)
    {};
    virtual int _parse(_Base* parser) const throw()
        {   if (!use.size() || !parser->level)
                return eError|eBadRule;
            if (dynamic_cast<const Action*>(use[0])) {
                return use[0]->_parse(parser); }
            int size = parser->cntxV.size();
            std::pair<void*, int> up = parser->_pre_call(callback);
            int stat = use[0]->_parse(parser);
            if ((stat & eOk) && parser->cntxV.size() - size > 1) {
                parser->_do_call(up, callback, parser->cntxV[size], parser->cntxV.back(), name.c_str());
                parser->cntxV[(++size)++] = parser->cntxV.back(); }
            parser->cntxV.resize(size);
            parser->_post_call(up);
            return stat; }
public:
    explicit Rule() :_Tie(), callback(0)
        {   _setname(this); }
    virtual ~Rule()
        {   _safe_delete(this); }
    Rule(const _Tie& link) :_Tie(), callback(0)
        {   const Rule* rl = dynamic_cast<const Rule*>(&link);
            if (rl) { _clone(&link);  callback = rl->callback; name = rl->name; }
            else { _clue(link);   callback = 0; _setname(this);  } }
    Rule& operator=(const _Tie& link)
        {   _clue(link); return *this; }
    Rule& operator=(const Rule& rule)
        {   if (&rule == this) return *this;
            return this->operator=((const _Tie&)rule); }
    template <class U> friend Rule& Bind(Rule& rule, U (*callback)(std::vector<U>&));
    template <class U> Rule& operator[](U (*callback)(std::vector<U>&));
};

/* friendly debug interface */
#define LEXEM(lexem) Lexem lexem; lexem.setName(#lexem); lexem
#define RULE(rule) Rule rule; rule.setName(#rule); rule

/* internal class to support repeat constructions of BNF elements */
class _Cycle: public _Tie
{
    unsigned int min;
    unsigned int max;
    int flag;
protected: friend class _Tie;
    explicit _Cycle(const _Cycle* cl) :_Tie(cl), min(cl->min), max(cl->max), flag(cl->flag)
        {};
    _Cycle(const _Cycle& cycle) :_Tie(cycle), min(cycle.min), max(cycle.max), flag(cycle.flag)
        {};
    int _parse(_Base* parser) const throw()
        {   int stat; unsigned int i;
            for (stat = 0, i = 0; i < max; i++, stat &= ~(e1st|eTry|eSkip|eRet|eOk)) {
                stat |= use[0]->_parse(parser);
                if ((stat & (eOk|eError)) == eOk)
                    continue;
                return i < min? stat & ~eOk : stat | eOk; }
            return stat | flag | eOk; }
    _Cycle(int at_least, const _Tie& link, int total = maxIterate, int limit = maxIterate)
        :_Tie(std::string("Iterate")), min(at_least), max(total), flag(total < limit? eNone : eOver)
        {   _clue(link); }
public:
    ~_Cycle()
        {   _safe_delete(this); }
    friend _Cycle operator*(int x, const _Tie& link);
    friend _Cycle Repeat(int at_least, const Rule& rule, int total = maxLexemLength, int limit = maxLexemLength);
    friend _Cycle Iterate(int at_least, const Lexem& lexem, int total = maxLexemLength, int limit = maxLexemLength);
    friend _Cycle Series(int at_least, const Token& token, int total = maxLexemLength, int limit = maxLexemLength);
};
inline _Cycle _Tie::operator*()
    {   return _Cycle(0, *this); }
inline _Cycle _Tie::operator!()
    {   return _Cycle(0, *this, 1); }
inline _Cycle _Tie::operator()(int at_least, int total)
    {   return _Cycle(at_least, *this, total); }
inline _Cycle operator*(int x, const _Tie& link)
    {   return _Cycle((int)x, link); }
inline _Cycle Repeat(int at_least, const Rule& rule, int total, int limit)
    {   return _Cycle(at_least, rule, total, limit); }
inline _Cycle Iterate(int at_least, const Lexem& lexem, int total, int limit)
    {   return _Cycle(at_least, lexem, total, limit); }
inline _Cycle Series(int at_least, const Token& token, int total, int limit)
    {   return _Cycle(at_least, token, total, limit); }

inline int _Base::_analyze(_Tie& root, const char* text)
{   cntxV.push_back(text); cntxV.push_back(text);
    return root._parse(this); }

/* context class to support the second kind of callback */
template <class U> class _Parser : public _Base
{
protected:
    std::vector<U>* cntxU;
    unsigned int off;
    void _erase(int low, int up = 0)
        {   cntxV.erase(cntxV.begin() + low,  up? cntxV.begin() + up : cntxV.end() );
            if (cntxU && level)
                cntxU->erase(cntxU->begin() + (low - off) / 2,
                 up? cntxU->begin() + (up - off) / 2 : cntxU->end()); }
    virtual std::pair<void*, int> _pre_call(void* callback)
        {   std::pair<void*, int> up = std::make_pair(cntxU, off);
            cntxU = callback? new std::vector<U> : 0;
            off = callback? cntxV.size() : 0;
            return up; }
    virtual void  _post_call(std::pair<void*, int> up)
        {   if (cntxU) {
                delete cntxU; }
            cntxU = (std::vector<U>*)up.first;
            off = up.second; }
    virtual void _do_call(std::pair<void*, int> up,
            void* callback, const char* begin, const char* end, const char* name)
        {   if (callback) {
                if (up.first) {
                    ((std::vector<U>*)up.first)->push_back(U(reinterpret_cast<
                        U(*)(std::vector<U>&)>(callback)(*cntxU), begin, end - begin, name));
                } else { reinterpret_cast<U(*)(std::vector<U>&)>(callback)(*cntxU); }
            } else if (up.first) {
                    ((std::vector<U>*)up.first)->push_back(U(begin, end - begin, name)); } }
    virtual void _stub_call( const char* begin, const char* end,  const char* name)
        {   if (cntxU) {
                cntxU->push_back(U(begin, end - begin, name)); } }
public:
    _Parser(std::vector<U>* res = 0) :cntxU(res), off(0)
        {};
    virtual ~_Parser()
        {};
    void Get_result(U& u)
        { if (cntxU && cntxU->size()) u = cntxU->front(); }
    template <class W> friend Rule& Bind(Rule& rule, W (*callback)(std::vector<W>&));
};

/* User interface template to support the second kind of callback */
/* like: Interface<Foo> CallBack(std::vector<Interface<Foo>>& res); */
/* The user has to specify own 'Data' abstract type to work with this template */
/* The user also can create own class just supporting mandatory constructors */
template <typename Data = bool> struct Interface
{
    Data data;              //  user data element
    const char* text;       //  pointer to parsed text according to bound Rule
    size_t length;          //  length of parsed text according to bound Rule
    const char* name;       //  the name of bound Rule
    Interface(const Interface& ifc, const char* text, size_t length, const char* name)
        :data(ifc.data) , text(text), length(length), name(name)
        {}; // mandatory constructor with user data to be called from library
    Interface(const char* text, size_t length,  const char* name)
        :data(0), text(text), length(length), name(name)
        {}; //  mandatory default constructor to be called from library
    Interface(Data data, std::vector<Interface>& res, const char* name = "")
        :data(data), text(res.size()? res[0].text: ""),
          length(res.size()? res[res.size() - 1].text
            - res[0].text + res[res.size() - 1].length : 0), name(name)
        {}; // constructor to pass data from user's callback to library
    Interface(const Interface& front, const Interface& back, const char* name = "")
        : data(0), text(front.text), length(back.text - front.text + back.length), name(name)
        {}; // constructor to pass data from user's callback to library
    Interface(): data(0), text(0), length(0), name(0)
        {}; // default constructor
    static Interface ByPass(std::vector<Interface>& res) // simplest user callback example
        {   return res.size()? res[0]: Interface(); }   // just to pass data to upper level
};


/* Start parsing with supporting the first kind of callback only (faster) */
inline int Analyze(_Tie& root, const char* text, const char** pstop)
    {   _Base base; return base._analyze(root, text) | base.Get_tail(pstop); }

/* Start parsing with supporting both kinds of callback */
template <class U> inline int Analyze(_Tie& root, const char* text, const char** pstop, U& u)
    {   std::vector<U> v; _Parser<U> parser(&v);
        int stat = parser._analyze(root, text);
        parser.Get_result(u);
        return stat | parser.Get_tail(pstop); }

/* Start custom parsing, use getPStop and getResult to obtain results */
template <class P> inline int Analyze(_Tie& root, const char* text, P& parser)
    {   return parser._analyze(root, text) | parser.Get_tail(0); }


/* Create association between Rule and user's callback */
template <class U> inline Rule& Bind(Rule& rule, U (*callback)(std::vector<U>&))
    {   rule.callback = reinterpret_cast<void*>(callback); return rule; }
template <class U> inline Rule& Rule::operator[](U (*callback)(std::vector<U>&)) // for C++11
    {   this->callback = reinterpret_cast<void*>(callback); return *this; }


}; // bnf::
#endif // BNFLITE_H
