
check_PROGRAMS = test-geonames

test_geonames_SOURCES = \
	test-geonames.c

test_geonames_CFLAGS = \
	-Wall $(GIO_CFLAGS) \
	-DPACKAGE=\"$(PACKAGE)\" \
	-DLOCALEDIR=\"$(abs_builddir)/locales\" \
	-I$(top_srcdir)/src

test_geonames_LDADD = $(GIO_LIBS) $(top_srcdir)/src/libgeonames.la

AM_TESTS_ENVIRONMENT = \
	if [ ! -d locales ]; then \
		mkdir -p locales; \
		for po in fr zh zh_TW; do \
			mkdir -p "locales/$${po}/LC_MESSAGES"; \
			msgfmt "$(top_srcdir)/src/po/$${po}.po" -o "locales/$${po}/LC_MESSAGES/$(PACKAGE).mo"; \
		done; \
	fi;

LOG_COMPILER = gtester
TESTS = $(check_PROGRAMS)

clean-local:
	-rm -rf locales
