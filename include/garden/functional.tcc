#pragma once
#include<functional>
#include<tuple>
#include<variant>
#include<garden/proof.tcc>


namespace garden
{
  template<class X>
  concept bool Defined = requires
  { sizeof(X); };
}
namespace garden
{ // β-redux

  template<class...>
  struct beta_redux;

  template<class F, class... X>
  requires requires
  (F f, X... xs)
  { f.eval( xs... ); }
  struct beta_redux<F(X...)>
  {
    using result_t = decltype(
      std::declval<F>()
      .eval( std::declval<X>()... )
    );
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
    requires Defined<beta_redux<F(X...)>>
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
      constexpr auto operator()
      (auto... y) const -> auto
      {
        return std::apply( f,
          std::tuple_cat( x,
            std::tuple( y... )
          ) 
        );
      }

      F f; std::tuple<X...> x;
    };

    template<class... X>
    requires not Defined<beta_redux<F(X...)>>
    constexpr auto eval
    (X... x) const -> bound_fn<X...>
    {
      auto& f = static_cast<const F&>(*this);

      return { f, { x... } };
    }
  };

  template<class F>
  struct fn<F> : fn<F, with<>>{};

  template<class Y, class... X>
  struct fn<Y(X...)>
  : fn<fn<Y(X...)>>
  {
    fn(std::function<Y(X...)> f)
    : f( f ){}

    constexpr auto eval
    (X... x) -> Y
    {
      return f( x... );
    }

  private:

    std::function<Y(X...)> f;
  };
}
namespace garden
{ // function regularization
  template<class Y, class... X>
  static constexpr auto 
  function(std::function<Y(X...)> f) -> auto
  {
    return fn<Y(X...)>( f );
  }

  template<class F>
  requires requires
  { F::eval; }
  static constexpr auto 
  function(F f) -> F
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
      ::operator()( function( g )... );
    }
  };

  template<class F>
  struct combinator_fn<F>
  : combinator_fn<F, with<>>{};
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
      == Defined<beta_redux<Fx>>
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
      g.f, std::tuple( h... )
    ) ){}

    template<class... X>
    requires Defined<beta_redux<
      std::tuple_element_t<
        sizeof...(F)-1,
        std::tuple<F...>
      >(X...)
    > >
    constexpr auto eval
    (X... x) const -> auto
    {
      return evaluate(
        std::make_index_sequence<sizeof...(F)>(),
        std::tuple( x... )
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

  struct compose_fn
  : combinator_fn<compose_fn>
  {
    static constexpr auto eval
    (auto... f) -> auto
    {
      return composed_fn( f... );
    }
  };

  static constexpr auto
  compose = compose_fn{};
}
namespace garden
{ // functional operators
  constexpr auto operator^
  (auto x, auto f) -> auto
  {
    return fn{ f }( x );
  }
  constexpr auto operator<<
  (auto f, auto g) -> auto
  {
    return compose( f, g );
  }
  constexpr auto operator>>
  (auto f, auto g) -> auto
  {
    return compose( g, f );
  }
}
