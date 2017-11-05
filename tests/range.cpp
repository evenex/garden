#include<garden/range.tcc>
#include<garden/testing.tcc>

#define MODULE "range"

namespace garden
{
  int x[5] = { 1, 2, 3, 4, 5 };

  auto r = range::from_ptr( x, 5 );

  EXAMPLE( hi )
  {
    auto a = 0;
    for( auto i : r )
      a += i;

    auto b = 0;
    for( auto i : x )
      b += i;

    REQUIRE(( a == b ));
  }
  EXAMPLE( equality )
  {
    REQUIRE(( r == r ));
  }
  EXAMPLE( recurrence )
  {
    auto f = range::recur(
      [](auto x){ return x+1; }
    );

    auto a = 0;
    for( auto i : f( 6 ) )
      if( i < 10 )
        ++a;
      else
        break;

     REQUIRE(( a == 4 ));
  }
  EXAMPLE( drop/take )
  {
    using namespace range;

    REQUIRE(( r == ( 
      1^(
        range::recur(
          [](auto i){ return i + 1; }
        ) >> take( 5 )
      )
    ) ));

    REQUIRE(( r == (
      -4^(
        range::recur(
          [](auto i){ return i + 1; }
        ) >> take( 10 ) >> drop( 5 )
      )
    ) ));
  }
  EXAMPLE( transform )
  {
    using namespace range;

    auto a = 0;
    for( auto i : 
      r^transform( [](auto j){ return j*2; } ) 
    )
      a += i;

    auto b = 0;
    for( auto i : x )
      b += i;

    REQUIRE(( a == b*2 ));
  }
  EXAMPLE( functor )
  {
    auto f = [](auto i){ return i*i; };

    REQUIRE((
      ( r^functor::transform( f ) )
      == ( r^range::transform( f ) )
    ));
  }
  EXAMPLE( only )
  {
    auto a = 0;
    for( auto i
      : range::only( 1, 2, 3 )
    )
      a += i;

    REQUIRE(( a == 6 ));
  }
}
