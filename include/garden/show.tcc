#pragma once
#include<iostream>
#include<sstream>

namespace garden
{ // show/text
  struct show_fn
  {
    std::ostream& out;

    show_fn(std::ostream& out = std::cout)
    : out( out ){}

    auto operator()
    () -> show_fn
    {
      out << std::endl; 

      return *this;
    }

    auto operator()
    (auto x) -> show_fn
    {
      out << x;

      return *this;
    }

    auto operator()
    (auto x, auto... xs) -> show_fn
    {
      operator()( x ); 
      return operator()( xs... );
    }
  };
  static auto show = show_fn{};

  struct text_fn
  {
    auto operator()
    (auto... xs) const -> std::string
    {
      std::stringstream str;

      show_fn{ str }( xs... );

      return str.str();
    }
  };
  let text = text_fn{};
}
