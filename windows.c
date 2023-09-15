// Copyright 2023 David Lareau. This source code form is subject to the terms of the Mozilla Public License 2.0.
// cl /I "C:\_\apps\libsodium-1.0.19-stable-msvc\libsodium\include" windows.c /link /LIBPATH:"C:\_\apps\libsodium-1.0.19-stable-msvc\libsodium\Win32\Release\v143\dynamic" libsodium.lib
// set PATH=%PATH%;C:\_\apps\libsodium-1.0.19-stable-msvc\libsodium\Win32\Release\v143\dynamic
// windows.exe
#include <sodium.h>
#include <stdio.h>
#include <string.h>

#include <Windows.h>
#include <memoryapi.h>
#include <strsafe.h>
#include <malloc.h>

static const char * decrypt_init(const char * path, size_t * out_encrypted_n, FILE ** out_fd, uint64_t * out_algo, uint64_t * out_algo_p1, uint64_t * out_algo_p2, unsigned char out_salt[crypto_pwhash_SALTBYTES], unsigned char out_nonce[crypto_aead_xchacha20poly1305_ietf_NPUBBYTES]) {
  const char * error = NULL;
  // read header
  FILE * fd = *out_fd = fopen(path, "rb"); if(!fd) error = "fopen()";
  if(!error) if(fread(out_algo, sizeof(uint64_t), 1, fd) != 1) error = "fread()";
  if(!error) if(fread(out_algo_p1, sizeof(uint64_t), 1, fd) != 1) error = "fread()";
  if(!error) if(fread(out_algo_p2, sizeof(uint64_t), 1, fd) != 1) error = "fread()";
  if(!error) if(fread(out_salt, crypto_pwhash_SALTBYTES, 1, fd) != 1) error = "fread()";
  if(!error) if(fread(out_nonce, crypto_aead_xchacha20poly1305_ietf_NPUBBYTES, 1, fd) != 1) error = "fread()";
  uint64_t encrypted_n; if(!error) if(fread(&encrypted_n, sizeof(uint64_t), 1, fd) != 1) error = "fread()";
  if(!error) {
    if(encrypted_n <= crypto_aead_xchacha20poly1305_ietf_ABYTES) error = "encrypted length too short"; else {
      *out_encrypted_n = encrypted_n;
    }
  }
  return error;
}

static const char * decrypt(FILE * fd, unsigned char * encrypted_buffer, unsigned char * decrypted_buffer, size_t * out_decrypted_n, size_t encrypted_n, uint64_t algo, uint64_t algo_p1, uint64_t algo_p2, unsigned char salt[crypto_pwhash_SALTBYTES], unsigned char nonce[crypto_aead_xchacha20poly1305_ietf_NPUBBYTES], const char * master_passphrase) {
  const char * error = NULL;
  unsigned char key[crypto_aead_xchacha20poly1305_ietf_KEYBYTES] = {0}; if(!VirtualLock(key, crypto_aead_xchacha20poly1305_ietf_KEYBYTES)) error = "VirtualLock()"; else {
    // read encrypted data
    SSIZE_T n = fread(encrypted_buffer, 1, encrypted_n, fd); if(n != encrypted_n) error = "fread()"; else { if(fclose(fd)) error = "close()"; else {
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
  SecureZeroMemory(key, crypto_aead_xchacha20poly1305_ietf_KEYBYTES); if(!VirtualUnlock(key, crypto_aead_xchacha20poly1305_ietf_KEYBYTES)) error = "VirtualUnlock() a";
  return error;
}

int main (int argc, char * argv[]) {
  if(argc != 2) { fprintf(stderr, "Usage: windows-cli.exe filename\n"); return EXIT_FAILURE; }
  if(sodium_init()) { fprintf(stderr, "ERROR: sodium_init()\n"); return EXIT_FAILURE; }
  const char * path = argv[1];
  const char * error = NULL;

  uint64_t algo, algo_p1, algo_p2; size_t encrypted_n; FILE * fd; unsigned char salt[crypto_pwhash_SALTBYTES]; unsigned char nonce[crypto_aead_xchacha20poly1305_ietf_NPUBBYTES]; error = decrypt_init(path, &encrypted_n, &fd, &algo, &algo_p1, &algo_p2, salt, nonce); if(error && fd && fclose(fd)) error = "close()";
  if(!error) {
    unsigned char * encrypted = _alloca(encrypted_n);
    unsigned char * decrypted = _alloca(encrypted_n - crypto_aead_xchacha20poly1305_ietf_ABYTES + 1); if(!VirtualLock(decrypted, encrypted_n - crypto_aead_xchacha20poly1305_ietf_ABYTES + 1)) error = "VirtualLock()";
    if(!error) {
      printf("Master Passphrase: \n");
      HANDLE stdinHandle = GetStdHandle(STD_INPUT_HANDLE); DWORD mode; GetConsoleMode(stdinHandle, &mode); mode &= ~ENABLE_ECHO_INPUT; SetConsoleMode(stdinHandle, mode);
      char master_buffer[1024]; if(FAILED(StringCbGetsA(master_buffer, sizeof(master_buffer)))) error = "failed to read passphrase";
      SetConsoleMode(stdinHandle, mode | ENABLE_ECHO_INPUT);
      if(!error) {
        const char * master_passphrase = master_buffer;
        size_t decrypted_n; error = decrypt(fd, encrypted, decrypted, &decrypted_n, encrypted_n, algo, algo_p1, algo_p2, salt, nonce, master_passphrase);
        SecureZeroMemory(master_buffer, sizeof(master_buffer));
        if(!error) {
          decrypted[decrypted_n] = '\0';
          // populate entry widgets
          char * ptr = (char *)decrypted;
          char * entry = ptr; ptr = strchr(ptr, '\n'); if(ptr) { *ptr++ = '\0';
          char * url = ptr; ptr = strchr(ptr, '\n'); if(ptr) { *ptr++ = '\0';
          char * username = ptr; ptr = strchr(ptr, '\n'); if(ptr) { *ptr++ = '\0';
          char * password = ptr; ptr = strchr(ptr, '\n'); if(ptr) { *ptr++ = '\0';
          char * notes = ptr; printf("notes: %s\n", notes);
          } printf("password: %s\n", password); } printf("username: %s\n", username); } printf("url: %s\n", url); } printf("entry: %s\n", entry);
        }
      }
      SecureZeroMemory(decrypted, encrypted_n - crypto_aead_xchacha20poly1305_ietf_ABYTES + 1); if(!VirtualUnlock(decrypted, encrypted_n - crypto_aead_xchacha20poly1305_ietf_ABYTES + 1)) ;/*error = "VirtualUnlock b";*/ // This unlock always seems to fail.
    }
  } else { if(fd) fclose(fd); }

  if(error) { fprintf(stderr, "ERROR: %s\n", error); return EXIT_FAILURE; }
  return EXIT_SUCCESS;
}