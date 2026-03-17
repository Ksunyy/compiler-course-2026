#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/Type.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "llvm/Support/raw_ostream.h"

namespace {

class ThrowVisitor : public clang::RecursiveASTVisitor<ThrowVisitor> {
public:
  bool hasThrow = false;

  bool VisitCXXThrowExpr(clang::CXXThrowExpr *) {
    hasThrow = true;
    return false;
  }
};

class NoexceptVisitor final
    : public clang::RecursiveASTVisitor<NoexceptVisitor> {

public:
  explicit NoexceptVisitor(clang::ASTContext *context) : context(context) {}

  bool VisitFunctionDecl(clang::FunctionDecl *func) {

    auto &SM = context->getSourceManager();

    if (SM.isInSystemHeader(func->getLocation()))
      return true;

    if (!func->hasBody())
      return true;

    if (llvm::isa<clang::CXXConstructorDecl>(func))
      return true;

    if (llvm::isa<clang::CXXDestructorDecl>(func))
      return true;

    if (func->isOverloadedOperator())
      return true;

    if (auto *method = llvm::dyn_cast<clang::CXXMethodDecl>(func)) {
      if (method->isVirtual())
        return true;
    }

    if (func->isTemplated())
      return true;

    const auto *type = func->getType()->getAs<clang::FunctionProtoType>();

    if (!type)
      return true;

    if (type->isNothrow())
      return true;

    ThrowVisitor throwVisitor;
    throwVisitor.TraverseStmt(func->getBody());

    if (throwVisitor.hasThrow)
      return true;

    llvm::errs() << "Function can be marked noexcept: "
                 << func->getQualifiedNameAsString() << " at ";

    func->getLocation().print(llvm::errs(), SM);
    llvm::errs() << "\n";

    return true;
  }

private:
  clang::ASTContext *context;
};

class NoexceptConsumer final : public clang::ASTConsumer {
public:
  explicit NoexceptConsumer(clang::ASTContext *context) : visitor(context) {}

  void HandleTranslationUnit(clang::ASTContext &context) override {
    visitor.TraverseDecl(context.getTranslationUnitDecl());
  }

private:
  NoexceptVisitor visitor;
};

class NoexceptAction final : public clang::PluginASTAction {
public:
  std::unique_ptr<clang::ASTConsumer>
  CreateASTConsumer(clang::CompilerInstance &CI, llvm::StringRef) override {

    return std::make_unique<NoexceptConsumer>(&CI.getASTContext());
  }

  bool ParseArgs(const clang::CompilerInstance &CI,
                 const std::vector<std::string> &args) override {
    return true;
  }
};

} // namespace

static clang::FrontendPluginRegistry::Add<NoexceptAction>
    X("noexcept_plugin", "Find functions that can be marked noexcept");