; RUN: opt -load-pass-plugin %llvmshlibdir/shvetsova_k_replace_with_shift_LLVM_IR%pluginext\
; RUN: -passes=replaceWithShiftPass -S %s | FileCheck %s

; --- Умножение: Константа справа (val * 8) ---
; CHECK-LABEL: @test_mul_right
define i32 @test_mul_right(i32 %val) {
  ; CHECK: %leftShift = shl i32 %val, 3
  %res = mul i32 %val, 8
  ret i32 %res
}

; --- Умножение: Константа слева (8 * val) ---
; CHECK-LABEL: @test_mul_left
define i32 @test_mul_left(i32 %val) {
  ; CHECK: %leftShift = shl i32 %val, 3
  %res = mul i32 8, %val
  ret i32 %res
}

; --- Деление: Знаковое (SDiv) на 4 ---
; CHECK-LABEL: @test_sdiv
define i32 @test_sdiv(i32 %val) {
  ; CHECK: %arithShiftRight = ashr i32 %val, 2
  %res = sdiv i32 %val, 4
  ret i32 %res
}

; --- Деление: Беззнаковое (UDiv) на 16 ---
; CHECK-LABEL: @test_udiv
define i32 @test_udiv(i32 %val) {
  ; CHECK: %logicalShiftRight = lshr i32 %val, 4
  %res = udiv i32 %val, 16
  ret i32 %res
}

; --- Крайний случай: Умножение на 1 (2^0) ---
; CHECK-LABEL: @test_mul_one
define i32 @test_mul_one(i32 %val) {
  ; CHECK: %leftShift = shl i32 %val, 0
  %res = mul i32 %val, 1
  ret i32 %res
}

; --- Отрицательный тест: Не степень двойки (7) ---
; CHECK-LABEL: @test_mul_not_power_of_2
define i32 @test_mul_not_power_of_2(i32 %val) {
  ; CHECK: %res = mul i32 %val, 7
  ; CHECK-NOT: shl
  %res = mul i32 %val, 7
  ret i32 %res
}