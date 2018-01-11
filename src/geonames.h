/*
 * Copyright 2015 Canonical Ltd.
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

#ifndef GEONAMES_H
#define GEONAMES_H

#include <gio/gio.h>

G_BEGIN_DECLS

#ifndef _GEONAMES_EXPORT
#define _GEONAMES_EXPORT __attribute__((visibility("default"))) extern
#endif

/**
 * GeonamesQueryFlags:
 * @GEONAMES_QUERY_DEFAULT: no flags
 *
 * Flags used when querying the geonames database.
 */
typedef enum
{
  GEONAMES_QUERY_DEFAULT
} GeonamesQueryFlags;

typedef GVariant GeonamesCity;

_GEONAMES_EXPORT
void                    geonames_query_cities                           (const gchar         *query,
                                                                         GeonamesQueryFlags    flags,
                                                                         GCancellable        *cancellable,
                                                                         GAsyncReadyCallback  callback,
                                                                         gpointer             user_data);

_GEONAMES_EXPORT
gint *                  geonames_query_cities_finish                    (GAsyncResult        *result,
                                                                         guint               *length,
                                                                         GError             **error);
_GEONAMES_EXPORT
gint *                  geonames_query_cities_sync                      (const gchar         *query,
                                                                         GeonamesQueryFlags   flags,
                                                                         guint               *length,
                                                                         GCancellable        *cancellable,
                                                                         GError             **error);

_GEONAMES_EXPORT
gint                    geonames_get_n_cities                           (void);

_GEONAMES_EXPORT
GeonamesCity *          geonames_get_city                               (gint index);

_GEONAMES_EXPORT
void                    geonames_city_free                              (GeonamesCity *city);

_GEONAMES_EXPORT
const gchar *           geonames_city_get_name                          (GeonamesCity *city);

_GEONAMES_EXPORT
const gchar *           geonames_city_get_state                         (GeonamesCity *city);

_GEONAMES_EXPORT
const gchar *           geonames_city_get_country                       (GeonamesCity *city);

_GEONAMES_EXPORT
const gchar *           geonames_city_get_country_code                  (GeonamesCity *city);

_GEONAMES_EXPORT
const gchar *           geonames_city_get_timezone                      (GeonamesCity *city);

_GEONAMES_EXPORT
gdouble                 geonames_city_get_latitude                      (GeonamesCity *city);

_GEONAMES_EXPORT
gdouble                 geonames_city_get_longitude                     (GeonamesCity *city);

_GEONAMES_EXPORT
guint                   geonames_city_get_population                    (GeonamesCity *city);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (GeonamesCity, geonames_city_free)

G_END_DECLS

#endif
