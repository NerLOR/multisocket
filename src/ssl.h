


static void multi_ssl_init() {
    SSL_load_error_strings();
    SSL_library_init();
    ERR_load_crypto_strings();
    OpenSSL_add_all_algorithms();
}

static void multi_ssl_destroy() {
    ERR_free_strings();
    EVP_cleanup();
}

static const char* multi_ssl_get_error(SSL* ssl, int ret) {
    if (ret > 0) {
        return NULL;
    }

    unsigned long ret2 = ERR_get_error();
    const char* err2 = strerror(errno);
    const char* err1 = ERR_reason_error_string(ret2);

    switch (SSL_get_error(ssl, ret)) {
        case SSL_ERROR_NONE: return "none";
        case SSL_ERROR_ZERO_RETURN: return "closed";
        case SSL_ERROR_WANT_READ: return "want_read";
        case SSL_ERROR_WANT_WRITE: return "want_write";
        case SSL_ERROR_WANT_CONNECT: return "want_connect";
        case SSL_ERROR_WANT_ACCEPT: return "want_accept";
        case SSL_ERROR_WANT_X509_LOOKUP: return "want_cert_lookup";
        case SSL_ERROR_SYSCALL: return (ret2 == 0)? (ret == 0)? "protocol violation" : err2 : err1;
        case SSL_ERROR_SSL: return err1;
        default: return "unknown error";
    }
}

static int multi_ssl_close(SSL* ssl) {
    SSL_shutdown(ssl);
    //SSL_free(ssl);
    return 0;
}

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
    if (lua_gettop(L) != 1 && lua_gettop(L) != 2) {
        lua_pushnil(L);
        lua_pushstring(L, "Wrong number of arguments");
        return 2; // Return nil, [String] error
    } else if (!lua_isuserdata(L, 1) || !luaL_checkudata(L, 1, "multisocket_tcp")) {
        lua_pushnil(L);
        lua_pushstring(L, "Argument #0 has to be [Multisocket] socket (TCP)");
        return 2; // Return nil, [String] error
    } else if (lua_gettop(L) == 2 && !lua_istable(L, 2)) {
        lua_pushnil(L);
        lua_pushstring(L, "Argument #1 has to be [Table] sslParams");
        return 2; // Return nil, [String] error
    }

    // Cast userdata to Multisocket
    Multisocket *sock = (Multisocket *) lua_touserdata(L, 1);

    if (sock->enc) {
        lua_pushnil(L);
        lua_pushstring(L, "Socket is already encrypted");
        return 2; // Return nil, [String] error
    }

    if (sock->servers && !lua_gettop(L) == 2) {
        lua_pushnil(L);
        lua_pushstring(L, "Argument #1 has to be [Table] sslParams");
        return 2; // Return nil, [String] error
    }

    const SSL_METHOD *method;
    if (sock->servers) {
        method = TLSv1_2_server_method();
    } else if (sock->clients) {
        method = TLSv1_2_client_method();
    } else {
        method = TLSv1_2_method();
    }

    SSL_CTX *ctx = SSL_CTX_new(method);
    SSL_CTX_set_options(ctx, SSL_OP_SINGLE_DH_USE | SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);
    SSL_CTX_set_cipher_list(ctx, "HIGH:!aNULL:!kRSA:!PSK:!SRP:!MD5:!RC4");
    SSL_CTX_set_ecdh_auto(ctx, 1);

    const char* certfile = NULL; // "/home/lorenz/Documents/Projects/lua/multisocket/rsa/localhost_cert.pem";
    const char* keyfile = NULL; // "/home/lorenz/Documents/Projects/lua/multisocket/rsa/localhost_privkey.pem";

    if (lua_gettop(L) == 2) {
        lua_pushvalue(L, 2);
        lua_pushnil(L);
        while (lua_next(L, -2)) {
            lua_pushvalue(L, -2);
            const char *key = lua_tostring(L, -1);
            if (strcmp(key, "certfile") == 0) {
                if (!lua_isstring(L, -2)) {
                    lua_pushnil(L);
                    lua_pushstring(L, "Field 'certfile' has to be [String] path");
                    return 2; // Return nil, [String] err
                }
                certfile = lua_tostring(L, -2);
            } else if (strcmp(key, "keyfile") == 0) {
                if (!lua_isstring(L, -2)) {
                    lua_pushnil(L);
                    lua_pushstring(L, "Field 'keyfile' has to be [String] path");
                    return 2; // Return nil, [String] err
                }
                keyfile = lua_tostring(L, -2);
            }
            lua_pop(L, 2);
        }
        lua_pop(L, 1);
    }

    if (sock->servers) {
        if (certfile == NULL || keyfile == NULL) {
            lua_pushnil(L);
            lua_pushstring(L, "Field 'certfile' and/or 'keyfile' not found in sslParams");
            return 2; // Return nil, [String] error
        }

        if (SSL_CTX_use_certificate_file(ctx, certfile, SSL_FILETYPE_PEM) != 1) {
            lua_pushnil(L);
            lua_pushstring(L, ERR_reason_error_string(ERR_get_error()));
            return 2; // Return nil, [String] error
        }

        if (SSL_CTX_use_PrivateKey_file(ctx, keyfile, SSL_FILETYPE_PEM) != 1) {
            lua_pushnil(L);
            lua_pushstring(L, ERR_reason_error_string(ERR_get_error()));
            return 2; // Return nil, [String] error
        }
    }

    sock->ssl = SSL_new(ctx);
    SSL_set_fd(sock->ssl, sock->socket);
    sock->enc = 1;

    int ret = 0;
    if (sock->servers) {
        ret = SSL_accept(sock->ssl);
    } else if (sock->clients) {
        ret = SSL_connect(sock->ssl);
    }

    if (ret <= 0) {
        lua_pushnil(L);
        lua_pushstring(L, multi_ssl_get_error(sock->ssl, ret));
        return 2; // Return nil, [String] error
    }

    lua_pushboolean(L, 1);
    return 1; // Return [Boolean] success (true)
}

