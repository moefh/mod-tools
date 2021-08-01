# mod-tools

These are some tools for handling [MOD
files](https://en.wikipedia.org/wiki/MOD_(file_format)).

The code includes functions for reading, playing and converting mod
files to C source header files.

## Compilation

```bash
git clone https://github.com/moefh/mod-tools.git
cd mod-tools
make release
```

This will create `mod2wav`, `mod2h` and `dump_samples`.

(Note: using simply `make` instead of `make release` will produce
debug versions of the tools with the address sanitizer and undefined
behavior sanitizer enabled)

## mod2wav

Converts a MOD file to a WAV file:

```bash
# convert mymod.mod to file.wav with 22050 Hz
./mod2wav mymod.mod

# specifying the .wav file frequency:
./mod2wav mymod.mod 44100

# specifying the frequency and output file name:
./mod2wav mymod.mod 44100 other_name.wav
```

The converter is fairly primitive, supporting only mono output with 8
bits per sample.  It also doesn't support many MOD effects (like
tremolo and vibrato).


## mod2h

Converts a MOD file to a C header file (`.h`) for inclusion in a
C program:

```bash
# converts mymod.mod to mymod.h
./mod2h mod_mymod.mod

# specifying the output file name:
./mod2h mymod.mod other_name.h
```

The generated header file should be included in a C source file after
`mod_data.h`:

```C
#include "mod_data.h"
#include "mymod.h"
```

This defines a `static const struct MOD_DATA mod_mymod` global that
can be used with the functions `mod_play_start()` and
`mod_play_step()` from `mod_play.c` to play the MOD (see the example
in `mod2wav.c`).


## dump_samples

Writes the samples from a MOD file to WAV files:

```bash
# writes samples from mymod.mod as 22050 Hz wave files named sample_XX.wav:
./dump_samples mymod.mod

# specifying the frequency of the wav files:
./dump_samples mymod.mod 11025

# specifying the frequency and name prefix of the wav files:
./dump_samples mymod.mod 11025 spl_
```

