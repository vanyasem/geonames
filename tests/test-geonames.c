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
 *
 * Authors:
 *     Lars Uebernickel <lars.uebernickel@canonical.com>
 */

#include <gio/gio.h>
#include <libintl.h>
#include <locale.h>
#include <geonames.h>

static void
change_lang (const gchar *lang)
{
  g_setenv("LANGUAGE", lang, TRUE);
  setlocale (LC_ALL, "");
}

static void
assert_contains (const gchar *query,
                 const gchar *expected_city,
                 const gchar *expected_country_code,
                 gdouble expected_latitude,
                 gdouble expected_longitude,
                 guint expected_population)
{
  g_autofree gint *indices;
  guint i, len;
  g_autoptr(GeonamesCity) city = NULL;

  indices = geonames_query_cities_sync (query, GEONAMES_QUERY_DEFAULT, &len, NULL, NULL);
  g_assert (indices);
  g_assert_cmpint (indices[len], ==, -1);

  g_assert_cmpint (len, >, 0);

  for (i = 0; i < len; ++i) {
    city = geonames_get_city (indices[i]);

    if (g_strcmp0(geonames_city_get_name (city), expected_city) == 0) {
      g_assert_cmpstr (geonames_city_get_country_code (city), ==, expected_country_code);
      g_assert_cmpfloat (geonames_city_get_latitude (city), ==, expected_latitude);
      g_assert_cmpfloat (geonames_city_get_longitude (city), ==, expected_longitude);
      g_assert_cmpint (geonames_city_get_population (city), ==, expected_population);
      break;
    }
  }
  g_assert (i < len);
}


static void
assert_first_stats (const gchar *query,
                    const gchar *expected_city,
                    const gchar *expected_country_code,
                    gdouble expected_latitude,
                    gdouble expected_longitude,
                    guint expected_population)
{
  g_autofree gint *indices;
  guint len;
  g_autoptr(GeonamesCity) city = NULL;

  indices = geonames_query_cities_sync (query, GEONAMES_QUERY_DEFAULT, &len, NULL, NULL);
  g_assert (indices);
  g_assert_cmpint (indices[len], ==, -1);

  g_assert_cmpint (len, >, 0);
  city = geonames_get_city (indices[0]);

  g_assert_cmpstr (geonames_city_get_name (city), ==, expected_city);
  g_assert_cmpstr (geonames_city_get_country_code (city), ==, expected_country_code);
  g_assert_cmpfloat (geonames_city_get_latitude (city), ==, expected_latitude);
  g_assert_cmpfloat (geonames_city_get_longitude (city), ==, expected_longitude);
  g_assert_cmpint (geonames_city_get_population (city), ==, expected_population);
}

static void
assert_first_names (const gchar *query,
                    const gchar *expected_city,
                    const gchar *expected_state,
                    const gchar *expected_country)
{
  g_autofree gint *indices;
  guint len;
  g_autoptr(GeonamesCity) city = NULL;

  indices = geonames_query_cities_sync (query, GEONAMES_QUERY_DEFAULT, &len, NULL, NULL);
  g_assert (indices);
  g_assert_cmpint (indices[len], ==, -1);

  g_assert_cmpint (len, >, 0);
  city = geonames_get_city (indices[0]);

  g_assert_cmpstr (geonames_city_get_name (city), ==, expected_city);
  g_assert_cmpstr (geonames_city_get_state (city), ==, expected_state);
  g_assert_cmpstr (geonames_city_get_country (city), ==, expected_country);
}

static void
test_common_cities (void)
{
  assert_first_stats ("berlin", "Berlin", "DE", 52.52437, 13.41053, 3426354);
  assert_first_stats ("new york", "New York", "US", 40.71427, -74.00597, 8175133);
  assert_first_stats ("san fran", "San Francisco", "US", 37.77493, -122.41942, 805235);
  assert_first_stats ("amster", "Amsterdam", "NL", 52.37403, 4.88969, 741636);
  assert_first_stats ("montreal", "Montreal", "CA", 45.50884, -73.58781, 1600000);
}

static void
test_translations (void)
{
  change_lang ("en_US");
  assert_first_names ("bos", "Boston", "Massachusetts", "United States of America");
  assert_first_names ("san fr", "San Francisco", "California", "United States of America");
  assert_first_names ("montre", "Montreal", "Quebec", "Canada");
  assert_first_names ("regin", "Regina", "Saskatchewan", "Canada");

  change_lang ("fr_CA");
  assert_first_names ("bos", "Boston", "Massachusetts", "États-Unis");
  assert_first_names ("san fr", "San Francisco", "Californie", "États-Unis");
  assert_first_names ("montre", "Montréal", "Québec", "Canada");
  assert_first_names ("montré", "Montréal", "Québec", "Canada");
  assert_first_names ("regin", "Régina", "Saskatchewan", "Canada");

  change_lang ("zh_TW");
  assert_first_names ("bos", "波士顿", "麻薩諸塞州", "美国");
  assert_first_names ("里賈", "里賈納", "薩斯喀徹溫", "加拿大");

  change_lang ("zh");
  assert_first_names ("bos", "波士顿", "麻薩諸塞州", "美国");
  assert_first_names ("里贾", "里贾纳", "薩斯喀徹溫", "加拿大"); // different than above

  change_lang ("C");
}

static void
test_cities_without_some_words (void)
{
    assert_contains ("hospitalet", "L'Hospitalet de Llobregat", "ES", 41.35967, 2.10028, 257038);
    assert_contains ("hague", "The Hague", "NL", 52.07667, 4.29861, 474292);
}

static void
test_edge_cases (void)
{
  gint *indices;
  guint len;

  indices = geonames_query_cities_sync ("", GEONAMES_QUERY_DEFAULT, &len, NULL, NULL);
  g_assert_cmpint (len, ==, 0);
  g_assert_cmpint (indices[0], ==, -1);
  g_free (indices);

  indices = geonames_query_cities_sync ("a city that doesn't exist", GEONAMES_QUERY_DEFAULT, &len, NULL, NULL);
  g_assert_cmpint (len, ==, 0);
  g_assert_cmpint (indices[0], ==, -1);
  g_free (indices);
}

int
main (int argc, char **argv)
{
  bindtextdomain(PACKAGE, LOCALEDIR);
  setlocale (LC_ALL, "");

  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/common-cities", test_common_cities);
  g_test_add_func ("/translations", test_translations);
  g_test_add_func ("/edge-cases", test_edge_cases);
  g_test_add_func ("/cities-without-some-words", test_cities_without_some_words);

  return g_test_run ();
}
