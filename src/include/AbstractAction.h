#ifndef CBOUNDS_SRC_INCLUDE_ABSTRACTACTION_H_
#define CBOUNDS_SRC_INCLUDE_ABSTRACTACTION_H_

#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace cbounds {

/**
 * Represents an abstract MatchCallback. Used to couple the Matcher to the
 * callback it's intended to be used with. This is important because AST Nodes
 * can be `bind`'d to particular names; if this is expected by a callback but
 * not done by the `Matcher`, weirdness can result.
 *
 * @typename T type of matcher used for this callback (e.g., DeclarationMatcher
 * for Decl)
 */
template<typename T>
class AbstractAction : public clang::ast_matchers::MatchFinder::MatchCallback {
public:
  virtual void run(
      const clang::ast_matchers::MatchFinder::MatchResult& mr) override = 0;
  virtual T matcher() = 0;
};

} // namespace cbounds

#endif // CBOUNDS_SRC_INCLUDE_ABSTRACTACTION_H_

