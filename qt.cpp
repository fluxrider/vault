// qmake6 && make

#include "qt.h"

#include <QApplication>
#include <QTimer>
#include <QClipboard>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QGridLayout>
#include <QPlainTextEdit>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QFile>
#include <QFileInfo>

#include <sodium.h>
#include <sys/mman.h> // mlock()
#include <sys/uio.h> // writev()
#include <fcntl.h> // open()
#include <unistd.h> // read(), close()

#if defined(__ANDROID__)
#include <android/log.h>
#define flog(...) __android_log_print(ANDROID_LOG_WARN, "Archon", __VA_ARGS__)
#define ferr(...) __android_log_print(ANDROID_LOG_ERROR, "Archon", __VA_ARGS__)
#else
#include <stdio.h>
#define flog(...) printf(__VA_ARGS__)
#define ferr(...) fprintf(stderr, __VA_ARGS__)
#endif

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
    ssize_t n = read(fd, encrypted_buffer, encrypted_n); if(n != (ssize_t)encrypted_n) error = "read()"; else { if(close(fd) == -1) error = "close()"; else {
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

static QString * folder;
void remember_folder(const QString & filename) {
  QFileInfo info(filename); delete folder; folder = new QString(info.absolutePath());
  QFile file("folder.txt"); if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    file.write(folder->toUtf8());
    file.close();
  }
}
void restore_folder() {
  delete folder;
  QFile file("folder.txt"); if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    folder = new QString(file.readLine());
    file.close();
  } else {
    folder = new QString;
  }
}

Window::Window(QWidget * parent)
: QWidget(parent)
{
  name_edit = new QLineEdit; name_edit->setPlaceholderText("name");
  url_edit = new QLineEdit; url_edit->setPlaceholderText("url");
  username_edit = new QLineEdit; username_edit->setPlaceholderText("username"); username_edit->setEchoMode(QLineEdit::Password);
  password_edit = new QLineEdit; password_edit->setPlaceholderText("password"); password_edit->setEchoMode(QLineEdit::Password);
  notes_edit = new QPlainTextEdit; notes_edit->setPlaceholderText("notes");

  QLabel * name_label = new QLabel(tr("id"));
  QLabel * url_label = new QLabel(tr("url")); url_label->setPixmap(QIcon::fromTheme(QIcon::ThemeIcon::GoHome).pixmap(QSize(16, 16)));
  QLabel * username_label = new QLabel(tr("usr"));
  QLabel * password_label = new QLabel(tr("pass")); password_label->setPixmap(QIcon::fromTheme(QIcon::ThemeIcon::DialogPassword).pixmap(QSize(16, 16)));
  QLabel * notes_label = new QLabel(tr("notes"));

  QPushButton * save = new QPushButton(QIcon::fromTheme(QIcon::ThemeIcon::DocumentSave), tr("Save"));
  QPushButton * open = new QPushButton(QIcon::fromTheme(QIcon::ThemeIcon::DocumentOpen), tr("Open"));
  QPushButton * copy_url = new QPushButton(QIcon::fromTheme(QIcon::ThemeIcon::EditCopy), tr("Copy"));
  QPushButton * copy_username = new QPushButton(QIcon::fromTheme(QIcon::ThemeIcon::EditCopy), tr("Copy"));
  QPushButton * copy_password = new QPushButton(QIcon::fromTheme(QIcon::ThemeIcon::EditCopy), tr("Copy"));
  QIcon eye("qt-eye.png");
  QPushButton * viz_username = new QPushButton(eye, tr("Viz"));
  QPushButton * viz_password = new QPushButton(eye, tr("Viz"));
  
  clear_clipboard_timer = new QTimer(this);
  clear_clipboard_timer->setInterval(10000);
  clear_clipboard_timer->setSingleShot(true);
  connect(clear_clipboard_timer, &QTimer::timeout, this, &Window::on_clear_clipboard);

  connect(save, &QPushButton::clicked, this, &Window::on_save);
  connect(open, &QPushButton::clicked, this, &Window::on_open);
  connect(copy_url, &QPushButton::clicked, this, &Window::on_copy_url);
  connect(copy_username, &QPushButton::clicked, this, &Window::on_copy_username);
  connect(copy_password, &QPushButton::clicked, this, &Window::on_copy_password);
  connect(viz_username, &QPushButton::clicked, this, &Window::on_viz_username);
  connect(viz_password, &QPushButton::clicked, this, &Window::on_viz_password);

  QGridLayout * layout = new QGridLayout;
  layout->addWidget(name_label, 0, 0); layout->addWidget(name_edit, 0, 1, 1, -1);
  layout->addWidget(url_label, 1, 0); layout->addWidget(url_edit, 1, 1, 1, 9); layout->addWidget(copy_url, 1, 10);
  layout->addWidget(username_label, 2, 0); layout->addWidget(username_edit, 2, 1, 1, 9); layout->addWidget(viz_username, 2, 10); layout->addWidget(copy_username, 2, 11);
  layout->addWidget(password_label, 3, 0); layout->addWidget(password_edit, 3, 1, 1, 9); layout->addWidget(viz_password, 3, 10); layout->addWidget(copy_password, 3, 11);
  layout->addWidget(notes_label, 4, 0); layout->addWidget(notes_edit, 4, 1, 10, -1);
  QGridLayout * buttons_layout = new QGridLayout; buttons_layout->addWidget(save, 0, 0); buttons_layout->addWidget(open, 0, 1); layout->addLayout(buttons_layout, 14, 0, -1, -1);
  setLayout(layout);

  setWindowTitle(tr("Password Vault"));
}

void Window::on_save() {
}

void Window::on_open() {
  QString filename = QFileDialog::getOpenFileName(this, tr("Open"), *folder, tr(".enc (*.enc)")); if(!filename.isEmpty()) { // printf("filename: %s\n", filename.toLocal8Bit().data());
    remember_folder(filename);
    ask_for_passphrase:
    bool ok; QString passphrase = QInputDialog::getText(this, tr("Passphrase"), tr("Passphrase:"), QLineEdit::Password, "", &ok); if (ok && !passphrase.isEmpty()) {
      uint64_t algo, algo_p1, algo_p2; size_t encrypted_n; int fd; unsigned char salt[crypto_pwhash_SALTBYTES]; unsigned char nonce[crypto_aead_xchacha20poly1305_ietf_NPUBBYTES]; char * path = filename.toLocal8Bit().data(); const char * error = decrypt_init(path, &encrypted_n, &fd, &algo, &algo_p1, &algo_p2, salt, nonce); if(error && fd != -1 && ::close(fd) == -1) error = "close()";
      if(!error) {
        unsigned char encrypted[encrypted_n];
        const char * master_passphrase = passphrase.toLocal8Bit().data();
        unsigned char decrypted[encrypted_n - crypto_aead_xchacha20poly1305_ietf_ABYTES + 1]; if(mlock(decrypted, encrypted_n - crypto_aead_xchacha20poly1305_ietf_ABYTES + 1) == -1) error = "mlock()";
        if(!error) {
          size_t decrypted_n = 0; error = decrypt(fd, encrypted, decrypted, &decrypted_n, encrypted_n, algo, algo_p1, algo_p2, salt, nonce, master_passphrase);
          if(!error) {
            decrypted[decrypted_n] = '\0';
            // populate entry widgets
            char * ptr = (char *)decrypted;
            char * entry = ptr; ptr = strchr(ptr, '\n'); if(ptr) { *ptr++ = '\0';
            char * url = ptr; ptr = strchr(ptr, '\n'); if(ptr) { *ptr++ = '\0';
            char * username = ptr; ptr = strchr(ptr, '\n'); if(ptr) { *ptr++ = '\0';
            char * password = ptr; ptr = strchr(ptr, '\n'); if(ptr) { *ptr++ = '\0';
            char * notes = ptr; notes_edit->setPlainText(QString(notes));
            } password_edit->setText(password); } username_edit->setText(username); } url_edit->setText(url); } name_edit->setText(entry);
          }
          explicit_bzero(decrypted, encrypted_n - crypto_aead_xchacha20poly1305_ietf_ABYTES + 1); if(munlock(decrypted, encrypted_n - crypto_aead_xchacha20poly1305_ietf_ABYTES + 1) == -1) error = "munlock";
        }
      } else { if(fd != -1) ::close(fd); }
      // if something bad happened, show a message dialog on top, and don't destroy the master password dialog
      if(error) {
        QMessageBox::critical(this, tr("Error"), error);
        goto ask_for_passphrase;
      }
    }
  }
}

void Window::on_copy_url() { clear_clipboard_timer->stop(); QGuiApplication::clipboard()->setText(url_edit->text()); }
void Window::on_copy_username() { clear_clipboard_timer->stop(); QGuiApplication::clipboard()->setText(username_edit->text()); clear_clipboard_timer->start(); }
void Window::on_copy_password() { clear_clipboard_timer->stop(); QGuiApplication::clipboard()->setText(password_edit->text()); clear_clipboard_timer->start(); }
void Window::on_clear_clipboard() { QGuiApplication::clipboard()->setText(""); } // WEIRD: QGuiApplication::clipboard()->clear() doesn't work
void Window::on_viz_username() { username_edit->setEchoMode(username_edit->echoMode() == QLineEdit::Password? QLineEdit::Normal : QLineEdit::Password); }
void Window::on_viz_password() { password_edit->setEchoMode(password_edit->echoMode() == QLineEdit::Password? QLineEdit::Normal : QLineEdit::Password); }

int main(int argc, char * argv[]) {
  if(sodium_init()) { ferr("ERROR: sodium_init()\n"); return EXIT_FAILURE; }
  QApplication app(argc, argv);
  restore_folder();
  Window window; window.show();
  return app.exec();
}