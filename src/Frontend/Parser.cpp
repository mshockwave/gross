#include "gross/Support/Log.h"
#include "gross/Graph/Graph.h"
#include "gross/Graph/NodeUtils.h"
#include "gross/Graph/NodeMarker.h"
#include "gross/Graph/AttributeBuilder.h"
#include "Parser.h"
#include <sstream>

using namespace gross;

void Parser::SetNodeIdxMarker(NodeMarker<uint16_t>* Marker) {
  NodeIdxMarker = Marker;
  G.SetNodeIdxMarker(Marker);
}
void Parser::ClearNodeIdxMarker() {
  NodeIdxMarker = nullptr;
  G.ClearNodeIdxMarker();
}

uint16_t Parser::GetNodeIdx(Node* N) {
  assert(NodeIdxMarker);
  return NodeIdxMarker->Get(N);
}
uint16_t Parser::GetCurrentNodeIdx() {
  return GetNodeIdx(G.getNode(G.node_size() - 1));
}

Node* Parser::getInitialValue(Node* Decl) {
  if(!InitialValCache.count(Decl)) {
    // create one
    auto* N = NodeBuilder<IrOpcode::SrcInitialArray>(&G, Decl)
              .Build();
    InitialValCache[Decl] = N;
  }
  return InitialValCache.at(Decl);
}

void Parser::InspectFuncNodeUsages(Node* FuncEnd) {
  assert(FuncEnd);
  SubGraph SG(FuncEnd);
  Node* FuncNode = nullptr;
  for(auto* N : FuncEnd->inputs()) {
    if(N->getOp() == IrOpcode::Start) {
      FuncNode = N;
      break;
    }
  }
  assert(FuncNode);

  // mark global variable usages
  AttributeBuilder FuncAttrBuilder(G);
  for(auto* N : SG.nodes()) {
    // to see if any of the node access global vars
    switch(N->getOp()) {
    case IrOpcode::SrcAssignStmt: {
      NodeProperties<IrOpcode::SrcAssignStmt> NP(N);
      NodeProperties<IrOpcode::VirtSrcDesigAccess> ANP(NP.dest());
      assert(ANP);
      if(G.IsGlobalVar(ANP.decl()) &&
         !FuncAttrBuilder.hasAttr<Attr::WriteMem>()) {
        FuncAttrBuilder.Add<Attr::WriteMem>();
      }
      break;
    }
    case IrOpcode::SrcVarAccess:
    case IrOpcode::SrcArrayAccess: {
      bool Found = false;
      for(auto* VU : N->value_users()) {
        if(VU->getOp() != IrOpcode::SrcAssignStmt) {
          Found = true;
          break;
        }
      }
      if(Found) {
        NodeProperties<IrOpcode::VirtSrcDesigAccess> ANP(N);
        if(G.IsGlobalVar(ANP.decl()) &&
           !FuncAttrBuilder.hasAttr<Attr::ReadMem>()) {
          FuncAttrBuilder.Add<Attr::WriteMem>();
        }
      }
      break;
    }
    case IrOpcode::Call: {
      auto* Stub = NodeProperties<IrOpcode::Call>(N).getFuncStub();
      NodeProperties<IrOpcode::FunctionStub> StubNP(Stub);
      assert(StubNP);
      // propagate attributes
      // FIXME: use iteration
      if(StubNP.hasAttribute<Attr::WriteMem>(G)) {
        FuncAttrBuilder.Add<Attr::WriteMem>();
      }
      if(StubNP.hasAttribute<Attr::ReadMem>(G)) {
        FuncAttrBuilder.Add<Attr::ReadMem>();
      }
      if(StubNP.hasAttribute<Attr::HasSideEffect>(G)) {
        FuncAttrBuilder.Add<Attr::HasSideEffect>();
      }
      break;
    }
    default: continue;
    }
  }
  if(!FuncAttrBuilder.hasAttr<Attr::ReadMem>() &&
     !FuncAttrBuilder.hasAttr<Attr::WriteMem>()) {
    FuncAttrBuilder.Add<Attr::NoMem>();
  }
  if(!FuncAttrBuilder.empty()) FuncAttrBuilder.Attach(FuncNode);
}

void Parser::InstallBuiltin(const std::string& name, size_t NumArgs,
                            AttributeBuilder&& AttrBuilder) {
  NodeBuilder<IrOpcode::VirtFuncPrototype> FB(&G);
  FB.FuncName(name.c_str());
  for(auto i = 0; i < NumArgs; ++i) {
    std::stringstream SS;
    SS << "arg" << i;
    auto* Param = NodeBuilder<IrOpcode::Argument>(&G, SS.str()).Build();
    FB.AddParameter(Param);
  }
  auto* FuncNode = FB.Build();
  CurSymTable().insert({name, FuncNode});

  NodeBuilder<IrOpcode::End> EB(&G, FuncNode);
  auto* EndNode = EB.Build();
  AttrBuilder.Attach(FuncNode);

  SubGraph SG(EndNode);
  G.AddSubRegion(SG);
  (void) NodeBuilder<IrOpcode::FunctionStub>(&G, SG).Build();
}

/// Parse computation
bool Parser::Parse(bool StepLexer) {
  if(StepLexer)
    (void) NextTok();

  auto Tok = CurTok();
  if(!(Tok == Lexer::TOK_IDENT &&
       TokBuffer() == "main")) {
    Log::E() << "Expecting 'main' here\n";
    return false;
  }
  Tok = NextTok();

  // we basically treat main as special function
  NodeBuilder<IrOpcode::VirtFuncPrototype> FB(&G);
  FB.FuncName("main");

  // global variable scope
  NewSymScope();
  auto* FuncNode = FB.Build();
  CurSymTable().insert({"main", FuncNode});

  // for the entire function
  NodeMarker<uint16_t> NodeIdxMarker(G, 10000);
  SetNodeIdxMarker(&NodeIdxMarker);

  std::vector<Node*> GlobalVars;
  if(!ParseVarDeclTop(&GlobalVars)) return false;
  for(auto* GV : GlobalVars) {
    if(GV->getOp() == IrOpcode::SrcVarDecl) {
      // transform into array decl for the sake of
      // lowering to memory load/store later
      NodeProperties<IrOpcode::VirtSrcDecl> DNP(GV);
      auto* NewDecl = NodeBuilder<IrOpcode::SrcArrayDecl>(&G)
                      .SetSymbolName(DNP.ident_name(G))
                      .AddConstDim(1).Build();
      CurSymTable()[DNP.ident_name(G)] = NewDecl;
      GV->ReplaceWith(NewDecl);
      G.MarkGlobalVar(NewDecl);
    } else {
      G.MarkGlobalVar(GV);
    }
  }

  // builtins
  InstallBuiltin("InputNum", 0,
                 std::move(AttributeBuilder(G)
                           .Add<Attr::IsBuiltin>()
                           .Add<Attr::NoMem>()));
  InstallBuiltin("OutputNum", 1,
                 std::move(AttributeBuilder(G)
                           .Add<Attr::IsBuiltin>()
                           .Add<Attr::HasSideEffect>()));
  InstallBuiltin("OutputNewLine", 0,
                 std::move(AttributeBuilder(G)
                           .Add<Attr::IsBuiltin>()
                           .Add<Attr::HasSideEffect>()));

  for(Tok = CurTok();
      Tok == Lexer::TOK_FUNCTION || Tok == Lexer::TOK_PROCEDURE;
      Tok = CurTok()) {
    auto* FuncEnd = ParseFuncDecl();
    if(!FuncEnd) return false;
    InspectFuncNodeUsages(FuncEnd);
  }

  Tok = CurTok();
  if(Tok != Lexer::TOK_L_CUR_BRACKET) {
    Log::E() << "Expecting '{' here\n";
    return false;
  }

  NewLastControlPoint();
  NewLastModified();
  NewLastMemAccess();

  setLastCtrlPoint(FuncNode);
  (void) NextTok();
  std::vector<Node*> FuncBodyStmts;
  if(!ParseStatements(FuncBodyStmts)) return false;
  NodeBuilder<IrOpcode::End> EB(&G, FuncNode);
  EB.AddTerminator(getLastCtrlPoint());
  // must 'wait' after all side effects terminate
  // if there is no consumers
  auto MAE = LastMemAccess.end();
  for(auto& P : LastModified) {
    auto* LastMod = P.second;
    // for non-memory access, if there is no consumers,
    // well, then it's dead code
    if(LastMemAccess.find(LastMod) == MAE)
      EB.AddEffectDep(LastMod);
  }
  auto* EndNode = EB.Build();

  Tok = CurTok();
  if(Tok != Lexer::TOK_R_CUR_BRACKET) {
    Log::E() << "Expecting '}' here\n";
    return false;
  }
  Tok = NextTok();
  if(Tok != Lexer::TOK_DOT) {
    Log::E() << "Expect '.' at the end\n";
    return false;
  }
  (void) NextTok();
  PopSymScope();

  // Add function sub-graph
  SubGraph SG(EndNode);
  G.AddSubRegion(SG);
  (void) NodeBuilder<IrOpcode::FunctionStub>(&G, SG).Build();

  ClearNodeIdxMarker();

  return true;
}

