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

#include "geonames-query.h"
#include <string.h>

typedef struct
{
  gint index;
  gdouble weight;
} Match;

static Match *
match_new (gint    index,
           gdouble weight)
{
  Match *match;

  match = g_slice_new (Match);
  match->index = index;
  match->weight = weight;

  return match;
}

static void
match_free (gpointer data)
{
  g_slice_free (Match, data);
}

static gint
compare_matches (gconstpointer a,
                 gconstpointer b,
                 gpointer      user_data)
{
  const Match *match_a = a;
  const Match *match_b = b;
  gdouble delta;

  delta = match_b->weight - match_a->weight;

  if (delta < -1e-5)
    return -1;
  else if (delta > 1e-5)
    return 1;
  else
    return 0;
}

static gboolean
str_prefix_matches (const gchar *str,
                    const gchar *prefix)
{
  if (g_str_has_prefix (str, prefix))
    return TRUE;

  if (!g_str_is_ascii (str))
    {
      g_autofree gchar *ascii;

      ascii = g_str_to_ascii (str, NULL);
      if (g_str_has_prefix (ascii, prefix))
        return TRUE;
    }

  return FALSE;
}

static gdouble
match_query (gchar       **query_tokens,
             const gchar  *potential_hit,
             gboolean     *all_prefix_match)
{
  g_auto(GStrv) tokens = NULL;
  gint i;
  gdouble weight = 0.0;

  *all_prefix_match = TRUE;

  tokens = g_str_tokenize_and_fold (potential_hit, NULL, NULL);
  for (i = 0; query_tokens[i]; i++)
    {
      if (tokens[i] == 0) {
        *all_prefix_match = FALSE;
        return 0.0;
      }

      if (str_prefix_matches (tokens[i], query_tokens[i]))
        {
          weight += (gdouble) strlen (query_tokens[i]) / strlen (tokens[i]);
        }
      else
        {
          const gdouble aux = match_query(query_tokens, &potential_hit[strlen (tokens[i])], all_prefix_match);
          *all_prefix_match = FALSE;
          return aux;
        }
    }

  return weight / i;
}

static void
collect_indices (gpointer data,
                 gpointer user_data)
{
  Match *match = data;
  GArray *indices = user_data;

  g_array_append_val (indices, match->index);
}

static gdouble
calculate_weight (GStrv query_tokens,
                  const gchar *name,
                  guint population,
                  gdouble best_weight)
{
  gdouble weight;
  gboolean all_prefix_match;

  weight = match_query (query_tokens, name, &all_prefix_match);
  weight *= (gdouble) CLAMP (population, 1, 1000000) / 1000000;
  if (all_prefix_match)
    weight += 1;

  return MAX (weight, best_weight);
}

GArray *
geonames_query_cities_db (GVariant    *db,
                          const gchar *query)
{
  g_auto(GStrv) query_tokens = NULL;
  g_autoptr(GSequence) matches = NULL;
  gsize n_cities;
  gsize i;
  GArray *indices;

  g_return_val_if_fail (g_variant_is_of_type (db, G_VARIANT_TYPE ("a(sssssssudd)")), NULL);
  g_return_val_if_fail (query != NULL, NULL);

  query_tokens = g_str_tokenize_and_fold (query, NULL, NULL);
  matches = g_sequence_new (match_free);

  n_cities = g_variant_n_children (db);
  for (i = 0; i < n_cities; i++)
    {
      const gchar *id;
      const gchar *en_name;
      const gchar *translation;
      guint population;
      gdouble best_weight = 0;

      g_variant_get_child (db, i, "(&s&s&s&s&s&s&sudd)", &id, &en_name, NULL, NULL, NULL, NULL, NULL, &population, NULL, NULL);

      best_weight = calculate_weight (query_tokens, en_name, population, best_weight);

      translation = g_dgettext (PACKAGE, id);
      if (g_strcmp0 (translation, id) != 0)
        best_weight = calculate_weight (query_tokens, translation, population, best_weight);

      if (best_weight > 0.0)
        g_sequence_insert_sorted (matches, match_new (i, best_weight), compare_matches, NULL);
    }

  indices = g_array_new (FALSE, FALSE, sizeof (gint));
  g_sequence_foreach (matches, collect_indices, indices);

  return indices;
}
