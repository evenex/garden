#include<garden/tree.tcc>
#include<garden/testing.tcc>

#define MODULE "tree"

using namespace garden;

struct T
{
  auto top() const -> int
  {
    return value;
  }
  auto down() const -> Range
  {
    return range::from_container( children );
  }

  int value;
  std::vector<T> children;
};

EXAMPLE( hello )
{
  static_assert( Tree<T> );

  auto t = T{ 1, { T{ 2 }, T{ 3 } } };

  REQUIRE(( t.top() == 1 ));
  REQUIRE(( t.down().front().top() == 2 ));
  REQUIRE(( t.down().next().front().top() == 3 ));
}
