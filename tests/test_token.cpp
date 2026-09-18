#include "catch.hpp"
#include "token.hpp"
#include <type_traits>
#include <utility>

namespace {
template<class T> class CanBorrowTokenValue {
    template<class U> static auto check(int)
        -> decltype(std::declval<U>().valueRef(), std::true_type());
    template<class> static std::false_type check(...);
public:
    static const bool value = decltype(check<T>(0))::value;
};
static_assert(std::is_same<decltype(std::declval<const Token&>().value()), std::string>::value,
              "value() must preserve the owning public API");
static_assert(CanBorrowTokenValue<Token&>::value, "lvalue tokens may be borrowed");
static_assert(CanBorrowTokenValue<const Token&>::value, "const lvalue tokens may be borrowed");
static_assert(!CanBorrowTokenValue<Token&&>::value, "temporary tokens must not be borrowed");
static_assert(!CanBorrowTokenValue<const Token&&>::value, "const temporary tokens must not be borrowed");
}

TEST_CASE("Token values outlive temporary tokens", "[token][regression]") {
    // Use heap-backed strings so ASan also detects the former dangling reference.
    const auto& value = Token(STRING, 7, std::string(1024, 'x')).value();
    const std::string overwrite(1024, 'y');
    REQUIRE(value == std::string(1024, 'x'));
    REQUIRE(value != overwrite);
}

TEST_CASE("Owning Token values do not alias later assignments", "[token][regression]") {
    Token token(STRING, 3, "before");
    const auto& owned = token.value();
    token = Token(STRING, 3, "after");
    REQUIRE(owned == "before");
    REQUIRE(token.valueRef() == "after");
}

TEST_CASE("Token borrowing avoids copies for live lvalues", "[token][regression]") {
    const Token token(STRING, 9, std::string(1024, 'z'));
    const auto& borrowed = token.valueRef();
    REQUIRE(&borrowed == &token.valueRef());
    REQUIRE(borrowed == token.value());
    REQUIRE(token == Token(STRING, 9, borrowed));
}
