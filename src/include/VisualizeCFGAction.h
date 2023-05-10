#ifndef CBOUNDS_SRC_INCLUDE_VISUALIZECFGACTION_H_
#define CBOUNDS_SRC_INCLUDE_VISUALIZECFGACTION_H_

#include <fstream>

#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>

#include "AbstractAction.h"

namespace cbounds {

/**
 * Produces a GraphVIZ file that repreesents the CFGs of all FunctionDecls in
 * the translation unit.
 */
class VisualizeCFGAction :
  public AbstractAction<clang::ast_matchers::DeclarationMatcher> {
private:
  std::fstream file;
public:
  /**
   * @arg file_name Where to output the graphviz file
   */
  VisualizeCFGAction(std::string file_name) : file(file_name, std::ios::out) { }

  virtual void run(
      const clang::ast_matchers::MatchFinder::MatchResult& mr) override;
  virtual clang::ast_matchers::DeclarationMatcher matcher() override;
};

} // namespace cbounds

#endif // CBOUNDS_SRC_INCLUDE_VISUALIZECFGACTION_H_

