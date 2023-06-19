#include "../headers/stb_vorbis.c"
#include <SDL2/SDL.h>
#include <SDL2/SDL_error.h>
#include <limits.h>
#include <stdio.h>

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

  int min, max;
  min_max(decoded, len, &min, &max);

  printf("min: %d, max: %d\n", min, max);

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
