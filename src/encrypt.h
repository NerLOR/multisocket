

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>



static void multi_init_ssl() {
    SSL_load_error_strings();
    SSL_library_init();
    OpenSSL_add_all_algorithms();
}

static void multi_destroy_ssl() {
    ERR_free_strings();
    EVP_cleanup();
}

/*static void ShutdownSSL() {
    SSL_shutdown(ssl);
    SSL_free(ssl);
}*/


static int multi_tcp_encrypt(lua_State *L) {



    return 0;
}

