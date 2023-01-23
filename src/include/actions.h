#include <fstream>

#include <clang/AST/Decl.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>

using namespace clang;
using namespace clang::ast_matchers;

/**
 * Represents an abstract MatchCallback. Used to couple the Matcher to the
 * callback it's intended to be used with. This is important because AST Nodes
 * can be `bind`'d to particular names; if this is expected by a callback but
 * not done by the `Matcher`, weirdness can result.
 *
 * @typename T type of matcher used for this callback (e.g., Decl for
 * DeclarationMatcher)
 */
template<typename T>
class AbstractAction : public clang::ast_matchers::MatchFinder::MatchCallback {
public:
  virtual void run(const MatchFinder::MatchResult &mr) override = 0;
  virtual T matcher() = 0;
};

/**
 * Produces a GraphVIZ file that repreesents the CFGs of all FunctionDecls in
 * the translation unit.
 */
class VisualizeCFGAction : public AbstractAction<DeclarationMatcher> {
private:
  std::fstream file;
public:
  /**
   * @arg file_name Where to output the graphviz file
   */
  VisualizeCFGAction(std::string file_name) : file(file_name, std::ios::out) { }

  virtual void run(const MatchFinder::MatchResult &mr) override;
  virtual DeclarationMatcher matcher() override;
};

