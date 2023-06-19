#include "../headers/stb_vorbis.c"
#include <SDL2/SDL.h>
#include <SDL2/SDL_error.h>
#include <SDL2/SDL_quit.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_video.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#define WINDOW_HEIGHT 512
#define WINDOW_WIDTH 512
#define CENTER_Y (WINDOW_HEIGHT / 2)

void min_max(short *dat, int len, int *min, int *max);
void compute_points(SDL_Point *points, short *dat, size_t len);

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

  // Main-Loop
  size_t i = 0;
  while (!SDL_QuitRequested()) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 0);

    if (i >= len / WINDOW_WIDTH)
      break;

    SDL_RenderDrawPoints(renderer, points + (i * WINDOW_WIDTH), WINDOW_WIDTH);
    // SDL_RenderDrawPoint(renderer, x, cy + (decoded[i] * step_y));
    SDL_RenderPresent(renderer);

    i++;
    SDL_Delay(11);
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
    if (x % 512 == 0)
      x = 0;

    short val = dat[i];
    int y = 0;
    if (val < 0) {
      float norm = (float)abs(val) / (float)min;
      y = (int)CENTER_Y - ((float)WINDOW_HEIGHT / 2) * norm;
    } else {
      float norm = (float)val / (float)min;
      y = (int)CENTER_Y + ((float)WINDOW_HEIGHT / 2) * norm;
    }

    x += 1;
    points[i] = (SDL_Point){.x = x, .y = y};
  }
}
