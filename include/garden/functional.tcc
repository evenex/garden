#pragma once
#include<functional>
#include<tuple>
#include<variant>
#include<map>
#include<garden/let.tcc>
#include<garden/typelist.tcc>

namespace garden
{ template<class...> class Fn;

  struct evaluation
  {
    template<class F, class... X>
    requires 
    requires(F f, X... x)
    { f.eval( x... ); }
    constexpr evaluation
    (F, X...){}
  };
  struct assumption
  {
    template<class F, class... X>
    requires 
    requires(F f, X... x)
    { f.assume( x... ); }
    constexpr assumption
    (F f, X... x)
    { f.assume( x... ); }

    template<class F>
    requires 
    not requires
    { F::assume; }
    constexpr assumption
    (F, auto...){}
  };

  class no_partial_application {};
  class no_structural_decomposition {};
  class no_pattern_matching {};

  template<class F, class... options>
  struct Fn<F, options...>
  {
    template<class... X>
    requires std::is_constructible_v<
      assumption, F, X...
    >
    constexpr auto operator()
    (X... x) const -> auto
    {
      auto& f = static_cast<const F&>(*this);

      assumption( f, x... );

      return eval( x... );
    }

    template<class... X>
    requires not contains_type_v<
      no_structural_decomposition, options...
    >
    and std::is_constructible_v<
      evaluation, F, X...
    >
    constexpr auto operator()
    (std::tuple<X...> x) const -> auto
    {
      return std::apply( *this, x );
    }

    template<class... X>
    requires not contains_type_v<
      no_pattern_matching, options...
    >
    and sizeof...(X) > 0
    and ( ...  and std::is_constructible_v<
      evaluation, F, X
    > )
    constexpr auto operator()
    (std::variant<X...> x) const -> auto
    {
      return std::visit( *this, x );
    }

    template<class... X>
    requires std::is_constructible_v<
      evaluation, F, X...
    >
    constexpr auto eval
    (X... x) const -> auto
    {
      auto& f = static_cast<const F&>(*this);

      return f.eval( x... );
    }

    template<class... X>
    struct Bind
    : Fn<Bind<X...>>
    {
      Bind(F f, X... x)
      : f( f ), x( x... ){}

      constexpr auto eval
      (auto... y) const -> auto
      {
        return std::apply( f,
          std::tuple_cat( x,
            std::tuple{ y... }
          ) 
        );
      }

    private:

      F f; std::tuple<X...> x;
    };

    template<class... X>
    requires not contains_type_v<
      no_partial_application, options...
    >
    and not std::is_constructible_v<
      evaluation, F, X...
    >
    constexpr auto eval
    (X... x) const -> Bind<X...>
    {
      auto& f = static_cast<const F&>(*this);

      return { f, x... };
    }
  };
}
namespace garden
{ // function regularization
  template<class F>
  struct fn
  : Fn<fn<F>>
  {
    constexpr
    fn(F f): f( f ){}

    template<class... X>
    requires std::is_invocable_v<F, X...>
    constexpr auto eval
    (X... x) const -> auto
    {
      return f( x... );
    }

    constexpr auto operator=
    (fn&& g) -> decltype(auto)
    {
      new(&f) F{ g.f };
      return *this;
    }

    constexpr fn(const fn& g)
    : f( g.f ){}

  private:

    F f;
  };

  template<class F>
  requires not requires
  { F::eval; }
  let func
  (F f) -> auto
  {
    return fn<F>{ f };
  }

  template<class F>
  requires requires
  { F::eval; }
  let func
  (F f) -> F
  {
    return f;
  }
}
namespace garden
{ template<class...> class Lifted;

  template<class F, class... options>
  struct Lifted<F, options...>
  : Fn<F, options...>
  {
    using fn_t = Fn<F, options...>; 

    template<class G, class... X>
    constexpr auto operator()
    (G g, X... x) const -> auto
    {
      return fn_t::operator()(
        func( g ), x... 
      );
    }
  };
}
namespace garden
{ template<class...> class Combinator;

  template<class F, class... options>
  struct Combinator<F, options...>
  : Fn<F,
    options..., no_partial_application
  >
  {
    using fn_t = Fn<F, 
      options..., no_partial_application
    >;

    template<class... G>
    constexpr auto operator()
    (G... g) const -> auto
    {
      return fn_t::operator()(
        func( g )...
      );
    }
  };
}
namespace garden
{ // composition
  template<class... F>
  struct composed_fn
  : Fn<composed_fn<F...>>
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
    requires std::is_constructible_v<
      evaluation, 
      ith_t<sizeof...(F)-1,
        F...
      >, X...
    >
    constexpr auto eval
    (X... x) const -> auto
    {
      return do_eval(
        std::make_index_sequence<sizeof...(F)>(),
        std::tuple{ x... }
      );
    }

  private:

    template<class X, auto i, auto... ns>
    constexpr auto do_eval
    (std::index_sequence<i, ns...>, X x) const -> auto
    {
      return do_eval( 
        std::index_sequence<ns...>{},
        std::get<sizeof...(ns)>( f )( x )
      );
    }

    template<class X>
    constexpr auto do_eval
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
  : Combinator<compose_fn>
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
  : Fn<adjoined_fn<F...>>
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
      return do_eval(
        std::make_index_sequence<sizeof...(F)>(),
        std::tuple{ x... }
      );
    }

  private:

    template<class X, auto... ns>
    constexpr auto do_eval
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
  : Combinator<adjoin_fn>
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
  : Fn<matched_fn<F...>>
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
      return do_eval( x... );
    }

  private:

    template<auto i = 0, class... X>
    requires std::is_constructible_v<
      evaluation,
      ith_t<i, F...>, X...
    >
    constexpr auto do_eval
    (X... x) const -> auto
    {
      return std::invoke(
        std::get<i>( f ), x...
      );
    }

    template<auto i = 0, class... X>
    constexpr auto do_eval
    (X... x) const -> auto
    {
      return do_eval<i+1>( x... );
    }

    std::tuple<F...> f;
  };

  template<class... F>
  matched_fn(F...) -> matched_fn<F...>;

  template<class... F, class... G>
  matched_fn(matched_fn<F...>, G...) -> matched_fn<F..., G...>;

  struct match_fn
  : Combinator<match_fn>
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
  let operator
  ^(auto x, auto f) -> auto
  { return f( x ); }

  template<class F, class G>
  requires std::is_copy_constructible_v<F>
  let operator
  <<(F f, G g) -> auto
  { return compose( f, g ); }

  template<class F, class G>
  requires std::is_copy_constructible_v<F>
  let operator
  >>(F f, G g) -> auto
  { return compose( g, f ); }

  let operator
  &(auto f, auto g) -> auto
  { return adjoin( f, g ); }

  let operator
  |(auto f, auto g) -> auto
  { return match( f, g ); }
}

namespace garden
{ template<class...> struct Memoized;

  template<class F, class... options, class Y, class... X>
  struct Memoized<Y(X...), F, options...>
  : Fn<Memoized<Y(X...), F, options...>
    , options...
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
{ // constant fn
  struct constant_fn
  : Fn<constant_fn>
  {
    let assume(auto){}
    let assume(auto, auto){}

    let eval
    (auto x, auto) -> auto
    { return x; }
  };
  let constant = constant_fn{};
}
namespace garden
{ // identity fn
  struct identity_fn
  : Fn<identity_fn>
  {
    let assume(auto){}

    let eval
    (auto x) -> auto
    { return x; }
  };
  let identity = identity_fn{};
}

namespace garden
{ // concepts
  template<class F, class X>
  concept bool Operator = requires
  {
    requires requires(F f, X x)
    { { f(x) } -> X; };
  };
}
