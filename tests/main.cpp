#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreorder" 
#pragma GCC diagnostic ignored "-Wsign-compare" 
#include<pandaFramework.h>
#include<pandaSystem.h>
#include<load_prc_file.h>
#include<fontPool.h>
#include<geomTriangles.h>
#include<orthographicLens.h>
#include<pnmImage.h>
#pragma GCC diagnostic pop
#include<texture-font.h>

namespace app
{
  void quit(const Event*, void*)
  {
    exit(0);
  }
}

namespace ftgl
{
  struct TextureAtlas
  : std::shared_ptr<texture_atlas_t>
  {
    auto operator->() -> texture_atlas_t*
    {
      return *this;
    }

    TextureAtlas(
      const size_t width,
      const size_t height,
      const size_t depth
    )
    : std::shared_ptr<texture_atlas_t>(
      texture_atlas_new( width, height, depth ),
      &texture_atlas_delete
    ){}

    operator texture_atlas_t*()
    {
      return std::shared_ptr<texture_atlas_t>
      ::get();
    }
  };
  struct TextureFont
  : std::shared_ptr<texture_font_t>
  {
    auto get_glyph(const char* codepoint)
    {
      return texture_font_get_glyph(
        static_cast<texture_font_t*>( *this ),
        codepoint
      );
    }
    -> texture_glyph_t*
    auto find_glyph(const char* codepoint)
    -> texture_glyph_t*
    {
      return texture_font_find_glyph(
        static_cast<texture_font_t*>(
          *this 
        ),
        codepoint
      );
    }

    auto operator->() -> texture_font_t*
    {
      return *this;
    }

    TextureFont(
      TextureAtlas atlas,
      const float pt_size,
      std::string filename
    )
    : std::shared_ptr<texture_font_t>(
      texture_font_new_from_file(
        static_cast<texture_atlas_t*>(
          atlas
        ),
        pt_size,
        filename.c_str()
      ),
      &texture_font_delete
    )
    , atlas( atlas )
    {}

    operator texture_font_t*()
    {
      return std::shared_ptr<texture_font_t>
      ::get();
    }

  private:

    TextureAtlas atlas;
  };
}

namespace//p3d
{
  template<
    class PandaObj,
    class... Xs
  >
  auto p3d_new(Xs... xs) 
  -> PointerTo<PandaObj>
  {
    return new PandaObj( xs... );
  }

  struct TextureFont
  {
    auto get_glyph(const char* codepoint)
    -> texture_glyph_t*
    {
      if( auto glyph = font
        .find_glyph( codepoint );
        glyph != nullptr
      )
      {
        /* TODO
          when glyph node created,
          query texfont
          texfont will load if needed
          and set needs_reload flag;
          on glyph node draw callback,
          check needs_reload flag
          of texfont, and reload if set
        */
      }
    }

    TextureFont(
      const size_t width,
      const size_t height,
      const float pt_size,
      std::string filename
    )
    : font(
      ftgl::TextureAtlas(
        width, height, 1
      ),
      pt_size, filename
    )
    , texture( new Texture() )
    {
      auto image = PNMImage(
        width, height, 1
      );

      image.fill_val( 255 );
      image.add_alpha();

      auto src = font->atlas->data;
      auto tgt = image.get_alpha_array();

      std::copy(
        src, src + ( height * width ),
        tgt
      );

      texture->load( image );
    }

    ftgl::TextureFont font;
    PointerTo<Texture> texture;
    bool needs_reload = true;
  };

  struct GlyphNode : GeomNode
  {
    void set_color(
      float r, float b, float g, float a
    )
    {
      NodePath( this )
      .set_color_scale(
        r, g, b, a 
      );
    }

    GlyphNode(
      std::string name,
      TextureFont font,
      const char* codepoint
    )
    : GeomNode( name )
    {
      // TODO single instance in texfont
      auto vdata = p3d_new<
        GeomVertexData
      >(
        name + "_quad",
        GeomVertexFormat::get_v3t2(),
        Geom::UH_static
      );
      vdata->set_num_rows( 4 );

      auto vertex = GeomVertexWriter(
        vdata, "vertex"
      );
      {
        vertex.add_data3f( -0.5, 0, -0.5 );
        vertex.add_data3f( +0.5, 0, -0.5 );
        vertex.add_data3f( +0.5, 0, +0.5 );
        vertex.add_data3f( -0.5, 0, +0.5 );
      }

      auto glyph = font.font
      .get_glyph( codepoint );

      auto texcoord = GeomVertexWriter(
        vdata, "texcoord"
      );
      {
        texcoord.add_data2f( 
          glyph->s0, glyph->t0
        );
        texcoord.add_data2f( 
          glyph->s1, glyph->t0
        );
        texcoord.add_data2f( 
          glyph->s1, glyph->t1
        );
        texcoord.add_data2f( 
          glyph->s0, glyph->t1
        );
      }

      auto prim = p3d_new<GeomTriangles>(
        Geom::UH_static
      );
      prim->add_vertices( 0, 1, 2 );
      prim->add_vertices( 0, 2, 3 );

      auto geom = p3d_new<Geom>(
        vdata
      );
      geom->add_primitive( prim );

      GeomNode::add_geom( geom );

      NodePath( this )
      .set_texture( font.texture );
    }
  };
}

int main(int argc, char** argv)
{
  auto framework = PandaFramework{};
  framework.open_framework(
    argc, argv 
  );

  framework.set_window_title(
    "hello" 
  );
  auto& window = *framework
  .open_window();

  // a
  auto font = TextureFont(
    1024, 1024,
    48,
    "./font/DejaVuSansMono.ttf"
  );
  /* TODO
    to make this dynamic...
    retain all data (cpu and gpu)
    fetch texcoords on draw
    if glyph not found, rebuild texture
  */
  //font.get_glyph( "☢" );
  //font.get_glyph( "☣" );

  auto glyph_node = p3d_new<
    GlyphNode
  >( "glyphnode", font, "≢" );

  // kb
  window.enable_keyboard();
  framework.define_key(
    "escape", "", &app::quit, nullptr 
  );

  // TODO ortho lens, disable depth?
  auto camera = window
  .get_camera_group();
  //camera.set_pos( gpath, 0, -1, 0 );

  framework.main_loop();
  framework.close_framework();

  return EXIT_SUCCESS;
}
