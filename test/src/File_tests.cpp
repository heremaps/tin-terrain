#include "catch.hpp"

#include <fstream>
#include <boost/filesystem.hpp>
#include <boost/scope_exit.hpp>

#include "tntn/File.h"

namespace tntn {
namespace unittests {

TEST_CASE("File open existing and read", "[tntn]")
{
    auto tempfilename =
        boost::filesystem::temp_directory_path() / boost::filesystem::unique_path();

    File fout;
    REQUIRE(fout.open(tempfilename.c_str(), File::OM_RWC));
    BOOST_SCOPE_EXIT(&tempfilename) { boost::filesystem::remove(tempfilename); }
    BOOST_SCOPE_EXIT_END
    CHECK(fout.write(0, "fooo", 4));
    CHECK(fout.write(3, "bar", 3));
    CHECK(fout.size() == 6);
    CHECK(fout.close());

    REQUIRE(boost::filesystem::exists(tempfilename));

    File fin;
    REQUIRE(fin.open(tempfilename.c_str(), File::OM_R));
    CHECK(fin.size() == 6);
    std::string foobar;
    fin.read(0, foobar, 6);
    CHECK(fin.is_good());
    CHECK(foobar == "foobar");
}

TEST_CASE("File write past end", "[tntn]")
{
    auto tempfilename =
        boost::filesystem::temp_directory_path() / boost::filesystem::unique_path();

    File fout;
    REQUIRE(fout.open(tempfilename.c_str(), File::OM_RWC));
    BOOST_SCOPE_EXIT(&tempfilename) { boost::filesystem::remove(tempfilename); }
    BOOST_SCOPE_EXIT_END

    CHECK(fout.write(42, "foo", 3));

    std::vector<char> foo;
    fout.read(0, foo, 42 + 3);
    CHECK(foo.size() == 42 + 3);
}

TEST_CASE("getline on MemoryFile", "[tntn]")
{

    MemoryFile f;
    std::string foo_line = "foo\n";
    std::string bar_line = "bar\n";
    std::string baz_line = "baz";

    f.write(0, foo_line);
    f.write(f.size(), bar_line);
    f.write(f.size(), baz_line);

    REQUIRE(f.size() == foo_line.size() + bar_line.size() + baz_line.size());

    std::string line;
    FileLike::position_type from_offset = 0;
    from_offset = getline(from_offset, f, line);
    CHECK(from_offset < f.size());
    CHECK(line == "foo");

    from_offset = getline(from_offset, f, line);
    CHECK(from_offset < f.size());
    CHECK(line == "bar");

    from_offset = getline(from_offset, f, line);
    CHECK(from_offset == f.size());
    CHECK(line == "baz");
}

} //namespace unittests
} //namespace tntn
