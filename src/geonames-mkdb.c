/*
 * Copyright 2015-2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <gio/gio.h>
#include <glib/gstdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>

enum
{
  ADMIN1_CODE = 0,
  ADMIN1_NAME,
  ADMIN1_ASCII_NAME,
  ADMIN1_ID
};

enum
{
  CITIES_ID = 0,
  CITIES_NAME,
  CITIES_ASCIINAME,
  CITIES_ALTERNATE_NAMES,
  CITIES_LATITUDE,
  CITIES_LONGITUDE,
  CITIES_FEATURE_CLASS,
  CITIES_FEATURE_CODE,
  CITIES_COUNTRY_CODE,
  CITIES_ALTERNATE_COUNTRY_CODES,
  CITIES_ADMIN1,
  CITIES_ADMIN2,
  CITIES_ADMIN3,
  CITIES_ADMIN4,
  CITIES_POPULATION,
  CITIES_ELEVATION,
  CITIES_DEM,
  CITIES_TIMEZONE,
  CITIES_MODIFICATION_DATE
};

enum
{
  ALTERNATES_ID = 0,
  ALTERNATES_CITY_ID,
  ALTERNATES_LANG,
  ALTERNATES_NAME,
  ALTERNATES_IS_PREFERRED,
  ALTERNATES_IS_SHORT,
  ALTERNATES_IS_COLLOQUIAL,
  ALTERNATES_IS_HISTORIC
};

enum
{
  COUNTRIES_ISO,
  COUNTRIES_ISO3,
  COUNTRIES_ISO_NUMERIC,
  COUNTRIES_FIPS,
  COUNTRIES_NAME,
  COUNTRIES_CAPITAL,
  COUNTRIES_AREA,
  COUNTRIES_POPULATION,
  COUNTRIES_CONTINENT,
  COUNTRIES_TLD,
  COUNTRIES_CURRENCYCODE,
  COUNTRIES_CURRENCYNAME,
  COUNTRIES_PHONE,
  COUNTRIES_POSTAL_CODE_FORMAT,
  COUNTRIES_POSTAL_CODE_REGEX,
  COUNTRIES_LANGUAGES,
  COUNTRIES_ID,
  COUNTRIES_NEIGHBOURS,
  COUNTRIES_EQUIVALENTFIPSCODE
};

typedef struct
{
  GHashTable *admin1;
  GHashTable *admin1_ids;
  GHashTable *countries;
  GHashTable *countries_ids;
  GHashTable *cities_ids;
  GHashTable *alternates;
  GVariantBuilder builder;
} CityData;

static GVariant *
variant_new_normalize_string (const gchar *str)
{
  gchar *normalized;

  normalized = g_utf8_normalize (str, -1, G_NORMALIZE_ALL_COMPOSE);

  return g_variant_new_take_string (normalized);
}

static void
ensure_english_translation (CityData *data, const gchar *id, const gchar *en_name)
{
  GHashTable *places;

  places = g_hash_table_lookup (data->alternates, "en");
  if (places == NULL)
    {
      places = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
      g_hash_table_insert (data->alternates, g_strdup ("en"), places);
    }
  if (!g_hash_table_contains (places, id))
    {
      g_hash_table_insert (places, g_strdup (id),
                           g_utf8_normalize (en_name, -1, G_NORMALIZE_ALL_COMPOSE));
    }
}

static const gchar *
get_english_translation (CityData *data, const gchar *id)
{
  GHashTable *places;
  const gchar *translation = NULL;

  places = g_hash_table_lookup (data->alternates, "en");
  if (places)
    translation = g_hash_table_lookup (places, id);

  return translation ? translation : "";
}

static void
handle_admin1_line (gchar    **fields,
                    gpointer   user_data,
                    GError   **error)
{
  CityData *data = user_data;

  ensure_english_translation (data, fields[ADMIN1_ID], fields[ADMIN1_NAME]);

  g_hash_table_insert (data->admin1, g_strdup (fields[ADMIN1_CODE]), g_strdup (fields[ADMIN1_ID]));
  g_hash_table_insert (data->admin1_ids, g_strdup (fields[ADMIN1_ID]), g_strdup (fields[ADMIN1_CODE]));
}

static void
handle_country_line (gchar    **fields,
                     gpointer   user_data,
                     GError   **error)
{
  CityData *data = user_data;

  ensure_english_translation (data, fields[COUNTRIES_ID], fields[COUNTRIES_NAME]);

  g_hash_table_insert (data->countries, g_strdup (fields[COUNTRIES_ISO]), g_strdup (fields[COUNTRIES_ID]));
  g_hash_table_insert (data->countries_ids, g_strdup (fields[COUNTRIES_ID]), g_strdup (fields[COUNTRIES_ISO]));
}

static void
handle_alternates_line (gchar    **fields,
                        gpointer   user_data,
                        GError   **error)
{
  CityData *data = user_data;
  GHashTable *places;
  gchar *lang;

  if ((strchr (fields[ALTERNATES_LANG], '-') == NULL &&
       (strlen (fields[ALTERNATES_LANG]) < 2 ||
        strlen (fields[ALTERNATES_LANG]) > 3)) ||
      (g_strcmp0 (fields[ALTERNATES_IS_PREFERRED], "1") != 0 &&
       (g_strcmp0 (fields[ALTERNATES_IS_SHORT], "1") == 0 ||
        g_strcmp0 (fields[ALTERNATES_IS_COLLOQUIAL], "1") == 0 ||
        g_strcmp0 (fields[ALTERNATES_IS_HISTORIC], "1") == 0)))
    {
      return;
    }

  lang = g_strdelimit (fields[ALTERNATES_LANG], "-", '_');

  places = g_hash_table_lookup (data->alternates, lang);
  if (places == NULL)
    {
      places = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
      g_hash_table_insert (data->alternates, g_strdup (lang), places);
    }

  /* Only replace a translation if we are currently processing the preferred version */
  if (g_hash_table_contains (places, fields[ALTERNATES_CITY_ID]) &&
      g_strcmp0 (fields[ALTERNATES_IS_PREFERRED], "1") != 0)
    return;

  g_hash_table_insert (places,
                       g_strdup (fields[ALTERNATES_CITY_ID]),
                       g_utf8_normalize (fields[ALTERNATES_NAME], -1, G_NORMALIZE_ALL_COMPOSE));
}

static void
handle_city_line (gchar    **fields,
                  gpointer   user_data,
                  GError   **error)
{
  CityData *data = user_data;
  g_autofree gchar *index = NULL;
  gchar *admin1_id;
  gchar *country_id;

  /* only include cities and villages and ignore sections of other places (PPLX) */
  if (fields[CITIES_FEATURE_CLASS][0] != 'P' ||
      g_str_equal (fields[CITIES_FEATURE_CODE], "PPLX"))
    return;

  /* The documentation states that "00" is used for cities without a
   * specified admin1 zone. However, it is sometimes set to the empty
   * string (or even other, non-existing codes). This tool is not useful
   * for integrity checks anyway, so just ignore anything that's not in
   * admin1Codes.txt
   */
  index = g_strdup_printf ("%s.%s", fields[CITIES_COUNTRY_CODE], fields[CITIES_ADMIN1]);
  admin1_id = g_hash_table_lookup (data->admin1, index);
  if (!admin1_id)
    return;

  /* However, do discard cities without associated countries */
  country_id = g_hash_table_lookup (data->countries, fields[CITIES_COUNTRY_CODE]);
  if (!country_id)
    return;

  g_hash_table_add (data->cities_ids, g_strdup (fields[CITIES_ID]));

  ensure_english_translation (data, fields[CITIES_ID], fields[CITIES_NAME]);

  g_variant_builder_add (&data->builder, "(@ss@ss@ss@sudd)",
                         variant_new_normalize_string (fields[CITIES_ID]),
                         get_english_translation (data, fields[CITIES_ID]),
                         variant_new_normalize_string (fields[CITIES_ADMIN1]),
                         get_english_translation (data, admin1_id),
                         variant_new_normalize_string (fields[CITIES_COUNTRY_CODE]),
                         get_english_translation (data, country_id),
                         variant_new_normalize_string (fields[CITIES_TIMEZONE]),
                         strtoul (fields[CITIES_POPULATION], NULL, 10),
                         g_ascii_strtod (fields[CITIES_LATITUDE], NULL),
                         g_ascii_strtod (fields[CITIES_LONGITUDE], NULL));
}

static gboolean
parse_geo_names_file (GFile     *file,
                      guint      n_columns,
                      void     (*callback) (gchar **, gpointer, GError **),
                      gpointer   user_data,
                      GError   **error)
{
  g_autoptr(GFileInputStream) filestream = NULL;
  g_autoptr(GDataInputStream) datastream = NULL;
  guint line_nr = 0;

  filestream = g_file_read (file, NULL, error);
  if (filestream == NULL)
    return FALSE;

  datastream = g_data_input_stream_new (G_INPUT_STREAM (filestream));

  for (;;)
    {
      g_autofree gchar *line = NULL;
      gsize length;
      g_auto(GStrv) fields = NULL;

      line = g_data_input_stream_read_line_utf8 (datastream, &length, NULL, error);
      if (line == NULL)
        return length == 0 ? TRUE : FALSE;

      line_nr++;

      if (line[0] == '#')
        continue;

      fields = g_strsplit (line, "\t", 0);
      if (g_strv_length (fields) != n_columns)
        {
          g_set_error (error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA,
                       "line %u doesn't contain %u fields", line_nr, n_columns);
          return FALSE;
        }

      callback (fields, user_data, error);
    }

  return TRUE;
}

void
write_po_file (CityData *data, const gchar *lang, GHashTable *translations)
{
  g_autofree gchar *path;
  g_autoptr(GIOChannel) po;
  g_autoptr(GError) error = NULL;
  GHashTableIter iter;
  gchar *id;
  gchar *translation;
  GHashTable *base_lang = NULL;

  path = g_strdup_printf ("po/%s.po", lang);
  po = g_io_channel_new_file (path, "w+", &error);
  if (!po)
    {
      g_printerr ("Unable to write po file '%s': %s\n", path, error->message);
      return;
    }

  g_io_channel_write_chars (po, "msgid \"\"\n", -1, NULL, NULL);
  g_io_channel_write_chars (po, "msgstr \"\"\n", -1, NULL, NULL);
  g_io_channel_write_chars (po, "\"Project-Id-Version: geonames\\n\"\n", -1, NULL, NULL);
  g_io_channel_write_chars (po, "\"MIME-Version: 1.0\\n\"\n", -1, NULL, NULL);
  g_io_channel_write_chars (po, "\"Content-Type: text/plain; charset=UTF-8\\n\"\n", -1, NULL, NULL);
  g_io_channel_write_chars (po, "\"Content-Transfer-Encoding: 8bit\\n\"\n", -1, NULL, NULL);

  if (strchr (lang, '_') != NULL)
    {
      g_autofree gchar *base_lang_name;
      base_lang_name = g_strdelimit (g_strdup (lang), "_", '\0');
      base_lang = g_hash_table_lookup (data->alternates, base_lang_name);
    }

  g_hash_table_iter_init (&iter, translations);
  while (g_hash_table_iter_next (&iter, (gpointer*)&id, (gpointer*)&translation))
    {
      g_auto(GStrv) tokens_slash = NULL;
      g_auto(GStrv) tokens_quote = NULL;
      g_autofree gchar *translation_slashed = NULL;
      g_autofree gchar *translation_quoted = NULL;
      gchar *code;

      code = g_hash_table_lookup (data->admin1_ids, id);
      if (code == NULL)
        code = g_hash_table_lookup (data->countries_ids, id);
      if (code == NULL)
        {
          if (!g_hash_table_contains (data->cities_ids, id))
            continue; // an airport or some other translation we don't care about
          code = id;
        }

      // optimize a tiny amount by skipping country-specific translations that aren't really specific
      if (base_lang && g_strcmp0 (g_hash_table_lookup (base_lang, id), translation) == 0)
        continue;

      // Is there not a nicer glib way of replacing parts of strings?
      tokens_slash = g_strsplit (translation, "\\", -1);
      translation_slashed = g_strjoinv ("\\\\", tokens_slash);
      tokens_quote = g_strsplit (translation_slashed, "\"", -1);
      translation_quoted = g_strjoinv ("\\\"", tokens_quote);

      g_io_channel_write_chars (po, "\n", -1, NULL, NULL);
      g_io_channel_write_chars (po, "msgid \"", -1, NULL, NULL);
      g_io_channel_write_chars (po, code, -1, NULL, NULL);
      g_io_channel_write_chars (po, "\"\n", -1, NULL, NULL);
      g_io_channel_write_chars (po, "msgstr \"", -1, NULL, NULL);
      g_io_channel_write_chars (po, translation_quoted, -1, NULL, NULL);
      g_io_channel_write_chars (po, "\"\n", -1, NULL, NULL);
    }
}

void
write_po_files (CityData *data)
{
  GHashTableIter iter;
  GHashTable *translations;
  gchar *lang;

  g_mkdir ("po", 0755);

  g_hash_table_iter_init (&iter, data->alternates);
  while (g_hash_table_iter_next (&iter, (gpointer*)&lang, (gpointer*)&translations))
    {
      write_po_file (data, lang, translations);
    }
}

int
main (int argc, char **argv)
{
  g_autoptr(GFile) dir = NULL;
  g_autoptr(GFile) admin1_file = NULL;
  g_autoptr(GFile) countries_file = NULL;
  g_autoptr(GFile) cities_file = NULL;
  g_autoptr(GFile) alternates_file = NULL;
  g_autoptr(GError) error = NULL;
  g_autoptr(GVariant) v = NULL;
  CityData data;

  setlocale (LC_ALL, "");

  dir = g_file_new_for_path (argc == 2 ? argv[1] : ".");
  admin1_file = g_file_get_child (dir, "admin1Codes.txt");
  countries_file = g_file_get_child (dir, "countryInfo.txt");
  cities_file = g_file_get_child (dir, "cities15000.txt");
  alternates_file = g_file_get_child (dir, "alternateNames.txt");

  data.admin1 = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
  data.admin1_ids = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
  data.countries = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
  data.countries_ids = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
  data.cities_ids = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
  data.alternates = g_hash_table_new_full (g_str_hash, g_str_equal, g_free,
                                           (GDestroyNotify)g_hash_table_unref);
  g_variant_builder_init (&data.builder, G_VARIANT_TYPE ("a(sssssssudd)"));

  if (!parse_geo_names_file (alternates_file, 8, handle_alternates_line, &data, &error))
    {
      g_printerr ("Unable to read alternates file: %s\n", error->message);
      return 1;
    }

  if (!parse_geo_names_file (admin1_file, 4, handle_admin1_line, &data, &error))
    {
      g_printerr ("Unable to read admin file: %s\n", error->message);
      return 1;
    }

  if (!parse_geo_names_file (countries_file, 19, handle_country_line, &data, &error))
    {
      g_printerr ("Unable to read countries file: %s\n", error->message);
      return 1;
    }

  if (!parse_geo_names_file (cities_file, 19, handle_city_line, &data, &error))
    {
      g_printerr ("Unable to read cities file: %s\n", error->message);
      return 1;
    }

  v = g_variant_new ("a(sssssssudd)", &data.builder);

  if (!g_file_set_contents ("cities.compiled", g_variant_get_data (v), g_variant_get_size (v), &error))
    {
      g_printerr ("Unable to write output: %s\n", error->message);
      return 1;
    }

  write_po_files (&data);

  g_hash_table_unref (data.admin1);
  g_hash_table_unref (data.admin1_ids);
  g_hash_table_unref (data.countries);
  g_hash_table_unref (data.countries_ids);
  g_hash_table_unref (data.cities_ids);
  g_hash_table_unref (data.alternates);

  return 0;
}
