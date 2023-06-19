#include "../headers/stb_vorbis.c"
#include <SDL2/SDL.h>
#include <SDL2/SDL_error.h>
#include <SDL2/SDL_quit.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_video.h>
#include <limits.h>
#include <stdio.h>

#define WINDOW_HEIGHT 512
#define WINDOW_WIDTH 512

void min_max(short *dat, int len, int *min, int *max);

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

  // Main-Loop
  while (!SDL_QuitRequested()) {
    SDL_RenderDrawPoint(renderer, WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2);

    SDL_RenderPresent(renderer);
    SDL_Delay(250);
  }

  int min, max;
  min_max(decoded, len, &min, &max);

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
