#include<garden/functional.tcc>
#include<garden/testing.tcc>

#define MODULE "functional"

using namespace garden;
using namespace std::literals::string_literals;

struct increment_fn
: Fn<increment_fn>
{
  let eval
  (auto x) -> auto
  { return x+1; }

  template<class X>
  requires 
  requires(X x){ x+1; }
  let assume
  (X){}
};
struct square_fn
: Fn<square_fn>
{
  let eval
  (auto x) -> auto
  { return x*x; }

  template<class X>
  requires
  requires(X x){ x*x; }
  let assume
  (X){}
};
struct add_fn
: Fn<add_fn>
{
  let eval
  (auto a, auto b) -> auto
  { return a+b; }

  let assume(auto){}

  template<class X, class Y>
  requires requires
  { typename std::common_type<X,Y>::type; }
  let assume
  (X a, Y b)
  {
    if( eval( a,b ) != eval( b,a ) )
      throw 0;
  }
};
struct size_of_fn
: Fn<size_of_fn>
{
  template<class X>
  let eval
  (X) -> size_t
  {
    return sizeof(X);
  }
};

EXAMPLE( assumption validation ) 
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
  
  G g;

  REQUIRE_NOTHROW( g( 1, 2 ) );
  REQUIRE_THROWS( g( "foo"s, "bar"s ) );
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
EXAMPLE( destructuring with partial application )
{
  add_fn f;

  auto g = f(1);

  using T = std::tuple<long>;
  using V = std::variant<char, int>;

  REQUIRE( g( T( 1 ) ) == 2 );
  REQUIRE( g( V( 256 ) ) == 257 );
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
