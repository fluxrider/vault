// Copyright 2022 David Lareau. This source code form is subject to the terms of the Mozilla Public License 2.0.
// gcc --pedantic -Wall -Werror-implicit-function-declaration gtk.c $(pkg-config --cflags --libs gtk4)
#include <gtk/gtk.h>

static void on_activate(GtkApplication * app) {
  GtkWidget * entry = gtk_entry_new(); gtk_widget_set_hexpand(entry, true);
  GtkWidget * entry_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_box_append(GTK_BOX(entry_row), gtk_label_new("Entry: "));
  gtk_box_append(GTK_BOX(entry_row), entry);

  GtkWidget * url = gtk_entry_new(); gtk_widget_set_hexpand(url, true);
  GtkWidget * url_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_box_append(GTK_BOX(url_row), gtk_label_new("URL: "));
  gtk_box_append(GTK_BOX(url_row), url);

  GtkWidget * username = gtk_password_entry_new(); gtk_widget_set_hexpand(username, true); gtk_password_entry_set_show_peek_icon(GTK_PASSWORD_ENTRY(username), true);
  GtkWidget * username_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_box_append(GTK_BOX(username_row), gtk_label_new("Username: "));
  gtk_box_append(GTK_BOX(username_row), username);

  GtkWidget * password = gtk_password_entry_new(); gtk_widget_set_hexpand(password, true); gtk_password_entry_set_show_peek_icon(GTK_PASSWORD_ENTRY(password), true);
  GtkWidget * password_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_box_append(GTK_BOX(password_row), gtk_label_new("Password: "));
  gtk_box_append(GTK_BOX(password_row), password);

  GtkWidget * notes = gtk_text_view_new(); gtk_widget_set_hexpand(notes, true); gtk_widget_set_vexpand(notes, true);
  GtkWidget * notes_scroll = gtk_scrolled_window_new(); gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(notes_scroll), notes);
  GtkWidget * notes_frame = gtk_frame_new(NULL); gtk_frame_set_child(GTK_FRAME(notes_frame), notes_scroll);  
  GtkWidget * notes_label = gtk_label_new("Notes: "); gtk_widget_set_valign(notes_label, GTK_ALIGN_START);
  GtkWidget * notes_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);  
  gtk_box_append(GTK_BOX(notes_row), notes_label);
  gtk_box_append(GTK_BOX(notes_row), notes_frame);

  GtkWidget * save = gtk_button_new_from_icon_name("document-save");

  GtkWidget * vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 3);
  gtk_box_append(GTK_BOX(vbox), entry_row);
  gtk_box_append(GTK_BOX(vbox), url_row);
  gtk_box_append(GTK_BOX(vbox), username_row);
  gtk_box_append(GTK_BOX(vbox), password_row);
  gtk_box_append(GTK_BOX(vbox), notes_row);
  gtk_box_append(GTK_BOX(vbox), save);

  GtkWidget * window = gtk_application_window_new(app);
  gtk_window_set_icon_name(GTK_WINDOW(window), "dialog-password"); // TODO: this looks low rez when alt-tabbing
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