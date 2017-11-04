#pragma once
#include<any>
#include<garden/functional.tcc>
namespace garden
{
  template<class R, template<class> class... traits>
  concept bool Range = requires
  (const R r, R s)
  {
    { r.front() } -> std::any;
    { s.next() } -> void;
    { r.empty() } -> bool;
  }
  and ( ... and traits<R>::value );

  auto operator*(Range r) -> auto
  {
    if( r.empty() )
      throw std::range_error(
        "accessed empty range" 
      );

    return r.front();
  }
  auto operator!=(Range r, nullptr_t) -> auto
  {
    return not r.empty();
  }
  auto operator++(Range& r) -> Range&
  {
    if( r.empty() )
      throw std::range_error(
        "iterated empty range" 
      );

    r.next();
    return r;
  }

  auto begin(Range& r) -> Range&
  {
    return r;
  }
  auto end(Range) -> nullptr_t
  {
    return {};
  }
}
namespace garden::range
{
  template<Range R>
  using item_t = decltype( std::declval<R>().front() );

  template<Range R> 
  struct with_contiguous_memory
  {
    template<class X>
    requires requires
    (X x){ { x.data() } -> item_t<R>*; }
    static constexpr auto get()
    {
      return true;
    }
    template<class>
    static constexpr auto get()
    {
      return false;
    }

    static constexpr auto value = get<R>();
  };

  template<class X>
  auto slice(X* x, size_t n) 
  -> Range<with_contiguous_memory>
  {
    struct range_t
    {
      auto front() const -> X
      {
        return *x;
      }
      auto empty() const -> bool
      {
        return n == 0;
      }
      void next()
      {
        ++x;
        --n;
      }
      auto data() -> X*
      {
        return x;
      }

      X* x; size_t n;
    };

    return range_t{ x, n };
  }
}
