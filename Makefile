# Makefile for ratmen
#
# Jonathan Walther
# djw@reactor-core.org
#
# modified by
# Zrajm C Akfohg
# ratmen-mail@klingonska.org

PROG       = ratmen
DISTFILES  = ChangeLog LICENSE Makefile README README.9menu debian ratmen.c $(EBUILDFILE)
OUTFILES   = $(PROG) $(PROG).1 $(TARFILE)

FULLNAME   = $(PROG)-$(VERSION)
TARFILE    = $(FULLNAME).tar.gz
EBUILDFILE = $(FULLNAME).ebuild
UPLOADDEST = psilocybe.update.uu.se:public_html/programs/$(PROG)

# extract version by running './ratmen --version'
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

all: doc $(PROG)

$(PROG): $(PROG).c
	$(CC) $(CFLAGS) $< $(LIBS) -o $@

# FIXME: $(PROG) should be uppercased when used with `-n' below
$(PROG).1: $(PROG).c
	pod2man -s1 -c '' -n$(PROG) -r"Zrajm C Akfohg" $(PROG).c $(PROG).1

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

doc: $(PROG).1

install: $(OUTFILES)
	install -D -p -m 755 -s $(PROG) $(PREFIX)/bin/$(PROG)
	install -D -p -m 755 $(PROG).1 $(MANDIR)/man1/$(PROG).1

upload: dist
	scp -p $(TARFILE) $(EBUILDFILE) $(UPLOADDEST)

test: $(PROG)
	cd ./tests && ./gen_menu.sh test 100

#[[eof]]
