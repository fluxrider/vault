// gcc --pedantic -Wall -Werror-implicit-function-declaration gtk.c $(pkg-config --cflags --libs gtk4)
#include <gtk/gtk.h>

static void on_activate(GtkApplication * app) {
//  GtkWidget * button = gtk_button_new_with_label("Hello, World!");
//  g_signal_connect_swapped(button, "clicked", G_CALLBACK(gtk_window_close), window);
//  gtk_window_set_child(GTK_WINDOW (window), button);

  GtkWidget * username = gtk_password_entry_new(); gtk_password_entry_set_show_peek_icon(GTK_PASSWORD_ENTRY(username), true);
  GtkWidget * username_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_box_append(GTK_BOX(username_row), username);  

  GtkWidget * vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 3);
  gtk_box_append(GTK_BOX(vbox), username_row);
  //gtk_box_append(vbox, password_row);
  //gtk_box_append(vbox, url_row);
  //gtk_box_append(vbox, notes_row);

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