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
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define SWAP(x, y, T)                                                          \
  do {                                                                         \
    T tmp = x;                                                                 \
    x = y;                                                                     \
    y = tmp;                                                                   \
  } while (0);

#define WINDOW_HEIGHT 512
#define WINDOW_WIDTH 512
#define CENTER_Y (WINDOW_HEIGHT / 2)
#define SPECTRE_DFT

void min_max(short *dat, int len, int *min, int *max);
void signal_calc_coordinates(SDL_Point *points, short *dat, size_t len);
bool is_power_of_2(size_t n);
void pad_arr(short **dat, size_t *n);
void arr_rev(double complex *dat, size_t n);
size_t bit_rev(size_t n);

void dft_compute(double complex *input, double complex *output, size_t n);
void dft_amp(double complex *input, double *output, size_t n);
void dft_calc_coordinates(SDL_Point *points, const size_t width,
                          const double *dat, const size_t n);

void fft_compute(double complex *input, double complex *output, size_t n);

void signal_draw(SDL_Point *points, size_t len, int sample_rate,
                 SDL_Renderer *renderer);
void dft_draw(SDL_Point *points, size_t len, SDL_Renderer *renderer);

int main() {
  // Load and decode OGG file.
  int channels;
  int sample_rate;
  short *decoded;

  size_t len =
      stb_vorbis_decode_filename("/home/toni/Documents/Files/Personal_TG/"
                                 "programming/Projects/spectrogram/sine.ogg",
                                 &channels, &sample_rate, &decoded);

  if (!len) {
    fprintf(stderr, "ERROR: Could not read file!\n");
    return 1;
  }

  pad_arr(&decoded, &len);

#ifdef SPECTRE_DFT
  // Specify the length of the snippet to apply the transformation to.
  // Allocate all the neccesary buffers for the DFT to store the data
  size_t dft_len = len / 50;
  double complex *complex_decoded = malloc(sizeof(*complex_decoded) * dft_len);
  double complex *dft_out = malloc(sizeof(*dft_out) * dft_len);
  double *amplitudes = malloc(sizeof(*amplitudes) * dft_len);

  // Convert the real numbers (decoded data) into imaginary numbers.
  for (size_t i = 0; i < dft_len; i++) {
    complex_decoded[i] = (double)decoded[i] + 0 * I;
  }

  for (size_t i = 0; i < 16; i++) {
    printf("%lf+ %lfi\n", creal(complex_decoded[i]), cimag(complex_decoded[i]));
  }
  printf("\n\n");

  arr_rev(complex_decoded, dft_len);

  for (size_t i = 0; i < 16; i++) {
    printf("%lf+ %lfi\n", creal(complex_decoded[i]), cimag(complex_decoded[i]));
  }

  // Compute DFT and extract the amplitudes from the complex numbers.
  dft_compute(complex_decoded, dft_out, dft_len);
  dft_amp(dft_out, amplitudes, dft_len);

  // Free unnecessary buffers.
  free(dft_out);
  free(complex_decoded);
#endif // SPECTRE_DFT

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

#ifdef SPECTRE_DFT
  SDL_Point *points = malloc(sizeof(SDL_Point) * WINDOW_WIDTH);
  dft_calc_coordinates(points, WINDOW_WIDTH, amplitudes, dft_len);
  free(amplitudes);

  dft_draw(points, len, renderer);
#else
  SDL_Point *points = malloc(sizeof(SDL_Point) * len);
  signal_calc_coordinates(points, decoded, len);
  free(decoded);

  signal_draw(points, len, sample_rate, renderer);
#endif // SPECTRE_DFT

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

void signal_calc_coordinates(SDL_Point *points, short *dat, size_t len) {
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

void dft_compute(double complex *input, double complex *output, size_t n) {
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

void dft_amp(double complex *input, double *output, size_t n) {
  for (size_t i = 0; i < n; i++) {
    double real = creal(input[i]);
    double imag = cimag(input[i]);
    output[i] = sqrt(real * real + imag * imag) / (double)n;
  }
}

void dft_calc_coordinates(SDL_Point *points, const size_t width,
                          const double *dat, const size_t n) {
  size_t values_per_pixel = n / width;

  if (!values_per_pixel) {
    fprintf(stderr, "ERROR: values_per_pixel = 0\n");
    return;
  }

  for (size_t i = 0; i < width; i++) {
    // Finds the range from where to where we need to iterate to find the
    // average.
    size_t begin_offset = i * values_per_pixel;
    size_t end_offset = (i + 1) * values_per_pixel;

    // Find average height for the pixel.
    size_t sum = 0;
    for (size_t j = begin_offset; j < end_offset; j++) {
      sum += dat[i];
    }
    size_t avg = sum / values_per_pixel;

    points[i] = (SDL_Point){.x = i, .y = WINDOW_HEIGHT - 1 - avg};
  }
}

void signal_draw(SDL_Point *points, size_t len, int sample_rate,
                 SDL_Renderer *renderer) {
  size_t i = 0;
  // Main - Loop->iterates once per screen.size_t i = 0;
  while (!SDL_QuitRequested()) {
    // Set Background to black.
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);
    // Set Point color to red.
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 0);

    // Break out the loop when all the points have been printed.
    if (i >= len / WINDOW_WIDTH)
      break;

    // Render the points from the last point drawn with a count of
    // WINDOW_WIDTH.
    // Pointer Arithmetic to skip how many screens were drawn.
    SDL_RenderDrawPoints(renderer, points + (i * WINDOW_WIDTH), WINDOW_WIDTH);
    SDL_RenderPresent(renderer);

    i++;
    // Sleep for the correct time to make the waveform in sync with the
    // audio.
    SDL_Delay(1000.0f / (sample_rate / (float)WINDOW_WIDTH));
  }
}

void dft_draw(SDL_Point *points, size_t len, SDL_Renderer *renderer) {
  // Set Background to black.
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
  SDL_RenderClear(renderer);
  // Set Point color to red.
  SDL_SetRenderDrawColor(renderer, 255, 0, 0, 0);

  // Render all the points.
  SDL_RenderDrawPoints(renderer, points, WINDOW_WIDTH);
  SDL_RenderPresent(renderer);

  while (!SDL_QuitRequested()) {
    SDL_Delay(200);
  }
}

bool is_power_of_2(size_t n) { return ((n & (n - 1)) == 0) ? true : false; }

void pad_arr(short **dat, size_t *n) {
  if (is_power_of_2(*n))
    return;

  // Find next power of two.
  size_t tmp = *n;
  while (tmp & (tmp - 1)) {
    tmp &= tmp - 1;
  }

  size_t new_len = tmp << 1;

  // Reallocate and fill array with 0.
  short *new_dat = realloc(*dat, sizeof(*dat) * new_len);

  if (new_dat == NULL) {
    fprintf(stderr, "ERROR: Could not make array bigger\n");
    return;
  }

  for (size_t i = *n; i < new_len; i++) {
    new_dat[i] = 0;
  }

  *n = new_len;
  *dat = new_dat;
}

void arr_rev(double complex *dat, size_t n) {
  // Iterate only half of the samples, because if you swap the second half you
  // would just be reversing the previous swap.
  size_t j = 0;
  for (size_t i = 0; i < n / 2; i++) {
    if (j > i) {
      SWAP(dat[i], dat[j], double complex);

      if ((j / 2) < (n / 4)) {
        SWAP(dat[n - (i + 1)], dat[n - (j + 1)], double complex);
      }
    }

    // Bit reversal stuff.
    // size_t m = n / 2;
    j = bit_rev(i);
    // printf("j afer iter: %zu\n", j);
  }
}

size_t bit_rev(size_t n) {
  size_t rev = 0;

  for (size_t i = 0; i < sizeof(n) * 8; i++, n >>= 1) {
    rev = (rev << 1) | (n & 0x01);
  }

  return rev;
}

void fft_compute(double complex *input, double complex *output, size_t n) {}
