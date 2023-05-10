#include <llvm/ADT/IntervalMap.h>
#include <memory>

#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Frontend/FrontendPluginRegistry.h>

#include "BuildDataflowAction.h"
#include "VisualizeCFGAction.h"

using namespace clang;
using namespace ast_matchers;

namespace {

class AssignmentAction : public cbounds::AbstractAction<StatementMatcher> {
private:

public:
  AssignmentAction() {}

  void run(const MatchFinder::MatchResult &mr) override {
    auto *val = mr.Nodes.getNodeAs<BinaryOperator>("op");
    val->dump();
    if (auto *d = dyn_cast<DeclRefExpr>(val->getLHS())) {
      llvm::errs() << "Decl:\n";
      d->getDecl()->dump();
    }
  }
  StatementMatcher matcher() override {
    return binaryOperator(hasOperatorName("="), hasLHS(hasType(isInteger())))
        .bind("op");
  }
};

// We need to hold references to all of these because the ASTConsuer returned
// by CreateASTConsumer hold references to them, so we have to keep them
// alive.
//
// I'm not sure why static lifetime is needed for these pointers, but having
// them in the ASTAction causes clang to segfault.
std::unique_ptr<MatchFinder> matchFinder;
std::unique_ptr<cbounds::BuildDataflowAction> act;

class BoundsCheckAction : public PluginASTAction {
protected:
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                 llvm::StringRef) override {
    assert(CI.hasASTContext());

    matchFinder = std::make_unique<MatchFinder>();
    act = std::make_unique<cbounds::BuildDataflowAction>();
    matchFinder->addMatcher(traverse(TK_AsIs, act->matcher()), act.get());
    
    //XXX REMOVE ME DEBUG
    llvm::IntervalMap<int, std::string>::Allocator alloc;
    llvm::IntervalMap<int, std::string> map(alloc);
    map.insert(-1, 4, "-1 to 4");
    map.insert(5, 6, "5 to 6");
    llvm::errs() << "-1: " << *map.find(-1) << "\n";
    llvm::errs() << "0: " << *map.find(0) << "\n";
    llvm::errs() << "1: " << *map.find(1) << "\n";
    llvm::errs() << "2: " << *map.find(2) << "\n";
    llvm::errs() << "3: " << *map.find(3) << "\n";
    llvm::errs() << "4: " << *map.find(4) << "\n";
    llvm::errs() << "5: " << *map.find(5) << "\n";
    llvm::errs() << "6: " << *map.find(6) << "\n";
    //XXX

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

