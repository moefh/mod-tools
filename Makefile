
CC = gcc
CFLAGS = -O1 -g -Wall -Wextra -std=c99 -pedantic -fsanitize=address,undefined
LDFLAGS = -fsanitize=address,undefined

CFLAGS_RELEASE = -O2 -Wall -Wextra -std=c99
LDFLAGS_RELEASE = -s

MOD2H_OBJS        = mod2h.o mod_file.o
MOD2WAV_OBJS      = mod2wav.o mod_file.o wav_file.o mod_play.o
DUMP_SAMPLES_OBJS = dump_samples.o mod_file.o wav_file.o

ALL = mod2h mod2wav dump_samples

.PHONY: all clean

all: $(ALL)

release:
	$(MAKE) "CFLAGS=$(CFLAGS_RELEASE)" "LDFLAGS=$(LDFLAGS_RELEASE)" $(ALL)

clean:
	rm -f *.o *~ $(ALL)

mod2h: $(MOD2H_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(MOD2H_OBJS)

mod2wav: $(MOD2WAV_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(MOD2WAV_OBJS)

dump_samples: $(DUMP_SAMPLES_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(DUMP_SAMPLES_OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<
