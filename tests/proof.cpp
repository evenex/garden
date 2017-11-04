#include<garden/proof.tcc>
#include<garden/testing.tcc>

using namespace garden;

#define MODULE "proof"

struct trivial_property : property
{};
struct impossible_property : property
{
  template<class> requires false
  class proof{};
};

EXAMPLE( proof 0 )
{
  static_assert( models<int, trivial_property> );
  static_assert( not models<int, impossible_property> );
}

struct Integral : property
{
  template<class X>
  requires std::is_integral_v<X>
  class proof{};
};

EXAMPLE( proof 1 )
{
  static_assert( models<int, Integral> );
  static_assert( not models<float, Integral> );
}
