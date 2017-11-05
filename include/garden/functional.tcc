#pragma once
#include<functional>
#include<tuple>
#include<variant>
#include<garden/proof.tcc>

namespace garden
{
  template<auto i, class... X>
  requires i < sizeof...(X)
  using ith_t = std::tuple_element_t<
    i, std::tuple<X...>
  >;
}
namespace garden
{ template<class...> class beta_redux;

  template<class F, class... X>
  requires requires
  (F f, X... xs)
  { f.eval( xs... ); }
  struct beta_redux<F(X...)>
  {};

  template<class Expr>
  concept bool BetaReducible = requires
  {
    requires requires 
    { sizeof(beta_redux<Expr>); };
  };
}
namespace garden
{ template<class...> class expr;
  
  template<class F, class... X>
  struct expr<F(X...)>
  {
    static constexpr auto 
    arity = sizeof...(X);
  };
}
namespace garden
{ template<class...> class fn;

  template<class F, class... properties>
  struct fn<F, with<properties...>>
  {
    template<class... X>
    requires ( ... and models<F(X...), properties> )
    constexpr auto operator()
    (X... x) const -> auto
    {
      return eval( x... );
    }

    template<class... X>
    constexpr auto operator()
    (std::tuple<X...> x) const -> auto
    {
      return std::apply( *this, x );
    }

    template<class... X>
    constexpr auto operator()
    (std::variant<X...> x) const -> auto
    {
      return std::visit( *this, x );
    }

  private:

    template<class... X>
    requires BetaReducible<F(X...)>
    constexpr auto eval
    (X... x) const -> auto
    {
      auto& f = static_cast<const F&>(*this);

      ( ... , properties::assume( f, x... ) );

      auto y = f.eval( x... );

      ( ... , properties::ensure( f, y, x... ) );

      return y;
    }

    template<class... X>
    struct bound_fn
    {
      constexpr auto eval
      (auto... y) const -> auto
      {
        return std::apply( f,
          std::tuple_cat( x,
            std::tuple{ y... }
          ) 
        );
      }

      constexpr auto operator()
      (auto... y) const -> auto
      {
        return eval( y... );
      }

      F f; std::tuple<X...> x;
    };

    template<class... X>
    requires not BetaReducible<F(X...)>
    constexpr auto eval
    (X... x) const -> bound_fn<X...>
    {
      auto& f = static_cast<const F&>(*this);

      return { f, { x... } };
    }
  };

  template<class F>
  struct fn<F> : fn<F, with<>>{};
}
namespace garden
{ // function regularization
  template<class F>
  struct reg_fn
  : fn<reg_fn<F>>
  {
    constexpr
    reg_fn(F f): f( f ){}

    template<class... X>
    requires std::is_invocable_v<F, X...>
    constexpr auto eval
    (X... x) const -> auto
    {
      return f( x... );
    }

  private:

    F f;
  };

  template<class F>
  requires not requires
  { F::eval; }
  static constexpr auto 
  functional(F f) -> auto
  {
    return reg_fn<F>{ f };
  }

  template<class F>
  requires requires
  { F::eval; }
  static constexpr auto 
  functional(F f) -> F
  {
    return f;
  }
}
namespace garden
{ template<class...> class combinator_fn;

  template<class F, class... properties>
  struct combinator_fn<F, with<properties...>>
  : fn<F, with<properties...>>
  {
    template<class... G>
    constexpr auto operator()
    (G... g) const -> auto
    {
      return fn<F, with<properties...>>
      ::operator()( functional( g )... );
    }
  };

  template<class F>
  struct combinator_fn<F>
  : combinator_fn<F, with<>>{};
}
namespace garden
{ // composition
  template<class... F>
  struct composed_fn
  : fn<composed_fn<F...>>
  {
    constexpr
    composed_fn(F... f)
    : f( f... ){}

    template<class... G, class... H>
    requires std::is_same_v<
      std::tuple<F...>,
      std::tuple<G..., H...>
    >
    constexpr
    composed_fn(composed_fn<G...> g, H... h)
    : f( std::tuple_cat(
      g.f, std::tuple{ h... }
    ) ){}

    template<class... X>
    requires BetaReducible<
      ith_t<sizeof...(F)-1,
        F...
      >(X...)
    >
    constexpr auto eval
    (X... x) const -> auto
    {
      return evaluate(
        std::make_index_sequence<sizeof...(F)>(),
        std::tuple{ x... }
      );
    }

  private:

    template<class X, auto i, auto... ns>
    constexpr auto evaluate
    (std::index_sequence<i, ns...>, X x) const -> auto
    {
      return evaluate( 
        std::index_sequence<ns...>{},
        std::get<sizeof...(ns)>( f )( x )
      );
    }

    template<class X>
    constexpr auto evaluate
    (std::index_sequence<>, X x) const -> auto
    {
      return x;
    }

    std::tuple<F...> f;
  };

  template<class... F>
  composed_fn(F...) -> composed_fn<F...>;

  template<class... F, class... G>
  composed_fn(composed_fn<F...>, G...) -> composed_fn<F..., G...>;

  struct compose_fn
  : combinator_fn<compose_fn>
  {
    static constexpr auto eval
    (auto... f) -> auto
    {
      return composed_fn( f... );
    }
  };

}
namespace garden
{ // adjoinment
  template<class... F>
  struct adjoined_fn
  : fn<adjoined_fn<F...>>
  {
    constexpr
    adjoined_fn(F... f)
    : f( f... ){}

    template<class... G, class... H>
    requires std::is_same_v<
      std::tuple<F...>,
      std::tuple<G..., H...>
    >
    constexpr
    adjoined_fn(adjoined_fn<G...> g, H... h)
    : f( std::tuple_cat(
      g.f, std::tuple{ h... }
    ) ){}

    template<class... X>
    constexpr auto eval
    (X... x) const -> auto
    {
      return evaluate(
        std::make_index_sequence<sizeof...(F)>(),
        std::tuple{ x... }
      );
    }

  private:

    template<class X, auto... ns>
    constexpr auto evaluate
    (std::index_sequence<ns...>, X x) const -> auto
    {
      return std::tuple
      { std::get<ns>( f )( x )... };
    }

    std::tuple<F...> f;
  };

  template<class... F>
  adjoined_fn(F...) -> adjoined_fn<F...>;

  template<class... F, class... G>
  adjoined_fn(adjoined_fn<F...>, G...) -> adjoined_fn<F..., G...>;

  struct adjoin_fn
  : combinator_fn<adjoin_fn>
  {
    static constexpr auto eval
    (auto... f) -> auto
    {
      return adjoined_fn( f... );
    }
  };
}
namespace garden
{ // matching
  template<class... F>
  struct matched_fn
  : fn<matched_fn<F...>>
  {
    constexpr
    matched_fn(F... f)
    : f( f... ){}

    template<class... G, class... H>
    requires std::is_same_v<
      std::tuple<F...>,
      std::tuple<G..., H...>
    >
    constexpr
    matched_fn(matched_fn<G...> g, H... h)
    : f( std::tuple_cat(
      g.f, std::tuple{ h... }
    ) ){}

    template<class... X>
    constexpr auto eval
    (X... x) const -> auto
    {
      return evaluate( x... );
    }

  private:

    template<auto i = 0, class... X>
    requires BetaReducible<
      ith_t<i, F...>(X...)
    >
    constexpr auto evaluate
    (X... x) const -> auto
    {
      return std::invoke(
        std::get<i>( f ), x...
      );
    }

    template<auto i = 0, class... X>
    constexpr auto evaluate
    (X... x) const -> auto
    {
      return evaluate<i+1>( x... );
    }

    std::tuple<F...> f;
  };

  template<class... F>
  matched_fn(F...) -> matched_fn<F...>;

  template<class... F, class... G>
  matched_fn(matched_fn<F...>, G...) -> matched_fn<F..., G...>;

  struct match_fn
  : combinator_fn<match_fn>
  {
    static constexpr auto eval
    (auto... f) -> auto
    {
      return matched_fn( f... );
    }
  };
}
namespace garden
{ // basic combinators
  static constexpr auto
  compose = compose_fn{};
  static constexpr auto
  adjoin = adjoin_fn{};
  static constexpr auto
  match = match_fn{};
}
namespace garden
{ // functional operators
  constexpr auto operator^
  (auto x, auto f) -> auto
  { return fn{ f }( x ); }
  constexpr auto operator<<
  (auto f, auto g) -> auto
  { return compose( f, g ); }
  constexpr auto operator>>
  (auto f, auto g) -> auto
  { return compose( g, f ); }
  constexpr auto operator&
  (auto f, auto g) -> auto
  { return adjoin( f, g ); }
  constexpr auto operator|
  (auto f, auto g) -> auto
  { return match( f, g ); }
}
namespace garden
{ // some properties
  template<size_t n>
  struct arity : property
  {
    template<class Fx>
    requires expr<Fx>::arity <= n
    and ( 
      ( expr<Fx>::arity == 0 )
      == ( n == 0 )
    )
    and ( 
      ( expr<Fx>::arity == n )
      == BetaReducible<Fx>
    )
    class proof{};
  };

  struct commutativity : property
  {
    template<class F, class X>
    static constexpr auto
    ensure(F f, X y, X a, X b)
    {
      if( y != f.eval( b,a ) )
        throw property::failed<commutativity>
        { __PRETTY_FUNCTION__ };
    }
  };
}
