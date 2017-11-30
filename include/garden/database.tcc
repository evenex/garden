#pragma once
#include<boost/multi_index_container.hpp>
#include<boost/multi_index/ordered_index.hpp>
#include<boost/multi_index/global_fun.hpp>
#include<garden/functional.tcc>
#include<garden/range.tcc>

namespace garden::database
{
  namespace detail
  {
    template<class Key, class Entry>
    struct Extraction
    {
      using field_t = std::invoke_result_t<Key, Entry>;

      let extract
      (const Entry& x) -> field_t
      {
        return Key{}( x );
      }

      using extraction_fn = boost::multi_index::global_fun<
        const Entry&, field_t, extract
      >;
    };

    template<class Key, class Entry>
    requires requires
    {
      typename Extraction<Key, Entry>::field_t;
    }
    using field_t = typename Extraction<Key, Entry>::field_t;

    template<class Key, class Entry>
    requires requires
    {
      typename Extraction<Key, Entry>::extraction_t;
    }
    using extraction_fn = typename Extraction<Key, Entry>::extraction_fn;

    template<class Entry, class Key, class... Attributes>
    using container_t = boost::multi_index_container<
      Entry, 
      boost::multi_index::indexed_by<
        boost::multi_index::ordered_unique<
          boost::multi_index::tag<Key>,
          extraction_fn<Key, Entry>
        >,
        boost::multi_index::ordered_non_unique<
          boost::multi_index::tag<Attributes>,
          extraction_fn<Attributes, Entry>
        >...
      >
    >;
  }

  template<class Entry, class Key, class... Attributes>
  requires (
    std::is_invocable_v<Key, Entry>
    and ... and
    std::is_invocable_v<Attributes, Entry>
  )
  struct Table
  {
    using Container = detail::container_t<Entry, Key, Attributes...>;

    std::shared_ptr<Container> ptr;

  public:

    template<class K>
    using field_t = std::invoke_result_t<K, Entry>;

    let entry_is_key = std::is_same_v<
      field_t<Key>, Entry
    >;

    Table(): ptr( new Container ){}

    // operations

    constexpr auto insert
    (Entry x) -> std::optional<Entry>
    {
      auto[ at, succeeded ] = ptr->insert( x );

      if( succeeded )
        return *at;
      else
        return {};
    }
    constexpr auto update
    (Entry x) -> std::optional<Entry>
    {
      return modify( x, x^constant );
    }

    constexpr auto modify
    (field_t<Key> i, Operator<Entry> f) -> std::optional<Entry>
    {
      if( auto it = ptr->find( i );
        it != ptr->end()
      )
      {
        ptr->replace( it, f( *it ) );

        return *it;
      }
      else
      {
        return {};
      }
    }
    template<class=void>
    requires not entry_is_key
    constexpr auto modify
    (Entry x, Operator<Entry> f) -> std::optional<Entry>
    {
      return modify( Key{}( x ), f );
    }

    constexpr void erase
    (field_t<Key> i)
    {
      ptr->erase( i );
    }
    template<class=void>
    requires not entry_is_key
    constexpr void erase
    (Entry x)
    {
      erase( Key{}( x ) );
    }

    // lookup

    constexpr auto operator()
    (field_t<Key> i) const 
    -> std::optional<Entry>
    {
      return operator()( Key{}, i );
    }
    template<class=void>
    requires not entry_is_key
    constexpr auto operator()
    (Entry x) const 
    -> std::optional<Entry>
    {
      return operator()( Key{}( x ) );
    }

    constexpr auto operator()
    (Key, field_t<Key> i) const 
    -> std::optional<Entry>
    {
      auto& entries = ptr->template get<Key>();

      if( auto it = entries.find( i );
        it != entries.end()
      )
        return *it;
      else
        return {};
    }

    template<class A>
    requires ( ... or std::is_same_v<A, Attributes> )
    constexpr auto operator()
    (A, field_t<A> i) const -> Range<range::of<Entry>>
    {
      return ptr->template get<A>()
      .equal_range( i );
    }

    // traversal

    constexpr auto begin
    () const -> auto
    {
      return ptr->begin();
    }
    constexpr auto end
    () const -> auto
    {
      return ptr->end();
    }

    constexpr auto by
    (Key) const -> Range<range::of<Entry>>
    {
      return std::pair
      { begin(), end() };
    }
    template<class A>
    requires ( ... or std::is_same_v<A, Attributes> )
    constexpr auto by
    (A) const -> Range<range::of<Entry>>
    {
      auto& cursor = ptr->template get<A>();

      return std::pair
      { cursor.begin(), cursor.end() };
    }
  };

  template<class Entry>
  struct table_fn
  : Fn<table_fn<Entry>>
  {
    template<class Key, class... Attributes>
    let eval
    (Key, Attributes...) -> Table<Entry, Key, Attributes...>
    { return {}; }
  };
  template<class X>
  let table = table_fn<X>{};
}
