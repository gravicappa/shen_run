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
	-rm $(dst) 2>/dev/null

%: %.c
	$(CC) $(CFLAGS) $< $(LDFLAGS) -o $@
