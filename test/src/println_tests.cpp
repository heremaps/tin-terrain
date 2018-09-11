#include "catch.hpp"

#include "tntn/println.h"
#include <sstream>

namespace tntn {
namespace unittests {

struct Printable
{
    int foo = 45;
};
static std::ostream& operator<<(std::ostream& os, const Printable& p)
{
    os << p.foo;
    return os;
}

TEST_CASE("println overloads and templates compile", "[tntn]")
{
    println("{}");
    println(std::string("{}"));
    println();

    println(std::string("{}"), 42);
    println("{}", 43);
    println(std::stringstream() << 44);
    println(Printable());
}

TEST_CASE("print overloads and templates compile", "[tntn]")
{
    print("{}");
    print(std::string("{}"));

    print(std::string("{}"), 42);
    print("{}", 42);
    print(std::stringstream() << 44);
    print(Printable());

    println();
}
} // namespace unittests
} // namespace tntn
