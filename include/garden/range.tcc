#pragma once
#include<any>
#include<memory>
#include<optional>
#include<garden/functional.tcc>
#include<garden/functor.tcc>
namespace garden
{ // concept
  namespace range
  { template<class> class instance; }
  namespace range
  {
    template<class R> 
    requires 
    requires(const R r, R r_mut)
    {
      { r.front() } -> std::any;
      { r.empty() } -> bool;
      { r_mut.advance() } -> R&;
    }
    struct instance<R>
    {
      let front
      (R r) -> auto
      {
        return r.front();
      }
      let empty
      (R r) -> auto
      {
        return r.empty();
      }
      let advance
      (R& r) -> auto&
      {
        return r.advance();
      }
    };
  }

  template<class R, class... P>
  concept bool Range = requires
  {
    requires
    requires(range::instance<R> i, const R r, R r_mut)
    {
      { i.front( r ) } -> std::any;
      { i.empty( r ) } -> bool;
      { i.advance( r_mut ) } -> R&;
    }
    and ( ... and std::is_constructible_v<P, R> );
  };
}
namespace garden::range
{ // exceptions
  template<Range R>
  struct Error : std::range_error
  { using std::range_error::range_error; };
}
namespace garden
{ // STL-interface
  template<Range R>
  auto operator*(R r) -> auto
  {
    using Instance = range::instance<R>;

    if( Instance::empty( r ) )
      throw range::Error<R>(
        "accessed empty range" 
      );

    return Instance::front( r );
  }
  template<Range R>
  auto operator!=(R r, nullptr_t) -> bool
  {
    return not range::instance<R>::empty( r );
  }
  template<Range R>
  auto operator++(R& r) -> R&
  {
    using Instance = range::instance<R>;

    if( Instance::empty( r ) )
      throw range::Error<R>(
        "advanced empty range" 
      );

    return Instance::advance( r );
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
{ // D-interface
  struct front_fn
  : Fn<front_fn>
  {
    let assume(Range){}

    let eval
    (auto r) -> auto
    {
      return *r;
    }
  };
  struct empty_fn
  : Fn<empty_fn>
  {
    let assume(Range){}

    let eval
    (auto r) -> bool
    {
      return not ( r != nullptr );
    }
  };

  let front = front_fn{};
  let empty = empty_fn{};
}
namespace garden::range
{ // item traits & concepts
  template<Range R>
  using item_t = decltype(
    instance<R>::front( std::declval<R>() )
  );

  template<class X>
  struct of
  {
    template<Range R>
    requires std::is_convertible_v<item_t<R>, X>
    constexpr
    of(R){}
  };
}
namespace garden::range
{ // equality
  template<Range A, Range B>
  auto operator==(A a, B b) -> bool
  {
    for( ;
      not( a^empty or b^empty );
      ++a, ++b
    )
      if( *a == *b )
        continue;
      else
        return false;

    return a^empty and b^empty;
  }
}
namespace garden::range
{ // from ptr
  struct with_contiguous_memory
  {
    template<Range R> 
    requires 
    requires(R r)
    { { r.data() } -> item_t<R>*; }
    with_contiguous_memory(R){}
  };

  template<class X>
  let from_ptr
  (X* x, size_t n) 
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
      auto advance() -> range_t&
      {
        ++x; --n;
        return *this;
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
{ // from_container
  struct from_container_fn
  : Fn<from_container_fn>
  {
    template<class C>
    struct range_t
    {
      range_t(C c)
      : ptr( std::make_shared<C>( c ) )
      , iter( ptr->begin() ) {}

      constexpr auto front() const
      {
        return *iter;
      }
      constexpr auto empty() const -> bool
      {
        return iter == ptr->end();
      }
      constexpr auto advance() -> range_t&
      {
        ++iter;
        return *this;
      }

    private:

      using It = decltype(std::declval<C>().begin());

      range_t(std::shared_ptr<C> ptr, It iter)
      : ptr( ptr ), iter( iter ) {}

      std::shared_ptr<C> ptr;
      It iter;
    };

    template<class C>
    let eval
    (C c) -> Range
    {
      return range_t<C>( c );
    }

    let assume(auto){}
  };
  let from_container = from_container_fn{};
}
namespace garden::range
{ // to_container
  template<template<class...> class C>
  struct to_container_fn
  : Fn<to_container_fn<C>>
  {
    let assume(Range){}

    template<class R>
    let eval
    (R r) -> C<item_t<R>>
    {
      C<item_t<R>> c;

      for( auto x : r )
        c.push_back( x );

      return c;
    }
  };

  template<template<class...> class C>
  let to_container = to_container_fn<C>{};
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
    auto empty() const -> bool
    {
      return false;
    }
    auto advance() -> Recurrence&
    {
      x = f( x );
      return *this;
    }

    F f; X x;
  };

  template<class F, class X>
  Recurrence(F,X) -> Recurrence<F,X>;

  struct recurrence_fn
  : Lifted<recurrence_fn>
  {
    let assume(auto){}
    let assume(auto,auto){}

    let eval
    (auto f, auto x)
    {
      return Recurrence{ f, x };
    }
  };
  let recur = recurrence_fn{};
}
namespace garden::range
{ // drop/take
  template<Range R>
  struct Take
  {
    R r; size_t n;

    constexpr auto front() const -> item_t<R>
    {
      return *r;
    }
    constexpr auto empty() const -> bool
    {
      return n == 0 or r.empty();
    }
    constexpr auto advance() -> Take&
    {
      ++r; --n;
      return *this;
    }
  };

  template<class R>
  Take(R, size_t) -> Take<R>;
  
  struct take_fn
  : Fn<take_fn>
  {
    let assume(size_t){}
    let assume(size_t, Range){}

    let eval
    (size_t n, auto r) -> Range
    {
      return Take{ r, n };
    }
  };

  struct drop_fn
  : Fn<drop_fn>
  {
    let assume(size_t){}
    let assume(size_t, Range){}

    let eval
    (size_t n, auto r) -> Range
    {
      while( n > 0 and not( r^empty ) )
        --n, ++r;

      return r;
    }
  };

  let drop = drop_fn{};
  let take = take_fn{};
}
namespace garden::range
{ // take until
  template<class F, Range R>
  struct TakeUntil
  {
    constexpr
    TakeUntil(F f, R r)
    : f( f ), r( r ){}

    constexpr auto front() const -> item_t<R>
    {
      return *r;
    }
    constexpr auto empty() const -> bool
    {
      return r.empty() or f( *r );
    }
    constexpr auto advance() -> TakeUntil&
    {
      ++r;

      return *this;
    }

  private:
    
    F f; R r;
  };

  template<class F, Range R>
  TakeUntil(F,R) -> TakeUntil<F,R>;

  struct take_until_fn
  : Lifted<take_until_fn>
  {
    let assume(auto){}
    let assume(auto, Range){}

    let eval
    (auto f, auto r) -> Range
    {
      return TakeUntil( f,r );
    }
  };
  let take_until = take_until_fn{};
}
namespace garden::range
{ // transform

  template<class F, Range R>
  struct Transformed
  {
    R r; F f;

    constexpr auto front() const -> auto
    {
      return f( *r );
    }
    constexpr auto empty() const -> bool
    {
      return r.empty();
    }
    constexpr auto advance() -> Transformed&
    {
      ++r;
      return *this;
    }
  };

  template<class F, class R>
  Transformed(R,F) -> Transformed<F,R>;
  
  struct transform_fn
  : Lifted<transform_fn>
  {
    let assume(auto){}
    let assume(auto, Range){}

    let eval
    (auto f, auto r) -> Range
    {
      return Transformed{ r, f };
    }
  };

  let transform = transform_fn{};
}
namespace garden
{ // functor
  template<Range R>
  struct functor::instance<R>
  {
    let transform = range::transform;
  };
}
namespace garden::range
{ // only
  template<class X, size_t n>
  struct Only : std::array<X,n>
  {
    Only(auto... x)
    : std::array<X,n>({ x... })
    , i( 0 ){}

    constexpr auto front() const -> X
    {
      return (*this)[i];
    }
    constexpr auto empty() const -> bool
    {
      return i == n;
    }
    constexpr auto advance() -> Only&
    {
      ++i;
      return *this;
    }
    constexpr auto begin() -> auto
    {
      return std::array<X,n>::begin() + i;
    }

  private:

    size_t i;
  };

  template<class... X>
  Only(X...) -> Only<std::common_type_t<X...>, sizeof...(X)>;

  struct only_fn
  : Fn<only_fn, no_partial_application>
  {
    template<class... X>
    let eval
    (X... x) -> Range
    {
      return Only
      { static_cast<std::common_type_t<X...>>( x )... };
    }
  };

  let only = only_fn{};
}
namespace garden
{ // applicative

  namespace applicative
  { template<class> class instance; }
  
  template<Range R>
  struct applicative::instance<R>
  {
    let pure = range::only;
    let apply = 0;
  };
}
namespace garden::range
{ // chain
  template<Range... R>
  struct Chain
  {
    constexpr
    Chain(R... r)
    : rs( r... ){}

    template<auto i = 0>
    requires i < sizeof...(R)
    auto front() const -> std::common_type_t<item_t<R>...>
    {
      auto& r = std::get<i>( rs );

      if( r.empty() )
        return front<i+1>();
      else
        return *r;
    }
    template<int=0>
    auto front() const -> std::common_type_t<item_t<R>...>
    {
      throw std::range_error( "front out of bounds" );
    }

    template<auto i = 0>
    requires i < sizeof...(R)
    constexpr auto empty
    () const -> bool
    {
      auto& r = std::get<i>( rs );

      if( r.empty() )
        return empty<i+1>();
      else
        return false;
    }
    template<int=0>
    constexpr auto empty
    () const -> bool
    {
      return true;
    }

    template<auto i = 0>
    requires i < sizeof...(R)
    constexpr auto advance
    () -> Chain&
    {
      auto& r = std::get<i>( rs );

      if( r^range::empty )
        return advance<i+1>();
      else
        ++r;

      return *this;
    }
    template<int=0>
    constexpr auto advance
    () -> Chain&
    {
      throw std::range_error( "next out of bounds" );
    }

  private:

    constexpr 
    Chain(std::tuple<R...> rs)
    : rs( rs ){}

    std::tuple<R...> rs;
  };

  template<class... R>
  Chain(R...) -> Chain<R...>;

  struct chain_fn
  : Fn<chain_fn, no_partial_application>
  {
    let eval
    (auto... r) -> Range
    {
      return Chain( r... );
    }
  };
  let chain = chain_fn{};
}
namespace garden::range
{ // join
  template<Range R>
  requires Range<item_t<R>>
  struct Join
  {
    Join(R ranges)
    : rs( ranges )
    {
      while( not( rs^range::empty ) )
      {
        r = *rs;
        ++rs;

        if( not r->empty() )
          break;
      }

      if( r and r->empty() )
        r = {};
    }

    auto front() const -> item_t<item_t<R>>
    {
      return **r;
    }
    auto empty() const -> bool
    {
      return not r;
    }
    auto advance() -> Join&
    {
      r->advance();

      while( r->empty() and not rs.empty() )
      {
        r = *rs;
        ++rs;
      }

      if( r->empty() )
        r = {};

      return *this;
    }

  private:

    R rs;
    std::optional<item_t<R>> r;
  };

  template<class R>
  Join(R) -> Join<R>;

  struct join_fn
  : Fn<join_fn, no_partial_application>
  {
    let eval
    (auto r) -> Range
    {
      return Join( r );
    }
  };
  let join = join_fn{};
}
namespace garden
{ // common predicates
  struct equal_to_fn
  : Fn<equal_to_fn>
  {
    let assume(auto){}
    let assume(auto,auto){}

    let eval
    (auto a, auto b) -> bool
    {
      return a == b;
    }
  };
  let equal_to = equal_to_fn{};
}
namespace garden::range
{ // split
  template<Range R>
  struct Split
  {
    constexpr auto front() const -> Range
    {
      return cursor;
    }
    constexpr auto empty() const -> bool
    {
      return r.empty();
    }
    constexpr auto advance() -> Split&
    {
      for( auto _ : cursor )
        ++r;

      if( r != nullptr )
        ++r;

      cursor = take_until( equal_to( delim ), r );

      return *this;
    }

    constexpr
    Split(item_t<R> d, R r)
    : delim( d ), r( r )
    , cursor( 
      take_until( equal_to( delim ), r )
    ) {}

  private:

    item_t<R> delim;
    R r;
    TakeUntil<
      equal_to_fn::Bind<item_t<R>>,
      R
    > cursor;
  };

  template<class D, class R>
  Split(D, R) -> Split<R>;

  struct split_fn
  : Fn<split_fn>
  {
    let assume(auto){}
    template<Range R>
    let assume(item_t<R>, R){}

    template<class R>
    let eval
    (item_t<R> delim, R r) -> Range
    {
      return Split( delim, r );
    }
  };
  let split = split_fn{};
}
#include<iostream>
namespace garden::range
{ // fold
  struct foldl_fn
  : Fn<foldl_fn>
  {
    let eval
    (auto f, Range r) -> auto
    {
      auto z = *r;

      return eval( f, z, ++r );
    }

    let eval
    (auto f, auto z, Range r) -> auto
    {
      for( auto x : r )
        z = f( z, x );

      return z;
    }
  };
  let foldl = foldl_fn{};
}
#include<map>
#include<set>
namespace garden // TODO searchable concept?
{ // container functions
  struct contains_fn
  : Fn<contains_fn>
  {
    let assume(auto){}
    let assume(auto, auto){}

    template<class X>
    let eval
    (X x, std::map<X, auto> in) -> bool
    {
      return in.count( x );
    }

    template<class X>
    let eval
    (X x, std::set<X> in) -> bool
    {
      return in.count( x );
    }
  };

  struct find_fn
  : Fn<find_fn>
  {
    let assume(auto){}
    let assume(auto, auto){}

    template<class X, class Y>
    let eval
    (X x, std::map<X, Y> in) -> std::optional<Y>
    {
      if( auto it = in.find( x );
        it != in.end() 
      )
        return it->second;
      else
        return {};
    }
  };

  let contains = contains_fn{};
  let find = find_fn{};
}
namespace garden::range
{ // maybe_front
  struct maybe_front_fn
  : Fn<maybe_front_fn>
  {
    let assume(Range){}

    template<class R>
    let eval
    (R r) -> std::optional<item_t<R>>
    {
      if( r^empty )
        return {};
      else
        return r^front;
    }
  };
  let maybe_front = maybe_front_fn{};
}
