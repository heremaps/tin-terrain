#include "catch.hpp"

#include <string>
#include <unordered_set>

#include "tntn/ObjPool.h"

namespace tntn {
namespace unittests {

TEST_CASE("ObjPool operations", "[tntn]")
{
    auto pool = ObjPool<std::string>::create();

    auto p1 = pool->spawn();
    p1->assign("foo");

    auto s1 = *p1;
    CHECK(s1 == "foo");

    const auto p2 = p1.pool()->spawn();

    CHECK(p1 != p2);
    CHECK(!(p1 == p2));

    if(p1)
    {
        CHECK(true);
    }
    if(!p2)
    {
        CHECK(false);
    }

    std::unordered_set<pool_ptr<std::string>> ptr_hash_set;
    ptr_hash_set.insert(p1);
    ptr_hash_set.insert(p2);
    CHECK(ptr_hash_set.size() == 2);
    ptr_hash_set.insert(p2);
    CHECK(ptr_hash_set.size() == 2);

    auto p3 = p1;
    pool_ptr<std::string> p4;
    CHECK(!p4);
    p4 = p1;
    CHECK(p4);

    CHECK(p3 == p4);
}

} // namespace unittests
} // namespace tntn
