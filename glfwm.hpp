/*\
 * ======================================================================================
 *  File: GLFWM.hpp
 *  Author: @LeszekDev
 *  Version: 0.9.0-beta
 *  Repository: https://github.com/LeszekDev/GLFWM
 *  Requirements: C++20
 *  Required GLFW Version: 3.4
 *  Supported Graphics API: OpenGL ONLY! Vulkan Unsupported!
 * ======================================================================================
 *  License Agreement can be found at the bottom of this file
 * ======================================================================================
 *  This is a Single-Header library - You copy and paste it into your project.
 *  To set it up you should create a GLFWM.cpp and write there:
 * 
 *  #define LESZEK_GLFWM_IMPLEMENTATION
 *  #include <glfw-3.4.h>
 *  #include <glfwm.hpp>
 * 
 *  You should put implementation into ONLY ONE .cpp file! Putting it into multiple
 *  .cpp files is NOT SUPPORTED and will throw compiler errors!
 *  INFO: Everywhere you include glfwm.hpp, you should also include the GLFW library!
 *
 *  This library is a wrapper around every glfw function with a main purpose of
 *  allowing you to call glfw functions from threads other than the main-thread.
 *  Operating Systems enforce programs to call window related sys calls from main-thread.
 * 
 *  See the FAQ section below to get a better idea of what this library is.
 * 
 *  This wrapper basically has it's own event queue where every event is put,
 *  executed by main-thread and the return values are sent back to the calling thread.
 *  Some data is cached, but it's mostly:
 *  request added to queue -> main thread -> calling thread got the data
 *  Meaning that each call to glfwm, blocks the current thread until the main thread
 *  executes the task. If it's detected that the calling thread IS the main thread,
 *  then it automatically executes all tasks in queue.
 * 
 *  When migrating from glfw to glfwm, you can #define LESZEK_GLFWM_PASSTHROUGH_MODE
 *  With the Passthrough Mode, each function is just a passthrough to the glfw
 *  functions underneath them. This allows you to gradually migrate from glfw
 *  to glfwm and test if the app works without fully breaking it.
 * 
 *  You can also temporarily #define LESZEK_GLFWM_FORBID_GLFW so that C++ throws
 *  errors whenever you use glfw function. It makes it easier to locate old code
 *  and replace it with the glfwm wrapper.
 *
 *  How to setup the default queue executor on main thread:
 * 
 *  std::thread gameThread = std::thread([&]() { game.run(); });
 *  while(appRunning) {
 *      glfwm_defaultWaitForTasks();
 *      glfwm_executeQueueTasks();
 *  }
 * 
 * You can always setup your own system if you need more manual control
 * (for example to add your own scheduler). For more information look below.
 * 
 * IMPORTANT: glfwmInit() needs to be called on EVERY window thread.
 * Meaning that you cannot put it into the main thread and init the glfwm.
 * That's the important thing to know when you are going to run multiple
 * windows (or for example applications/games) on multiple threads.
 * 
 * For debugging undefined behaviour and other bugs related to this library
 * you can always #define LESZEK_GLFWM_ALWAYS_WAIT to disable "fire and forget"
 * optimizations.
 * 
 * INFO: This library is still in the testing phase, so many glfw functions are unsupported.
 * For more info about which function is supported, which function to call from which thread
 * and which function isn't yet implemented - look below
 * 
 * For information about maintaining the codebase look just below #ifdef LESZEK_GLFWM_IMPLEMENTATION
 * ======================================================================================
 *  FAQ Section:
 * 
 * [?] Is it meant to replace GLFW?
 *  >  No, it's not! Instead it's a wrapper that works alongside it.
 * 
 * [?] Is it meant to be fully asynchronous wrapper?
 *  >  No, it's not! Instead it's mainly a tool for debugging and developing GLFW apps.
 *     GLFW is a mostly Main-Thread only library and GLFWM is meant to allow starting up
 *     multiple App/Game instances in multiple threads with basically 0 additional code
 *     (for example a developer testing some multiplayer system in a game)
 *     Apps made with GLFWM should be made the same way as they would be with GLFW, it's just
 *     that they can run on thread other than Main Thread, but they are still pretty limited
 *     to running on a single thread. It's caused by the need to stay compatible with GLFW API!
 * 
 * [?] Are there any drawbacks when using GLFWM over vanilla GLFW?
 *  >  Yes and No. You can always switch to the "passthrough mode" and then GLFWM will
 *     just forward all calls to GLFW. In standard mode there's still an overhead of
 *     queue and of need to wait for main thread to finish execution of function before
 *     returning to the calling thread. To enable a "Drop-in" experience every function
 *     is the same as it's GLFW counter-part, meaning that for example glfwmCreateWindow
 *     returns GLFWwindow just like glfw does, meaning that it needs to wait for main thread
 *     before continuing.
 *
 * [?] Are there better solutions for multithreaded windowed apps?
 *  >  Yes. The best solution would probably be to completely redesign the api
 *     so that it would work entirely on custom handles that would return instantly
 *     instead of being dependent on main thread running glfw functions creating handle
 *     and thus making the App/Game thread run slower.
 * 
 * [?] Will the library be redesigned for better performance?
 *  >  There will probably be some optimalizations by adding cache here and there,
 *     removing the task schedule somewhere and other stuff like that, but since
 *     it's meant as a drop-in replacement of GLFW i don't believe there's lots of
 *     room where stuff can be optimized without breaking the API. Maybe some time
 *     in the future I'll rewrite it from a GLFW compatible GLFWM API -> GLFW wrapper
 *     into something like GLFW compatible GLFWM API -> Fully usable Asynchronous
 *     GLFWM API -> GLFW, but it's something for the far far future.
 * 
 * [?] When are Window events executed?
 *  >  Each window is tied to the thread that created it. When that thread does
 *     glfwmPollEvents(), every event related to every window tied to the current
 *     thread gets processed. This means that the events are safely separated across
 *     different threads. Monitor Event Callbacks are also tied to the thread that
 *     registered the callback, and callbacks process those events when glfwmPollEvents()
 *     on their tied thread is called.
 * 
 * [?] Some callbacks are not running for me.
 *  >  Other than the requirement to run glfwmPollEvents() on the thread where window
 *     has been created, you also need to do glfwmInit() on that thread. glfwmInit() sets up
 *     thread data and without it the glfwmPollEvents() just won't work.
 * 
 * [?] Are there any FPS drops with it?
 *  >  Yes, although they are mostly noticeable only with the higher FPS counts 
 *     (like 500+). This is mainly Development tool, but the drops aren't/shouldn't be 
 *     as bad as to make the games unplayable and untestable. Drops are caused by the
 *     scheduling mechanism meanwhile games on main thread execute GLFW functions immediately.
 *     Should be fine for testing purposes, and for production you can always just put it on
 *     main thread and enable passthrough mode (same as directly running glfw functions).
 * 
 * ======================================================================================
\*/
#pragma once

namespace Leszek::GLFWM {

	// -------------------------------------------------------------------------
	// Additional GLFWM-specific functions for you to use!
	// (they start with glfwm_, for example: glfwm_Function() instead of glfwmFunction())
	// -------------------------------------------------------------------------

	typedef void (*GLFWMneweventfun)();

	// Set a callback that gets called everytime there's new event waiting for main thread
	// Useful if alongside glfwm, you also want other things running on the main thread
	// (for example a custom task scheduler). In that case this function allows you to
	// manually control how main thread behaves inbetween tasks.
	void glfwm_setNewEventCallback(GLFWMneweventfun callback);

	// It's the default callback for new events.
	void glfwm_defaultNewEventCallback();

	// Runs all the tasks in the queue
	void glfwm_executeQueueTasks();

	// Default function that waits for the new tasks to arrive
	void glfwm_defaultWaitForTasks(int max_ms_wait_time = 250);

	// Example usage of all the functions above:
	// 
	// std::thread gameThread = std::thread([&]() { game.run(); });
	// while(appRunning) {
	//     glfwm_defaultWaitForTasks();
	//     glfwm_executeQueueTasks();
	// }

	// -------------------------------------------------------------------------
	// All GLFWM replacements for GLFW!
	// The functions below are tagged with:
	// 
	// Unsupported    - Function is not yet implemented. Still fine to use with LESZEK_GLFWM_PASSTHROUGH_MODE
	// Any-Thread     - Function can be called from any thread.
	// Window-Thread  - Function should be called in the thread where the window was created. 
	//                  You can still call it from any thread, but it's tied one way or another to the thread the window operates on.
	// OpenGL-Context - A thread that currently owns OpenGL context.
	// 
	// Every function (other than glfwmTerminate) that doesn't return anything
	// works as a fire and forget, meaning that it doesn't wait for the main thread
	// to finish executing a task before returning. Some other functions can return
	// cached data instead of calling underlying glfw functions (if possible).
	// 
	// Info: When creating the window, it's "binded" to the current thread.
	// All the callbacks will execute through glfwmPollEvents() and then only the
	// callbacks to the windows on the calling thread will be executed.
	// Meaning that if you create window on thread #1, you need to poll it's events
	// on the same thread!
	// -------------------------------------------------------------------------

	/* Window-Thread */ int glfwmInit(void);
	/* Window-Thread */ void glfwmTerminate(void);
	/* Window-Thread */	void glfwmPollEvents(void); // All callbacks, events, errors etc. are parsed and executed here

	/* Any-Thread */	void glfwmInitHint(int hint, int value); // Not fully supported, works as intended only when each thread has the same hints
		/* !!! Unsupported !!! */ void glfwmInitAllocator(const GLFWallocator* allocator);
	/* Any-Thread */    void glfwmGetVersion(int* major, int* minor, int* rev);
	/* Any-Thread */    const char* glfwmGetVersionString(void);
	/* Any-Thread */    int glfwmGetError(const char** description);
	/* Window-Thread */ GLFWerrorfun glfwmSetErrorCallback(GLFWerrorfun callback);
	/* Any-Thread */    int glfwmGetPlatform(void);
	/* Any-Thread */    int glfwmPlatformSupported(int platform);
	/* Any-Thread */    GLFWmonitor** glfwmGetMonitors(int* count);
	/* Any-Thread */    GLFWmonitor* glfwmGetPrimaryMonitor(void);
	/* Any-Thread */	void glfwmGetMonitorPos(GLFWmonitor* monitor, int* xpos, int* ypos);
	/* Any-Thread */	void glfwmGetMonitorWorkarea(GLFWmonitor* monitor, int* xpos, int* ypos, int* width, int* height);
	/* Any-Thread */	void glfwmGetMonitorPhysicalSize(GLFWmonitor* monitor, int* widthMM, int* heightMM);
	/* Any-Thread */	void glfwmGetMonitorContentScale(GLFWmonitor* monitor, float* xscale, float* yscale);
	/* Any-Thread */	const char* glfwmGetMonitorName(GLFWmonitor* monitor);
	/* Any-Thread */	void glfwmSetMonitorUserPointer(GLFWmonitor* monitor, void* pointer);
	/* Any-Thread */	void* glfwmGetMonitorUserPointer(GLFWmonitor* monitor);
	/* Window-Thread */ GLFWmonitorfun glfwmSetMonitorCallback(GLFWmonitorfun callback);
		/* !!! Unsupported !!! */ const GLFWvidmode* glfwmGetVideoModes(GLFWmonitor* monitor, int* count);
	/* Any-Thread */	const GLFWvidmode* glfwmGetVideoMode(GLFWmonitor* monitor);
	/* Any-Thread */	void glfwmSetGamma(GLFWmonitor* monitor, float gamma);
		/* !!! Unsupported !!! */ const GLFWgammaramp* glfwmGetGammaRamp(GLFWmonitor* monitor);
		/* !!! Unsupported !!! */ void glfwmSetGammaRamp(GLFWmonitor* monitor, const GLFWgammaramp* ramp);
	/* Window-Thread */ void glfwmDefaultWindowHints(void);
	/* Window-Thread */ void glfwmWindowHint(int hint, int value);
	/* Window-Thread */ void glfwmWindowHintString(int hint, const char* value);
	/* Window-Thread */ GLFWwindow* glfwmCreateWindow(int width, int height, const char* title, GLFWmonitor* monitor, GLFWwindow* share);
	/* Any-Thread */    void glfwmDestroyWindow(GLFWwindow* window);
	/* Any-Thread */    int glfwmWindowShouldClose(GLFWwindow* window);
	/* Any-Thread */	void glfwmSetWindowShouldClose(GLFWwindow* window, int value);
	/* Window-Thread */	const char* glfwmGetWindowTitle(GLFWwindow* window);
	/* Any-Thread */	void glfwmSetWindowTitle(GLFWwindow* window, const char* title);
	/* Any-Thread */	void glfwmSetWindowIcon(GLFWwindow* window, int count, const GLFWimage* images);
	/* Any-Thread */	void glfwmGetWindowPos(GLFWwindow* window, int* xpos, int* ypos);
	/* Any-Thread */	void glfwmSetWindowPos(GLFWwindow* window, int xpos, int ypos);
	/* Any-Thread */	void glfwmGetWindowSize(GLFWwindow* window, int* width, int* height);
	/* Any-Thread */	void glfwmSetWindowSizeLimits(GLFWwindow* window, int minwidth, int minheight, int maxwidth, int maxheight);
	/* Any-Thread */	void glfwmSetWindowAspectRatio(GLFWwindow* window, int numer, int denom);
	/* Any-Thread */	void glfwmSetWindowSize(GLFWwindow* window, int width, int height);
	/* Any-Thread */	void glfwmGetFramebufferSize(GLFWwindow* window, int* width, int* height);
	/* Any-Thread */	void glfwmGetWindowFrameSize(GLFWwindow* window, int* left, int* top, int* right, int* bottom);
	/* Any-Thread */	void glfwmGetWindowContentScale(GLFWwindow* window, float* xscale, float* yscale);
	/* Any-Thread */	float glfwmGetWindowOpacity(GLFWwindow* window);
	/* Any-Thread */	void glfwmSetWindowOpacity(GLFWwindow* window, float opacity);
	/* Any-Thread */	void glfwmIconifyWindow(GLFWwindow* window);
	/* Any-Thread */	void glfwmRestoreWindow(GLFWwindow* window);
	/* Any-Thread */	void glfwmMaximizeWindow(GLFWwindow* window);
	/* Any-Thread */	void glfwmShowWindow(GLFWwindow* window);
	/* Any-Thread */	void glfwmHideWindow(GLFWwindow* window);
	/* Any-Thread */	void glfwmFocusWindow(GLFWwindow* window);
	/* Any-Thread */	void glfwmRequestWindowAttention(GLFWwindow* window);
	/* Any-Thread */	GLFWmonitor* glfwmGetWindowMonitor(GLFWwindow* window);
	/* Any-Thread */	void glfwmSetWindowMonitor(GLFWwindow* window, GLFWmonitor* monitor, int xpos, int ypos, int width, int height, int refreshRate);
	/* Any-Thread */	int glfwmGetWindowAttrib(GLFWwindow* window, int attrib);
	/* Any-Thread */	void glfwmSetWindowAttrib(GLFWwindow* window, int attrib, int value);
	/* Any-Thread */	void glfwmSetWindowUserPointer(GLFWwindow* window, void* pointer);
	/* Any-Thread */	void* glfwmGetWindowUserPointer(GLFWwindow* window);
	/* Any-Thread */	GLFWwindowposfun glfwmSetWindowPosCallback(GLFWwindow* window, GLFWwindowposfun callback);
	/* Any-Thread */	GLFWwindowsizefun glfwmSetWindowSizeCallback(GLFWwindow* window, GLFWwindowsizefun callback);
	/* Any-Thread */	GLFWwindowclosefun glfwmSetWindowCloseCallback(GLFWwindow* window, GLFWwindowclosefun callback);
	/* Any-Thread */	GLFWwindowrefreshfun glfwmSetWindowRefreshCallback(GLFWwindow* window, GLFWwindowrefreshfun callback);
	/* Any-Thread */	GLFWwindowfocusfun glfwmSetWindowFocusCallback(GLFWwindow* window, GLFWwindowfocusfun callback);
	/* Any-Thread */	GLFWwindowiconifyfun glfwmSetWindowIconifyCallback(GLFWwindow* window, GLFWwindowiconifyfun callback);
	/* Any-Thread */	GLFWwindowmaximizefun glfwmSetWindowMaximizeCallback(GLFWwindow* window, GLFWwindowmaximizefun callback);
	/* Any-Thread */	GLFWframebuffersizefun glfwmSetFramebufferSizeCallback(GLFWwindow* window, GLFWframebuffersizefun callback);
	/* Any-Thread */	GLFWwindowcontentscalefun glfwmSetWindowContentScaleCallback(GLFWwindow* window, GLFWwindowcontentscalefun callback);
		/* !!! Unsupported !!! */ void glfwmWaitEvents(void);
		/* !!! Unsupported !!! */ void glfwmWaitEventsTimeout(double timeout);
		/* !!! Unsupported !!! */ void glfwmPostEmptyEvent(void);
	/* Any-Thread */	int glfwmGetInputMode(GLFWwindow* window, int mode);
	/* Any-Thread */	void glfwmSetInputMode(GLFWwindow* window, int mode, int value);
	/* Any-Thread */	int glfwmRawMouseMotionSupported(void);
	/* Any-Thread */	const char* glfwmGetKeyName(int key, int scancode);
	/* Any-Thread */	int glfwmGetKeyScancode(int key);
	/* Any-Thread */	int glfwmGetKey(GLFWwindow* window, int key);
	/* Any-Thread */	int glfwmGetMouseButton(GLFWwindow* window, int button);
	/* Any-Thread */	void glfwmGetCursorPos(GLFWwindow* window, double* xpos, double* ypos);
	/* Any-Thread */	void glfwmSetCursorPos(GLFWwindow* window, double xpos, double ypos);
	/* Any-Thread */	GLFWcursor* glfwmCreateCursor(const GLFWimage* image, int xhot, int yhot);
	/* Any-Thread */	GLFWcursor* glfwmCreateStandardCursor(int shape);
	/* Any-Thread */	void glfwmDestroyCursor(GLFWcursor* cursor);
	/* Any-Thread */	void glfwmSetCursor(GLFWwindow* window, GLFWcursor* cursor);
	/* Any-Thread */	GLFWkeyfun glfwmSetKeyCallback(GLFWwindow* window, GLFWkeyfun callback);
	/* Any-Thread */	GLFWcharfun glfwmSetCharCallback(GLFWwindow* window, GLFWcharfun callback);
	/* Any-Thread */	GLFWcharmodsfun glfwmSetCharModsCallback(GLFWwindow* window, GLFWcharmodsfun callback);
	/* Any-Thread */	GLFWmousebuttonfun glfwmSetMouseButtonCallback(GLFWwindow* window, GLFWmousebuttonfun callback);
	/* Any-Thread */	GLFWcursorposfun glfwmSetCursorPosCallback(GLFWwindow* window, GLFWcursorposfun callback);
	/* Any-Thread */	GLFWcursorenterfun glfwmSetCursorEnterCallback(GLFWwindow* window, GLFWcursorenterfun callback);
	/* Any-Thread */	GLFWscrollfun glfwmSetScrollCallback(GLFWwindow* window, GLFWscrollfun callback);
	/* Any-Thread */	GLFWdropfun glfwmSetDropCallback(GLFWwindow* window, GLFWdropfun callback);
	/* Any-Thread */	int glfwmJoystickPresent(int jid);
		/* !!! Unsupported !!! */ const float* glfwmGetJoystickAxes(int jid, int* count);
		/* !!! Unsupported !!! */ const unsigned char* glfwmGetJoystickButtons(int jid, int* count);
		/* !!! Unsupported !!! */ const unsigned char* glfwmGetJoystickHats(int jid, int* count);
		/* !!! Unsupported !!! */ const char* glfwmGetJoystickName(int jid);
		/* !!! Unsupported !!! */ const char* glfwmGetJoystickGUID(int jid);
		/* !!! Unsupported !!! */ void glfwmSetJoystickUserPointer(int jid, void* pointer);
		/* !!! Unsupported !!! */ void* glfwmGetJoystickUserPointer(int jid);
	/* Any-Thread */	int glfwmJoystickIsGamepad(int jid);
		/* !!! Unsupported !!! */ GLFWjoystickfun glfwmSetJoystickCallback(GLFWjoystickfun callback);
		/* !!! Unsupported !!! */ int glfwmUpdateGamepadMappings(const char* string);
		/* !!! Unsupported !!! */ const char* glfwmGetGamepadName(int jid);
	/* Any-Thread */	int glfwmGetGamepadState(int jid, GLFWgamepadstate* state);
	/* Any-Thread */	void glfwmSetClipboardString(GLFWwindow* window, const char* string);
	/* Window-Thread */	const char* glfwmGetClipboardString(GLFWwindow* window);
	/* Any-Thread */	double glfwmGetTime(void);
	/* Any-Thread */	void glfwmSetTime(double time);
	/* Any-Thread */	uint64_t glfwmGetTimerValue(void);
	/* Any-Thread */	uint64_t glfwmGetTimerFrequency(void);
	/* OpenGL-Context */ void glfwmMakeContextCurrent(GLFWwindow* window);
	/* OpenGL-Context */ GLFWwindow* glfwmGetCurrentContext(void);
	/* OpenGL-Context */ void glfwmSwapBuffers(GLFWwindow* window);
	/* OpenGL-Context */ void glfwmSwapInterval(int interval);
	/* OpenGL-Context */ int glfwmExtensionSupported(const char* extension);
	/* OpenGL-Context */ GLFWglproc glfwmGetProcAddress(const char* procname);
		/* !!! Unsupported !!! */ int glfwmVulkanSupported(void);
		/* !!! Unsupported !!! */ const char** glfwmGetRequiredInstanceExtensions(uint32_t* count);
#if defined(VK_VERSION_1_0) || defined(GLFW_INCLUDE_VULKAN)
		/* !!! Unsupported !!! */ GLFWvkproc glfwmGetInstanceProcAddress(VkInstance instance, const char* procname);
		/* !!! Unsupported !!! */ int glfwmGetPhysicalDevicePresentationSupport(VkInstance instance, VkPhysicalDevice device, uint32_t queuefamily);
		/* !!! Unsupported !!! */ VkResult glfwmCreateWindowSurface(VkInstance instance, GLFWwindow* window, const VkAllocationCallbacks* allocator, VkSurfaceKHR* surface);
#endif
}

#ifdef LESZEK_GLFWM_IMPLEMENTATION
#ifndef LESZEK_GLFWM_PASSTHROUGH_MODE

/*\
 * Information about Maintaining the codebase:
 * 
 * # Mutex Ordering:
 * To prevent deadlocks, mutexes are suffixed with
 * _lX with X being the mutex level. Mutexes should be
 * locked ONLY in the increasing order, not the other
 * way around. Example:
 * mutex_l1.lock() -> mutex_l3.lock() // Good!
 * mutex_l4.lock() -> mutex_l3.lock() // Bad!
 * 
 * Current Levels:
 * [1] ctx->m_init_l1, ctx->m_monitorCallbackRegistered_l1
 * [2] ctx->m_threadData_l2
 * [3] ctx->m_windowData_l3
 * [4] ctx->m_lastError_l4
 * [5] ctx->m_keyNameCacheTable_l5, 
 *     ctx->m_keyNameCacheTable_unknown_l5,
 *	   ctx->m_keyScanCodeTable_l5,
 *     GLFWMWindowData::m_keyStates_l5,
 * 
\*/

#include <stdexcept>
#include <thread>
#include <future>
#include <unordered_set>
#include <atomic>
#include <array>
#include <optional>

namespace Leszek::GLFWM {

// A placeholder macro for functions that are not yet implemented
#define _LESZEK_GLFWM_UNSUPPORTED_FUNCTION(X) throw std::runtime_error("GLFWM: Unsupported function " X)

	// =========================================================================
	// Private (internal implementation)
	// =========================================================================

#ifdef LESZEK_GLFWM_ALWAYS_WAIT
	template<typename T>
	using GLFWMFuture = std::shared_future<T>;
#else
	template<typename T>
	using GLFWMFuture = std::future<T>;
#endif

	namespace Private {

		template<typename Lock>
		struct unlock_guard {
			Lock& lock;
			unlock_guard(Lock& l) : lock(l) { lock.unlock(); }
			~unlock_guard() { lock.lock(); }
		};

		// -------------------------------------------------------------------------
		// Forward declarations
		// -------------------------------------------------------------------------

		class GLFWMContext;
		struct GLFWMEvent_Init;
		struct GLFWMEvent_Terminate;

		// -------------------------------------------------------------------------
		// Globals
		// -------------------------------------------------------------------------

		const std::thread::id g_mainThreadId = std::this_thread::get_id();
		std::atomic<std::thread::id> g_currentlyProcessedEvent_sourceThread = {};

		// It's thread_local instead of being tied to GLFWMThreadData, since the users
		// due to GLFW documentation are encouraged to register error callbacks
		// BEFORE initialization of GLFW.
		thread_local GLFWerrorfun g_errorCallback = nullptr;

		// Some variables for Default versions of event handler.
		// Those are NOT integral parts of GLFWM! Those are things
		// that are even sometimes just skipped due to users setting
		// up their own new event callbacks.
		std::mutex g_wakeMutex;
		std::condition_variable g_wakeCV;
		bool g_signalPending = false; // requires locking g_wakeMutex

		bool isMainThread() {
			static thread_local const bool isMainThread = g_mainThreadId == std::this_thread::get_id();
			return isMainThread;
		}

		// -------------------------------------------------------------------------
		// Window hints
		// -------------------------------------------------------------------------

		// This struct contains ALL of the glfw window hints
		// that can be set with glfwWindowHint(hint, value).
		// It's here, since each thread needs to keep 
		// it's own version of currently set hints.
		struct GLFWMWindowHints {
			// Info: Inside GLFW, GLFW_SCALE_FRAMEBUFFER and
			// GLFW_COCOA_RETINA_FRAMEBUFFER are treated the same!

			int redBits = 8, greenBits = 8, blueBits = 8, alphaBits = 8;
			int depthBits = 24, stencilBits = 8;
			int accumRedBits = 0, accumGreenBits = 0, accumBlueBits = 0, accumAlphaBits = 0;
			int auxBuffers = 0, stereo = 0, doublebuffer = GLFW_TRUE, transparent = 0;
			int samples = 0, sRGB = 0, refreshRate = GLFW_DONT_CARE;
			int resizable = GLFW_TRUE, decorated = GLFW_TRUE, focused = GLFW_TRUE;
			int autoIconify = GLFW_TRUE, floating = 0, maximized = 0, visible = GLFW_TRUE;
			int xpos = static_cast<int>(GLFW_ANY_POSITION), ypos = static_cast<int>(GLFW_ANY_POSITION);
			int keymenu = 0, showDefault = 0, cocoaGraphicsSwitching = 0;
			int scaleToMonitor = 0, scaleFramebuffer = GLFW_TRUE;
			int centerCursor = GLFW_TRUE, focusOnShow = GLFW_TRUE, mousePassthrough = 0;
			int client = GLFW_OPENGL_API, source = GLFW_NATIVE_CONTEXT_API;
			int major = 1, minor = 0, robustness = 0;
			int forward = GLFW_FALSE, debug = 0, noerror = 0, profile = 0, release = 0;

			std::string frameName = "", className = "", instanceName = "", appId = "";

			void setHint(int hint, int value) {
				switch (hint) {
				case GLFW_RED_BITS:                 redBits = value; return;
				case GLFW_GREEN_BITS:               greenBits = value; return;
				case GLFW_BLUE_BITS:                blueBits = value; return;
				case GLFW_ALPHA_BITS:               alphaBits = value; return;
				case GLFW_DEPTH_BITS:               depthBits = value; return;
				case GLFW_STENCIL_BITS:             stencilBits = value; return;
				case GLFW_ACCUM_RED_BITS:           accumRedBits = value; return;
				case GLFW_ACCUM_GREEN_BITS:         accumGreenBits = value; return;
				case GLFW_ACCUM_BLUE_BITS:          accumBlueBits = value; return;
				case GLFW_ACCUM_ALPHA_BITS:         accumAlphaBits = value; return;
				case GLFW_AUX_BUFFERS:              auxBuffers = value; return;
				case GLFW_SAMPLES:                  samples = value; return;
				case GLFW_POSITION_X:               xpos = value; return;
				case GLFW_POSITION_Y:               ypos = value; return;
				case GLFW_CLIENT_API:               client = value; return;
				case GLFW_CONTEXT_CREATION_API:     source = value; return;
				case GLFW_CONTEXT_VERSION_MAJOR:    major = value; return;
				case GLFW_CONTEXT_VERSION_MINOR:    minor = value; return;
				case GLFW_CONTEXT_ROBUSTNESS:       robustness = value; return;
				case GLFW_OPENGL_PROFILE:           profile = value; return;
				case GLFW_CONTEXT_RELEASE_BEHAVIOR: release = value; return;
				case GLFW_REFRESH_RATE:             refreshRate = value; return;
				case GLFW_STEREO:                   stereo = value ? GLFW_TRUE : GLFW_FALSE; return;
				case GLFW_DOUBLEBUFFER:             doublebuffer = value ? GLFW_TRUE : GLFW_FALSE; return;
				case GLFW_TRANSPARENT_FRAMEBUFFER:  transparent = value ? GLFW_TRUE : GLFW_FALSE; return;
				case GLFW_SRGB_CAPABLE:             sRGB = value ? GLFW_TRUE : GLFW_FALSE; return;
				case GLFW_RESIZABLE:                resizable = value ? GLFW_TRUE : GLFW_FALSE; return;
				case GLFW_DECORATED:                decorated = value ? GLFW_TRUE : GLFW_FALSE; return;
				case GLFW_FOCUSED:                  focused = value ? GLFW_TRUE : GLFW_FALSE; return;
				case GLFW_AUTO_ICONIFY:             autoIconify = value ? GLFW_TRUE : GLFW_FALSE; return;
				case GLFW_FLOATING:                 floating = value ? GLFW_TRUE : GLFW_FALSE; return;
				case GLFW_MAXIMIZED:                maximized = value ? GLFW_TRUE : GLFW_FALSE; return;
				case GLFW_VISIBLE:                  visible = value ? GLFW_TRUE : GLFW_FALSE; return;
				case GLFW_WIN32_KEYBOARD_MENU:      keymenu = value ? GLFW_TRUE : GLFW_FALSE; return;
				case GLFW_WIN32_SHOWDEFAULT:        showDefault = value ? GLFW_TRUE : GLFW_FALSE; return;
				case GLFW_COCOA_GRAPHICS_SWITCHING: cocoaGraphicsSwitching = value ? GLFW_TRUE : GLFW_FALSE; return;
				case GLFW_SCALE_TO_MONITOR:         scaleToMonitor = value ? GLFW_TRUE : GLFW_FALSE; return;
				case GLFW_SCALE_FRAMEBUFFER:
				case GLFW_COCOA_RETINA_FRAMEBUFFER: scaleFramebuffer = value ? GLFW_TRUE : GLFW_FALSE; return;
				case GLFW_CENTER_CURSOR:            centerCursor = value ? GLFW_TRUE : GLFW_FALSE; return;
				case GLFW_FOCUS_ON_SHOW:            focusOnShow = value ? GLFW_TRUE : GLFW_FALSE; return;
				case GLFW_MOUSE_PASSTHROUGH:        mousePassthrough = value ? GLFW_TRUE : GLFW_FALSE; return;
				case GLFW_OPENGL_FORWARD_COMPAT:    forward = value ? GLFW_TRUE : GLFW_FALSE; return;
				case GLFW_CONTEXT_DEBUG:            debug = value ? GLFW_TRUE : GLFW_FALSE; return;
				case GLFW_CONTEXT_NO_ERROR:         noerror = value ? GLFW_TRUE : GLFW_FALSE; return;
				}
			}
			void setHint(int hint, const char* value) {
				switch (hint) {
					case GLFW_COCOA_FRAME_NAME:		frameName	 = std::string(value);
					case GLFW_X11_CLASS_NAME:		className	 = std::string(value);
					case GLFW_X11_INSTANCE_NAME:	instanceName = std::string(value);
					case GLFW_WAYLAND_APP_ID:		appId		 = std::string(value);
				}
			}

			int getHint(int hint) {
				switch (hint) {
				case GLFW_RED_BITS:                 return redBits;
				case GLFW_GREEN_BITS:               return greenBits;
				case GLFW_BLUE_BITS:				return blueBits;
				case GLFW_ALPHA_BITS:               return alphaBits;
				case GLFW_DEPTH_BITS:               return depthBits;
				case GLFW_STENCIL_BITS:             return stencilBits;
				case GLFW_ACCUM_RED_BITS:           return accumRedBits;
				case GLFW_ACCUM_GREEN_BITS:         return accumGreenBits;
				case GLFW_ACCUM_BLUE_BITS:          return accumBlueBits;
				case GLFW_ACCUM_ALPHA_BITS:         return accumAlphaBits;
				case GLFW_AUX_BUFFERS:              return auxBuffers;
				case GLFW_STEREO:                   return stereo;
				case GLFW_DOUBLEBUFFER:             return doublebuffer;
				case GLFW_TRANSPARENT_FRAMEBUFFER:  return transparent;
				case GLFW_SAMPLES:                  return samples;
				case GLFW_SRGB_CAPABLE:             return sRGB;
				case GLFW_RESIZABLE:                return resizable;
				case GLFW_DECORATED:                return decorated;
				case GLFW_FOCUSED:                  return focused;
				case GLFW_AUTO_ICONIFY:             return autoIconify;
				case GLFW_FLOATING:                 return floating;
				case GLFW_MAXIMIZED:                return maximized;
				case GLFW_VISIBLE:                  return visible;
				case GLFW_POSITION_X:               return xpos;
				case GLFW_POSITION_Y:               return ypos;
				case GLFW_WIN32_KEYBOARD_MENU:      return keymenu;
				case GLFW_WIN32_SHOWDEFAULT:        return showDefault;
				case GLFW_COCOA_GRAPHICS_SWITCHING: return cocoaGraphicsSwitching;
				case GLFW_SCALE_TO_MONITOR:         return scaleToMonitor;
				case GLFW_SCALE_FRAMEBUFFER:
				case GLFW_COCOA_RETINA_FRAMEBUFFER: return scaleFramebuffer;
				case GLFW_CENTER_CURSOR:            return centerCursor;
				case GLFW_FOCUS_ON_SHOW:            return focusOnShow;
				case GLFW_MOUSE_PASSTHROUGH:        return mousePassthrough;
				case GLFW_CLIENT_API:               return client;
				case GLFW_CONTEXT_CREATION_API:     return source;
				case GLFW_CONTEXT_VERSION_MAJOR:    return major;
				case GLFW_CONTEXT_VERSION_MINOR:    return minor;
				case GLFW_CONTEXT_ROBUSTNESS:       return robustness;
				case GLFW_OPENGL_FORWARD_COMPAT:    return forward;
				case GLFW_CONTEXT_DEBUG:            return debug;
				case GLFW_CONTEXT_NO_ERROR:         return noerror;
				case GLFW_OPENGL_PROFILE:           return profile;
				case GLFW_CONTEXT_RELEASE_BEHAVIOR: return release;
				case GLFW_REFRESH_RATE:             return refreshRate;
				}
				return 0;
			}

			void resetToDefault() { *this = GLFWMWindowHints{}; }

			void executeGLFWWindowHint() {
				glfwWindowHint(GLFW_RED_BITS, redBits);
				glfwWindowHint(GLFW_GREEN_BITS, greenBits);
				glfwWindowHint(GLFW_BLUE_BITS, blueBits);
				glfwWindowHint(GLFW_ALPHA_BITS, alphaBits);
				glfwWindowHint(GLFW_DEPTH_BITS, depthBits);
				glfwWindowHint(GLFW_STENCIL_BITS, stencilBits);
				glfwWindowHint(GLFW_ACCUM_RED_BITS, accumRedBits);
				glfwWindowHint(GLFW_ACCUM_GREEN_BITS, accumGreenBits);
				glfwWindowHint(GLFW_ACCUM_BLUE_BITS, accumBlueBits);
				glfwWindowHint(GLFW_ACCUM_ALPHA_BITS, accumAlphaBits);
				glfwWindowHint(GLFW_AUX_BUFFERS, auxBuffers);           
				glfwWindowHint(GLFW_STEREO, stereo);
				glfwWindowHint(GLFW_DOUBLEBUFFER, doublebuffer);        
				glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, transparent);
				glfwWindowHint(GLFW_SAMPLES, samples);                  
				glfwWindowHint(GLFW_SRGB_CAPABLE, sRGB);
				glfwWindowHint(GLFW_RESIZABLE, resizable);              
				glfwWindowHint(GLFW_DECORATED, decorated);
				glfwWindowHint(GLFW_FOCUSED, focused);                  
				glfwWindowHint(GLFW_AUTO_ICONIFY, autoIconify);
				glfwWindowHint(GLFW_FLOATING, floating);                
				glfwWindowHint(GLFW_MAXIMIZED, maximized);
				glfwWindowHint(GLFW_VISIBLE, visible);                  
				glfwWindowHint(GLFW_POSITION_X, xpos);
				glfwWindowHint(GLFW_POSITION_Y, ypos);                  
				glfwWindowHint(GLFW_WIN32_KEYBOARD_MENU, keymenu);
				glfwWindowHint(GLFW_WIN32_SHOWDEFAULT, showDefault);    
				glfwWindowHint(GLFW_COCOA_GRAPHICS_SWITCHING, cocoaGraphicsSwitching);
				glfwWindowHint(GLFW_SCALE_TO_MONITOR, scaleToMonitor);  
				glfwWindowHint(GLFW_SCALE_FRAMEBUFFER, scaleFramebuffer);
				//glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, scaleFramebuffer);
				glfwWindowHint(GLFW_CENTER_CURSOR, centerCursor);       
				glfwWindowHint(GLFW_FOCUS_ON_SHOW, focusOnShow);
				glfwWindowHint(GLFW_MOUSE_PASSTHROUGH, mousePassthrough);
				glfwWindowHint(GLFW_CLIENT_API, client);                
				glfwWindowHint(GLFW_CONTEXT_CREATION_API, source);
				glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, major);      
				glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, minor);
				glfwWindowHint(GLFW_CONTEXT_ROBUSTNESS, robustness);    
				glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, forward);
				glfwWindowHint(GLFW_CONTEXT_DEBUG, debug);              
				glfwWindowHint(GLFW_CONTEXT_NO_ERROR, noerror);
				glfwWindowHint(GLFW_OPENGL_PROFILE, profile);           
				glfwWindowHint(GLFW_CONTEXT_RELEASE_BEHAVIOR, release);
				glfwWindowHint(GLFW_REFRESH_RATE, refreshRate);

				glfwWindowHintString(GLFW_COCOA_FRAME_NAME, frameName.c_str());
				glfwWindowHintString(GLFW_X11_CLASS_NAME, className.c_str());
				glfwWindowHintString(GLFW_X11_INSTANCE_NAME, instanceName.c_str());
				glfwWindowHintString(GLFW_WAYLAND_APP_ID, appId.c_str());
			}
		};

		struct GLFWMInitHints {
			std::atomic<int> hatButtons = GLFW_TRUE,
							 angleType = GLFW_ANGLE_PLATFORM_TYPE_NONE,
							 platformID = GLFW_ANY_PLATFORM,
							 ns_chdir = GLFW_TRUE,
							 ns_menubar = GLFW_TRUE,
							 x11_xcbVulkanSurface = GLFW_TRUE,
							 wl_libdecorMode = GLFW_WAYLAND_PREFER_LIBDECOR;

			GLFWMInitHints() {}
			GLFWMInitHints(const GLFWMInitHints& other) :
				hatButtons(other.hatButtons.load()),
				angleType(other.angleType.load()),
				platformID(other.platformID.load()),
				ns_chdir(other.ns_chdir.load()),
				ns_menubar(other.ns_menubar.load()),
				x11_xcbVulkanSurface(other.x11_xcbVulkanSurface.load()),
				wl_libdecorMode(other.wl_libdecorMode.load())
			{}
			GLFWMInitHints& operator=(const GLFWMInitHints& other) {
				hatButtons = other.hatButtons.load();
				angleType = other.angleType.load();
				platformID = other.platformID.load();
				ns_chdir = other.ns_chdir.load();
				ns_menubar = other.ns_menubar.load();
				x11_xcbVulkanSurface = other.x11_xcbVulkanSurface.load();
				wl_libdecorMode = other.wl_libdecorMode.load();
				return *this;
			}

			void resetToDefault() {
				hatButtons = GLFW_TRUE;
				angleType = GLFW_ANGLE_PLATFORM_TYPE_NONE;
				platformID = GLFW_ANY_PLATFORM;
				ns_chdir = GLFW_TRUE;
				ns_menubar = GLFW_TRUE;
				x11_xcbVulkanSurface = GLFW_TRUE;
				wl_libdecorMode = GLFW_WAYLAND_PREFER_LIBDECOR;
			}

			void setHint(int hint, int value) {
				switch (hint) {
					case GLFW_JOYSTICK_HAT_BUTTONS:		hatButtons = value; return;
					case GLFW_ANGLE_PLATFORM_TYPE:		angleType = value; return;
					case GLFW_PLATFORM:					platformID = value; return;
					case GLFW_COCOA_CHDIR_RESOURCES:	ns_chdir = value; return;
					case GLFW_COCOA_MENUBAR:			ns_menubar = value; return;
					case GLFW_X11_XCB_VULKAN_SURFACE:	x11_xcbVulkanSurface = value; return;
					case GLFW_WAYLAND_LIBDECOR:			wl_libdecorMode = value; return;
				}
			}
			int getHint(int hint) {
				switch (hint) {
					case GLFW_JOYSTICK_HAT_BUTTONS:		return hatButtons;
					case GLFW_ANGLE_PLATFORM_TYPE:		return angleType;
					case GLFW_PLATFORM:					return platformID;
					case GLFW_COCOA_CHDIR_RESOURCES:	return ns_chdir;
					case GLFW_COCOA_MENUBAR:			return ns_menubar;
					case GLFW_X11_XCB_VULKAN_SURFACE:	return x11_xcbVulkanSurface;
					case GLFW_WAYLAND_LIBDECOR:			return wl_libdecorMode;
				}
				return 0;
			}

			void executeGLFWInitHint() {

				glfwInitHint(GLFW_JOYSTICK_HAT_BUTTONS,		hatButtons);
				glfwInitHint(GLFW_ANGLE_PLATFORM_TYPE,		angleType);
				glfwInitHint(GLFW_PLATFORM,					platformID);
				glfwInitHint(GLFW_COCOA_CHDIR_RESOURCES,	ns_chdir);
				glfwInitHint(GLFW_COCOA_MENUBAR,			ns_menubar);
				glfwInitHint(GLFW_X11_XCB_VULKAN_SURFACE,	x11_xcbVulkanSurface);
				glfwInitHint(GLFW_WAYLAND_LIBDECOR,			wl_libdecorMode);

			}
		};

		// -------------------------------------------------------------------------
		// Window callbacks and data
		// -------------------------------------------------------------------------

		struct GLFWMWindowCallbacks {
			GLFWwindowposfun			windowposfun = nullptr;
			GLFWwindowsizefun			windowsizefun = nullptr;
			GLFWwindowclosefun			windowclosefun = nullptr;
			GLFWwindowrefreshfun		windowrefreshfun = nullptr;
			GLFWwindowfocusfun			windowfocusfun = nullptr;
			GLFWwindowiconifyfun		windowiconifyfun = nullptr;
			GLFWwindowmaximizefun		windowmaximizefun = nullptr;
			GLFWframebuffersizefun		framebuffersizefun = nullptr;
			GLFWwindowcontentscalefun	windowcontentscalefun = nullptr;
			GLFWkeyfun					keyfun = nullptr;
			GLFWcharfun					charfun = nullptr;
			GLFWcharmodsfun				charmodsfun = nullptr;
			GLFWmousebuttonfun			mousebuttonfun = nullptr;
			GLFWcursorposfun			cursorposfun = nullptr;
			GLFWcursorenterfun			cursorenterfun = nullptr;
			GLFWscrollfun				scrollfun = nullptr;

			// Manually handled callbacks
			GLFWdropfun					dropfun = nullptr;
		};

		struct GLFWMWindowEvent {
			GLFWwindow* window = nullptr;
			virtual ~GLFWMWindowEvent() = default;
			virtual void execute(GLFWMWindowCallbacks& wdata) = 0;
		};

		template<auto CallbackMemberPtr, typename... Args>
		struct GLFWMConstructedWindowEvent : GLFWMWindowEvent {
			std::tuple<Args...> args;
			void execute(GLFWMWindowCallbacks& callbacks) override {
				std::apply(callbacks.*CallbackMemberPtr, std::tuple_cat(std::make_tuple(window), args));
			}
		};

		struct GLFWMWindowEvent_Drop : GLFWMWindowEvent {
			std::vector<std::string> paths;
			void execute(GLFWMWindowCallbacks& callbacks) override {
				std::vector<const char*> paths_cstr;
				paths_cstr.resize(paths.size());
				for (int i = 0; i < paths.size(); i++) {
					paths_cstr[i] = paths[i].c_str();
				}

				callbacks.dropfun(window, paths_cstr.size(), paths_cstr.data());
			}
		};

		struct GLFWMWindowData {
			GLFWwindow* window = nullptr;
			std::thread::id ownerThread = {};
			std::vector<std::unique_ptr<GLFWMWindowEvent>> events;

			GLFWMWindowCallbacks callbacks;

			std::mutex m_keyStates_l5;
			char keyStates[GLFW_KEY_LAST + 1];

			GLFWMWindowData() {
				for (int i = 0; i < GLFW_KEY_LAST + 1; i++)
					keyStates[i] = GLFW_RELEASE;
			}

			void onKeyEvent(int key, int scancode, int action, int mods) {
				if (key >= 0 && key <= GLFW_KEY_LAST)
				{
					std::lock_guard<std::mutex> lock(m_keyStates_l5);
					keyStates[key] = (char)action;
				}
			}
			int getKeyState(int key) {
				if (key >= 0 && key <= GLFW_KEY_LAST) {
					std::lock_guard<std::mutex> lock(m_keyStates_l5);
					return keyStates[key];
				}
				return GLFW_RELEASE;
			}

			// Used to hold strings, so the returned const char* is kept valid
			std::string windowTitleStr, clipboardStr;
		};

		// -------------------------------------------------------------------------
		// Error and thread data
		// -------------------------------------------------------------------------

		struct GLFWMError {
			int errorCode = 0;
			std::string description;
		};
		struct GLFWMLastError {
			// heldDescription is updated with description data whenever
			// glfwmGetError is received. It then holds the data until the next glfwmGetError
			// call. The getError function returns const char* and according to the docs, the
			// GLFW API guarantees it to be valid until the next glfwGetError() call.
			// The approach with heldDescription variable mirrors that functionality.

			int errorCode = 0;
			std::string description;
			std::string heldDescription;
		};

		struct GLFWMThreadData {
			int initCount = 0;
			std::thread::id threadID = {};
			std::unordered_set<GLFWwindow*> windows;
			
			std::vector<GLFWMError> errors;

			GLFWmonitorfun monitorCallback = nullptr;
			std::vector<std::pair<GLFWmonitor*, int>> monitorEvents;

			GLFWMWindowHints hints;
		};

		// -------------------------------------------------------------------------
		// Context
		// -------------------------------------------------------------------------

		struct GLFWMEvent {
			std::thread::id callingThread = {};
			virtual ~GLFWMEvent() = default;

			// This method ONLY gets called on the main thread!
			virtual void execute(GLFWMContext* ctx) = 0;
		};

		template<typename Func, typename... Args>
		struct GLFWMEvent_ScheduleTask : GLFWMEvent {
			using Ret = std::invoke_result_t<Func, Args...>;

			Func func;
			std::tuple<Args...> args;

			std::promise<Ret> promise;
			GLFWMFuture<Ret> getFuture() { return promise.get_future(); }

			GLFWMEvent_ScheduleTask(Func _func, Args... _args)
				: func(std::move(_func)), args(std::forward<Args>(_args)...) {
			};

			void execute(GLFWMContext* ctx) override {
				try {
					if constexpr (std::is_void_v<Ret>) {
						std::apply(func, args);
						promise.set_value();
					}
					else {
						promise.set_value(std::apply(func, args));
					}
				}
				catch (...) { promise.set_exception(std::current_exception()); }
			}
		};

		class GLFWMContext {
		private:

			std::mutex m_queuedEvents;
			std::vector<std::unique_ptr<GLFWMEvent>> queuedEvents;

			friend GLFWMEvent_Init;
			friend GLFWMEvent_Terminate;

		public:

			GLFWMneweventfun newEventCallback = glfwm_defaultNewEventCallback;

			std::mutex m_init_l1;
			std::atomic<int> initCount = 0;

			std::mutex m_monitorCallbackRegistered_l1;
			bool monitorCallbackRegistered = false;

			struct {
				int initCode = 0;
				GLFWMInitHints initHints;
			} initData;

			// Schedules the event for main thread to execute. 
			// If the current thread is main thread, then it also executes 
			// all of the events before returning, so no deadlock is possible.
			template<typename TEvent>
			auto enqueue(std::unique_ptr<TEvent> event) -> decltype(event->getFuture()) {
				auto future = event->getFuture();
				{
					std::lock_guard<std::mutex> lock(m_queuedEvents);
					event->callingThread = std::this_thread::get_id();
					queuedEvents.push_back(std::move(event));
				}
				if (newEventCallback != nullptr) newEventCallback();
				if (Private::isMainThread()) Private::GLFWMContext::getContext()->executeEvents();

#ifdef LESZEK_GLFWM_ALWAYS_WAIT
				future.get();
#endif
				return future;
			}

			// Schedules the event for main thread to execute. 
			// If the current thread is main thread, then it also executes 
			// all of the events before returning, so no deadlock is possible.
			template<typename TEvent, typename... Args>
			auto enqueue(Args&&... args) -> decltype(std::declval<TEvent>().getFuture()) {
				std::unique_ptr<TEvent> event = std::make_unique<TEvent>(std::forward<Args>(args)...);
				return enqueue(std::move(event));
			}

			template<typename Func, typename... Args>
			auto enqueueTask(Func&& func, Args&&... args) {
				auto event = std::make_unique<GLFWMEvent_ScheduleTask<std::decay_t<Func>, std::decay_t<Args>...>>(
					std::forward<Func>(func), std::forward<Args>(args)...
				);
				return enqueue(std::move(event));
			}

			static GLFWMContext* getContext() {
				static GLFWMContext context;
				return &context;
			}

			void executeEvents() {
				std::vector<std::unique_ptr<GLFWMEvent>> events;
				{
					std::lock_guard<std::mutex> lock(m_queuedEvents);
					events = std::move(queuedEvents);
					queuedEvents.clear();
				}
				
				for (const std::unique_ptr<GLFWMEvent>& event : events) {
					g_currentlyProcessedEvent_sourceThread = event->callingThread;
					event->execute(this);

#ifdef _DEBUG
					// For debugging
					const char* description;
					int code = glfwGetError(&description);
					if (code != GLFW_NO_ERROR) {
						printf("GLFW Error %d: %s\n", code, description);
					}
#endif
				}
				g_currentlyProcessedEvent_sourceThread.store({});
			}

			std::mutex m_threadData_l2;
			std::unordered_map<std::thread::id, GLFWMThreadData> threadData;

			std::mutex m_windowData_l3;
			std::unordered_map<GLFWwindow*, GLFWMWindowData> windowData;

			std::mutex m_lastError_l4;
			std::unordered_map<std::thread::id, GLFWMLastError> lastError;


			// Info: If user switches the layout during gameplay,
			// this cache can still contain names of the old keyboard layout!
			// Currently GLFW exposes no callbacks for that event, so it's
			// pretty much impossible to both keep cache and handle layout changes.
			std::mutex m_keyNameCacheTable_l5;
			std::array<bool, GLFW_KEY_LAST + 1> keyNameCacheTable_PresentEntry;
			std::array<std::unique_ptr<std::string>, GLFW_KEY_LAST + 1> keyNameCacheTable;
			// For things that can't be cached inside table
			std::mutex m_keyNameCacheTable_unknown_l5;
			std::unordered_map<uint64_t, std::unique_ptr<std::string>> keyNameCacheTable_unknown;

			std::mutex m_keyScanCodeTable_l5;
			std::array<bool, GLFW_KEY_LAST + 1> keyScanCodeTable_PresentEntry;
			std::array<int, GLFW_KEY_LAST + 1> keyScanCodeTable;

		};


		// -------------------------------------------------------------------------
		// GLFWM Events
		// -------------------------------------------------------------------------

		struct GLFWMEvent_Fence : GLFWMEvent {
			std::promise<void> promise;
			GLFWMFuture<void>  getFuture() { return promise.get_future(); }

			void execute(GLFWMContext* ctx) override {
				try {
					promise.set_value();
				}
				catch (...) {
					promise.set_exception(std::current_exception());
				}
			}
		};

		struct GLFWMEvent_Init : GLFWMEvent {
			GLFWMInitHints initHints;

			std::promise<int> promise;
			GLFWMFuture<int>  getFuture() { return promise.get_future(); }

			void execute(GLFWMContext* ctx) override {
				try {
					initHints.executeGLFWInitHint();
					ctx->initData.initCode = glfwInit();
					glfwSetErrorCallback([](int error_code, const char* description) -> void {
						const std::thread::id sourceThread = g_currentlyProcessedEvent_sourceThread;
						if (sourceThread != std::thread::id{}) {
							Private::GLFWMContext* const ctx = Private::GLFWMContext::getContext();

							std::lock_guard<std::mutex> lock(ctx->m_threadData_l2);
							if (ctx->threadData.contains(sourceThread)) {

								ctx->threadData.at(sourceThread).errors
									.push_back({ error_code, std::string(description) });
							}

							std::lock_guard<std::mutex> lock2(ctx->m_lastError_l4);
							ctx->lastError[sourceThread].errorCode = error_code;
							ctx->lastError[sourceThread].description = std::string(description);
						};
					});
					promise.set_value(ctx->initData.initCode);
				}
				catch (...) {
					promise.set_exception(std::current_exception());
				}
			}
		};
		struct GLFWMEvent_Terminate : GLFWMEvent {
			std::promise<void> promise;
			GLFWMFuture<void>  getFuture() { return promise.get_future(); }

			void execute(GLFWMContext* ctx) override {
				try {
					{
						std::lock_guard<std::mutex> lock(ctx->m_threadData_l2);
						ctx->threadData.clear();
					}
					{
						std::lock_guard<std::mutex> lock(ctx->m_windowData_l3);
						ctx->windowData.clear();
					}
					{
						std::lock_guard<std::mutex> lock(ctx->m_lastError_l4);
						ctx->lastError.clear();
					}
					{
						std::lock_guard<std::mutex> lock(ctx->m_keyNameCacheTable_unknown_l5);
						ctx->keyNameCacheTable_unknown.clear();
					}
					{
						std::lock_guard<std::mutex> lock(ctx->m_keyNameCacheTable_l5);
						for (int i = 0; i < ctx->keyNameCacheTable.size(); i++) {
							ctx->keyNameCacheTable[i] = nullptr;
						}
					}

					glfwTerminate();
					promise.set_value();
				}
				catch (...) {
					promise.set_exception(std::current_exception());
				}
			}
		};

		struct GLFWMEvent_CreateWindow : GLFWMEvent {
			int width = 0;
			int height = 0;
			std::string title = {};
			GLFWmonitor* monitor = nullptr;
			GLFWwindow* share = nullptr;

			GLFWMWindowHints hints;

			std::promise<GLFWwindow*> promise;
			GLFWMFuture<GLFWwindow*>  getFuture() { return promise.get_future(); }

			void execute(GLFWMContext* ctx) override {
				try {
					hints.executeGLFWWindowHint();
					GLFWwindow* window = glfwCreateWindow(width, height, title.c_str(), monitor, share);
					promise.set_value(window);
				}
				catch (...) {
					promise.set_exception(std::current_exception());
				}
			}
		};
		struct GLFWMEvent_DestroyWindow : GLFWMEvent {
			GLFWwindow* window = nullptr;
			GLFWMEvent_DestroyWindow(GLFWwindow* _window) : window(_window) {};

			std::promise<void> promise;
			GLFWMFuture<void>  getFuture() { return promise.get_future(); }

			void execute(GLFWMContext* ctx) override {
				try {
					glfwDestroyWindow(window);
					promise.set_value();
				}
				catch (...) {
					promise.set_exception(std::current_exception());
				}
			}
		};
		struct GLFWMEvent_PollEvents : GLFWMEvent {
			std::promise<void> promise;
			GLFWMFuture<void>  getFuture() { return promise.get_future(); }

			void execute(GLFWMContext* ctx) override {
				try {
					glfwPollEvents();
					promise.set_value();
				}
				catch (...) {
					promise.set_exception(std::current_exception());
				}
			}
		};

		template <typename CallbackFunc>
		struct GLFWMEvent_WindowCallback : GLFWMEvent {
			GLFWwindow* window;
			CallbackFunc callback;
			std::function<CallbackFunc(GLFWwindow*, CallbackFunc)> setter;

			std::promise<CallbackFunc> promise;
			GLFWMFuture<CallbackFunc> getFuture() { return promise.get_future(); }

			GLFWMEvent_WindowCallback(
				GLFWwindow* window,
				CallbackFunc callback,
				std::function<CallbackFunc(GLFWwindow*, CallbackFunc)> setter
			) : window(window), callback(callback), setter(setter) {
			}

			void execute(GLFWMContext* ctx) override {
				try {
					CallbackFunc oldCallback = setter(window, callback);
					promise.set_value(oldCallback);
				}
				catch (...) {
					promise.set_exception(std::current_exception());
				}
			}
		};

		// -------------------------------------------------------------------------
		// Utility Functions
		// -------------------------------------------------------------------------

		template<
			typename CallbackFuncT,
			CallbackFuncT GLFWMWindowCallbacks::* CallbackMemberPtr,
			CallbackFuncT(*GLFWFunction)(GLFWwindow*, CallbackFuncT),
			auto AdditionalCallbackFunc,
			typename... Args
		>
		CallbackFuncT registerWindowCallbackWithAdditionalCallback(GLFWwindow* window, CallbackFuncT callback) {
			GLFWMContext* const ctx = GLFWMContext::getContext();
			CallbackFuncT prevCallback = nullptr;
			{
				std::lock_guard<std::mutex> lock(ctx->m_windowData_l3);
				if (ctx->windowData.contains(window)) {
					GLFWMWindowCallbacks& callbacks = ctx->windowData.at(window).callbacks;
					prevCallback = callbacks.*CallbackMemberPtr;
					callbacks.*CallbackMemberPtr = callback;
				}
			}
			const CallbackFuncT middlemanCallback = [](GLFWwindow* window, Args... args) -> void {
				if constexpr (AdditionalCallbackFunc != nullptr) {
					AdditionalCallbackFunc(window, args...);
				}

				GLFWMContext* const ctx = GLFWMContext::getContext();

				std::lock_guard<std::mutex> lock(ctx->m_windowData_l3);
				if (!ctx->windowData.contains(window)) return;

				GLFWMWindowData& data = ctx->windowData.at(window);

				using EventT = GLFWMConstructedWindowEvent<CallbackMemberPtr, Args...>;
				std::unique_ptr<EventT> event = std::make_unique<EventT>();

				event->window = window;
				event->args = std::make_tuple(args...);

				data.events.push_back(std::move(event));
			};

			CallbackFuncT active = middlemanCallback;
			if constexpr (AdditionalCallbackFunc == nullptr) {
				if (callback == nullptr) active = nullptr;
			}
			auto promise = ctx->enqueue<GLFWMEvent_WindowCallback<CallbackFuncT>>(window, active, GLFWFunction);
			return prevCallback;
		}

		template<
			typename CallbackFuncT,
			CallbackFuncT GLFWMWindowCallbacks::* CallbackMemberPtr,
			CallbackFuncT(*GLFWFunction)(GLFWwindow*, CallbackFuncT),
			typename... Args
		>
		CallbackFuncT registerWindowCallback(GLFWwindow* window, CallbackFuncT callback) {
			return registerWindowCallbackWithAdditionalCallback<
				CallbackFuncT,
				CallbackMemberPtr, 
				GLFWFunction,
				nullptr,
				Args...
			>(window, callback);
		}

	} // namespace Private

#define _LESZEK_GLFWM_INIT_THREAD_REQUIRED() {\
	Private::GLFWMContext* const _ctx = Private::GLFWMContext::getContext();\
	std::thread::id _currentThread = std::this_thread::get_id();\
	std::lock_guard _lock(_ctx->m_threadData_l2);\
	if (!_ctx->threadData.contains(_currentThread))\
		throw std::runtime_error(std::string(__func__) + "(): Thread Not Initialized! Did you forget glfwmInit()?");\
}
#define _LESZEK_GLFWM_INIT_REQUIRED() {\
	Private::GLFWMContext* const _ctx = Private::GLFWMContext::getContext();\
	if (!_ctx->initCount)\
		throw std::runtime_error(std::string(__func__) + "(): GLFWM Not Initialized! Did you forget glfwmInit()?");\
}
#define _LESZEK_GLFWM_ERROR(CODE, MESSAGE) {\
	Private::GLFWMContext* const ctx = Private::GLFWMContext::getContext();\
	const std::thread::id currentThread = std::this_thread::get_id();\
	{\
		std::lock_guard<std::mutex> lock(ctx->m_threadData_l2);\
		if (ctx->threadData.contains(currentThread))\
			ctx->threadData.at(currentThread).errors.push_back({ CODE, MESSAGE });\
	}\
}
#define _LESZEK_GLFWM_INVALID_ARG() _LESZEK_GLFWM_ERROR(GLFW_INVALID_VALUE, "invalid arguments passed")

	// =========================================================================
	// GLFWM specific function implementations
	// =========================================================================
	
	void glfwm_setNewEventCallback(GLFWMneweventfun callback) {
		Private::GLFWMContext::getContext()->newEventCallback = callback;
	}

	void glfwm_defaultNewEventCallback() {
		{
			std::lock_guard<std::mutex> lock(Private::g_wakeMutex);
			Private::g_signalPending = true;
		}
		Private::g_wakeCV.notify_one();
	}

	void glfwm_executeQueueTasks() {
		if (std::this_thread::get_id() != Private::g_mainThreadId) {
			// INFO: This method NEEDS to be called on the main thread!
			// Thats the whole point of this library!
			// 
			// Other threads send tasks for the main thread to execute
			// which need to be executed on the main thread, and this
			// function runs all of those scheduled tasks!
			throw std::runtime_error("Tried to run glfwm_executeQueueTasks() on not main thread!");
		}
		Private::GLFWMContext::getContext()->executeEvents();
	}

	void glfwm_defaultWaitForTasks(int max_ms_wait_time) {
		std::unique_lock<std::mutex> lock(Private::g_wakeMutex);
		Private::g_wakeCV.wait_for(lock,
			std::chrono::milliseconds(max_ms_wait_time),
			[] { return Private::g_signalPending; }
		);
		Private::g_signalPending = false;
	}

	// =========================================================================
	// GLFWM function implementations
	// The tags (Main-Thread / Any-Thread) are not for the user!
	// Those are helper tags related to underlying glfw functions.
	// If function is tagged Main-Thread, it means that for example
	// glfwInit needs to be called on the main thread, not that the
	// wrapped glfwmInit also needs to do so. For that info look
	// at the top of this file where all the functions are listed.
	// =========================================================================

	/* Main-Thread */ int glfwmInit(void) {
		Private::GLFWMContext* ctx = Private::GLFWMContext::getContext();

		// GLFWM Specific Stuff
		{
			std::thread::id currentThread = std::this_thread::get_id();
			std::lock_guard<std::mutex> lock(ctx->m_threadData_l2);
			if (!ctx->threadData.contains(currentThread)) {
				Private::GLFWMThreadData threadData;
				threadData.threadID = currentThread;
				ctx->threadData.insert({ currentThread, threadData });
			}
			Private::GLFWMThreadData& threadData = ctx->threadData.at(currentThread);
			threadData.initCount++;
		}

		// Running glfwInit():
		std::lock_guard<std::mutex> lock(ctx->m_init_l1);
		if (ctx->initCount) {
			ctx->initCount++;
			return ctx->initData.initCode;
		}

		std::unique_ptr<Private::GLFWMEvent_Init> initEvent = std::make_unique<Private::GLFWMEvent_Init>();
		initEvent->initHints = ctx->initData.initHints;
		auto future = ctx->enqueue(std::move(initEvent));
		int returnCode = future.get();
		ctx->initCount++;
		return returnCode;
	}
	/* Main-Thread */ void glfwmTerminate(void) {
		_LESZEK_GLFWM_INIT_THREAD_REQUIRED();
		Private::GLFWMContext* ctx = Private::GLFWMContext::getContext();

		// GLFWM Specific Stuff
		std::thread::id currentThread = std::this_thread::get_id();
		std::unordered_set<GLFWwindow*> windowsToDestroy = {};
		{
			std::lock_guard<std::mutex> lock(ctx->m_threadData_l2);
			if (ctx->threadData.contains(currentThread)) {
				Private::GLFWMThreadData& threadData = ctx->threadData.at(currentThread);
				threadData.initCount--;
				if (threadData.initCount == 0) {
					windowsToDestroy = threadData.windows;
					ctx->threadData.erase(currentThread);
					{
						std::lock_guard<std::mutex> lock(ctx->m_lastError_l4);
						ctx->lastError.erase(currentThread);
					}
				}
			}
		}

		for (GLFWwindow* window : windowsToDestroy) {
			glfwmDestroyWindow(window);
		}

		// Running glfwTerminate():
		{
			std::lock_guard<std::mutex> lock(ctx->m_init_l1);
			if (ctx->initCount > 1) {
				ctx->initCount--;
				return;
			}
			auto future = ctx->enqueue<Private::GLFWMEvent_Terminate>();
			future.get();
			ctx->initCount--;
		}
	}
	/* Main-Thread */ void glfwmPollEvents(void) {
		_LESZEK_GLFWM_INIT_THREAD_REQUIRED();
		Private::GLFWMContext* ctx = Private::GLFWMContext::getContext();
		std::thread::id currentThread = std::this_thread::get_id();
		if (Private::isMainThread()) {
			glfwPollEvents();
		}
		else {
			auto future = ctx->enqueue<Private::GLFWMEvent_PollEvents>();
			future.get();
		}
		struct EventData {
			std::vector<std::unique_ptr<Private::GLFWMWindowEvent>> events;
			Private::GLFWMWindowCallbacks callbacks;
			GLFWwindow* window;
		};
		std::vector<EventData> eventsToParse;
		std::vector<Private::GLFWMError> errors;
		GLFWmonitorfun monitorCallback = nullptr;
		std::vector<std::pair<GLFWmonitor*, int>> monitorEvents;
		{
			std::lock_guard<std::mutex> lock(ctx->m_threadData_l2);
			if (ctx->threadData.contains(currentThread)) {
				Private::GLFWMThreadData& threadData = ctx->threadData.at(currentThread);
				{
					std::lock_guard<std::mutex> lock(ctx->m_windowData_l3);
					for (GLFWwindow* window : threadData.windows) {
						if (ctx->windowData.contains(window)) {
							Private::GLFWMWindowData& windowData = ctx->windowData.at(window);

							EventData data;
							data.events = std::move(windowData.events);
							windowData.events.clear();
							data.callbacks = windowData.callbacks;
							data.window = window;

							eventsToParse.push_back(std::move(data));
						}
					}
				}
				errors = std::move(threadData.errors);
				threadData.errors.clear();

				monitorEvents = std::move(threadData.monitorEvents);
				threadData.monitorEvents.clear();
				monitorCallback = threadData.monitorCallback;
			}
		}
		if (Private::g_errorCallback != nullptr) {
			for (auto& error : errors) {
				Private::g_errorCallback(error.errorCode, error.description.c_str());
			}
		}
		if (monitorCallback != nullptr) {
			for (auto [monitor, event] : monitorEvents) {
				monitorCallback(monitor, event);
			}
		}
		for (EventData& data : eventsToParse) {
			for (auto& event : data.events) {
				event->window = data.window;
				event->execute(data.callbacks);
			}
		}
	}

	/* Main-Thread */ void glfwmInitHint(int hint, int value) {
		Private::GLFWMContext* const ctx = Private::GLFWMContext::getContext();
		ctx->initData.initHints.setHint(hint, value);
	}
	/* Main-Thread */ void glfwmInitAllocator(const GLFWallocator* allocator) {
		_LESZEK_GLFWM_UNSUPPORTED_FUNCTION("glfwmInitAllocator");
		return glfwInitAllocator(allocator);
	}
	/* Any-Thread  */ void glfwmGetVersion(int* major, int* minor, int* rev) {
		glfwGetVersion(major, minor, rev);
	}
	/* Any-Thread  */ const char* glfwmGetVersionString(void) {
		return glfwGetVersionString();
	}
	/* Any-Thread  */ int glfwmGetError(const char** description) {
		Private::GLFWMContext* const ctx = Private::GLFWMContext::getContext();
		std::thread::id currentThread = std::this_thread::get_id();
		{
			std::lock_guard<std::mutex> lock(ctx->m_lastError_l4);
			if (ctx->lastError.contains(currentThread)) {
				Private::GLFWMLastError& lastError = ctx->lastError.at(currentThread);
				lastError.heldDescription = std::move(lastError.description);
				lastError.description.clear();
				if (description) {
					if (lastError.errorCode == GLFW_NO_ERROR)
						*description = nullptr;
					else
						*description = lastError.heldDescription.c_str();
				}

				int code = lastError.errorCode;
				lastError.errorCode = GLFW_NO_ERROR;
				// Not clearing the lastError.description, since we need const char to stay alive

				return code;
			}
		}
		return GLFW_NO_ERROR;
	}
	/* Main-Thread */ GLFWerrorfun glfwmSetErrorCallback(GLFWerrorfun callback) {
		GLFWerrorfun prevCallback = Private::g_errorCallback;
		Private::g_errorCallback = callback;
		return prevCallback;
	}
	/* Any-Thread  */ int glfwmGetPlatform(void) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		return glfwGetPlatform();
	}
	/* Any-Thread  */ int glfwmPlatformSupported(int platform) {
		return glfwPlatformSupported(platform);
	}
	/* Main-Thread */ GLFWmonitor** glfwmGetMonitors(int* count) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		Private::GLFWMContext* const ctx = Private::GLFWMContext::getContext();
		return ctx->enqueueTask(glfwGetMonitors, count).get();
	}
	/* Main-Thread */ GLFWmonitor* glfwmGetPrimaryMonitor(void) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		Private::GLFWMContext* const ctx = Private::GLFWMContext::getContext();
		return ctx->enqueueTask(glfwGetPrimaryMonitor).get();
	}
	/* Main-Thread */ void glfwmGetMonitorPos(GLFWmonitor* monitor, int* xpos, int* ypos) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		Private::GLFWMContext* const ctx = Private::GLFWMContext::getContext();
		return ctx->enqueueTask(glfwGetMonitorPos, monitor, xpos, ypos).get();
	}
	/* Main-Thread */ void glfwmGetMonitorWorkarea(GLFWmonitor* monitor, int* xpos, int* ypos, int* width, int* height) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		Private::GLFWMContext* const ctx = Private::GLFWMContext::getContext();
		return ctx->enqueueTask(glfwGetMonitorWorkarea, monitor, xpos, ypos, width, height).get();
	}
	/* Main-Thread */ void glfwmGetMonitorPhysicalSize(GLFWmonitor* monitor, int* widthMM, int* heightMM) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		Private::GLFWMContext* const ctx = Private::GLFWMContext::getContext();
		return ctx->enqueueTask(glfwGetMonitorPhysicalSize, monitor, widthMM, heightMM).get();
	}
	/* Main-Thread */ void glfwmGetMonitorContentScale(GLFWmonitor* monitor, float* xscale, float* yscale) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		Private::GLFWMContext* const ctx = Private::GLFWMContext::getContext();
		return ctx->enqueueTask(glfwGetMonitorContentScale, monitor, xscale, yscale).get();
	}
	/* Main-Thread */ const char* glfwmGetMonitorName(GLFWmonitor* monitor) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		Private::GLFWMContext* const ctx = Private::GLFWMContext::getContext();
		return ctx->enqueueTask(glfwGetMonitorName, monitor).get();
	}
	/* Any-Thread  */ void glfwmSetMonitorUserPointer(GLFWmonitor* monitor, void* pointer) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		Private::GLFWMContext* const ctx = Private::GLFWMContext::getContext();
		ctx->enqueueTask(glfwSetMonitorUserPointer, monitor, pointer);
	}
	/* Any-Thread  */ void* glfwmGetMonitorUserPointer(GLFWmonitor* monitor) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		Private::GLFWMContext* const ctx = Private::GLFWMContext::getContext();
		return ctx->enqueueTask(glfwGetMonitorUserPointer, monitor).get();
	}
	/* Main-Thread */ GLFWmonitorfun glfwmSetMonitorCallback(GLFWmonitorfun callback) {
		_LESZEK_GLFWM_INIT_THREAD_REQUIRED();
		Private::GLFWMContext* const ctx = Private::GLFWMContext::getContext();
		const std::thread::id currentThread = std::this_thread::get_id();

		{
			std::lock_guard lock(ctx->m_monitorCallbackRegistered_l1);
			if (!ctx->monitorCallbackRegistered) {
				ctx->monitorCallbackRegistered = true;
				ctx->enqueueTask(glfwSetMonitorCallback, [](GLFWmonitor* monitor, int event) {
					Private::GLFWMContext* const ctx = Private::GLFWMContext::getContext();
					{
						std::lock_guard<std::mutex> lock(ctx->m_threadData_l2);
						for (auto& [id, data] : ctx->threadData) {
							if (data.monitorCallback)
								data.monitorEvents.push_back({ monitor, event });
						}
					}
				});
			}
		}
		GLFWmonitorfun prevCallback = nullptr;
		{
			std::lock_guard<std::mutex> lock(ctx->m_threadData_l2);
			if (ctx->threadData.contains(currentThread)) {
				prevCallback = ctx->threadData.at(currentThread).monitorCallback;
				ctx->threadData.at(currentThread).monitorCallback = callback;
			}
		}
		return prevCallback;
	}
	/* Main-Thread */ const GLFWvidmode* glfwmGetVideoModes(GLFWmonitor* monitor, int* count) {
		_LESZEK_GLFWM_UNSUPPORTED_FUNCTION("glfwmGetVideoModes");
		return glfwGetVideoModes(monitor, count);
	}
	/* Main-Thread */ const GLFWvidmode* glfwmGetVideoMode(GLFWmonitor* monitor) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		Private::GLFWMContext* const ctx = Private::GLFWMContext::getContext();
		return ctx->enqueueTask(glfwGetVideoMode, monitor).get();
	}
	/* Main-Thread */ void glfwmSetGamma(GLFWmonitor* monitor, float gamma) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		Private::GLFWMContext* const ctx = Private::GLFWMContext::getContext();
		ctx->enqueueTask(glfwSetGamma, monitor, gamma);
	}
	/* Main-Thread */ const GLFWgammaramp* glfwmGetGammaRamp(GLFWmonitor* monitor) {
		_LESZEK_GLFWM_UNSUPPORTED_FUNCTION("glfwmGetGammaRamp");
		return glfwGetGammaRamp(monitor);
	}
	/* Main-Thread */ void glfwmSetGammaRamp(GLFWmonitor* monitor, const GLFWgammaramp* ramp) {
		_LESZEK_GLFWM_UNSUPPORTED_FUNCTION("glfwmSetGammaRamp");
		glfwSetGammaRamp(monitor, ramp);
	}
	/* Main-Thread */ void glfwmDefaultWindowHints(void) {
		_LESZEK_GLFWM_INIT_THREAD_REQUIRED();

		Private::GLFWMContext* const ctx = Private::GLFWMContext::getContext();
		const std::thread::id currentThread = std::this_thread::get_id();
		std::lock_guard<std::mutex> lock(ctx->m_threadData_l2);

		if(ctx->threadData.contains(currentThread))
			ctx->threadData.at(currentThread).hints.resetToDefault();
	}
	/* Main-Thread */ void glfwmWindowHint(int hint, int value) {
		_LESZEK_GLFWM_INIT_THREAD_REQUIRED();

		Private::GLFWMContext* const ctx = Private::GLFWMContext::getContext();
		const std::thread::id currentThread = std::this_thread::get_id();
		std::lock_guard<std::mutex> lock(ctx->m_threadData_l2);

		if (ctx->threadData.contains(currentThread))
			ctx->threadData.at(currentThread).hints.setHint(hint, value);
	}
	/* Main-Thread */ void glfwmWindowHintString(int hint, const char* value) {
		_LESZEK_GLFWM_INIT_THREAD_REQUIRED();

		Private::GLFWMContext* const ctx = Private::GLFWMContext::getContext();
		const std::thread::id currentThread = std::this_thread::get_id();
		std::lock_guard<std::mutex> lock(ctx->m_threadData_l2);

		if (ctx->threadData.contains(currentThread))
			ctx->threadData.at(currentThread).hints.setHint(hint, value);
	}
	/* Main-Thread */ GLFWwindow* glfwmCreateWindow(int width, int height, const char* title, GLFWmonitor* monitor, GLFWwindow* share) {
		_LESZEK_GLFWM_INIT_THREAD_REQUIRED();
		Private::GLFWMContext* const ctx = Private::GLFWMContext::getContext();
		const std::thread::id currentThread = std::this_thread::get_id();

		std::unique_ptr<Private::GLFWMEvent_CreateWindow> event = std::make_unique<Private::GLFWMEvent_CreateWindow>();
		event->width = width;
		event->height = height;
		event->title = std::string(title);
		event->monitor = monitor;
		event->share = share;

		{
			std::lock_guard<std::mutex> lock(ctx->m_threadData_l2);
			if (ctx->threadData.contains(currentThread))
				event->hints = ctx->threadData.at(currentThread).hints;
		}

		auto promise = ctx->enqueue(std::move(event));
		GLFWwindow* window = promise.get();
		if (window == nullptr) return nullptr;

		{
			std::lock_guard<std::mutex> lock(ctx->m_windowData_l3);

			Private::GLFWMWindowData& windowData = ctx->windowData[window];
			windowData.window = window;
			windowData.ownerThread = currentThread;
		}
		{
			std::lock_guard<std::mutex> lock(ctx->m_threadData_l2);
			if (ctx->threadData.contains(currentThread)) {
				Private::GLFWMThreadData& threadData = ctx->threadData.at(currentThread);
				threadData.windows.insert(window);
			}
		}
		// Make sure it also registers key state caching callback
		glfwmSetKeyCallback(window, nullptr);
		return window;
	}
	/* Main-Thread */ void glfwmDestroyWindow(GLFWwindow* window) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		Private::GLFWMContext* const ctx = Private::GLFWMContext::getContext();
		auto promise = ctx->enqueue<Private::GLFWMEvent_DestroyWindow>(window);
		promise.get();
		{
			std::lock_guard lock1(ctx->m_threadData_l2);
			std::lock_guard lock2(ctx->m_windowData_l3);
			if (ctx->windowData.contains(window)) {
				Private::GLFWMWindowData& data = ctx->windowData.at(window);
				if (ctx->threadData.contains(data.ownerThread)) {
					ctx->threadData.at(data.ownerThread).windows.erase(window);
				}
				ctx->windowData.erase(window);
			}
		}
	}
	/* Any-Thread  */ int glfwmWindowShouldClose(GLFWwindow* window) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		if (window == nullptr) {
			_LESZEK_GLFWM_INVALID_ARG();
			return true;
		}
		// Other than window == nullptr and not initialized, there are no other errors that could occur here
		return glfwWindowShouldClose(window);
	}
	/* Any-Thread  */ void glfwmSetWindowShouldClose(GLFWwindow* window, int value) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		if (window == nullptr) {
			_LESZEK_GLFWM_INVALID_ARG();
			return;
		}
		// Other than window == nullptr and not initialized, there are no other errors that could occur here
		return glfwSetWindowShouldClose(window, value);
	}
	/* Main-Thread */ const char* glfwmGetWindowTitle(GLFWwindow* window) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		if (window == nullptr) {
			_LESZEK_GLFWM_INVALID_ARG();
			return nullptr;
		}

		Private::GLFWMContext* const ctx = Private::GLFWMContext::getContext();
		auto promise = ctx->enqueueTask([window]() -> const char* {
			Private::GLFWMContext* const ctx = Private::GLFWMContext::getContext();
			std::lock_guard lock(ctx->m_windowData_l3);
			auto it = ctx->windowData.find(window);
			if (it != ctx->windowData.end()) {
				Private::GLFWMWindowData& wnd = it->second;
				const char* cstr = glfwGetWindowTitle(window);
				wnd.windowTitleStr = std::string(cstr);
				return wnd.windowTitleStr.c_str();
			}
			return nullptr;
		});
		return promise.get();
	}
	/* Main-Thread */ void glfwmSetWindowTitle(GLFWwindow* window, const char* title) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		Private::GLFWMContext* const ctx = Private::GLFWMContext::getContext();
		std::string titleStr = title;
		ctx->enqueueTask([window, titleStr]() {
			glfwSetWindowTitle(window, titleStr.c_str());
		});
	}
	/* Main-Thread */ void glfwmSetWindowIcon(GLFWwindow* window, int count, const GLFWimage* images) {
		_LESZEK_GLFWM_INIT_REQUIRED();

		using Pixel = std::decay_t<decltype(*std::declval<GLFWimage>().pixels)>;

		// Exactly the same as GLFWimage, but
		// std::vector<unsigned char> instead of unsigned char*
		struct Icon {
			int width;
			int height;
			std::vector<Pixel> pixels;
		};

		std::vector<Icon> icons(count);
		for (int i = 0; i < count; i++) {
			const GLFWimage* img = &images[i];
			icons[i].width = img->width;
			icons[i].height = img->height;

			icons[i].pixels.resize(img->width * img->height);
			std::memcpy(icons[i].pixels.data(), img->pixels, img->width * img->height * sizeof(Pixel) * 4);
		}

		Private::GLFWMContext* const ctx = Private::GLFWMContext::getContext();
		ctx->enqueueTask([window, icons = std::move(icons)]() {
			// Rebuild the GLFWimage* array;

			std::vector<GLFWimage> images;
			images.resize(icons.size());
			for (int i = 0; i < icons.size(); i++) {
				images[i].width = icons[i].width;
				images[i].height = icons[i].height;
				images[i].pixels = const_cast<Pixel*>(icons[i].pixels.data());
			}

			glfwSetWindowIcon(window, images.size(), images.data());
		});
	}
	/* Main-Thread */ void glfwmGetWindowPos(GLFWwindow* window, int* xpos, int* ypos) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		Private::GLFWMContext* const ctx = Private::GLFWMContext::getContext();
		return ctx->enqueueTask(glfwGetWindowPos, window, xpos, ypos).get();
	}
	/* Main-Thread */ void glfwmSetWindowPos(GLFWwindow* window, int xpos, int ypos) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		Private::GLFWMContext* const ctx = Private::GLFWMContext::getContext();
		ctx->enqueueTask(glfwSetWindowPos, window, xpos, ypos);
	}
	/* Main-Thread */ void glfwmGetWindowSize(GLFWwindow* window, int* width, int* height) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		Private::GLFWMContext* const ctx = Private::GLFWMContext::getContext();
		return ctx->enqueueTask(glfwGetWindowSize, window, width, height).get();
	}
	/* Main-Thread */ void glfwmSetWindowSizeLimits(GLFWwindow* window, int minwidth, int minheight, int maxwidth, int maxheight) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		Private::GLFWMContext* const ctx = Private::GLFWMContext::getContext();
		ctx->enqueueTask(glfwSetWindowSizeLimits, window, minwidth, minheight, maxwidth, maxheight);
	}
	/* Main-Thread */ void glfwmSetWindowAspectRatio(GLFWwindow* window, int numer, int denom) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		Private::GLFWMContext* const ctx = Private::GLFWMContext::getContext();
		ctx->enqueueTask(glfwSetWindowAspectRatio, window, numer, denom);
	}
	/* Main-Thread */ void glfwmSetWindowSize(GLFWwindow* window, int width, int height) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		Private::GLFWMContext* const ctx = Private::GLFWMContext::getContext();
		ctx->enqueueTask(glfwSetWindowSize, window, width, height);
	}
	/* Main-Thread */ void glfwmGetFramebufferSize(GLFWwindow* window, int* width, int* height) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		Private::GLFWMContext* const ctx = Private::GLFWMContext::getContext();
		return ctx->enqueueTask(glfwGetFramebufferSize, window, width, height).get();
	}
	/* Main-Thread */ void glfwmGetWindowFrameSize(GLFWwindow* window, int* left, int* top, int* right, int* bottom) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		Private::GLFWMContext* const ctx = Private::GLFWMContext::getContext();
		return ctx->enqueueTask(glfwGetWindowFrameSize, window, left, top, right, bottom).get();
	}
	/* Main-Thread */ void glfwmGetWindowContentScale(GLFWwindow* window, float* xscale, float* yscale) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		Private::GLFWMContext* const ctx = Private::GLFWMContext::getContext();
		return ctx->enqueueTask(glfwGetWindowContentScale, window, xscale, yscale).get();
	}
	/* Main-Thread */ float glfwmGetWindowOpacity(GLFWwindow* window) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		Private::GLFWMContext* const ctx = Private::GLFWMContext::getContext();
		return ctx->enqueueTask(glfwGetWindowOpacity, window).get();
	}
	/* Main-Thread */ void glfwmSetWindowOpacity(GLFWwindow* window, float opacity) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		Private::GLFWMContext* const ctx = Private::GLFWMContext::getContext();
		ctx->enqueueTask(glfwSetWindowOpacity, window, opacity);
	}
	/* Main-Thread */ void glfwmIconifyWindow(GLFWwindow* window) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		Private::GLFWMContext* const ctx = Private::GLFWMContext::getContext();
		ctx->enqueueTask(glfwIconifyWindow, window);
	}
	/* Main-Thread */ void glfwmRestoreWindow(GLFWwindow* window) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		Private::GLFWMContext* const ctx = Private::GLFWMContext::getContext();
		ctx->enqueueTask(glfwRestoreWindow, window);
	}
	/* Main-Thread */ void glfwmMaximizeWindow(GLFWwindow* window) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		Private::GLFWMContext* const ctx = Private::GLFWMContext::getContext();
		ctx->enqueueTask(glfwMaximizeWindow, window);
	}
	/* Main-Thread */ void glfwmShowWindow(GLFWwindow* window) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		Private::GLFWMContext* const ctx = Private::GLFWMContext::getContext();
		ctx->enqueueTask(glfwShowWindow, window);
	}
	/* Main-Thread */ void glfwmHideWindow(GLFWwindow* window) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		Private::GLFWMContext* const ctx = Private::GLFWMContext::getContext();
		ctx->enqueueTask(glfwHideWindow, window);
	}
	/* Main-Thread */ void glfwmFocusWindow(GLFWwindow* window) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		Private::GLFWMContext* const ctx = Private::GLFWMContext::getContext();
		ctx->enqueueTask(glfwFocusWindow, window);
	}
	/* Main-Thread */ void glfwmRequestWindowAttention(GLFWwindow* window) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		Private::GLFWMContext* const ctx = Private::GLFWMContext::getContext();
		ctx->enqueueTask(glfwRequestWindowAttention, window);
	}
	/* Main-Thread */ GLFWmonitor* glfwmGetWindowMonitor(GLFWwindow* window) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		Private::GLFWMContext* const ctx = Private::GLFWMContext::getContext();
		return ctx->enqueueTask(glfwGetWindowMonitor, window).get();
	}
	/* Main-Thread */ void glfwmSetWindowMonitor(GLFWwindow* window, GLFWmonitor* monitor, int xpos, int ypos, int width, int height, int refreshRate) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		Private::GLFWMContext* const ctx = Private::GLFWMContext::getContext();
		ctx->enqueueTask(glfwSetWindowMonitor, window, monitor, xpos, ypos, width, height, refreshRate);
	}
	/* Main-Thread */ int glfwmGetWindowAttrib(GLFWwindow* window, int attrib) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		Private::GLFWMContext* const ctx = Private::GLFWMContext::getContext();
		return ctx->enqueueTask(glfwGetWindowAttrib, window, attrib).get();
	}
	/* Main-Thread */ void glfwmSetWindowAttrib(GLFWwindow* window, int attrib, int value) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		Private::GLFWMContext* const ctx = Private::GLFWMContext::getContext();
		ctx->enqueueTask(glfwSetWindowAttrib, window, attrib, value);
	}
	/* Any-Thread  */ void glfwmSetWindowUserPointer(GLFWwindow* window, void* pointer) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		Private::GLFWMContext* const ctx = Private::GLFWMContext::getContext();
		ctx->enqueueTask(glfwSetWindowUserPointer, window, pointer);
	}
	/* Any-Thread  */ void* glfwmGetWindowUserPointer(GLFWwindow* window) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		Private::GLFWMContext* const ctx = Private::GLFWMContext::getContext();
		return ctx->enqueueTask(glfwGetWindowUserPointer, window).get();
	}
	/* Main-Thread */ GLFWwindowposfun glfwmSetWindowPosCallback(GLFWwindow* window, GLFWwindowposfun callback) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		return Private::registerWindowCallback<
			GLFWwindowposfun,
			&Private::GLFWMWindowCallbacks::windowposfun,
			glfwSetWindowPosCallback
			, int, int
		>(window, callback);
	}
	/* Main-Thread */ GLFWwindowsizefun glfwmSetWindowSizeCallback(GLFWwindow* window, GLFWwindowsizefun callback) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		return Private::registerWindowCallback<
			GLFWwindowsizefun,
			&Private::GLFWMWindowCallbacks::windowsizefun,
			glfwSetWindowSizeCallback
			, int, int
		>(window, callback);
	}
	/* Main-Thread */ GLFWwindowclosefun glfwmSetWindowCloseCallback(GLFWwindow* window, GLFWwindowclosefun callback) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		return Private::registerWindowCallback<
			GLFWwindowclosefun,
			&Private::GLFWMWindowCallbacks::windowclosefun,
			glfwSetWindowCloseCallback
		>(window, callback);
	}
	/* Main-Thread */ GLFWwindowrefreshfun glfwmSetWindowRefreshCallback(GLFWwindow* window, GLFWwindowrefreshfun callback) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		return Private::registerWindowCallback<
			GLFWwindowrefreshfun,
			&Private::GLFWMWindowCallbacks::windowrefreshfun,
			glfwSetWindowRefreshCallback
		>(window, callback);
	}
	/* Main-Thread */ GLFWwindowfocusfun glfwmSetWindowFocusCallback(GLFWwindow* window, GLFWwindowfocusfun callback) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		return Private::registerWindowCallback<
			GLFWwindowfocusfun,
			&Private::GLFWMWindowCallbacks::windowfocusfun,
			glfwSetWindowFocusCallback
			, int
		>(window, callback);
	}
	/* Main-Thread */ GLFWwindowiconifyfun glfwmSetWindowIconifyCallback(GLFWwindow* window, GLFWwindowiconifyfun callback) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		return Private::registerWindowCallback<
			GLFWwindowiconifyfun,
			&Private::GLFWMWindowCallbacks::windowiconifyfun,
			glfwSetWindowIconifyCallback
			, int
		>(window, callback);
	}
	/* Main-Thread */ GLFWwindowmaximizefun glfwmSetWindowMaximizeCallback(GLFWwindow* window, GLFWwindowmaximizefun callback) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		return Private::registerWindowCallback<
			GLFWwindowmaximizefun,
			&Private::GLFWMWindowCallbacks::windowmaximizefun,
			glfwSetWindowMaximizeCallback
			, int
		>(window, callback);
	}
	/* Main-Thread */ GLFWframebuffersizefun glfwmSetFramebufferSizeCallback(GLFWwindow* window, GLFWframebuffersizefun callback) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		return Private::registerWindowCallback<
			GLFWframebuffersizefun,
			&Private::GLFWMWindowCallbacks::framebuffersizefun,
			glfwSetFramebufferSizeCallback
			, int, int
		>(window, callback);
	}
	/* Main-Thread */ GLFWwindowcontentscalefun glfwmSetWindowContentScaleCallback(GLFWwindow* window, GLFWwindowcontentscalefun callback) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		return Private::registerWindowCallback<
			GLFWwindowcontentscalefun,
			&Private::GLFWMWindowCallbacks::windowcontentscalefun,
			glfwSetWindowContentScaleCallback
			, float, float
		>(window, callback);
	}
	/* Main-Thread */ void glfwmWaitEvents(void) {
		_LESZEK_GLFWM_UNSUPPORTED_FUNCTION("glfwmWaitEvents");
		glfwWaitEvents();
	}
	/* Main-Thread */ void glfwmWaitEventsTimeout(double timeout) {
		_LESZEK_GLFWM_UNSUPPORTED_FUNCTION("glfwmWaitEventsTimeout");
		glfwWaitEventsTimeout(timeout);
	}
	/* Any-Thread  */ void glfwmPostEmptyEvent(void) {
		_LESZEK_GLFWM_UNSUPPORTED_FUNCTION("glfwmPostEmptyEvent");
		glfwPostEmptyEvent();
	}
	/* Main-Thread */ int glfwmGetInputMode(GLFWwindow* window, int mode) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		Private::GLFWMContext* const ctx = Private::GLFWMContext::getContext();
		return ctx->enqueueTask(glfwGetInputMode, window, mode).get();
	}
	/* Main-Thread */ void glfwmSetInputMode(GLFWwindow* window, int mode, int value) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		Private::GLFWMContext* const ctx = Private::GLFWMContext::getContext();
		ctx->enqueueTask(glfwSetInputMode, window, mode, value);
	}
	/* Main-Thread */ int glfwmRawMouseMotionSupported(void) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		Private::GLFWMContext* const ctx = Private::GLFWMContext::getContext();
		return ctx->enqueueTask(glfwRawMouseMotionSupported).get();
	}
	/* Main-Thread */ const char* glfwmGetKeyName(int key, int scancode) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		if (key != GLFW_KEY_UNKNOWN && (key < GLFW_KEY_SPACE || key > GLFW_KEY_LAST))
			return nullptr;

		if (key != GLFW_KEY_UNKNOWN && (key != GLFW_KEY_KP_EQUAL &&
			(key < GLFW_KEY_KP_0 || key > GLFW_KEY_KP_ADD) &&
			(key < GLFW_KEY_APOSTROPHE || key > GLFW_KEY_WORLD_2)))
			return nullptr;

		uint64_t mapkey = (uint32_t)key << 32 | (uint32_t)scancode;

		Private::GLFWMContext* const ctx = Private::GLFWMContext::getContext();

		if (key != GLFW_KEY_UNKNOWN && key >= GLFW_KEY_SPACE && key <= GLFW_KEY_LAST
			&& key >= 0 && key < ctx->keyNameCacheTable.size()) {

			std::lock_guard lock(ctx->m_keyNameCacheTable_l5);
			std::unique_ptr<std::string>& namePtr = ctx->keyNameCacheTable[key];
			if (!ctx->keyNameCacheTable_PresentEntry[key]) {
				std::optional<std::string> newName;
				{
					Private::unlock_guard unlock(ctx->m_keyNameCacheTable_l5);
					newName = ctx->enqueueTask([key, scancode]() -> std::optional<std::string> {
						const char* cstr = glfwGetKeyName(key, scancode);
						return cstr == nullptr ? std::nullopt : std::optional<std::string>(cstr);
					}).get();
				}

				if (!ctx->keyNameCacheTable_PresentEntry[key]) {
					ctx->keyNameCacheTable_PresentEntry[key] = true;
					if(newName.has_value())
						namePtr = std::make_unique<std::string>(newName.value());
				}
			}
			return namePtr == nullptr ? nullptr : namePtr->c_str();
		}

		{
			std::lock_guard lock(ctx->m_keyNameCacheTable_unknown_l5);
			if (ctx->keyNameCacheTable_unknown.contains(mapkey)) {
				std::unique_ptr<std::string>& namePtr = ctx->keyNameCacheTable_unknown.at(mapkey);
				return namePtr == nullptr ? nullptr : namePtr->c_str();
			}
		}

		std::optional<std::string> newName = ctx->enqueueTask([key, scancode]() -> std::optional<std::string> {
			const char* cstr = glfwGetKeyName(key, scancode);
			return cstr == nullptr ? std::nullopt : std::optional<std::string>(cstr);
		}).get();

		{
			std::lock_guard lock(ctx->m_keyNameCacheTable_unknown_l5);
			if (newName.has_value()) {
				ctx->keyNameCacheTable_unknown.try_emplace(mapkey, std::make_unique<std::string>(newName.value()));
				std::unique_ptr<std::string>& namePtr = ctx->keyNameCacheTable_unknown.at(mapkey);
				return namePtr->c_str();
			}
			else {
				ctx->keyNameCacheTable_unknown.try_emplace(mapkey, nullptr);
				return nullptr;
			}
		}
	}
	/* Any-Thread  */ int glfwmGetKeyScancode(int key) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		Private::GLFWMContext* const ctx = Private::GLFWMContext::getContext();

		if (key < GLFW_KEY_SPACE || key > GLFW_KEY_LAST 
			|| key < 0 || key >= ctx->keyScanCodeTable.size())
			return -1;

		{
			std::lock_guard lock(ctx->m_keyScanCodeTable_l5);
			if (ctx->keyScanCodeTable_PresentEntry[key])
				return ctx->keyScanCodeTable[key];
		}
		
		auto promise = ctx->enqueueTask(glfwGetKeyScancode, key);
		int scancode = promise.get();

		{
			std::lock_guard lock(ctx->m_keyScanCodeTable_l5);
			if (!ctx->keyScanCodeTable_PresentEntry[key]) {
				ctx->keyScanCodeTable[key] = scancode;
				ctx->keyScanCodeTable_PresentEntry[key] = true;
			}
		}

		return scancode;
	}
	/* Main-Thread */ int glfwmGetKey(GLFWwindow* window, int key) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		Private::GLFWMContext* const ctx = Private::GLFWMContext::getContext();
		std::lock_guard<std::mutex> lock(ctx->m_windowData_l3);
		if (ctx->windowData.contains(window)) {
			return ctx->windowData.at(window).getKeyState(key);
		}
		return GLFW_RELEASE;
	}
	/* Main-Thread */ int glfwmGetMouseButton(GLFWwindow* window, int button) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		Private::GLFWMContext* const ctx = Private::GLFWMContext::getContext();
		return ctx->enqueueTask(glfwGetMouseButton, window, button).get();
	}
	/* Main-Thread */ void glfwmGetCursorPos(GLFWwindow* window, double* xpos, double* ypos) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		Private::GLFWMContext* const ctx = Private::GLFWMContext::getContext();
		return ctx->enqueueTask(glfwGetCursorPos, window, xpos, ypos).get();
	}
	/* Main-Thread */ void glfwmSetCursorPos(GLFWwindow* window, double xpos, double ypos) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		Private::GLFWMContext* const ctx = Private::GLFWMContext::getContext();
		return ctx->enqueueTask(glfwSetCursorPos, window, xpos, ypos).get();
	}
	/* Main-Thread */ GLFWcursor* glfwmCreateCursor(const GLFWimage* image, int xhot, int yhot) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		Private::GLFWMContext* const ctx = Private::GLFWMContext::getContext();
		return ctx->enqueueTask(glfwCreateCursor, image, xhot, yhot).get();
	}
	/* Main-Thread */ GLFWcursor* glfwmCreateStandardCursor(int shape) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		Private::GLFWMContext* const ctx = Private::GLFWMContext::getContext();
		return ctx->enqueueTask(glfwCreateStandardCursor, shape).get();
	}
	/* Main-Thread */ void glfwmDestroyCursor(GLFWcursor* cursor) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		Private::GLFWMContext* const ctx = Private::GLFWMContext::getContext();
		ctx->enqueueTask(glfwDestroyCursor, cursor);
	}
	/* Main-Thread */ void glfwmSetCursor(GLFWwindow* window, GLFWcursor* cursor) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		Private::GLFWMContext* const ctx = Private::GLFWMContext::getContext();
		ctx->enqueueTask(glfwSetCursor, window, cursor);
	}
	/* Main-Thread */ GLFWkeyfun glfwmSetKeyCallback(GLFWwindow* window, GLFWkeyfun callback) {
		_LESZEK_GLFWM_INIT_REQUIRED();

		return Private::registerWindowCallbackWithAdditionalCallback<
			GLFWkeyfun,
			&Private::GLFWMWindowCallbacks::keyfun,
			glfwSetKeyCallback,
			[](GLFWwindow* window, int key, int scancode, int action, int mods) -> void {
				Private::GLFWMContext* const ctx = Private::GLFWMContext::getContext();
				std::lock_guard<std::mutex> lock(ctx->m_windowData_l3);
				if (ctx->windowData.contains(window)) {
					Private::GLFWMWindowData& data = ctx->windowData.at(window);
					data.onKeyEvent(key, scancode, action, mods);
				}
			}
			, int, int, int, int
		>(window, callback);
	}
	/* Main-Thread */ GLFWcharfun glfwmSetCharCallback(GLFWwindow* window, GLFWcharfun callback) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		return Private::registerWindowCallback<
			GLFWcharfun,
			&Private::GLFWMWindowCallbacks::charfun,
			glfwSetCharCallback
			, unsigned int
		>(window, callback);
	}
	/* Main-Thread */ GLFWcharmodsfun glfwmSetCharModsCallback(GLFWwindow* window, GLFWcharmodsfun callback) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		return Private::registerWindowCallback<
			GLFWcharmodsfun,
			&Private::GLFWMWindowCallbacks::charmodsfun,
			glfwSetCharModsCallback
			, unsigned int, int
		>(window, callback);
	}
	/* Main-Thread */ GLFWmousebuttonfun glfwmSetMouseButtonCallback(GLFWwindow* window, GLFWmousebuttonfun callback) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		return Private::registerWindowCallback<
			GLFWmousebuttonfun,
			&Private::GLFWMWindowCallbacks::mousebuttonfun,
			glfwSetMouseButtonCallback
			, int, int, int
		>(window, callback);
	}
	/* Main-Thread */ GLFWcursorposfun glfwmSetCursorPosCallback(GLFWwindow* window, GLFWcursorposfun callback) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		return Private::registerWindowCallback<
			GLFWcursorposfun,
			&Private::GLFWMWindowCallbacks::cursorposfun,
			glfwSetCursorPosCallback
			, double, double
		>(window, callback);
	}
	/* Main-Thread */ GLFWcursorenterfun glfwmSetCursorEnterCallback(GLFWwindow* window, GLFWcursorenterfun callback) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		return Private::registerWindowCallback<
			GLFWcursorenterfun,
			&Private::GLFWMWindowCallbacks::cursorenterfun,
			glfwSetCursorEnterCallback
			, int
		>(window, callback);
	}
	/* Main-Thread */ GLFWscrollfun glfwmSetScrollCallback(GLFWwindow* window, GLFWscrollfun callback) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		return Private::registerWindowCallback<
			GLFWscrollfun,
			&Private::GLFWMWindowCallbacks::scrollfun,
			glfwSetScrollCallback
			, double, double
		>(window, callback);
	}
	/* Main-Thread */ GLFWdropfun glfwmSetDropCallback(GLFWwindow* window, GLFWdropfun callback) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		Private::GLFWMContext* const ctx = Private::GLFWMContext::getContext();
		GLFWdropfun prevCallback = nullptr;
		{
			std::lock_guard<std::mutex> lock(ctx->m_windowData_l3);
			if (ctx->windowData.contains(window)) {
				Private::GLFWMWindowCallbacks& callbacks = ctx->windowData.at(window).callbacks;
				prevCallback = callbacks.dropfun;
				callbacks.dropfun = callback;
			}
		}
		const GLFWdropfun middlemanCallback = [](GLFWwindow* window, int count, const char** paths) -> void {
			std::vector<std::string> paths_str;
			paths_str.reserve(count);
			for (int i = 0; i < count; i++) {
				paths_str.emplace_back(paths[i]);
			}

			Private::GLFWMContext* const ctx = Private::GLFWMContext::getContext();

			std::lock_guard<std::mutex> lock(ctx->m_windowData_l3);
			if (!ctx->windowData.contains(window)) return;

			Private::GLFWMWindowData& data = ctx->windowData.at(window);

			using EventT = Private::GLFWMWindowEvent_Drop;
			std::unique_ptr<EventT> event = std::make_unique<EventT>();

			event->window = window;
			event->paths = std::move(paths_str);

			data.events.push_back(std::move(event));
		};

		auto promise = ctx->enqueueTask(glfwSetDropCallback, window, callback == nullptr ? nullptr : middlemanCallback);
		return prevCallback;
	}
	/* Main-Thread */ int glfwmJoystickPresent(int jid) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		Private::GLFWMContext* const ctx = Private::GLFWMContext::getContext();
		return ctx->enqueueTask(glfwJoystickPresent, jid).get();
	}
	/* Main-Thread */ const float* glfwmGetJoystickAxes(int jid, int* count) {
		_LESZEK_GLFWM_UNSUPPORTED_FUNCTION("glfwmGetJoystickAxes");
		return glfwGetJoystickAxes(jid, count);
	}
	/* Main-Thread */ const unsigned char* glfwmGetJoystickButtons(int jid, int* count) {
		_LESZEK_GLFWM_UNSUPPORTED_FUNCTION("glfwmGetJoystickButtons");
		return glfwGetJoystickButtons(jid, count);
	}
	/* Main-Thread */ const unsigned char* glfwmGetJoystickHats(int jid, int* count) {
		_LESZEK_GLFWM_UNSUPPORTED_FUNCTION("glfwmGetJoystickHats");
		return glfwGetJoystickHats(jid, count);
	}
	/* Main-Thread */ const char* glfwmGetJoystickName(int jid) {
		_LESZEK_GLFWM_UNSUPPORTED_FUNCTION("glfwmGetJoystickName");
		return glfwGetJoystickName(jid);
	}
	/* Main-Thread */ const char* glfwmGetJoystickGUID(int jid) {
		_LESZEK_GLFWM_UNSUPPORTED_FUNCTION("glfwmGetJoystickGUID");
		return glfwGetJoystickGUID(jid);
	}
	/* Any-Thread  */ void glfwmSetJoystickUserPointer(int jid, void* pointer) {
		_LESZEK_GLFWM_UNSUPPORTED_FUNCTION("glfwmSetJoystickUserPointer");
		glfwSetJoystickUserPointer(jid, pointer);
	}
	/* Any-Thread  */ void* glfwmGetJoystickUserPointer(int jid) {
		_LESZEK_GLFWM_UNSUPPORTED_FUNCTION("glfwmGetJoystickUserPointer");
		return glfwGetJoystickUserPointer(jid);
	}
	/* Main-Thread */ int glfwmJoystickIsGamepad(int jid) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		Private::GLFWMContext* const ctx = Private::GLFWMContext::getContext();
		return ctx->enqueueTask(glfwJoystickIsGamepad, jid).get();
	}
	/* Main-Thread */ GLFWjoystickfun glfwmSetJoystickCallback(GLFWjoystickfun callback) {
		_LESZEK_GLFWM_UNSUPPORTED_FUNCTION("glfwmSetJoystickCallback");
		return glfwSetJoystickCallback(callback);
	}
	/* Main-Thread */ int glfwmUpdateGamepadMappings(const char* string) {
		_LESZEK_GLFWM_UNSUPPORTED_FUNCTION("glfwmUpdateGamepadMappings");
		return glfwUpdateGamepadMappings(string);
	}
	/* Main-Thread */ const char* glfwmGetGamepadName(int jid) {
		_LESZEK_GLFWM_UNSUPPORTED_FUNCTION("glfwmGetGamepadName");
		return glfwGetGamepadName(jid);
	}
	/* Main-Thread */ int glfwmGetGamepadState(int jid, GLFWgamepadstate* state) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		Private::GLFWMContext* const ctx = Private::GLFWMContext::getContext();
		return ctx->enqueueTask(glfwGetGamepadState, jid, state).get();
	}
	/* Main-Thread */ void glfwmSetClipboardString(GLFWwindow* window, const char* string) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		Private::GLFWMContext* const ctx = Private::GLFWMContext::getContext();
		std::string str = string;
		ctx->enqueueTask([window, str]() {
			glfwSetClipboardString(window, str.c_str());
		});
	}
	/* Main-Thread */ const char* glfwmGetClipboardString(GLFWwindow* window) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		if (window == nullptr) {
			_LESZEK_GLFWM_INVALID_ARG();
			return nullptr;
		}

		Private::GLFWMContext* const ctx = Private::GLFWMContext::getContext();
		auto promise = ctx->enqueueTask([window]() -> const char* {
			Private::GLFWMContext* const ctx = Private::GLFWMContext::getContext();
			std::lock_guard lock(ctx->m_windowData_l3);
			auto it = ctx->windowData.find(window);
			if (it != ctx->windowData.end()) {
				Private::GLFWMWindowData& wnd = it->second;
				const char* cstr = glfwGetClipboardString(window);
				wnd.clipboardStr = std::string(cstr);
				return wnd.clipboardStr.c_str();
			}
			return nullptr;
		});
		return promise.get();
	}
	/* Any-Thread  */ double glfwmGetTime(void) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		return glfwGetTime();
	}
	/* Any-Thread  */ void glfwmSetTime(double time) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		glfwSetTime(time);
	}
	/* Any-Thread  */ uint64_t glfwmGetTimerValue(void) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		return glfwGetTimerValue();
	}
	/* Any-Thread  */ uint64_t glfwmGetTimerFrequency(void) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		return glfwGetTimerFrequency();
	}
	/* Any-Thread  */ void glfwmMakeContextCurrent(GLFWwindow* window) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		glfwMakeContextCurrent(window);
	}
	/* Any-Thread  */ GLFWwindow* glfwmGetCurrentContext(void) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		return glfwGetCurrentContext();
	}
	/* Any-Thread  */ void glfwmSwapBuffers(GLFWwindow* window) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		return glfwSwapBuffers(window);
	}
	/* Any-Thread  */ void glfwmSwapInterval(int interval) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		return glfwSwapInterval(interval);
	}
	/* Any-Thread  */ int glfwmExtensionSupported(const char* extension) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		return glfwExtensionSupported(extension);
	}
	/* Any-Thread  */ GLFWglproc glfwmGetProcAddress(const char* procname) {
		_LESZEK_GLFWM_INIT_REQUIRED();
		return glfwGetProcAddress(procname);
	}
	/* Any-Thread  */ int glfwmVulkanSupported(void) {
		_LESZEK_GLFWM_UNSUPPORTED_FUNCTION("glfwmVulkanSupported");
		return glfwVulkanSupported();
	}
	/* Any-Thread  */ const char** glfwmGetRequiredInstanceExtensions(uint32_t* count) {
		_LESZEK_GLFWM_UNSUPPORTED_FUNCTION("glfwmGetRequiredInstanceExtensions");
		return glfwGetRequiredInstanceExtensions(count);
	}
#if defined(VK_VERSION_1_0) || defined(GLFW_INCLUDE_VULKAN)
	/* Any-Thread  */ GLFWvkproc glfwmGetInstanceProcAddress(VkInstance instance, const char* procname) {
		_LESZEK_GLFWM_UNSUPPORTED_FUNCTION("glfwmGetInstanceProcAddress");
		return glfwGetInstanceProcAddress(instance, procname);
	}
	/* Any-Thread  */ int glfwmGetPhysicalDevicePresentationSupport(VkInstance instance, VkPhysicalDevice device, uint32_t queuefamily) {
		_LESZEK_GLFWM_UNSUPPORTED_FUNCTION("glfwmGetPhysicalDevicePresentationSupport");
		return glfwGetPhysicalDevicePresentationSupport(instance, device, queuefamily);
	}
	/* Any-Thread  */ VkResult glfwmCreateWindowSurface(VkInstance instance, GLFWwindow* window, const VkAllocationCallbacks* allocator, VkSurfaceKHR* surface) {
		_LESZEK_GLFWM_UNSUPPORTED_FUNCTION("glfwmCreateWindowSurface");
		return glfwCreateWindowSurface(instance, window, allocator, surface);
	}
#endif
} // namespace Leszek::GLFWM

#else // LESZEK_GLFWM_PASSTHROUGH_MODE
namespace Leszek::GLFWM {

	void glfwm_setNewEventCallback(GLFWMneweventfun callback) {}
	void glfwm_defaultNewEventCallback() {}
	void glfwm_executeQueueTasks() {}
	void glfwm_defaultWaitForTasks(int max_ms_wait_time) {}

	// =========================================================================
	// Passthrough implementations (every glfwm function wraps its glfw counterpart)
	// =========================================================================

	/* Main-Thread */ int glfwmInit(void) { return glfwInit(); }
	/* Main-Thread */ void glfwmTerminate(void) { glfwTerminate(); }
	/* Main-Thread */ void glfwmInitHint(int hint, int value) { glfwInitHint(hint, value); }
	/* Main-Thread */ void glfwmInitAllocator(const GLFWallocator* allocator) { glfwInitAllocator(allocator); }
	/* Any-Thread  */ void glfwmGetVersion(int* major, int* minor, int* rev) { glfwGetVersion(major, minor, rev); }
	/* Any-Thread  */ const char* glfwmGetVersionString(void) { return glfwGetVersionString(); }
	/* Any-Thread  */ int glfwmGetError(const char** description) { return glfwGetError(description); }
	/* Main-Thread */ GLFWerrorfun glfwmSetErrorCallback(GLFWerrorfun callback) { return glfwSetErrorCallback(callback); }
	/* Any-Thread  */ int glfwmGetPlatform(void) { return glfwGetPlatform(); }
	/* Any-Thread  */ int glfwmPlatformSupported(int platform) { return glfwPlatformSupported(platform); }
	/* Main-Thread */ GLFWmonitor** glfwmGetMonitors(int* count) { return glfwGetMonitors(count); }
	/* Main-Thread */ GLFWmonitor* glfwmGetPrimaryMonitor(void) { return glfwGetPrimaryMonitor(); }
	/* Main-Thread */ void glfwmGetMonitorPos(GLFWmonitor* monitor, int* xpos, int* ypos) { glfwGetMonitorPos(monitor, xpos, ypos); }
	/* Main-Thread */ void glfwmGetMonitorWorkarea(GLFWmonitor* monitor, int* xpos, int* ypos, int* width, int* height) { glfwGetMonitorWorkarea(monitor, xpos, ypos, width, height); }
	/* Main-Thread */ void glfwmGetMonitorPhysicalSize(GLFWmonitor* monitor, int* widthMM, int* heightMM) { glfwGetMonitorPhysicalSize(monitor, widthMM, heightMM); }
	/* Main-Thread */ void glfwmGetMonitorContentScale(GLFWmonitor* monitor, float* xscale, float* yscale) { glfwGetMonitorContentScale(monitor, xscale, yscale); }
	/* Main-Thread */ const char* glfwmGetMonitorName(GLFWmonitor* monitor) { return glfwGetMonitorName(monitor); }
	/* Any-Thread  */ void glfwmSetMonitorUserPointer(GLFWmonitor* monitor, void* pointer) { glfwSetMonitorUserPointer(monitor, pointer); }
	/* Any-Thread  */ void* glfwmGetMonitorUserPointer(GLFWmonitor* monitor) { return glfwGetMonitorUserPointer(monitor); }
	/* Main-Thread */ GLFWmonitorfun glfwmSetMonitorCallback(GLFWmonitorfun callback) { return glfwSetMonitorCallback(callback); }
	/* Main-Thread */ const GLFWvidmode* glfwmGetVideoModes(GLFWmonitor* monitor, int* count) { return glfwGetVideoModes(monitor, count); }
	/* Main-Thread */ const GLFWvidmode* glfwmGetVideoMode(GLFWmonitor* monitor) { return glfwGetVideoMode(monitor); }
	/* Main-Thread */ void glfwmSetGamma(GLFWmonitor* monitor, float gamma) { glfwSetGamma(monitor, gamma); }
	/* Main-Thread */ const GLFWgammaramp* glfwmGetGammaRamp(GLFWmonitor* monitor) { return glfwGetGammaRamp(monitor); }
	/* Main-Thread */ void glfwmSetGammaRamp(GLFWmonitor* monitor, const GLFWgammaramp* ramp) { glfwSetGammaRamp(monitor, ramp); }
	/* Main-Thread */ void glfwmDefaultWindowHints(void) { glfwDefaultWindowHints(); }
	/* Main-Thread */ void glfwmWindowHint(int hint, int value) { glfwWindowHint(hint, value); }
	/* Main-Thread */ void glfwmWindowHintString(int hint, const char* value) { glfwWindowHintString(hint, value); }
	/* Main-Thread */ GLFWwindow* glfwmCreateWindow(int width, int height, const char* title, GLFWmonitor* monitor, GLFWwindow* share) { return glfwCreateWindow(width, height, title, monitor, share); }
	/* Main-Thread */ void glfwmDestroyWindow(GLFWwindow* window) { glfwDestroyWindow(window); }
	/* Any-Thread  */ int glfwmWindowShouldClose(GLFWwindow* window) { return glfwWindowShouldClose(window); }
	/* Any-Thread  */ void glfwmSetWindowShouldClose(GLFWwindow* window, int value) { glfwSetWindowShouldClose(window, value); }
	/* Main-Thread */ const char* glfwmGetWindowTitle(GLFWwindow* window) { return glfwGetWindowTitle(window); }
	/* Main-Thread */ void glfwmSetWindowTitle(GLFWwindow* window, const char* title) { glfwSetWindowTitle(window, title); }
	/* Main-Thread */ void glfwmSetWindowIcon(GLFWwindow* window, int count, const GLFWimage* images) { glfwSetWindowIcon(window, count, images); }
	/* Main-Thread */ void glfwmGetWindowPos(GLFWwindow* window, int* xpos, int* ypos) { glfwGetWindowPos(window, xpos, ypos); }
	/* Main-Thread */ void glfwmSetWindowPos(GLFWwindow* window, int xpos, int ypos) { glfwSetWindowPos(window, xpos, ypos); }
	/* Main-Thread */ void glfwmGetWindowSize(GLFWwindow* window, int* width, int* height) { glfwGetWindowSize(window, width, height); }
	/* Main-Thread */ void glfwmSetWindowSizeLimits(GLFWwindow* window, int minwidth, int minheight, int maxwidth, int maxheight) { glfwSetWindowSizeLimits(window, minwidth, minheight, maxwidth, maxheight); }
	/* Main-Thread */ void glfwmSetWindowAspectRatio(GLFWwindow* window, int numer, int denom) { glfwSetWindowAspectRatio(window, numer, denom); }
	/* Main-Thread */ void glfwmSetWindowSize(GLFWwindow* window, int width, int height) { glfwSetWindowSize(window, width, height); }
	/* Main-Thread */ void glfwmGetFramebufferSize(GLFWwindow* window, int* width, int* height) { glfwGetFramebufferSize(window, width, height); }
	/* Main-Thread */ void glfwmGetWindowFrameSize(GLFWwindow* window, int* left, int* top, int* right, int* bottom) { glfwGetWindowFrameSize(window, left, top, right, bottom); }
	/* Main-Thread */ void glfwmGetWindowContentScale(GLFWwindow* window, float* xscale, float* yscale) { glfwGetWindowContentScale(window, xscale, yscale); }
	/* Main-Thread */ float glfwmGetWindowOpacity(GLFWwindow* window) { return glfwGetWindowOpacity(window); }
	/* Main-Thread */ void glfwmSetWindowOpacity(GLFWwindow* window, float opacity) { glfwSetWindowOpacity(window, opacity); }
	/* Main-Thread */ void glfwmIconifyWindow(GLFWwindow* window) { glfwIconifyWindow(window); }
	/* Main-Thread */ void glfwmRestoreWindow(GLFWwindow* window) { glfwRestoreWindow(window); }
	/* Main-Thread */ void glfwmMaximizeWindow(GLFWwindow* window) { glfwMaximizeWindow(window); }
	/* Main-Thread */ void glfwmShowWindow(GLFWwindow* window) { glfwShowWindow(window); }
	/* Main-Thread */ void glfwmHideWindow(GLFWwindow* window) { glfwHideWindow(window); }
	/* Main-Thread */ void glfwmFocusWindow(GLFWwindow* window) { glfwFocusWindow(window); }
	/* Main-Thread */ void glfwmRequestWindowAttention(GLFWwindow* window) { glfwRequestWindowAttention(window); }
	/* Main-Thread */ GLFWmonitor* glfwmGetWindowMonitor(GLFWwindow* window) { return glfwGetWindowMonitor(window); }
	/* Main-Thread */ void glfwmSetWindowMonitor(GLFWwindow* window, GLFWmonitor* monitor, int xpos, int ypos, int width, int height, int refreshRate) { glfwSetWindowMonitor(window, monitor, xpos, ypos, width, height, refreshRate); }
	/* Main-Thread */ int glfwmGetWindowAttrib(GLFWwindow* window, int attrib) { return glfwGetWindowAttrib(window, attrib); }
	/* Main-Thread */ void glfwmSetWindowAttrib(GLFWwindow* window, int attrib, int value) { glfwSetWindowAttrib(window, attrib, value); }
	/* Any-Thread  */ void glfwmSetWindowUserPointer(GLFWwindow* window, void* pointer) { glfwSetWindowUserPointer(window, pointer); }
	/* Any-Thread  */ void* glfwmGetWindowUserPointer(GLFWwindow* window) { return glfwGetWindowUserPointer(window); }
	/* Main-Thread */ GLFWwindowposfun glfwmSetWindowPosCallback(GLFWwindow* window, GLFWwindowposfun callback) { return glfwSetWindowPosCallback(window, callback); }
	/* Main-Thread */ GLFWwindowsizefun glfwmSetWindowSizeCallback(GLFWwindow* window, GLFWwindowsizefun callback) { return glfwSetWindowSizeCallback(window, callback); }
	/* Main-Thread */ GLFWwindowclosefun glfwmSetWindowCloseCallback(GLFWwindow* window, GLFWwindowclosefun callback) { return glfwSetWindowCloseCallback(window, callback); }
	/* Main-Thread */ GLFWwindowrefreshfun glfwmSetWindowRefreshCallback(GLFWwindow* window, GLFWwindowrefreshfun callback) { return glfwSetWindowRefreshCallback(window, callback); }
	/* Main-Thread */ GLFWwindowfocusfun glfwmSetWindowFocusCallback(GLFWwindow* window, GLFWwindowfocusfun callback) { return glfwSetWindowFocusCallback(window, callback); }
	/* Main-Thread */ GLFWwindowiconifyfun glfwmSetWindowIconifyCallback(GLFWwindow* window, GLFWwindowiconifyfun callback) { return glfwSetWindowIconifyCallback(window, callback); }
	/* Main-Thread */ GLFWwindowmaximizefun glfwmSetWindowMaximizeCallback(GLFWwindow* window, GLFWwindowmaximizefun callback) { return glfwSetWindowMaximizeCallback(window, callback); }
	/* Main-Thread */ GLFWframebuffersizefun glfwmSetFramebufferSizeCallback(GLFWwindow* window, GLFWframebuffersizefun callback) { return glfwSetFramebufferSizeCallback(window, callback); }
	/* Main-Thread */ GLFWwindowcontentscalefun glfwmSetWindowContentScaleCallback(GLFWwindow* window, GLFWwindowcontentscalefun callback) { return glfwSetWindowContentScaleCallback(window, callback); }
	/* Main-Thread */ void glfwmPollEvents(void) { glfwPollEvents(); }
	/* Main-Thread */ void glfwmWaitEvents(void) { glfwWaitEvents(); }
	/* Main-Thread */ void glfwmWaitEventsTimeout(double timeout) { glfwWaitEventsTimeout(timeout); }
	/* Any-Thread  */ void glfwmPostEmptyEvent(void) { glfwPostEmptyEvent(); }
	/* Main-Thread */ int glfwmGetInputMode(GLFWwindow* window, int mode) { return glfwGetInputMode(window, mode); }
	/* Main-Thread */ void glfwmSetInputMode(GLFWwindow* window, int mode, int value) { glfwSetInputMode(window, mode, value); }
	/* Main-Thread */ int glfwmRawMouseMotionSupported(void) { return glfwRawMouseMotionSupported(); }
	/* Main-Thread */ const char* glfwmGetKeyName(int key, int scancode) { return glfwGetKeyName(key, scancode); }
	/* Any-Thread  */ int glfwmGetKeyScancode(int key) { return glfwGetKeyScancode(key); }
	/* Main-Thread */ int glfwmGetKey(GLFWwindow* window, int key) { return glfwGetKey(window, key); }
	/* Main-Thread */ int glfwmGetMouseButton(GLFWwindow* window, int button) { return glfwGetMouseButton(window, button); }
	/* Main-Thread */ void glfwmGetCursorPos(GLFWwindow* window, double* xpos, double* ypos) { glfwGetCursorPos(window, xpos, ypos); }
	/* Main-Thread */ void glfwmSetCursorPos(GLFWwindow* window, double xpos, double ypos) { glfwSetCursorPos(window, xpos, ypos); }
	/* Main-Thread */ GLFWcursor* glfwmCreateCursor(const GLFWimage* image, int xhot, int yhot) { return glfwCreateCursor(image, xhot, yhot); }
	/* Main-Thread */ GLFWcursor* glfwmCreateStandardCursor(int shape) { return glfwCreateStandardCursor(shape); }
	/* Main-Thread */ void glfwmDestroyCursor(GLFWcursor* cursor) { glfwDestroyCursor(cursor); }
	/* Main-Thread */ void glfwmSetCursor(GLFWwindow* window, GLFWcursor* cursor) { glfwSetCursor(window, cursor); }
	/* Main-Thread */ GLFWkeyfun glfwmSetKeyCallback(GLFWwindow* window, GLFWkeyfun callback) { return glfwSetKeyCallback(window, callback); }
	/* Main-Thread */ GLFWcharfun glfwmSetCharCallback(GLFWwindow* window, GLFWcharfun callback) { return glfwSetCharCallback(window, callback); }
	/* Main-Thread */ GLFWcharmodsfun glfwmSetCharModsCallback(GLFWwindow* window, GLFWcharmodsfun callback) { return glfwSetCharModsCallback(window, callback); }
	/* Main-Thread */ GLFWmousebuttonfun glfwmSetMouseButtonCallback(GLFWwindow* window, GLFWmousebuttonfun callback) { return glfwSetMouseButtonCallback(window, callback); }
	/* Main-Thread */ GLFWcursorposfun glfwmSetCursorPosCallback(GLFWwindow* window, GLFWcursorposfun callback) { return glfwSetCursorPosCallback(window, callback); }
	/* Main-Thread */ GLFWcursorenterfun glfwmSetCursorEnterCallback(GLFWwindow* window, GLFWcursorenterfun callback) { return glfwSetCursorEnterCallback(window, callback); }
	/* Main-Thread */ GLFWscrollfun glfwmSetScrollCallback(GLFWwindow* window, GLFWscrollfun callback) { return glfwSetScrollCallback(window, callback); }
	/* Main-Thread */ GLFWdropfun glfwmSetDropCallback(GLFWwindow* window, GLFWdropfun callback) { return glfwSetDropCallback(window, callback); }
	/* Main-Thread */ int glfwmJoystickPresent(int jid) { return glfwJoystickPresent(jid); }
	/* Main-Thread */ const float* glfwmGetJoystickAxes(int jid, int* count) { return glfwGetJoystickAxes(jid, count); }
	/* Main-Thread */ const unsigned char* glfwmGetJoystickButtons(int jid, int* count) { return glfwGetJoystickButtons(jid, count); }
	/* Main-Thread */ const unsigned char* glfwmGetJoystickHats(int jid, int* count) { return glfwGetJoystickHats(jid, count); }
	/* Main-Thread */ const char* glfwmGetJoystickName(int jid) { return glfwGetJoystickName(jid); }
	/* Main-Thread */ const char* glfwmGetJoystickGUID(int jid) { return glfwGetJoystickGUID(jid); }
	/* Any-Thread  */ void glfwmSetJoystickUserPointer(int jid, void* pointer) { glfwSetJoystickUserPointer(jid, pointer); }
	/* Any-Thread  */ void* glfwmGetJoystickUserPointer(int jid) { return glfwGetJoystickUserPointer(jid); }
	/* Main-Thread */ int glfwmJoystickIsGamepad(int jid) { return glfwJoystickIsGamepad(jid); }
	/* Main-Thread */ GLFWjoystickfun glfwmSetJoystickCallback(GLFWjoystickfun callback) { return glfwSetJoystickCallback(callback); }
	/* Main-Thread */ int glfwmUpdateGamepadMappings(const char* string) { return glfwUpdateGamepadMappings(string); }
	/* Main-Thread */ const char* glfwmGetGamepadName(int jid) { return glfwGetGamepadName(jid); }
	/* Main-Thread */ int glfwmGetGamepadState(int jid, GLFWgamepadstate* state) { return glfwGetGamepadState(jid, state); }
	/* Main-Thread */ void glfwmSetClipboardString(GLFWwindow* window, const char* string) { glfwSetClipboardString(window, string); }
	/* Main-Thread */ const char* glfwmGetClipboardString(GLFWwindow* window) { return glfwGetClipboardString(window); }
	/* Any-Thread  */ double glfwmGetTime(void) { return glfwGetTime(); }
	/* Any-Thread  */ void glfwmSetTime(double time) { glfwSetTime(time); }
	/* Any-Thread  */ uint64_t glfwmGetTimerValue(void) { return glfwGetTimerValue(); }
	/* Any-Thread  */ uint64_t glfwmGetTimerFrequency(void) { return glfwGetTimerFrequency(); }
	/* Any-Thread  */ void glfwmMakeContextCurrent(GLFWwindow* window) { glfwMakeContextCurrent(window); }
	/* Any-Thread  */ GLFWwindow* glfwmGetCurrentContext(void) { return glfwGetCurrentContext(); }
	/* Any-Thread  */ void glfwmSwapBuffers(GLFWwindow* window) { glfwSwapBuffers(window); }
	/* Any-Thread  */ void glfwmSwapInterval(int interval) { glfwSwapInterval(interval); }
	/* Any-Thread  */ int glfwmExtensionSupported(const char* extension) { return glfwExtensionSupported(extension); }
	/* Any-Thread  */ GLFWglproc glfwmGetProcAddress(const char* procname) { return glfwGetProcAddress(procname); }
	/* Any-Thread  */ int glfwmVulkanSupported(void) { return glfwVulkanSupported(); }
	/* Any-Thread  */ const char** glfwmGetRequiredInstanceExtensions(uint32_t* count) { return glfwGetRequiredInstanceExtensions(count); }
	/* Any-Thread  */ GLFWvkproc glfwmGetInstanceProcAddress(VkInstance instance, const char* procname) { return glfwGetInstanceProcAddress(instance, procname); }
#if defined(VK_VERSION_1_0) || defined(GLFW_INCLUDE_VULKAN)
	/* Any-Thread  */ int glfwmGetPhysicalDevicePresentationSupport(VkInstance instance, VkPhysicalDevice device, uint32_t queuefamily) { return glfwGetPhysicalDevicePresentationSupport(instance, device, queuefamily); }
#endif
	/* Any-Thread  */ VkResult glfwmCreateWindowSurface(VkInstance instance, GLFWwindow* window, const VkAllocationCallbacks* allocator, VkSurfaceKHR* surface) { return glfwCreateWindowSurface(instance, window, allocator, surface); }

} // namespace Leszek::GLFWM
#endif // LESZEK_GLFWM_PASSTHROUGH_MODE
#endif // LESZEK_GLFWM_IMPLEMENTATION

#ifdef LESZEK_GLFWM_FORBID_GLFW

#define glfwInit								 glfwm_USE_glfwmInit_INSTEAD
#define glfwTerminate							 glfwm_USE_glfwmTerminate_INSTEAD
#define glfwInitHint							 glfwm_USE_glfwmInitHint_INSTEAD
#define glfwInitAllocator						 glfwm_USE_glfwmInitAllocator_INSTEAD
#define glfwGetVersion							 glfwm_USE_glfwmGetVersion_INSTEAD
#define glfwGetVersionString					 glfwm_USE_glfwmGetVersionString_INSTEAD
#define glfwGetError							 glfwm_USE_glfwmGetError_INSTEAD
#define glfwSetErrorCallback					 glfwm_USE_glfwmSetErrorCallback_INSTEAD
#define glfwGetPlatform							 glfwm_USE_glfwmGetPlatform_INSTEAD
#define glfwPlatformSupported					 glfwm_USE_glfwmPlatformSupported_INSTEAD
#define glfwGetMonitors							 glfwm_USE_glfwmGetMonitors_INSTEAD
#define glfwGetPrimaryMonitor					 glfwm_USE_glfwmGetPrimaryMonitor_INSTEAD
#define glfwGetMonitorPos						 glfwm_USE_glfwmGetMonitorPos_INSTEAD
#define glfwGetMonitorWorkarea					 glfwm_USE_glfwmGetMonitorWorkarea_INSTEAD
#define glfwGetMonitorPhysicalSize				 glfwm_USE_glfwmGetMonitorPhysicalSize_INSTEAD
#define glfwGetMonitorContentScale				 glfwm_USE_glfwmGetMonitorContentScale_INSTEAD
#define glfwGetMonitorName						 glfwm_USE_glfwmGetMonitorName_INSTEAD
#define glfwSetMonitorUserPointer				 glfwm_USE_glfwmSetMonitorUserPointer_INSTEAD
#define glfwGetMonitorUserPointer				 glfwm_USE_glfwmGetMonitorUserPointer_INSTEAD
#define glfwSetMonitorCallback					 glfwm_USE_glfwmSetMonitorCallback_INSTEAD
#define glfwGetVideoModes						 glfwm_USE_glfwmGetVideoModes_INSTEAD
#define glfwGetVideoMode						 glfwm_USE_glfwmGetVideoMode_INSTEAD
#define glfwSetGamma							 glfwm_USE_glfwmSetGamma_INSTEAD
#define glfwGetGammaRamp						 glfwm_USE_glfwmGetGammaRamp_INSTEAD
#define glfwSetGammaRamp						 glfwm_USE_glfwmSetGammaRamp_INSTEAD
#define glfwDefaultWindowHints					 glfwm_USE_glfwmDefaultWindowHints_INSTEAD
#define glfwWindowHint							 glfwm_USE_glfwmWindowHint_INSTEAD
#define glfwWindowHintString					 glfwm_USE_glfwmWindowHintString_INSTEAD
#define glfwCreateWindow						 glfwm_USE_glfwmCreateWindow_INSTEAD
#define glfwDestroyWindow						 glfwm_USE_glfwmDestroyWindow_INSTEAD
#define glfwWindowShouldClose					 glfwm_USE_glfwmWindowShouldClose_INSTEAD
#define glfwSetWindowShouldClose				 glfwm_USE_glfwmSetWindowShouldClose_INSTEAD
#define glfwGetWindowTitle						 glfwm_USE_glfwmGetWindowTitle_INSTEAD
#define glfwSetWindowTitle						 glfwm_USE_glfwmSetWindowTitle_INSTEAD
#define glfwSetWindowIcon						 glfwm_USE_glfwmSetWindowIcon_INSTEAD
#define glfwGetWindowPos						 glfwm_USE_glfwmGetWindowPos_INSTEAD
#define glfwSetWindowPos						 glfwm_USE_glfwmSetWindowPos_INSTEAD
#define glfwGetWindowSize						 glfwm_USE_glfwmGetWindowSize_INSTEAD
#define glfwSetWindowSizeLimits					 glfwm_USE_glfwmSetWindowSizeLimits_INSTEAD
#define glfwSetWindowAspectRatio				 glfwm_USE_glfwmSetWindowAspectRatio_INSTEAD
#define glfwSetWindowSize						 glfwm_USE_glfwmSetWindowSize_INSTEAD
#define glfwGetFramebufferSize					 glfwm_USE_glfwmGetFramebufferSize_INSTEAD
#define glfwGetWindowFrameSize					 glfwm_USE_glfwmGetWindowFrameSize_INSTEAD
#define glfwGetWindowContentScale				 glfwm_USE_glfwmGetWindowContentScale_INSTEAD
#define glfwGetWindowOpacity					 glfwm_USE_glfwmGetWindowOpacity_INSTEAD
#define glfwSetWindowOpacity					 glfwm_USE_glfwmSetWindowOpacity_INSTEAD
#define glfwIconifyWindow						 glfwm_USE_glfwmIconifyWindow_INSTEAD
#define glfwRestoreWindow						 glfwm_USE_glfwmRestoreWindow_INSTEAD
#define glfwMaximizeWindow						 glfwm_USE_glfwmMaximizeWindow_INSTEAD
#define glfwShowWindow							 glfwm_USE_glfwmShowWindow_INSTEAD
#define glfwHideWindow							 glfwm_USE_glfwmHideWindow_INSTEAD
#define glfwFocusWindow							 glfwm_USE_glfwmFocusWindow_INSTEAD
#define glfwRequestWindowAttention				 glfwm_USE_glfwmRequestWindowAttention_INSTEAD
#define glfwGetWindowMonitor					 glfwm_USE_glfwmGetWindowMonitor_INSTEAD
#define glfwSetWindowMonitor					 glfwm_USE_glfwmSetWindowMonitor_INSTEAD
#define glfwGetWindowAttrib						 glfwm_USE_glfwmGetWindowAttrib_INSTEAD
#define glfwSetWindowAttrib						 glfwm_USE_glfwmSetWindowAttrib_INSTEAD
#define glfwSetWindowUserPointer				 glfwm_USE_glfwmSetWindowUserPointer_INSTEAD
#define glfwGetWindowUserPointer				 glfwm_USE_glfwmGetWindowUserPointer_INSTEAD
#define glfwSetWindowPosCallback				 glfwm_USE_glfwmSetWindowPosCallback_INSTEAD
#define glfwSetWindowSizeCallback				 glfwm_USE_glfwmSetWindowSizeCallback_INSTEAD
#define glfwSetWindowCloseCallback				 glfwm_USE_glfwmSetWindowCloseCallback_INSTEAD
#define glfwSetWindowRefreshCallback			 glfwm_USE_glfwmSetWindowRefreshCallback_INSTEAD
#define glfwSetWindowFocusCallback				 glfwm_USE_glfwmSetWindowFocusCallback_INSTEAD
#define glfwSetWindowIconifyCallback			 glfwm_USE_glfwmSetWindowIconifyCallback_INSTEAD
#define glfwSetWindowMaximizeCallback			 glfwm_USE_glfwmSetWindowMaximizeCallback_INSTEAD
#define glfwSetFramebufferSizeCallback			 glfwm_USE_glfwmSetFramebufferSizeCallback_INSTEAD
#define glfwSetWindowContentScaleCallback		 glfwm_USE_glfwmSetWindowContentScaleCallback_INSTEAD
#define glfwPollEvents							 glfwm_USE_glfwmPollEvents_INSTEAD
#define glfwWaitEvents							 glfwm_USE_glfwmWaitEvents_INSTEAD
#define glfwWaitEventsTimeout					 glfwm_USE_glfwmWaitEventsTimeout_INSTEAD
#define glfwPostEmptyEvent						 glfwm_USE_glfwmPostEmptyEvent_INSTEAD
#define glfwGetInputMode						 glfwm_USE_glfwmGetInputMode_INSTEAD
#define glfwSetInputMode						 glfwm_USE_glfwmSetInputMode_INSTEAD
#define glfwRawMouseMotionSupported				 glfwm_USE_glfwmRawMouseMotionSupported_INSTEAD
#define glfwGetKeyName							 glfwm_USE_glfwmGetKeyName_INSTEAD
#define glfwGetKeyScancode						 glfwm_USE_glfwmGetKeyScancode_INSTEAD
#define glfwGetKey								 glfwm_USE_glfwmGetKey_INSTEAD
#define glfwGetMouseButton						 glfwm_USE_glfwmGetMouseButton_INSTEAD
#define glfwGetCursorPos						 glfwm_USE_glfwmGetCursorPos_INSTEAD
#define glfwSetCursorPos						 glfwm_USE_glfwmSetCursorPos_INSTEAD
#define glfwCreateCursor						 glfwm_USE_glfwmCreateCursor_INSTEAD
#define glfwCreateStandardCursor				 glfwm_USE_glfwmCreateStandardCursor_INSTEAD
#define glfwDestroyCursor						 glfwm_USE_glfwmDestroyCursor_INSTEAD
#define glfwSetCursor							 glfwm_USE_glfwmSetCursor_INSTEAD
#define glfwSetKeyCallback						 glfwm_USE_glfwmSetKeyCallback_INSTEAD
#define glfwSetCharCallback						 glfwm_USE_glfwmSetCharCallback_INSTEAD
#define glfwSetCharModsCallback					 glfwm_USE_glfwmSetCharModsCallback_INSTEAD
#define glfwSetMouseButtonCallback				 glfwm_USE_glfwmSetMouseButtonCallback_INSTEAD
#define glfwSetCursorPosCallback				 glfwm_USE_glfwmSetCursorPosCallback_INSTEAD
#define glfwSetCursorEnterCallback				 glfwm_USE_glfwmSetCursorEnterCallback_INSTEAD
#define glfwSetScrollCallback					 glfwm_USE_glfwmSetScrollCallback_INSTEAD
#define glfwSetDropCallback						 glfwm_USE_glfwmSetDropCallback_INSTEAD
#define glfwJoystickPresent						 glfwm_USE_glfwmJoystickPresent_INSTEAD
#define glfwGetJoystickAxes						 glfwm_USE_glfwmGetJoystickAxes_INSTEAD
#define glfwGetJoystickButtons					 glfwm_USE_glfwmGetJoystickButtons_INSTEAD
#define glfwGetJoystickHats						 glfwm_USE_glfwmGetJoystickHats_INSTEAD
#define glfwGetJoystickName						 glfwm_USE_glfwmGetJoystickName_INSTEAD
#define glfwGetJoystickGUID						 glfwm_USE_glfwmGetJoystickGUID_INSTEAD
#define glfwSetJoystickUserPointer				 glfwm_USE_glfwmSetJoystickUserPointer_INSTEAD
#define glfwGetJoystickUserPointer				 glfwm_USE_glfwmGetJoystickUserPointer_INSTEAD
#define glfwJoystickIsGamepad					 glfwm_USE_glfwmJoystickIsGamepad_INSTEAD
#define glfwSetJoystickCallback					 glfwm_USE_glfwmSetJoystickCallback_INSTEAD
#define glfwUpdateGamepadMappings				 glfwm_USE_glfwmUpdateGamepadMappings_INSTEAD
#define glfwGetGamepadName						 glfwm_USE_glfwmGetGamepadName_INSTEAD
#define glfwGetGamepadState						 glfwm_USE_glfwmGetGamepadState_INSTEAD
#define glfwSetClipboardString					 glfwm_USE_glfwmSetClipboardString_INSTEAD
#define glfwGetClipboardString					 glfwm_USE_glfwmGetClipboardString_INSTEAD
#define glfwGetTime								 glfwm_USE_glfwmGetTime_INSTEAD
#define glfwSetTime								 glfwm_USE_glfwmSetTime_INSTEAD
#define glfwGetTimerValue						 glfwm_USE_glfwmGetTimerValue_INSTEAD
#define glfwGetTimerFrequency					 glfwm_USE_glfwmGetTimerFrequency_INSTEAD
#define glfwMakeContextCurrent					 glfwm_USE_glfwmMakeContextCurrent_INSTEAD
#define glfwGetCurrentContext					 glfwm_USE_glfwmGetCurrentContext_INSTEAD
#define glfwSwapBuffers							 glfwm_USE_glfwmSwapBuffers_INSTEAD
#define glfwSwapInterval						 glfwm_USE_glfwmSwapInterval_INSTEAD
#define glfwExtensionSupported					 glfwm_USE_glfwmExtensionSupported_INSTEAD
#define glfwGetProcAddress						 glfwm_USE_glfwmGetProcAddress_INSTEAD
#define glfwVulkanSupported						 glfwm_USE_glfwmVulkanSupported_INSTEAD
#define glfwGetRequiredInstanceExtensions		 glfwm_USE_glfwmGetRequiredInstanceExtensions_INSTEAD
#if defined(VK_VERSION_1_0) || defined(GLFW_INCLUDE_VULKAN)
#define glfwGetInstanceProcAddress				 glfwm_USE_glfwmGetInstanceProcAddress_INSTEAD
#define glfwGetPhysicalDevicePresentationSupport glfwm_USE_glfwmGetPhysicalDevicePresentationSupport_INSTEAD
#define glfwCreateWindowSurface					 glfwm_USE_glfwmCreateWindowSurface_INSTEAD
#endif

#endif // LESZEK_GLFWM_FORBID_GLFW

/*\
 * ======================================================================================
 *  License:
 * ======================================================================================
 *  Copyright 2026 @LeszekDev
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 * ======================================================================================
\*/