#include<garden.tcc>
#include<panda.tcc>

using namespace garden;

let fmap = functor::transform;

int main(int argc, char** argv)
{
  p3d::main( argc, argv,
    [](auto&&...){ return [](){}; }
  );
}
