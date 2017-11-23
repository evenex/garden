#pragma once
#include<garden/functional.tcc>

namespace garden
{
  namespace functor
  { template<class> class instance; }

  template<class F>
  concept bool Functor = requires
  {
    functor::instance<F>::transform;
  };

  namespace functor
  {
    struct transform_fn
    : Lifted<transform_fn>
    {
      let eval
      (auto f, Functor x) -> Functor
      {
        return functor::instance<
          std::decay_t<decltype(x)>
        >::transform( f, x );
      }

      let assume
      (auto){}
      let assume
      (auto, auto){}
    };
    let transform = transform_fn{};
  }
}
namespace garden
{
  let operator
  %(auto f, Functor x) -> Functor
  {
    return functor::transform( f, x );
  }
}
#include<optional>
namespace garden
{ // some example instances

  template<class X>
  struct functor::instance<X*>
  {
    struct transform_fn
    : Fn<transform_fn>
    {
      template<class F>
       let eval
      (F f, X* x) -> std::result_of_t<F(X)>*
      {
        if( x == nullptr )
          return nullptr;
        else
          return new std::result_of_t<F(X)>
          { f( *x ) };
      }
    };
    let transform = transform_fn{};
  };

  template<class X>
  struct functor::instance<std::optional<X>>
  {
    struct transform_fn
    : Fn<transform_fn>
    {
      template<class F>
      let eval
      (F f, std::optional<X> x) -> std::optional<std::result_of_t<F(X)>>
      {
        if( x )
          return std::optional( f( *x ) );
        else
          return {};
      }
    };
    let transform = transform_fn{};
  };

  template<class X>
  struct functor::instance<std::vector<X>>
  {
    struct transform_fn
    : Fn<transform_fn>
    {
      template<class F>
      let eval
      (F f, std::vector<X> x) -> std::vector<auto>
      {
        auto y = std::vector<std::result_of_t<F(X)>>
        { x.size() };

        std::transform(
          x.begin(), x.end(),
          y.begin(), f
        );

        return y;
      }
    };
    let transform = transform_fn{};
  };
}
namespace garden
{ // or_default TODO move
  struct or_default_fn
  : Fn<or_default_fn>
  {
    let assume
    (auto){}
    let assume
    (auto, std::optional<auto>){}

    template<class X>
    let eval
    (X x, std::optional<X> mx) -> X
    {
      if( mx )
        return *mx;
      else
        return x;
    }

    template<class X>
    requires std::is_constructible_v<
      X
    >
    let eval
    (std::optional<X> mx) -> X
    {
      return eval( X{}, mx );
    }
  };
  let or_default = or_default_fn{};
}
