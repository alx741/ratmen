PROG       = vmenu
DISTFILES  = LICENSE Makefile README.md README.9menu vmenu.c $(EBUILDFILE)
OUTFILES   = $(PROG) $(TARFILE)

FULLNAME   = $(PROG)-$(VERSION)
TARFILE    = $(FULLNAME).tar.gz
EBUILDFILE = $(FULLNAME).ebuild

# extract version by running './vmenu --version'
VERSION = $(shell                     \
    if [ -e ./$(PROG) ]; then         \
    ./$(PROG) --version |             \
        awk "{                        \
            gsub(/\([^()]*\)/, \"\"); \
            gsub(/^[^0-9]*/, \"\");   \
            print \$$1;               \
            exit;                     \
        }";                           \
    else                              \
        echo XXX;                     \
    fi;                               \
)


PREFIX=/usr/local
MANDIR=$(PREFIX)/man

OPTIMIZE ?= -Os
DEBUG    ?=
WARN     ?= -Wall -ansi -pedantic

CC     = gcc
LIBS   = -L/usr/X11R6/lib -lX11
CFLAGS = $(OPTIMIZE) $(WARN) $(DEBUG)

all: $(PROG)

$(PROG): $(PROG).c
	$(CC) $(CFLAGS) $< $(LIBS) -o $@

clean:
	rm -f $(OUTFILES)

$(TARFILE): $(PROG)
	mkdir -p $(FULLNAME)
	cp -pr $(DISTFILES) $(FULLNAME)/
	tar czf $(TARFILE) $(FULLNAME)
	rm -rf $(FULLNAME)

dist: $(TARFILE)

dist_clean:
	rm -f $(TARFILE)

install: $(OUTFILES)
	install -D -p -m 755 -s $(PROG) $(PREFIX)/bin/$(PROG)
