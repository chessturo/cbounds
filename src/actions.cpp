#include "actions.h"

#include <sstream>

#include <clang/Analysis/CFG.h>
#include <clang/AST/Decl.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>

void VisualizeCFGAction::run(const MatchFinder::MatchResult& mr)  {
  auto* decl = mr.Nodes.getNodeAs<FunctionDecl>("decl");
  Stmt* body = decl->getBody();

  std::unique_ptr<CFG> cfg =
      CFG::buildCFG(decl, body, mr.Context, CFG::BuildOptions());

  this->file << "digraph g {";

  std::ostringstream edges = std::ostringstream();
  for (auto itr = cfg->begin(); itr != cfg->end(); ++itr) {
    CFGBlock* curr = *itr;

    curr->dump();

    int lbl = curr->getBlockID();
    this->file << "\"" << lbl << "\" [ ];" << std::endl;
    for (auto succ_itr = curr->succ_begin();
        succ_itr != curr->succ_end();
        ++succ_itr) {
      if(succ_itr->getReachableBlock()) {
        this->file << "\"" << lbl << "\" -> " << "\"" <<
          succ_itr->getReachableBlock()->BlockID << "\"" << std::endl;
      }
    }
  }

  this->file << edges.str() << "}" << std::endl;
}

DeclarationMatcher VisualizeCFGAction::matcher() {
  return functionDecl().bind("decl");
}
