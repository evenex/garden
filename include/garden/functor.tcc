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
    : lifted_fn<transform_fn, arity<2>>
    {
      let eval
      (auto f, Functor x) -> Functor
      {
        return functor::instance<
          std::decay_t<decltype(x)>
        >::transform( f, x );
      }
    };
    let transform = transform_fn{};
  }
}
#include<optional>
namespace garden
{ // some example instances

  template<class X>
  struct functor::instance<X*>
  {
    struct transform_fn
    : fn<transform_fn>
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
    : fn<transform_fn>
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
    : fn<transform_fn>
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
