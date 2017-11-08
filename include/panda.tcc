#pragma once
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreorder" 
#pragma GCC diagnostic ignored "-Wsign-compare" 
// system
#include<pandaFramework.h>
#include<pandaSystem.h>
#pragma GCC diagnostic pop

namespace p3d
{
  using main_loop_fn_t = std::function<void()>;
  using main_setup_fn_t = std::function<
    main_loop_fn_t(
      PandaFramework&, WindowFramework&
    )
  >;

  static auto main_loop_callback
  (GenericAsyncTask*, void* f_addr) -> AsyncTask::DoneStatus
  {
    auto& f = *reinterpret_cast<main_loop_fn_t*>( f_addr );

    f();

    return AsyncTask::DS_cont;
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

  static auto main
  (int argc, char** argv, main_setup_fn_t f)
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

    auto loop = f( framework, *window );

    AsyncTaskManager::get_global_ptr()
    ->add( 
      make<GenericAsyncTask>( 
        "main", main_loop_callback,
        &loop
      )
    );

    framework.main_loop();

    framework.close_framework();
  }
}
