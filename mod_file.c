
#define _POSIX_C_SOURCE 200112L
#define _FILE_OFFSET_BITS 64

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "mod_file.h"

struct READFILE {
  size_t len;
  unsigned char *data;
  size_t pos;
};

static int read_open(struct READFILE *f, const char *filename)
{
  FILE *file = fopen(filename, "rb");
  if (! file) {
    return -1;
  }

  fseeko(file, 0, SEEK_END);
  off_t len = ftell(file);
  fseeko(file, 0, SEEK_SET);

  f->len = (size_t) len;
  f->pos = 0;
  f->data = malloc(f->len);
  if (! f->data) goto error;
  if (fread(f->data, 1, f->len, file) != f->len) goto error;
  
  fclose(file);
  return 0;

 error:
  fclose(file);
  return -1;
}

static void read_close(struct READFILE *f)
{
  free(f->data);
  f->len = f->pos = 0;
}

static inline size_t read_pos(struct READFILE *f) { return f->pos; }
static inline void read_seek(struct READFILE *f, size_t pos) { f->pos = pos; if (f->pos > f->len) f->pos = f->len; }
static inline void read_skip(struct READFILE *f, size_t len) { f->pos += len; if (f->pos > f->len) f->pos = f->len; }
static inline uint8_t read_u8(struct READFILE *f) { if (f->pos < f->len) return f->data[f->pos++]; return 0; }

static inline uint16_t read_u16(struct READFILE *f)
{
  uint16_t b1 = read_u8(f);
  uint16_t b2 = read_u8(f);
  return (b1<<8) | b2;
}

static inline uint32_t read_u32(struct READFILE *f)
{
  uint32_t b1 = read_u8(f);
  uint32_t b2 = read_u8(f);
  uint32_t b3 = read_u8(f);
  uint32_t b4 = read_u8(f);
  return (b1<<24) | (b2<<16) | (b3<<8) | b4;
}

static inline void read_data(struct READFILE *f, void *data, size_t len)
{
  char *dest = data;
  size_t copy_len = len;
  if (copy_len > f->len - f->pos) {
    copy_len = f->len - f->pos;
  }
  memcpy(dest, &f->data[f->pos], copy_len);
  f->pos += copy_len;
  
  if (copy_len < len) {
    memset(dest + copy_len, 0, len - copy_len);
  }
}

static inline void read_string(struct READFILE *f, char *str, size_t len)
{
  read_data(f, str, len);
  str[len] = '\0';
}

static struct MOD_FILE *mod_file_new(void)
{
  struct MOD_FILE *mod_file = malloc(sizeof(struct MOD_FILE));
  if (! mod_file) {
    return NULL;
  }
  mod_file->title[0] = '\0';
  mod_file->id[0] = '\0';
  for (int i = 0; i < 31; i++) {
    mod_file->samples[i].name[0] = '\0';
  }
  for (int i = 0; i < 31; i++) {
    mod_file->mod.samples[i].len        = 0;
    mod_file->mod.samples[i].finetune   = 0;
    mod_file->mod.samples[i].volume     = 0;
    mod_file->mod.samples[i].loop_start = 0;
    mod_file->mod.samples[i].loop_len   = 0;
    mod_file->mod.samples[i].data       = NULL;
  }
  mod_file->mod.pattern = NULL;
  return mod_file;
}

void mod_file_free(struct MOD_FILE *mod_file)
{
  if (! mod_file) {
    return;
  }
  
  for (int i = 0; i < 31; i++) {
    free((char*)mod_file->mod.samples[i].data);
  }
  free((char*)mod_file->mod.pattern);
  free(mod_file);
}

struct MOD_FILE *mod_file_read(const char *filename)
{
  struct READFILE file;
  if (read_open(&file, filename) != 0) {
    return NULL;
  }
  struct MOD_FILE *mod_file = mod_file_new();
  if (! mod_file) {
    goto error;
  }
  struct MOD_DATA *mod = &mod_file->mod;
  struct READFILE *f = &file;

  // read mod ID to get number of samples and channels
  read_seek(f, 1080);
  read_string(f, mod_file->id, 4);
  int num_samples = 0;
  if (strcmp(mod_file->id, "M.K.") == 0 || strcmp(mod_file->id, "M!K!") == 0 || strcmp(mod_file->id, "4CHN") == 0 || strcmp(mod_file->id, "FLT4") == 0) {
    num_samples = 31;
    mod->num_channels = 4;
  } else if (strcmp(mod_file->id, "6CHN") == 0) {
    num_samples = 31;
    mod->num_channels = 6;
  } else if (strcmp(mod_file->id, "8CHN") == 0 || strcmp(mod_file->id, "FLT8") == 0) {
    num_samples = 31;
    mod->num_channels = 8;
  } else {
    num_samples = 15;
    mod->num_channels = 4;
  }

  // read header and sample info
  read_seek(f, 0);
  read_string(f, mod_file->title, 20);
  for (int i = 0; i < num_samples; i++) {
    read_string(f, mod_file->samples[i].name, 22);

    mod->samples[i].len        = read_u16(f) * 2;
    mod->samples[i].finetune   = read_u8(f) & 0x0f;
    mod->samples[i].volume     = read_u8(f);
    mod->samples[i].loop_start = read_u16(f) * 2;
    mod->samples[i].loop_len   = read_u16(f) * 2;

    if (mod->samples[i].finetune > 7) {
      mod->samples[i].finetune -= 16;
    }
  }
  
  // read song positions
  read_seek(f, 950);
  mod->num_song_positions = read_u8(f);
  read_u8(f);  // ignore (restart song position?)
  read_data(f, mod->song_positions, 128);

  // read pattern
  mod->num_patterns = 0;
  for (int i = 0; i < 128; i++) {
    if (mod->num_patterns < mod->song_positions[i]+1) {
      mod->num_patterns = mod->song_positions[i]+1;
    }
  }  
  struct MOD_CELL *cell = malloc(sizeof(struct MOD_CELL) * mod->num_patterns * 64 * mod->num_channels);
  if (! cell) goto error;
  mod->pattern = cell;
  read_seek(f, 1084);
  for (int p = 0; p < mod->num_patterns; p++) {
    for (int r = 0; r < 64; r++) {
      for (int c = 0; c < mod->num_channels; c++) {
        unsigned char data[4];
        read_data(f, data, 4);
        cell->sample = (data[0] & 0xf0) | (data[2] >> 4);
        cell->period = ((data[0] & 0x0f) << 8) | data[1];
        cell->effect = ((data[2] & 0x0f) << 8) | data[3];
        cell++;
      }
    }
  }

  // read sample data
  for (int i = 0; i < num_samples; i++) {
    struct MOD_SAMPLE *sample = &mod->samples[i];
    if (sample->len == 0) continue;
    int8_t *data = malloc(sample->len);
    if (! data) goto error;
    read_data(f, data, sample->len);
    sample->data = data;
  }

  read_close(f);
  return mod_file;

 error:
  mod_file_free(mod_file);
  read_close(&file);
  return NULL;
}
