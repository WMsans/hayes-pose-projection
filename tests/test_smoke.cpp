#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

namespace pose { int link_anchor(); }

TEST_CASE("core library links") { CHECK(pose::link_anchor() == 0); }
