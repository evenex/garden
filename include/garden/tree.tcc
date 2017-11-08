#pragma once
#include<any>
#include<garden/functional.tcc>
#include<garden/range.tcc>

namespace garden
{
  template<class T, Property... desc>
  concept bool Tree = requires
  (const T t)
  {
    { t.top() } -> std::any;
    { t.down() } -> Range;
  }
  and ( ... and models<T, desc> );
}
namespace garden::tree
{
  
}
