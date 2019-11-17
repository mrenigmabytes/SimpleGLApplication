// %BANNER_BEGIN%
// ---------------------------------------------------------------------
// %COPYRIGHT_BEGIN%
//
// Copyright (c) 2018 Magic Leap, Inc. All Rights Reserved.
// Use of this file is governed by the Creator Agreement, located
// here: https://id.magicleap.com/creator-terms
//
// %COPYRIGHT_END%
// ---------------------------------------------------------------------
// %BANNER_END%

#include "SimpleGLApplication.h"

#include <stdio.h>
#include <stdlib.h>

#include <chrono>
#include <cmath>

#if USE_GLFW
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#else
#ifndef EGL_EGLEXT_PROTOTYPES
#define EGL_EGLEXT_PROTOTYPES
#endif

#include <EGL/egl.h>
#include <EGL/eglext.h>

#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES
#endif

#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#endif

#include <ml_graphics.h>
#include <ml_head_tracking.h>
#include <ml_perception.h>
#include <ml_lifecycle.h>
#include <ml_logging.h>

// Constants
const char application_name[] = "com.magicleap.simpleglapp";

// Structures
struct application_context_t {
	int dummy_value;
};

struct graphics_context_t {

#if USE_GLFW
	GLFWwindow* window;
#else
	EGLDisplay egl_display;
	EGLContext egl_context;
#endif

	GLuint framebuffer_id;
	GLuint vertex_shader_id;
	GLuint fragment_shader_id;
	GLuint program_id;

	graphics_context_t();
	~graphics_context_t();

	void makeCurrent();
	void swapBuffers();
	void unmakeCurrent();
};

#if USE_GLFW

graphics_context_t::graphics_context_t() {
	if (!glfwInit()) {
		ML_LOG(Error, "glfwInit() failed");
		exit(1);
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
#if ML_OSX
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
#else
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
#endif
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

	// We open a 1x1 window here -- this is not where the action happens, however;
	// this program renders to a texture.  This is done merely to make GL happy.
	window = glfwCreateWindow(1, 1, "rendering test", NULL, NULL);
	if (!window) {
		ML_LOG(Error, "glfwCreateWindow() failed");
		exit(1);
	}

	glfwMakeContextCurrent(window);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		ML_LOG(Error, "gladLoadGLLoader() failed");
		exit(1);
	}
}

void graphics_context_t::makeCurrent() {
	glfwMakeContextCurrent(window);
}

void graphics_context_t::unmakeCurrent() {
}

void graphics_context_t::swapBuffers() {
	glfwSwapBuffers(window);
}

graphics_context_t::~graphics_context_t() {
	glfwDestroyWindow(window);
	window = nullptr;
}

#else // !USE_GLFW

graphics_context_t::graphics_context_t() {
	egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

	EGLint major = 4;
	EGLint minor = 0;
	eglInitialize(egl_display, &major, &minor);
	eglBindAPI(EGL_OPENGL_API);

	EGLint config_attribs[] = {
	  EGL_RED_SIZE, 5,
	  EGL_GREEN_SIZE, 6,
	  EGL_BLUE_SIZE, 5,
	  EGL_ALPHA_SIZE, 0,
	  EGL_DEPTH_SIZE, 24,
	  EGL_STENCIL_SIZE, 8,
	  EGL_NONE
	};
	EGLConfig egl_config = nullptr;
	EGLint config_size = 0;
	eglChooseConfig(egl_display, config_attribs, &egl_config, 1, &config_size);

	EGLint context_attribs[] = {
	  EGL_CONTEXT_MAJOR_VERSION_KHR, 3,
	  EGL_CONTEXT_MINOR_VERSION_KHR, 0,
	  EGL_NONE
	};
	egl_context = eglCreateContext(egl_display, egl_config, EGL_NO_CONTEXT, context_attribs);
}

void graphics_context_t::makeCurrent() {
	eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, egl_context);
}

void graphics_context_t::unmakeCurrent() {
	eglMakeCurrent(NULL, EGL_NO_SURFACE, EGL_NO_SURFACE, NULL);
}

void graphics_context_t::swapBuffers() {
	// buffer swapping is implicit on device (MLGraphicsEndFrame)
}

graphics_context_t::~graphics_context_t() {
	eglDestroyContext(egl_display, egl_context);
	eglTerminate(egl_display);
}

#endif // USE_GLFW

// Callbacks
static void onStop(void* application_context)
{
	((struct application_context_t*)application_context)->dummy_value = 0;
	ML_LOG(Info, "%s: On stop called.", application_name);
}

static void onPause(void* application_context)
{
	((struct application_context_t*)application_context)->dummy_value = 1;
	ML_LOG(Info, "%s: On pause called.", application_name);
}

static void onResume(void* application_context)
{
	((struct application_context_t*)application_context)->dummy_value = 2;
	ML_LOG(Info, "%s: On resume called.", application_name);
}

int main() {
	// set up host-specific graphics surface
	graphics_context_t graphics_context;

	// let system know our app has started
	MLLifecycleCallbacksEx lifecycle_callbacks = {};
	lifecycle_callbacks.on_stop = onStop;
	lifecycle_callbacks.on_pause = onPause;
	lifecycle_callbacks.on_resume = onResume;

	struct application_context_t application_context;
	application_context.dummy_value = 2;

	
	//MLLifecycleErrorCode lifecycle_status = MLLifecycleInit(&lifecycle_callbacks, (void*)&application_context);
	MLResult lifecycle_status = MLLifecycleInitEx(&lifecycle_callbacks, (void*)&application_context);
	if (lifecycle_status != MLResult_Ok) {
		ML_LOG(Error, "%s: Failed to initialize lifecycle.", application_name);
		return -1;
	}

	// initialize perception system
	MLPerceptionSettings perception_settings;

	MLResult perception_result =  MLPerceptionInitSettings(&perception_settings);
	if (perception_result != MLResult_Ok) {
		ML_LOG(Error, "%s: Failed to initialize perception.", application_name);
	}

	MLResult perception_startup_status = MLPerceptionStartup(&perception_settings);
	if (perception_startup_status != MLResult_Ok) {
		ML_LOG(Error, "%s: Failed to startup perception.", application_name);
	}

	// Get ready to connect our GL context to the MLSDK graphics API
	graphics_context.makeCurrent();
	glGenFramebuffers(1, &graphics_context.framebuffer_id);

	MLGraphicsOptions graphics_options = { 0, MLSurfaceFormat_RGBA8UNorm, MLSurfaceFormat_D32Float };
#if USE_GLFW
	MLHandle opengl_context = reinterpret_cast<MLHandle>(glfwGetCurrentContext());
#else
	MLHandle opengl_context = reinterpret_cast<MLHandle>(graphics_context.egl_context);
#endif
	MLHandle graphics_client = ML_INVALID_HANDLE;
	//MLResult graphics_create_status = 
	if (MLGraphicsCreateClientGL(&graphics_options, opengl_context, &graphics_client) != MLResult_Ok)
		ML_LOG(Error, "%s: Failed to create graphics client.", application_name);

	
	// Now that graphics is connected, the app is ready to go
	if (MLLifecycleSetReadyIndication() != MLResult_Ok) {
		ML_LOG(Error, "%s: Failed to indicate lifecycle ready.", application_name);
		return -1;
	}

	MLHandle head_tracker;
	MLHeadTrackingStaticData head_static_data;
	MLResult track_result = MLHeadTrackingCreate(&head_tracker);

	if (MLHandleIsValid(head_tracker) && track_result == MLResult_Ok) {
		MLHeadTrackingGetStaticData(head_tracker, &head_static_data);
	}
	else {
		ML_LOG(Error, "%s: Failed to create head tracker.", application_name);
	}

	ML_LOG(Info, "%s: Start loop.", application_name);

	auto start = std::chrono::steady_clock::now();

	while (application_context.dummy_value) {
		/*MLResult out_result;
		if (!MLGraphicsFrameParamsExInit(&frame_params) = MLResult_Ok) {
			ML_LOG(Error, "MLGraphicsBeginFrame complained: %d", out_status);
		}*/
		MLGraphicsFrameParamsEx frame_params;
		MLGraphicsFrameParamsExInit(&frame_params);

		frame_params.surface_scale = 1.0f;
		frame_params.projection_type = MLGraphicsProjectionType_ReversedInfiniteZ;
		frame_params.near_clip = 1.0f;
		frame_params.focus_distance = 1.0f;

		MLGraphicsFrameInfo out_frame_info;
		MLGraphicsFrameInfoInit(&out_frame_info);

		MLResult out_result = MLGraphicsBeginFrameEx(graphics_client, &frame_params, &out_frame_info);
		if(out_result != MLResult_Ok){
			ML_LOG(Error, "MLGraphicsBeginFrameEx complained: %d", out_result);
		}

		auto msRuntime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
		auto factor = labs(msRuntime % 2000 - 1000) / 1000.0;

		for (int camera = 0; camera < 2; ++camera) {
			glBindFramebuffer(GL_FRAMEBUFFER, graphics_context.framebuffer_id);
			glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, out_frame_info.virtual_camera_info_array.color_id, 0, camera);
			glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, out_frame_info.virtual_camera_info_array.depth_id, 0, camera);

			const MLRectf& viewport = out_frame_info.virtual_camera_info_array.viewport;
			glViewport((GLint)viewport.x, (GLint)viewport.y, (GLsizei)viewport.w, (GLsizei)viewport.h);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				if (camera == 0) {
					glClearColor(1.0 - factor, 0.0, 0.0, 0.0);
				}
				else {
					glClearColor(0.0, 0.0, factor, 0.0);
				}
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				MLGraphicsSignalSyncObjectGL(graphics_client, out_frame_info.virtual_camera_info_array.virtual_cameras[camera].sync_object);
		}
		out_result = MLGraphicsEndFrame(graphics_client, out_frame_info.handle);
		if (out_result != MLResult_Ok) {
			ML_LOG(Error, "MLGraphicsEndFrame complained: %d", out_result);
		}

		graphics_context.swapBuffers();
	}

	ML_LOG(Info, "%s: End loop.", application_name);

	graphics_context.unmakeCurrent();

	glDeleteFramebuffers(1, &graphics_context.framebuffer_id);

	// clean up system
	MLResult graphics_destroy_status = MLGraphicsDestroyClient(&graphics_client);
	if (graphics_destroy_status != MLResult_Ok) {
		ML_LOG(Error, "%s: Could not Destroy Graphics Client.", graphics_destroy_status);
		return 1;
	}
	//MLGraphicsDestroyClient(&graphics_client, &graphics_destroy_status);
	MLPerceptionShutdown();

	return 0;
}
