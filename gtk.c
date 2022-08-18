// Copyright 2022 David Lareau. This source code form is subject to the terms of the Mozilla Public License 2.0.
// gcc --pedantic -Wall -Werror-implicit-function-declaration gtk.c $(pkg-config --cflags --libs gtk4 libsodium)
#include <time.h>
#include <gtk/gtk.h>
#include <sodium.h>
// gettext()
#include <libintl.h>
#define _(String) gettext(String)
#include <unistd.h> // access()
#include <fcntl.h> // open()
#include <sys/mman.h> // mlock()
#include <sys/uio.h> // writev()

static void on_save_with_passphrase(GtkDialog * dialog, gint response_id, gpointer _data) { GtkWidget ** _widgets = _data; GtkWidget * _entry = _widgets[0]; GtkWidget * _url = _widgets[1]; GtkWidget * _username = _widgets[2]; GtkWidget * _password = _widgets[3]; GtkWidget * _notes = _widgets[4]; GtkWidget * _master_passphrase = _widgets[6];
  if(response_id != GTK_RESPONSE_ACCEPT) { gtk_window_destroy(GTK_WINDOW(dialog)); return; }
  const char * error = NULL;
  // derive key
  const char * master_passphrase = gtk_editable_get_text(GTK_EDITABLE(_master_passphrase));
  unsigned char key[crypto_aead_xchacha20poly1305_ietf_KEYBYTES]; if(mlock(key, crypto_aead_xchacha20poly1305_ietf_KEYBYTES) == -1) error = "mlock()"; else {
  unsigned char salt[crypto_pwhash_SALTBYTES]; randombytes_buf(salt, crypto_pwhash_SALTBYTES);
  uint64_t algo = crypto_pwhash_ALG_ARGON2ID13, algo_p1 = crypto_pwhash_OPSLIMIT_MODERATE, algo_p2 = crypto_pwhash_MEMLIMIT_MODERATE;
  if(crypto_pwhash(key, crypto_aead_xchacha20poly1305_ietf_KEYBYTES, master_passphrase, strlen(master_passphrase), salt, (unsigned long long)algo_p1, (size_t)algo_p2, (int)algo)) error = "crypto_pwhash()"; else {
  // read entry
  const char * entry = gtk_editable_get_text(GTK_EDITABLE(_entry)); size_t entry_len = strlen(entry);
  const char * url = gtk_editable_get_text(GTK_EDITABLE(_url)); size_t url_len = strlen(url);
  const char * username = gtk_editable_get_text(GTK_EDITABLE(_username)); size_t username_len = strlen(username);
  const char * password = gtk_editable_get_text(GTK_EDITABLE(_password)); size_t password_len = strlen(password);
  GtkTextBuffer * notes_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(_notes)); GtkTextIter notes_begin, notes_end; gtk_text_buffer_get_bounds(notes_buffer, &notes_begin, &notes_end);
  const char * notes = gtk_text_buffer_get_text(notes_buffer, &notes_begin, &notes_end, false); size_t notes_len = strlen(notes);
  // encrypt
  unsigned char nonce[crypto_aead_xchacha20poly1305_ietf_NPUBBYTES]; randombytes_buf(nonce, crypto_aead_xchacha20poly1305_ietf_NPUBBYTES);
  size_t src_n = entry_len+1+url_len+1+username_len+1+password_len+1+notes_len; char src[src_n]; if(mlock(src, src_n) == -1) error = "mlock()"; else {
  char * ptr = src;
  memcpy(ptr, entry, entry_len); ptr += entry_len; *ptr++ = '\n';
  memcpy(ptr, url, url_len); ptr += url_len; *ptr++ = '\n';
  memcpy(ptr, username, username_len); ptr += username_len; *ptr++ = '\n';
  memcpy(ptr, password, password_len); ptr += password_len; *ptr++ = '\n';
  memcpy(ptr, notes, notes_len); g_free((gpointer)notes); notes = NULL;
  unsigned char dst[src_n+crypto_aead_xchacha20poly1305_ietf_ABYTES];
  unsigned long long _dst_n; crypto_aead_xchacha20poly1305_ietf_encrypt(dst, &_dst_n, (unsigned char *)src, src_n, NULL, 0, NULL, nonce, key); uint64_t encrypted_n = _dst_n;
  memset(src, 0, src_n); memset(key, 0, crypto_aead_xchacha20poly1305_ietf_KEYBYTES); if(munlock(src, src_n) == -1) error = "munlock()"; else {
  // write encrypted file (kdf_algo, kdf_algo_p1, kdf_algo_p2, kdf_salt, encryption_nonce, encrypted_len, encrypted_message)
  struct iovec iov[7];
  iov[0].iov_base = &algo; iov[0].iov_len = sizeof(uint64_t);
  iov[1].iov_base = &algo_p1; iov[1].iov_len = sizeof(uint64_t);
  iov[2].iov_base = &algo_p2; iov[2].iov_len = sizeof(uint64_t);
  iov[3].iov_base = salt; iov[3].iov_len = crypto_pwhash_SALTBYTES;
  iov[4].iov_base = nonce; iov[4].iov_len = crypto_aead_xchacha20poly1305_ietf_NPUBBYTES;
  iov[5].iov_base = &encrypted_n; iov[5].iov_len = sizeof(uint64_t);
  iov[6].iov_base = dst; iov[6].iov_len = encrypted_n;
  // TODO sanitize entry for filename
  // filename is derived from entry name and timestamp
  time_t now; if(time(&now) == -1) error = "time()"; else { struct tm * t = localtime(&now); if(!t) error = "localtime()"; else {
  char filename[1024]; int n = snprintf(filename, 1024, "%s.%04d-%02d-%02d_%02d-%02d-%02d.enc", entry, t->tm_year, t->tm_mon, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec); if(n > 1024 || n == -1) error = "snprintf()"; else {
  int fd = open(filename, O_CREAT | O_EXCL | O_WRONLY, S_IRUSR | S_IWUSR ); if(fd == -1) error = "open()"; else {
  if(writev(fd, iov, 7) == -1) error = "writev()"; else {
  if(close(fd) == -1) error = "close()"; else {

  }}}}}}}}if(notes) g_free((gpointer)notes);}}
  // if something bad happened, show a message dialog on top, and don't destroy the master password dialog
  if(!error) gtk_window_destroy(GTK_WINDOW(dialog));
  else {
    GtkWidget * message = gtk_message_dialog_new(GTK_WINDOW(dialog), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "ERROR: %s", error);
    g_signal_connect(message, "response", G_CALLBACK(gtk_window_destroy), NULL);
    gtk_window_present(GTK_WINDOW(message));
  }
  memset(key, 0, crypto_aead_xchacha20poly1305_ietf_KEYBYTES); munlock(key, crypto_aead_xchacha20poly1305_ietf_KEYBYTES);
}

static void on_save(GtkWidget * widget, gpointer _data) { GtkWidget ** _widgets = _data; GtkWidget * window = _widgets[5];
  GtkWidget * master_passphrase = gtk_password_entry_new(); gtk_password_entry_set_show_peek_icon(GTK_PASSWORD_ENTRY(master_passphrase), true); _widgets[6] = master_passphrase;
  GtkWidget * dialog = gtk_dialog_new_with_buttons("Master Passphrase", GTK_WINDOW(window), GTK_DIALOG_MODAL, _("_OK"), GTK_RESPONSE_ACCEPT, _("_Cancel"), GTK_RESPONSE_REJECT, NULL);
  GtkWidget * content = gtk_dialog_get_content_area(GTK_DIALOG(dialog)); gtk_box_append(GTK_BOX(content), master_passphrase);
  g_signal_connect(dialog, "response", G_CALLBACK(on_save_with_passphrase), _data);
  gtk_window_present(GTK_WINDOW(dialog));
}

static void on_activate(GtkApplication * app) {
  GtkWidget * entry = gtk_entry_new(); gtk_widget_set_hexpand(entry, true);
  GtkWidget * entry_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_box_append(GTK_BOX(entry_row), gtk_image_new_from_icon_name("document-open-symbolic"));
  gtk_box_append(GTK_BOX(entry_row), entry);

  GtkWidget * url = gtk_entry_new(); gtk_widget_set_hexpand(url, true);
  GtkWidget * url_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_box_append(GTK_BOX(url_row), gtk_image_new_from_icon_name("go-home-symbolic"));
  gtk_box_append(GTK_BOX(url_row), url);

  GtkWidget * username = gtk_password_entry_new(); gtk_widget_set_hexpand(username, true); gtk_password_entry_set_show_peek_icon(GTK_PASSWORD_ENTRY(username), true);
  GtkWidget * username_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_box_append(GTK_BOX(username_row), gtk_image_new_from_icon_name("avatar-default-symbolic"));
  gtk_box_append(GTK_BOX(username_row), username);

  GtkWidget * password = gtk_password_entry_new(); gtk_widget_set_hexpand(password, true); gtk_password_entry_set_show_peek_icon(GTK_PASSWORD_ENTRY(password), true);
  GtkWidget * password_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_box_append(GTK_BOX(password_row), gtk_image_new_from_icon_name("dialog-password-symbolic"));
  gtk_box_append(GTK_BOX(password_row), password);

  GtkWidget * notes = gtk_text_view_new(); gtk_widget_set_hexpand(notes, true); gtk_widget_set_vexpand(notes, true);
  GtkWidget * notes_scroll = gtk_scrolled_window_new(); gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(notes_scroll), notes);
  GtkWidget * notes_frame = gtk_frame_new(NULL); gtk_frame_set_child(GTK_FRAME(notes_frame), notes_scroll);  
  GtkWidget * notes_label = gtk_image_new_from_icon_name("accessories-text-editor"); gtk_widget_set_valign(notes_label, GTK_ALIGN_START);
  GtkWidget * notes_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);  
  gtk_box_append(GTK_BOX(notes_row), notes_label);
  gtk_box_append(GTK_BOX(notes_row), notes_frame);

  GtkWidget * window = gtk_application_window_new(app);
  GtkWidget * save = gtk_button_new_from_icon_name("document-save");
  static GtkWidget * widgets[7]; widgets[0] = entry; widgets[1] = url; widgets[2] = username; widgets[3] = password; widgets[4] = notes; widgets[5] = window; widgets[6] = NULL;
  g_signal_connect(save, "clicked", G_CALLBACK(on_save), widgets);

  GtkWidget * vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 3);
  gtk_box_append(GTK_BOX(vbox), entry_row);
  gtk_box_append(GTK_BOX(vbox), url_row);
  gtk_box_append(GTK_BOX(vbox), username_row);
  gtk_box_append(GTK_BOX(vbox), password_row);
  gtk_box_append(GTK_BOX(vbox), notes_row);
  gtk_box_append(GTK_BOX(vbox), save);

  GFile * path = g_file_new_for_commandline_arg("."); GtkWidget * file_chooser = gtk_file_chooser_widget_new(GTK_FILE_CHOOSER_ACTION_OPEN); gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(file_chooser), path, NULL); g_object_unref(path);
  GtkFileFilter * filter = gtk_file_filter_new(); gtk_file_filter_add_suffix(filter, "enc"); gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(file_chooser), filter); g_object_unref(filter);
  GtkWidget * hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_box_append(GTK_BOX(hbox), vbox);
  gtk_box_append(GTK_BOX(hbox), file_chooser);

  gtk_window_set_icon_name(GTK_WINDOW(window), "dialog-password-symbolic");
  gtk_window_set_title(GTK_WINDOW(window), "Vault");
  gtk_window_set_default_size(GTK_WINDOW(window), -1, -1);
  gtk_window_set_child(GTK_WINDOW (window), hbox);
  gtk_window_present(GTK_WINDOW (window));
}

int main (int argc, char * argv[]) {
  if(sodium_init()) { fprintf(stderr, "ERROR: sodium_init()\n"); return EXIT_FAILURE; }
  GtkApplication * app = gtk_application_new("com.github.fluxrider.vault", G_APPLICATION_FLAGS_NONE);
  g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);
  int status = g_application_run(G_APPLICATION(app), argc, argv); g_object_unref(app); return status;
}