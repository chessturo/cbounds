#ifndef CBOUNDS_SRC_INCLUDE_BUILDDATAFLOWACTION_H_
#define CBOUNDS_SRC_INCLUDE_BUILDDATAFLOWACTION_H_

#include <optional>
#include <variant>

#include <clang/Analysis/CFG.h>
#include <clang/AST/Expr.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <llvm/ADT/IntervalMap.h>

#include "AbstractAction.h"

namespace cbounds {

using namespace clang;
using namespace clang::ast_matchers;

namespace internal {

struct ValueSet {
  enum struct Sign {
    POSITIVE,
    NEGATIVE,
    ZERO,

    TOP,
    BOT
  };
  Sign sign;

  ValueSet();
  ValueSet(int val);
  ValueSet(Sign s);
  // Creates a ValueSet that represents the join of v1 and v2
  ValueSet(ValueSet v1, ValueSet v2);
};

ValueSet operator+(const ValueSet& lhs, const ValueSet& rhs);
ValueSet operator-(const ValueSet& lhs, const ValueSet& rhs);
ValueSet operator*(const ValueSet& lhs, const ValueSet& rhs);
ValueSet operator/(const ValueSet& lhs, const ValueSet& rhs);

ValueSet operator++(const ValueSet& op);
ValueSet operator--(const ValueSet& op);

bool operator==(const ValueSet& lhs, const ValueSet& rhs);
bool operator!=(const ValueSet& lhs, const ValueSet& rhs);

} // namespace cbounds::internal

class BuildDataflowAction : public AbstractAction<DeclarationMatcher> {
  using ValueSet = typename cbounds::internal::ValueSet;
  using Ranges = typename llvm::IntervalMap<int, ValueSet>;
  // For a particular Decl*, maps a range of indicies into the corresponding
  // CFGBlock to the value set assumed by that Decl for those statements.
  using DeclToRanges = typename std::unordered_map<const Decl*, Ranges>;
  using CFGBlocktoRanges = typename std::unordered_map<CFGBlock*, DeclToRanges>;
private:
  Ranges::Allocator alloc;
public:
  virtual void run(const MatchFinder::MatchResult& mr) override;
  virtual DeclarationMatcher matcher() override;
};

} // namespace cbounds

#endif // CBOUNDS_SRC_INCLUDE_BUILDDATAFLOWACTION_H_

