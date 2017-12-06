


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

/**
 * Lua Method
 * Encrypt the TPC connection with SSL/TLS
 * @param0 [Multisocket] sock (TCP)
 * @param1 [Table] sslParams
 * @return1 [Boolean] success / nil
 * @return2 nil / [String] error
 */
static int multi_tcp_encrypt(lua_State *L) {
    // Check if there are two parameters and if they have valid values
    if (lua_gettop(L) != 1) {
        lua_pushnil(L);
        lua_pushstring(L, "Wrong number of arguments");
        return 2; // Return nil, [String] error
    } else if (!lua_isuserdata(L, 1) || !luaL_checkudata(L, 1, "multisocket_tcp")) {
        lua_pushnil(L);
        lua_pushstring(L, "Argument #0 has to be [Multisocket] socket (TCP)");
        return 2; // Return nil, [String] error
    }

    // Cast userdata to Multisocket
    Multisocket *sock = (Multisocket *) lua_touserdata(L, 1);

    if (sock->enc) {
        lua_pushnil(L);
        lua_pushstring(L, "Socket is already encrypted");
        return 2; // Return nil, [String] error
    }

    const SSL_METHOD *method;
    if (sock->servers) {
        method = TLSv1_2_server_method();
    } else if (sock->clients) {
        method = TLSv1_2_client_method();
    }

    SSL_CTX *sslctx = SSL_CTX_new(method);
    SSL_CTX_set_options(sslctx, SSL_OP_SINGLE_DH_USE);

    //int use_cert = SSL_CTX_use_certificate_file(sslctx, "/serverCertificate.pem" , SSL_FILETYPE_PEM);
    //int use_prv = SSL_CTX_use_PrivateKey_file(sslctx, "/serverCertificate.pem", SSL_FILETYPE_PEM);

    sock->ssl = SSL_new(sslctx);
    SSL_set_fd(sock->ssl, sock->socket);
    sock->enc = 1;

    SSL_connect(sock->ssl);

    lua_pushboolean(L, 1);
    return 1; // Return [Boolean] success (true)
}

