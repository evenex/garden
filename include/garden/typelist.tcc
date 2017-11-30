#pragma once
#include<tuple>
#include<garden/let.tcc>

namespace garden
{
  template<auto i, class... X>
  requires i < sizeof...(X)
  using ith_t = std::tuple_element_t<
    i, std::tuple<X...>
  >;

  template<class, class...>
  struct contains_type;

  template<class X>
  struct contains_type<X>
  { let value = false; };

  template<class X, class... Y>
  struct contains_type<X, X, Y...>
  { let value = true; };

  template<class X, class Y, class... Z>
  struct contains_type<X, Y, Z...>
  : contains_type<X, Z...> {};

  template<class X, class... Y>
  let contains_type_v = contains_type<X, Y...>::value;
}
