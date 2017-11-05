#include<garden/functor.tcc>
#include<garden/testing.tcc>

#define MODULE "functor"

using namespace garden;

EXAMPLE( functor )
{
  auto f = [](auto i){ return i*i; };

  auto i = new int{ 5 };
  int* j = nullptr;

  REQUIRE((
    *( i^functor::transform( f ) ) == 25
  ));
  REQUIRE((
    ( j^functor::transform( f ) ) == nullptr
  ));
}
