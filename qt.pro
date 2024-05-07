QT += widgets
requires(qtConfig(combobox))

HEADERS = qt.h
SOURCES = qt.cpp

# pkg-config --cflags --libs libsodium
LIBS = -lsodium