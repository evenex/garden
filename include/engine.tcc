#pragma once

namespace game
{ // geometry
  template<class Primitive>
  struct geometry_fn
  : Fn<geometry_fn<Primitive>>
  {
    FIELD_MEM( format, const GeomVertexFormat*, {} )
    FIELD_MEM( vertex_usage, Geom::UsageHint, {} )
    FIELD_MEM( primitive_usage, Geom::UsageHint, {} )

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
  : Fn<geom_node_fn>
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
  : Memoized<PT(Geom)(),
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
  : Fn<unit_grid_fn>
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
  : Memoized<PT(Geom)(),
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
  : Memoized<PT(Geom)(),
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
  : Fn<attach_to_fn>
  {
    let assume(auto){}
    let assume(auto,auto){}

    static auto eval
    (NodePath root, p3d::Node node) -> NodePath
    {
      return root.attach_new_node( node );
    }
  };
  let attach_to = attach_to_fn{};

  struct group_node_fn
  : Fn<group_node_fn>
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
  : Fn<make_path_fn, no_partial_application>
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
  : Fn<enable_transparency_fn>
  {
    let assume(auto){}

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
  : Fn<set_color_fn>
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
  : Fn<set_scale_fn>
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
  : Fn<position_fn>
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
  : Fn<set_position_fn>
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
  : Fn<draw_order_fn>
  {
    static auto eval
    (auto node) -> int
    {
      return NodePath( node )
      .get_bin_draw_order();
    }
  };
  struct set_draw_order_fn
  : Fn<set_draw_order_fn>
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
  : Fn<set_rotation_fn>
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
  : Fn<set_texture_fn>
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
  : Fn<move_camera_to_fn>
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
  : Fn<load_font_fn>
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
  : Fn<get_glyph_fn>
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
