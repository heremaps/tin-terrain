#include "catch.hpp"

#include "tntn/util.h"

namespace tntn {
namespace unittests {

TEST_CASE("SimpleRange::for_each with lambda", "[tntn]")
{
    std::vector<int> v{0, 1, 2, 3, 4, 5};
    SimpleRange<std::vector<int>::iterator> r{v.begin(), v.end()};

    std::vector<int> cp;
    r.for_each([&cp](int x) { cp.push_back(x); });

    CHECK(cp == v);
}

TEST_CASE("SimpleRange:copy_into vector", "[tntn]")
{
    std::vector<int> v{0, 1, 2, 3, 4, 5};
    SimpleRange<std::vector<int>::iterator> r{v.begin(), v.end()};

    std::vector<int> v2;
    r.copy_into(v2);

    CHECK(v == v2);
}

} // namespace unittests
} // namespace tntn
