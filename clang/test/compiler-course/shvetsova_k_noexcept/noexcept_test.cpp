// RUN: %clang_cc1 -load %llvmshlibdir/shvetsova_k_noexcept_func_ClangAST%pluginext -plugin noexcept_plugin -fsyntax-only -fcxx-exceptions %s 2>&1 | FileCheck %s


// CHECK: Function can be marked noexcept: simpleSafeFunction
// CHECK: Function can be marked noexcept: complexSafeFunction
// CHECK: Function can be marked noexcept: safeFunctionWithLoops
// CHECK: Function can be marked noexcept: functionWithConditions
// CHECK: Function can be marked noexcept: functionWithSwitch
// CHECK: Function can be marked noexcept: functionA
// CHECK: Function can be marked noexcept: functionB
// CHECK: Function can be marked noexcept: main
// CHECK: Function can be marked noexcept: TestClass::staticFunction

int simpleSafeFunction() {
    return 42;
}

int complexSafeFunction() {
    int a = 10;
    int b = 20;
    return a + b;
} 

int safeFunctionWithLoops() {
    int sum = 0;
    for(int i = 0; i < 10; ++i) {
        sum += i;
    }
    return sum;
}

int functionWithConditions(int x) {
    if (x > 0) {
        return x;
    } else {
        return -x;
    }
}

int functionWithSwitch(int x) {
    switch(x) {
        case 1: return 10;
        case 2: return 20;
        default: return 0;
    }
}

namespace Outer {
    namespace Inner {
        int namespacedFunction() {
            return 100;
        }
    }
    
    int outerFunction() {
        return Inner::namespacedFunction();
    }
}

int functionA() { return 1; }

int functionB() { return functionA() + 1; }

int main() {
    safeFunctionWithLoops();
    functionWithConditions(10);
    return 0;
}

// CHECK-NOT: Function can be marked noexcept: throwingFunction
int throwingFunction() {
    throw 1;
    return 0;
}

// CHECK-NOT: Function can be marked noexcept: functionWithNestedThrow
void nestedThrowHelper() {
    throw 42;
}

int functionWithNestedThrow() {
    nestedThrowHelper();
    return 0;
}

// CHECK-NOT: Function can be marked noexcept: alreadyNoexcept
int alreadyNoexcept() noexcept {
    return 42;
}

// CHECK-NOT: Function can be marked noexcept: tryCatchFunction
int tryCatchFunction() {
    try {
        throw 1;
    } catch (...) {
        return 0;
    }
    return 42;
}

// CHECK-NOT: Function can be marked noexcept: functionWithRethrow
void functionWithRethrow() {
    try {
        throw 1;
    } catch (...) {
        throw;
    }
}

namespace Outer {
    namespace Inner {
        // CHECK-NOT: Function can be marked noexcept: Outer::Inner::throwingNamespacedFunction
        int throwingNamespacedFunction() {
            throw 200;
            return 0;
        }
    }
}

// CHECK-NOT: Function can be marked noexcept: functionC
int functionC() { throw 1; return 0; }


class TestClass {
public:
    // CHECK-NOT: Function can be marked noexcept: TestClass
    TestClass() {}
    
    // CHECK-NOT: Function can be marked noexcept: ~TestClass
    ~TestClass() {}
    
    // CHECK-NOT: Function can be marked noexcept: virtualFunction
    virtual int virtualFunction() {
        return 42;
    }
    
    // CHECK-NOT: Function can be marked noexcept: operator+
    TestClass operator+(const TestClass& other) const {
        return TestClass();
    }
    
    static int staticFunction() {
        return 100;
    }
};


struct Number {
    int value;
    
    // CHECK-NOT: Function can be marked noexcept: operator+
    Number operator+(const Number& other) const {
        return Number{value + other.value};
    }
    
    // CHECK-NOT: Function can be marked noexcept: operator-
    Number operator-(const Number& other) const {
        return Number{value - other.value};
    }
};


// CHECK-NOT: Function can be marked noexcept: templateFunction
template<typename T>
T templateFunction(T a, T b) {
    return a + b;
}

// CHECK-NOT: Function can be marked noexcept: templateWithThrow
template<typename T>
T templateWithThrow(T a, T b) {
    if (a == 0) throw 1;
    return a + b;
}


// CHECK-NOT: Function can be marked noexcept: declarationOnly
int declarationOnly();

// CHECK-NOT: Function can be marked noexcept: externFunction
extern void externFunction();
