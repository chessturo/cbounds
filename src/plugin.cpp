#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Analysis/CFG.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/Sema/Sema.h"
#include "llvm/Support/raw_ostream.h"
#include <clang/AST/ASTTypeTraits.h>
#include <clang/AST/Expr.h>
#include <clang/ASTMatchers/ASTMatchersInternal.h>
#include <memory>

using namespace clang;
using namespace ast_matchers;

namespace {

class IntLiteralAction : public MatchFinder::MatchCallback {
private:
  unsigned int diag_id;

public:
  IntLiteralAction(unsigned int diag_id) : diag_id(diag_id) {}

  void run(const MatchFinder::MatchResult &mr) override {
    auto &diag = mr.Context->getDiagnostics();
    auto *val = mr.Nodes.getNodeAs<IntegerLiteral>("val");
    // diag.Report(val->getBeginLoc(), diag_id) << val->getSourceRange();
  }
};

class AssignmentAction : public MatchFinder::MatchCallback {
private:
  unsigned int diag_id;

public:
  AssignmentAction(unsigned int diag_id) : diag_id(diag_id) {}

  void run(const MatchFinder::MatchResult &mr) override {
    auto &diag = mr.Context->getDiagnostics();
    auto *val = mr.Nodes.getNodeAs<BinaryOperator>("op");
    // diag.Report(val->getBeginLoc(), diag_id) << val->getSourceRange();
  }
};

class FunctionDeclAction : public MatchFinder::MatchCallback {
private:
  unsigned int linear_diag, nonlinear_diag;

public:
  FunctionDeclAction(unsigned int linear_diag, unsigned int nonlinear_diag)
      : linear_diag(linear_diag), nonlinear_diag(nonlinear_diag) {}

  void run(const MatchFinder::MatchResult &mr) override {
    auto &diag = mr.Context->getDiagnostics();
    auto *decl = mr.Nodes.getNodeAs<FunctionDecl>("decl");
    Stmt *body = decl->getBody();

    std::unique_ptr<CFG> cfg =
        CFG::buildCFG(decl, body, mr.Context, CFG::BuildOptions());

    if (cfg->isLinear()) {
      diag.Report(decl->getBeginLoc(), linear_diag) << decl->getSourceRange();
    } else {
      diag.Report(decl->getBeginLoc(), nonlinear_diag)
          << decl->getSourceRange();
    }

    cfg->front().front().getKind();
    cfg->dump(LangOptions(), true);
  }
};

DeclarationMatcher buildPositiveIntMatcher() {
  return varDecl(hasInitializer(integerLiteral().bind("val")));
}
DeclarationMatcher buildNegativeIntMatcher() {
  return varDecl(hasInitializer(unaryOperator(
      hasOperatorName("-"), hasUnaryOperand(integerLiteral().bind("val")))));
}
StatementMatcher buildIntegerAssignmentMatcher() {
  return binaryOperator(hasOperatorName("="), hasLHS(hasType(isInteger())))
      .bind("op");
}
DeclarationMatcher buildFunctionMatcher() {
  return functionDecl().bind("decl");
}

// We need to hold references to all of these because the ASTConsuer returned
// by CreateASTConsumer hold references to them, so we have to keep them
// alive.
//
// I'm not sure why static lifetime is needed for these pointers, but having
// them in the ASTAction causes clang to segfault.
std::unique_ptr<MatchFinder> matchFinder;
std::unique_ptr<MatchFinder::MatchCallback> act1, act2, act3, act4;

class BoundsCheckAction : public PluginASTAction {
protected:
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                 llvm::StringRef) override {
    assert(CI.hasASTContext());

    auto &diag = CI.getASTContext().getDiagnostics();
    unsigned int diag_id =
        diag.getCustomDiagID(DiagnosticsEngine::Warning, "match detected");
    unsigned int linear_id = diag.getCustomDiagID(DiagnosticsEngine::Warning,
                                                  "linear function detected");
    unsigned int nonlinear_id = diag.getCustomDiagID(
        DiagnosticsEngine::Warning, "nonlinear function detected");

    matchFinder = std::make_unique<MatchFinder>();
    act1 = std::make_unique<IntLiteralAction>(diag_id);
    act2 = std::make_unique<IntLiteralAction>(diag_id);
    act3 = std::make_unique<AssignmentAction>(diag_id);
    act4 = std::make_unique<FunctionDeclAction>(linear_id, nonlinear_id);

    matchFinder->addMatcher(traverse(TK_AsIs, buildPositiveIntMatcher()),
                            act1.get());
    matchFinder->addMatcher(traverse(TK_AsIs, buildNegativeIntMatcher()),
                            act2.get());
    matchFinder->addMatcher(traverse(TK_AsIs, buildIntegerAssignmentMatcher()),
                            act3.get());
    matchFinder->addMatcher(traverse(TK_AsIs, buildFunctionMatcher()),
                            act4.get());

    return matchFinder->newASTConsumer();
  }

  bool ParseArgs(const CompilerInstance &CI,
                 const std::vector<std::string> &args) override {
    return true;
  }
};

} // namespace

static FrontendPluginRegistry::Add<BoundsCheckAction> X("cbounds",
                                                        "Check array accesses");

