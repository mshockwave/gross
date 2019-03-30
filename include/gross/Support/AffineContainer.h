#ifndef GROSS_SUPPORT_AFFINECONTAINER_H
#define GROSS_SUPPORT_AFFINECONTAINER_H
#include <array>
#include <functional>
#include <list>
#include <set>
#include <memory>
#include <unordered_map>
#include <vector>
#include <cassert>

namespace gross {
template<class ContainerTy,
         size_t Affinity = 2>
struct AffineContainer {
  using value_type = ContainerTy;

protected:
  // TODO: release container if no longer richable
  std::set<std::unique_ptr<ContainerTy>> Repo;

  struct Scope {
    size_t ParentBranch, CurrentBranch;
    std::array<ContainerTy*, Affinity> Branches;

    Scope() : ParentBranch(0), CurrentBranch(0) {}
    Scope(ContainerTy* Init, size_t ParentBr) :
      ParentBranch(ParentBr), CurrentBranch(0) {
      Branches[0] = Init;
    }

    size_t CurBranch() const {
      return CurrentBranch;
    }

    void AddBranch(ContainerTy* Container) {
      size_t NewBr = CurBranch() + 1;
      assert(NewBr < Affinity && "new branch out-of-bound");
      Branches[NewBr] = Container;
      ++CurrentBranch;
    }
  };

  // back is the stack top
  std::list<Scope> ScopeStack;
  using scope_iterator = typename decltype(ScopeStack)::iterator;
  // we use std::list because we want consistent iterator
  scope_iterator CurScope, PrevScope;

  ContainerTy* PrevEntry() {
    if(PrevScope == ScopeStack.end() ||
       CurScope == ScopeStack.end())
      return nullptr;

    auto ParentBr = CurScope->ParentBranch;
    return PrevScope->Branches.at(ParentBr);
  }

  void CopyOnWrite() {
    // copy from parent if needed
    if(PrevEntry() && CurEntry() == PrevEntry()) {
      const auto& Orig = *PrevEntry();
      std::unique_ptr<ContainerTy> Ptr(new ContainerTy(Orig));
      CurScope->Branches[CurScope->CurBranch()] = Ptr.get();
      Repo.insert(std::move(Ptr));
    }
  }

public:
  AffineContainer() {
    // create default scope
    std::unique_ptr<ContainerTy> TablePtr(new ContainerTy());
    ScopeStack.push_back(Scope(TablePtr.get(), 0));
    Repo.insert(std::move(TablePtr));

    CurScope = ScopeStack.begin();
    PrevScope = ScopeStack.end();
  }

  size_t num_scopes() const {
    return ScopeStack.size();
  }
  // number of branches in current scope
  size_t num_entries() const {
    return CurScope->CurBranch() + 1;
  }

  ContainerTy* CurEntry() {
    if(CurScope == ScopeStack.end()) return nullptr;
    return CurScope->Branches.at(CurScope->CurBranch());
  }
  ContainerTy* CurEntryMutable() {
    CopyOnWrite();
    return CurEntry();
  }

  void NewAffineScope() {
    size_t CurBr = CurScope->CurBranch();
    auto* CurTable = CurScope->Branches.at(CurBr);
    PrevScope = CurScope;
    CurScope = ScopeStack.insert(ScopeStack.end(),
                                 Scope(CurTable, CurBr));
  }
  // create and switch to the new branch
  void NewBranch() {
    CurScope->AddBranch(PrevEntry());
  }

  template<
    class T = AffineContainer<ContainerTy,Affinity>,
    class Func = std::function<void(T&,
                                    const std::vector<ContainerTy*>&)>
  >
  void CloseAffineScope(Func Callback) {
    assert(PrevScope != ScopeStack.end() &&
           "cannot pop out the only scope");
    auto& CurEntries = CurScope->Branches;
    std::vector<ContainerTy*> Entries(
      CurEntries.begin(),
      CurEntries.begin() + (CurScope->CurBranch() + 1)
    );
    // switch to previous scope without poping the current scope
    CurScope = PrevScope;
    if(PrevScope == ScopeStack.end())
      PrevScope = ScopeStack.end();
    else
      --PrevScope;
    Callback(*static_cast<T*>(this), Entries);

    ScopeStack.pop_back();
  }
};

template<class K, class V,
         size_t Affinity = 2>
class AffineRecordTable
  : public AffineContainer<std::unordered_map<K,V>, Affinity> {
  using BaseTy = AffineContainer<std::unordered_map<K,V>, Affinity>;

public:
  using TableTy = std::unordered_map<K,V>;
  using table_iterator = typename TableTy::iterator;

  AffineRecordTable() = default;

  size_t num_tables() const { return BaseTy::num_entries(); }

  // begin/end iterators for current table
  table_iterator begin() {
    return BaseTy::CurEntry()->begin();
  }
  table_iterator end() {
    return BaseTy::CurEntry()->end();
  }
  // find in current table
  table_iterator find(const K& key) {
    return BaseTy::CurEntry()->find(key);
  }
  size_t count(const K& key) {
    return BaseTy::CurEntry()->count(key);
  }
  // insert new entry in current table:
  // copy from parent then modify
  V& operator[](const K& key) {
    return (*BaseTy::CurEntryMutable())[key];
  }
  V& at(const K& key) {
    return (*BaseTy::CurEntryMutable()).at(key);
  }

  template<
    class Func = std::function<void(AffineRecordTable<K,V,Affinity>&,
                                    const std::vector<TableTy*>&)>>
  void CloseAffineScope(Func Callback) {
    BaseTy::template CloseAffineScope<AffineRecordTable<K,V,Affinity>,
                                      Func>(Callback);
  }
};
} // end namespace gross
#endif
