#pragma once
#include<functional>
#include<tuple>
#include<variant>
#include<map>
#include<garden/proof.tcc>

namespace garden
{ // typelist utils
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
{ // expressions
  template<class> 
  struct is_expression
  { let value = false; };

  template<class F, class... X> 
  struct is_expression<F(X...)>
  { let value = true; };

  template<class Fx>
  let is_expression_v = is_expression<Fx>::value;
}
namespace garden::expr
{ // expression traits
  template<class> struct symbol;
  template<class> struct arity;
  template<class> struct domain;
  template<class> struct codomain;

  template<class F, class... X>
  struct symbol<F(X...)>
  { using type = F; };
  template<class F, class... X>
  struct arity<F(X...)>
  { let value = sizeof...(X); };
  template<class F, class... X>
  struct domain<F(X...)>
  { using type = std::tuple<X...>; };
  template<class F, class... X>
  struct codomain<F(X...)>
  { using type = std::result_of_t<F(X...)>; };

  template<class Fx>
  using symbol_t = typename symbol<Fx>::type;
  template<class Fx>
  let arity_v = arity<Fx>::value;
  template<class Fx>
  using domain_t = typename domain<Fx>::type;
  template<class Fx>
  using codomain_t = typename codomain<Fx>::type;
}
namespace garden
{ template<class...> class fn;

  struct no_partial_application : property {};
  struct no_structural_decomposition : property {};
  struct no_pattern_matching : property {};

  template<class F, class... properties>
  struct fn<F, properties...>
  {
    struct has
    {
      let partial_application = not (
        ... or std::is_same_v<
          no_partial_application, properties
        >
      );
      let structural_decomposition = not (
        ... or std::is_same_v<
          no_structural_decomposition, properties
        >
      );
      let pattern_matching = not (
        ... or std::is_same_v<
          no_structural_decomposition, properties
        >
      );
    };

    template<class... X>
    requires ( ... and models<F(X...), properties> )
    constexpr auto operator()
    (X... x) const -> auto
    {
      return eval( x... );
    }

    template<class... X>
    requires has::structural_decomposition
    and std::is_invocable_v<F, X...>
    constexpr auto operator()
    (std::tuple<X...> x) const -> auto
    {
      return std::apply( *this, x );
    }

    template<class... X>
    requires has::pattern_matching
    and sizeof...(X) > 0
    and ( ... and std::is_invocable_v<F, X> )
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
    requires has::partial_application
    and not BetaReducible<F(X...)>
    constexpr auto eval
    (X... x) const -> bound_fn<X...>
    {
      auto& f = static_cast<const F&>(*this);

      return { f, { x... } };
    }
  };
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

    constexpr auto operator=
    (reg_fn&& g) -> decltype(auto)
    {
      new(&f) F{ g.f };
      return *this;
    }

    constexpr reg_fn(const reg_fn& g)
    : f( g.f ){}

  private:

    F f;
  };

  template<class F>
  requires not requires
  { F::eval; }
  let functional
  (F f) -> auto
  {
    return reg_fn<F>{ f };
  }

  template<class F>
  requires requires
  { F::eval; }
  let functional
  (F f) -> F
  {
    return f;
  }
}
namespace garden
{ template<class...> class combinator_fn;

  template<class F, class... properties>
  struct combinator_fn<F, properties...>
  : fn<F, properties...
    , no_partial_application
  >
  {
    using fn_t = fn<F, properties...
      , no_partial_application
    >;

    template<class... G>
    constexpr auto operator()
    (G... g) const -> auto
    {
      return fn_t::operator()(
        functional( g )...
      );
    }
  };
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
    let eval
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
    let eval
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
    let eval
    (auto... f) -> auto
    {
      return matched_fn( f... );
    }
  };
}
namespace garden
{ // basic combinators
  let compose = compose_fn{};
  let adjoin = adjoin_fn{};
  let match = match_fn{};
}
namespace garden
{ // functional operators
  constexpr auto operator^
  (auto&& x, auto f) -> auto
  { return f( x ); }

  template<class F, class G>
  requires std::is_copy_constructible_v<F>
  constexpr auto operator<<
  (F f, G g) -> auto
  { return compose( f, g ); }

  template<class F, class G>
  requires std::is_copy_constructible_v<F>
  constexpr auto operator>>
  (F f, G g) -> auto
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
  struct max_arity : property
  {
    template<class Fx>
    requires expr::arity_v<Fx> <= n
    and ( 
      ( expr::arity_v<Fx> == 0 )
      == ( n == 0 )
    )
    and ( 
      ( expr::arity_v<Fx> == n )
      == BetaReducible<Fx>
    )
    class proof{};
  };

  using unary = max_arity<1>;

  struct commutativity : property
  {
    template<class F, class X>
    let ensure
    (F f, X y, X a, X b) -> void
    {
      if( y != f.eval( b,a ) )
        throw property::failed<commutativity>
        { __PRETTY_FUNCTION__ };
    }
  };

  template<auto i, template<class...> class trait, class... TraitArgs>
  struct arg : property
  {
    template<class... X>
    requires trait<ith_t<i, X...>, TraitArgs...>::value
    class proof{};
  };

  template<template<class...> class trait, class... TraitArgs>
  struct all_args : property
  {
    template<class... X>
    requires ( ... and trait<X, TraitArgs...>::value )
    class proof{};
  };

  template<template<class...> class trait, class... TraitArgs>
  struct any_args : property
  {
    template<class... X>
    requires ( ... or trait<X, TraitArgs...>::value )
    class proof{};
  };
}
namespace garden
{ template<class...> struct memoized_fn;

  template<class F, class... properties, class Y, class... X>
  struct memoized_fn<Y(X...), F, properties...>
  : fn<memoized_fn<Y(X...), F, properties...>
    , properties...
  >
  {
    auto eval
    (X... x) const -> Y
    {
      if( auto found 
        = memoized.find( std::tuple<X...>( x... ) );
        found != memoized.end()
      )
      {
        return found->second;
      }
      else
      {
        auto& f = static_cast<const F&>( *this );

        return memoized.insert(
          { std::tuple<X...>( x... ), f.eval( x... ) }
        ).first->second;
      }
    }

  private:
    
    inline static auto 
    memoized = std::map<
      std::tuple<X...>, Y
    >{};
  };
}
namespace garden
{ template<class...> class lifted_fn;

  template<class F, class... properties>
  struct lifted_fn<F, properties...>
  : fn<F, properties...>
  {
    template<class G, class... X>
    constexpr auto operator()
    (G g, X... x) const -> auto
    {
      return fn<F, properties...>
      ::operator()( functional( g ), x... );
    }
  };
}
