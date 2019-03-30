#ifndef GROSS_FRONTEND_PARSER_H
#define GROSS_FRONTEND_PARSER_H
#include "Lexer.h"
#include "gross/Support/AffineContainer.h"
#include <array>
#include <functional>
#include <list>
#include <unordered_map>
#include <utility>
#include <set>
#include <memory>
#include <vector>

namespace gross {
// Forward declarations
namespace IrOpcode {
  enum ID : unsigned;
};
class Graph;
class Node;
template<IrOpcode::ID OC>
struct NodeBuilder;
template<class T>
struct NodeMarker;
struct AttributeBuilder;

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
  void push_new(Args&&... args) {
    Storage.emplace_front(std::forward<Args>(args)...);
  }

  void pop() { Storage.erase(Storage.begin()); }

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

  const SymTableTy& CurSymTable() const { return SymTable.top(); }
  SymTableTy& CurSymTable() { return SymTable.top(); }

  typename ScopedSymTableTy::const_iterator
  CurSymTableIt() const { return SymTable.cbegin(); }
  typename ScopedSymTableTy::iterator
  CurSymTableIt() { return SymTable.begin(); }

  // hide the (prev/next)direction of symtable implementation
  void MoveToPrevSymTable(typename ScopedSymTableTy::iterator& ItScope) {
    ++ItScope;
  }
  void MoveToNextSymTable(typename ScopedSymTableTy::iterator& ItScope) {
    --ItScope;
  }

  class SymbolLookup;

  // record last modify node on a certain decl
  // {decl, modifier}
  AffineRecordTable<Node*, Node*> LastModified;
  // record last memory read/write node last memory write
  // {MemStore, accessor}
  AffineRecordTable<Node*, std::set<Node*>> LastMemAccess;
  AffineContainer<std::array<Node*,1>> LastControlPoint;
  std::unordered_map<Node*, Node*> InitialValCache;

  Node* getInitialValue(Node* Decl);

  inline void NewLastControlPoint() {
    LastControlPoint = std::move(decltype(LastControlPoint)());
  }
  inline void NewLastModified() {
    LastModified = std::move(decltype(LastModified)());
    InitialValCache.clear();
  }
  inline void NewLastMemAccess() {
    LastMemAccess = std::move(decltype(LastMemAccess)());
  }

  NodeMarker<uint16_t>* NodeIdxMarker;

  void InspectFuncNodeUsages(Node* FuncEnd);

  void InstallBuiltin(const std::string& name, size_t NumArgs,
                      AttributeBuilder&& AttrBuilder);

  /// placeholder function to avoid link time error
  /// for un-specialized decl template functions
  void __SupportedParseVarDecls();

public:
  Parser(std::istream& IS, Graph& graph)
    : G(graph),
      Lex(IS),
      NodeIdxMarker(nullptr) {}

  Lexer& getLexer() { return Lex; }

  void NewSymScope() {
    SymTable.push_new();
  }
  void PopSymScope() {
    if(!SymTable.empty())
      SymTable.pop();
  }

  inline Node* getLastCtrlPoint() {
    return std::get<0>(*LastControlPoint.CurEntry());
  }
  inline void setLastCtrlPoint(Node* N) {
    (*LastControlPoint.CurEntryMutable())[0] = N;
  }

  void SetNodeIdxMarker(NodeMarker<uint16_t>* Marker);
  uint16_t GetNodeIdx(Node*);
  uint16_t GetCurrentNodeIdx();
  void ClearNodeIdxMarker();

  /// For each ParseXXX, it would expect the lexer cursor
  /// starting on the first token of its semantic rule.
  bool Parse(bool StepLexer = true);

  template<IrOpcode::ID OC>
  bool ParseTypeDecl(NodeBuilder<OC>& NB);
  template<IrOpcode::ID OC>
  bool ParseVarDecl(std::vector<Node*>* Results = nullptr);
  bool ParseVarDeclTop(std::vector<Node*>* Results = nullptr);

  Node* ParseDesignator();
  Node* ParseArrayAccessDesignator(Node* Decl, Node* Effect,
                                   const std::string& IdentName = "");
  Node* ParseGlobalVarAccessDesignator(Node* Decl, Node* Effect);

  Node* ParseTerm();
  Node* ParseFactor();
  Node* ParseExpr();
  Node* ParseRelation();

  Node* ParseAssignment();
  bool ParseStatements(std::vector<Node*>& Stmts);
  Node* ParseIfStmt();
  Node* ParseWhileStmt();
  Node* ParseReturnStmt();
  Node* ParseFuncCall();

  // return the end node
  Node* ParseFuncDecl();
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
