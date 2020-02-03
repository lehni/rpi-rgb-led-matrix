// -*- mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
// Small example how to use the library.
// For more examples, look at demo-main.cc
//
// This code is public domain
// (but note, that the led-matrix library this depends on is GPL v2)

#include "led-matrix.h"

#include <unistd.h>
#include <math.h>
#include <stdio.h>

using rgb_matrix::GPIO;
using rgb_matrix::RGBMatrix;
using rgb_matrix::Canvas;

Canvas *canvas = NULL;

void wait(int duration = 1) {
  const int sleep = 10; // 100;
  usleep(sleep * 1000 * duration);
}

const short int max = 255;
const short int on = 63;
const short int off = 0;

void setPixel(int x, int y, int r, int g, int b, bool print = true) {
  canvas->SetPixel(x, y, r, g, b);
  if (print) {
    printf("(%i, %i): #%02x%02x%02x\n", x, y, r, g, b);
  }
}

int main(int argc, char *argv[]) {
  RGBMatrix::Options matrix_options;
  rgb_matrix::RuntimeOptions runtime_opt;

  // These are the defaults when no command-line flags are given.
  matrix_options.rows = 16;
  matrix_options.cols = 16;
  matrix_options.chain_length = 1;
  matrix_options.parallel = 1;

  // First things first: extract the command line flags that contain
  // relevant matrix options.
  if (!ParseOptionsFromFlags(&argc, &argv, &matrix_options, &runtime_opt)) {
    return 0;
  }

  /*
   * Set up GPIO pins. This fails when not running as root.
   */
  GPIO io;
  if (!io.Init())
    return 1;

  /*
   * Set up the RGBMatrix. It implements a 'Canvas' interface.
   */
  /*
  int rows = 8;
  int chain = 1;
  int parallel = 1; // Number of chains in parallel (1..3). > 1 for plus or Pi2
  canvas = new RGBMatrix(&io, rows, chain, parallel);
  */
  int rows = matrix_options.rows;
  int cols = matrix_options.cols;
  canvas = CreateMatrixFromOptions(matrix_options, runtime_opt);

  /*
  int map[256][16];

  for (int i=0; i<256*16; i++){
    map[i%256][i/256] = i%8 + (2*(i/8)+1-(i%2048)/1024)*8%2048 + (i/2048)*2048;
  }

  printf("map table :\n");
  for (int i=0; i<256; i++){
    for(int j=0; j<16; j++){
      printf("%d\t",map[i][j]);
    }
    printf("\n");
  }
  */

  for (int y = 0; y < rows; y++) {
    for (int x = 0; x < cols * 6; x++) {
      setPixel(x, y, max, max, max);
      wait();
      // setPixel(x, y, off, off, off, false);
    }
  }

  /*
  for (int y = 0; y < 4; y++) {
    for (int x = 0; x < 8; x++) {
      setPixel(x, y, true, 0, 0);
    }
    wait(10);
    for (int x = 8; x < 16; x++) {
      setPixel(x, y, 0, true, 0);
    }
    wait(10);
    for (int x = 16; x < 24; x++) {
      setPixel(x, y, 0, 0, true);
    }
    wait(10);
    for (int x = 24; x < 32; x++) {
      setPixel(x, y, true, 0, 0);
    }
    wait(20);
  }

  for (int y = 8; y < 12; y++) {
    for (int x = 0; x < 16; x++) {
      setPixel(x, y, true, 0, 0);
    }
    wait(10);
    for (int x = 16; x < 32; x++) {
      setPixel(x, y, 0, true, 0);
    }
    wait(20);
  }
  */

  while(true);

  // Animation finished. Shut down the RGB matrix.
  canvas->Clear();
  delete canvas;

  return 0;
}