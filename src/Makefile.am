
noinst_PROGRAMS = geonames-mkdb
lib_LTLIBRARIES = libgeonames.la

geonames_mkdb_SOURCES = geonames-mkdb.c
geonames_mkdb_CFLAGS = -Wall $(GIO_CFLAGS)
geonames_mkdb_LDADD = $(GIO_LIBS)

libgeonames_ladir = $(includedir)/geonames

libgeonames_la_SOURCES = \
	geonames.c \
	geonames-query.c geonames-query.h

libgeonames_la_HEADERS = geonames.h

nodist_libgeonames_la_SOURCES = geonames-resources.c
libgeonames_la_CFLAGS = -fvisibility=hidden -Wall -DPACKAGE=\"$(PACKAGE)\" $(GIO_CFLAGS)
libgeonames_la_LIBADD = $(GIO_LIBS)

geonames-resources.c: geonames.gresources.xml $(builddir)/cities.compiled
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) --target=$@ --generate-source $<

cities.compiled: geonames-mkdb
	$(AM_V_GEN) $(builddir)/geonames-mkdb $(top_srcdir)/data

pkgconfig_DATA = geonames.pc
pkgconfigdir = $(libdir)/pkgconfig

install-data-local: cities.compiled
	for po in $(builddir)/po/*.po; do \
		export target="$(DESTDIR)$(datadir)/locale/$$(basename $${po} .po)/LC_MESSAGES"; \
		mkdir -p "$${target}"; \
		msgfmt "$${po}" -o "$${target}/$(PACKAGE).mo"; \
	done

EXTRA_DIST = geonames.gresources.xml geonames.pc.in

CLEANFILES = geonames-resources.c cities.compiled geonames.pc

clean-local:
	-rm -rf po
