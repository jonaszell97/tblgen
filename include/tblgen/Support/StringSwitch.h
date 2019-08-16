
#ifndef TBLGEN_STRINGSWITCH_H
#define TBLGEN_STRINGSWITCH_H

#include <cassert>
#include <cstring>

namespace tblgen {

template<typename T, typename R = T>
class StringSwitch {
  /// The string we are matching.
  std::string Str;

  /// The pointer to the result of this switch statement, once known,
  /// null before that.
  Optional<T> Result;

public:
  LLVM_ATTRIBUTE_ALWAYS_INLINE
  explicit StringSwitch(std::string_view S)
  : Str(S), Result() { }

  // StringSwitch is not copyable.
  StringSwitch(const StringSwitch &) = delete;

  // StringSwitch is not assignable due to 'Str' being 'const'.
  void operator=(const StringSwitch &) = delete;
  void operator=(StringSwitch &&other) = delete;

  StringSwitch(StringSwitch &&other)
    : Str(other.Str), Result(std::move(other.Result)) { }

  ~StringSwitch() = default;

  // Case-sensitive case matchers
  LLVM_ATTRIBUTE_ALWAYS_INLINE
  StringSwitch &Case(llvm::StringLiteral S, T Value) {
    if (!Result && Str == S) {
      Result = std::move(Value);
    }
    return *this;
  }

  LLVM_ATTRIBUTE_ALWAYS_INLINE
  StringSwitch& EndsWith(llvm::StringLiteral S, T Value) {
    if (!Result && Str.endswith(S)) {
      Result = std::move(Value);
    }
    return *this;
  }

  LLVM_ATTRIBUTE_ALWAYS_INLINE
  StringSwitch& StartsWith(llvm::StringLiteral S, T Value) {
    if (!Result && Str.startswith(S)) {
      Result = std::move(Value);
    }
    return *this;
  }

  LLVM_ATTRIBUTE_ALWAYS_INLINE
  StringSwitch &Cases(llvm::StringLiteral S0, llvm::StringLiteral S1, T Value) {
    return Case(S0, Value).Case(S1, Value);
  }

  LLVM_ATTRIBUTE_ALWAYS_INLINE
  StringSwitch &Cases(llvm::StringLiteral S0, llvm::StringLiteral S1, llvm::StringLiteral S2,
                      T Value) {
    return Case(S0, Value).Cases(S1, S2, Value);
  }

  LLVM_ATTRIBUTE_ALWAYS_INLINE
  StringSwitch &Cases(llvm::StringLiteral S0, llvm::StringLiteral S1, llvm::StringLiteral S2,
                      llvm::StringLiteral S3, T Value) {
    return Case(S0, Value).Cases(S1, S2, S3, Value);
  }

  LLVM_ATTRIBUTE_ALWAYS_INLINE
  StringSwitch &Cases(llvm::StringLiteral S0, llvm::StringLiteral S1, llvm::StringLiteral S2,
                      llvm::StringLiteral S3, llvm::StringLiteral S4, T Value) {
    return Case(S0, Value).Cases(S1, S2, S3, S4, Value);
  }

  LLVM_ATTRIBUTE_ALWAYS_INLINE
  StringSwitch &Cases(llvm::StringLiteral S0, llvm::StringLiteral S1, llvm::StringLiteral S2,
                      llvm::StringLiteral S3, llvm::StringLiteral S4, llvm::StringLiteral S5,
                      T Value) {
    return Case(S0, Value).Cases(S1, S2, S3, S4, S5, Value);
  }

  LLVM_ATTRIBUTE_ALWAYS_INLINE
  StringSwitch &Cases(llvm::StringLiteral S0, llvm::StringLiteral S1, llvm::StringLiteral S2,
                      llvm::StringLiteral S3, llvm::StringLiteral S4, llvm::StringLiteral S5,
                      llvm::StringLiteral S6, T Value) {
    return Case(S0, Value).Cases(S1, S2, S3, S4, S5, S6, Value);
  }

  LLVM_ATTRIBUTE_ALWAYS_INLINE
  StringSwitch &Cases(llvm::StringLiteral S0, llvm::StringLiteral S1, llvm::StringLiteral S2,
                      llvm::StringLiteral S3, llvm::StringLiteral S4, llvm::StringLiteral S5,
                      llvm::StringLiteral S6, llvm::StringLiteral S7, T Value) {
    return Case(S0, Value).Cases(S1, S2, S3, S4, S5, S6, S7, Value);
  }

  LLVM_ATTRIBUTE_ALWAYS_INLINE
  StringSwitch &Cases(llvm::StringLiteral S0, llvm::StringLiteral S1, llvm::StringLiteral S2,
                      llvm::StringLiteral S3, llvm::StringLiteral S4, llvm::StringLiteral S5,
                      llvm::StringLiteral S6, llvm::StringLiteral S7, llvm::StringLiteral S8,
                      T Value) {
    return Case(S0, Value).Cases(S1, S2, S3, S4, S5, S6, S7, S8, Value);
  }

  LLVM_ATTRIBUTE_ALWAYS_INLINE
  StringSwitch &Cases(llvm::StringLiteral S0, llvm::StringLiteral S1, llvm::StringLiteral S2,
                      llvm::StringLiteral S3, llvm::StringLiteral S4, llvm::StringLiteral S5,
                      llvm::StringLiteral S6, llvm::StringLiteral S7, llvm::StringLiteral S8,
                      llvm::StringLiteral S9, T Value) {
    return Case(S0, Value).Cases(S1, S2, S3, S4, S5, S6, S7, S8, S9, Value);
  }

  // Case-insensitive case matchers.
  LLVM_ATTRIBUTE_ALWAYS_INLINE
  StringSwitch &CaseLower(llvm::StringLiteral S, T Value) {
    if (!Result && Str.equals_lower(S))
      Result = std::move(Value);

    return *this;
  }

  LLVM_ATTRIBUTE_ALWAYS_INLINE
  StringSwitch &EndsWithLower(llvm::StringLiteral S, T Value) {
    if (!Result && Str.endswith_lower(S))
      Result = Value;

    return *this;
  }

  LLVM_ATTRIBUTE_ALWAYS_INLINE
  StringSwitch &StartsWithLower(llvm::StringLiteral S, T Value) {
    if (!Result && Str.startswith_lower(S))
      Result = std::move(Value);

    return *this;
  }

  LLVM_ATTRIBUTE_ALWAYS_INLINE
  StringSwitch &CasesLower(llvm::StringLiteral S0, llvm::StringLiteral S1, T Value) {
    return CaseLower(S0, Value).CaseLower(S1, Value);
  }

  LLVM_ATTRIBUTE_ALWAYS_INLINE
  StringSwitch &CasesLower(llvm::StringLiteral S0, llvm::StringLiteral S1, llvm::StringLiteral S2,
                           T Value) {
    return CaseLower(S0, Value).CasesLower(S1, S2, Value);
  }

  LLVM_ATTRIBUTE_ALWAYS_INLINE
  StringSwitch &CasesLower(llvm::StringLiteral S0, llvm::StringLiteral S1, llvm::StringLiteral S2,
                           llvm::StringLiteral S3, T Value) {
    return CaseLower(S0, Value).CasesLower(S1, S2, S3, Value);
  }

  LLVM_ATTRIBUTE_ALWAYS_INLINE
  StringSwitch &CasesLower(llvm::StringLiteral S0, llvm::StringLiteral S1, llvm::StringLiteral S2,
                           llvm::StringLiteral S3, llvm::StringLiteral S4, T Value) {
    return CaseLower(S0, Value).CasesLower(S1, S2, S3, S4, Value);
  }

  LLVM_NODISCARD
  LLVM_ATTRIBUTE_ALWAYS_INLINE
  R Default(T Value) {
    if (Result)
      return std::move(*Result);
    return Value;
  }

  LLVM_NODISCARD
  LLVM_ATTRIBUTE_ALWAYS_INLINE
  operator R() {
    assert(Result && "Fell off the end of a string-switch");
    return std::move(*Result);
  }
};

} // namespace tblgen

#endif //TBLGEN_STRINGSWITCH_H
