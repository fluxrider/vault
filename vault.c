// gcc --pedantic -Wall -Werror-implicit-function-declaration vault.c $(pkg-config --cflags --libs libsodium)
#include <stdio.h>
#include <sys/uio.h>
#include <sodium.h>

#define E_CHK(std_func) __extension__ ({ typeof(std_func) retval; if((retval = std_func) == (typeof(std_func))-1) { perror("ERROR: " #std_func); exit(EXIT_FAILURE); } retval; })

int main(int argc, char * argv[]) {
  if(sodium_init()) { fprintf(stderr, "ERROR: sodium_init()\n"); return EXIT_FAILURE; }

  // key derivation
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
    unsigned char src[src_n];
    unsigned char dst[src_n+crypto_aead_xchacha20poly1305_ietf_ABYTES];
    unsigned long long dst_n; crypto_aead_xchacha20poly1305_ietf_encrypt(dst, &dst_n, src, src_n, NULL, 0, NULL, nonce, key);
    // write encrypted file (kdf_algo, kdf_algo_p1, kdf_algo_p2, kdf_salt, encryption_nonce, encrypted_message)
    struct iovec iov[6];
    iov[0].iov_base = &algo; iov[0].iov_len = sizeof(uint64_t);
    iov[1].iov_base = &algo_p1; iov[1].iov_len = sizeof(uint64_t);
    iov[2].iov_base = &algo_p2; iov[2].iov_len = sizeof(uint64_t);
    iov[0].iov_base = salt; iov[0].iov_len = crypto_pwhash_SALTBYTES;
    iov[0].iov_base = nonce; iov[0].iov_len = crypto_aead_xchacha20poly1305_ietf_NPUBBYTES;
    iov[0].iov_base = dst; iov[0].iov_len = dst_n;
    int fd = E_CHK(open("out.encrypted", O_RDONLY));
    E_CHK(writev(fd, iov, 6));
    E_CHK(close(fd));
  }

  // decrypt
  {
    unsigned char src[n];
    unsigned char dst[n-crypto_secretstream_xchacha20poly1305_HEADERBYTES-crypto_secretstream_xchacha20poly1305_ABYTES];
    crypto_secretstream_xchacha20poly1305_state st;
    if(crypto_secretstream_xchacha20poly1305_init_pull(&st, src, key)) { fprintf(stderr, "ERROR: crypto_secretstream_xchacha20poly1305_init_pull()\n"); return EXIT_FAILURE; };
    unsigned char tag; if(crypto_secretstream_xchacha20poly1305_pull(&st, dst, NULL, &tag, src+crypto_secretstream_xchacha20poly1305_HEADERBYTES, n-crypto_secretstream_xchacha20poly1305_HEADERBYTES, NULL, 0)) { fprintf(stderr, "ERROR: crypto_secretstream_xchacha20poly1305_pull()\n"); return EXIT_FAILURE; };
    if(tag != crypto_secretstream_xchacha20poly1305_TAG_FINAL) { fprintf(stderr, "ERROR: unexpected tag\n"); return EXIT_FAILURE; };
    
  }
  
  return EXIT_SUCCESS;
}