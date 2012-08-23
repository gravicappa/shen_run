dst = shen_run
LDFLAGS += -lutil
destdir=/usr/local
bindir=$(destdir)/bin
mandir=$(destdir)/share/man

all: $(dst)

install:
	install -m 755 $(dst) $(bindir)
	install -m 644 shen_run.1 $(mandir)/man1

clean:
	rm -f script.h $(dst) 2>/dev/null

config.h:
	cp config.def.h $@

shen_run: shen_run.c config.h script.h

script.h: script.shen
	./mkrun <$< >$@

%: %.c
	$(CC) $(CFLAGS) $< $(LDFLAGS) -o $@
