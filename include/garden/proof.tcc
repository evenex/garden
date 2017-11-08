#pragma once
#include<type_traits>
#include<stdexcept>

#define let static constexpr auto

namespace garden
{
  struct property
  {
    virtual ~property() = 0;

    template<class...>
    class proof{};

    static constexpr void 
    assume(auto...){}

    static constexpr void 
    ensure(auto...){}

    template<class P>
    requires std::is_base_of_v<property, P>
    struct failed : std::logic_error
    { using std::logic_error::logic_error; };
  };

  template<class P>
  concept bool Property = requires
  {
    requires std::is_base_of_v<
      property, P
    >;
  };

  template<Property P, class... X>
  using proof = typename P::template proof<X...>;

  template<class X, Property P>
  concept bool models = requires
  { typename proof<P, X>; };
}
