#include "actions.h"

#include <regex>
#include <sstream>

#include <llvm/Support/Casting.h>
#include <llvm/Support/raw_ostream.h>

#include <clang/Analysis/CFG.h>
#include <clang/AST/Decl.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>

void VisualizeCFGAction::run(const MatchFinder::MatchResult& mr)  {
  const FunctionDecl* decl = mr.Nodes.getNodeAs<FunctionDecl>("decl");
  Stmt* body = decl->getBody();

  std::unique_ptr<CFG> cfg =
      CFG::buildCFG(decl, body, mr.Context, CFG::BuildOptions());

  cfg->dump(LangOptions(), true);
  this->file << "digraph g {";

  for (auto itr = cfg->begin(); itr != cfg->end(); ++itr) {
    CFGBlock* curr = *itr;

    // Print the block itself
    int lbl = curr->getBlockID();
    this->file << "\"" << lbl << "\" [";
    if (&cfg->getEntry() != curr && &cfg->getExit() != curr && !curr->empty()) {
      this->file << "label=<<table>";
      std::string stmt_str;
      llvm::raw_string_ostream raw_oss(stmt_str);
      for (auto& elt : *curr) {
        switch(elt.getKind()) {
          case CFGElement::Kind::Statement: {
            CFGStmt cfg_stmt = elt.castAs<CFGStmt>();
            const Stmt* stmt = cfg_stmt.getStmt();

            this->file << "<tr><td>";

            stmt->printPretty(raw_oss, nullptr, PrintingPolicy(LangOptions()));
            stmt_str = std::regex_replace(stmt_str, std::regex("&"), "&amp;");
            stmt_str = std::regex_replace(stmt_str, std::regex("<"), "&lt;");
            stmt_str = std::regex_replace(stmt_str, std::regex(">"), "&gt;");
            this->file << stmt_str;
            stmt_str.clear();

            this->file << "</td></tr>";
            break;
          }
          default:
            assert(false);
        }
      }
      this->file << "</table>>";
    }
    this->file << "];" << std::endl;

    // Print the successors
    if (curr->succ_size() == 1) {
      if (CFGBlock* block = curr->succ_begin()->getReachableBlock()) {
        this->file << "\"" << lbl << "\" -> " << "\"" << block->BlockID << "\""
          << std::endl;
      }
    } else if (curr->succ_size() == 2) {
      auto itr = curr->succ_begin();
      if (CFGBlock* block = itr->getReachableBlock()) {
        this->file << "\"" << lbl << "\" -> " << "\"" << block->BlockID << "\""
          << "[ label = \"true\"]; " << std::endl;
      }
      ++itr;
      if (CFGBlock* block = itr->getReachableBlock()) {
        this->file << "\"" << lbl << "\" -> " << "\"" << block->BlockID << "\""
          << "[ label = \"false\"]; " << std::endl;
      }
    } else {
      assert(&cfg->getExit() == curr);
    }
  }

  this->file << "}" << std::endl;
}

DeclarationMatcher VisualizeCFGAction::matcher() {
  return functionDecl(isDefinition()).bind("decl");
}
