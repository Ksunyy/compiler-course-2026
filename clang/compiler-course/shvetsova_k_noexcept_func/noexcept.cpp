#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ExprCXX.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/Type.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/Lex/Lexer.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/Support/raw_ostream.h"

#include <vector>

namespace {

class NoexceptScanner final
    : public clang::RecursiveASTVisitor<NoexceptScanner> {
public:
  NoexceptScanner(const clang::FunctionDecl *current,
                  const llvm::DenseMap<const clang::FunctionDecl *, bool> &safe)
      : m_current(current), m_safe(safe) {}

  bool VisitCXXThrowExpr(clang::CXXThrowExpr *) {
    m_noexcept = false;
    return false;
  }

  bool VisitCXXTryStmt(clang::CXXTryStmt *) {
    m_noexcept = false;
    return false;
  }

  bool VisitCXXConstructExpr(clang::CXXConstructExpr *) {
    m_noexcept = false;
    return false;
  }

  bool VisitCallExpr(clang::CallExpr *call) {
    const auto *callee = call->getDirectCallee();
    if (!callee) {
      m_noexcept = false;
      return false;
    }

    if (callee == m_current)
      return true;

    const auto it = m_safe.find(callee);
    if (it != m_safe.end() && it->second)
      return true;

    const auto *fpt = callee->getType()->getAs<clang::FunctionProtoType>();
    if (fpt && fpt->hasNoexceptExceptionSpec() && !fpt->getNoexceptExpr())
      return true;

    m_noexcept = false;
    return false;
  }

  bool isNoexcept() const { return m_noexcept; }

private:
  const clang::FunctionDecl *m_current;
  const llvm::DenseMap<const clang::FunctionDecl *, bool> &m_safe;
  bool m_noexcept = true;
};

class NoexceptVisitor final
    : public clang::RecursiveASTVisitor<NoexceptVisitor> {
public:
  explicit NoexceptVisitor(std::vector<clang::FunctionDecl *> &functions)
      : m_functions(functions) {}

  bool VisitFunctionDecl(clang::FunctionDecl *func) {
    if (func && func->hasBody() && !func->isImplicit())
      m_functions.push_back(func);
    return true;
  }

private:
  std::vector<clang::FunctionDecl *> &m_functions;
};

class NoexceptConsumer final : public clang::ASTConsumer {
public:
  NoexceptConsumer(clang::ASTContext &context, clang::Rewriter &rewriter)
      : m_context(context), m_rewriter(rewriter), m_visitor(m_functions) {}

  void HandleTranslationUnit(clang::ASTContext &context) override {
    m_functions.clear();
    m_safe.clear();

    m_visitor.TraverseDecl(context.getTranslationUnitDecl());

    for (auto *func : m_functions) {
      if (isAlreadyNoexcept(func))
        m_safe[func] = true;
    }

    bool changed = true;
    while (changed) {
      changed = false;
      for (auto *func : m_functions) {
        if (m_safe[func])
          continue;
        if (functionDoesNotThrow(func)) {
          m_safe[func] = true;
          changed = true;
        }
      }
    }

    for (auto *func : m_functions) {
      if (!m_safe[func])
        continue;
      if (isAlreadyNoexcept(func))
        continue;
      addNoexcept(func);
    }

    emitRewrittenSource();
  }

private:
  bool isAlreadyNoexcept(const clang::FunctionDecl *func) const {
    const auto *fpt = func->getType()->getAs<clang::FunctionProtoType>();
    if (!fpt)
      return false;

    if (fpt->getExceptionSpecType() == clang::EST_DynamicNone)
      return true;

    if (fpt->hasNoexceptExceptionSpec() && !fpt->getNoexceptExpr())
      return true;

    return false;
  }

  bool functionDoesNotThrow(const clang::FunctionDecl *func) const {
    const clang::Stmt *body = func->getBody();
    if (!body)
      return false;

    NoexceptScanner scanner(func, m_safe);
    scanner.TraverseStmt(const_cast<clang::Stmt *>(body));
    return scanner.isNoexcept();
  }

  void addNoexcept(const clang::FunctionDecl *func) {
    const auto *body = clang::dyn_cast<clang::CompoundStmt>(func->getBody());
    if (!body)
      return;

    clang::SourceLocation braceLoc = body->getLBracLoc();
    braceLoc = m_context.getSourceManager().getSpellingLoc(braceLoc);

    if (braceLoc.isInvalid())
      return;

    if (!m_context.getSourceManager().isWrittenInMainFile(braceLoc))
      return;

    m_rewriter.InsertTextBefore(braceLoc, " noexcept");
  }

  void emitRewrittenSource() {
    clang::SourceManager &sm = m_rewriter.getSourceMgr();
    const clang::FileID mainFile = sm.getMainFileID();

    if (const auto *rewriteBuf = m_rewriter.getRewriteBufferFor(mainFile)) {
      rewriteBuf->write(llvm::outs());
      return;
    }

    llvm::outs() << sm.getBufferData(mainFile);
  }

private:
  clang::ASTContext &m_context;
  clang::Rewriter &m_rewriter;
  std::vector<clang::FunctionDecl *> m_functions;
  llvm::DenseMap<const clang::FunctionDecl *, bool> m_safe;
  NoexceptVisitor m_visitor;
};

class NoexceptAction final : public clang::PluginASTAction {
public:
  std::unique_ptr<clang::ASTConsumer>
  CreateASTConsumer(clang::CompilerInstance &ci, llvm::StringRef) override {
    m_rewriter.setSourceMgr(ci.getSourceManager(), ci.getLangOpts());
    return std::make_unique<NoexceptConsumer>(ci.getASTContext(), m_rewriter);
  }

  bool ParseArgs(const clang::CompilerInstance &,
                 const std::vector<std::string> &) override {
    return true;
  }

private:
  clang::Rewriter m_rewriter;
};

} // namespace

static clang::FrontendPluginRegistry::Add<NoexceptAction>
    X("noexcept_plugin", "Add noexcept to functions that do not throw");