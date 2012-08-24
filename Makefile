name = shen_run
cfgdir = .

CFLAGS = -Wall
CFLAGS += -I$(cfgdir)
LDFLAGS += -lutil
destdir=/usr/local
bindir=$(destdir)/bin
mandir=$(destdir)/share/man

all: $(name)

install:
	install -m 755 $(name) $(bindir)
	install -m 644 shen_run.1 $(mandir)/man1

clean:
	rm -f script.h $(name) 2>/dev/null

$(cfgdir)/config.h:
	test -f $@ || cp config.def.h $@

$(name): shen_run.c $(cfgdir)/config.h script.h
	$(CC) $(CFLAGS) $< $(LDFLAGS) -o $@

script.h: script.shen
	./mkrun <$< >$@
