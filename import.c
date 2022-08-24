// Copyright 2022 David Lareau. This source code form is subject to the terms of the Mozilla Public License 2.0.
// gcc --pedantic -Wall -Werror-implicit-function-declaration import.c $(pkg-config --cflags --libs libsodium) -o import
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <sodium.h>
#include <sys/mman.h> // mlock()
#include <stdlib.h>
#include <termios.h>
    
static const char * write_encrypted_entry(const char * master_passphrase, const char * entry, const char * url, const char * username, const char * password, const char * notes) {
  const char * error = NULL;
  // derive key
  unsigned char key[crypto_aead_xchacha20poly1305_ietf_KEYBYTES] = {0}; if(mlock(key, crypto_aead_xchacha20poly1305_ietf_KEYBYTES) == -1) error = "mlock()"; else {
  unsigned char salt[crypto_pwhash_SALTBYTES]; randombytes_buf(salt, crypto_pwhash_SALTBYTES);
  uint64_t algo = crypto_pwhash_ALG_ARGON2ID13, algo_p1 = crypto_pwhash_OPSLIMIT_MODERATE, algo_p2 = crypto_pwhash_MEMLIMIT_MODERATE;
  if(crypto_pwhash(key, crypto_aead_xchacha20poly1305_ietf_KEYBYTES, master_passphrase, strlen(master_passphrase), salt, (unsigned long long)algo_p1, (size_t)algo_p2, (int)algo)) error = "crypto_pwhash()"; else {
  // read entry
  size_t entry_len = strlen(entry);
  size_t url_len = strlen(url);
  size_t username_len = strlen(username);
  size_t password_len = strlen(password);
  size_t notes_len = strlen(notes);
  // encrypt
  unsigned char nonce[crypto_aead_xchacha20poly1305_ietf_NPUBBYTES]; randombytes_buf(nonce, crypto_aead_xchacha20poly1305_ietf_NPUBBYTES);
  size_t src_n = entry_len+1+url_len+1+username_len+1+password_len+1+notes_len; char src[src_n]; if(mlock(src, src_n) == -1) error = "mlock()"; else {
  char * ptr = src;
  memcpy(ptr, entry, entry_len); ptr += entry_len; *ptr++ = '\n';
  memcpy(ptr, url, url_len); ptr += url_len; *ptr++ = '\n';
  memcpy(ptr, username, username_len); ptr += username_len; *ptr++ = '\n';
  memcpy(ptr, password, password_len); ptr += password_len; *ptr++ = '\n';
  memcpy(ptr, notes, notes_len);
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
  // TODO sanitize entry for filename
  // filename is derived from entry name and timestamp
  time_t now; if(time(&now) == -1) error = "time()"; else { struct tm * t = localtime(&now); if(!t) error = "localtime()"; else {
  char filename[1024]; int n = snprintf(filename, 1024, "%s.%04d-%02d-%02d_%02d-%02d-%02d.enc", entry, t->tm_year+1900, t->tm_mon, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec); if(n > 1024 || n == -1) error = "snprintf()"; else {
  int fd = open(filename, O_CREAT | O_EXCL | O_WRONLY, S_IRUSR | S_IWUSR ); if(fd == -1) error = "open()"; else {
  if(writev(fd, iov, 7) == -1) error = "writev()"; else {
  if(close(fd) == -1) error = "close()"; else {
  }}}}}}}}}}
  explicit_bzero(key, crypto_aead_xchacha20poly1305_ietf_KEYBYTES); munlock(key, crypto_aead_xchacha20poly1305_ietf_KEYBYTES);
  return error;
}


int main(int argc, char * argv[]) {
  if(sodium_init()) { fprintf(stderr, "ERROR: sodium_init()\n"); return EXIT_FAILURE; }
  const char * error = NULL;
  
  struct termios termold, termnew; tcgetattr(0, &termold); termnew = termold; termnew.c_lflag &= ~ECHO; termnew.c_cc[VINTR] = termnew.c_cc[VWERASE] = termnew.c_cc[VSUSP] = termnew.c_cc[VSTOP] = termnew.c_cc[VSTART] = termnew.c_cc[VQUIT] = termnew.c_cc[VLNEXT] = termnew.c_cc[VKILL] = termnew.c_cc[VEOF] = '\0';
  tcsetattr(0, TCSANOW, &termnew);
  char * master_passphrase = NULL; size_t len = 0; size_t read = getline(&master_passphrase, &len, stdin); mlock(master_passphrase, len);
  tcsetattr(0, TCSANOW, &termold);
  if(master_passphrase[read-1] == '\n') master_passphrase[read-1] = '\0';

  error = write_encrypted_entry(master_passphrase, "test_entry", "test_url", "test_username", "test_password", "test_notes");
  if(error) fprintf(stderr, "ERROR: %s\n", error);

  explicit_bzero(master_passphrase, len); munlock(master_passphrase, len); free(master_passphrase);
  return error? EXIT_FAILURE : EXIT_SUCCESS;
}