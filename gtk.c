// Copyright 2022 David Lareau. This source code form is subject to the terms of the Mozilla Public License 2.0.
// gcc --pedantic -Wall -Werror-implicit-function-declaration gtk.c $(pkg-config --cflags --libs gtk4 libsodium x11)
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
#include <dirent.h> // directory listing
int filter_file(const struct dirent * entry) { if(entry->d_type != DT_REG) return 0; const char * ext = strrchr(entry->d_name, '.'); if(!ext) return 0; return strcmp(ext, ".enc") == 0; }

// gtk4 does not support window position, and neither does wayland, but of course I'm on x11 so I'm sad
// but of course, not all is well, screen size isn't desktop area size, am I in the good screen anyway, and window size isn't even resolved
#include <gdk/x11/gdkx.h>
static void my_window_set_position(GtkWindow * window, int x, int y) {
  Window xw = GDK_SURFACE_XID(GDK_SURFACE(gtk_native_get_surface(GTK_NATIVE(window))));
  Display * xd = GDK_SURFACE_XDISPLAY(GDK_SURFACE(gtk_native_get_surface(GTK_NATIVE(window))));
  XMoveWindow(xd, xw, x, y);
}
static void my_window_set_position_center(GtkWindow * window, int default_w, int default_h) {
  Display * xd = GDK_SURFACE_XDISPLAY(GDK_SURFACE(gtk_native_get_surface(GTK_NATIVE(window)))); Screen * screen = ScreenOfDisplay(xd,0);
  int W = WidthOfScreen(screen), H = HeightOfScreen(screen), w = gtk_widget_get_width(GTK_WIDGET(window)), h = gtk_widget_get_height(GTK_WIDGET(window));
  if(w == 0) w = default_w;
  if(h == 0) h = default_h;
  my_window_set_position(window, (W - w) / 2, (H - h) / 2);
}

static gboolean on_window_key_escape(GtkWidget * widget, GVariant * args, gpointer user_data) { gtk_window_destroy(GTK_WINDOW(widget)); return true; }

static const char * decrypt_init(const char * path, size_t * out_encrypted_n, int * out_fd, uint64_t * out_algo, uint64_t * out_algo_p1, uint64_t * out_algo_p2, unsigned char out_salt[crypto_pwhash_SALTBYTES], unsigned char out_nonce[crypto_aead_xchacha20poly1305_ietf_NPUBBYTES]) {
  const char * error = NULL;
  // read header
  struct iovec iov[6]; uint64_t encrypted_n;
  iov[0].iov_base = out_algo; iov[0].iov_len = sizeof(uint64_t);
  iov[1].iov_base = out_algo_p1; iov[1].iov_len = sizeof(uint64_t);
  iov[2].iov_base = out_algo_p2; iov[2].iov_len = sizeof(uint64_t);
  iov[3].iov_base = out_salt; iov[3].iov_len = crypto_pwhash_SALTBYTES;
  iov[4].iov_base = out_nonce; iov[4].iov_len = crypto_aead_xchacha20poly1305_ietf_NPUBBYTES;
  iov[5].iov_base = &encrypted_n; iov[5].iov_len = sizeof(uint64_t);
  int fd = *out_fd = open(path, O_RDONLY); if(fd == -1) error = "open()"; else {
    if(readv(fd, iov, 6) == -1) error = "readv()"; else {
      if(encrypted_n <= crypto_aead_xchacha20poly1305_ietf_ABYTES) error = "encrypted length too short"; else {
        *out_encrypted_n = encrypted_n;
      }
    }
  }
  return error;
}

static const char * decrypt(int fd, unsigned char * encrypted_buffer, unsigned char * decrypted_buffer, size_t * out_decrypted_n, size_t encrypted_n, uint64_t algo, uint64_t algo_p1, uint64_t algo_p2, unsigned char salt[crypto_pwhash_SALTBYTES], unsigned char nonce[crypto_aead_xchacha20poly1305_ietf_NPUBBYTES], const char * master_passphrase) {
  const char * error = NULL;
  unsigned char key[crypto_aead_xchacha20poly1305_ietf_KEYBYTES] = {0}; if(mlock(key, crypto_aead_xchacha20poly1305_ietf_KEYBYTES) == -1) error = "mlock()"; else {
    // read encrypted data
    ssize_t n = read(fd, encrypted_buffer, encrypted_n); if(n != encrypted_n) error = "read()"; else { if(close(fd) == -1) error = "close()"; else {
      // derive key
      if(crypto_pwhash(key, crypto_aead_xchacha20poly1305_ietf_KEYBYTES, master_passphrase, strlen(master_passphrase), salt, (unsigned long long)algo_p1, (size_t)algo_p2, (int)algo)) error = "crypto_pwhash()"; else {
        // decrypt data
        unsigned long long buffer_n;
        if(crypto_aead_xchacha20poly1305_ietf_decrypt(decrypted_buffer, &buffer_n, NULL, encrypted_buffer, encrypted_n, NULL, 0, nonce, key)) error = "crypto_aead_xchacha20poly1305_ietf_decrypt()"; else {
          *out_decrypted_n = buffer_n;
        }
      }
    }}
  }
  explicit_bzero(key, crypto_aead_xchacha20poly1305_ietf_KEYBYTES); if(munlock(key, crypto_aead_xchacha20poly1305_ietf_KEYBYTES) == -1) error = "munlock()";
  return error;
}

static void on_file_with_passphrase(GtkDialog * dialog, gint response_id, gpointer _data) { GtkWidget ** _widgets = _data; GtkWidget * _entry = _widgets[0]; GtkWidget * _url = _widgets[1]; GtkWidget * _username = _widgets[2]; GtkWidget * _password = _widgets[3]; GtkWidget * _notes = _widgets[4]; GtkWidget * _master_passphrase = _widgets[6]; GFile * file = (GFile *) _widgets[7];
  if(response_id != GTK_RESPONSE_ACCEPT) { gtk_window_destroy(GTK_WINDOW(dialog)); return; }
  uint64_t algo, algo_p1, algo_p2; size_t encrypted_n; int fd; unsigned char salt[crypto_pwhash_SALTBYTES]; unsigned char nonce[crypto_aead_xchacha20poly1305_ietf_NPUBBYTES]; char * path = g_file_get_path(file); const char * error = decrypt_init(path, &encrypted_n, &fd, &algo, &algo_p1, &algo_p2, salt, nonce); g_free(path); if(error && fd != -1 && close(fd) == -1) error = "close()";
  if(!error) {
    unsigned char encrypted[encrypted_n];
    const char * master_passphrase = gtk_editable_get_text(GTK_EDITABLE(_master_passphrase));
    unsigned char decrypted[encrypted_n - crypto_aead_xchacha20poly1305_ietf_ABYTES + 1]; if(mlock(decrypted, encrypted_n - crypto_aead_xchacha20poly1305_ietf_ABYTES + 1) == -1) error = "mlock()";
    if(!error) {
      size_t decrypted_n; error = decrypt(fd, encrypted, decrypted, &decrypted_n, encrypted_n, algo, algo_p1, algo_p2, salt, nonce, master_passphrase);
      if(!error) {
        decrypted[decrypted_n] = '\0';
        // populate entry widgets
        char * ptr = (char *)decrypted;
        char * entry = ptr; ptr = strchr(ptr, '\n'); if(ptr) { *ptr++ = '\0';
        char * url = ptr; ptr = strchr(ptr, '\n'); if(ptr) { *ptr++ = '\0';
        char * username = ptr; ptr = strchr(ptr, '\n'); if(ptr) { *ptr++ = '\0';
        char * password = ptr; ptr = strchr(ptr, '\n'); if(ptr) { *ptr++ = '\0';
        char * notes = ptr; gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(_notes)), notes, -1);
        } gtk_editable_set_text(GTK_EDITABLE(_password), password); } gtk_editable_set_text(GTK_EDITABLE(_username), username); } gtk_editable_set_text(GTK_EDITABLE(_url), url); } gtk_editable_set_text(GTK_EDITABLE(_entry), entry);
      }
      explicit_bzero(decrypted, encrypted_n - crypto_aead_xchacha20poly1305_ietf_ABYTES + 1); if(munlock(decrypted, encrypted_n - crypto_aead_xchacha20poly1305_ietf_ABYTES + 1) == -1) error = "munlock";
    }
  } else { if(fd != -1) close(fd); }
  // if something bad happened, show a message dialog on top, and don't destroy the master password dialog
  if(!error) gtk_window_destroy(GTK_WINDOW(dialog)); else {
    GtkWidget * message = gtk_message_dialog_new(GTK_WINDOW(dialog), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "ERROR: %s", error);
    g_signal_connect(message, "response", G_CALLBACK(gtk_window_destroy), NULL);
    gtk_window_present(GTK_WINDOW(message));
  }
}

static void on_file(GtkDialog * _dialog, gint response_id, gpointer _data) { GtkWidget ** _widgets = _data;
  if(response_id == GTK_RESPONSE_ACCEPT) {
    _widgets[7] = (GtkWidget *)gtk_file_chooser_get_file(GTK_FILE_CHOOSER(_dialog));
    GtkWidget * master_passphrase = gtk_password_entry_new(); gtk_password_entry_set_show_peek_icon(GTK_PASSWORD_ENTRY(master_passphrase), true); _widgets[6] = master_passphrase;
    GtkWidget * dialog = gtk_dialog_new_with_buttons("Master Passphrase", GTK_WINDOW(_dialog), GTK_DIALOG_MODAL, _("_OK"), GTK_RESPONSE_ACCEPT, _("_Cancel"), GTK_RESPONSE_REJECT, NULL);
    GtkWidget * content = gtk_dialog_get_content_area(GTK_DIALOG(dialog)); gtk_box_append(GTK_BOX(content), master_passphrase);
    g_signal_connect(dialog, "response", G_CALLBACK(on_file_with_passphrase), _data);
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT); GValue value = {0}; g_value_init(&value, G_TYPE_BOOLEAN); g_value_set_boolean(&value, true); g_object_set_property(G_OBJECT(master_passphrase), "activates-default", &value);
    gtk_window_present(GTK_WINDOW(dialog));
  }
  gtk_window_destroy(GTK_WINDOW(_dialog));
}

static void on_save_with_passphrase(GtkDialog * dialog, gint response_id, gpointer _data) { GtkWidget ** _widgets = _data; GtkWidget * _entry = _widgets[0]; GtkWidget * _url = _widgets[1]; GtkWidget * _username = _widgets[2]; GtkWidget * _password = _widgets[3]; GtkWidget * _notes = _widgets[4]; GtkWidget * _master_passphrase = _widgets[6];
  if(response_id != GTK_RESPONSE_ACCEPT) { gtk_window_destroy(GTK_WINDOW(dialog)); return; }
  const char * error = NULL;

  // test key on a random existing file in folder
  bool master_passphrase_is_the_common_one = false;
  struct dirent ** listing;
  char * path = NULL;
  int n = scandir(".", &listing, filter_file, alphasort); if(n == -1) error = "scandir()"; if(!error) {
    if(!n) {
      master_passphrase_is_the_common_one = true;
      error = "no other files were found to verify the passphrase, no worries though, just letting you know we tried";
    } else {
      path = strdup(listing[0]->d_name);
      while(n--) { free(listing[n]); } free(listing);
    }
  }
  if(!error) {
    uint64_t algo, algo_p1, algo_p2; size_t encrypted_n; int fd; unsigned char salt[crypto_pwhash_SALTBYTES]; unsigned char nonce[crypto_aead_xchacha20poly1305_ietf_NPUBBYTES]; error = decrypt_init(path, &encrypted_n, &fd, &algo, &algo_p1, &algo_p2, salt, nonce); free(path); if(error && fd != -1 && close(fd) == -1) error = "close()";
    if(!error) {
      unsigned char encrypted[encrypted_n];
      const char * master_passphrase = gtk_editable_get_text(GTK_EDITABLE(_master_passphrase));
      unsigned char decrypted[encrypted_n - crypto_aead_xchacha20poly1305_ietf_ABYTES + 1]; if(mlock(decrypted, encrypted_n - crypto_aead_xchacha20poly1305_ietf_ABYTES + 1) == -1) error = "mlock()";
      if(!error) {
        size_t decrypted_n; error = decrypt(fd, encrypted, decrypted, &decrypted_n, encrypted_n, algo, algo_p1, algo_p2, salt, nonce, master_passphrase);
        master_passphrase_is_the_common_one = !error;
        explicit_bzero(decrypted, decrypted_n); if(munlock(decrypted, encrypted_n - crypto_aead_xchacha20poly1305_ietf_ABYTES + 1) == -1) error = "munlock";
      }
    } else { if(fd != -1) close(fd); }
  }
  // if something bad happened, show a warning message to the user and if key was not confirmed bail
  if(error) {
    GtkWidget * message = gtk_message_dialog_new(GTK_WINDOW(dialog), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "WARNING: %s", error);
    g_signal_connect(message, "response", G_CALLBACK(gtk_window_destroy), NULL);
    gtk_window_present(GTK_WINDOW(message));
    if(!master_passphrase_is_the_common_one) return;
    error = NULL;
  }

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
  explicit_bzero(src, src_n); explicit_bzero(key, crypto_aead_xchacha20poly1305_ietf_KEYBYTES); if(munlock(src, src_n) == -1) error = "munlock()"; else {
  // write encrypted file (kdf_algo, kdf_algo_p1, kdf_algo_p2, kdf_salt, encryption_nonce, encrypted_len, encrypted_message)
  struct iovec iov[7];
  iov[0].iov_base = &algo; iov[0].iov_len = sizeof(uint64_t);
  iov[1].iov_base = &algo_p1; iov[1].iov_len = sizeof(uint64_t);
  iov[2].iov_base = &algo_p2; iov[2].iov_len = sizeof(uint64_t);
  iov[3].iov_base = salt; iov[3].iov_len = crypto_pwhash_SALTBYTES;
  iov[4].iov_base = nonce; iov[4].iov_len = crypto_aead_xchacha20poly1305_ietf_NPUBBYTES;
  iov[5].iov_base = &encrypted_n; iov[5].iov_len = sizeof(uint64_t);
  iov[6].iov_base = dst; iov[6].iov_len = encrypted_n;
  // sanitize entry for filename
  char * sanitized_entry = strdup(entry); { char * ptr = sanitized_entry; while(*ptr) { switch(*ptr) { case '/': case '\\': case '?': case '%': case '*': case ':': case '|': case '"': case '<': case '>': case '.': case ',': case ';': case '=': case ' ': case '+': case '[': case ']': case '(': case ')': case '!': case '@': case '$':case '#': case '-': *ptr = '_'; } ptr++; } }
  // filename is derived from entry name and timestamp
  time_t now; if(time(&now) == -1) error = "time()"; else { struct tm * t = localtime(&now); if(!t) error = "localtime()"; else {
  char filename[1024]; int n = snprintf(filename, 1024, "%s.%04d-%02d-%02d_%02d-%02d-%02d.enc", sanitized_entry, t->tm_year+1900, t->tm_mon, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec); if(n > 1024 || n == -1) error = "snprintf()"; else {
  int fd = open(filename, O_CREAT | O_EXCL | O_WRONLY, S_IRUSR | S_IWUSR ); if(fd == -1) error = "open()"; else {
  if(writev(fd, iov, 7) == -1) error = "writev()"; else {
  if(close(fd) == -1) error = "close()"; else {

  }}}}}}free(sanitized_entry);}}if(notes) g_free((gpointer)notes);}}
  // if something bad happened, show a message dialog on top, and don't destroy the master password dialog
  if(!error) gtk_window_destroy(GTK_WINDOW(dialog));
  else {
    GtkWidget * message = gtk_message_dialog_new(GTK_WINDOW(dialog), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "ERROR: %s", error);
    g_signal_connect(message, "response", G_CALLBACK(gtk_window_destroy), NULL);
    gtk_window_present(GTK_WINDOW(message));
  }
  explicit_bzero(key, crypto_aead_xchacha20poly1305_ietf_KEYBYTES); munlock(key, crypto_aead_xchacha20poly1305_ietf_KEYBYTES);
}

static void on_save(GtkWidget * widget, gpointer _data) { GtkWidget ** _widgets = _data; GtkWidget * window = _widgets[5];
  GtkWidget * master_passphrase = gtk_password_entry_new(); gtk_password_entry_set_show_peek_icon(GTK_PASSWORD_ENTRY(master_passphrase), true); _widgets[6] = master_passphrase;
  GtkWidget * dialog = gtk_dialog_new_with_buttons("Master Passphrase", GTK_WINDOW(window), GTK_DIALOG_MODAL, _("_OK"), GTK_RESPONSE_ACCEPT, _("_Cancel"), GTK_RESPONSE_REJECT, NULL);
  GtkWidget * content = gtk_dialog_get_content_area(GTK_DIALOG(dialog)); gtk_box_append(GTK_BOX(content), master_passphrase);
  g_signal_connect(dialog, "response", G_CALLBACK(on_save_with_passphrase), _data);
  gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT); GValue value = {0}; g_value_init(&value, G_TYPE_BOOLEAN); g_value_set_boolean(&value, true); g_object_set_property(G_OBJECT(master_passphrase), "activates-default", &value);
  gtk_window_present(GTK_WINDOW(dialog));
}

static void on_open(GtkWidget * widget, gpointer _data) { GtkWidget ** _widgets = _data; GtkWidget * window = _widgets[5];
  GFile * path = g_file_new_for_commandline_arg("."); GtkWidget * file_chooser = gtk_file_chooser_dialog_new("Decrypt", GTK_WINDOW(window), GTK_FILE_CHOOSER_ACTION_OPEN, _("_Cancel"), GTK_RESPONSE_CANCEL, _("_Open"), GTK_RESPONSE_ACCEPT, NULL); gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(file_chooser), path, NULL); g_object_unref(path);
  GtkFileFilter * filter = gtk_file_filter_new(); gtk_file_filter_add_suffix(filter, "enc"); gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(file_chooser), filter); g_object_unref(filter);
  g_signal_connect(file_chooser, "response", G_CALLBACK(on_file), _widgets);
  gtk_widget_show(file_chooser);
}

static guint copy_timeout_id;
static gboolean clear_copy(gpointer user_data) {
  copy_timeout_id = 0;
  gdk_clipboard_set_text(gdk_display_get_clipboard(gdk_display_get_default()), "");
  return G_SOURCE_REMOVE;
}
static void on_copy(GtkWidget * widget, gpointer _data) {
  if(copy_timeout_id) g_source_remove(copy_timeout_id);
  gdk_clipboard_set_text(gdk_display_get_clipboard(gdk_display_get_default()), gtk_editable_get_text(GTK_EDITABLE(_data)));
  copy_timeout_id = g_timeout_add(1000 * 10, clear_copy, NULL);
}

static gboolean on_window_key_copy(GtkWidget * widget, GVariant * args, gpointer _data) { on_copy(NULL, _data); return true; }

static void on_activate(GtkApplication * app) {
  GtkWidget * entry = gtk_entry_new(); gtk_widget_set_hexpand(entry, true);
  GtkWidget * entry_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_box_append(GTK_BOX(entry_row), gtk_image_new_from_icon_name("document-open-symbolic"));
  gtk_box_append(GTK_BOX(entry_row), entry);

  GtkWidget * url = gtk_entry_new(); gtk_widget_set_hexpand(url, true);
  GtkWidget * copy_url = gtk_button_new_from_icon_name("edit-copy"); g_signal_connect(copy_url, "clicked", G_CALLBACK(on_copy), url);
  GtkWidget * url_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 3);
  gtk_box_append(GTK_BOX(url_row), gtk_image_new_from_icon_name("go-home-symbolic"));
  gtk_box_append(GTK_BOX(url_row), url);
  gtk_box_append(GTK_BOX(url_row), copy_url);

  GtkWidget * username = gtk_password_entry_new(); gtk_widget_set_hexpand(username, true); gtk_password_entry_set_show_peek_icon(GTK_PASSWORD_ENTRY(username), true);
  GtkWidget * copy_username = gtk_button_new_from_icon_name("edit-copy"); g_signal_connect(copy_username, "clicked", G_CALLBACK(on_copy), username);
  GtkWidget * username_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 3);
  gtk_box_append(GTK_BOX(username_row), gtk_image_new_from_icon_name("avatar-default-symbolic"));
  gtk_box_append(GTK_BOX(username_row), username);
  gtk_box_append(GTK_BOX(username_row), copy_username);

  GtkWidget * password = gtk_password_entry_new(); gtk_widget_set_hexpand(password, true); gtk_password_entry_set_show_peek_icon(GTK_PASSWORD_ENTRY(password), true);
  GtkWidget * copy_password = gtk_button_new_from_icon_name("edit-copy"); g_signal_connect(copy_password, "clicked", G_CALLBACK(on_copy), password);
  GtkWidget * password_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 3);
  gtk_box_append(GTK_BOX(password_row), gtk_image_new_from_icon_name("dialog-password-symbolic"));
  gtk_box_append(GTK_BOX(password_row), password);
  gtk_box_append(GTK_BOX(password_row), copy_password);

  GtkWidget * notes = gtk_text_view_new(); gtk_widget_set_hexpand(notes, true); gtk_widget_set_vexpand(notes, true);
  GtkWidget * notes_scroll = gtk_scrolled_window_new(); gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(notes_scroll), notes);
  GtkWidget * notes_frame = gtk_frame_new(NULL); gtk_frame_set_child(GTK_FRAME(notes_frame), notes_scroll);  
  GtkWidget * notes_label = gtk_image_new_from_icon_name("accessories-text-editor"); gtk_widget_set_valign(notes_label, GTK_ALIGN_START);
  GtkWidget * notes_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);  
  gtk_box_append(GTK_BOX(notes_row), notes_label);
  gtk_box_append(GTK_BOX(notes_row), notes_frame);

  GtkWidget * window = gtk_application_window_new(app);
  GtkWidget * save = gtk_button_new_from_icon_name("document-save");
  static GtkWidget * widgets[8]; widgets[0] = entry; widgets[1] = url; widgets[2] = username; widgets[3] = password; widgets[4] = notes; widgets[5] = window; widgets[6] = NULL; widgets[7] = NULL;
  g_signal_connect(save, "clicked", G_CALLBACK(on_save), widgets);
  GtkWidget * open = gtk_button_new_from_icon_name("document-open");
  g_signal_connect(open, "clicked", G_CALLBACK(on_open), widgets);
  GtkWidget * controls_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);  
  gtk_box_append(GTK_BOX(controls_row), save);
  gtk_box_append(GTK_BOX(controls_row), open);

  GtkWidget * vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 3);
  gtk_box_append(GTK_BOX(vbox), entry_row);
  gtk_box_append(GTK_BOX(vbox), url_row);
  gtk_box_append(GTK_BOX(vbox), username_row);
  gtk_box_append(GTK_BOX(vbox), password_row);
  gtk_box_append(GTK_BOX(vbox), notes_row);
  gtk_box_append(GTK_BOX(vbox), controls_row);

  gtk_window_set_icon_name(GTK_WINDOW(window), "dialog-password-symbolic");
  gtk_window_set_title(GTK_WINDOW(window), "Vault");
  gtk_window_set_default_size(GTK_WINDOW(window), -1, -1);
  gtk_window_set_child(GTK_WINDOW(window), vbox);
  gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
  gtk_window_present(GTK_WINDOW(window));
  my_window_set_position_center(GTK_WINDOW(window), 800, 600);
  GtkApplicationWindowClass * window_class = g_type_class_ref(GTK_TYPE_APPLICATION_WINDOW);
  gtk_widget_class_add_binding(GTK_WIDGET_CLASS(window_class), GDK_KEY_Escape, 0, on_window_key_escape, NULL);
  { GtkShortcut * shortcut = gtk_shortcut_new(gtk_keyval_trigger_new(GDK_KEY_B, GDK_CONTROL_MASK), gtk_callback_action_new(on_window_key_copy, username, NULL)); gtk_widget_class_add_shortcut(GTK_WIDGET_CLASS(window_class), shortcut); g_object_unref(shortcut); }
  { GtkShortcut * shortcut = gtk_shortcut_new(gtk_keyval_trigger_new(GDK_KEY_P, GDK_CONTROL_MASK), gtk_callback_action_new(on_window_key_copy, password, NULL)); gtk_widget_class_add_shortcut(GTK_WIDGET_CLASS(window_class), shortcut); g_object_unref(shortcut); }
  { GtkShortcut * shortcut = gtk_shortcut_new(gtk_keyval_trigger_new(GDK_KEY_W, GDK_CONTROL_MASK), gtk_callback_action_new(on_window_key_copy, url, NULL)); gtk_widget_class_add_shortcut(GTK_WIDGET_CLASS(window_class), shortcut); g_object_unref(shortcut); }
  g_type_class_unref(window_class);

  GFile * path = g_file_new_for_commandline_arg("."); GtkWidget * file_chooser = gtk_file_chooser_dialog_new("Decrypt", GTK_WINDOW(window), GTK_FILE_CHOOSER_ACTION_OPEN, _("_Cancel"), GTK_RESPONSE_CANCEL, _("_Open"), GTK_RESPONSE_ACCEPT, NULL); gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(file_chooser), path, NULL); g_object_unref(path);
  GtkFileFilter * filter = gtk_file_filter_new(); gtk_file_filter_add_suffix(filter, "enc"); gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(file_chooser), filter); g_object_unref(filter);
  g_signal_connect(file_chooser, "response", G_CALLBACK(on_file), widgets);
  gtk_widget_show(file_chooser);
}

int main (int argc, char * argv[]) {
  if(sodium_init()) { fprintf(stderr, "ERROR: sodium_init()\n"); return EXIT_FAILURE; }
  GtkApplication * app = gtk_application_new("com.github.fluxrider.vault", G_APPLICATION_FLAGS_NONE);
  g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);
  int status = g_application_run(G_APPLICATION(app), argc, argv); g_object_unref(app); return status;
}