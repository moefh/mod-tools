#ifndef WAV_FILE_H_FILE
#define WAV_FILE_H_FILE

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

void wav_write_header(FILE *f, int frequency, int bits_per_sample, int n_channels);
void wav_finish_header(FILE *f, unsigned int sample_data_size);
int wav_write_file(const char *filename, int frequency, int bits_per_sample, int n_channels, const void *sample_data, unsigned int sample_data_size);

#ifdef __cplusplus
}
#endif

#endif /* WAV_FILE_H_FILE */
