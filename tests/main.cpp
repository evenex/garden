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


#include<any>
namespace garden
{
  #define CHECK_TYPE(X) using TERMINATE_COMPILATION = decltype(X)::TYPE_CHECK;

  using std::declval;

  template<class X>
  auto fwd(X&& x) -> decltype(auto)
  {
    return std::forward<X>( x );
  }

  template<class...>
  struct Arg;

  template<>
  struct Arg<>{};

  constexpr auto _ = Arg<>{};

  template<class F>
  struct function
  {
    auto operator()(auto... xs) const-> decltype(auto)
    {
      return fn.eval( fwd( xs )... );
    }

    F fn;
  };

  struct Identity
  {
    constexpr static 
    auto eval(auto&& x) -> decltype(auto)
    {
      return fwd( x );
    }
  };
  constexpr static auto identity = function<Identity>{};

  /////////////

  class Typeclass{};

  template<class Typeclass, class Type>
  struct instance;

  template<class Type, class Typeclass>
  using proof = typename Typeclass::template proof<Type>;

  template<class Type, class Typeclass>
  concept bool models = requires
  { typename proof<std::decay_t<Type>, Typeclass>; };

  ///////

  struct Functor : Typeclass
  {
    template<class F>
    requires requires
    (F f)
    {
      { instance<Functor, F>
        ::transform( identity, f )
        } -> F;
    }
    struct proof {};

    struct transform
    {
      static constexpr auto
      eval(auto&& f, models<Functor>&& fx) -> decltype(auto)
      {
        return instance<Functor,
          std::decay_t<decltype(fx)>
        >::transform( fwd( f ), fwd( fx ) );
      }
    };
  };
  namespace functor
  {
    constexpr auto transform = function<Functor::transform>{};
  }

  /// 

  template<class X>
  struct instance<Functor, X*>
  {
    template<class F>
    constexpr static auto
    transform(F f, X* fx) -> auto*
    {
      return new std::decay_t<
        std::result_of_t<
          F(X)
        >
      >{ f( *fx ) };
    };
  };
}

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

template<class R>
concept bool Range = requires(R r)
{
  { ++r } -> R;
  { *r } -> std::any;
  { r != nullptr } -> bool;
};
auto begin(Range& x) -> auto
{
  return x;
}
auto end(Range&) -> nullptr_t
{
  return {};
}

template<class X>
concept bool Contiguous = requires(X x)
{
  { &*++x.ptr };
  { x.size } -> size_t;
};
template<class X>
auto slice(X* ptr, size_t size) -> Contiguous
{
  struct
  {
    auto operator*() -> X&
    {
      return *ptr;
    }
    auto operator++() -> auto&
    {
      ++ptr;
      --size;

      return *this;
    }
    auto operator!=(nullptr_t) const -> bool
    {
      return size > 0;
    }

    X* ptr; size_t size; 
  }
  ans;

  ans.ptr = ptr;
  ans.size = size;

  return ans;
}
auto chunks(size_t n, Contiguous range) -> Range
{
  struct
  {
    auto operator*() -> auto
    {
      return slice( r.ptr, n );
    }
    auto operator++() -> auto&
    {
      auto dx = min( r.size, n );

      while( dx --> 0 )
        ++r;

      return *this;
    }
    auto operator!=(nullptr_t) const -> bool
    {
      return r != nullptr;
    }

    decltype(range) r;
    size_t n;
  }
  ans;

  ans.r = range;
  ans.n = n;

  return ans;
}
auto zip(Range xs, Range ys) -> auto
{
  struct
  {
    auto operator*() -> auto
    {
      return std::tuple( *x, *y );
    }
    auto operator++() -> auto&
    {
      ++x;
      ++y;

      return *this;
    }
    auto operator!=(nullptr_t) -> bool
    {
      return x != nullptr and y != nullptr;
    }

    decltype(xs) x;
    decltype(ys) y;
  }
  ans;

  ans.x = xs;
  ans.y = ys;

  return ans;
}
auto save(Range xs) -> std::vector<auto>
{
  auto ys = std::vector<std::decay_t<decltype(*xs)>>();

  for( auto x : xs )
    ys.push_back( x );

  return ys;
}

template<class A, class B>
auto operator<<(std::ostream& out, std::tuple<A,B> xs) -> std::ostream&
{
  return out << "(" 
  << std::get<0>( xs ) << ", " 
  << std::get<1>( xs )
  << ")";
}
template<Range R>
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
  #include<liquid.h>

  struct Buffer
  {
    void print()
    {
      windowf_print( ptr.get() );
    }
    void debug_print()
    {
      windowf_debug_print( ptr.get() );
    }
    void reset()
    {
      windowf_reset( ptr.get() );
    }

    auto size() -> uint32_t
    {
      return n;
    }
    void resize(uint32_t n)
    {
      windowf_recreate( ptr.get(), n );
    }
    void clear()
    {
      windowf_reset( ptr.get() );
    }

    auto operator[](uint32_t i) -> float
    {
      float x;

      windowf_index( ptr.get(), i, &x );

      return x;
    }
    void push(float x)
    {
      windowf_push( ptr.get(), x );
    }

    auto read() -> float*
    {
      float* xs;

      windowf_read( ptr.get(), &xs );

      return xs;
    }
    void write(const float* xs, uint32_t n)
    {
      windowf_write(
        ptr.get(), const_cast<float*>( xs ), n 
      );
    }

    Buffer(uint32_t n)
    : n( n )
    {
      ptr = decltype(ptr)(
        windowf_create( n ),
        windowf_destroy
      );
    }

  private:

    std::shared_ptr<windowf_s> ptr;
    const uint32_t n;
  };

  auto fft(Contiguous signal) -> std::vector<float>
  {
    auto spectrum = std::vector<float>( signal.size );

    auto plan = fft_create_plan_r2r_1d(
      signal.size, signal.ptr,
      spectrum.data(),
      LIQUID_FFT_REDFT10,
      0x0
    );

    fft_execute( plan );

    fft_destroy_plan( plan );

    return spectrum;
  }

  auto inv_fft(std::vector<float> spectrum) -> std::vector<float>
  {
    auto signal = std::vector<float>( spectrum.size() );

    auto plan = fft_create_plan_r2r_1d(
      spectrum.size(), spectrum.data(),
      signal.data(),
      LIQUID_FFT_REDFT01,
      0x0
    );

    fft_execute( plan );

    fft_destroy_plan( plan );

    return signal;
  }

  auto power_spectrum(Contiguous range) -> Range
  {
    using R = float;
    using C = std::complex<R>;

    auto in_time = std::vector<C>( range.size );
    auto in_freq = std::vector<C>( range.size );

    std::transform(
      range.ptr, range.ptr + range.size,
      in_time.data(),
      [](R re){ return C( re ); }
    );

    // create fft plans
    fftplan forward = fft_create_plan(
      range.size, 
      in_time.data(), in_freq.data(),
      LIQUID_FFT_FORWARD, 0
    );

    // execute fft plans
    fft_execute( forward );

    // destroy fft plans
    fft_destroy_plan( forward );

    struct
    {
      R* ptr; size_t size;
      std::shared_ptr<std::vector<R>> m;

      auto begin() -> auto&
      {
        return *this;
      }
      auto end() -> nullptr_t
      {
        return {};
      }

      auto operator*() -> float&
      {
        return *ptr;
      }
      auto operator++() -> auto&
      {
        ++ptr;
        --size;

        return *this;
      }
      auto operator!=(nullptr_t) const -> bool
      {
        return size > 0;
      }

      operator std::vector<R>() const
      {
        return *m;
      }
    }
    ans;

    ans.m = std::make_shared<std::vector<R>>(
      range.size
    );
    ans.ptr = ans.m->data();
    ans.size = ans.m->size();

    std::transform(
      in_freq.begin(), in_freq.end(),
      ans.ptr, [](C c)
      { return std::abs( c ); }
    );

    return ans;
  }

  auto reconstructed_from_spectrum(Contiguous range) -> Range
  {
    using R = float;
    using C = std::complex<R>;

    auto in_time = std::vector<C>( range.size );
    auto in_freq = std::vector<C>( range.size );

    std::transform(
      range.ptr, range.ptr + range.size,
      in_time.data(),
      [](R re){ return C( re ); }
    );

    // create fft plans
    fftplan forward = fft_create_plan(
      range.size, 
      in_time.data(), in_freq.data(),
      LIQUID_FFT_BACKWARD, 0
    );

    // execute fft plans
    fft_execute( forward );

    // destroy fft plans
    fft_destroy_plan( forward );

    struct
    {
      R* ptr; size_t size;
      std::shared_ptr<std::vector<R>> m;

      auto begin() -> auto&
      {
        return *this;
      }
      auto end() -> nullptr_t
      {
        return {};
      }

      auto operator*() -> float&
      {
        return *ptr;
      }
      auto operator++() -> auto&
      {
        ++ptr;
        --size;

        return *this;
      }
      auto operator!=(nullptr_t) const -> bool
      {
        return size > 0;
      }

      operator std::vector<R>() const
      {
        return *m;
      }
    }
    ans;

    ans.m = std::make_shared<std::vector<R>>(
      range.size
    );
    ans.ptr = ans.m->data();
    ans.size = ans.m->size();

    int i = 0;
    for( auto& x : *ans.m )
      x = in_freq[i++].real() / 1000;

    return ans;
  }

  auto autocorrelation(Contiguous range) -> Contiguous
  {
    struct
    {
      float* ptr; size_t size;

      std::shared_ptr<std::vector<float>> m;
    }
    ans;

    ans.m = std::make_shared<std::vector<float>>( range.size );
    ans.ptr = ans.m->data();
    ans.size = ans.m->size();

    for( size_t i = 0; i < range.size; ++i )
      ans.m->at( i ) = dsp::liquid_filter_autocorr(
        range.ptr, range.size, i 
      );

    return ans;
  }

  auto hamming_window(size_t n) -> std::vector<float>
  {
    auto w = std::vector<float>( n );

    for( size_t i = 0; i < n; ++i )
      w[i] = hamming( i, n );

    return w;
  }
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
    void set_data(float* xs, const size_t n)
    {
      auto v = GeomVertexWriter(
        geom->vertex_data, "vertex" 
      );
      {
        for( auto i = n; i --> 0; )
          v.add_data3f(
            float( i ) / n, 0, xs[i] 
          );
      }

      auto p = geom->primitive_data;
      {
        p->clear_vertices();
        p->add_consecutive_vertices( 0, n );
        p->close_primitive();
      }
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
    dsp::Buffer buffer, output;
    std::vector<float> window;
    float volume;
    std::map<std::string, PT(PlotNode)> plots;

    void audio_frame(const float* in, float* out, size_t n)
    {
      buffer.write( in, n );

      output.write( in, n );

      while( n --> 0 )
      {
        auto x = output[ output.size() - n - 1 ];

        *out++ = x * volume;
        *out++ = x * volume;
      }
    };

    auto video_frame() -> AsyncTask::DoneStatus
    {
      auto raw = slice( buffer.read(), buffer.size() );
      auto post = slice( output.read(), output.size() );

      auto fftraw = dsp::fft( raw );
      //auto ffts = slice( fftraw.data(), fftraw.size() );

      auto plot = [&](string name, Contiguous& buffer)
      {
        plots[ name ]->set_data(
          buffer.ptr, buffer.size
        );
      };

      plot( "raw", raw );
      //plot( "post", ffts );

      return AsyncTask::DS_cont;
    };

    Session(int argc, char** argv)
    : buffer( 8192 )
    , output( 8192 )
    , window( dsp::hamming_window( 8192 ) )
    , volume( 16 )
    {
      using namespace garden;

      audio::open_stream();

      audio::callback = [this](auto... xs)
      { audio_frame( xs... ); };

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
          p.set_pos( { -1, 0, -0.5 } );
          p.set_scale( { 2, 1, 1 } );
        }
        { // make plot
          auto p = window.get_render_2d()
          .attach_new_node(
            plots["post"] = (
              p3d::make<PlotNode>( "", cyan )
            )
          );
          p.set_pos( { -1, 0, 0.5 } );
          p.set_scale( { 2, 1, 1 } );
        }

        { // set per-frame callback
          app::task_callbacks
          .push_back(
            [this](){ return video_frame(); } 
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

namespace tests
{
  void functor_basics()
  {
    using namespace garden;

    auto x = new int( 5 );

    auto f = [](int a) -> int
    { return a + 1; };

    auto y = functor::transform( f, x );

    std::cout << std::boolalpha
    << models<int*, Functor>
    << ", "
    << *y
    << std::endl;
  }

  void range_basics()
  {
    int data[6] = { 1, 2, 3, 4, 5, 6 };

    auto xs = slice( data, 6 );

    std::cout << xs << std::endl;

    for( auto chunk : chunks( 2, xs ) )
      std::cout << chunk << std::endl;

    std::cout << zip( xs, xs ) << std::endl;
  }
}

int main(int argc, char** argv)
{
  dspdev::Session( argc, argv );

  return EXIT_SUCCESS;
}
