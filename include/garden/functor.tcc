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
      static constexpr auto eval
      (auto f, Functor x) -> Functor
      {
        return functor::instance<
          std::decay_t<decltype(x)>
        >::transform( f, x );
      }
    };
    static constexpr auto
    transform = transform_fn{};
  }
}
namespace garden::functor
{ // some example instances
  template<class X>
  struct instance<X*>
  {
    struct transform_fn
    : fn<transform_fn>
    {
      template<class F>
      static constexpr auto eval
      (F f, X* x) -> std::result_of_t<F(X)>*
      {
        if( x == nullptr )
          return nullptr;
        else
          return new std::result_of_t<F(X)>
          { f( *x ) };
      }
    };
    static constexpr auto
    transform = transform_fn{};
  };
}
