#include<garden/functional.tcc>
#include<garden/testing.tcc>

#define MODULE "functional"

using namespace garden;
using namespace std::literals::string_literals;

struct increment_fn
: fn<increment_fn, with<arity<1>>>
{
  template<class X>
  requires 
  requires(X x){ x+1; }
  static constexpr auto eval
  (X x) -> auto
  { return x+1; }
};
struct square_fn
: fn<square_fn, with<arity<1>>>
{
  template<class X>
  requires
  requires(X x){ x*x; }
  static constexpr auto eval
  (X x) -> auto
  { return x*x; }
};
struct add_fn
: fn<add_fn, with<arity<2>, commutativity>>
{
  template<class X>
  static constexpr auto eval
  (X a, X b) -> auto
  { return a+b; }
};
struct size_of_fn
: fn<size_of_fn, with<arity<1>>>
{
  template<class X>
  static constexpr auto eval
  (X) -> size_t
  {
    return sizeof(X);
  }
};

EXAMPLE( arity property ) 
{
  using F = increment_fn;

  REQUIRE(( not std::is_invocable_v<F> ));
  REQUIRE(( std::is_invocable_v<F, int> ));
  REQUIRE(( not std::is_invocable_v<F, int, int> ));
  REQUIRE(( not std::is_invocable_v<F, int, int> ));

  using G = add_fn;

  REQUIRE(( not std::is_invocable_v<G> ));
  REQUIRE(( std::is_invocable_v<G, int> ));
  REQUIRE(( std::is_invocable_v<G, int, int> ));
  REQUIRE(( not std::is_invocable_v<G, int, int, int> ));

  using H = square_fn;

  REQUIRE(( not std::is_invocable_v<H, void*> ));
}
EXAMPLE( commutativity property ) 
{
  add_fn f;

  REQUIRE_NOTHROW( f( 1, 2 ) );

  REQUIRE_THROWS_AS( f( "foo"s, "bar"s ),
    property::failed<commutativity>
  );
}
EXAMPLE( partial application )
{
  add_fn f;

  REQUIRE(( f( 1, 2 ) == 3 ));
  REQUIRE(( f( 1 )( 2 ) == 3 ));
}
EXAMPLE( destructuring )
{
  add_fn f;

  REQUIRE( f( std::tuple( 1, 2 ) ) == 3 );

  size_of_fn g;
  using V = std::variant<long, short>;

  REQUIRE( g( V( 1L ) ) == sizeof(long) );
}
EXAMPLE( composition )
{
  auto f = increment_fn{} << square_fn{};
  auto g = increment_fn{} >> square_fn{};

  REQUIRE(( f( 3 ) == 10 ));
  REQUIRE(( g( 3 ) == 16 ));

  auto h = add_fn{} << square_fn{};
  auto k = add_fn{} >> square_fn{};

  REQUIRE(( h( 3 )( 3 ) == 12 ));
  REQUIRE(( h( 3 )( 3 ) == 12 ));
  REQUIRE(( k( 3 )( 3 ) == 36 ));

}
EXAMPLE( adjoinment )
{
  auto f = increment_fn{};
  auto g = square_fn{};

  REQUIRE(( ( f & g )( 3 ) == std::tuple( 4, 9 ) ));
}
EXAMPLE( matching )
{
  auto f = (
    [](int i){ return i*3; }
    | [](auto s){ return s.size(); }
  );

  REQUIRE(( 
    f( std::string( "hi" ) ) == 2
  ));
  REQUIRE(( 
    f( 2 ) == 6
  ));
}
