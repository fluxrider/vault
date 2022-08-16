// gcc --pedantic -Wall -Werror-implicit-function-declaration gtk.c $(pkg-config --cflags --libs gtk4)
#include <gtk/gtk.h>

static void on_activate(GtkApplication * app) {
//  GtkWidget * button = gtk_button_new_with_label("Hello, World!");
//  g_signal_connect_swapped(button, "clicked", G_CALLBACK(gtk_window_close), window);
//  gtk_window_set_child(GTK_WINDOW (window), button);

  GtkWidget * entry_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_box_append(GTK_BOX(entry_row), gtk_label_new("Entry: "));
  gtk_box_append(GTK_BOX(entry_row), gtk_entry_new());

  GtkWidget * url_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_box_append(GTK_BOX(url_row), gtk_label_new("URL: "));
  gtk_box_append(GTK_BOX(url_row), gtk_entry_new());

  GtkWidget * username = gtk_password_entry_new(); gtk_password_entry_set_show_peek_icon(GTK_PASSWORD_ENTRY(username), true);
  GtkWidget * username_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_box_append(GTK_BOX(username_row), gtk_label_new("Username: "));
  gtk_box_append(GTK_BOX(username_row), username);

  GtkWidget * password = gtk_password_entry_new(); gtk_password_entry_set_show_peek_icon(GTK_PASSWORD_ENTRY(password), true);
  GtkWidget * password_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_box_append(GTK_BOX(password_row), gtk_label_new("Password: "));
  gtk_box_append(GTK_BOX(password_row), password);

  GtkWidget * notes = gtk_text_view_new();
  GtkWidget * notes_scroll = gtk_scrolled_window_new();
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(notes_scroll), notes);
  GtkWidget * notes_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_box_append(GTK_BOX(notes_row), gtk_label_new("Notes: "));
  gtk_box_append(GTK_BOX(notes_row), notes_scroll);

  GtkWidget * vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 3);
  gtk_box_append(GTK_BOX(vbox), entry_row);
  gtk_box_append(GTK_BOX(vbox), url_row);
  gtk_box_append(GTK_BOX(vbox), username_row);
  gtk_box_append(GTK_BOX(vbox), password_row);
  gtk_box_append(GTK_BOX(vbox), notes_row);

  GtkWidget * window = gtk_application_window_new(app);
  gtk_window_set_title(GTK_WINDOW(window), "Vault");
  gtk_window_set_default_size(GTK_WINDOW(window), -1, -1);
  gtk_window_set_child(GTK_WINDOW (window), vbox);
  gtk_window_present(GTK_WINDOW (window));
}

int main (int argc, char * argv[]) {
  GtkApplication * app = gtk_application_new("com.github.fluxrider.vault", G_APPLICATION_FLAGS_NONE);
  g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);
  int status = g_application_run(G_APPLICATION(app), argc, argv); g_object_unref(app); return status;
}