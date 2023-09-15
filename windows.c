// Copyright 2023 David Lareau. This source code form is subject to the terms of the Mozilla Public License 2.0.
// cl /I "C:\_\apps\libsodium-1.0.19-stable-msvc\libsodium\include" windows.c /link /LIBPATH:"C:\_\apps\libsodium-1.0.19-stable-msvc\libsodium\Win32\Release\v143\dynamic" libsodium.lib
// set PATH=%PATH%;C:\_\apps\libsodium-1.0.19-stable-msvc\libsodium\Win32\Release\v143\dynamic
// windows.exe
#include <sodium.h>
#include <stdio.h>

int main(int argc, char * argv[]) {
  if(sodium_init()) { fprintf(stderr, "ERROR: sodium_init()\n"); return EXIT_FAILURE; }
  printf("Hello, World! This is a native C program compiled on the command line.\n");
  return EXIT_SUCCESS;
}