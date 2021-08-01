
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "wav_file.h"

#define WAV_HEADER_SIZE            44
#define WAV_HEADER_FILESIZE_OFFSET 4
#define WAV_HEADER_DATASIZE_OFFSET 40

static void write_u8(FILE *f, uint8_t i) { fputc(i, f); }
static void write_u16(FILE *f, uint16_t i) { write_u8(f, i&0xff); write_u8(f, i>>8); }
static void write_u32(FILE *f, uint32_t i) { write_u8(f, i&0xff); write_u8(f, (i>>8)&0xff); write_u8(f, (i>>16)&0xff); write_u8(f, (i>>24)&0xff); }
static void write_data(FILE *f, const void *data, size_t len) { fwrite(data, 1, len, f); }
static void write_string(FILE *f, const char *str) { write_data(f, str, strlen(str)); }

void wav_write_header(FILE *f, int frequency, int bits_per_sample, int n_channels)
{
  write_string(f, "RIFF");
  write_u32(f, 0);          // file size in bytes - 8 (filled later)
  write_string(f, "WAVE");

  int bytes_per_spl = bits_per_sample/8;
  int bytes_per_sec = frequency * bits_per_sample/8 * n_channels;
  
  write_string(f, "fmt ");
  write_u32(f, 16);              // fmt chunk size
  write_u16(f, 1);               // 1=PCM
  write_u16(f, 1);               // number of channels
  write_u32(f, frequency);       // frequency
  write_u32(f, bytes_per_sec);   // bytes/sec
  write_u16(f, bytes_per_spl);   // sample size in bytes
  write_u16(f, bits_per_sample); // bits per sample
  
  write_string(f, "data");
  write_u32(f, 0);          // size of sample data in bytes (filled later)
}

void wav_finish_header(FILE *f, unsigned int sample_data_size)
{
  fseek(f, WAV_HEADER_FILESIZE_OFFSET, SEEK_SET);
  write_u32(f, WAV_HEADER_SIZE-8+sample_data_size);

  fseek(f, WAV_HEADER_DATASIZE_OFFSET, SEEK_SET);
  write_u32(f, sample_data_size);
}

int wav_write_file(const char *filename, int frequency, int bits_per_sample, int n_channels, const void *sample_data, unsigned int sample_data_size)
{
  FILE *f = fopen(filename, "wb");
  if (! f) {
    return -1;
  }

  wav_write_header(f, frequency, bits_per_sample, n_channels);
  write_data(f, sample_data, sample_data_size);
  wav_finish_header(f, sample_data_size);
  
  fclose(f);
  return 0;
}
