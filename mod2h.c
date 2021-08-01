
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mod_file.h"

// copy src to dest replacing non-printable characters and backslashes with spaces
static void sanitize_string(char *dest, size_t dest_size, const char *src)
{
  if (dest_size == 0) return;
  
  char *d = dest;
  for (size_t i = 0; i+1 < dest_size && src[i] != '\0'; i++) {
    if (src[i] >= 32 && src[i] < 127 && src[i] != '\\') {
      *d++ = src[i];
    } else {
      *d++ = ' ';
    }
  }
  *d = '\0';
}

// write to dest a C source identifier based on the mod filename
static void get_mod_ident(char *dest, size_t dest_size, const char *prefix, const char *mod_filename)
{
  if (dest_size == 0) return;
  
  // skip directories
  const char *file_start = strrchr(mod_filename, '/');
  file_start = (file_start) ? file_start+1 : mod_filename;

  const char *file_end = strrchr(file_start, '.');
  size_t file_size = (file_end) ? (size_t)(file_end-file_start) : strlen(file_start);

  // write identifier prefix
  size_t prefix_len = strlen(prefix);
  if (prefix_len >= dest_size-1) {
    memcpy(dest, prefix, dest_size-1);
    dest[dest_size-1] = '\0';
    return;
  }
  memcpy(dest, prefix, prefix_len);
  char *d = dest + prefix_len;

  // build identifier from file name
  int last_was_underscore = (prefix_len > 0 && d[-1] == '_'); // last char was underscore?
  for (size_t i = 0; i+1 < dest_size && i < file_size; i++) {
    char c = file_start[i];
    if (c >= 'A' && c <= 'Z') {
      *d++ = (c - 'A') + 'a';
      last_was_underscore = 0;
    } else if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
      *d++ = c;
      last_was_underscore = 0;
    } else if (! last_was_underscore) {
      *d++ = '_';
      last_was_underscore = 1;
    }
  }
  *d = '\0';
}

static void write_mod_header(FILE *out, struct MOD_FILE *mod_file, const char *mod_filename)
{
  struct MOD_DATA *mod = &mod_file->mod;

  char mod_ident[256];
  get_mod_ident(mod_ident, sizeof(mod_ident), "mod_", mod_filename);

  // write comment with original filename
  char sanitized_mod_filename[256];
  sanitize_string(sanitized_mod_filename, sizeof(sanitized_mod_filename), mod_filename);
  char *sanitized_mod_filename_no_path = strrchr(sanitized_mod_filename, '/');
  if (sanitized_mod_filename_no_path) {
    sanitized_mod_filename_no_path++;  // skip last slash
  } else {
    sanitized_mod_filename_no_path = sanitized_mod_filename;
  }
  fprintf(out, "// generated from %s\n\n", sanitized_mod_filename_no_path);
  
  // write samples
  for (int i = 0; i < 31; i++) {
    const struct MOD_SAMPLE *sample = &mod->samples[i];
    if (sample->len == 0) {
      fprintf(out, "#define %s_sample_%02d ((void*)0)\n\n", mod_ident, i+1);
      continue;
    }
    fprintf(out, "static const signed char %s_sample_%02d[] = {", mod_ident, i+1);
    const int8_t *data = sample->data;
    for (uint32_t s = 0; s < sample->len; s++) {
      if (s % 16 == 0) fprintf(out, "\n  ");
      fprintf(out, "%d,", data[s]);
    }
    fprintf(out, "\n};\n\n");
  }

  // write pattern
  fprintf(out, "static const struct MOD_CELL %s_pattern[] = {\n", mod_ident);
  const struct MOD_CELL *cell = mod->pattern;
  for (int p = 0; p < mod->num_patterns; p++) {
    fprintf(out, "  // pattern %d\n", p);
    for (int r = 0; r < 64; r++) {
      for (int c = 0; c < mod->num_channels; c++) {
        fprintf(out, "  { .sample = %2d, .period = %5d, .effect = 0x%03x },\n", cell->sample, cell->period, cell->effect);
        cell++;
      }
      if (p+1 < mod->num_patterns || r+1 < 64) {
        fprintf(out, "\n");
      }
    }
  }
  fprintf(out, "};\n\n");
  
  char mod_title[21];
  sanitize_string(mod_title, sizeof(mod_title), mod_file->title);
  fprintf(out, "// %s\n", mod_title);
  fprintf(out, "static const struct MOD_DATA %s = {\n", mod_ident);
  fprintf(out, "  .samples = {\n");
  for (int i = 0; i < 31; i++) {
    struct MOD_SAMPLE *sample = &mod->samples[i];
    char sample_name[23];
    sanitize_string(sample_name, sizeof(sample_name), mod_file->samples[i].name);
    fprintf(out, "    { .len = %6d, .loop_start = %6d, .loop_len = %6d, .finetune = %d, .volume = %2d, .data = %s_sample_%02d }, // %s\n",
           sample->len, sample->loop_start, sample->loop_len, sample->finetune, sample->volume, mod_ident, i+1, sample_name);
  }
  fprintf(out, "  },\n");
  fprintf(out, "  .num_song_positions = %d,\n", mod->num_song_positions);
  fprintf(out, "  .song_positions = {");
  for (int i = 0; i < mod->num_song_positions; i++) {
    if (i % 32 == 0) fprintf(out, "\n   ");
    fprintf(out, " %2d,", mod->song_positions[i]);
  }
  fprintf(out, "\n  },\n");
  fprintf(out, "  .num_channels = %d,\n", mod->num_channels);
  fprintf(out, "  .num_patterns = %d,\n", mod->num_patterns);
  fprintf(out, "  .pattern = %s_pattern,\n", mod_ident);
  
  fprintf(out, "};\n");
}

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
  if (argc != 2 && argc != 3) {
    printf("USAGE: %s filename.mod [filename.h]\n", argv[0]);
    return 1;
  }
  char *mod_filename = argv[1];
  char *out_filename = argv[2];
  char out_filename_data[1024];
  
  if (out_filename == NULL) {
    get_out_filename(out_filename_data, sizeof(out_filename_data), mod_filename, ".h");
    out_filename = out_filename_data;
  }

  // read input mod file
  struct MOD_FILE *mod_file = mod_file_read(mod_filename);
  if (! mod_file) {
    printf("%s: can't read '%s'\n", argv[0], mod_filename);
    return 1;
  }

  // open output header file
  FILE *f = fopen(out_filename, "w");
  if (! f) {
    printf("%s: can't open output '%s'\n", argv[0], out_filename);
    mod_file_free(mod_file);
    exit(1);
  }

  printf("-> converting '%s' to '%s'\n", mod_filename, out_filename);
  
  // write output
  write_mod_header(f, mod_file, mod_filename);

  // cleanup
  fclose(f);
  mod_file_free(mod_file);

  return 0;
}
