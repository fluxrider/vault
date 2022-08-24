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
#include <stdbool.h>
    
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
  // sanitize entry for filename
  char * sanitized_entry = strdup(entry); { char * ptr = sanitized_entry; while(*ptr) { switch(*ptr) { case '/': case '\\': case '?': case '%': case '*': case ':': case '|': case '"': case '<': case '>': case '.': case ',': case ';': case '=': case ' ': case '+': case '[': case ']': case '(': case ')': case '!': case '@': case '$':case '#': case '-': *ptr = '_'; } ptr++; } }
  // filename is derived from entry name and timestamp
  time_t now; if(time(&now) == -1) error = "time()"; else { struct tm * t = localtime(&now); if(!t) error = "localtime()"; else {
  char filename[1024]; int n = snprintf(filename, 1024, "%s.%04d-%02d-%02d_%02d-%02d-%02d.enc", sanitized_entry, t->tm_year+1900, t->tm_mon, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec); if(n > 1024 || n == -1) error = "snprintf()"; else {
  int fd = open(filename, O_CREAT | O_EXCL | O_WRONLY, S_IRUSR | S_IWUSR ); if(fd == -1) error = "open()"; else {
  if(writev(fd, iov, 7) == -1) error = "writev()"; else {
  if(close(fd) == -1) error = "close()"; else {
  }}}}}}free(sanitized_entry);}}}}
  explicit_bzero(key, crypto_aead_xchacha20poly1305_ietf_KEYBYTES); munlock(key, crypto_aead_xchacha20poly1305_ietf_KEYBYTES);
  return error;
}


int main(int argc, char * argv[]) {
  if(argc != 2) { fprintf(stderr, "usage: import keepassxc.cvs\n"); return EXIT_FAILURE; }
  if(sodium_init()) { fprintf(stderr, "ERROR: sodium_init()\n"); return EXIT_FAILURE; }
  const char * error = NULL;
  
  // read master passphrase
  char * master_passphrase = NULL; size_t mp_len = 0; {
    printf("master passphrase:\n");
    struct termios termold, termnew; tcgetattr(0, &termold); termnew = termold; termnew.c_lflag &= ~ECHO; termnew.c_cc[VINTR] = termnew.c_cc[VWERASE] = termnew.c_cc[VSUSP] = termnew.c_cc[VSTOP] = termnew.c_cc[VSTART] = termnew.c_cc[VQUIT] = termnew.c_cc[VLNEXT] = termnew.c_cc[VKILL] = termnew.c_cc[VEOF] = '\0';
    tcsetattr(0, TCSANOW, &termnew);
    size_t read = getline(&master_passphrase, &mp_len, stdin); mlock(master_passphrase, mp_len);
    tcsetattr(0, TCSANOW, &termold);
    if(master_passphrase[read-1] == '\n') master_passphrase[read-1] = '\0';
  }

  // buffers
  char group[1024]; // that's a keepassxc thing
  char entry[1024];
  char url[1024];
  char username[1024];
  char password[1024];
  char * notes = malloc(1024*100);
  char * buffers[] = {group, entry, username, password, url, notes, NULL, NULL, NULL, NULL};

  // read input file (i.e. cvs export of keepassxc)
  int fd = open(argv[1], O_RDONLY); if(fd == -1) error = "open()";
  while(!error) {
    size_t index = -1; // [0,9]
    size_t lens[] = {0,0,0,0,0,0,0,0,0};
    ssize_t n;
    bool outside = true, maybe_outside = false;
    for(;;) {
      char c; n = read(fd, &c, 1); if(n == -1) error = "read()"; if(n == 0) break;
      if(maybe_outside) {
        maybe_outside = false;
        if(c == '"') {
          if(buffers[index]) { if((index != 5 && lens[index] < 1023) || (index == 5 && lens[index] < 1024*100-1)) buffers[index][lens[index]++] = c; else { error = "buffer overflow"; break; } }
          continue;
        } else {
          outside = true;
        }
      }
      if(outside && c == '\n') break;
      if(outside && c == '"') { outside = false; index++; continue; }
      if(!outside && c == '"') { maybe_outside = true; continue; }
      if(!outside && buffers[index]) { if((index != 5 && lens[index] < 1023) || (index == 5 && lens[index] < 1024*100-1)) buffers[index][lens[index]++] = c; else { error = "buffer overflow"; break; } }
      //printf("T %zd %c\n", index, c);
    }
    if(!error) {
      for(int i = 0; i < 10; i++) if(buffers[i]) buffers[i][lens[i]] = '\0';
      printf("Parsed %zd %zd %zd %zd %zd\n", lens[1], lens[2], lens[3], lens[4], lens[5]);
      printf("entry: %s\n", entry);
      printf("url: %s\n", url);
      printf("username: %s\n", username);
      printf("password: %s\n", password);
      printf("notes: %s\n", notes);
      if(lens[1] != 0 && strcmp(entry, "Title") && strcmp(group, "Root/Recycle Bin")) error = write_encrypted_entry(master_passphrase, entry, url, username, password, notes);
      if(n == 0) break;
    }
  }
  if(error) fprintf(stderr, "ERROR: %s\n", error);
  if(fd != -1) close(fd);
  free(notes);

  explicit_bzero(master_passphrase, mp_len); munlock(master_passphrase, mp_len); free(master_passphrase);
  return error? EXIT_FAILURE : EXIT_SUCCESS;
}