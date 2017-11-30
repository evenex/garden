#include<garden/repl.tcc>
#include<garden/lens.tcc>
#include<garden/database.tcc>
#include<garden/testing.tcc>

using namespace garden;

let pi = std::atan( 1.f ) * 4;

let iota = range::recur( [](auto i){ return ++i; }, 0 );

#include<iostream>
#include<list>

namespace garden
{ // tuple/variant concept
  template<class X>
  concept bool Tuple = requires
  {
    std::tuple_size<X>::value;
  };
  template<class X>
  concept bool Variant = requires
  {
    std::variant_size<X>::value;
  };
}

namespace std
{ // common show types
  template<garden::Tuple X>
  auto operator<<
  (std::ostream& out, X x) -> std::ostream&
  {
    return out << std::pair(
      std::make_index_sequence<
        std::tuple_size_v<X>
      >(),
      x
    );
  }
  template<Tuple X, auto i, auto... ns>
  auto operator<<
  (std::ostream& out, 
    std::pair<std::index_sequence<i,ns...>, X> to_print
  ) -> std::ostream&
  {
    auto& x = std::get<1>( to_print );

    if constexpr( i == 0 )
    {
      out << '(';
    }

    out << std::get<i>( x );

    if constexpr( i+1 < std::tuple_size_v<X> )
      out << ", ";

    return out << std::pair(
      std::index_sequence<ns...>{},
      x
    );
  }
  auto operator<<
  (std::ostream& out, 
    std::pair<std::index_sequence<>, Tuple>
  ) -> std::ostream&
  {
    return out << ')';
  }

  auto operator<<
  (std::ostream& out, Variant x) -> std::ostream&
  {
    out << '<';
    std::visit( 
      [&](auto a){ out << a; },
      x
    );
    out<< '>';

    return out;
  }

  auto operator<<
  (std::ostream& out, std::optional<auto> x) -> std::ostream&
  {
    return ( x?
      out << *x 
      : out << "{}"
    );
  }
}

#include<tuple>

namespace garden
{
  template<size_t i>
  struct element_fn
  : Fn<element_fn<i>>
  {
    template<class X>
    requires requires
    {
      std::tuple_size<X>::value;
      std::tuple_element_t<i, X>{};
    }
    let assume(X){}

    let eval
    (auto x) -> auto
    {
      return std::get<i>( x );
    }
  };
  template<size_t i>
  let element = element_fn<i>{};

  template<class I>
  requires 
  requires(I i)
  {
    { *i } -> std::any;
    { i != i } -> bool;
    { ++i } -> I&;
  }
  struct range::instance<std::pair<I,I>>
  {
    let front
    (std::pair<I,I> r) -> auto
    {
      return *r.first;
    }

    let empty
    (std::pair<I,I> r) -> auto
    {
      return r.first == r.second;
    }

    let advance
    (std::pair<I,I>& r) -> std::pair<I,I>&
    {
      ++r.first; return r;
    }
  };
}
namespace std
{
  let begin
  (std::pair<auto, auto>& r) -> auto&
  { return r; }

  let end
  (std::pair<auto, auto>& r) -> nullptr_t
  { return {}; }
}

namespace garden
{ // optional adjoin
  let operator
  &(std::optional<auto> x, std::optional<auto> y)
  -> std::optional<auto>
  {
    auto adjoin = [&]()
    {
      return std::tuple_cat(
        std::tuple{ *x },
        std::tuple{ *y }
      );
    };

    if( x and y )
      return std::optional( adjoin() );
    else
      return std::optional<decltype(adjoin())>{};
  }
}

namespace garden
{ // arithmetic
  struct add_fn
  : Fn<add_fn>
  {
    template<class B, class A>
    requires 
    requires(B b, A a)
    { a+b; }
    let assume(B, A){}
    let assume(auto){}

    let eval
    (auto b, auto a) -> auto
    {
      return a + b;
    }
  };
  let add = add_fn{};

  struct subtract_fn
  : Fn<subtract_fn>
  {
    template<class B, class A>
    requires 
    requires(B b, A a)
    { a - b; }
    let assume(B, A){}
    let assume(auto){}

    let eval
    (auto b, auto a) -> auto
    {
      return a - b;
    }
  };
  let subtract = subtract_fn{};
}

// TODO using functor::operator%;
// TODO using monoid::operator+;

namespace game
{
  class Location;
  class Offer;
  class Agent;
  class Item;

  template<class X>
  struct Link
  {
    explicit constexpr Link
    (auto... args)
    : ptr( std::make_shared<X>( args... ) )
    {}

    constexpr auto operator
    ->() -> auto
    {
      return ptr;
    }
    constexpr auto operator
    ->() const -> auto
    {
      return ptr;
    }

    constexpr auto operator
    <(const Link& that) const
    {
      return ptr < that.ptr;
    }

  private:

    std::shared_ptr<X> ptr;
  };

  struct Node
  {
    Node() = default;
    Node(Node&&) = default;
    Node(const Node&) = delete;
  };

  LENS( name )
  LENS( owner )
  struct Item : Node
  {
    std::string name;

    std::variant<
      Link<Agent>,
      Link<Location>
    > owner;
  };

  LENS( location )
  LENS( offers )
  LENS( items )
  struct Agent : Node
  {
    std::string name;
    Link<Location> location;

    database::Table<
      Link<Offer>, identity_fn
    > offers;

    database::Table<
      Link<Item>, identity_fn,
      name_fn
    > items;
  };

  LENS( agents )
  struct Location : Node
  {
    std::string name;

    database::Table<
      Link<Agent>, name_fn
    > agents;

    database::Table<
      Link<Item>, identity_fn,
      name_fn
    > items;
  };

  auto operator
  <<(std::ostream& out, Link<Agent> a) -> std::ostream&
  {
    out << (a^name) 
    << " @ " << (a^location^name)
    << ": " << std::endl;

    for( auto x : a^items )
      out << '\t' << (x^name) << std::endl;
    
    return out;
  }
  auto operator
  <<(std::ostream& out, Link<Location> l) -> std::ostream&
  {
    out << (l^name) << ": " << std::endl;

    for( auto a : l^agents )
      out << '\t' << (a^name) << std::endl;

    return out;
  }
  auto operator
  <<(std::ostream& out, Link<Item> x) -> std::ostream&
  {
    out << (x^name) << " @ "
    << (x^owner^name) << std::endl;

    return out;
  }

  struct relocate_to_fn
  : Fn<relocate_to_fn>
  {
    static auto eval
    (Link<Location> l, Link<Agent> a) -> auto
    {
      a^( location 
        >> agents
        >> erase( a )
      );

      l^( agents
        >> insert( a )
      );

      a^location( l );

      return l;
    }
  };
  let relocate_to = relocate_to_fn{};

  struct transfer_to_fn
  : Fn<transfer_to_fn>
  {
    static auto eval
    (Link<Agent> a, Link<Item> x) -> auto
    {
      x^owner^items^erase( x );
      
      a^items^insert( x );

      x^owner( a );

      return x;
    }
  };
  let transfer_to = transfer_to_fn{};
}
namespace game
{
  struct World
  {
    static inline auto 
    locations = database::table<
      Link<Location>
    >( name );

    static inline auto
    agents = database::table<
      Link<Agent>
    >( name, location );

    static inline auto
    items = database::table<
      Link<Item>
    >( identity, name, owner );
  };
}

int main(int argc, char** argv)
{
  using namespace game;
  using namespace range;

  auto new_agent = [&]
  (std::string name, Link<Location> loc) -> Link<Agent>
  {
    auto x = Link<Agent>{};

    x->name = name;
    x->location = loc;

    loc^agents
    ^insert( x );

    return *World::agents.insert( x );
  };
  auto new_location = [&]
  (std::string name) -> Link<Location>
  {
    auto x = Link<Location>{};

    x->name = name;

    return *World::locations.insert( x );
  };
  auto new_item = [&]
  (std::string name, Link<auto> owner) -> Link<auto>
  {
    auto x = Link<Item>{};

    x->name = name;
    x->owner = owner;

    owner^items^insert( x );

    return *World::items.insert( x );
  };

  auto zone_a = new_location( "zone_a" );
  auto zone_b = new_location( "zone_b" );

  auto fred = new_agent( "fred", zone_a );
  auto bill = new_agent( "bill", zone_b );
  auto sprocket = new_item( "sprocket", fred );

  show( sprocket );
  show( fred );
  show( bill );

  fred^relocate_to( zone_b );

  show( sprocket );
  show( fred );
  show( bill );

  sprocket^transfer_to( bill );

  show( sprocket );
  show( fred );
  show( bill );
}
