name = shen_run
impl = clisp

CFLAGS = -Wall
CFLAGS += -I$(impl)
LDFLAGS += -lutil
destdir=/usr/local
bindir=$(destdir)/bin
mandir=$(destdir)/share/man

all: $(name)_$(impl)

install:
	install -m 755 $(name) $(bindir)
	install -m 644 shen_run.1 $(mandir)/man1

clean:
	rm -f script.h $(name) 2>/dev/null

$(impl)/config.h:
	mkdir -p $(impl)
	test -f $@ || cp config.def.h $@

$(name)_$(impl): shen_run.c $(impl)/config.h script.h
	$(CC) $(CFLAGS) $< $(LDFLAGS) -o $@

script.h: script.shen
	./mkrun <$< >$@
