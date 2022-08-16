// Copyright 2022 David Lareau. This source code form is subject to the terms of the Mozilla Public License 2.0.
// gcc --pedantic -Wall -Werror-implicit-function-declaration vault.c $(pkg-config --cflags --libs libsodium)
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <sodium.h>

#define E_CHK(std_func) __extension__ ({ typeof(std_func) retval; if((retval = std_func) == (typeof(std_func))-1) { perror("ERROR: " #std_func); exit(EXIT_FAILURE); } retval; })

int main(int argc, char * argv[]) {
  if(sodium_init()) { fprintf(stderr, "ERROR: sodium_init()\n"); return EXIT_FAILURE; }

  // key derivation TODO use salt read from header of decrypt
  const char * passphrase = "hello my friend";
  unsigned char key[crypto_aead_xchacha20poly1305_ietf_KEYBYTES];
  unsigned char salt[crypto_pwhash_SALTBYTES]; randombytes_buf(salt, crypto_pwhash_SALTBYTES);
  uint64_t algo = crypto_pwhash_ALG_ARGON2ID13;
  uint64_t algo_p1 = crypto_pwhash_OPSLIMIT_MODERATE;
  uint64_t algo_p2 = crypto_pwhash_MEMLIMIT_MODERATE;
  if(crypto_pwhash(key, crypto_aead_xchacha20poly1305_ietf_KEYBYTES, passphrase, strlen(passphrase), salt, (unsigned long long)algo_p1, (size_t)algo_p2, (int)algo)) { fprintf(stderr, "ERROR: crypto_pwhash()\n"); return EXIT_FAILURE; }

  {
    // encrypt
    unsigned char nonce[crypto_aead_xchacha20poly1305_ietf_NPUBBYTES]; randombytes_buf(nonce, crypto_aead_xchacha20poly1305_ietf_NPUBBYTES);
    unsigned char * src = "username\npassword\nurl\nnote1\nnote2"; size_t src_n = strlen(src);
    unsigned char dst[src_n+crypto_aead_xchacha20poly1305_ietf_ABYTES];
    unsigned long long _dst_n; crypto_aead_xchacha20poly1305_ietf_encrypt(dst, &_dst_n, src, src_n, NULL, 0, NULL, nonce, key);
    uint64_t encrypted_n = _dst_n;
    // write encrypted file (kdf_algo, kdf_algo_p1, kdf_algo_p2, kdf_salt, encryption_nonce, encrypted_len, encrypted_message)
    struct iovec iov[7];
    iov[0].iov_base = &algo; iov[0].iov_len = sizeof(uint64_t);
    iov[1].iov_base = &algo_p1; iov[1].iov_len = sizeof(uint64_t);
    iov[2].iov_base = &algo_p2; iov[2].iov_len = sizeof(uint64_t);
    iov[3].iov_base = salt; iov[3].iov_len = crypto_pwhash_SALTBYTES;
    iov[4].iov_base = nonce; iov[4].iov_len = crypto_aead_xchacha20poly1305_ietf_NPUBBYTES;
    iov[5].iov_base = &encrypted_n; iov[5].iov_len = sizeof(uint64_t);
    iov[6].iov_base = dst; iov[6].iov_len = encrypted_n;
    int fd = E_CHK(open("entry.encrypted", O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR ));
    E_CHK(writev(fd, iov, 7));
    E_CHK(close(fd));
  }

  // decrypt
  {
    algo = algo_p1 = algo_p2 = 0; memset(salt, 0, crypto_pwhash_SALTBYTES);
    unsigned char nonce[crypto_aead_xchacha20poly1305_ietf_NPUBBYTES];
    uint64_t encrypted_n;
    int fd = E_CHK(open("entry.encrypted", O_RDONLY));
    struct iovec iov[6];
    iov[0].iov_base = &algo; iov[0].iov_len = sizeof(uint64_t);
    iov[1].iov_base = &algo_p1; iov[1].iov_len = sizeof(uint64_t);
    iov[2].iov_base = &algo_p2; iov[2].iov_len = sizeof(uint64_t);
    iov[3].iov_base = salt; iov[3].iov_len = crypto_pwhash_SALTBYTES;
    iov[4].iov_base = nonce; iov[4].iov_len = crypto_aead_xchacha20poly1305_ietf_NPUBBYTES;
    iov[5].iov_base = &encrypted_n; iov[5].iov_len = sizeof(uint64_t);
    if(encrypted_n <= crypto_aead_xchacha20poly1305_ietf_ABYTES) { fprintf(stderr, "ERROR: encrypted length too short\n"); return EXIT_FAILURE; }
    E_CHK(readv(fd, iov, 6));
    unsigned char encrypted[encrypted_n];
    ssize_t n = E_CHK(read(fd, encrypted, encrypted_n)); if(n != encrypted_n) { fprintf(stderr, "ERROR: read()\n"); return EXIT_FAILURE; }
    unsigned char buffer[encrypted_n - crypto_aead_xchacha20poly1305_ietf_ABYTES + 1]; // +1 tmp, null terminate
    unsigned long long buffer_n;
    if(crypto_aead_xchacha20poly1305_ietf_decrypt(buffer, &buffer_n, NULL, encrypted, encrypted_n, NULL, 0, nonce, key)) { fprintf(stderr, "ERROR: crypto_aead_xchacha20poly1305_ietf_decrypt()\n"); return EXIT_FAILURE; }
    E_CHK(close(fd));
    buffer[buffer_n] = '\0'; // tmp
    printf("Decrypted %s\n", buffer);
  }
  
  return EXIT_SUCCESS;
}