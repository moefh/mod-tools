
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mod_file.h"
#include "wav_file.h"

static void dump_sample(const struct MOD_SAMPLE *sample, int frequency, const char *out_file_prefix, int num)
{
  char filename[1024];
  snprintf(filename, sizeof(filename), "%s%02d.wav", out_file_prefix, num+1);

  unsigned char *data = malloc(sample->len);
  if (! data) {
    printf("ERROR: out of memory for sample %d\n", num+1);
    return;
  }
  for (uint32_t i = 0; i < sample->len; i++) {
    data[i] = sample->data[i] + 128;
  }
  
  printf("-> writing %s\n", filename);
  if (wav_write_file(filename, frequency, 8, 1, data, sample->len) != 0) {
    printf("ERROR: can't write '%s'\n", filename);
  }

  free(data);
}

int main(int argc, char *argv[])
{
  if (argc < 2 || argc > 4) {
    printf("USAGE: %s filename.mod [frequency] [sample_file_prefix]\n", argv[0]);
    return 1;
  }
  char *mod_filename       = argv[1];
  int frequency            = (argc > 2) ? atoi(argv[2]) : 11025;
  char *sample_file_prefix = (argc > 3) ? argv[3] : "sample_";

  if (frequency <= 0) {
    printf("%s: invalid frequency: '%s'\n", argv[0], argv[2]);
    exit(1);
  }
  
  // read mod file
  struct MOD_FILE *mod_file = mod_file_read(mod_filename);
  if (! mod_file) {
    printf("%s: can't open '%s'\n", argv[0], mod_filename);
    return 1;
  }
  struct MOD_DATA *mod = &mod_file->mod;

  // dump smaples
  for (int i = 0; i < 31; i++) {
    struct MOD_SAMPLE *sample = &mod->samples[i];
    if (sample->len == 0 || ! sample->data) continue;
    dump_sample(sample, frequency, sample_file_prefix, i);
  }

  // cleanup
  mod_file_free(mod_file);

  return 0;
}
