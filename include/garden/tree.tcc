#pragma once
#include<any>
#include<garden/functional.tcc>
#include<garden/range.tcc>

namespace garden
{
  template<class T, class... P>
  concept bool Tree = requires
  (const T t)
  {
    { t.top() } -> std::any;
    { t.down() } -> Range;
  }
  and ( ... and std::is_constructible_v<P, T> );
}
namespace garden::tree
{
  
}
