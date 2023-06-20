#include "../headers/stb_vorbis.c"
#include <SDL2/SDL.h>
#include <SDL2/SDL_error.h>
#include <SDL2/SDL_quit.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_video.h>
#include <complex.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define WINDOW_HEIGHT 512
#define WINDOW_WIDTH 512
#define CENTER_Y (WINDOW_HEIGHT / 2)

void min_max(short *dat, int len, int *min, int *max);
void compute_points(SDL_Point *points, short *dat, size_t len);
void compute_dft(double complex *input, double complex *output, size_t n);

int main() {
  // Load and decode OGG file.
  int channels;
  int sample_rate;
  short *decoded;

  int len = stb_vorbis_decode_filename(
      "/home/toni/Music/The Beatles/Let It Be (deluxe "
      "2CD edition)/05 - Dig It (2021 mix).ogg",
      &channels, &sample_rate, &decoded);

  if (!len) {
    fprintf(stderr, "ERROR: Could not read file!\n");
    return 1;
  }

  double complex *in = malloc(sizeof(*in) * len / 50);
  double complex *out = malloc(sizeof(*out) * len / 50);

  for (size_t i = 0; i < len / 50; i++) {
    in[i] = (double)decoded[i] + 0 * I;
  }

  printf("Pre DFT\n");
  compute_dft(in, out, len / 50);
  printf("After DFT\n");

  for (size_t i = 0; i < len / 50; i++) {
    printf("%lf + %lfi\n", creal(out[i]), cimag(out[i]));
  }

  free(out);
  free(in);

  // Init SDL
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    fprintf(stderr, "ERROR: Could not initialise SDL_VIDEO: %s\n",
            SDL_GetError());
    return 1;
  }

  // Create window and renderer to be used.
  SDL_Window *window = SDL_CreateWindow("Spectrogram", 0, 0, WINDOW_WIDTH,
                                        WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
  SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);

  // Setup Color
  SDL_SetRenderDrawColor(renderer, 255, 0, 0, 0);
  SDL_ShowWindow(window);

  SDL_Point *points = malloc(sizeof(SDL_Point) * len);
  compute_points(points, decoded, len);

  // Main-Loop -> iterates once per screen.
  size_t i = 0;
  while (!SDL_QuitRequested()) {
    // Set Background to black.
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);
    // Set Point color to red.
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 0);

    // Break out the loop when all the points have been printed.
    if (i >= len / WINDOW_WIDTH)
      break;

    // Render the points from the last point drawn with a count of WINDOW_WIDTH.
    // Pointer Arithmetic to skip how many screens were drawn.
    SDL_RenderDrawPoints(renderer, points + (i * WINDOW_WIDTH), WINDOW_WIDTH);
    SDL_RenderPresent(renderer);

    i++;
    // Sleep for the correct time to make the waveform in sync with the audio.
    SDL_Delay(1000.0f / (sample_rate / (float)WINDOW_WIDTH));
  }

  free(points);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}

void min_max(short *dat, int len, int *min, int *max) {
  *min = INT_MAX;
  *max = INT_MIN;
  for (size_t i = 0; i < len; i++) {
    if (dat[i] < *min)
      *min = dat[i];
    if (dat[i] > *max)
      *max = dat[i];
  }
}

void compute_points(SDL_Point *points, short *dat, size_t len) {
  int min, max;
  min_max(dat, len, &min, &max);

  int x = 0;
  for (size_t i = 0; i < len; i++) {
    // Reset the x-coordinate, each time we go over a screen width.
    if (x % WINDOW_WIDTH == 0)
      x = 0;

    short val = dat[i];
    int y = 0;

    // If the value is negative, it needs to be below the center (height / 2)
    // otherwise it is above the center.
    //
    // To find the right value the algorithm normalises the point with its
    // respective minimum or maximum.
    //
    // It follows this by setting the y-coordinate value with regart to the
    // beforementioned center y. The normalised value is firslty scaled then
    // subtracted or added to the center, depending on the orientation of the
    // value (<0 || >0).
    min = abs(min);
    if (val < 0) {
      float norm = (float)abs(val) / (float)min;
      y = (int)CENTER_Y - ((float)WINDOW_HEIGHT / 2) * norm;
    } else {
      float norm = (float)val / (float)max;
      y = (int)CENTER_Y + ((float)WINDOW_HEIGHT / 2) * norm;
    }

    x += 1;
    points[i] = (SDL_Point){.x = x, .y = y};
  }
}

void compute_dft(double complex *input, double complex *output, size_t n) {
  // Do for each output element.
  for (size_t i = 0; i < n; i++) {
    // Do for each input element. (Compute the sum).
    double complex sum = 0.0;
    for (size_t j = 0; j < n; j++) {
      // sum += input * e^(complex)(-2 * PI * j * i / n)
      // sum += input * e^exponent
      double complex exponent = -2 * M_PI * j * i / n * I;
      sum += input[j] * cexp(exponent);
    }

    output[i] = sum;
  }
}
