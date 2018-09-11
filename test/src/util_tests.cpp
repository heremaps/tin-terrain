#include "catch.hpp"

#include "tntn/util.h"

namespace tntn {
namespace unittests {

TEST_CASE("tokenize simple line", "[tntn]")
{
    std::vector<std::string> tokens;
    tokenize("abc def ", tokens);
    REQUIRE(tokens.size() == 2);
    CHECK(tokens[0] == "abc");
    CHECK(tokens[1] == "def");
}

TEST_CASE("tokenize empty line", "[tntn]")
{
    std::vector<std::string> tokens;
    tokenize("", tokens);
    CHECK(tokens.empty());
}

} // namespace unittests
} // namespace tntn
