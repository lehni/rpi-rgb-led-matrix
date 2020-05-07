// -*- mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; -*-

#include "led-matrix.h"
#include "threaded-canvas-manipulator.h"
#include "pixel-mapper.h"
#include "graphics.h"

#include <assert.h>
#include <getopt.h>
#include <limits.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <math.h>

#include <cstdlib>
#include <iostream>
#include <string>

using namespace rgb_matrix;

enum RGBLedCommand {
  LED_COMMAND_SET_BRIGHTNESS = 1,
  LED_COMMAND_SET_GAMMA = 2,
  LED_COMMAND_CLEAR = 3,
  LED_COMMAND_FILL = 4,
  LED_COMMAND_SET_PIXELS = 5
};

volatile bool interrupt_received = false;
static void InterruptHandler(int signo) {
  interrupt_received = true;
}

class RGBLedInterface : public ThreadedCanvasManipulator {
public:
  RGBLedInterface(RGBMatrix *m) : ThreadedCanvasManipulator(m), matrix(m) {
  	canvas = m->CreateFrameCanvas();
    for (int i = 0; i <= 255; i++) {
      gamma8[i] = i;
    }
  }

  void Run() {
    while (running() && !interrupt_received) {
      uint8_t command;
      bool handled = false;
      if (ReadByte(&command)) {
        switch (command) {
        case LED_COMMAND_SET_BRIGHTNESS:
          handled = SetBrightness();
          break;
        case LED_COMMAND_SET_GAMMA:
          handled = SetGamma();
          break;
        case LED_COMMAND_SET_PIXELS:
          handled = SetPixels();
          break;
        case LED_COMMAND_FILL:
          handled = Fill();
          break;
        case LED_COMMAND_CLEAR:
          handled = Clear();
          break;
        default:
          std::cerr << "ERROR: Received unsupported command: " << (int) command << std::endl;
          continue;
        }
        if (!handled) {
          std::cerr << "ERROR: Command not handled: " << (int) command << std::endl;
        }
      }
    }
  }

private:
  RGBMatrix *const matrix;
  FrameCanvas *canvas;
  uint8_t gamma8[256];

  bool SetBrightness() {
    uint8_t brightness;
    if (ReadByte(&brightness)) {
      matrix->SetBrightness(brightness);
      return true;
    }
    return false;
  }

  bool SetGamma() {
    float gamma;
    if (ReadFloat(&gamma)) {
      for (int i = 0; i <= 255; i++) {
        gamma8[i] = (int)(pow((float)i / 255.0, gamma) * 255.0 + 0.5);
      }
      return true;
    }
    return false;
  }



  bool Clear() {
    canvas->Clear();
    canvas = matrix->SwapOnVSync(canvas);
    return true;
  }

  bool Fill() {
    uint8_t r, g, b;
    if (ReadByte(&r) && ReadByte(&g) && ReadByte(&b)) {
      canvas->Fill(r, g, b);
      canvas = matrix->SwapOnVSync(canvas);
      return true;
    }
    return false;
  }

  bool SetPixels() {
    uint16_t width, height;
    uint8_t bytesPerPixel;
    if (ReadShort(&width) && ReadShort(&height) && ReadByte(&bytesPerPixel)) {
      uint32_t size = width * height * bytesPerPixel;
      uint8_t buffer[size];
      if (ReadBuffer(buffer, size)) {
        int i = 0;
        for(int y = 0; y < height; y++) {
          for(int x = 0; x < width; x++) {
            int r = buffer[i + 0];
            int g = buffer[i + 1];
            int b = buffer[i + 2];
            if (bytesPerPixel == 4) {
              // Flatten alpha channel into RGB values:
              int a = buffer[i + 3];
              r = r * a / 255;
              g = g * a / 255;
              b = b * a / 255;
            }
            canvas->SetPixel(x, y, gamma8[r], gamma8[g], gamma8[b]);
            i += bytesPerPixel;
          }
        }
        canvas = matrix->SwapOnVSync(canvas);
        return true;
      }
    }
    return false;
  }

  bool ReadByte(uint8_t *value) {
    return read(STDIN_FILENO, value, 1) == 1;
  }

  bool ReadShort(uint16_t *value) {
    return read(STDIN_FILENO, value, 2) == 2;
  }

  bool ReadLong(uint32_t *value) {
    return read(STDIN_FILENO, value, 4) == 4;
  }

  bool ReadFloat(float *value) {
    return read(STDIN_FILENO, value, 4) == 4;
  }

  bool ReadDouble(double *value) {
    return read(STDIN_FILENO, value, 8) == 8;
  }

  bool ReadBuffer(uint8_t *buffer, int size) {
    return read(STDIN_FILENO, buffer, size) == size;
  }
};

int main(int argc, char *argv[]) {
  RGBMatrix::Options matrix_options;
  rgb_matrix::RuntimeOptions runtime_options;

  if (!ParseOptionsFromFlags(&argc, &argv, &matrix_options, &runtime_options)) {
    return 1;
  }

  RGBMatrix *matrix = CreateMatrixFromOptions(matrix_options, runtime_options);
  if (matrix == NULL) {
    return 1;
  }

  RGBLedInterface *server = new RGBLedInterface(matrix);

  signal(SIGTERM, InterruptHandler);
  signal(SIGINT, InterruptHandler);

  server->Start();
  while (!interrupt_received) {
    sleep(1); // Time doesn't really matter. The syscall will be interrupted.
  }

  delete server;
  delete matrix;

  return 0;
}
