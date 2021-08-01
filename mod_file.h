#ifndef MOD_H_FILE
#define MOD_H_FILE

#include "mod_data.h"

struct MOD_FILE {
  char title[21];
  struct {
    char name[23];
  } samples[31];
  char id[5];
  struct MOD_DATA mod;
};

#ifdef __cplusplus
extern "C" {
#endif

struct MOD_FILE *mod_file_read(const char *filename);
void mod_file_free(struct MOD_FILE *mod_file);

#ifdef __cplusplus
}
#endif

#endif /* MOD_H_FILE */
