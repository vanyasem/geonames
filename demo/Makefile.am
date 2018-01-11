
bin_PROGRAMS = geonames-demo

geonames_demo_SOURCES = \
	geonames-demo.c

geonames_demo_CFLAGS = \
	-Wall \
	$(GTK_CFLAGS) \
	-I$(top_srcdir)/src

geonames_demo_LDADD = \
	$(GTK_LIBS) \
	$(top_builddir)/src/libgeonames.la
