#ifndef GROSS_FRONTEND_PARSER_H
#define GROSS_FRONTEND_PARSER_H
#include "Lexer.h"
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

template<class K, class V,
         size_t Affinity = 2>
struct AffineRecordTable {
  using TableTy = std::unordered_map<K,V>;
  using table_iterator = typename TableTy::iterator;

private:
  // TODO: release table if no longer richable
  std::set<std::unique_ptr<TableTy>> TableRepo;

  struct Scope {
    size_t ParentBranch, CurrentBranch;
    std::array<TableTy*, Affinity> TableBranches;

    Scope() : ParentBranch(0), CurrentBranch(0) {}
    Scope(TableTy* InitTable, size_t ParentBr) :
      ParentBranch(ParentBr), CurrentBranch(0) {
      TableBranches[0] = InitTable;
    }

    size_t CurBranch() const {
      return CurrentBranch;
    }

    void AddBranch(TableTy* Table) {
      size_t NewBr = CurBranch() + 1;
      assert(NewBr < Affinity && "new branch out-of-bound");
      TableBranches[NewBr] = Table;
      ++CurrentBranch;
    }
  };

  // back is the stack top
  std::list<Scope> ScopeStack;
  using scope_iterator = typename decltype(ScopeStack)::iterator;
  // we use std::list because we want consistent iterator
  scope_iterator CurScope, PrevScope;

  TableTy* PrevTable() {
    if(PrevScope == ScopeStack.end() ||
       CurScope == ScopeStack.end())
      return nullptr;

    auto ParentBr = CurScope->ParentBranch;
    return PrevScope->TableBranches.at(ParentBr);
  }

  TableTy* CurTable() {
    if(CurScope == ScopeStack.end()) return nullptr;
    return CurScope->TableBranches.at(CurScope->CurBranch());
  }

public:
  AffineRecordTable() {
    // create default scope
    std::unique_ptr<TableTy> TablePtr(new TableTy());
    ScopeStack.push_back(Scope(TablePtr.get(), 0));
    TableRepo.insert(std::move(TablePtr));

    CurScope = ScopeStack.begin();
    PrevScope = ScopeStack.end();
  }

  size_t num_scopes() const {
    return ScopeStack.size();
  }
  // number of table branches in current scope
  size_t num_tables() const {
    return CurScope->CurBranch() + 1;
  }

  void NewAffineScope() {
    size_t CurBr = CurScope->CurBranch();
    auto* CurTable = CurScope->TableBranches.at(CurBr);
    PrevScope = CurScope;
    CurScope = ScopeStack.insert(ScopeStack.cend(),
                                 Scope(CurTable, CurBr));
  }
  // create and switch to the new branch
  void NewBranch() {
    CurScope->AddBranch(PrevTable());
  }

  // end iterator of current table
  table_iterator end() {
    return CurTable()->end();
  }
  // find in current table
  table_iterator find(const K& key) {
    return CurTable()->find(key);
  }
  // insert new entry in current table:
  // copy from parent then modify
  V& operator[](const K& key) {
    // copy from parent if needed
    if(PrevTable() && CurTable() == PrevTable()) {
      const auto& Orig = *PrevTable();
      std::unique_ptr<TableTy> TablePtr(new TableTy(Orig));
      CurScope->TableBranches[CurScope->CurBranch()] = TablePtr.get();
      TableRepo.insert(std::move(TablePtr));
    }
    return (*CurTable())[key];
  }

  // where you should insert PHI node
  // and update table
  template<
    class Func = std::function<void(TableTy*,const std::vector<TableTy*>&)>
  >
  void CloseAffineScope(Func Callback) {
    assert(PrevScope != ScopeStack.end() &&
           "cannot pop out the only scope");
    auto& CurTables = CurScope->TableBranches;
    std::vector<TableTy*> Tables(CurTables.begin(),
                                 CurTables.begin() + (CurScope->CurBranch() + 1)
                                );
    Callback(PrevTable(), Tables);
    CurScope = PrevScope;
    if(PrevScope == ScopeStack.end())
      PrevScope = ScopeStack.end();
    else
      --PrevScope;
    ScopeStack.pop_back();
  }
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

  class SymbolLookup;

  // record last modify node on a certain decl
  // {decl, modifier}
  //std::unordered_map<Node*, Node*> LastModified;
  AffineRecordTable<Node*, Node*> LastModified;

  /// placeholder function to avoid link time error
  /// for un-specialized decl template functions
  void __SupportedParseVarDecls();

public:
  Parser(std::istream& IS, Graph& graph)
    : G(graph),
      Lex(IS) {}

  Lexer& getLexer() { return Lex; }

  void NewSymScope() {
    SymTable.new_scope();
  }

  bool Parse();

  template<IrOpcode::ID OC>
  bool ParseTypeDecl(NodeBuilder<OC>& NB);

  template<IrOpcode::ID OC>
  bool ParseVarDecl(std::vector<Node*>* Results = nullptr);

  Node* ParseDesignator();
  Node* ParseArrayAccessDesignator(Node* Decl, Node* Effect,
                                   const std::string& IdentName = "");

  Node* ParseTerm();
  Node* ParseFactor();
  Node* ParseExpr();
  Node* ParseRelation();

  Node* ParseAssignment();
  bool ParseStatements(std::vector<Node*>& Stmts);
  Node* ParseIfStmt();

  bool ParseFuncDecl();
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
