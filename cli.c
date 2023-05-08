// Copyright 2023 David Lareau. This source code form is subject to the terms of the Mozilla Public License 2.0.
// gcc --pedantic -Wall -Werror-implicit-function-declaration cli.c $(pkg-config --cflags --libs libsodium libbsd-overlay) -o cli
#include <sodium.h>
#include <unistd.h> // access()
#include <fcntl.h> // open()
#include <sys/mman.h> // mlock()
#include <sys/uio.h> // writev()
#include <readpassphrase.h>
#include <string.h>

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

int main (int argc, char * argv[]) {
  if(argc != 2) { fprintf(stderr, "Usage: ./cli filename\n"); return EXIT_FAILURE; }
  if(sodium_init()) { fprintf(stderr, "ERROR: sodium_init()\n"); return EXIT_FAILURE; }
  const char * path = argv[1];
  const char * error = NULL;

  uint64_t algo, algo_p1, algo_p2; size_t encrypted_n; int fd; unsigned char salt[crypto_pwhash_SALTBYTES]; unsigned char nonce[crypto_aead_xchacha20poly1305_ietf_NPUBBYTES]; error = decrypt_init(path, &encrypted_n, &fd, &algo, &algo_p1, &algo_p2, salt, nonce); if(error && fd != -1 && close(fd) == -1) error = "close()";
  if(!error) {
    unsigned char encrypted[encrypted_n];
    unsigned char decrypted[encrypted_n - crypto_aead_xchacha20poly1305_ietf_ABYTES + 1]; if(mlock(decrypted, encrypted_n - crypto_aead_xchacha20poly1305_ietf_ABYTES + 1) == -1) error = "mlock()";
    if(!error) {
      char master_buffer[1024]; const char * master_passphrase = readpassphrase("Master Passphrase: ", master_buffer, sizeof(master_buffer), RPP_REQUIRE_TTY); if(!master_passphrase) error = "failed to read passphrase";
      if(!error) {
        size_t decrypted_n; error = decrypt(fd, encrypted, decrypted, &decrypted_n, encrypted_n, algo, algo_p1, algo_p2, salt, nonce, master_passphrase);
        explicit_bzero(master_buffer, sizeof(master_buffer));
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
      explicit_bzero(decrypted, encrypted_n - crypto_aead_xchacha20poly1305_ietf_ABYTES + 1); if(munlock(decrypted, encrypted_n - crypto_aead_xchacha20poly1305_ietf_ABYTES + 1) == -1) error = "munlock";
    }
  } else { if(fd != -1) close(fd); }

  if(error) { fprintf(stderr, "ERROR: %s\n", error); return EXIT_FAILURE; }
  return EXIT_SUCCESS;
}