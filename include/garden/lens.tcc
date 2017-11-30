#pragma once
#include<garden/functional.tcc>

namespace garden::lens
{
  struct modify_fn
  : Fn<modify_fn>
  {
    template<class Lens, class F, class A>
    requires std::is_same_v<
      std::invoke_result_t<Lens, A>,
      std::invoke_result_t<F, 
        std::invoke_result_t<Lens, A>
      >
    >
    let assume(Lens, F, A){}
    let assume(auto, auto){}
    let assume(auto){}

    template<class A>
    let eval
    (auto lens, auto f, A a) -> A
    {
      return lens( func( f )( lens( a ) ), a );
    }
  };
  let modify = modify_fn{};

  #define LENS(FIELD)	\
  struct FIELD##_fn	\
  : Fn<FIELD##_fn>	\
  {	\
    let assume(auto){}  \
    let assume(auto, auto){}  \
    \
    template<class A>	\
    requires 	\
    requires(A a)	\
    { { a.FIELD } -> std::any; }	\
    let eval	\
    (A a) -> auto	\
    {	return a.FIELD;	}	\
    \
    template<class X, class A>	\
    requires 	\
    requires(X x, A a)	\
    { { a.FIELD } -> X; }	\
    let eval	\
    (X x, A a) -> auto	\
    {	A b = a; b.FIELD = x; return b;	}	\
    \
    template<class A>	\
    requires 	\
    requires(A a)	\
    { { a.FIELD() } -> std::any; }	\
    let eval	\
    (A a) -> auto	\
    {	return a.FIELD();	}	\
    \
    template<class X, class A>	\
    requires 	\
    requires(X x, A a)	\
    { { a.FIELD( x ) } -> A; }	\
    let eval	\
    (X x, A a) -> A	\
    {	return a.FIELD( x );	}	\
    \
    template<class A>	\
    requires 	\
    requires(A a)	\
    { { a->FIELD } -> std::any; }	\
    let eval	\
    (A a) -> auto	\
    {	return a->FIELD;	}	\
    \
    template<class X, class A>	\
    requires 	\
    requires(X x, A a)	\
    { { a->FIELD } -> X; }	\
    let eval	\
    (X x, A a) -> A	\
    {	a->FIELD = x; return a;	}	\
    \
    template<class A>	\
    requires 	\
    requires(A a)	\
    { { a->FIELD() } -> std::any; }	\
    let eval	\
    (A a) -> auto	\
    {	return a->FIELD();	}	\
    \
    template<class X, class A>	\
    requires 	\
    requires(X x, A a)	\
    { { a->FIELD( x ) } -> A; }	\
    let eval	\
    (X x, A a) -> A	\
    {	return a->FIELD( x );	}	\
    \
  };	\
  let FIELD = FIELD##_fn{};
}
