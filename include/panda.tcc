#pragma once
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreorder" 
#pragma GCC diagnostic ignored "-Wsign-compare" 
// system
#include<pandaFramework.h>
#include<pandaSystem.h>
// attrib
#include<colorAttrib.h>
// geometry
#include<geomPoints.h>
#include<geomLines.h>
#include<geomLinestrips.h>
#include<geomTriangles.h>
#include<geomTristrips.h>
// font
#include<dynamicTextFont.h>
// physics
#include<collisionSphere.h>
#include<collisionNode.h>
#include<collisionHandlerQueue.h>
#include<collisionTraverser.h>
// gui
#include<pgEntry.h>
#pragma GCC diagnostic pop

namespace p3d
{
  using main_loop_fn_t = std::function<void()>;
  using key_response_fn_t = std::function<void()>;
  using main_setup_fn_t = std::function<
    main_loop_fn_t(
      PandaFramework*,
      WindowFramework*,
      std::map<std::string, key_response_fn_t>*
    )
  >;

  static auto main_loop_callback
  (GenericAsyncTask*, void* f_addr) -> AsyncTask::DoneStatus
  {
    auto& f = *reinterpret_cast<
      main_loop_fn_t*
    >( f_addr );

    f();

    return AsyncTask::DS_cont;
  }

  static void key_callback
  (const Event*, void* f_addr)
  {
    auto& f = *reinterpret_cast<
      key_response_fn_t*
    >( f_addr );

    f();
  }

  template<class Panda>
  auto make
  (auto... args) -> PT(Panda)
  {
    PT(Panda) ptr;

    ptr = new Panda(
      std::forward<
        decltype(args)
      >( args )...
    );

    return ptr;
  }

  template<class X>
  concept bool Node = requires
  {
    requires std::is_same_v<PandaNode*, X>
    or std::is_base_of_v<PandaNode*, X>
    or std::is_base_of_v<PointerToVoid, X>
  };

  static auto main
  (int argc, char** argv, main_setup_fn_t setup)
  {
    auto framework = PandaFramework{};

    framework.open_framework(
      argc, argv 
    );

    framework.set_window_title(
      argv[0]
    );

    auto window = framework.open_window();

    if( window == nullptr )
      throw std::runtime_error(
        "failed to open window"
      );

    window->enable_keyboard();

    std::map<std::string,
      key_response_fn_t
    > keymap;

    auto loop = setup( &framework, window, &keymap );

    for( auto&[ key, response ] : keymap )
      framework.define_key(
        key, "", 
        &key_callback, &response
      );

    AsyncTaskManager::get_global_ptr()
    ->add( make<GenericAsyncTask>( 
      "main", main_loop_callback,
      &loop
    ) );

    framework.main_loop();

    framework.close_framework();
  }
}
