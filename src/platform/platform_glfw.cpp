
#include "platform.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#define MA_NO_DECODING
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio/miniaudio.h"

#include <cstdio>
#include <vector>

namespace {
  struct PlayingAudio : AudioBuffer {
    int position{ };
  };

  GLFWwindow* g_window;
  std::vector<PlayingAudio> g_playing_audio;

  void set_fullscreen(bool fullscreen) {
    auto monitor = glfwGetPrimaryMonitor();
    const auto video_mode = glfwGetVideoMode(monitor);
    auto x = 0, y = 0, w = video_mode->width, h = video_mode->height;
    if (!fullscreen) {
      x += (w - platform_initial_width) / 2;
      y += (h - platform_initial_height) / 2;
      w = platform_initial_width;
      h = platform_initial_height;
      monitor = nullptr;
    }
    glfwSetWindowMonitor(g_window, monitor, x, y, w, h, video_mode->refreshRate);
    if (!fullscreen)
      glfwRestoreWindow(g_window);
  }

  void error_callback(int, const char* message) {
    platform_error(message);
  }

  void maximize_callback(GLFWwindow*, int maximized) {
    set_fullscreen(maximized == GLFW_TRUE);
  }

  void key_callback(GLFWwindow*, int key, int, int action, int) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
      if (glfwGetWindowAttrib(g_window, GLFW_MAXIMIZED))
        set_fullscreen(false);

    platform_on_key(static_cast<KeyCode>(key), (action != GLFW_RELEASE));
  }

  void text_callback(GLFWwindow*, unsigned int code) {
    platform_on_text(code >= ' ' && code <= '~' ? static_cast<char>(code) : '?');
  }

  void mouse_button_callback(GLFWwindow*, int button, int action, int) {
    platform_on_mouse_button(button, action == GLFW_PRESS);
  }

  void mouse_move_callback(GLFWwindow*, double x, double y) {
    platform_on_mouse_move(static_cast<int>(x), static_cast<int>(y));
  }

  void mouse_wheel_callback(GLFWwindow*, double deltax, double deltay) {
    platform_on_mouse_wheel(deltax, deltay);
  }

  void audio_callback(ma_device*, void* output_frames_, const void*, ma_uint32 frame_count_) {
    const auto output_frames = static_cast<float*>(output_frames_);
    const auto frame_count = static_cast<int>(frame_count_);

    if (output_frames && frame_count) {
      platform_music_callback(output_frames, frame_count);

      for (auto it = begin(g_playing_audio); it != end(g_playing_audio); ) {
        const auto sample_count =
          std::min(it->sample_count - it->position, frame_count);

        for (auto i = 0; i < 2 * sample_count; ) {
          const auto sample = (it->samples.get())[it->position++];
          output_frames[i++] += sample;
          output_frames[i++] += sample;
        }

        if (it->position == it->sample_count)
          it = g_playing_audio.erase(it);
        else
          ++it;
      }
    }
  }
} // namespace

void platform_error(const char* message) {
  std::fprintf(stderr, "%s", message);
  std::abort();
}

void platform_play_audio(AudioBuffer buffer) {
  g_playing_audio.push_back({ std::move(buffer) });
}

#if !defined(_WIN32)
int main() {
#else
int APIENTRY WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
#endif
  glfwSetErrorCallback(error_callback);

  if (!glfwInit())
    return EXIT_FAILURE;

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

  g_window = glfwCreateWindow(
    platform_initial_width, platform_initial_height,
    platform_window_title, NULL, NULL);
  if (!g_window) {
    glfwTerminate();
    return EXIT_FAILURE;
  }

  set_fullscreen(false);
  glfwSetWindowSizeLimits(g_window,
    platform_minimum_width, platform_minimum_height,
    GLFW_DONT_CARE, GLFW_DONT_CARE);
  glfwSetWindowAspectRatio(g_window,
    platform_minimum_width, platform_minimum_height);

  glfwSetWindowMaximizeCallback(g_window, maximize_callback);
  glfwSetKeyCallback(g_window, key_callback);
  glfwSetCharCallback(g_window, text_callback);
  glfwSetMouseButtonCallback(g_window, mouse_button_callback);
  glfwSetCursorPosCallback(g_window, mouse_move_callback);
  glfwSetScrollCallback(g_window, mouse_wheel_callback);

  glfwMakeContextCurrent(g_window);
#if defined(_WIN32)
  gladLoadGL(glfwGetProcAddress);
#endif
  glfwSwapInterval(1);

  auto audio_device_config = ma_device_config_init(ma_device_type_playback);
  audio_device_config.playback.format   = ma_format_f32;
  audio_device_config.playback.channels = 2;
  audio_device_config.sampleRate = 44100;
  audio_device_config.dataCallback = audio_callback;

  auto audio_device = ma_device{ };
  if (ma_device_init(nullptr, &audio_device_config, &audio_device) != MA_SUCCESS)
    return EXIT_FAILURE;

  if (ma_device_start(&audio_device) != MA_SUCCESS)
    return EXIT_FAILURE;

  platform_on_setup();

  while (!glfwWindowShouldClose(g_window)) {
    auto width = 0, height = 0;
    glfwGetWindowSize(g_window, &width, &height),
    platform_on_update(width, height, glfwGetTime());

    glfwSwapBuffers(g_window);
    glfwPollEvents();
  }

  ma_device_uninit(&audio_device);

  glfwDestroyWindow(g_window);

  glfwTerminate();
  return EXIT_SUCCESS;
}
