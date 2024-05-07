#ifndef MAIN_H
#define MAIN_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QPlainTextEdit;
class QTimer;
QT_END_NAMESPACE

class Window : public QWidget {
  Q_OBJECT

public:
  Window(QWidget * parent = nullptr);

public slots:
  void on_save();
  void on_open();
  void on_copy_url();
  void on_copy_username();
  void on_copy_password();
  void on_clear_clipboard();
  void on_viz_username();
  void on_viz_password();

private:
  QLineEdit * name_edit;
  QLineEdit * url_edit;
  QLineEdit * username_edit;
  QLineEdit * password_edit;
  QPlainTextEdit * notes_edit;
  QTimer * clear_clipboard_timer;
};

#endif