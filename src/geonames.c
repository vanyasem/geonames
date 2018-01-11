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

#include "geonames.h"
#include "geonames-query.h"

/**
 * SECTION: geonames
 * @title: Geonames
 * @short_description: Access the geonames.org database
 * @include: geonames.h
 *
 * This library provides access to a local copy of a subset of the city
 * and country data of geonames.org.
 */

static GVariant *geonames_data = NULL;

enum {
  DATA_FIELD_CITIES,
  DATA_FIELD_STATES,
  DATA_FIELD_COUNTRIES,
};

enum {
  CITY_FIELD_ID,
  CITY_FIELD_NAME_EN,
  CITY_FIELD_STATE_CODE,
  CITY_FIELD_STATE_NAME_EN,
  CITY_FIELD_COUNTRY_CODE,
  CITY_FIELD_COUNTRY_NAME_EN,
  CITY_FIELD_TIMEZONE,
  CITY_FIELD_POPULATION,
  CITY_FIELD_LATITUDE,
  CITY_FIELD_LONGITUDE,
};

static void
ensure_geonames_data (void)
{
  if (g_once_init_enter (&geonames_data))
    {
      g_autoptr(GBytes) data;
      GVariant *v;

      data = g_resources_lookup_data ("/com/ubuntu/geonames/cities.compiled", G_RESOURCE_LOOKUP_FLAGS_NONE, NULL);
      g_assert (data);

      v = g_variant_new_from_bytes (G_VARIANT_TYPE ("a(sssssssudd)"), data, TRUE);

      g_once_init_leave (&geonames_data, v);
    }
}

static void
task_func (GTask        *task,
           gpointer      source_object,
           gpointer      task_data,
           GCancellable *cancellable)
{
  gchar *query = task_data;
  GArray *indices;

  indices = geonames_query_cities_db (geonames_data, query);

  g_task_return_pointer (task, indices, (GDestroyNotify) g_array_unref);
}

/**
 * geonames_query_cities:
 * @query: the search string
 * @flags: #GeonamesQueryFlags
 * @cancellable: (nullable): a #GCancellable
 * @callback: (nullable): a #GAsyncReadyCallback
 * @user_data: user data passed into @callback
 *
 * Asynchronously queries the geonames city database with the search
 * terms in @query. When the operation is finished, @callback is called
 * from the thread-default main context you are calling this method
 * from. Call geonames_query_cities_finish() from @callback to retrieve
 * the list of results.
 *
 * Results are weighted by how well and how many tokens match a
 * particular city, as well as importance of a city.
 *
 * If @query is empty, no results are returned.
 */
void
geonames_query_cities (const gchar         *query,
                       GeonamesQueryFlags   flags,
                       GCancellable        *cancellable,
                       GAsyncReadyCallback  callback,
                       gpointer             user_data)
{
  GTask *task;

  ensure_geonames_data ();

  task = g_task_new (NULL, cancellable, callback, user_data);
  g_task_set_task_data (task, g_strdup (query), g_free);

  g_task_run_in_thread (task, task_func);
}

static gint *
free_index_array (GArray *array,
                  guint  *length)
{
  const gint sentinel = -1;

  if (length)
    *length = array->len;

  /* append sentinel after taking the length */
  g_array_append_val (array, sentinel);

  return (gint *) g_array_free (array, FALSE);
}

/**
 * geonames_query_cities_finish:
 * @result: the #GAsyncResult from the callback passed to
 *   geonames_query_cities()
 * @length: (out) (optional): optional location for storing the number
 *   of returned cities
 * @error: a #GError
 *
 * Finishes an operation started with geonames_query_cities() and
 * returns the resulting matches.
 *
 * Returns: (array length=@length): The list of cities matching the
 * search query, as indices that can be passed into cities with
 * geonames_get_city().
 */
gint *
geonames_query_cities_finish (GAsyncResult        *result,
                              guint               *length,
                              GError             **error)
{
  GArray *array;

  g_return_val_if_fail (g_task_is_valid (result, NULL), NULL);

  array = g_task_propagate_pointer (G_TASK (result), error);
  if (array == NULL)
    return NULL;

  return free_index_array (array, length);
}

/**
 * geonames_query_cities_sync:
 * @query: the search string
 * @flags: #GeonamesQueryFlags
 * @cancellable: (nullable): a #GCancellable
 * @length: (out) (optional): optional location for storing the number
 *   of returned cities
 * @error: a #GError
 *
 * Synchronous version of geonames_query_cities().
 *
 * Returns: (array length=@length): The list of cities matching the
 * search query, as indices that can be passed into cities with
 * geonames_get_city().
 */
gint *
geonames_query_cities_sync (const gchar         *query,
                            GeonamesQueryFlags   flags,
                            guint               *length,
                            GCancellable        *cancellable,
                            GError             **error)
{
  GArray *indices;

  ensure_geonames_data ();

  indices = geonames_query_cities_db (geonames_data, query);

  return free_index_array (indices, length);
}

/**
 * geonames_get_n_cities:
 *
 * Returns the amount of cities in the geonames database.
 *
 * Returns: The amount of cities
 */
gint
geonames_get_n_cities (void)
{
  ensure_geonames_data ();

  return g_variant_n_children (geonames_data);
}

/**
 * geonames_get_city:
 * @index: The index of the city to retrieve
 *
 * Retrieves the city at @index in the geonames database.
 *
 * Returns: (transfer full): the #GeonamesCity at @index in the database
 */
GeonamesCity *
geonames_get_city (gint index)
{
  ensure_geonames_data ();

  g_return_val_if_fail (index < g_variant_n_children (geonames_data), NULL);

  return g_variant_get_child_value (geonames_data, index);
}

/**
 * geonames_city_free:
 * @city: a #GeonamesCity
 *
 * Frees @city.
 */
void
geonames_city_free (GeonamesCity *city)
{
  g_variant_unref (city);
}

/**
 * geonames_city_get_name:
 * @city: a #GeonamesCity
 *
 * Returns: the name of @city, in the current language
 */
const gchar *
geonames_city_get_name (GeonamesCity *city)
{
  const gchar *name;
  const gchar *id;

  g_variant_get_child (city, CITY_FIELD_ID, "&s", &id);

  name = g_dgettext (PACKAGE, id);
  if (g_strcmp0 (name, id) == 0)
    g_variant_get_child (city, CITY_FIELD_NAME_EN, "&s", &name);

  return name;
}

/**
 * geonames_city_get_state:
 * @city: a #GeonamesCity
 *
 * Returns: the state of @city
 */
const gchar *
geonames_city_get_state (GeonamesCity *city)
{
  const gchar *state;
  const gchar *state_code;
  const gchar *country_code;
  g_autofree gchar *index;

  g_variant_get_child (city, CITY_FIELD_STATE_CODE, "&s", &state_code);
  g_variant_get_child (city, CITY_FIELD_COUNTRY_CODE, "&s", &country_code);
  index = g_strdup_printf ("%s.%s", country_code, state_code);

  state = g_dgettext (PACKAGE, index);
  if (g_strcmp0 (state, index) == 0)
    g_variant_get_child (city, CITY_FIELD_STATE_NAME_EN, "&s", &state);

  return state;
}

/**
 * geonames_city_get_country:
 * @city: a #GeonamesCity
 *
 * Returns: the country of @city
 */
const gchar *
geonames_city_get_country (GeonamesCity *city)
{
  const gchar *country;
  const gchar *code;

  g_variant_get_child (city, CITY_FIELD_COUNTRY_CODE, "&s", &code);

  country = g_dgettext (PACKAGE, code);
  if (g_strcmp0 (country, code) == 0)
    g_variant_get_child (city, CITY_FIELD_COUNTRY_NAME_EN, "&s", &country);

  return country;
}

/**
 * geonames_city_get_country_code:
 * @city: a #GeonamesCity
 *
 * Returns: the ISO-3166 two-letter country code of @city
 */
const gchar *
geonames_city_get_country_code (GeonamesCity *city)
{
  const gchar *country_code;

  g_variant_get_child (city, CITY_FIELD_COUNTRY_CODE, "&s", &country_code);

  return country_code;
}

/**
 * geonames_city_get_timezone:
 * @city: a #GeonamesCity
 *
 * Returns: the timezone of @city
 */
const gchar *
geonames_city_get_timezone (GeonamesCity *city)
{
  const gchar *timezone;

  g_variant_get_child (city, CITY_FIELD_TIMEZONE, "&s", &timezone);

  return timezone;
}

/**
 * geonames_city_get_latitude:
 * @city: a #GeonamesCity
 *
 * Returns: the latitude of @city
 */
gdouble
geonames_city_get_latitude (GeonamesCity *city)
{
  gdouble latitude;

  g_variant_get_child (city, CITY_FIELD_LATITUDE, "d", &latitude);

  return latitude;
}

/**
 * geonames_city_get_longitude:
 * @city: a #GeonamesCity
 *
 * Returns: the longitude of @city
 */
gdouble
geonames_city_get_longitude (GeonamesCity *city)
{
  gdouble longitude;

  g_variant_get_child (city, CITY_FIELD_LONGITUDE, "d", &longitude);

  return longitude;
}


/**
 * geonames_city_get_population:
 * @city: a #GeonamesCity
 *
 * Returns: the population of @city
 */
guint
geonames_city_get_population (GeonamesCity *city)
{
  guint population;

  g_variant_get_child (city, CITY_FIELD_POPULATION, "u", &population);

  return population;
}
