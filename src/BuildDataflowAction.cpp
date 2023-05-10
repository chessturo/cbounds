#include "BuildDataflowAction.h"

#include <deque>

#include <clang/Analysis/CFG.h>
#include <clang/AST/StmtVisitor.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <limits>
#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/IntervalMap.h>
#include <unordered_map>

using namespace clang;
using namespace clang::ast_matchers;

namespace cbounds {

namespace internal {

ValueSet::ValueSet() { this->sign = Sign::BOT; }

ValueSet::ValueSet(int val) {
  if (val == 0) this->sign = Sign::ZERO;
  else if (val > 0) this->sign = Sign::POSITIVE;
  else this->sign = Sign::NEGATIVE;
}

ValueSet::ValueSet(Sign s) {
  this->sign = s;
}

ValueSet::ValueSet(ValueSet v1, ValueSet v2) {
  if (v1.sign == v2.sign) this->sign = v1.sign;
  else this->sign = Sign::TOP;
}

using Sign = ValueSet::Sign;

ValueSet operator+(const ValueSet& lhs, const ValueSet& rhs) {
  if (lhs.sign == Sign::BOT || rhs.sign == Sign::BOT)
    return Sign::BOT;
  if (lhs.sign == Sign::TOP || rhs.sign == Sign::TOP)
    return Sign::TOP;
  if (rhs.sign == Sign::ZERO) return lhs.sign;
  if (lhs.sign == Sign::ZERO) return rhs.sign;

  if (lhs.sign == Sign::POSITIVE && rhs.sign == Sign::POSITIVE)
    return Sign::POSITIVE;
  if (lhs.sign == Sign::NEGATIVE && rhs.sign == Sign::NEGATIVE)
    return Sign::NEGATIVE;

  return Sign::TOP;
}

ValueSet operator-(const ValueSet& lhs, const ValueSet& rhs) {
  if (lhs.sign == Sign::BOT || rhs.sign == Sign::BOT)
    return Sign::BOT;
  if (lhs.sign == Sign::TOP || rhs.sign == Sign::TOP)
    return Sign::TOP;
  if (rhs.sign == Sign::ZERO) return lhs.sign;
  if (lhs.sign == Sign::ZERO) return rhs.sign;

  if (lhs.sign == Sign::POSITIVE && rhs.sign == Sign::NEGATIVE)
    return Sign::POSITIVE;
  if (lhs.sign == Sign::NEGATIVE && rhs.sign == Sign::POSITIVE)
    return Sign::NEGATIVE;

  return Sign::TOP;
}

ValueSet operator*(const ValueSet& lhs, const ValueSet& rhs) {
  if (lhs.sign == Sign::BOT || rhs.sign == Sign::BOT)
    return Sign::BOT;
  if (lhs.sign == Sign::TOP || rhs.sign == Sign::TOP)
    return Sign::TOP;
  if (rhs.sign == Sign::ZERO || lhs.sign == Sign::ZERO) return Sign::ZERO;

  if (lhs.sign == Sign::POSITIVE && rhs.sign == Sign::POSITIVE)
    return Sign::POSITIVE;
  if (lhs.sign == Sign::NEGATIVE && rhs.sign == Sign::NEGATIVE)
    return Sign::POSITIVE;

  return Sign::TOP;
}

ValueSet operator/(const ValueSet& lhs, const ValueSet& rhs) {
  // This is integer division, so e.g., 1/2 = 0 so I don't think we can do
  // better than giving up
  return Sign::TOP;
}

ValueSet operator++(const ValueSet& op) {
  switch(op.sign) {
    case Sign::TOP:
      return Sign::TOP;
    case Sign::BOT:
      return Sign::BOT;
    case Sign::ZERO:
    case Sign::POSITIVE:
      return Sign::POSITIVE;
    case Sign::NEGATIVE:
      return Sign::TOP;
  }
}

ValueSet operator--(const ValueSet& op) {
  switch(op.sign) {
    case Sign::TOP:
      return Sign::TOP;
    case Sign::BOT:
      return Sign::BOT;
    case Sign::ZERO:
    case Sign::NEGATIVE:
      return Sign::NEGATIVE;
    case Sign::POSITIVE:
      return Sign::TOP;
  }
}

bool operator==(const ValueSet& lhs, const ValueSet& rhs) {
  return lhs.sign == rhs.sign;
}
bool operator!=(const ValueSet& lhs, const ValueSet& rhs) {
  return lhs.sign != rhs.sign;
}

class DataflowStmtVisitor : public ConstStmtVisitor<DataflowStmtVisitor> {
 private:
  using Ranges = typename llvm::IntervalMap<int, ValueSet>;
  Ranges::Allocator& alloc;

  using DeclToRanges = typename std::unordered_map<const Decl*, Ranges>;
  DeclToRanges& finalized_ranges;

  std::unordered_map<const Decl*, std::pair<int, ValueSet>>& unclosed_valuesets;
  int stmt_idx;
  ValueSet res;
 public:
  DataflowStmtVisitor(
      int stmt_idx,
      std::unordered_map<const Decl*, std::pair<int, ValueSet>>& unclosed_valuesets,
      Ranges::Allocator& alloc,
      DeclToRanges& finalized_ranges
      ) :
    alloc(alloc),
    stmt_idx(stmt_idx),
    unclosed_valuesets(unclosed_valuesets),
    finalized_ranges(finalized_ranges) { }

  void VisitBinAssign(const BinaryOperator* s) {
    if (const DeclRefExpr* lhs = dyn_cast<const DeclRefExpr>(s->getLHS())) {
      const VarDecl* canon_decl =
        cast<const VarDecl>(lhs->getDecl()->getCanonicalDecl());
      if (this->unclosed_valuesets.count(canon_decl) != 0) {
        auto old_val = this->unclosed_valuesets.at(canon_decl);
        if (this->finalized_ranges.count(canon_decl) == 0) {
          this->finalized_ranges.emplace(canon_decl, this->alloc);
        }
        this->finalized_ranges
          .at(canon_decl)
          .insert(old_val.first, this->stmt_idx - 1, old_val.second);
        this->Visit(s->getRHS());
        this->unclosed_valuesets[canon_decl] =
          std::make_pair(this->stmt_idx, this->res);
      }
    }
  }

  void VisitBinAdd(const BinaryOperator* s) {
    this->Visit(s->getLHS());
    ValueSet lhs = this->res;
    this->Visit(s->getRHS());
    ValueSet rhs = this->res;
    this->res = lhs + rhs;
  }
  void VisitBinSub(const BinaryOperator* s) {
    this->Visit(s->getLHS());
    ValueSet lhs = this->res;
    this->Visit(s->getRHS());
    ValueSet rhs = this->res;
    this->res = lhs - rhs;
  }
  void VisitBinMul(const BinaryOperator* s) {
    this->Visit(s->getLHS());
    ValueSet lhs = this->res;
    this->Visit(s->getRHS());
    ValueSet rhs = this->res;
    this->res = lhs * rhs;
  }
  void VisitBinDiv(const BinaryOperator* s) {
    this->Visit(s->getLHS());
    ValueSet lhs = this->res;
    this->Visit(s->getRHS());
    ValueSet rhs = this->res;
    this->res = lhs / rhs;
  }
  void VisitIntegerLiteral(const IntegerLiteral* s) {
    this->res = ValueSet(s->getValue().getLimitedValue());
  }
  void VisitDeclRefExpr(const DeclRefExpr* s) {
    const Decl* canon_decl = s->getDecl()->getCanonicalDecl();
    if (this->unclosed_valuesets.count(canon_decl) == 0) {
      // Must be a global/static variable
      this->res = ValueSet(Sign::TOP);
    } else {
    this->res = (this->unclosed_valuesets.at(canon_decl)).second;
    }
  }
  void VisitCallExpr(const CallExpr* s) {
    for (int i = 0; i < s->getNumArgs(); i++) {
      this->Visit(s->getArgs()[i]);
    }
  }
  void VisitUnaryAddrOf(const UnaryOperator* s) {
    if (auto* target = dyn_cast<DeclRefExpr>(s->getSubExpr())) {
      if (auto* vd = dyn_cast<VarDecl>(target->getDecl())) {
        auto* canon_decl = vd->getCanonicalDecl();
        if (this->unclosed_valuesets.count(canon_decl) != 0) {
          auto old_val = this->unclosed_valuesets.at(canon_decl);
          if (this->finalized_ranges.count(canon_decl) == 0) {
            this->finalized_ranges.emplace(canon_decl, this->alloc);
          }
          this->finalized_ranges
            .at(canon_decl)
            .insert(old_val.first, this->stmt_idx - 1, old_val.second);
          this->unclosed_valuesets[canon_decl] =
            std::make_pair(this->stmt_idx, ValueSet(Sign::TOP));
        }
      }
    }
  }
  void VisitExpr(const Expr* s) {
    // Fallback, we don't know what this expr thing does.
    this->res = ValueSet(Sign::TOP);
  }

  void VisitBinAddAssign(const BinaryOperator* s) {
    llvm::errs() << "FIXME binop\n";
  }
  void VisitBinSubAssign(const BinaryOperator* s) {
    llvm::errs() << "FIXME binop\n";
  }
  void VisitBinMulAssign(const BinaryOperator* s) {
    llvm::errs() << "FIXME binop\n";
  }
  void VisitBinDivAssign(const BinaryOperator* s) {
    llvm::errs() << "FIXME binop\n";
  }

  void VisitDeclStmt(const DeclStmt* s) {
    for (auto f : s->decls()) {
      if (const VarDecl* f_vd = dyn_cast<const VarDecl>(f)) {
        const VarDecl* cvd = f_vd->getCanonicalDecl();
        if (cvd->getType()->isIntegerType()) {
          assert(this->unclosed_valuesets.count(cvd) == 0);
          this->res = ValueSet(Sign::BOT);
          if (cvd->hasInit()) {
            this->Visit(cvd->getInit());
          }
          this->unclosed_valuesets[cvd] =
            std::make_pair(this->stmt_idx, this->res);
        }
      }
    }
  }

};

using Ranges = typename llvm::IntervalMap<int, ValueSet>;
using DeclToRanges = typename std::unordered_map<const Decl*, Ranges>;
using CFGBlocktoRanges = typename std::unordered_map<CFGBlock*, DeclToRanges>;
void dumpDataflow(const CFGBlocktoRanges& cfg_dataflow) {
  auto& stream = llvm::errs();
  for (auto& blk_to_decl : cfg_dataflow) {
    stream << "[" << blk_to_decl.first << "]\n";
    for (auto& decl_to_rng : blk_to_decl.second) {
      const VarDecl* vd = cast<VarDecl>(decl_to_rng.first);
      stream << "\t" << vd->getName() << "\n";
      for (auto it = decl_to_rng.second.begin(); it != decl_to_rng.second.end(); ++it) {
        stream << "\t\t" << it.start() << " -- " << it.stop() << ": ";
        switch(it.value().sign) {
          case Sign::POSITIVE: stream << "POSITIVE"; break;
          case Sign::NEGATIVE: stream << "NEGATIVE"; break;
          case Sign::ZERO: stream << "ZERO"; break;
          case Sign::TOP: stream << "TOP"; break;
          case Sign::BOT: stream << "BOT"; break;
        }
        stream << "\n";
      }
    }
  }
}

} // namespace cbounds::internal

DeclarationMatcher BuildDataflowAction::matcher() {
  return functionDecl(isDefinition()).bind("func");
}


void BuildDataflowAction::run(const MatchFinder::MatchResult& mr) {
  const FunctionDecl* decl = mr.Nodes.getNodeAs<FunctionDecl>("func");
  Stmt* body = decl->getBody();
  std::unique_ptr<CFG> cfg =
      CFG::buildCFG(decl, body, mr.Context, CFG::BuildOptions());
  CFGBlocktoRanges cfg_dataflow;
  dumpDataflow(cfg_dataflow);
  // IDEA: worklist approach.
  // - Each CFGBlock is added to a worklist, processed, and then readded if any
  //   predecessor blocks are still in the worklist *or* if the block's results
  //   changed
  // - Algorithm terminates when worklist is empty
  // - Convergence is guaranteed because eventually everything will just be TOP
  //   in the worst case
  //
  // FIXME: We can do a lot better performance wise, since we might not be able
  // to make progress on descendent nodes until we've finished processing the
  // ancesors. If this presents a bottleneck, we could use some kind of DAG to
  // guide the choice of which node to process next rather than just going
  // round-robin

  std::deque<CFGBlock*> worklist(cfg->begin(), cfg->end());

  while (!worklist.empty()) {
    CFGBlock* curr_blk = worklist.front(); worklist.pop_front();
    DeclToRanges curr_decl_ranges;

    // This keeps track of the "unclosed valuesets" as we traverse through the
    // statements (e.g., a_var is currently POSITIVE since idx 4, and we haven't
    // seen anything to the contrary yet)
    std::unordered_map<const Decl*, std::pair<int, ValueSet>> unclosed_valuesets;

    for (auto pred : curr_blk->preds()) {
      CFGBlock* pred_blk = pred.getReachableBlock();
      auto pred_ranges_itr = cfg_dataflow.find(pred_blk);
      if (pred_ranges_itr == cfg_dataflow.end()) {
        cfg_dataflow.insert(std::pair(pred_blk, DeclToRanges()));
      } else {
        DeclToRanges& pred_ranges = pred_ranges_itr->second;
        for (auto& pair : pred_ranges) {
          const Decl* decl = pair.first;
          auto& map = pair.second;
          if (unclosed_valuesets.count(decl) != 0) {
            // If we already have information about decl in curr_ranges, update
            // it to be the join of what's in map and what's already there.
            unclosed_valuesets.at(decl) = std::pair(
                std::numeric_limits<int>::min(),
                ValueSet(
                  *map.find(std::numeric_limits<int>::max()),
                  unclosed_valuesets.at(decl).second
                  ));
          } else {
            unclosed_valuesets[decl] = std::pair(
                std::numeric_limits<int>::min(),
                *map.find(std::numeric_limits<int>::max())
                );
          }
        }
      }
    }

    // TODO build the most accurate current value set (i.e., populate
    // cfg_dataflow[curr])

    for (CFGBlock::CFGElementRef curr_cfg_elt : curr_blk->refs()) {
      switch (curr_cfg_elt->getKind()) {
        case clang::CFGElement::Statement: {
          CFGStmt curr_cfg_stmt = curr_cfg_elt->castAs<CFGStmt>();
          const Stmt* curr_stmt = curr_cfg_stmt.getStmt();
          int idx = curr_cfg_elt.getIndexInBlock();
          internal::DataflowStmtVisitor dfsv(
              idx, unclosed_valuesets, this->alloc, curr_decl_ranges);
          dfsv.Visit(curr_stmt);
          break;
        }
        default:
          llvm::errs() << "Unexpected cfg element kind\n";
          curr_cfg_elt.dump();
          assert(false);
      }
    }
    // Close all the unclosed ValueSets
    for (auto pair : unclosed_valuesets) {
      if (curr_decl_ranges.count(pair.first) == 0) {
        curr_decl_ranges.emplace(pair.first, this->alloc);
      }
      curr_decl_ranges.at(pair.first)
        .insert(
            pair.second.first,
            std::numeric_limits<int>::max(),
            pair.second.second);
    }
    cfg_dataflow[curr_blk] = std::move(curr_decl_ranges);

    // TODO also need to readd to worklist if the analysis for this block
    // changed
    for (auto pred : curr_blk->preds()) {
      CFGBlock* pred_blk = pred.getReachableBlock();

      // If the current block is still in the worklist, that doesn't affect
      // convergence unless *other* predecessors still haven't converged
      if (pred_blk == curr_blk) continue;

      if (std::find(worklist.begin(), worklist.end(), pred_blk) != worklist.end()) {
        // Predecessors still haven't converged, so we'll need to revisit this
        // block
        worklist.push_back(curr_blk);
        break;
      }
    }
  }
}

} // namespace cbounds

