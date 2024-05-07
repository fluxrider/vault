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
    bool ok; QString passphrase = QInputDialog::getText(this, tr("Passphrase"), tr("Passphrase:"), QLineEdit::Password, "", &ok); if (ok && !passphrase.isEmpty()) {
      // TODO decode file and populate widgets
      QMessageBox::critical(this, tr("Error"), tr("Failed to decode"));
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
  QApplication app(argc, argv);
  restore_folder();
  Window window; window.show();
  return app.exec();
}