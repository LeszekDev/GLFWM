![GLFWM](images/glfwm-banner.png)

# GLFWM - Multithreaded GLFW

**Version:** 0.9.0-beta  
**Author:** @LeszekDev  
**License:** Apache 2.0  
**Requires:** C++20, GLFW 3.4  
**Supported Graphics API:** Only OpenGL! (Vulkan Unsupported)

GLFWM is a single-header C++ wrapper around GLFW that allows calling GLFW functions from threads other than the main thread. It is designed as a **development and testing tool** - letting you run multiple app or game instances in parallel threads with minimal changes to your existing codebase.

Important things to know:
* **No Vulkan support** - OpenGL only for now.
* **Not release-ready** - use `LESZEK_GLFWM_PASSTHROUGH_MODE` for production builds (maps every `glfwm*` call directly to its `glfw*` counterpart).
* **Not all functions implemented** - see the Unsupported Functions section below.
* **Don't remove GLFW** - GLFWM is not meant to replace or rewrite all of GLFW functionality. Instead it's a fancy wrapper around GLFW to make multithreading easier.

## How It Works

GLFWM maintains an internal task queue. When any thread calls a `glfwm*` function, the call is packaged as a task and submitted to the queue. The main thread picks it up, executes the underlying GLFW call, and returns the result to the caller.

* **Void-returning functions** are fire-and-forget - they don't block the calling thread.
* **Value-returning functions** (e.g. `glfwmCreateWindow`) block until the main thread completes the task.
* **Calls on main thread** execute immediately with no queueing overhead.

**Window binding:** Each window is bound to the thread that created it. `glfwmPollEvents()` and all callbacks for that window must be driven from the same thread. `glfwmInit()` must also be called on every window thread.

## Key Features

* **Drop-in API** - function signatures mirror GLFW exactly; `glfwmCreateWindow` returns `GLFWwindow*`, etc.
* **Passthrough mode** (`LESZEK_GLFWM_PASSTHROUGH_MODE`) - zero overhead; all calls forward directly to GLFW. Useful for production or gradual migration.
* **Migration helper** (`LESZEK_GLFWM_FORBID_GLFW`) - makes direct `glfw*` calls compiler errors, helping locate unmigrated code.
* **Custom scheduler support** - `glfwm_setNewEventCallback()` notifies you when new tasks arrive, for integration with your own main-loop.

## Ideal Use Cases

* **Multiplayer/networking tests** - run two full game instances in separate threads and test client–server interaction. You don't need to configure running multiple processes after each compilation - just run multiple instances in multiple threads.
* **Multi-window tooling** - spin up independent editor panels or debug views on separate threads.
* **Automated UI testing** - drive multiple window threads from a test harness without restructuring your renderer.

## Integration

GLFWM is a single-header library. In **exactly one** `.cpp` file, define `LESZEK_GLFWM_IMPLEMENTATION` before including:

```cpp
// glfwm.cpp
#define LESZEK_GLFWM_IMPLEMENTATION
#include <glfw-3.4.h>
#include "glfwm.hpp"
```

Everywhere else, include both GLFW and `glfwm.hpp` (without the define).

Then drive the task queue from your main thread:

```cpp
// main.cpp
using namespace Leszek::GLFWM; // Namespace with all functions
int main() {
    std::thread gameThread = std::thread([&]() { 
        game.run(); // glfwCreateWindow, OpenGL context etc...
    });
    while (appRunning) {
        glfwm_defaultWaitForTasks(); // Built into GLFWM.hpp
        glfwm_executeQueueTasks(); // Built into GLFWM.hpp
    }
    return 0;
}
```

For custom scheduler integration, use `glfwm_setNewEventCallback()` to receive task-arrival notifications and call `glfwm_executeQueueTasks()` at your own cadence.

## Function Thread Context

Each function in the header is annotated with its required calling context:

* **Any-Thread** - safe from any thread.
* **Window-Thread** - must be called from the thread that created the window.

## Unsupported Functions

The following are not yet implemented and will throw `std::runtime_error` if called outside passthrough mode:

`glfwmInitAllocator`, `glfwmGetVideoModes`, `glfwmGetGammaRamp`, `glfwmSetGammaRamp`, `glfwmWaitEvents`, `glfwmWaitEventsTimeout`, `glfwmPostEmptyEvent`, `glfwmGetJoystickAxes`, `glfwmGetJoystickButtons`, `glfwmGetJoystickHats`, `glfwmGetJoystickName`, `glfwmGetJoystickGUID`, `glfwmSetJoystickUserPointer`, `glfwmGetJoystickUserPointer`, `glfwmSetJoystickCallback`, `glfwmUpdateGamepadMappings`, `glfwmGetGamepadName`, `glfwmVulkanSupported`, `glfwmGetRequiredInstanceExtensions`, and all Vulkan surface functions.

## Multithreaded ImGui?
Look at [imgui/README.md](imgui/README.md) if you want to setup multithreaded ImGui. There is a tutorial for patching imgui files aswell as already patched, ready to use `imgui_impl_glfwm.cpp`.

## FAQ
- **Is it meant to replace GLFW?** No, it's not! Instead it's mainly a tool for debugging and developing GLFW apps.
- **Is it meant to be fully asynchronous wrapper?** No, it's not! Instead it's mainly a tool for debugging and developing GLFW apps. GLFW is a mostly Main-Thread only library and GLFWM is meant to allow starting up multiple App/Game instances in multiple threads with basically 0 additional code (for example a developer testing some multiplayer system in a game) Apps made with GLFWM should be made the same way as they would be with GLFW, it's just that they can run on thread other than Main Thread, but they are still pretty limited to running on a single thread. It's caused by the need to stay compatible with GLFW API!
- **Are there any drawbacks when using GLFWM over vanilla GLFW?** Yes and No. You can always switch to the "passthrough mode" and then GLFWM will just forward all calls to GLFW. In standard mode there's still an overhead of queue and of need to wait for main thread to finish execution of function beforere turning to the calling thread. To enable a "Drop-in" experience every function is the same as it's GLFW counter-part, meaning that for example glfwmCreateWindow returns GLFWwindow just like glfw does, meaning that it needs to wait for main thread before continuing.
- **Are there better solutions for multithreaded windowed apps?** Yes. The best solution would probably be to completely redesign the api so that it would work entirely on custom handles that would return instantly instead of being dependent on main thread running glfw functions creating handle and thus making the App/Game thread run slower.
- **Will the library be redesigned for better performance?** There will probably be some optimalizations by adding cache here and there, removing the task schedule somewhere and other stuff like that, but since it's meant as a drop-in replacement of GLFW i don't believe there's lots of room where stuff can be optimized without breaking the API. Maybe some time in the future I'll rewrite it from a `GLFW compatible GLFWM API -> GLFW` wrapper into something like `GLFW compatible GLFWM API -> Fully usable Asynchronous GLFWM API -> GLFW`, but it's something for the far far future.
- **When are Window events executed?** Each window is tied to the thread that created it. When that thread does glfwmPollEvents(), every event related to every window tied to the current thread gets processed. This means that the events are safely separated across different threads. Monitor Event Callbacks are also tied to the thread that registered the callback, and callbacks process those events when glfwmPollEvents() on their tied thread is called.
- **Some callbacks are not running for me.** Other than the requirement to run glfwmPollEvents() on the thread where window has been created, you also need to do glfwmInit() on that thread. glfwmInit() sets up thread data and without it the glfwmPollEvents() just won't work.
- **Are there any FPS drops with it?** Yes, although they are mostly noticeable only with the higher FPS counts  (like 500+). This is mainly Development tool, but the drops aren't/shouldn't be as bad as to make the games unplayable and untestable. Drops are caused by the scheduling mechanism meanwhile games on main thread execute GLFW functions immediately. Should be fine for testing purposes, and for production you can always just put it on main thread and enable passthrough mode (same as directly running glfw functions).
- **Will Vulkan be supported in future?** Library is still in the beta, but the plans are to also support Vulkan.

## License

Copyright 2026 @LeszekDev

Licensed under the Apache License, Version 2.0. See the bottom of `glfwm.hpp` for the full license text.