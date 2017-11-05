#pragma once
#include<any>
#include<garden/functional.tcc>
#include<garden/functor.tcc>
namespace garden
{
  template<class R, Property... desc>
  concept bool Range = requires
  (const R r)
  {
    { r.front() } -> std::any;
    { r.next() } -> R;
    { r.is_empty() } -> bool;
  }
  and ( ... and models<R, desc> );
}
namespace garden::range
{
  template<Range R>
  using item_t = decltype( std::declval<R>().front() );
}
namespace garden::range
{ // D-interface
  struct front_fn
  : fn<front_fn, arity<1>>
  {
    static constexpr auto eval
    (Range r) -> auto
    {
      if( r.is_empty() )
        throw std::range_error(
          "accessed empty range" 
        );

      return r.front();
    }
  };

  struct next_fn
  : fn<next_fn, arity<1>>
  {
    static constexpr auto eval
    (Range r) -> Range
    {
      if( r.is_empty() )
        throw std::range_error(
          "iterated empty range" 
        );

      return r.next();
    }
  };

  struct is_empty_fn
  : fn<is_empty_fn, arity<1>>
  {
    static constexpr auto eval
    (Range r) -> bool
    {
      return r.is_empty();
    }
  };

  static constexpr auto
  front = front_fn{};
  static constexpr auto
  next = next_fn{};
  static constexpr auto
  is_empty = is_empty_fn{};
}
namespace garden
{ // STL-interface
  auto operator*(Range r) -> auto
  {
    return r^range::front;
  }
  auto operator!=(Range r, nullptr_t) -> auto
  {
    return not( r^range::is_empty );
  }
  auto operator++(Range& r) -> Range&
  {
    return r = r^range::next;
  }
  namespace range
  {
    auto begin(Range& r) -> Range&
    {
      return r;
    }
    auto end(Range) -> nullptr_t
    {
      return {};
    }
  }
}
namespace garden::range
{ // equality
  template<Range A, Range B>
  auto operator==(A a, B b) -> bool
  {
    for( ;
      not( a^is_empty or b^is_empty );
      a = a^next, b = b^next
    )
      if( ( a^front ) == ( b^front ) )
        continue;
      else
        return false;

    return a^is_empty and b^is_empty;
  }
}
namespace garden::range
{ // from ptr
  struct contiguous_memory : property
  {
    template<Range X> 
    requires 
    requires(X x)
    { { x.data() } -> item_t<X>*; }
    class proof{};
  };

  template<class X>
  auto from_ptr(X* x, size_t n) 
  -> Range<contiguous_memory>
  {
    struct range_t
    {
      auto front() const -> X
      {
        return *x;
      }
      auto is_empty() const -> bool
      {
        return n == 0;
      }
      auto next() const -> range_t
      {
        return { x+1, n-1 };
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
namespace garden::range
{ // recurrence
  template<class F, class X>
  struct Recurrence
  {
    auto front() const -> X
    {
      return x;
    }
    auto next() const -> Recurrence
    {
      return { f, f( x ) };
    }
    auto is_empty() const -> bool
    {
      return false;
    }

    F f; X x;
  };

  template<class F, class X>
  Recurrence(F,X) -> Recurrence<F,X>;

  struct recurrence_fn
  : lifted_fn<recurrence_fn, arity<2>>
  {
    static constexpr auto eval
    (auto f, auto x)
    {
      return Recurrence{ f, x };
    }
  };
  static constexpr auto
  recur = recurrence_fn{};
}
namespace garden::range
{ // drop/take

  template<Range R>
  struct Take
  {
    R r; size_t n;

    constexpr auto front() const -> item_t<R>
    {
      return r^range::front;
    }
    constexpr auto next() const -> Take
    {
      return { r^range::next, n-1 };
    }
    constexpr auto is_empty() const -> bool
    {
      return n == 0 or r^range::is_empty;
    }
  };
  
  struct take_fn
  : fn<take_fn, arity<2>>
  {
    template<Range R> 
    static constexpr auto eval
    (size_t n, R r) -> Range
    {
      return Take<R>{ r, n };
    }
  };

  struct drop_fn
  : fn<drop_fn, arity<2>>
  {
    static constexpr auto eval
    (size_t n, Range r) -> Range
    {
      for(; n > 0 and not( r^range::is_empty ); --n )
        r = r^range::next;

      return r;
    }
  };

  static constexpr auto
  drop = drop_fn{};
  static constexpr auto
  take = take_fn{};
}
namespace garden::range
{ // transform

  template<class F, Range R>
  struct Transformed
  {
    R r; F f;

    constexpr auto front() const -> auto
    {
      return f( r^range::front );
    }
    constexpr auto next() const -> Transformed
    {
      return { r^range::next, f };
    }
    constexpr auto is_empty() const -> bool
    {
      return r^range::is_empty;
    }
  };

  template<class F, class R>
  Transformed(R,F) -> Transformed<F,R>;
  
  struct transform_fn
  : lifted_fn<transform_fn, arity<2>>
  {
    template<Range R> 
    static constexpr auto eval
    (auto f, R r) -> Range
    {
      return Transformed{ r, f };
    }
  };

  static constexpr auto
  transform = transform_fn{};
}
namespace garden
{ // functor
  template<Range R>
  struct functor::instance<R>
  {
    static constexpr auto
    transform = range::transform;
  };
}
namespace garden::range
{ // only
  template<class X, auto n>
  struct Only : std::array<X,n>
  {
    Only(auto... x)
    : std::array<X,n>({ x... })
    , i( 0 ){}

    constexpr auto next() const -> Only
    {
      return { *this, i+1 };
    }
    constexpr auto is_empty() const -> bool
    {
      return i == n;
    }

    private:

    Only(const Only& that, size_t i)
    : std::array<X,n>( that )
    , i( i ){}

    size_t i;
  };

  template<class... X>
  Only(X...) -> Only<std::common_type_t<X...>, sizeof...(X)>;

  struct only_fn
  : fn<only_fn>
  {
    template<class... X>
    static constexpr auto eval
    (X... x) -> Range
    {
      return Only{ x... };
    }
  };

  static constexpr auto
  only = only_fn{};
}
namespace garden
{ // applicative

  namespace applicative
  { template<class> class instance; }
  
  template<Range R>
  struct applicative::instance<R>
  {
    static constexpr auto
    pure = range::only;
    static constexpr auto
    apply = 0;
  };
}
