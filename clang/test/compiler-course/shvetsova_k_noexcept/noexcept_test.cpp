// RUN: %clang_cc1 -load %llvmshlibdir/shvetsova_k_noexcept_func_ClangAST%pluginext -plugin noexcept_plugin -fsyntax-only -fcxx-exceptions %s 2>&1 | FileCheck %s

int square(int value){
  return value * value;
}

bool isEven(int value){
  return value % 2 == 0;
}

int already(int value) noexcept {
  return value + 1;
}

void throwing(){
  throw 1;
}

// CHECK: {{^int square\(int value\) noexcept\{$}}
// CHECK-NEXT: {{^ *return value \* value;$}}
// CHECK-NEXT: {{^}$}}

// CHECK: {{^bool isEven\(int value\) noexcept\{$}}
// CHECK-NEXT: {{^ *return value % 2 == 0;$}}
// CHECK-NEXT: {{^}$}}

// CHECK: {{^int already\(int value\) noexcept \{$}}
// CHECK-NEXT: {{^ *return value \+ 1;$}}
// CHECK-NEXT: {{^}$}}

// CHECK: {{^void throwing\(\)\{$}}
// CHECK-NEXT: {{^ *throw 1;$}}
// CHECK-NEXT: {{^}$}}