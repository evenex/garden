#define CATCH_CONFIG_MAIN
#include<catch.hpp>
#include<SDL2/SDL.h>
#include<SDL2/SDL_render.h>
#include<SDL2/SDL_ttf.h>
#include<SDL2/SDL_opengl.h>
#include<SDL2/SDL_keyboard.h>
#include<btBulletDynamicsCommon.h>
#include<unicode/unistr.h>
#include<string>
#include<variant>
#include<thread>
#include<chrono>
#include<type_traits>

using std::string;
using Glyph = uint16_t;
using Color = SDL_Color;

namespace std
{
  template<class N>
  concept bool Integral = std::is_integral_v<N>;

  template<class N>
  concept bool Arithmetic = std::is_arithmetic_v<N>;
}

struct nat
{
  constexpr auto operator
  ++() -> decltype(auto)
  {
    ++n;

    return *this;
  }
  constexpr auto operator
  ++(int) -> decltype(auto)
  {
    n++;

    return *this;
  }

  constexpr auto operator
  <(const nat& that) const -> bool
  {
    return n < that.n;
  }

  constexpr nat(std::Integral n)
  : n( n )
  {
    assert( n >= 0 );
  }
  constexpr nat()
  : n( 0 ){}

  template<std::Arithmetic N>
  constexpr operator N() const
  {
    assert( n < std::numeric_limits<N>::max() );

    return N( n );
  }

private:

  unsigned long long int n;
};

struct real
{
  constexpr real(auto x)
  : x( float( x ) ){}

  template<std::Arithmetic R>
  constexpr operator R() const
  {
    assert( x < std::numeric_limits<R>::max() );

    return R( x );
  }

private:

  float x;
};

// video resolution operators
constexpr auto operator ""_p(unsigned long long int n_scanlines) -> std::array<nat, 2>
{
  if( n_scanlines == 480 )
    return { 640, 480 };
  else if( n_scanlines == 720 )
    return { 1280, 720 };
  else if( n_scanlines == 1080 )
    return { 1920, 1080 };
  else
    assert( false );
}
constexpr auto operator ""_K(unsigned long long int kilocolumns) -> std::array<nat, 2>
{
  if( kilocolumns == 4 )
    return { 3840, 2160 };
  else if( kilocolumns == 8 )
    return { 7680, 4320 };
  else
    assert( false );
}

namespace vector
{
  namespace operators
  {
    template<class N, size_t n, class M>
    auto operator-(std::array<N, n> a, std::array<M, n> b) -> std::array<N, n>
    {
      std::array<N, n> c;

      for( nat i = 0; i < n; ++i )
        c[i] = N( a[i] - b[i] );

      return c;
    }
    template<class N, size_t n, class M>
    auto operator+=(std::array<N, n>& a, std::array<M, n> b) -> std::array<N, n>&
    {
      for( nat i = 0; i < n; ++i )
        a[i] += b[i];

      return a;
    }
    template<class N, size_t n, class M> 
    auto operator*=(std::array<N, n>& a, M b) -> std::array<N, n>&
    {
      for( nat i = 0; i < n; ++i )
        a[i] *= b;

      return a;
    }
    template<class N, size_t n, class M> 
    auto operator/=(std::array<N, n>& a, M b) -> std::array<N, n>&
    {
      for( nat i = 0; i < n; ++i )
        a[i] /= b;

      return a;
    }
    template<class N, size_t n, class M> 
    auto operator/(std::array<N, n> a, M b) -> std::array<N, n>
    {
      std::array<N, n> c = a;

      return c /= b;
    }
    template<class N, size_t n, class M>
    auto operator*(std::array<N, n> a, M b) -> std::array<N, n>
    {
      std::array<N, n> c = a;

      return c *= b;
    }
  }

  template<class N, size_t n>
  auto norm(std::array<N,n> v) -> N
  {
    auto x = N(0);

    for( size_t i = 0; i < n; ++i )
      x += v[i]*v[i];

    return N( std::sqrt( x ) );
  }
  template<class N, size_t n>
  auto normalize(std::array<N,n> v) -> std::array<N,n>
  {
    return v / norm( v );
  }
}

using namespace vector::operators;

namespace graphics
{
  namespace sdl
  {
    void check(int status)
    {
      constexpr int sdl_success = 0;

      if( status != sdl_success )
      {
        throw std::runtime_error(
          string( "SDL: " )
          + string( SDL_GetError() )
        );
      }
    };
  }

  struct Context
  {
    Context(std::array<nat, 2> resolution)
    {
      uint32_t window_flags = 0x0;

      auto[ width, height ] = resolution;

      sdl::check(
        SDL_Init( SDL_INIT_VIDEO )
      );

      sdl::check(
        SDL_CreateWindowAndRenderer(
          width, height, window_flags,
          &window, &renderer
        )
      );

      TTF_Init();
    }
    ~Context()
    {
      TTF_Quit();

      SDL_DestroyWindow( window );
      SDL_DestroyRenderer( renderer );
    }
    Context(const Context&) = delete;

    SDL_Window* window;
    SDL_Renderer* renderer;
  };
  void clear(Context& gfx)
  {
    SDL_RenderClear( gfx.renderer );
  };
  void update(Context& gfx)
  {
    SDL_RenderPresent( gfx.renderer );
  }

  struct Font
  {
    Font(string path, nat size = 96)
    {
      resource = TTF_OpenFont(
        path.c_str(), size 
      );
    }
    ~Font()
    {
      if( resource != nullptr )
        TTF_CloseFont( resource );
    }
    Font(const Font&) = delete;

    TTF_Font* resource = nullptr;
  };
  struct Icon
  {
    Icon(Glyph c, Color fg, Font& font, Context& gfx)
    {
      texture = decltype(texture)(
        [&]()
        {
          auto surface = TTF_RenderGlyph_Blended(
            font.resource, c, fg
          );

          auto tex = SDL_CreateTextureFromSurface(
            gfx.renderer, surface
          );

          SDL_FreeSurface( surface );

          return tex;
        }
        (),
        &SDL_DestroyTexture
      );

      SDL_QueryTexture(
        texture.get(), nullptr, nullptr,
        &width, &height 
      );
    }

    std::shared_ptr<SDL_Texture> texture;
    int width, height;
  };

  void draw(Icon& icon, std::array<float, 2> pos, Context& gfx)
  {
    SDL_Rect rect = { int( pos[0] ) , int( pos[1] ), icon.width, icon.height };
    SDL_RenderCopy( gfx.renderer, icon.texture.get(), nullptr, &rect );
  }
}

namespace physics
{
  struct Context
  {
    Context()
    {
      collision_config = new btDefaultCollisionConfiguration();
      collision_dispatch = new btCollisionDispatcher(
        collision_config 
      );
      overlapping_pair_cache = new btDbvtBroadphase();
      constraint_solver = new btSequentialImpulseConstraintSolver();

      dynamics_world = new btDiscreteDynamicsWorld(
        collision_dispatch,
        overlapping_pair_cache,
        constraint_solver,
        collision_config
      );

      dynamics_world->setGravity(
        btVector3( 0, 0, 0 ) 
      );
    }
    ~Context()
    {
      delete collision_config;
      delete collision_dispatch; 
      delete overlapping_pair_cache;
      delete constraint_solver;
      delete dynamics_world;
    }
    Context(const Context&) = delete;

    btDefaultCollisionConfiguration* collision_config;
    btCollisionDispatcher* collision_dispatch; 
    btDbvtBroadphase* overlapping_pair_cache;
    btSequentialImpulseConstraintSolver* constraint_solver;
    btDiscreteDynamicsWorld* dynamics_world;
  };
  void step(real dt, Context& phy)
  {
    constexpr auto max_substeps = 10;

    phy.dynamics_world->stepSimulation( dt, max_substeps );
  }

  struct Box
  {
    Box(std::array<size_t, 2> size, std::array<float, 2> pos, Context& phy)
    : phy( phy )
    {
      shape = new btBoxShape(
        btVector3(
          real( std::get<0>( size ) ),
          real( std::get<1>( size ) ),
          1
        ) 
      );

      motion = new btDefaultMotionState();

      body = new btRigidBody(
        1, motion, 
        dynamic_cast<btCollisionShape*>(
          shape 
        ) 
      );

      auto tf = btTransform::getIdentity();

      tf.setOrigin( btVector3(
        std::get<0>( pos ), 
        std::get<1>( pos ), 
        0
      ) );

      body->setCenterOfMassTransform( tf );

      phy.dynamics_world->addRigidBody( body );
    }
    ~Box()
    {
      phy.dynamics_world->removeRigidBody( body );

      delete body;
      delete shape;
      delete motion;
    }
    Box(const Box&) = delete;

    btRigidBody* body;
    btBoxShape* shape;
    btMotionState* motion;
    Context& phy;
  };
  auto position(Box& box) -> std::array<float, 2> 
  {
    auto p = box.body->getCenterOfMassPosition();

    return { p.getX(), p.getY() };
  }
  auto velocity(Box& box) -> std::array<float, 2> 
  {
    auto p = box.body->getLinearVelocity();

    return { p.getX(), p.getY() };
  }
  auto dimensions(Box& box) -> std::array<size_t, 2> 
  {
    auto dim = box.shape->getHalfExtentsWithoutMargin();

    return { size_t( dim.getX() ), size_t( dim.getY() ) };
  }
  void position(std::array<float, 2> pos, Box& box, Context& phy)
  {
    auto tf = btTransform::getIdentity();

    tf.setOrigin( btVector3(
      std::get<0>( pos ), 
      std::get<1>( pos ), 
      0
    ) );

    box.body->setCenterOfMassTransform( tf );
  }
  void velocity(Box& box, std::array<float, 2> vel)
  {
    box.body->setLinearVelocity(
      btVector3( real( vel[0] ), real( vel[1] ), 0 )
    );
  }
}

namespace game
{
  struct Block
  {
    Block(graphics::Icon icon, std::array<float, 2> pos, physics::Context& phy)
    : icon( icon )
    , box( std::make_shared<physics::Box>(
      std::array<size_t, 2>
      { size_t( icon.width ), size_t( icon.height ) },
      pos, phy
    ) )
    {}

    graphics::Icon icon;
    std::shared_ptr<physics::Box> box;
  };
}

namespace graphics
{
  void draw(game::Block& block, Context& gfx)
  {
    graphics::draw(
      block.icon,
      physics::position( *block.box )
      - ( physics::dimensions( *block.box ) / 2 ),
      gfx 
    );
  }
}

TEST_CASE( "hello" )
{
  graphics::Context gfx{ 720_p };
  physics::Context phy;

  auto font = graphics::Font{
    "./font/DejaVuSansMono.ttf"
  };
  auto blocks = std::vector<game::Block>{
    game::Block{
      graphics::Icon{
        0x2622,
        { 255, 255, 0, 255 },
        font, gfx 
      },
      { 100, 100 },
      phy
    },
    game::Block{
      graphics::Icon{
        0x2624,
        { 255, 0, 255, 255 },
        font, gfx 
      },
      { 100, 200 },
      phy
    }
  };

  auto& pc = blocks[0];

  using namespace std::literals::chrono_literals;

  auto keyboard = std::set<int>{};

  physics::velocity(
    *pc.box, { 10, 10 }
  );

  auto loop = [&]()
  {
    auto start = std::chrono::system_clock::now();

    physics::step( 1.f/30, phy );

    for( SDL_Event event; SDL_PollEvent( &event ); )
    {
      if( event.type == SDL_KEYDOWN )
      {
        if( not event.key.repeat )
          keyboard.insert( event.key.keysym.scancode );
      }
      else if( event.type == SDL_KEYUP )
      {
        keyboard.erase( event.key.keysym.scancode );
      }
    }

    {
      constexpr auto speed = 250;

      auto wasd = std::array<int, 4>
      { SDL_SCANCODE_W, SDL_SCANCODE_A, SDL_SCANCODE_S, SDL_SCANCODE_D };

      using V = std::array<float, 2>;
      V vel = { 0,0 };

      auto move = std::array<V, 4>
      { V{ 0, -1 }, V{ -1, 0 }, V{ 0, +1 }, V{ +1, 0 } };

      for( nat i = 0; i < wasd.size(); ++i )
        if( keyboard.count( wasd[i] ) )
          vel += move[i];

      if( auto mag = vector::norm( vel );
        mag != 0
      )
        vel /= sqrt( mag );

      vel *= speed;

      physics::velocity( *pc.box, vel );
    }

    graphics::clear( gfx );

    for( auto& block : blocks )
      graphics::draw( block, gfx );

    graphics::update( gfx );

    std::this_thread::sleep_until( start + 30ms );
  };

  while( not keyboard.count( SDL_SCANCODE_ESCAPE ) )
    loop();
}
