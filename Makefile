name = shen_run
impl = clisp

CFLAGS = -Wall
CFLAGS += -I$(impl)
LDFLAGS += -lutil
prefix=/usr/local
bindir=$(prefix)/bin
mandir=$(prefix)/share/man

all: $(name)_$(impl)

install:
	mkdir -p $(destdir)/$(bindir) $(destdir)/$(mandir)/man1
	install -m 755 $(name)_$(impl) $(destdir)/$(bindir)
	install -m 644 shen_run.1 $(destdir)/$(mandir)/man1
	ln -s shen_run.1 $(destdir)/$(mandir)/man1/$(name)_$(impl).1

clean:
	rm -f script.h $(name)_$(impl) 2>/dev/null

$(impl)/config.h:
	mkdir -p $(impl)
	test -f $@ || cp config.def.h $@

$(name)_$(impl): shen_run.c $(impl)/config.h script.h
	$(CC) $(CFLAGS) $< $(LDFLAGS) -o $@

script.h: script.shen
	./mkrun <$< >$@
