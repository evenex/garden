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

namespace garden
{ // common show types
  template<Tuple X>
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

template<class X>
auto hash
(X x) -> std::size_t
{
  return std::hash<X>{}( x );
}

namespace garden
{ // optional application
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
namespace game
{
  using Id = std::string;
  
  struct Energy
  {
    Id type;
    int volume = 0;
  };
  LENS( type )
  LENS( volume )

  auto operator
  <<(std::ostream& out, Energy energy) -> std::ostream&
  {
    auto[ type, volume ] = energy;

    return out << text( type, ": ", volume );
  }

  struct Entity
  {
    struct Memory
    {
      Id who;
      Energy what;
      Id where;

      struct hash_fn
      : Fn<hash_fn>
      {
        static auto eval
        (Memory m) -> std::size_t
        {
          auto&[ who, what, where ] = m;

          return hash( who )
          ^ hash( what.type )
          ^ hash( what.volume )
          ^ hash( where )
          ;
        }
      };
    };

    Id name;
    Id location;

    database::Table<Energy, type_fn> inventory;
    database::Table<Memory, Memory::hash_fn> memory;

    std::vector<std::function<Entity(Entity)>> strategy;
  };
  LENS( name )
  LENS( location )

  struct transfer
  {
    Energy x;

    transfer(Energy x)
    : x( x ) {}

    auto to
    (Entity b) const -> auto
    {
      return [=]
      (Entity a) -> std::pair<Entity, Entity>
      {
        auto impulse = [&]
        (auto op, Entity e) -> Entity
        {
          if( not e.inventory( x ) )
            e.inventory.insert({ x.type });
            
          e.inventory.modify( 
            x.type, volume << (
              volume >> op( x^volume )
              & identity
            )
          );
            
          return e;
        };

        return std::pair(
          impulse( subtract, a ),
          impulse( add, b )
        );
      };
    }
  };
}

int main(int argc, char** argv)
{
  using namespace game;
  using namespace range;

  auto entities = database::table<Entity>(
    name, location
  );

  entities.insert({ "bill", "zone1" });
  entities.insert({ "ted", "zone1" });
  entities.insert({ "fred", "zone2" });

  entities.modify( "fred", []
    (auto fred) -> Entity
    {
      fred.inventory
      .insert({ "butter", 2 });

      return fred;
    }
  );

  auto play_as = [&]
  (Entity you) -> std::tuple<>
  {
    auto& your = you;

    auto update = [&]()
    {
      entities.update( you );

      for( auto entity : entities )
        if( entity.name != your.name )
          entities.update( entity );

      for( auto entity : entities )
        entities.modify( entity, identity );

      you = *entities( your.name );
    };

    auto quit = [&](auto) -> std::string
    {
      throw repl::exit_loop{};
    };
    auto help = [&](auto) -> std::string
    {
      throw repl::show_help{};
    };
    auto look = [&](auto subject)
    {
      if( subject.empty() )
      {
        std::stringstream out;

        auto show = show_fn{ out };

        show( your.location, ":\n" );

        for( auto entity : entities( location, your.location ) )
          if( entity.name != your.name )
            show( '\t', entity.name, '\n' );

        return out.str();
      }
      else
      {
        if( subject.front() == "self" )
          subject.front() = your.name;

        if( auto a = entities( subject.front() ) )
        {
          if( a->location == your.location )
          {
            std::stringstream out;

            auto show = show_fn{ out };

            show( a->name, ":\n" );

            for( auto x : a->inventory )
              show( '\t', x.type, ": ", x.volume, '\n' );

            return out.str();
          }
          else
          {
            return text( a->name, " is not here" );
          }
        }
        else
        {
          return text( subject.front(), " is not known" );
        }
      }
    };
    auto move = [&](auto destination)
    {
      if( destination.empty() )
        return text( "move (destination)" );

      auto x = destination.front();

      if( x == your.location )
        return text( 
          your.name, " is already at ", your.location 
        );

      your.strategy.push_back( location( x ) );

      update();

      return text( 
        your.name, " arrived at ", your.location
      );
    };
    auto wait = [&](auto)
    {
      update();

      return text( your.name, " waited at ", your.location );
    };
    auto hit = [&](auto args)
    {
      if( args.size() < 3 )
        return text( "hit (who) (how many) (what)" );

      auto who = args[0];
      auto how_many = std::stoi( args[1] );
      auto what = args[2];

      if( auto whom = entities( who ) )
      {
        you^transfer({ what, how_many })
        .to( *whom );

        return text( 
          "you have ", your.inventory( what ),
          ", ",
          who, " has ", whom->inventory( what )
        );
      }
    };
    auto ask = [&](auto args)
    {
      if( args.size() < 2 )
        return text( "ask (who) (what)" );

      auto who = args[0];
      auto what = args[1];

      // apply the diff
      // check constraints
      // rollback if violated

      // a diff is a temporary link in the graph
      // each entity struct need only define a variant
      // entity can have variant operators for diffs
      // direcy modification not allowed, only new diff links can be set
      // what about generating a candidate subgraph?

      // functions on diff links
      // typed engine evaluates typed links (visitor?)
      // create candidate subgraph (softlinked to outer graph?)
      // check constraints on combined candidate graph
      // but only up to some subgraph b/c we dont want to 
      // do a full graph traversal for every diff link

      if( auto entity = entities( who ) )
      {
        if( who == what )
        {
          return text( 
            entity->name, " is fine, thanks for asking" 
          );
        }
        else for( auto trade : entity->memory )
        {
          return text( "fail" );
        }

        return text( 
          entity->name, " doesn't know about ", what 
        );
      }
      else
      {
        return text( "unknown entity" );
      }
    };

    repl::main({
      { "help", help }
      , { "quit", quit }, { "q", quit }

      , { "look", look }, { "l", look }
      , { "move", move }, { "m", move }
      , { "wait", wait }, { "w", wait }

      , { "ask", ask }
      , { "hit", hit }
    });

    return {};
  };

  play_as % entities( "bill" );
}
