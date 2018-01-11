
#include <gtk/gtk.h>
#include <geonames.h>
#include <string.h>

static gchar *
city_display_string (GeonamesCity *city)
{
  const gchar *name;
  const gchar *state;
  const gchar *country;

  name = geonames_city_get_name (city);
  state = geonames_city_get_state (city);
  country = geonames_city_get_country (city);

  if (state[0])
    return g_strdup_printf ("%s (%s, %s)", name, state, country);
  else
    return g_strdup_printf ("%s (%s)", name, country);
}

static void
query_cities_cb (GObject       *source_object,
                 GAsyncResult  *result,
                 gpointer       user_data)
{
  GtkWidget *listbox = user_data;
  g_autofree gint *indices;
  guint len;
  gint i;

  gtk_container_foreach (GTK_CONTAINER (listbox), (GtkCallback) gtk_widget_destroy, NULL);

  indices = geonames_query_cities_finish (result, &len, NULL);
  if (indices == NULL)
    return;

  for (i = 0; i < len; i++)
    {
      g_autoptr(GeonamesCity) city;
      g_autofree gchar *text;
      GtkWidget *label;

      city = geonames_get_city (indices[i]);
      text = city_display_string (city);

      label = gtk_label_new (text);
      gtk_widget_set_halign (label, GTK_ALIGN_START);
      gtk_container_add (GTK_CONTAINER (listbox), label);
    }

  gtk_widget_show_all (listbox);
}

static void
entry_changed (GtkEditable *editable,
               gpointer     user_data)
{
  GtkWidget *listbox = user_data;
  const gchar *text;

  text = gtk_entry_get_text (GTK_ENTRY (editable));
  if (strlen (text) >= 2)
    geonames_query_cities (text, GEONAMES_QUERY_DEFAULT, NULL, query_cities_cb, listbox);
  else
    gtk_container_foreach (GTK_CONTAINER (listbox), (GtkCallback) gtk_widget_destroy, NULL);
}

int
main (int argc, char **argv)
{
  GtkWidget *window;
  GtkWidget *box;
  GtkWidget *entry;
  GtkWidget *scrolled_window;
  GtkWidget *listbox;

  gtk_init (&argc, &argv);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size (GTK_WINDOW (window), 400, 500);
  gtk_container_set_border_width (GTK_CONTAINER (window), 12);
  g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);

  listbox = gtk_list_box_new ();
  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_container_add (GTK_CONTAINER (scrolled_window), listbox);

  entry = gtk_entry_new ();
  g_signal_connect (entry, "changed", G_CALLBACK (entry_changed), listbox);

  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_box_pack_start (GTK_BOX (box), entry, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (box), scrolled_window, TRUE, TRUE, 0);

  gtk_container_add (GTK_CONTAINER (window), box);
  gtk_widget_show_all (window);

  gtk_main ();
}
