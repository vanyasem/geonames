DOC_MODULE=geonames
DOC_MAIN_SGML_FILE=$(DOC_MODULE)-docs.xml
DOC_SOURCE_DIR=$(top_srcdir)/src

MKDB_OPTIONS=--xml-mode --output-format=xml

HFILE_GLOB = $(top_srcdir)/src/*.h
CFILE_GLOB = $(top_srcdir)/src/*.c

GTKDOC_LIBS = $(GIO_LIBS) $(top_builddir)/src/libgeonames.la

include $(top_srcdir)/gtk-doc.make
