# $Id: Makefile,v 1.5 2001/08/12 18:25:13 prahl Exp $
# History:
# $Log: Makefile,v $
# Revision 1.5  2001/08/12 18:25:13  prahl
# latex2rtf version 1.9c
#
# 	Added support for \frac
# 	Complete support for all characters in the symbol font now
# 	Better support for unusual ansi characters (e.g., \dag and \ddag)
# 	Gave direct.cfg a spring cleaning
# 	Added support for \~n and \~N
# 	New file oddchars.tex for testing many of these translations.
# 	New file frac.tex to test \frac and \over
# 	Removed a lot of annoying warning messages that weren't very helpful
#
# Revision 1.21  1998/07/03 06:49:36  glehner
# updated dependencies of multiple .o files
# Substantial Changes in Documentation
# doc/ directory holds texinfo manual, added GNU GPL
# created cfg/directory for support files
# Added CangeLog to SOURCES
#
# Revision 1.20  1998/06/08 19:14:01  ralf
# Corrected clean target.
#
# Revision 1.19  1998/06/08 17:58:30  ralf
# removed second version in SOURCEFILES.
#
# Revision 1.18  1998/06/08 17:57:37  ralf
# Added TODO, credits and version.
#
# Revision 1.17  1997/02/15 21:17:42  ralf
# Created default for environment separator.
#
# Revision 1.16  1997/02/15 21:10:02  ralf
# Added environment separator to XCFLAGS used in cfg.c
#
# Revision 1.15  1997/02/15 20:33:25  ralf
# Added debian rules and corrected some targets.
#
# Revision 1.14  1995/05/24  16:10:45  ralf
# Added rules for additional files for DOS port
#
# Revision 1.13  1995/05/24  12:00:43  ralf
# Corrected dependencies, added LIBS for configuring system libraries
#
# Revision 1.12  1995/03/23  17:19:27  ralf
# Changed installation default to not automatically remove .cfg files
#
# Revision 1.11  1995/03/23  16:20:27  ralf
# changed the LIBDIR default
#
# Revision 1.10  1995/03/23  15:58:08  ralf
# Reworked version by Friedrich Polzer and Gerhard Trisko
#
# Revision 1.9  1994/07/13  09:27:31  ralf
# Corrected fpos/SEEK_SET bug for SunOs 4.3.1 reported by Ulrich Schmid
# <schmid@dkrz.d400.de>
#
# Revision 1.8  1994/06/21  08:13:57  ralf
# Added CFLAGS
#
# Revision 1.7  1994/06/17  15:13:44  ralf
# Added README and Copyright
#
# Revision 1.6  1994/06/17  14:35:46  ralf
# Added Makefile to SOURCES
#
# Revision 1.5  1994/06/17  14:32:29  ralf
# Added latex2rtf.tar.gz to clean list.
#
# Revision 1.4  1994/06/17  14:29:42  ralf
# Added version.h to dependecy list of main.o and to SOURCES
# Added dist target
#
# Revision 1.3  1994/06/17  14:19:41  ralf
# Corrected various bugs, for example interactive read of arguments
#
# Revision 1.2  1994/06/17  12:11:57  ralf
# Added intall target and rcs rules.
#
# Revision 1.1  1994/06/17  11:30:33  ralf
# Initial revision
#
# The Debian-specific parts of this Makefile are created by 
# Erick Branderhorst. Parts are written by Ian Jackson and Ian Murdock.
# TODO: add target "changes". 
CC=gcc    # C-Compiler 
CFLAGS=-g $(XCFLAGS) # Use -O here if you want it optimized
COPY=cp
INSTALL=install
DIR_MODE=755
BIN_MODE=755
DAT_MODE=644
DIR_USER=root
BIN_USER=root
DAT_USER=root
DIR_GROUP=root
BIN_GROUP=root
DAT_GROUP=root
# If you have the program install, use the following definitions
INST_DIR=$(INSTALL) -g $(DIR_GROUP) -o $(DIR_USER) -d -m $(BIN_MODE)
INST_BIN=$(INSTALL) -g $(BIN_GROUP) -o $(BIN_USER) -m $(DIR_MODE)
INST_DAT=$(INSTALL) -g $(DAT_GROUP) -o $(DAT_USER) -m $(DAT_MODE)
CHOWN_DIR=true
CHOWN_BIN=true
CHOWN_DAT=true
CHMOD_DIR=true
CHMOD_BIN=true
CHMOD_DAT=true
# If you do not have install, comment the definitions above and
# uncomment the following definitions
# If your mkdir does not support the -p option, you may want to create
# directories by hand and define INST_DIR=true or use the target
# simple_install instead.
# INST_BIN=$(COPY)
# INST_DIR=mkdir -p
# INST_DAT=$(COPY)
# CHOWN_DIR=chown $(DIR_USER).$(DIR_GROUP)
# CHOWN_BIN=chown $(BIN_USER).$(BIN_GROUP)
# CHOWN_DAT=chown $(DAT_USER).$(DAT_GROUP)
# CHMOD_DIR=chmod $(DIR_MODE)
# CHMOD_BIN=chmod $(DIR_MODE)
# CHMOD_DAT=chmod $(DIR_MODE)
#
# Note: If install doesn't work for you, use simple_install instead.
#
# Where support files are searched for by the executable
# prefix defaults to /usr/local, but may be set on the command line
prefix=/usr/local
LIBDIR=$(prefix)/lib/latex2rtf
# You can give several Directories separated by ':' for the following
# install targets
#
# Where supportfiles are installed should normally be the same as LIBDIR
# If you specify SEVERAL directories here, the files will get installed
# into EVERY directory. This is rather useful if you have mirrored images
# that you want to update, but is normally not necessary for a normal
# installation.
#LIBINSTALL=/quasi/local/lib/latex2rtf:/oberon/local/lib/latex2rtf
LIBINSTALL=$(LIBDIR)
# Where Binaries are installed
#BININSTALL=/quasi/local/bin:/oberon/local/bin
BININSTALL=$(prefix)/bin
MANINSTALL=$(prefix)/man/man1

# The following should fix compatibility problems on some machines, you
# may add the following option to XCFLAGS
# -DHAS_NO_FPOS for SunOs 4.1.3 (Thanks to Ulrich Schmid schmid@dkrz.d400.de)
#
# If you are using MSDOS, the environment separator ENVSEP shoud be
# ';' and PATHSEP '\'.
# If not specified it defaults to ':' and '/' (UNIX standard)

# If your target/system has no getopt() function, use Vladimir Menkov's
# instead, found in mygetopt.c.
# Add -DHAS_NO_FPOS

XCFLAGS=
#XCFLAGS=-DENVSEP="':'"
#XCFLAGS=-DPATHSEP="'/'"
#XCFLAGS=-DENVSEP="';'"
#XCFLAGS=-DPATHSEP="'\'"
#XCFLAGS=-DHAS_NO_FPOS
#XCFLAGS=-DHAS_NO_GETOPT

# Sometimes additional system libraries are needed, they can be defined
# here
# LIBS=libdebug_malloc.a
LIBS=

# Nothing to change below this line
SOURCES=commands.c commands.h direct.c direct.h encode.c encode.h l2r_fonts.c \
    l2r_fonts.h funct1.c funct1.h funct2.c funct2.h ignore.c ignore.h main.c \
    main.h stack.c stack.h version.h cfg.c cfg.h Makefile README README.DOS\
    Copyright mygetopt.c optind.c version debian.README \
    debian.control debian.rules util.c util.h  ChangeLog parser.c parser.h
SUPPORT=cfg/direct.cfg cfg/fonts.cfg cfg/ignore.cfg \
    cfg/english.cfg cfg/german.cfg cfg/spanish.cfg \
    l2r.bat l2r.exe
MANUALS=latex2rtf.1
MSDOS=l2r.bat l2r.exe
DOCS=doc/latex2rtf.info doc/l2r.html doc/l2r.pdf doc/l2r.txt\
     doc/TODO doc/credits doc/copying.txt doc/Makefile

ARCH="`dpkg --print-architecture`"

# Some defines for versions
VERSION="`./version`"

all build stamp-build: checkdir latex2rtf
	touch stamp-build

latex2rtf: l2r_fonts.o direct.o encode.o commands.o stack.o funct1.o funct2.o \
	ignore.o cfg.o main.o util.o parser.o mygetopt.o
	$(CC) $(CFLAGS) l2r_fonts.o direct.o encode.o commands.o stack.o \
	funct1.o funct2.o cfg.o main.o ignore.o util.o parser.o mygetopt.o \
	$(LIBS) -o latex2rtf

l2r_fonts.o: l2r_fonts.c main.h l2r_fonts.h cfg.h
	$(CC) $(CFLAGS) -c l2r_fonts.c -o l2r_fonts.o

direct.o: direct.c main.h direct.h l2r_fonts.h cfg.h
	$(CC) $(CFLAGS) -c direct.c -o direct.o

stack.o: stack.c stack.h
	$(CC) $(CFLAGS) -c stack.c -o stack.o

funct1.o: funct1.c main.h funct1.h funct2.h commands.h stack.h l2r_fonts.h cfg.h ignore.h util.h encode.h
	$(CC) $(CFLAGS) -c funct1.c -o funct1.o

funct2.o: funct2.c main.h funct1.h commands.h funct2.h stack.h cfg.h util.h
	$(CC) $(CFLAGS) -c funct2.c -o funct2.o

ignore.o: ignore.c main.h direct.h l2r_fonts.h cfg.h ignore.h util.h
	$(CC) $(CFLAGS) -c ignore.c -o ignore.o

encode.o: encode.c encode.h main.h funct1.h l2r_fonts.h
	$(CC) $(CFLAGS) -c encode.c -o encode.o

cfg.o: cfg.c cfg.h util.h
	$(CC) $(CFLAGS) -DLIBDIR=\"$(LIBDIR)\" -c cfg.c -o cfg.o

util.o: util.c util.h
	$(CC) $(CFLAGS) -c util.c -o util.o

main.o: main.c main.h commands.h funct1.h l2r_fonts.h stack.h funct2.h \
	direct.h ignore.h version.h cfg.h encode.h util.h
	$(CC) $(CFLAGS) -c main.c -o main.o

commands.o: commands.c main.h funct1.h commands.h funct2.h
	$(CC) $(CFLAGS) -c commands.c -o commands.o

parser.o: parser.c parser.h main.h
	$(CC) $(CFLAGS) -c parser.c  -o parser.o

mygetopt.o: mygetopt.c main.h
	$(CC) $(CFLAGS) -c mygetopt.c  -o mygetopt.o

change.log: ChangeLog
	cp ChangeLog change.log

doc:	checkdir change.log
	cd doc ; make -k

clean: checkdir
	rm -f stack.o main.o funct1.o funct2.o ignore.o commands.o mygetopt.o\
	    encode.o direct.o l2r_fonts.o cfg.o util.o core latex2rtf.tar.gz \
	    *~ ./#* stamp-build latex2rtf-$(VERSION).tar.gz __tmp__ \
	    *.deb
	rm -rf latex2rtf latex2rtf-$(VERSION)
	rm -rf debian-tmp
	cd doc ; make almostclean

$(SOURCES) $(SUPPORT) $(MANUALS):
	cp $@

checkout checkdir: $(SOURCES) $(SUPPORT) $(MANUALS)

dist source: $(SOURCES) $(SUPPORT) $(MANUALS) $(DOCS) $(MSDOS) clean
	mkdir latex2rtf-$(VERSION)
	mkdir latex2rtf-$(VERSION)/cfg
	mkdir latex2rtf-$(VERSION)/doc
	ln $(SOURCES) $(MANUALS) $(MSDOS) latex2rtf-$(VERSION)
	ln $(SUPPORT) latex2rtf-$(VERSION)/cfg
	ln $(DOCS) latex2rtf-$(VERSION)/doc
	tar cvf - latex2rtf-$(VERSION) | \
	    gzip -best > latex2rtf-$(VERSION).tar.gz
	rm -rf latex2rtf-$(VERSION)

install_and_delete_old_cfg: $(SUPPORT)
	IFS=: ; for i in $(LIBINSTALL) ; do \
	    $(INST_DIR) $$i; \
	    $(CHOWN_DIR) $$i; \
	    $(CHMOD_DIR) $$i; \
	    for j in $(SUPPORT) ; do \
		rm -f $$i/$$j; \
		$(INST_DAT) $$j $$i; \
		$(CHOWN_DAT) $$i/$$j; \
		$(CHMOD_DAT) $$i/$$j; \
	    done ;\
	done

complex_install: latex2rtf install.man
	IFS=: ; for i in $(BININSTALL) ; do \
	    $(INST_DIR) $$i; \
	    $(CHOWN_DIR) $$i; \
	    $(CHMOD_DIR) $$i; \
	    $(INST_BIN) latex2rtf $$i; \
	    $(CHOWN_BIN) $$i/latex2rtf; \
	    $(CHMOD_BIN) $$i/latex2rtf; \
	done

install.man: $(MANUALS)
	IFS=: ; for i in $(MANINSTALL) ; do \
	    $(INST_DIR) $$i; \
	    $(CHOWN_DIR) $$i; \
	    $(CHMOD_DIR) $$i; \
	    for j in $(MANUALS) ; do \
		rm -f $$i/$$j; \
		$(INST_DAT) $$j $$i; \
		$(CHOWN_DAT) $$i/$$j; \
		$(CHMOD_DAT) $$i/$$j; \
	    done ;\
	done

simple_cfg_install: $(SUPPORT)
	$(INST_DAT) $(SUPPORT) $(LIBINSTALL)

simple_install: $(MANUALS) latex2rtf
	-mkdir $(LIBINSTALL)
	$(CHOWN_DIR) $(LIBINSTALL)
	$(CHMOD_DIR) $(LIBINSTALL)
	$(INST_BIN) latex2rtf $(BININSTALL)
	$(CHOWN_BIN) $(BININSTALL)/latex2rtf
	$(CHMOD_BIN) $(BININSTALL)/latex2rtf
	$(INST_DAT) $(MANUALS) $(MANINSTALL)

install: complex_install

.PHONY: install complex_install simple_install simple_cfg_install \
	install.man complex_install install_and_delete_old_cfg dist \
	all clean checkout build checkdir diff checkroot binary source

# Debian-specific targets:

checkroot:
	test root = "`whoami`"

diff:
	echo "Debian release is maintained by source maintainer. No changes" | \
	gzip -best > latex2rtf-$(VERSION).diff

binary: checkroot debian.README install install_and_delete_old_cfg
	$(INST_DIR) debian-tmp/DEBIAN
	sed -e '2s/=/'$(VERSION)'/; 3s/=/'$(ARCH)/ debian.control > __tmp__
	$(INST_DAT) __tmp__ debian-tmp/DEBIAN/control
	rm -f __tmp__
	$(INST_DIR) debian-tmp/usr/doc/copyright
	cat debian.README Copyright > __tmp__
	$(INST_DAT) __tmp__ debian-tmp/usr/doc/copyright/latex2rtf
	rm -f __tmp__
	dpkg --build debian-tmp
	mv debian-tmp.deb latex2rtf-$(VERSION).$(ARCH).deb
	rm -rf debian-tmp	
