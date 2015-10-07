///////////////////////////////////////////////////////////////////////////////
//
// Module      :  purescript.hh
// Copyright   :  (c) Andy Arvanitis 2015
// License     :  MIT
//
// Maintainer  :  Andy Arvanitis <andy.arvanitis@gmail.com>
// Stability   :  experimental
// Portability :
//
// Basic types and functions to support purescript-to-C++1x rendering
//
///////////////////////////////////////////////////////////////////////////////
//
#ifndef PureScript_HH
#define PureScript_HH

// Standard includes

#if defined(DEBUG)
  #include <cassert>
#else
  #define NDEBUG
  #undef assert
  #define assert(x)
#endif

#include <memory>
#include <functional>
#include <string>
#include <vector>
#include <unordered_map>
#include <stdexcept>

#include "map_key.hh"
#include "hash.hh"

namespace PureScript {

using string = std::string;
using runtime_error = std::runtime_error;
using nullptr_t = std::nullptr_t;

// A variant data class designed to provide some features of dynamic typing.
//
class any {

  public:
  enum class Type : std::int8_t {
    Unknown,
    Integer,
    Double,
    Character,
    Boolean,
    String,
    Map,
    Vector,
    Function,
    EffFunction,
    Thunk,
    Pointer
  };

  Type type = Type::Unknown;

  struct as_thunk {
  };
  static constexpr as_thunk unthunk = as_thunk{};

  using map    = std::unordered_map<map_key_t, const any, map_key_t::hasher, map_key_t::equal>;
  using vector = std::vector<any>;
  using fn     = std::function<any(const any&)>;
  using eff_fn = std::function<any()>;
  using thunk  = std::function<const any& (const as_thunk)>;

  using shared_string = std::shared_ptr<string>;
  using shared_map    = std::shared_ptr<map>;
  using shared_vector = std::shared_ptr<vector>;

  using shared_fn     = std::shared_ptr<fn>;
  using shared_eff_fn = std::shared_ptr<eff_fn>;
  using shared_thunk  = std::shared_ptr<thunk>;

  template <typename T>
  using shared = std::shared_ptr<T>;

  using shared_void_ptr = std::shared_ptr<void>;

  template <typename T>
  static constexpr auto make_shared(const T& arg) {
    return std::make_shared<T>(arg);
  }

  template <typename T>
  static constexpr auto make_shared(T&& arg) {
    return std::make_shared<T>(std::move(arg));
  }

  template <typename T, typename U>
  static constexpr auto static_cast_shared(U arg) {
    return static_cast<T*>(arg.get());
  }

  private:
  union {
    long            i;
    double          d;
    char            c;
    bool            b;
    shared_string   s;
    shared_map      m;
    shared_vector   v;
    shared_fn       f;
    shared_eff_fn   e;
    shared_thunk    t;
    shared_void_ptr p;
  };

  public:

  any(const int val) : type(Type::Integer), i(val) {}

  any(const long val) : type(Type::Integer), i(val) {}

  any(const double val) : type(Type::Double), d(val) {}

  any(const char val) : type(Type::Character), c(val) {}

  any(const bool val) : type(Type::Boolean), b(val) {}

  any(const string& val) : type(Type::String), s(make_shared<string>(val)) {}
  any(string&& val) : type(Type::String), s(make_shared<string>(std::move(val))) {}

  any(const char val[]) : type(Type::String), s(make_shared<string>(val)) {}

  any(const shared_string& val) : type(Type::String), s(val) {}
  any(shared_string&& val) : type(Type::String), s(std::move(val)) {}

  any(const map& val) : type(Type::Map), m(make_shared<map>(val)) {}
  any(map&& val) : type(Type::Map), m(make_shared<map>(std::move(val))) {}

  any(const shared_map& val) : type(Type::Map), m(val) {}
  any(shared_map&& val) : type(Type::Map), m(std::move(val)) {}

  any(const vector& val) : type(Type::Vector), v(make_shared<vector>(val)) {}
  any(vector&& val) : type(Type::Vector), v(make_shared<vector>(std::move(val))) {}

  any(const shared_vector& val) : type(Type::Vector), v(val) {}
  any(shared_vector&& val) : type(Type::Vector), v(std::move(val)) {}

  any(const shared_fn& val) : type(Type::Function), f(val) {}
  any(shared_fn&& val) : type(Type::Function), f(std::move(val)) {}

  template <typename T>
  any(const T& val, typename std::enable_if<std::is_assignable<fn,T>::value>::type* = 0)
    : type(Type::Function), f(make_shared<fn>(val)) {}

  // template <typename T>
  // any(T&& val, typename std::enable_if<std::is_assignable<fn,T>::value>::type* = 0)
  //   : type(Type::Function), f(std::move(val)) {}

  any(const shared_eff_fn& val) : type(Type::EffFunction), e(val) {}
  any(shared_eff_fn&& val) : type(Type::EffFunction), e(std::move(val)) {}

  template <typename T>
  any(const T& val, typename std::enable_if<std::is_assignable<eff_fn,T>::value>::type* = 0)
    : type(Type::EffFunction), e(make_shared<eff_fn>(val)) {}

  any(const shared_thunk& val) : type(Type::Thunk), t(val) {}
  any(shared_thunk&& val) : type(Type::Thunk), t(std::move(val)) {}

  template <typename T>
  any(const T& val, typename std::enable_if<std::is_assignable<thunk,T>::value>::type* = 0)
    : type(Type::Thunk), t(make_shared<thunk>(val)) {}

  template <typename T>
  any(const T& val, typename std::enable_if<std::is_assignable<shared_void_ptr,T>::value>::type* = 0)
    : type(Type::Pointer), p(val) {
    }

  any(std::nullptr_t) : type(Type::Pointer), p(nullptr) {}

  template <typename T>
  any(T&& val, typename std::enable_if<std::is_assignable<shared_void_ptr,T>::value>::type* = 0)
    : type(Type::Pointer), p(std::move(val)) {
    }

  any(const any& val) : type(val.type) {
    switch (type) {
      case Type::Integer:         i = val.i;                         break;
      case Type::Double:          d = val.d;                         break;
      case Type::Character:       c = val.c;                         break;
      case Type::Boolean:         b = val.b;                         break;
      case Type::String:          new (&s) shared_string   (val.s);  break;
      case Type::Map:             new (&m) shared_map      (val.m);  break;
      case Type::Vector:          new (&v) shared_vector   (val.v);  break;
      case Type::Function:        new (&f) shared_fn       (val.f);  break;
      case Type::EffFunction:     new (&e) shared_eff_fn   (val.e);  break;
      case Type::Thunk:           new (&t) shared_thunk    (val.t);  break;
      case Type::Pointer:         new (&p) shared_void_ptr (val.p);  break;

      default: assert(not "supported type in copy ctor");
    }
  }

  auto swap(any&& val) {
    type = val.type;
    switch (type) {
      case Type::Integer:         i = val.i;                                    break;
      case Type::Double:          d = val.d;                                    break;
      case Type::Character:       c = val.c;                                    break;
      case Type::Boolean:         b = val.b;                                    break;
      case Type::String:          new (&s) shared_string   (std::move(val.s));  break;
      case Type::Map:             new (&m) shared_map      (std::move(val.m));  break;
      case Type::Vector:          new (&v) shared_vector   (std::move(val.v));  break;
      case Type::Function:        new (&f) shared_fn       (std::move(val.f));  break;
      case Type::EffFunction:     new (&e) shared_eff_fn   (std::move(val.e));  break;
      case Type::Thunk:           new (&t) shared_thunk    (std::move(val.t));  break;
      case Type::Pointer:         new (&p) shared_void_ptr (std::move(val.p));  break;

      default: assert(not "supported type in move ctor");
    }
  }

  any(any&& val) {
    swap(std::move(val));
  }

  auto operator=(any val) -> any& {
    swap(std::move(val));
    return *this;
  }

  any() = delete;
  ~any() {
    // std::cout << "destroy" << std::endl;
    switch (type) {
      case Type::Integer:         ;                     break;
      case Type::Double:          ;                     break;
      case Type::Character:       ;                     break;
      case Type::Boolean:         ;                     break;
      case Type::String:          s.~shared_string();   break;
      case Type::Map:             m.~shared_map();      break;
      case Type::Vector:          v.~shared_vector();   break;
      case Type::Function:        f.~shared_fn();       break;
      case Type::EffFunction:     e.~shared_eff_fn();   break;
      case Type::Thunk:           t.~shared_thunk();    break;
      case Type::Pointer:         p.~shared_void_ptr(); break;

      default: assert(not "supported type in destructor");
    }
  };

  #define RETURN_VALUE(TYPE, VAL, FN) \
    if (type == TYPE) { \
      return FN(VAL); \
    } else { \
      assert(type == Type::Thunk); \
      const any& value = (*t)(unthunk); \
      assert(value.type == TYPE); \
      return FN(value.VAL); \
    }

  template <typename T>
  constexpr auto cast() const -> typename std::enable_if<std::is_same<T, long>::value, T>::type {
    RETURN_VALUE(Type::Integer, i,)
  }

  template <typename T>
  constexpr auto cast() const -> typename std::enable_if<std::is_same<T, double>::value, T>::type {
    RETURN_VALUE(Type::Double, d,)
  }

  template <typename T>
  constexpr auto cast() const -> typename std::enable_if<std::is_same<T, char>::value, T>::type {
    RETURN_VALUE(Type::Character, c,)
  }

  template <typename T>
  constexpr auto cast() const -> typename std::enable_if<std::is_same<T, bool>::value, T>::type {
    RETURN_VALUE(Type::Boolean, b,)
  }

  template <typename T>
  constexpr auto cast() const -> typename std::enable_if<std::is_same<T, string>::value, const T&>::type {
    RETURN_VALUE(Type::String, s, *)
  }

  template <typename T>
  constexpr auto cast() const -> typename std::enable_if<std::is_same<T, map>::value, const T&>::type {
    RETURN_VALUE(Type::Map, m, *)
  }

  template <typename T>
  constexpr auto cast() const -> typename std::enable_if<std::is_same<T, vector>::value, const T&>::type {
    RETURN_VALUE(Type::Vector, v, *)
  }

  template <typename T>
  constexpr auto cast() const ->
      typename std::enable_if<std::is_assignable<shared_void_ptr,T>::value, typename T::element_type*>::type {
    RETURN_VALUE(Type::Pointer, p, static_cast_shared<typename T::element_type>)
  }

  auto operator()(const any arg) const -> any {
    if (type == Type::Function) {
      return (*f)(arg);
    } else {
      assert(type == Type::Thunk);
      const any& value = (*t)(unthunk);
      assert(value.type == Type::Function);
      return (*value.f)(arg);
    }
  }

  auto operator()(const as_thunk) const -> const any& {
    assert(type == Type::Thunk);
    return (*t)(unthunk);
  }

  inline static auto call(const any& a) -> any {
    assert(a.type == Type::EffFunction || a.type == Type::Function);
    if (a.type == Type::EffFunction) {
      return (*a.e)();
    } else {
      return (*a.f)(false);
    }
  }

  auto operator()() const -> any {
    if (type == Type::EffFunction || type == Type::Function) {
      return call(*this);
    } else {
      assert(type == Type::Thunk);
      const any& value = (*t)(unthunk);
      return call(value);
    }
  }

  explicit operator long() const {
    return i;
  }

  explicit operator double() const {
    return d;
  }

  explicit operator bool() const {
    return b;
  }

  operator const string&() const {
    RETURN_VALUE(Type::String, s, *)
  }

  operator const map&() const {
    RETURN_VALUE(Type::Map, m, *)
  }

  operator const vector&() const {
    RETURN_VALUE(Type::Vector, v, *)
  }

  auto extractPointer() const -> const void* {
    RETURN_VALUE(Type::Pointer, p.get(),)
  }

  inline auto operator[](const map_key_t rhs) const -> const any& {
    RETURN_VALUE(Type::Map, m->at(rhs),)
  }

  inline auto operator[](const vector::size_type rhs) const -> const any& {
    RETURN_VALUE(Type::Vector, v->at(rhs),)
  }

  inline auto operator[](const any& rhs) const -> const any& {
    RETURN_VALUE(Type::Vector, v->at(rhs.cast<long>()),)
  }

  #undef RETURN_VALUE

  static constexpr auto extract_value(const any& a) -> const any& {
    return a.type != Type::Thunk ? a : (*a.t)(unthunk);
  }

  #define DEFINE_OPERATOR_RHS(K, T, OP, V, R, P) \
  inline auto operator OP(const T rhs) const -> R { \
    auto& lhs = extract_value(*this); \
    assert(lhs.type == Type::K); \
    return P(lhs.V) OP rhs; \
  }

  inline auto operator==(const any& rhs_) const -> bool {
    auto& lhs = extract_value(*this);
    auto& rhs = extract_value(rhs_);
    assert(lhs.type == rhs.type);
    switch (lhs.type) {
      case Type::Integer:   return lhs.i == rhs.i;
      case Type::Double:    return lhs.d == rhs.d;
      case Type::Character: return lhs.c == rhs.c;
      case Type::Boolean:   return lhs.b == rhs.b;
      case Type::String:    return (*lhs.s) == (*rhs.s);
      case Type::Pointer:   return lhs.p == rhs.p;
      default: assert(not "supported type for '==' operator");
    }
    return false;
  }

  DEFINE_OPERATOR_RHS(Integer,   long,        ==, i, bool,)
  DEFINE_OPERATOR_RHS(Double,    double,      ==, d, bool,)
  DEFINE_OPERATOR_RHS(Character, char,        ==, c, bool,)
  DEFINE_OPERATOR_RHS(Boolean,   bool,        ==, b, bool,)
  DEFINE_OPERATOR_RHS(String,    string&,     ==, s, bool, *)
  DEFINE_OPERATOR_RHS(String,    char* const, ==, s, bool, *)

  inline auto operator!=(const any& rhs_) const -> bool {
    auto& lhs = extract_value(*this);
    auto& rhs = extract_value(rhs_);
    assert(lhs.type == rhs.type);
    switch (lhs.type) {
      case Type::Integer:   return lhs.i != rhs.i;
      case Type::Double:    return lhs.d != rhs.d;
      case Type::Character: return lhs.c != rhs.c;
      case Type::Boolean:   return lhs.b != rhs.b;
      case Type::String:    return (*lhs.s) != (*rhs.s);
      case Type::Pointer:   return lhs.p != rhs.p;
      default: assert(not "supported type for '!=' operator");
    }
    return false;
  }

  DEFINE_OPERATOR_RHS(Integer,   long,        !=, i, bool,)
  DEFINE_OPERATOR_RHS(Double,    double,      !=, d, bool,)
  DEFINE_OPERATOR_RHS(Character, char,        !=, c, bool,)
  DEFINE_OPERATOR_RHS(Boolean,   bool,        !=, b, bool,)
  DEFINE_OPERATOR_RHS(String,    string&,     !=, s, bool, *)
  DEFINE_OPERATOR_RHS(String,    char* const, !=, s, bool, *)

  inline auto operator<(const any& rhs_) const -> bool {
    auto& lhs = extract_value(*this);
    auto& rhs = extract_value(rhs_);
    assert(lhs.type == rhs.type);
    switch (lhs.type) {
      case Type::Integer:   return lhs.i < rhs.i;
      case Type::Double:    return lhs.d < rhs.d;
      case Type::Character: return lhs.c < rhs.c;
      case Type::Boolean:   return lhs.b < rhs.b;
      case Type::String:    return (*lhs.s) < (*rhs.s);
      default: assert(not "supported type for '<' operator");
    }
    return false;
  }

  DEFINE_OPERATOR_RHS(Integer,   long,        <, i, bool,)
  DEFINE_OPERATOR_RHS(Double,    double,      <, d, bool,)
  DEFINE_OPERATOR_RHS(Character, char,        <, c, bool,)
  DEFINE_OPERATOR_RHS(Boolean,   bool,        <, b, bool,)
  DEFINE_OPERATOR_RHS(String,    string&,     <, s, bool, *)
  DEFINE_OPERATOR_RHS(String,    char* const, <, s, bool, *)

  inline auto operator<=(const any& rhs_) const -> bool {
    auto& lhs = extract_value(*this);
    auto& rhs = extract_value(rhs_);
    assert(lhs.type == rhs.type);
    switch (lhs.type) {
      case Type::Integer:   return lhs.i <= rhs.i;
      case Type::Double:    return lhs.d <= rhs.d;
      case Type::Character: return lhs.c <= rhs.c;
      case Type::Boolean:   return lhs.b <= rhs.b;
      case Type::String:    return (*lhs.s) <= (*rhs.s);
      default: assert(not "supported type for '<' operator");
    }
    return false;
  }

  DEFINE_OPERATOR_RHS(Integer,   long,        <=, i, bool,)
  DEFINE_OPERATOR_RHS(Double,    double,      <=, d, bool,)
  DEFINE_OPERATOR_RHS(Character, char,        <=, c, bool,)
  DEFINE_OPERATOR_RHS(Boolean,   bool,        <=, b, bool,)
  DEFINE_OPERATOR_RHS(String,    string&,     <=, s, bool, *)
  DEFINE_OPERATOR_RHS(String,    char* const, <=, s, bool, *)

  inline auto operator>(const any& rhs_) const -> bool {
    auto& lhs = extract_value(*this);
    auto& rhs = extract_value(rhs_);
    assert(lhs.type == rhs.type);
    switch (lhs.type) {
      case Type::Integer:   return lhs.i > rhs.i;
      case Type::Double:    return lhs.d > rhs.d;
      case Type::Character: return lhs.c > rhs.c;
      case Type::Boolean:   return lhs.b > rhs.b;
      case Type::String:    return (*lhs.s) > (*rhs.s);
      default: assert(not "supported type for '>' operator");
    }
    return false;
  }

  DEFINE_OPERATOR_RHS(Integer,   long,        >, i, bool,)
  DEFINE_OPERATOR_RHS(Double,    double,      >, d, bool,)
  DEFINE_OPERATOR_RHS(Character, char,        >, c, bool,)
  DEFINE_OPERATOR_RHS(Boolean,   bool,        >, b, bool,)
  DEFINE_OPERATOR_RHS(String,    string&,     >, s, bool, *)
  DEFINE_OPERATOR_RHS(String,    char* const, >, s, bool, *)

  inline auto operator>=(const any& rhs_) const -> bool {
    auto& lhs = extract_value(*this);
    auto& rhs = extract_value(rhs_);
    assert(lhs.type == rhs.type);
    switch (lhs.type) {
      case Type::Integer:   return lhs.i >= rhs.i;
      case Type::Double:    return lhs.d >= rhs.d;
      case Type::Character: return lhs.c >= rhs.c;
      case Type::Boolean:   return lhs.b >= rhs.b;
      case Type::String:    return (*lhs.s) >= (*rhs.s);
      default: assert(not "supported type for '>' operator");
    }
    return false;
  }

  DEFINE_OPERATOR_RHS(Integer,   long,        >=, i, bool,)
  DEFINE_OPERATOR_RHS(Double,    double,      >=, d, bool,)
  DEFINE_OPERATOR_RHS(Character, char,        >=, c, bool,)
  DEFINE_OPERATOR_RHS(Boolean,   bool,        >=, b, bool,)
  DEFINE_OPERATOR_RHS(String,    string&,     >=, s, bool, *)
  DEFINE_OPERATOR_RHS(String,    char* const, >=, s, bool, *)

  inline auto operator+(const any& rhs_) const -> any {
    auto& lhs = extract_value(*this);
    auto& rhs = extract_value(rhs_);
    assert(lhs.type == rhs.type);
    switch (lhs.type) {
      case Type::Integer:   return lhs.i + rhs.i;
      case Type::Double:    return lhs.d + rhs.d;
      case Type::Character: return lhs.c + rhs.c;
      case Type::String:    return (*lhs.s) + (*rhs.s);
      default: assert(not "supported type for '+' operator");
    }
    return nullptr;
  }

  DEFINE_OPERATOR_RHS(Integer,   long,        +, i, long,)
  DEFINE_OPERATOR_RHS(Double,    double,      +, d, double,)
  DEFINE_OPERATOR_RHS(Character, char,        +, c, char,)
  DEFINE_OPERATOR_RHS(String,    string&,     +, s, string, *)
  DEFINE_OPERATOR_RHS(String,    char* const, +, s, string, *)

  inline auto operator-(const any& rhs_) const -> any {
    auto& lhs = extract_value(*this);
    auto& rhs = extract_value(rhs_);
    assert(lhs.type == rhs.type);
    switch (lhs.type) {
      case Type::Integer:   return lhs.i - rhs.i;
      case Type::Double:    return lhs.d - rhs.d;
      case Type::Character: return lhs.c - rhs.c;
      default: assert(not "supported type for '-' binary operator");
    }
    return nullptr;
  }

  DEFINE_OPERATOR_RHS(Integer,   long,   -, i, long,)
  DEFINE_OPERATOR_RHS(Double,    double, -, d, double,)
  DEFINE_OPERATOR_RHS(Character, char,   -, c, char,)

  inline auto operator*(const any& rhs_) const -> any {
    auto& lhs = extract_value(*this);
    auto& rhs = extract_value(rhs_);
    assert(lhs.type == rhs.type);
    switch (lhs.type) {
      case Type::Integer: return lhs.i * rhs.i;
      case Type::Double:  return lhs.d * rhs.d;
      default: assert(not "supported type for '*' operator");
    }
    return nullptr;
  }

  DEFINE_OPERATOR_RHS(Integer,   long,   *, i, long,)
  DEFINE_OPERATOR_RHS(Double,    double, *, d, double,)

  inline auto operator/(const any& rhs_) const -> any {
    auto& lhs = extract_value(*this);
    auto& rhs = extract_value(rhs_);
    assert(lhs.type == rhs.type);
    switch (lhs.type) {
      case Type::Integer: return lhs.i / rhs.i;
      case Type::Double:  return lhs.d / rhs.d;
      default: assert(not "supported type for '/' operator");
    }
    return nullptr;
  }

  DEFINE_OPERATOR_RHS(Integer,   long,   /, i, long,)
  DEFINE_OPERATOR_RHS(Double,    double, /, d, double,)

  inline auto operator%(const any& rhs_) const -> any {
    auto& lhs = extract_value(*this);
    auto& rhs = extract_value(rhs_);
    assert(lhs.type == rhs.type);
    switch (lhs.type) {
      case Type::Integer: return lhs.i % rhs.i;
      default: assert(not "supported type for '%' operator");
    }
    return nullptr;
  }

  DEFINE_OPERATOR_RHS(Integer,   long, %, i, long,)

  inline auto operator-() const -> any {
    auto& lhs = extract_value(*this);
    switch (lhs.type) {
      case Type::Integer: return (- lhs.i);
      case Type::Double:  return (- lhs.d);
      default: assert(not "supported type for unary '-' operator");
    }
    return nullptr;
  }

  friend auto operator==(const long, const any&) -> bool;
  friend auto operator!=(const long, const any&) -> bool;
  friend auto operator< (const long, const any&) -> bool;
  friend auto operator<=(const long, const any&) -> bool;
  friend auto operator> (const long, const any&) -> bool;
  friend auto operator>=(const long, const any&) -> bool;

  friend auto operator==(const double, const any&) -> bool;
  friend auto operator!=(const double, const any&) -> bool;
  friend auto operator< (const double, const any&) -> bool;
  friend auto operator<=(const double, const any&) -> bool;
  friend auto operator> (const double, const any&) -> bool;
  friend auto operator>=(const double, const any&) -> bool;

  friend auto operator==(const char, const any&) -> bool;
  friend auto operator!=(const char, const any&) -> bool;
  friend auto operator< (const char, const any&) -> bool;
  friend auto operator<=(const char, const any&) -> bool;
  friend auto operator> (const char, const any&) -> bool;
  friend auto operator>=(const char, const any&) -> bool;

  friend auto operator==(const bool, const any&) -> bool;
  friend auto operator!=(const bool, const any&) -> bool;
  friend auto operator< (const bool, const any&) -> bool;
  friend auto operator<=(const bool, const any&) -> bool;
  friend auto operator> (const bool, const any&) -> bool;
  friend auto operator>=(const bool, const any&) -> bool;

  friend auto operator==(const string&, const any&) -> bool;
  friend auto operator!=(const string&, const any&) -> bool;
  friend auto operator< (const string&, const any&) -> bool;
  friend auto operator<=(const string&, const any&) -> bool;
  friend auto operator> (const string&, const any&) -> bool;
  friend auto operator>=(const string&, const any&) -> bool;

  friend auto operator==(const char* const, const any&) -> bool;
  friend auto operator!=(const char* const, const any&) -> bool;
  friend auto operator< (const char* const, const any&) -> bool;
  friend auto operator<=(const char* const, const any&) -> bool;
  friend auto operator> (const char* const, const any&) -> bool;
  friend auto operator>=(const char* const, const any&) -> bool;

  friend auto operator+(const long, const any&) -> long;
  friend auto operator-(const long, const any&) -> long;
  friend auto operator*(const long, const any&) -> long;
  friend auto operator/(const long, const any&) -> long;
  friend auto operator%(const long, const any&) -> long;

  friend auto operator+(const double, const any&) -> double;
  friend auto operator-(const double, const any&) -> double;
  friend auto operator*(const double, const any&) -> double;
  friend auto operator/(const double, const any&) -> double;

  friend auto operator+(const char, const any&) -> char;
  friend auto operator-(const char, const any&) -> char;

  friend auto operator+(const string&, const any&) -> string;
  friend auto operator+(const char* const, const any&) -> string;

};

#define DEFINE_OPERATOR_LHS(K, T, OP, V, R, P) \
inline auto operator OP(const T lhs, const any& rhs_) -> R { \
  auto& rhs = any::extract_value(rhs_); \
  assert(rhs.type == any::Type::K); \
  return lhs OP P(rhs.V); \
}

DEFINE_OPERATOR_LHS(Integer, long, ==, i, bool,)
DEFINE_OPERATOR_LHS(Integer, long, !=, i, bool,)
DEFINE_OPERATOR_LHS(Integer, long, <,  i, bool,)
DEFINE_OPERATOR_LHS(Integer, long, <=, i, bool,)
DEFINE_OPERATOR_LHS(Integer, long, >,  i, bool,)
DEFINE_OPERATOR_LHS(Integer, long, >=, i, bool,)

DEFINE_OPERATOR_LHS(Double, double, ==, d, bool,)
DEFINE_OPERATOR_LHS(Double, double, !=, d, bool,)
DEFINE_OPERATOR_LHS(Double, double, <,  d, bool,)
DEFINE_OPERATOR_LHS(Double, double, <=, d, bool,)
DEFINE_OPERATOR_LHS(Double, double, >,  d, bool,)
DEFINE_OPERATOR_LHS(Double, double, >=, d, bool,)

DEFINE_OPERATOR_LHS(Character, char, ==, c, bool,)
DEFINE_OPERATOR_LHS(Character, char, !=, c, bool,)
DEFINE_OPERATOR_LHS(Character, char, <,  c, bool,)
DEFINE_OPERATOR_LHS(Character, char, <=, c, bool,)
DEFINE_OPERATOR_LHS(Character, char, >,  c, bool,)
DEFINE_OPERATOR_LHS(Character, char, >=, c, bool,)

DEFINE_OPERATOR_LHS(Boolean, bool, ==, b, bool,)
DEFINE_OPERATOR_LHS(Boolean, bool, !=, b, bool,)
DEFINE_OPERATOR_LHS(Boolean, bool, <,  b, bool,)
DEFINE_OPERATOR_LHS(Boolean, bool, <=, b, bool,)
DEFINE_OPERATOR_LHS(Boolean, bool, >,  b, bool,)
DEFINE_OPERATOR_LHS(Boolean, bool, >=, b, bool,)

DEFINE_OPERATOR_LHS(String, string&, ==, s, bool, *)
DEFINE_OPERATOR_LHS(String, string&, !=, s, bool, *)
DEFINE_OPERATOR_LHS(String, string&, <,  s, bool, *)
DEFINE_OPERATOR_LHS(String, string&, <=, s, bool, *)
DEFINE_OPERATOR_LHS(String, string&, >,  s, bool, *)
DEFINE_OPERATOR_LHS(String, string&, >=, s, bool, *)

DEFINE_OPERATOR_LHS(String, char* const, ==, s, bool, *)
DEFINE_OPERATOR_LHS(String, char* const, !=, s, bool, *)
DEFINE_OPERATOR_LHS(String, char* const, <,  s, bool, *)
DEFINE_OPERATOR_LHS(String, char* const, <=, s, bool, *)
DEFINE_OPERATOR_LHS(String, char* const, >,  s, bool, *)
DEFINE_OPERATOR_LHS(String, char* const, >=, s, bool, *)

DEFINE_OPERATOR_LHS(Integer, long, +, i, long,)
DEFINE_OPERATOR_LHS(Integer, long, -, i, long,)
DEFINE_OPERATOR_LHS(Integer, long, *, i, long,)
DEFINE_OPERATOR_LHS(Integer, long, /, i, long,)
DEFINE_OPERATOR_LHS(Integer, long, %, i, long,)

DEFINE_OPERATOR_LHS(Double, double, +, d, double,)
DEFINE_OPERATOR_LHS(Double, double, -, d, double,)
DEFINE_OPERATOR_LHS(Double, double, *, d, double,)
DEFINE_OPERATOR_LHS(Double, double, /, d, double,)

DEFINE_OPERATOR_LHS(Character, char, +, i, char,)
DEFINE_OPERATOR_LHS(Character, char, -, i, char,)

DEFINE_OPERATOR_LHS(String, string&, +, s, string, *)
DEFINE_OPERATOR_LHS(String, char* const, +, s, string, *)

#undef DEFINE_OPERATOR_RHS
#undef DEFINE_OPERATOR_LHS

} // namespace PureScript

#endif // PureScript_HH
