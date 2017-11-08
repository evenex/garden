#if 0
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreorder" 
#pragma GCC diagnostic ignored "-Wsign-compare" 
// system
#include<pandaFramework.h>
#include<pandaSystem.h>
#include<load_prc_file.h>
// art
#include<geomTriangles.h>
#include<geomLines.h>
#include<geomLinestrips.h>
#include<orthographicLens.h>
#include<pnmImage.h>
#include<dynamicTextFont.h>
#include<colorAttrib.h>
// physics
#include<collisionSphere.h>
#include<collisionNode.h>
#include<collisionHandlerQueue.h>
#include<collisionTraverser.h>
// gui
#include<pgTop.h>
#pragma GCC diagnostic pop
// stdcpp
#include<functional>
// dlib
#include<dlib/unicode/unicode.h>
#include<dlib/sliding_buffer.h>
#include<dlib/matrix.h>
namespace app
{
  void quit(const Event*, void*)
  {
    exit(0);
  }

  std::map<size_t, std::function<void()>> key_callbacks;
  void key_callback(const Event*, void* index)
  {
    key_callbacks.at( size_t( index ) )
    .operator()();
  }

  std::vector<std::function<AsyncTask::DoneStatus()>> task_callbacks;
  auto task_callback(GenericAsyncTask*, void* index) -> AsyncTask::DoneStatus
  {
    return task_callbacks.at( size_t( index ) )
    .operator()();
  }
}

namespace p3d
{
  template<
    class PandaObj,
    class... Xs
  >
  auto make(Xs... xs) 
  -> PointerTo<PandaObj>
  {
    return new PandaObj( xs... );
  }

  struct ArtFont : DynamicTextFont
  {
    auto get_glyph(int code)
    -> const DynamicTextGlyph*
    {
      auto glyph = ConstPointerTo<TextGlyph>{};

      DynamicTextFont::get_glyph(
        code, glyph 
      );

      return dynamic_cast<
        const DynamicTextGlyph*
      >( glyph.p() );
    }

    ArtFont(std::string path, int size = 36)
    : DynamicTextFont( path )
    {
      set_point_size( size );

      set_texture_margin( 0 );
      set_fg( { 1, 1, 1, 1 } );
      set_bg( { 0, 0, 0, 0 } );
      set_outline( { 1, 1, 1, 1 }, 1, 1.5 );
    }
  };
  struct ArtGlyphNode : GeomNode
  {
    ArtGlyphNode(
      std::string name,
      const DynamicTextGlyph* glyph,
      LColor color
    )
    : GeomNode( name )
    {
      auto geom = glyph->get_geom( Geom::UH_static );
      {
        auto v = GeomVertexWriter( geom->modify_vertex_data(), "vertex" );
        v.set_data3f( -0.5, 0, +0.5 );
        v.set_data3f( -0.5, 0, -0.5 );
        v.set_data3f( +0.5, 0, +0.5 );
        v.set_data3f( +0.5, 0, -0.5 );
      }

      add_geom( geom );

      set_attrib(
        TransparencyAttrib::make(
          TransparencyAttrib::M_binary
        ) 
      );

      auto tex_stage = p3d::make<TextureStage>("");
      tex_stage->set_mode(
        TextureStage::M_blend 
      );
      tex_stage->set_color(
        color 
      );

      auto texture = glyph->get_page();

      set_attrib(
        dynamic_cast<const TextureAttrib*>(
          TextureAttrib::make().p()
        )
        ->add_on_stage(
          tex_stage,
          texture,
          0
        )
      );

      set_attrib(
        ColorAttrib::make_flat(
          { 0, 0, 0, 1 } 
        )
      );
    }
  };

  void main(
    int argc, char** argv,
    std::function<void(
      PandaFramework&,
      WindowFramework&
    )> setup
  )
  {
    auto framework = PandaFramework{};

    framework.open_framework(
      argc, argv 
    );

    framework.set_window_title(
      argv[0]
    );

    auto window = framework.open_window();

    window->enable_keyboard();

    setup( framework, *window );

    framework.main_loop();

    framework.close_framework();
  }
}

// garden
#include<garden.tcc>
namespace garden // range
namespace garden // dlib
{
  namespace range
  {
    struct ToMatrix
    {
      template<class R>
      requires Range<R, with< known_size >>
      static constexpr auto
      eval(R r)
      {
        using N =std::decay_t<item_t<R>>; 

        auto m = dlib::matrix<N, 1, 0>(
          r.size()
        );

        for( auto[ i, x ]
          : enumerate( r )
        )
          m( i ) = x;

        return m;
      }
    };
    static constexpr auto
    to_matrix = func<ToMatrix>{};
  }
}
namespace garden // math
{
  struct AbsVal
  {
    static constexpr auto
    eval(auto x) -> auto
    {
      return std::abs( x );
    }
  };
  static constexpr auto
  absval = func<AbsVal>{};

  struct Multiply
  {
    static constexpr auto 
    eval(auto a, auto b) -> auto
    {
      return a*b;
    }
  };
  static constexpr auto
  multiply = func<Multiply>{};

  struct Mean
  {
    template<Range R>
    static constexpr auto
    eval(R r) -> range::item_t<R>
    {
      auto a = range::item_t<R>( 0 );
      auto n = size_t( 0 );

      for( auto x : r )
      {
        a += x;
        ++n;
      }

      return a / n;
    }
  };
  static constexpr auto
  mean = func<Mean>{};
}
namespace garden // print
{
  struct PrintLn
  {
    static void
    eval(auto x)
    {
      std::cout << x << std::endl;
    }
    static void
    eval()
    {
      std::cout << std::endl;
    }
  };
  static constexpr auto
  println = func<PrintLn>{};
}

template<class A, class B>
auto operator<<(std::ostream& out, std::tuple<A,B> xs) -> std::ostream&
{
  return out << "(" 
  << std::get<0>( xs ) << ", " 
  << std::get<1>( xs )
  << ")";
}
template<garden::Range R>
requires not std::is_convertible_v<R, const char*>
auto operator<<(std::ostream& out, R r) -> std::ostream&
{
  out << "[ " << std::boolalpha;

  while( r != nullptr )
  {
    out << *r;
    
    ++r;

    if( r != nullptr )
      out << ", ";
  }

  return out << " ]";
}

#include<complex>
namespace dsp
{
  using namespace garden;

  struct Buffer : dlib::circular_buffer<std::complex<float>>
  {
    Buffer(size_t n)
    {
      resize( n );
    }

    auto begin() -> Range
    {
      return range::transform(
        [this](auto i)
        { return operator[]( i ); },
        range::iota( this->size() )
      );
    }
    auto end() -> nullptr_t
    {
      return {};
    }
  };

  auto hamming_window(size_t n) -> Range
  {
    using namespace garden;

    constexpr auto a = 0.54f;
    constexpr auto b = 1 - a;

    return range::transform(
      [=](auto i)
      {
        return a - b * std::cos( 
          ( 2 * 3.14f * i )
          / ( n - 1 )
        );
      },
      range::iota( n )
    );
  }

  struct FastFourier
  {
    static constexpr auto
    eval(Range r) -> Range
    {
      return range::over( dlib::fft(
        r^range::to_matrix 
      ) );
    }

    struct Inverse
    {
      static constexpr auto
      eval(Range r) -> Range
      {
        return range::over( dlib::ifft(
          r^range::to_matrix 
        ) );
      }
    };
  };
  static constexpr auto
  fft = func<FastFourier>{};
  static constexpr auto
  ifft = func<FastFourier::Inverse>{};
}

namespace audio
{
  #include<portaudio.h>

  PaStream* stream;
  PaError error;

  struct Error : std::runtime_error
  {
    Error(PaError error)
    : std::runtime_error(
      Pa_GetErrorText( error ) 
    ){}
  };

  std::function<void(const float* in, float* out, size_t n)> callback;

  int sample_callback(
    const void* input,
    void* output,
    size_t frame_count,
    const PaStreamCallbackTimeInfo* time_info,
    PaStreamCallbackFlags status_flags,
    void* user_ptr
  )
  {
    callback(
      reinterpret_cast<const float*>( input ),
      reinterpret_cast<float*>( output ),
      frame_count
    );

    return 0;
  }

  void initialize()
  {
    error = Pa_Initialize();

    if( error != paNoError )
      throw Error( error );
  }
  void terminate()
  {
    error = Pa_Terminate();

    if( error != paNoError )
      throw Error( error );
  }

  void open_stream()
  {
    error = Pa_OpenDefaultStream(
      &stream,
      1, // input
      2, // output
      paFloat32,
      192000,
      paFramesPerBufferUnspecified,
      sample_callback,
      nullptr
    );

    if( error != paNoError )
      throw Error( error );
  }
  void start_stream()
  {
    error = Pa_StartStream( stream );

    if( error != paNoError )
      throw Error( error );
  }

  struct PortAudio
  {
    PortAudio()
    {
      audio::initialize();
    }
    ~PortAudio()
    {
      audio::terminate();
    }
  }
  service;
}

namespace gfx
{
  #define PROPERTY( NAME, TYPE ) \
    private:  \
    TYPE NAME ## _value;  \
    public: \
    auto NAME() const -> TYPE\
    {\
      return NAME ## _value;\
    }\
    auto NAME(const auto&& value) const -> auto\
    {\
      auto x = *this;\
      x.NAME ## _value = value;\
      return x; \
    }

  template<class> struct Build;

  template<class Primitive>
  struct BasicGeomNode : GeomNode
  {
    BasicGeomNode(string name)
    : GeomNode( name )
    {}

    PT(GeomVertexData) vertex_data;
    PT(Primitive) primitive_data;
    PT(Geom) geometry;
  };

  template<class Primitive>
  struct Build<BasicGeomNode<Primitive>>
  {
    PROPERTY( name, string )
    PROPERTY( format, CPT(GeomVertexFormat) )
    PROPERTY( vertex_usage, Geom::UsageHint )
    PROPERTY( primitive_usage, Geom::UsageHint )

    auto operator()() const -> PT(BasicGeomNode<Primitive>)
    {
      auto node = p3d::make<BasicGeomNode<Primitive>>(
        name() 
      );

      node->vertex_data = p3d::make<GeomVertexData>(
        "", format(), vertex_usage()
      );
      
      node->primitive_data = p3d::make<Primitive>( 
        primitive_usage()
      );

      node->geometry = p3d::make<Geom>(
        node->vertex_data
      );
      node->geometry->add_primitive(
        node->primitive_data
      );

      node->add_geom(
        node->geometry 
      );

      return node;
    }
  };

  const auto red = LColor{ 1, 0, 0, 1 };
  const auto green = LColor{ 0, 1, 0, 1 };
  const auto blue = LColor{ 0, 0, 1, 1 };
  const auto yellow = LColor{ 1, 1, 0, 1 };
  const auto cyan = LColor{ 0, 1, 1, 1 };
  const auto magenta = LColor{ 1, 0, 1, 1 };
  const auto white = LColor{ 1, 1, 1, 1 };
  const auto black = LColor{ 0, 0, 0, 1 };
}

#include<numeric>
namespace dspdev
{
  using namespace gfx;
  using namespace garden;

  struct PlotNode : PandaNode
  {
    PlotNode(std::string name, LColor color = white)
    : PandaNode( name )
    , title( name )
    {
      geom = Build<BasicGeomNode<GeomLinestrips>>{}
      .name( name + "_geom" )
      .format( GeomVertexFormat::get_v3() )
      .vertex_usage( Geom::UH_dynamic )
      .primitive_usage( Geom::UH_static )
      ();

      geom->set_attrib( RenderModeAttrib::make(
        RenderModeAttrib::M_wireframe, 2.5
      ) );

      add_child( geom );

      set_attrib( ColorAttrib::make_flat(
        color 
      ) );
    }

    void set_title(std::string title)
    {
      this->title = title;
    }
    void set_x_range(float min, float max)
    {
      x_range = { min, max };
      rescale();
    }
    void set_y_range(float min, float max)
    {
      y_range = { min, max };
      rescale();
    }

    void set_data(Range xs)
    {
      auto n = range::size( xs );

      auto v = GeomVertexWriter(
        geom->vertex_data, "vertex" 
      );
      {
        for( auto[ i, x ] : range::enumerate( xs ) )
          v.add_data3f(
            float( i ) / n, 0, x
          );
      }

      auto p = geom->primitive_data;
      {
        p->clear_vertices();
        p->add_consecutive_vertices( 0, n );
        p->close_primitive();
      }
    }

    void rescale()
    {
      auto sx = x_range[1] - x_range[0];
      auto sy = y_range[1] - y_range[0];

      NodePath( geom ).set_scale(
        { 1/sx, 1, 1/sy }
      );
    }

    std::string title;
    std::array<float, 2>
      x_range = { 0, 1 },
      y_range = { 0, 1 };
    PT(BasicGeomNode<GeomLinestrips>) geom;
  };

  struct Session
  {
    dsp::Buffer input;
    std::vector<float> output;
    std::vector<std::complex<float>> spectrum;
    float volume;
    std::map<std::string, PT(PlotNode)> plots;

    void analyze_audio()
    {
      using namespace range;
      using namespace dsp;
      using range::transform; // TODO
      using range::transpose; // TODO
      using range::iota; // TODO

      auto m = 10;
      auto n = input.size() / m;

      spectrum = save(
        transform( mean )
        << transpose
        << transform(
          fft << zip_with( multiply,
            hamming_window( n )
          )
        )
        << stride( m )
        << sliding_window( n )
        << over
      )( input );

      output = transform( absval,
        ifft( over( spectrum ) )
      )^save;
    }

    void render_audio(const float* in, float* out, size_t n)
    {
      analyze_audio();

      while( n --> 0 )
      {
        input.push_back( *in++ );

        auto x = output[ output.size()/2 - n ];

        *out++ = x * volume;
        *out++ = x * volume;
      }
    };
    auto render_video() -> AsyncTask::DoneStatus
    {
      auto render = [&](string name, Range buffer)
      {
        plots.at( name )->set_data( buffer );
      };

      using namespace range;
      using range::transform;

      render( "raw",
        over( output )
      );

      if(1)
      render( "fft", 
        transform( absval, over( spectrum ) ) 
      );

      return AsyncTask::DS_cont;
    };

    Session(int argc, char** argv)
    : input( 8192 )
    , output( input.size() )
    , volume( 21 )
    {
      using namespace garden;

      auto setup = [&](auto& framework, auto& window)
      {
        framework.define_key(
          "escape", "", &app::quit, nullptr 
        );

        window.get_display_region_3d()
        ->set_clear_color( black );

        { // make plot
          auto p = window.get_render_2d()
          .attach_new_node(
            plots["raw"] = (
              p3d::make<PlotNode>( "", green )
            )
          );
          p.set_pos( { -1, 0, +0.5 } );
          p.set_scale( { 2, 1, 1 } );
        }
        { // make plot
          auto p = window.get_render_2d()
          .attach_new_node(
            plots["fft"] = (
              p3d::make<PlotNode>( "", cyan )
            )
          );
          p.set_pos( { -1, 0, -0.5 } );
          p.set_scale( { 2, 1, 1e-2 } );
        }

        { // set per-frame callback
          app::task_callbacks
          .push_back(
            [this](){ return render_video(); } 
          );

          for( auto task : app::task_callbacks )
          {
            AsyncTaskManager::get_global_ptr()
            ->add( 
              p3d::make<GenericAsyncTask>( 
                "tasks", &app::task_callback,
                nullptr
              )
            );
          }
        }

        audio::start_stream();
      };

      audio::callback = [this](auto... xs)
      { render_audio( xs... ); };

      audio::open_stream();

      p3d::main( argc, argv, setup );
    }
  };
}

namespace game
{
  struct Physics
  {
    PT(CollisionHandlerQueue) queue;
    CollisionTraverser traverser;

    Physics()
    {
      queue = new CollisionHandlerQueue();
    }

    void add(auto collider)
    {
      traverser.add_collider(
        NodePath( collider ), queue 
      );
    }

    void update(auto scene, auto handler)
    {
      traverser.traverse( scene );

      for( auto i = queue->get_num_entries(); i --> 0; )
      {
        auto& e = *queue->get_entry( i );

        handler( e.get_from(), e.get_into() );
      }

      queue->clear_entries();
    }
  };
  struct Entity
  {
    struct Node : PandaNode
    {
      Node(string name, PT(p3d::ArtFont) font, Physics& phy, int symbol, LColor color)
      : PandaNode( name )
      {
        auto visual = p3d::make<p3d::ArtGlyphNode>(
          "", font->get_glyph( symbol ),
          color
        );

        add_child( visual );

        auto physical = p3d::make<CollisionNode>( "" );

        physical->add_solid(
          p3d::make<CollisionSphere>(
            LPoint3{ 0,0,0 }, 0.49
          )
        );

        add_child( physical );

        phy.add( physical );
      }
    };

    void position(int i, int j)
    {
      NodePath( root ).set_pos(
        { float( i ), 0, float( j ) } 
      );
    }

    PT(Node) root;
  };
}

namespace gamedev
{
  struct Player
  {
    int x = 0, y = 0;
  };

  struct Session
  {
    Session(int argc, char** argv)
    {
      p3d::main(
        argc, argv,
        [&](PandaFramework& framework, WindowFramework& window)
        {
          {
            window.get_display_region_3d()
            ->set_clear_color( { 0, 0, 0, 1 } );
          }

          // grid
          {
            auto vdata = p3d::make<GeomVertexData>(
              "", GeomVertexFormat::get_v3(),
              Geom::UH_static 
            );
            auto prim = p3d::make<GeomLines>( Geom::UH_static );
            {
              constexpr auto n_cells = 20;

              vdata->set_num_rows( n_cells * 2 * 2 );

              auto v = GeomVertexWriter( vdata, "vertex" );

              for( int i = 0; i < n_cells + 1; ++i )
              {
                auto x = ( 2*i / float( n_cells ) ) - 1;

                v.add_data3f( { x, 0, -1 } );
                v.add_data3f( { x, 0, +1 } );
                prim->add_vertices( 4*i+0, 4*i+1 );

                v.add_data3f( { -1, 0, x } );
                v.add_data3f( { +1, 0, x } );
                prim->add_vertices( 4*i+2, 4*i+3 );
              }
            }
            auto geom = p3d::make<Geom>( vdata );
            {
              geom->add_primitive( prim );
            }
            auto node = p3d::make<GeomNode>( "" );
            {
              node->add_geom( geom );
            }

            auto n = window.get_render()
            .attach_new_node( node );
            n.set_color( { 0.2, 0.2, 0.2, 1 } );
            n.set_pos( { -0.5, 0, 0.5 } );
            n.set_scale( 10 );
          }

          font = p3d::make<p3d::ArtFont>(
            "./font/DejaVuSansMono.ttf", 10
          );

          auto add_entity = [&](string name, int symbol, LColor color)
          -> game::Entity&
          {
            auto node = p3d::make<game::Entity::Node>(
              name, font, physics, symbol, color
            );

            window.get_render()
            .attach_new_node(
              node
            );

            entities[ name ] = game::Entity{ node };

            return entities[ name ];
          };


          auto& devil = add_entity(
            "devil", 0x263F, { 1.0, 0, 0, 1 }
          );
          {
            app::task_callbacks.push_back(
              [&]()
              {
                entities[ "devil" ].position(
                  player.x, player.y
                );

                return AsyncTask::DS_cont;
              }
            );
          }

          auto level = std::string{
            "                    "
            "         ▩          "
            "   ┃     ▥          "
            "   ┃     ▤▤▤▤       "
            "   ┃  ⌨     ▥       "
            "   ┃        ▥       "
            "   ┃        ▥       "
            "   ┃        ▥       "
            "   ┃                "
            "                    "
            "                    "
            "                    "
            "                    "
            "                    "
            "       ▦            "
            "       ▧▧▧          "
            "                    "
            "                    "
            "                    "
            "                    "
          };
          {
            auto lvl = dlib::convert_utf8_to_utf32(
              level
            );

            //std::map<dlib::unichar, LColor> colors;
            auto gray = LColor{ 0.7, 0.7, 0.7, 1 };
            
            int i = -10; int j = 10;
            for( auto block : lvl )
            {
              if( i >= 10 )
              {
                i = -10;
                --j;
              }

              if( block != ' ' )
                add_entity(
                  "block", block, gray 
                )
                .position( i, j );

              ++i;
            }
          }

          // keyboard
          {
            framework.define_key(
              "escape", "", &app::quit, nullptr 
            );

            app::key_callbacks['w'] = [&]()
            {
              move( 0, +1 );
            };
            app::key_callbacks['s'] = [&]()
            {
              move( 0, -1 );
            };
            app::key_callbacks['a'] = [&]()
            {
              move( -1, 0 );
            };
            app::key_callbacks['d'] = [&]()
            {
              move( +1, 0 );
            };
          }

          // register callbacks
          {
            for( auto[ key, action ] : app::key_callbacks )
              framework.define_key(
                std::string( 1, key ),
                "", &app::key_callback,
                reinterpret_cast<void*>( key ) 
              );

            auto i = 0;
            for( auto task : app::task_callbacks )
              AsyncTaskManager::get_global_ptr()
              ->add( p3d::make<GenericAsyncTask>( 
                "tasks", &app::task_callback,
                reinterpret_cast<void*>( i++ ) 
              ) );
          }

          window.get_camera_group()
          .set_pos( 0, -50, 0 );
        }
      );
    }

    PT(p3d::ArtFont) font;
    game::Physics physics;
    std::map<std::string, game::Entity> entities;
    Player player;

    void move(int dx, int dy)
    {
      player.x += dx;
      player.y += dy;
    }
  };
}

#include<dlib/directed_graph.h>
#include<queue>
#include<set>
namespace tests
{
  void functor_basics()
  {
    using namespace garden;

    auto x = new int( 5 );

    auto f = [](int a) -> int
    { return a + 1; };

    auto y = functor::transform( f, x );

    assert( *y == 6 );
  }

  void range_basics()
  {
    using namespace garden;

    int data[6] = { 1, 2, 3, 4, 5, 6 };

    auto xs = range::slice( data, 6 );

    std::cout << xs << std::endl;

    for( auto chunk : range::chunks( 2, xs ) )
      std::cout << chunk << std::endl;

    std::cout << range::zip( xs, xs ) << std::endl;
  }

  void bfs()
  {
    using namespace garden;
    using namespace garden::range;

    using G = dlib::directed_graph<int>
    ::kernel_1a;
    using V = G::node_type;

    auto breadth_first = [](auto f, G& graph)
    {
      if( graph.number_of_nodes() == 0 )
        return;

      auto visited = std::set<size_t>{};
      auto frontier = std::queue<size_t>{};

      auto was_visited = [&](auto i) -> bool
      {
        return visited.count( i ) > 0;
      };
      auto visit = [&](auto i) -> void
      {
        assert( not was_visited( i ) );

        f( graph.node( i ).data );
        visited.insert( i );

        assert( was_visited( i ) );
      };
      auto grow_frontier = [&](auto i) -> void
      {
        assert( not was_visited( i ) );

        frontier.push( i );
      };
      auto traverse_frontier = [&]() -> size_t
      {
        auto i = frontier.front();
        frontier.pop();
        return i;
      };
      auto children = [&](auto i) -> Range
      {
        struct range_t
        {
          auto operator*() -> size_t
          {
            return node.child( j ).index();
          }
          auto operator++() -> auto&
          {
            ++j;

            return *this;
          }
          auto operator!=(nullptr_t) -> bool
          {
            return j < node.number_of_children();
          }

          auto begin() -> auto&
          {
            return *this;
          }
          auto end() -> nullptr_t
          {
            return {};
          }

          V& node;
          size_t j;
        };

        return range_t{ graph.node( i ), 0 };
      };
      
      grow_frontier( 0 );

      while( not frontier.empty() )
      {
        auto i = traverse_frontier();

        visit( i );

        for( auto c : children( i ) )
          if( not was_visited( c ) )
            grow_frontier( c );
      }
    };

    G g;

    auto add_node = [&](auto d)
    {
      g.node( g.add_node() )
      .data = d;
    };

    for( auto i : range::iota( 10 ) )
      add_node( i );

    g.add_edge( 0, 1 );
      g.add_edge( 1, 4 );
      g.add_edge( 1, 5 );
    g.add_edge( 0, 2 );
      g.add_edge( 2, 6 );
      g.add_edge( 2, 7 );
        g.add_edge( 7, 8 );
        g.add_edge( 7, 9 );
    g.add_edge( 0, 3 );

    breadth_first( println, g );
  }
}
#endif
#define CATCH_CONFIG_MAIN
#include<garden/testing.tcc>
