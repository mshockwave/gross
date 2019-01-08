#ifndef GROSS_FRONTEND_PARSER_H
#define GROSS_FRONTEND_PARSER_H
#include "Lexer.h"
#include <list>
#include <unordered_map>
#include <utility>

namespace gross {
// Forward declaration
class Graph;

// Just a small wrapper providing
// iterable stack implementation
template<class T>
class ScopedSymTable {
  // use linked list since sometimes we need to store
  // the iterator of certain element
  std::list<T> Storage;

public:
  using iterator = typename decltype(Storage)::iterator;
  using const_iterator = typename decltype(Storage)::const_iterator;

  // front is the top
  void push(const T& val) {
    Storage.insert(Storage.begin(),
                   val);
  }

  template<class... Args>
  void new_scope(Args&&... args) {
    Storage.emplace_back(std::forward<Args>(args)...);
  }

  void pop() { Storage.erase(Storage.cbegin()); }

  T pop_val() {
    T val = top();
    pop();
    return std::move(val);
  }

  T& top() { return Storage.front(); }
  const T& top() const { return Storage.front(); }

  bool empty() const { return Storage.empty(); }

  iterator begin() { return Storage.begin(); }
  iterator end() { return Storage.end(); }
  const_iterator cbegin() const { return Storage.cbegin(); }
  const_iterator cend() const { return Storage.cend(); }
};

class Parser {
  Graph& G;
  Lexer Lex;

  inline
  Lexer::Token NextTok() {
    return Lex.getNextToken();
  }
  Lexer::Token CurTok() {
    return Lex.getToken();
  }

  inline
  const std::string& TokBuffer() {
    return Lex.getBuffer();
  }

  using SymTableTy
    = std::unordered_map<std::string, Node*>;
  using ScopedSymTableTy
    = ScopedSymTable<SymTableTy>;
  ScopedSymTableTy SymTable;

  void NewSymScope() {
    SymTable.new_scope();
  }

  const SymTableTy& CurSymTable() const { return SymTable.top(); }
  SymTableTy& CurSymTable() { return SymTable.top(); }

  typename ScopedSymTableTy::const_iterator
  CurSymTableIt() const { return SymTable.cbegin(); }
  typename ScopedSymTableTy::iterator
  CurSymTableIt() { return SymTable.begin(); }

  class SymbolLookup;

  bool ParseVarDecl();

  bool ParseArrayDecl();

  bool ParseFuncDecl();

public:
  Parser(std::istream& IS, Graph& graph)
    : G(graph),
      Lex(IS) {}

  bool Parse();
};

class Parser::SymbolLookup {
  const Parser& Ctx;
  const std::string& Sym;
  typename ScopedSymTableTy::const_iterator ItCurScope;

public:
  SymbolLookup(const Parser& P, const std::string& Symbol) :
    Ctx(P), Sym(Symbol) {
    ItCurScope = Ctx.CurSymTableIt();
  }

  // <node, level>
  using ResultTy = std::pair<Node*, unsigned>;
  ResultTy operator*() {
    unsigned Level = 0;
    Node* N = nullptr;
    for(auto SE = Ctx.SymTable.cend(), S = ItCurScope;
        S != SE; ++S, ++Level) {
      auto SR = S->find(Sym);
      if(SR != S->end()) {
        N = SR->second;
        break;
      }
    }
    return std::make_pair(N, Level);
  }

  inline
  bool InCurrentScope() {
    auto R = operator*();
    return R.first && !R.second; // level is zero
  }
};
} // end namespace gross
#endif
