#include<garden/range.tcc>
#include<garden/testing.tcc>

#define MODULE "range"

#if 0
namespace garden
{
  struct Example
  {
    int i = 0;
  
    auto front() const -> auto
    {
      return i;
    }
    auto empty() const -> auto
    {
      return i == 10;
    }
    void next()
    {
      ++i;
    }
  };

  EXAMPLE( basics )
  {
    int i = 0;
    Range x = Example{};

    for( auto n : x )
      REQUIRE( i++ == n );
  }
}
#endif
