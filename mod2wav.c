
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "mod_file.h"
#include "mod_play.h"
#include "wav_file.h"

// write to dest the output filename based on the mod filename
static void get_out_filename(char *dest, size_t dest_size, const char *mod_filename, const char *ext)
{
  if (dest_size == 0) return;
  
  const char *file_start = mod_filename;
  const char *file_end = strrchr(file_start, '.');

  size_t d = 0;

  // copy file name without extension
  for (const char *src = file_start; d+1 < dest_size && src != file_end; src++) {
    dest[d++] = *src;
  }
  
  // copy extension
  for (const char *src = ext; d+1 < dest_size && *src != '\0'; src++) {
    dest[d++] = *src;
  }
  
  dest[d] = '\0';
}

int main(int argc, char *argv[])
{
  if (argc < 2 || argc > 4) {
    printf("USAGE: %s filename.mod [frequency] [filename.wav]\n", argv[0]);
    exit(1);
  }
  char *mod_filename = argv[1];
  int frequency      = (argc > 2) ? atoi(argv[2]) : 22050;
  char *out_filename = (argc > 3) ? argv[3] : NULL;
  char out_filename_data[1024];

  if (frequency <= 0) {
    printf("%s: invalid frequency: '%s'\n", argv[0], argv[2]);
    exit(1);
  }
  
  if (out_filename == NULL) {
    get_out_filename(out_filename_data, sizeof(out_filename_data), mod_filename, ".wav");
    out_filename = out_filename_data;
  }

  // read input mod file
  struct MOD_FILE *mod_file = mod_file_read(mod_filename);
  if (! mod_file) {
    printf("%s: can't read '%s'\n", argv[0], mod_filename);
    return 1;
  }

  // open output wav file and write header
  FILE *f = fopen(out_filename, "wb");
  if (! f) {
    printf("%s: can't open '%s'\n", argv[0], out_filename);
    exit(1);
  }
  wav_write_header(f, frequency, 8, 1);

  printf("-> playing '%s' to '%s' at %d Hz\n", mod_filename, out_filename, frequency);
  
  // convert
  mod_play_start(&mod_file->mod, frequency, 0);
  int stop = 0;
  unsigned int sample_data_size = 0;
  while (! stop) {
    unsigned char buf[512];
    stop = mod_play_step(buf, sizeof(buf)/sizeof(buf[0]));
    fwrite(buf, 1, sizeof(buf), f);
    sample_data_size += sizeof(buf);
  }

  // fill missing wave header parts
  wav_finish_header(f, sample_data_size);
  
  // cleanup
  fclose(f);
  mod_file_free(mod_file);
  return 0;
}
