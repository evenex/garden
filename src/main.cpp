#include<garden.tcc>
#include<panda.tcc>
#include<garden/testing.tcc>

using namespace garden;

let pi = std::atan( 1.f ) * 4;

let iota = range::recur( [](auto i){ return ++i; }, 0 );

#include<iostream>

void show()
{
  std::cout << std::endl; 
}
void show(auto x)
{
  std::cout << x;
}
template<class X, auto i = 0>
requires requires
{ std::tuple_size<X>::value; }
void show(X x)
{
  if constexpr( i == 0 )
  {
    show( '(' );
  }

  if constexpr( i == std::tuple_size_v<X> )
  {
    show( ')' );
  }
  else
  {
    show( std::get<i>( x ) );

    if constexpr( i+1 < std::tuple_size_v<X> )
      show( ", " );

    return show<X, i+1>( x );
  }
}
void show(auto x, auto... xs)
{
  show( x ); show( xs... );
}

namespace
{ // get/set property macro
#define PROPERTY( NAME, TYPE, DEFAULT ) \
  TYPE _property_ ## NAME ## _value = DEFAULT;  \
  auto NAME() const -> TYPE\
  {\
    return _property_ ## NAME ## _value;\
  }\
  auto NAME(auto value) const -> auto\
  {\
    auto x = *this;\
    x._property_ ## NAME ## _value = value;\
    return x; \
  }
}
namespace game
{ // geometry
  template<class Primitive>
  struct geometry_fn
  : fn<geometry_fn<Primitive>>
  {
    PROPERTY( format, const GeomVertexFormat*, {} )
    PROPERTY( vertex_usage, Geom::UsageHint, {} )
    PROPERTY( primitive_usage, Geom::UsageHint, {} )

    auto eval
    () const -> PT(Geom)
    {
      if( format() == nullptr )
        return format(
          GeomVertexFormat::get_v3() 
        ).eval();
      if( vertex_usage() == Geom::UsageHint{} )
        return vertex_usage(
          Geom::UH_static 
        )
        .eval();
      if( primitive_usage() == Geom::UsageHint{} )
        return primitive_usage(
          Geom::UH_static
        ).eval();

      auto prim = p3d::make<Primitive>(
        primitive_usage()
      );
      auto vert = p3d::make<GeomVertexData>(
        "", format(), vertex_usage()
      );
      auto geom = p3d::make<Geom>(
        vert
      );

      geom->add_primitive( prim );

      return geom;
    }

    template<Range V, Range I>
    auto eval
    (PT(Geom) geom, V vertices, I indices) const -> PT(Geom)
    {
      auto v = GeomVertexWriter( 
        geom->modify_vertex_data(),
        "vertex" 
      );
      for( auto[ x,y ] : vertices )
        v.add_data3f( x, 0, y );

      auto p = geom->modify_primitive( 0 );
      p->clear_vertices();
      for( auto i : indices )
        p->add_vertex( i );
      p->close_primitive();

      return geom;
    }

    template<Range V, Range I>
    auto eval
    (V vertices, I indices) const -> PT(Geom)
    {
      return eval( eval(), vertices, indices );
    }
  };

  struct geom_node_fn
  : fn<geom_node_fn>
  {
    static auto eval
    (PT(Geom) geom, std::string name = {}) -> PT(GeomNode)
    {
      auto node = p3d::make<GeomNode>( name );

      node->add_geom( geom );

      return node;
    }
    static auto eval
    (CPT(DynamicTextGlyph) glyph, std::string name = {}) -> PT(GeomNode)
    {
      return eval( 
        glyph->get_geom( Geom::UH_static )
      );
    }
  };

  template<class P>
  let geometry = geometry_fn<P>{};
  let geom_node = geom_node_fn{};

  template<size_t n>
  struct regular_ngon_fn
  : memoized_fn<PT(Geom)(),
    regular_ngon_fn<n>
  >
  {
    using Point = std::array<float, 2>;

    static auto eval
    () -> PT(Geom)
    {
      return geometry<GeomTriangles>(
        range::chain(
          range::only(
            Point{ 0, 0 }
          ),
          range::take( n, iota )
          ^range::transform(
            [](auto i) -> float
            { return 2*i*pi/n; }
            >> [](auto t) -> Point
            { return { std::cos(t), std::sin(t) }; }
          )
        )
        ,
        range::join(
          range::take( n, iota )
          ^range::transform(
            [](auto i){ return i+1; }
            >> [](auto i)
            { return range::only( 0, i, (i%n)+1 ); }
          )
        )
      );
    }
  };
  struct unit_grid_fn
  : fn<unit_grid_fn>
  {
    using Point = std::array<float, 2>;

    static auto eval
    (size_t width) -> PT(Geom)
    {
      return geometry<GeomLines>(
        range::take( width + 1, iota )
        ^range::transform(
          [&](float i)
          { return 2*i/width; }
          >> (
            [](float t)
            {
              return range::only(
                Point{ -1 + t, -1 },
                Point{ -1 + t, +1 }
              );
            }
            & [](float t)
            {
              return range::only(
                Point{ -1, -1 + t },
                Point{ +1, -1 + t }
              );
            }
          )
          >> range::chain
        )
        ^range::join
        ,
        range::take( 4*(width+1), iota )
      );
    }
  };
  struct normalized_quad_fn
  : memoized_fn<PT(Geom)(),
    normalized_quad_fn
  >
  {
    using Point = std::array<float, 2>;

    static auto eval
    () -> PT(Geom)
    {
      using P = Point;

      return geometry<GeomTriangles>(
        range::only( 
          P{ 0,0 }, P{ 1,0 }, P{ 1,1 }, P{ 0,1 }
        ),
        range::only( 0, 1, 2, 2, 3, 0 )
      );
    }
  };
  struct normalized_box_fn
  : memoized_fn<PT(Geom)(),
    normalized_box_fn
  >
  {
    using Point = std::array<float, 2>;

    static auto eval
    () -> PT(Geom)
    {
      using P = Point;

      return geometry<GeomLines>(
        range::only( 
          P{ 0,0 }, P{ 1,0 }, P{ 1,1 }, P{ 0,1 }
        ),
        range::only( 0, 1, 1, 2, 2, 3, 3, 0 )
      );
    }
  };

  template<size_t n>
  let regular_ngon = regular_ngon_fn<n>{};
  let unit_grid = unit_grid_fn{};
  let normalized_quad = normalized_quad_fn{};
  let normalized_box = normalized_box_fn{};
}
namespace game
{ // scenegraph
  struct attach_to_fn
  : fn<attach_to_fn, max_arity<2>>
  {
    static auto eval
    (NodePath root, p3d::Node node) -> NodePath
    {
      return root.attach_new_node( node );
    }
  };
  let attach_to = attach_to_fn{};

  struct group_node_fn
  : fn<group_node_fn>
  {
    template<class S>
    requires std::is_convertible_v<S, std::string>
    static auto eval
    (S name, auto... nodes) -> p3d::Node
    {
      auto root = p3d::make<PandaNode>( name );

      ( ..., root->add_child( nodes ) );

      return root;
    }
    static auto eval
    (auto... nodes) -> p3d::Node
    {
      return eval( "", nodes... );
    }
  };
  let group_node = group_node_fn{};

  struct make_path_fn
  : fn<make_path_fn, no_partial_application>
  {
    template<p3d::Node Root, p3d::Node Next>
    static auto eval
    (Root r, Next n, auto... more) -> Root
    {
      r->add_child( n );

      eval( n, more... );

      return r;
    }

    static auto eval
    (p3d::Node root) -> p3d::Node
    {
      return root;
    }
  };
  let make_path = make_path_fn{};
}
namespace game
{ // color
  struct Color
  {
    float r, g, b, a = 1;

    auto red(float r) const -> Color
    {
      return { r, g, b, a };
    }
    auto green(float g) const -> Color
    {
      return { r, g, b, a };
    }
    auto blue(float b) const -> Color
    {
      return { r, g, b, a };
    }
    auto alpha(float a) const -> Color
    {
      return { r, g, b, a };
    }

    operator LColor() const
    {
      return { r, g, b, a };
    }
  };

  let red     = Color{ 1, 0, 0, 1 };
  let green   = Color{ 0, 1, 0, 1 };
  let blue    = Color{ 0, 0, 1, 1 };
  let yellow  = Color{ 1, 1, 0, 1 };
  let cyan    = Color{ 0, 1, 1, 1 };
  let magenta = Color{ 1, 0, 1, 1 };
  let white   = Color{ 1, 1, 1, 1 };
  let black   = Color{ 0, 0, 0, 1 };
}
namespace game
{ // render attributes
  struct enable_transparency_fn
  : fn<enable_transparency_fn, unary>
  {
    static auto eval
    (p3d::Node node) -> p3d::Node
    {
      node->set_attrib(
        TransparencyAttrib::make(
          TransparencyAttrib::M_dual
        ) 
      );

      return node;
    }
    static auto eval
    (NodePath node) -> NodePath
    {
      eval( node.node() );

      return node;
    }
    static auto eval
    (WindowFramework* window) -> WindowFramework*
    {
      eval( window->get_render() );
      eval( window->get_render_2d() );

      return window;
    }
  };
  struct set_color_fn
  : fn<set_color_fn>
  {
    static auto eval
    (Color c, PT(TextNode) node) -> p3d::Node
    {
      node->set_text_color( c );

      return node;
    }
    static auto eval
    (Color c, p3d::Node node) -> p3d::Node
    {
      node->set_attrib(
        ColorAttrib::make_flat( c )
      );

      return node;
    }
    static auto eval
    (Color c, NodePath node) -> NodePath
    {
      node.set_color( c );

      return node;
    }
    static auto eval
    (Color c, WindowFramework* w) -> WindowFramework*
    {
      w->get_display_region_3d()
      ->set_clear_color( c );

      return w;
    }
    static auto eval
    (Color c, PT(TextureStage) texture) -> PT(TextureStage)
    {
      texture->set_color( c );

      return texture;
    }
  };
  struct set_scale_fn
  : fn<set_scale_fn>
  {
    static auto eval
    (float a, p3d::Node node) -> p3d::Node
    {
      return eval( a, a, node );
    }
    static auto eval
    (float x, float y, p3d::Node node) -> p3d::Node
    {
      node->set_transform(
        node->get_transform()
        ->set_scale({ x, 1, y })
      );

      return node;
    }
    static auto eval
    (float x, float y, PT(TextNode) node) -> p3d::Node
    {
      eval( x, y, DCAST(PandaNode, node ) );

      return node;
    }
  };
  struct position_fn
  : fn<position_fn>
  {
    static auto eval
    (auto node) -> std::array<int, 2>
    {
      auto p = NodePath( node )
      .get_pos();

      return { int( p[0] ), int( p[2] ) };
    }
  };
  struct set_position_fn
  : fn<set_position_fn>
  {
    template<class Point>
    requires requires
    { std::tuple_size<Point>::value; }
    and requires
    { requires std::tuple_size_v<Point> == 2; }
    static auto eval
    (Point pos, p3d::Node node) -> p3d::Node
    {
      auto[ x,y ] = pos;

      return eval( x, y, node );
    }

    static auto eval
    (float x, float y, p3d::Node node) -> p3d::Node
    {
      node->set_transform(
        node->get_transform()
        ->set_pos(
          { x, 0, y }
        )
      );

      return node;
    }

    static auto eval
    (float x, float y, PT(TextNode) node) -> p3d::Node
    {
      eval( x, y,
        DCAST( PandaNode,
          node
        )
      );

      return node;
    }
  };
  struct draw_order_fn
  : fn<draw_order_fn>
  {
    static auto eval
    (auto node) -> int
    {
      return NodePath( node )
      .get_bin_draw_order();
    }
  };
  struct set_draw_order_fn
  : fn<set_draw_order_fn>
  {
    static auto eval
    (int order, auto node) -> auto
    {
      NodePath( node )
      .set_bin( "fixed", order, 0 );

      return node;
    }
  };
  struct set_rotation_fn
  : fn<set_rotation_fn>
  {
    static auto eval
    (float rad, p3d::Node node) -> p3d::Node
    {
      node->set_transform(
        node->get_transform()
        ->set_quat(
          { std::cos( rad/2 ), 0, std::sin( rad/2 ), 0 }
        )
      );

      return node;
    }

    static auto eval
    (float rad, PT(TextNode) node) -> p3d::Node
    {
      eval( rad, DCAST( PandaNode,
        node
      ) );

      return node;
    }
  };
  struct set_texture_fn
  : fn<set_texture_fn>
  {
    static auto eval
    (PT(Texture) texture, auto node) -> auto
    {
      auto tex_stage = p3d::make<TextureStage>("");

      tex_stage->set_mode(
        TextureStage::M_blend 
      );

      node->set_attrib(
        DCAST( const TextureAttrib,
          TextureAttrib::make()
        )
        ->add_on_stage(
          tex_stage,
          texture,
          0
        )
      );

      return node;
    }
    static auto eval
    (CPT(DynamicTextGlyph) glyph, Color color, auto node) -> auto
    {
      auto tex_stage = p3d::make<TextureStage>("");

      tex_stage->set_mode(
        TextureStage::M_blend 
      );

      tex_stage->set_color( color );

      node->set_attrib(
        DCAST( const TextureAttrib,
          TextureAttrib::make()
        )
        ->add_on_stage(
          tex_stage,
          glyph->get_page(),
          0
        )
      );

      return node;
    }
  };

  let set_color = set_color_fn{};
  let set_scale = set_scale_fn{};
  let set_rotation = set_rotation_fn{};
  let set_texture = set_texture_fn{};

  let position = position_fn{};
  let set_position = set_position_fn{};

  let draw_order = draw_order_fn{};
  let set_draw_order = set_draw_order_fn{};

  let enable_transparency = enable_transparency_fn{};
}
namespace game
{ // camera
  struct move_camera_to_fn
  : fn<move_camera_to_fn>
  {
    static auto eval
    (float x, float y, float z, WindowFramework* w) -> WindowFramework*
    {
      w->get_camera_group()
      .set_pos( x, y, z );

      return w;
    }
  };

  let move_camera_to = move_camera_to_fn{};
}
namespace game
{ // glyph
  struct load_font_fn
  : fn<load_font_fn>
  {
    static auto eval
    (std::string path, int size = 36) -> PT(DynamicTextFont)
    {
      auto font = p3d::make<DynamicTextFont>( path );

      font->set_point_size( size );
      font->set_texture_margin( 0 );
      font->set_fg( { 1, 1, 1, 1 } );
      font->set_bg( { 0, 0, 0, 0 } );
      font->set_outline( { 1, 1, 1, 1 }, 1, 1.5 );

      return font;
    }
  };
  struct get_glyph_fn
  : fn<get_glyph_fn>
  {
    static auto eval
    (int code, PT(DynamicTextFont) font) -> CPT(DynamicTextGlyph)
    {
      auto glyph = CPT(TextGlyph){};

      font->get_glyph(
        code, glyph 
      );

      return DCAST(
        const DynamicTextGlyph,
        glyph
      );
    }
  };

  let load_font = load_font_fn{};
  let get_glyph = get_glyph_fn{};
}
namespace game
{ // physics
  struct Physics : PandaNode
  {
    PT(CollisionHandlerQueue) queue;
    CollisionTraverser traverser;

    Physics()
    : PandaNode( "physics" )
    {
      queue = new CollisionHandlerQueue();
    }

    void add_collider(p3d::Node collider)
    {
      PandaNode::add_child( collider );

      traverser.add_collider(
        NodePath( collider )
        .find( "**/+CollisionNode" ),
        queue 
      );
    }

    void update(auto handle_collision)
    {
      traverser.traverse( NodePath( this ) );

      for( auto i = queue->get_num_entries(); i --> 0; )
      {
        auto& e = *queue->get_entry( i );

        handle_collision(
          e.get_from_node(),
          e.get_into_node()
        );
      }

      queue->clear_entries();
    }
  };
}
namespace game
{ // entity
  struct Entity
  {
    enum Type { item, agent, other };
    enum class Shape { tri, quad, circ };

    static auto shape_of
    (Type t) -> Shape
    {
      switch( t )
      {
        case item: return Shape::tri;
        case agent: return Shape::circ;
        case other: return Shape::quad;
        default: assert(0);
      }
    }

    struct create_fn
    : fn<create_fn>
    {
      static auto eval
      (PT(Physics) physics, PT(DynamicTextFont) font, string name, Type entity_type, Color geom_color, Color text_color) -> p3d::Node
      {
        auto physical = [&]() -> p3d::Node
        {
          auto node = p3d::make<CollisionNode>( "physical" );

          node->add_solid(
            p3d::make<CollisionSphere>(
              LPoint3{ 0,0,0 }, 0.49
            )
          );

          return node;
        }
        ();

        auto visual = [&]() -> p3d::Node
        {
          auto geom = p3d::make<GeomNode>( "" );

          switch( shape_of( entity_type ) )
          {
            case Shape::tri:
              geom = geom_node( regular_ngon<3>() );
              geom^set_rotation( -pi/2 );
              break;
            case Shape::quad:
              geom = geom_node( regular_ngon<4>() );
              geom^set_rotation( -pi/4 );
              break;
            case Shape::circ:
              geom = geom_node( regular_ngon<16>() );
              break;
          }

          geom^set_color( geom_color );

          auto text = p3d::make<TextNode>( "" );
          text->set_font( font );
          text->set_text( name );
          text->set_glyph_shift( -0.5 );
          text->set_align( TextNode::A_center );
          text^set_color( text_color );
          text^set_scale( 0.4 );

          geom^set_draw_order( 1 );
          text^set_draw_order( 0 );

          return group_node( "visual",
            text, geom
          );
        }
        ();

        auto entity = group_node( name,
          physical, 
          visual^set_scale( 0.5 - 0.1 )
        );

        physics->add_collider( entity );

        return entity;
      }
    };
    let create = create_fn{};
  };
}
namespace game
{ // prototypes
  template<auto i>
  void prototype(int, char**);
  template<>
  void prototype<0>(int argc, char** argv)
  {
    p3d::main( argc, argv,
      [](auto framework, auto window, auto keymap)
      {
        auto font = load_font(
          "font/DejaVuSansMono.ttf", 24
        );
        auto attach = attach_to(
          window->get_render()
        );

        window
        ^set_color( black )
        ^enable_transparency
        ^move_camera_to( 0, -30, 0 );

        auto physics = p3d::make<Physics>();
        attach( physics );

        let grid_size = 7;
        { // make grid
          auto grid = geom_node( unit_grid( grid_size ) )
          ^set_scale( grid_size/2.f )
          ^set_color( magenta.alpha( 0.2 ) )
          ^set_draw_order( std::numeric_limits<int>::max() );

          attach( grid );
        }

        auto fred = Entity::create( 
          physics, font,
          "frd", Entity::agent,
          Color{ 0.8, 0.6, 0.0 },
          Color{ 0.8, 0.2, 0.1, 0.5 }
        );
        auto george = Entity::create( 
          physics, font,
          "grg", Entity::agent,
          Color{ 0.3, 0.8, 0.5 },
          Color{ 0.2, 0.2, 0.1, 0.5 }
        )
        ^set_position( 3, 3 );

        struct MoveCmd
        {
          PandaNode* entity;
          int dx, dy;
        };

        auto move_cmds = std::make_shared<
          std::vector<MoveCmd>
        >();
        auto try_move = [=]
        (p3d::Node entity, int dx, int dy) -> std::function<void()>
        {
          return [=]()
          { move_cmds->push_back({ entity, dx, dy }); };
        };

        p3d::Node player = fred;

        *keymap = {
          { "escape", [](){ exit(0); } }
          , { "w", try_move( player, 0, +1 ) }
          , { "a", try_move( player, -1, 0 ) }
          , { "s", try_move( player, 0, -1 ) }
          , { "d", try_move( player, +1, 0 ) }
        };

        return [=]()
        {
          auto prev_positions 
          = std::map<PandaNode*, std::array<int, 2>>{};

          auto reset = [&](auto entity)
          {
            if( prev_positions.count( entity ) )
            {
              entity^set_position(  
                prev_positions[ entity ]
              );

              prev_positions.erase( entity );
            }
          };

          { // update
            for( auto[ entity, dx,dy ]: *move_cmds )
            {
              auto[ x,y ] = entity^position;

              if( not prev_positions.count( entity ) )
                prev_positions[ entity ] = { x,y };

              entity^set_position( x + dx, y + dy );
            }
            move_cmds->clear();
          }
          { // conflict resolution
            for( auto[ entity, _ ]: prev_positions )
            {
              auto[ x,y ] = entity^position;;

              if( std::abs( x )*2 > grid_size
                or std::abs( y )*2 > grid_size
              )
                reset( entity );
            }

            physics->update( [&]
              (auto from, auto into)
              {
                reset( from->get_parent( 0 ) );
                reset( into->get_parent( 0 ) );
              }
            );
          }
        };
      }
    );
  }

  struct Console : PGEntry
  {
    Console(std::string name, size_t max_buffer_lines, size_t width = 12)
    : PGEntry( name )
    , buffer( max_buffer_lines, "" )
    , input_display( p3d::make<TextNode>( "input" ) )
    , buffer_display( p3d::make<TextNode>( "buffer" ) )
    , backdrop( p3d::make<GeomNode>( "backdrop" ) )
    , frame( p3d::make<GeomNode>( "frame" ) )
    , max_buffer_lines( max_buffer_lines )
    , width( width )
    {
      setup_input();
      setup_buffer();
      setup_cursor();
      setup_backdrop();
    }

    void set_text_color(Color c)
    {
      buffer_display->set_text_color( c );

      input_display->set_text_color( c );

      PGEntry::get_cursor_def()
      .node()
      ^set_color( c.alpha( c.a * 0.5 ) );

      frame^set_color( c.alpha( c.a * 0.25 ) );
    }
    void set_backdrop_color(Color c)
    {
      backdrop^set_color( c );
    }

    void echo(std::string str)
    {
      for( auto s :
        str^range::from_container
        ^range::split( '\n' )
        ^range::transform(
          range::to_container<std::basic_string> 
        )
      )
      {
        buffer.push_back( s );
        buffer.pop_front();
      }
    }
    void consume_input()
    {
      auto input = PGEntry::get_text();

      auto split = input
      ^range::from_container
      ^range::split( ' ' ) // TODO range::split( std::iswhite )
      ^range::transform(
        range::to_container<std::basic_string>
      );

      PGEntry::set_text({});

      PGEntry::set_focus( true );

      if( split^range::empty )
        return;

      auto[ cmd, args ] = split^(
        range::front 
        & ( range::drop( 1 ) 
          >> range::to_container<std::vector>
        )
      );

      echo( input );

      if( commands.count( cmd ) )
        commands.at( cmd )( args );
      else
        echo( "command not found" );

      refresh_buffer();
    }
    void refresh_buffer()
    {
      auto display_text = std::string{};

      for( auto cmd : buffer )
        display_text += cmd + "\n";

      buffer_display->set_text( display_text );
    }

    std::map<std::string,
      std::function<void(std::vector<std::string>)>
    > commands;

  private:

    void setup_input()
    {
      PGEntry::setup( width, 1 );
      PGEntry::set_focus( true );
      PGEntry::set_text_def( 0, input_display );
    }
    void setup_buffer()
    {
      PandaNode::add_child( buffer_display );

      buffer_display
      ^set_position( 0, max_buffer_lines );
    }
    void setup_cursor()
    {
      auto root = PGEntry::get_cursor_def()
      .node();

      auto geometry = DCAST( GeomNode,
        root->get_child( 0 )
      );

      geometry
      ->add_geom( normalized_quad() );

      root
      ^set_scale( 0.5, 0.1 );
    }
    void setup_backdrop()
    {
      PGFrameStyle frame_style;

      frame_style.set_type( PGFrameStyle::T_none );

      PGEntry::set_frame_style( 0, frame_style );

      backdrop->add_geom( normalized_quad() );
      frame->add_geom( normalized_box() );

      PandaNode::add_child( backdrop );
      PandaNode::add_child( frame );

      backdrop
      ^set_position( -0.4, -0.5 )
      ^set_scale(
        PGEntry::get_max_width() + 1,
        max_buffer_lines + 1.6
      );

      frame
      ^set_position( -0.1, -0.2 )
      ^set_scale(
        PGEntry::get_max_width() + 0.5,
        max_buffer_lines + 1
      );
    }

    std::deque<std::string> buffer;
    PT(TextNode) input_display;
    PT(TextNode) buffer_display;
    PT(GeomNode) backdrop;
    PT(GeomNode) frame;
    size_t max_buffer_lines, width;
  };

  template<>
  void prototype<1>(int argc, char** argv)
  {
    p3d::main( argc, argv,
      [](auto framework, auto window, auto keymap)
      {
        auto font = load_font(
          "font/DejaVuSansMono.ttf", 24
        );

        window
        ^set_color( black )
        ^enable_transparency
        ^move_camera_to( 0, -30, 0 );

        auto quit = [](auto...){ exit(0); };

        auto console = p3d::make<Console>( "", 12 );
        console->set_backdrop_color( blue.alpha( 0.1 ) );
        console->set_text_color( blue.red( 0.5 ).green( 0.5 ) );

        console
        ^set_scale( 0.1 )
        ^set_position( -1, -0.5 )
        ^attach_to( window->get_aspect_2d() );

        console->commands[ "quit" ] = quit;
        console->commands[ "exit" ] = quit;
        console->commands[ "hello" ] = [=](auto args)
        {
          std::string s = "hi there";

          if( args.size() )
            s += ", ";
            
          for( auto a : args )
            s += a + " ";

          console->echo( s );
        };

        auto do_command = [=]()
        { console->consume_input(); };

        (*keymap)["enter"] = do_command;

        return [](){};
      }
    );
  }

  auto tokenize
  (std::string input) -> std::tuple<
    std::string,
    std::vector<std::string>
  >
  {
    auto split = input
    ^range::from_container
    ^range::split( ' ' )
    ^range::transform(
      range::to_container<std::basic_string>
    );

    if( split^range::empty )
      return { "", {} };

    return split^(
      range::front 
      & ( range::drop( 1 ) 
        >> range::to_container<std::vector>
      )
    );
  };

  struct ByName
  {
    auto operator()
    (auto&& a, auto&& b) const -> bool
    {
      return a.name() < b.name();
    }
  };

  struct Agent;

  struct Entity
  {
    struct Data;

    Entity(std::string name)
    : data( new Data{ name } )
    {}

    auto name() const -> std::string
    {
      return data->name;
    }

  private:

    struct Data
    {
      std::string name;
    };
    std::shared_ptr<Data> data;
  };

  struct Price
  {
    size_t credits;
  };
  struct Goods : Entity
  {
    struct Data;

    Goods(std::string name)
    : Entity( name )
    , data( new Data{} )
    {}

  private:

    struct Data
    {
    };
    std::shared_ptr<Data> data;
  };
  struct Zone : Entity
  {
    struct Data;

    Zone(std::string name)
    : Entity( name )
    , data( new Data{} )
    {}

    void link_to
    (Zone to, Price p)
    {
      assert( not data->transit.count( to ) );

      data->transit.insert({ to, p });
    }

  private:

    struct Data
    {
      std::map<Zone, Price, ByName> transit;
      std::set<Agent, ByName> population;
    };
    std::shared_ptr<Data> data;
  };
  struct Agent : Entity
  {
    struct Data;

    Agent(std::string name)
    : Entity( name )
    , data( new Data{} )
    {}

  private:

    struct Data
    {
      double credits = 0.0;
      Zone zone;
      std::map<Goods, size_t, ByName> inventory;
      std::map<Goods, Price, ByName> offers;
    };
    std::shared_ptr<Data> data;
  };

  auto operator<<
  (std::ostream& out, Goods goods) -> std::ostream&
  {
    return out << goods.name;
  }
  auto operator<<
  (std::ostream& out, Price price) -> std::ostream&
  {
    return out << price.credits << " credits";
  }
  auto operator<<
  (std::ostream& out, Zone zone) -> std::ostream&
  {
    return out << "zone " << zone.name();
  }
  auto operator<<
  (std::ostream& out, Agent agent) -> std::ostream&
  {
    return out << agent.name();
  }

  void offer_goods
  (Goods goods, Price price, Agent& agent)
  {
    assert( agent.inventory.count( goods ) );

    agent.offers[ goods ] = price;
  }
  void go_to
  (Zone& to, Agent& agent)
  {
    auto& from = *agent.zone;

    assert( from.transit.count( to ) );

    assert( agent.credits >= from.transit.at( to ).credits );

    agent.credits -= from.transit.at( to ).credits;
    agent.zone = &to;

    from.population.erase( agent );
    to.population.insert( agent );
  }
  void sell
  (Goods goods, Agent& buyer, Agent& seller)
  {
    assert( seller.inventory.count( goods ) );
    assert( seller.offers.count( goods ) );

    auto price = seller.offers[ goods ];

    assert( buyer.credits >= price.credits );

    buyer.credits -= price.credits;

    auto& count = seller.inventory[ goods ];

    --count;
    if( count == 0 )
    {
      seller.inventory.erase( goods );
      seller.offers.erase( goods );
    }

    ++buyer.inventory[ goods ];

    seller.credits += price.credits;
  }

  void look(Zone& zone)
  {
    show( zone, '\n' );

    for( auto b : zone.population )
      show( b, '\n' );
  }
  void look(Agent& agent)
  {
    show( agent, '\n' );
    
    for( auto offer : agent.offers )
      show( '\t', offer, '\n' );
  }

  void show_transits(Zone& zone)
  {
    show( "from ", zone.name(), " to:", '\n' );

    for( auto offer : zone.transit )
      show( offer, '\n' );
  }

  template<>
  void prototype<2>(int argc, char** argv)
  {
    auto region = Region{};
    region.create_zone( "A" );
    region.create_zone( "B" );

    region.make_transit_link( "A", "B", Price{ 10 } );
    region.make_transit_link( "B", "A", Price{ 5 } );

    offer_goods( region.zone( "A" ), Goods{ "guns" }, Price{ 100 } );
    offer_goods( region.zone( "B" ), Goods{ "butter" }, Price{ 25 } );

    auto player = Agent{ "fred", 100, &region.zone( "A" ) };

    look( *player.zone );
    for(;;)
    {
      std::string input;
      std::getline( std::cin, input );

      auto[ cmd, args ] = input^tokenize;

      if( cmd == "exit" )
        break;
      else if( cmd == "look" )
      {
        if( args.empty() )
        {
          look( *player.zone );
        }
        else
        {
          auto at = args[0];

          assert(0);
        }
      }
      else if( cmd == "market" )
      {
        show_market( *player.zone );
      }
      else if( cmd == "transit" )
      {
        show_transits( *player.zone );
      }
      else if( cmd == "goto" )
      {
        player^go_to( region.zone( args[0] ) );
      }

      std::flush( std::cout );
    }
  }
}

int main(int argc, char** argv)
{
  game::prototype<2>( argc, argv );
}
