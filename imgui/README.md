# Setting up MultiThreaded ImGUI
This tutorial is written for a Docking Branch of ImGui. As it allows dragging windows out of the main window (and docking them), it interacts with Windowing APIs a lot more than regular branch, meaning that it should be a lot harder to make it work with docking rather than regular, so if it works here it should work on regular branch aswell.

This tutorial is written for ImGui Version 1.92.0 Docking Branch. Should work with newer versions aswell. This tutorial also expects YOU to patch some ImGui files. Don't expect it to give you a ready to use code replacements, as ImGui updates regularily, so it's better to just write a "one fit all" tutorial.

**Important Thing:** GLFWM and this tutorial only supports OpenGL! Vulkan isn't tested or implemented!

## How to set it up?

According to the ImGui docs (code comments), to make imgui thread-safe you need to change global variable `ImGuiContext* GImGui` into a `thread_local` one:
```c++
// v1.92.8-docking imgui.cpp Line 1481:

//   If you want thread-safety to allow N threads to access N different contexts:
//   - Change this variable to use thread local storage so each thread can refer to a different context, in your imconfig.h:
//         struct ImGuiContext;
//         extern thread_local ImGuiContext* MyImGuiTLS;
//         #define GImGui MyImGuiTLS
//     And then define MyImGuiTLS in one of your cpp files. Note that thread_local is a C++11 keyword, earlier C++ uses compiler-specific keyword.
#ifndef GImGui
ImGuiContext*   GImGui = NULL;
#endif
```
What does it mean? It means you go into the `imconfig.h` and add it at the bottom:
```c++
struct ImGuiContext;
extern thread_local ImGuiContext* MyImGuiTLS;
#define GImGui MyImGuiTLS
```
Then you also create another .cpp file (for example `imconfig.cpp`):
```c++
struct ImGuiContext;
thread_local ImGuiContext* MyImGuiTLS;
```

## Implementation Files

You can try using premade implementation files for ImGui using glfwm. The problem is that they are made for ImGui v1.92.8 Docking Branch. You can test if it works with your version/branch and if it doesn't, you can use the tutorial on how to patch them yourself.

One important thing: With included files, each `ImGui_ImplGlfw_...` is changed into `ImGui_ImplGlfwm_...`

## Manual Patching of Implementation Files

You modify the `imgui_impl_glfw.cpp`. You can create a copy of it named `imgui_impl_glfwm.cpp` to keep things separate.

Change this variable into `thread_local` one (line 221 v1.92.8-docking):
```c++
static ImVector<ImGui_ImplGlfw_WindowToContext> g_ContextMap;
// Change into:
static thread_local ImVector<ImGui_ImplGlfw_WindowToContext> g_ContextMap;
```

At the end of `ImGui_ImplGlfw_WindowFocusCallback` function add this code:
```c++
    // Only propagate focus events to ImGui for the main window
    if (window == bd->Window)
    {
        ImGuiIO& io = ImGui::GetIO(bd->Context);
        io.AddFocusEvent(focused != 0);
    }
```

Replace `ImGui_ImplGlfw_CreateWindow` implementation with this one:
```c++
static void ImGui_ImplGlfw_CreateWindow(ImGuiViewport* viewport)
{
    ImGui_ImplGlfw_Data* bd = ImGui_ImplGlfw_GetBackendData();
    ImGui_ImplGlfw_ViewportData* vd = IM_NEW(ImGui_ImplGlfw_ViewportData)();
    viewport->PlatformUserData = vd;

    // Workaround for Linux: ignore mouse up events corresponding to losing focus of the previously focused window (#7733, #3158, #7922)
#ifdef __linux__
    bd->MouseIgnoreButtonUpWaitForFocusLoss = true;
#endif

    // GLFW 3.2 unfortunately always set focus on glfwCreateWindow() if GLFW_VISIBLE is set, regardless of GLFW_FOCUSED
    // With GLFW 3.3, the hint GLFW_FOCUS_ON_SHOW fixes this problem
    glfwWindowHint(GLFW_VISIBLE, false);
    glfwWindowHint(GLFW_FOCUSED, false);
#if GLFW_HAS_FOCUS_ON_SHOW
    glfwWindowHint(GLFW_FOCUS_ON_SHOW, false);
#endif
    glfwWindowHint(GLFW_DECORATED, (viewport->Flags & ImGuiViewportFlags_NoDecoration) ? false : true);
#if GLFW_HAS_WINDOW_TOPMOST
    glfwWindowHint(GLFW_FLOATING, (viewport->Flags & ImGuiViewportFlags_TopMost) ? true : false);
#endif

    GLFWwindow* share_window = (bd->ClientApi == GlfwClientApi_OpenGL) ? bd->Window : nullptr;

    // WGL (Windows OpenGL) requires the main context to NOT be current on any thread
    // while wglShareLists is called internally by glfwCreateWindow. If it is still
    // current, context creation fails with GLFW error 65543 (WGL: Failed to create OpenGL context).
    if (share_window)
        glfwMakeContextCurrent(nullptr);

    vd->Window = glfwCreateWindow((int)viewport->Size.x, (int)viewport->Size.y, "No Title Yet", nullptr, share_window);

    // Restore main context after shared context creation
    if (share_window)
        glfwMakeContextCurrent(share_window);

    vd->WindowOwned = true;
    ImGui_ImplGlfw_ContextMap_Add(vd->Window, bd->Context);
    viewport->PlatformHandle = (void*)vd->Window;
#ifdef IMGUI_GLFW_HAS_SETWINDOWFLOATING
    ImGui_ImplGlfw_SetWindowFloating(vd->Window);
#endif
#ifdef _WIN32
    viewport->PlatformHandleRaw = glfwGetWin32Window(vd->Window);
    ::SetPropA((HWND)viewport->PlatformHandleRaw, "IMGUI_BACKEND_DATA", bd);
#elif defined(__APPLE__)
    viewport->PlatformHandleRaw = (void*)glfwGetCocoaWindow(vd->Window);
#endif
    glfwSetWindowPos(vd->Window, (int)viewport->Pos.x, (int)viewport->Pos.y);

    // Install GLFW callbacks for secondary viewports
    glfwSetWindowFocusCallback(vd->Window, ImGui_ImplGlfw_WindowFocusCallback);
    glfwSetCursorEnterCallback(vd->Window, ImGui_ImplGlfw_CursorEnterCallback);
    glfwSetCursorPosCallback(vd->Window, ImGui_ImplGlfw_CursorPosCallback);
    glfwSetMouseButtonCallback(vd->Window, ImGui_ImplGlfw_MouseButtonCallback);
    glfwSetScrollCallback(vd->Window, ImGui_ImplGlfw_ScrollCallback);
    glfwSetKeyCallback(vd->Window, ImGui_ImplGlfw_KeyCallback);
    glfwSetCharCallback(vd->Window, ImGui_ImplGlfw_CharCallback);
    glfwSetWindowCloseCallback(vd->Window, ImGui_ImplGlfw_WindowCloseCallback);
    glfwSetWindowPosCallback(vd->Window, ImGui_ImplGlfw_WindowPosCallback);
    glfwSetWindowSizeCallback(vd->Window, ImGui_ImplGlfw_WindowSizeCallback);
    if (bd->ClientApi == GlfwClientApi_OpenGL)
    {
        glfwMakeContextCurrent(vd->Window);
        glfwSwapInterval(0);
        glfwMakeContextCurrent(share_window); // Leave main context current when done
    }
}
```

Finally you can CTRL+F all of "glfw" and replace it with "glfwm" (do it with "match case" enabled).

You can also change "ImGui_ImplGlfw" into "ImGui_ImplGlfwm" as to differentiate two libraries even more.

At the end, fix glfw includes (after replace all thing) and in the implementation file you should include `glfwm.hpp` file just below including the glfw libraries (Around line 185 in v1.92.8-docking):
```c++
#include "glfwm.hpp"
using namespace Leszek::GLFWM;
```

There will be some errors like undefined functions etc. so just change them back to raw glfw functions (example `glfwmGetWin32Window` into `glfwGetWin32Window`).