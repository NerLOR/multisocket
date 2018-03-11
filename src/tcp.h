


/**
 * Lua Function
 * Create a new TCP/IPv6 socket
 * @return1 [Multisocket] socket / nil
 * @return2 nil / [String] error
 */
static int multi_tcp6(lua_State *L) {
    // Check if there are any parameters
    if (lua_gettop(L) != 0) {
        lua_pushnil(L);
        lua_pushstring(L, "Wrong number of arguments");
        return 2; // Return nil, [String] error
    }

    // Init socket filedescriptor
    int desc = 0;
    if ((desc = socket(AF_INET6, SOCK_STREAM, 0)) == -1) {
        lua_pushnil(L);
        lua_pushstring(L, strerror(errno));
        return 2; // Return nil, [String] error
    }

    // Allocate memory for Multisocket
    Multisocket *sock = (Multisocket *) lua_newuserdata(L, sizeof(Multisocket));
    sock->socket = desc; // Set the socket filedescriptor
    sock->ssl = NULL;
    sock->ctx = NULL;
    sock->startT = getcurrenttime(); // Set connection start time in nanoseconds
    sock->lastT = getcurrenttime();  // Set last signal time in nanoseconds
    sock->recB = 0;  // Init received bytes
    sock->sndB = 0;  // Init sent bytes

    sock->listen = 0;
    sock->conn = 0;
    sock->servers = 0;
    sock->clients = 0;
    sock->tcp = 1;
    sock->udp = 0;
    sock->enc = 0;
    sock->ipv6 = 1;
    sock->ipv4 = 0;

    luaL_getmetatable(L, "multisocket_tcp"); // Get multisocket_tcp metatable
    lua_setmetatable(L, -2); // Set the metatable to the Multisocket

    return 1; // Return [Multisocket] socket
}

/**
 * Lua Function
 * Create a new TCP/IPv4 socket
 * @return1 [Multisocket] socket / nil
 * @return2 nil / [String] error
 */
static int multi_tcp4(lua_State *L) {
    // Check if there are any parameters
    if (lua_gettop(L) != 0) {
        lua_pushnil(L);
        lua_pushstring(L, "Wrong number of arguments");
        return 2; // Return nil, [String] error
    }

    // Init socket filedescriptor
    int desc = 0;
    if ((desc = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        lua_pushnil(L);
        lua_pushstring(L, strerror(errno));
        return 2; // Return nil, [String] error
    }

    // Allocate memory for Multisocket
    Multisocket *sock = (Multisocket *) lua_newuserdata(L, sizeof(Multisocket));
    sock->socket = desc; // Set the socket filedescriptor
    sock->ssl = NULL;
    sock->ctx = NULL;
    sock->startT = getcurrenttime(); // Set connection start time in nanoseconds
    sock->lastT = getcurrenttime();  // Set last signal time in nanoseconds
    sock->recB = 0;  // Init received bytes
    sock->sndB = 0;  // Init sent bytes

    sock->listen = 0;
    sock->conn = 0;
    sock->servers = 0;
    sock->clients = 0;
    sock->tcp = 1;
    sock->udp = 0;
    sock->enc = 0;
    sock->ipv6 = 0;
    sock->ipv4 = 1;

    luaL_getmetatable(L, "multisocket_tcp"); // Get multisocket_tcp metatable
    lua_setmetatable(L, -2); // Set the metatable to the Multisocket

    return 1; // Return [Multisocket] socket
}

/**
 * Lua Method
 * Bind the socket to an address an a port
 * @param0 [Multisocket] socket (TCP)
 * @param1 [String] address
 * @param2 [Integer] port (0-65535)
 * @return1 [Boolean] success / nil
 * @return2 nil / [String] error
 */
static int multi_tcp_bind(lua_State *L) {
    // Check if there are three parameters and if they have valid values
    if (lua_gettop(L) != 3) {
        lua_pushnil(L);
        lua_pushstring(L, "Wrong number of arguments");
        return 2; // Return nil, [String] error
    } else if (!lua_isuserdata(L, 1) || !luaL_checkudata(L, 1, "multisocket_tcp")) {
        lua_pushnil(L);
        lua_pushstring(L, "Argument #0 has to be [Multisocket] socket (TCP)");
        return 2; // Return nil, [String] error
    } else if (!lua_isstring(L, 2)) {
        lua_pushnil(L);
        lua_pushstring(L, "Argument #1 has to be [String] address");
        return 2; // Return nil, [String] error
    } else if (!lua_isinteger(L, 3) || lua_tointeger(L, 3) > 0xFFFF || lua_tointeger(L, 3) < 0) {
        lua_pushnil(L);
        lua_pushstring(L, "Argument #2 has to be [Integer] port (0-65535)");
        return 2; // Return nil, [String] error
    }

    // Cast userdata to Multisocket
    Multisocket *sock = (Multisocket *) lua_touserdata(L, 1);

    // Load parameters into variables
    size_t addressLength = 0;
    const char *address = lua_tolstring(L, 2, &addressLength);
    unsigned short port = (unsigned short) lua_tointeger(L, 3);

    // Init address structs for IPv6 and IPv4
    struct sockaddr_in6 address6;
    struct sockaddr_in address4;
    bzero(&address6, sizeof(address6));
    bzero(&address4, sizeof(address4));

    if (sock->ipv6) {
        // TCP/IPv6
        address6.sin6_family = AF_INET6;
        if (addressLength == 1 && address[0] == '*') {
            address6.sin6_addr = in6addr_any; // The OS decides which address it uses
        } else if (inet_pton(AF_INET6, address, &address6.sin6_addr) == 0) {
            lua_pushnil(L);
            lua_pushstring(L, strerror(errno));
            return 2; // Return nil, [String] error
        }
        // The address is set in inet_pton()
    } else if (sock->ipv4) {
        // TCP/IPv4
        address4.sin_family = AF_INET;
        if (addressLength == 1 && address[0] == '*') {
            address4.sin_addr.s_addr = INADDR_ANY; // The OS decides which address it uses
        } else if (inet_pton(AF_INET, address, &address4.sin_addr) == 0) {
            lua_pushnil(L);
            lua_pushstring(L, strerror(errno));
            return 2; // Return nil, [String] error
        }
    }

    // Copy the port to sin_port in network byte order
    address6.sin6_port = htons(port);
    address4.sin_port = htons(port);

    // Get the right address type and address size
    struct sockaddr *socketAddress;
    socklen_t addressSize;
    if (sock->ipv6) {
        socketAddress = (struct sockaddr *)&address6;
        addressSize = sizeof(address6);
    } else if (sock->ipv4) {
        socketAddress = (struct sockaddr *)&address4;
        addressSize = sizeof(address4);
    }

    // Bind the socket to address and port
    if (bind(sock->socket, socketAddress, addressSize) == -1) {
        lua_pushnil(L);
        lua_pushstring(L, strerror(errno));
        return 2; // Return nil, [String] error
    }

    // ReusaAddr
    int enable = 1;
    if (setsockopt(sock->socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) < 0) {
        lua_pushnil(L);
        lua_pushstring(L, strerror(errno));
        return 2; // Return nil, [String] error
    }

    lua_pushboolean(L, 1);
    return 1; // Return [Boolean] success (true)
}

/**
 * Lua Method
 * Mark the socket as passive socket
 * The socket will be used to accept incoming connection requests
 * @param0 [Multisocket] socket (TCP)
 * @param1 [Integer] backlog (0-2147483647)
 * @return1 [Boolean] success / nil
 * @return2 nil / [String] error
 */
static int multi_tcp_listen(lua_State *L) {
    // Check if there are two parameters and if they have valid values
    if (lua_gettop(L) != 2) {
        lua_pushnil(L);
        lua_pushstring(L, "Wrong number of arguments");
        return 2; // Return nil, [String] error
    } else if (!lua_isuserdata(L, 1) || !luaL_checkudata(L, 1, "multisocket_tcp")) {
        lua_pushnil(L);
        lua_pushstring(L, "Argument #0 has to be [Multisocket] socket (TCP)");
        return 2; // Return nil, [String] error
    } else if (!lua_isinteger(L, 2) || lua_tointeger(L, 2) > 0x7FFFFFFF || lua_tointeger(L, 2) < 0) {
        lua_pushnil(L);
        lua_pushstring(L, "Argument #1 has to be [Integer] backlog (0-2147483647)");
        return 2; // Return nil, [String] error
    }

    // Cast userdata to Multisocket
    Multisocket *sock = (Multisocket *) lua_touserdata(L, 1);

    // Load parameters into variables
    int backlog = (int) lua_tointeger(L, 2);

    // Use the socket as listener
    if (listen(sock->socket, backlog) == -1) {
        lua_pushnil(L);
        lua_pushstring(L, strerror(errno));
        return 2; // Return nil, [String] error
    }

    sock->listen = 1;
    sock->servers = 1;

    lua_pushboolean(L, 1);
    return 1; // Return [Boolean] success (true)
}

/**
 * Lua Method
 * Wait until a connection is ready to be accepted
 * @param0 [Multisocket] socket (TCP)
 * @return1 [Multisocket] client / nil
 * @return2 nil / [String] error
 */
static int multi_tcp_accept(lua_State *L) {
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

    // Init address variables
    struct sockaddr *address;
    socklen_t len;

    if (sock->ipv6) {
        // TCP/IPv6
        struct sockaddr_in6 address6;
        bzero(&address6, sizeof(address6));
        len = sizeof(address6);
        address = (struct sockaddr *) &address6;
    } else if (sock->ipv4) {
        // TCP/IPv4
        struct sockaddr_in address4;
        bzero(&address4, sizeof(address4));
        len = sizeof(address4);
        address = (struct sockaddr *) &address4;
    }

    // Accept a new incoming connection
    int desc = accept(sock->socket, address, &len);
    if (desc == -1) {
        lua_pushnil(L);
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            lua_pushstring(L, "timeout");
        } else if (errno == ECONNRESET) {
            lua_pushstring(L, "closed");
        } else {
            lua_pushstring(L, strerror(errno));
        }
        return 2; // Return nil, [String] error
    }

    // Allocate memory for Multisocket
    Multisocket *client = (Multisocket *) lua_newuserdata(L, sizeof(Multisocket));
    client->socket = desc; // Set the socket filedescriptor
    client->ssl = NULL;
    client->startT = getcurrenttime(); // Set connection start time in nanoseconds
    client->lastT = getcurrenttime();  // Set last signal time in nanoseconds
    client->recB = 0;  // Init received bytes
    client->sndB = 0;  // Init sent bytes

    client->listen = 0;
    client->conn = 1;
    client->servers = 1;
    client->clients = 0;
    client->tcp = 1;
    client->udp = 0;
    client->enc = sock->enc;
    client->ipv6 = sock->ipv6;
    client->ipv4 = sock->ipv4;

    luaL_getmetatable(L, "multisocket_tcp");
    lua_setmetatable(L, -2);

    return 1; // Return [Multisocket] client
}

/**
 * Lua Method
 * @param0 [Multisocket] socket (TCP)
 * @param1 [String] address (address or domain)
 * @param2 [Integer] port (0-65535)
 * @return1 [Boolean] success / nil
 * @return2 nil / [String] error
 */
static int multi_tcp_connect(lua_State *L) {
    // Check if there are three parameters and if they have valid values
    if (lua_gettop(L) != 3) {
        lua_pushnil(L);
        lua_pushstring(L, "Wrong number of arguments");
        return 2; // Return nil, [String] error
    } else if (!lua_isuserdata(L, 1) || !luaL_checkudata(L, 1, "multisocket_tcp")) {
        lua_pushnil(L);
        lua_pushstring(L, "Argument #0 has to be [Multisocket] socket (TCP)");
        return 2; // Return nil, [String] error
    } else if (!lua_isstring(L, 2)) {
        lua_pushnil(L);
        lua_pushstring(L, "Argument #1 has to be [String] address (address or domain)");
        return 2; // Return nil, [String] error
    } else if (!lua_isinteger(L, 3) || lua_tointeger(L, 3) > 0xFFFF || lua_tointeger(L, 3) < 0) {
        lua_pushnil(L);
        lua_pushstring(L, "Argument #2 has to be [Integer] port (0-65535)");
        return 2; // Return nil, [String] error
    }

    // Cast userdata to Multisocket
    Multisocket *sock = (Multisocket *) lua_touserdata(L, 1);

    // Load parameters into variables
    size_t addressLength = 0;
    const char *address = lua_tolstring(L, 2, &addressLength);
    unsigned short port = (unsigned short) lua_tointeger(L, 3);

    // Init address structs for IPv6 and IPv4
    struct sockaddr_in6 address6;
    struct sockaddr_in address4;
    bzero(&address6, sizeof(address6));
    bzero(&address4, sizeof(address4));

    address6.sin6_family = AF_INET6;
    address4.sin_family = AF_INET;

    struct addrinfo *result, *next;
    struct addrinfo hint;
    bzero(&hint, sizeof(hint));
    hint.ai_family = (sock->ipv6) ? AF_INET6 : AF_INET;

    int ret = getaddrinfo(address, NULL, &hint, &result);
    if (ret != 0) {
        lua_pushnil(L);
        lua_pushstring(L, gai_strerror(ret));
        return 2; // Return nil, [String] error
    }

    struct sockaddr *addr;
    socklen_t addrLen = 0;

    for (next = result; next != NULL; next = next->ai_next) {
        addrLen = next->ai_addrlen;
        if (sock->ipv6 && next->ai_family == AF_INET6) {
            memcpy(&address6, next->ai_addr, next->ai_addrlen);
            break;
        } else if (sock->ipv4 && next->ai_family == AF_INET) {
            memcpy(&address4, next->ai_addr, next->ai_addrlen);
            break;
        }
    }

    address6.sin6_port = htons(port);
    address4.sin_port = htons(port);

    if (sock->ipv6) {
        addr = (struct sockaddr *) &address6;
    } else if (sock->ipv4) {
        addr = (struct sockaddr *) &address4;
    }

    if (connect(sock->socket, addr, addrLen) == -1) {
        lua_pushnil(L);
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            lua_pushstring(L, "timeout");
        } else {
            lua_pushstring(L, strerror(errno));
        }
        return 2; // Return nil, [String] error
    }

    sock->conn = 1;
    sock->clients = 1;

    lua_pushboolean(L, 1);
    return 1; // Return [Boolean] success (true)
}

/**
 * Lua Method
 * @param0 [Multisocket] socket (TCP)
 * @param1 nil / [String] until (Length: 1-MULTISOCKET_BUFFER_SIZE) / [Integer] numBytes (64 Bit)
 * @return1 [String] data / nil
 * @return2 nil / [String] error
 * @return3 nil / [String] partData
 */
static int multi_tcp_receive(lua_State *L) {
    // Check if there are two parameters and if they have valid values
    if (lua_gettop(L) != 2 && lua_gettop(L) != 1) {
        lua_pushnil(L);
        lua_pushstring(L, "Wrong number of arguments");
        lua_pushstring(L, "");
        return 3; // Return nil, [String] error, [String] partData
    } else if (!lua_isuserdata(L, 1) || !luaL_checkudata(L, 1, "multisocket_tcp")) {
        lua_pushnil(L);
        lua_pushstring(L, "Argument #0 has to be [Multisocket] socket (TCP)");
        lua_pushstring(L, "");
        return 3; // Return nil, [String] error, [String] partData
    } else if (lua_gettop(L) == 2 && ((!lua_isinteger(L, 2) || lua_tointeger(L, 2) < 0) && !lua_isstring(L, 2) && !lua_isnil(L, 2))) {
        lua_pushnil(L);
        lua_pushfstring(L, "Argument #1 has to be nil / [String] until (Length: 1-%i) / [Integer] numBytes (64 Bit)", MULTISOCKET_BUFFER_SIZE);
        lua_pushstring(L, "");
        return 3; // Return nil, [String] error, [String] partData
    }

    // Cast userdata to Multisocket
    Multisocket *sock = (Multisocket *) lua_touserdata(L, 1);

    // Init variables
    long wantedBytes = 0;
    long wantedStringLen = 0;
    const char *wantedString = "";

    // Check which argument is passed
    char mode;
    if (lua_gettop(L) == 1 || lua_isnil(L, 2)) {
        mode = 0;
    } else if (lua_gettop(L) == 2 && lua_isinteger(L, 2)) {
        mode = 1;
        wantedBytes = lua_tointeger(L, 2);
    } else if (lua_gettop(L) == 2 && lua_isstring(L, 2)) {
        mode = 2;
        wantedString = lua_tolstring(L, 2, &wantedStringLen);
        if (wantedStringLen < 1 || wantedStringLen > MULTISOCKET_BUFFER_SIZE) {
            lua_pushnil(L);
            lua_pushfstring(L, "Argument #1 has to be nil / [String] until (Length: 1-%i) / [Integer] numBytes (64 Bit)", MULTISOCKET_BUFFER_SIZE);
            lua_pushstring(L, "");
            return 3; // Return nil, [String] error, [String] partData
        }
    } else {
        lua_pushnil(L);
        lua_pushstring(L, "Something went wrong");
        lua_pushstring(L, "");
        return 3; // Return nil, [String] error, [String] partData
    }

    printf("MODE %i\n", mode);

    // Init lua string
    luaL_Buffer str;
    luaL_buffinit(L, &str);
    long strLen = 0;

    // Init buffer and pre-buffer
    char pre[MULTISOCKET_BUFFER_SIZE];
    char buffer[MULTISOCKET_BUFFER_SIZE];

    // Init poll filedescriptor(s) to
    struct pollfd ufds[1];
    ufds[0].fd = sock->socket;
    ufds[0].events = POLLIN | POLLOUT | POLLERR | POLLHUP;

    // Receive until the condition applies
    while (1) {
        if (mode == 0) {

            long size;
            if (sock->enc) {
                size = SSL_read(sock->ssl, buffer, sizeof(buffer));
            } else {
                size = recv(sock->socket, buffer, sizeof(buffer), 0);
            }
            if (size < 0) {
                lua_pushnil(L);
                if (sock->enc) {
                    lua_pushstring(L, multi_ssl_get_error(sock->ssl, size));
                } else {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        lua_pushstring(L, "timeout");
                    } else if (errno == ECONNRESET) {
                        lua_pushstring(L, "closed");
                    } else {
                        lua_pushstring(L, strerror(errno));
                    }
                }
                luaL_pushresult(&str);
                return 3; // Return nil, [String] error, [String] partData
            }
            sock->recB += size;
            sock->lastT = getcurrenttime();
            strLen += size;
            luaL_addlstring(&str, buffer, size);

            if (size != sizeof(buffer)) {
                luaL_pushresult(&str);
                lua_pushnil(L);
                lua_pushnil(L);
                return 3;
            }


        } else if (mode == 1) {

            long len = sizeof(buffer);
            if (len > wantedBytes-strLen) {
                len = wantedBytes-strLen;
            }
            long size;
            if (sock->enc) {
                size = SSL_read(sock->ssl, buffer, len);
            } else {
                size = recv(sock->socket, buffer, len, 0);
            }

            if (size < 0) {
                lua_pushnil(L);
                if (sock->enc) {
                    lua_pushstring(L, multi_ssl_get_error(sock->ssl, size));
                } else {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        lua_pushstring(L, "timeout");
                    } else if (errno == ECONNRESET) {
                        lua_pushstring(L, "closed");
                    } else {
                        lua_pushstring(L, strerror(errno));
                    }
                }
                luaL_pushresult(&str);
                return 3; // Return nil, [String] error, [String] partData
            }
            sock->recB += size;
            sock->lastT = getcurrenttime();
            strLen += size;
            luaL_addlstring(&str, buffer, size);
            if (strLen == wantedBytes) {
                luaL_pushresult(&str);
                lua_pushnil(L);
                lua_pushnil(L);
                return 3; // Return [String] data, nil, nil
            }


        } else if (mode == 2) {

            memcpy(pre, buffer, sizeof(buffer)); // Copy buffer to pre

            long size;
            if (sock->enc) {
                size = SSL_peek(sock->ssl, buffer, sizeof(buffer));
            } else {
                size = recv(sock->socket, buffer, sizeof(buffer), MSG_PEEK);
            }
            if (size < 0 || sock->enc && size == 0) {
                lua_pushnil(L);
                if (sock->enc) {
                    lua_pushstring(L, multi_ssl_get_error(sock->ssl, (int) size));
                } else {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        lua_pushstring(L, "timeout");
                    } else if (errno == ECONNRESET) {
                        lua_pushstring(L, "closed");
                    } else {
                        lua_pushstring(L, strerror(errno));
                    }
                }
                luaL_pushresult(&str);
                return 3; // Return nil, [String] error, [String] partData
            }

            char *ptr = buffer - wantedStringLen + 1;
            while (1) {
                ptr = memchr(ptr, wantedString[0], size);
                if (ptr == NULL || memcmp(ptr, wantedString, wantedStringLen) == 0) {
                    break;
                }
                ptr++;
            }
            if (ptr != NULL) {
                strLen += ptr - buffer + wantedStringLen;
                luaL_addlstring(&str, buffer, ptr - buffer);
                long ret;
                if (sock->enc) {
                    ret = SSL_read(sock->ssl, buffer, ptr - buffer + wantedStringLen);
                } else {
                    ret = recv(sock->socket, buffer, ptr - buffer + wantedStringLen, 0);
                }
                if (ret < 0) {
                    lua_pushnil(L);
                    if (sock->enc) {
                        lua_pushstring(L, multi_ssl_get_error(sock->ssl, size));
                    } else {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            lua_pushstring(L, "timeout");
                        } else if (errno == ECONNRESET) {
                            lua_pushstring(L, "closed");
                        } else {
                            lua_pushstring(L, strerror(errno));
                        }
                    }
                    luaL_pushresult(&str);
                    return 3; // Return nil, [String] error, [String] partData
                }
                sock->recB += ptr - buffer + wantedStringLen;
                sock->lastT = getcurrenttime();
                luaL_pushresult(&str);
                lua_pushnil(L);
                lua_pushnil(L);
                return 3; // Return [String] data, nil, nil
            } else {
                strLen += size;
                luaL_addlstring(&str, buffer, size);
                long ret;
                if (sock->enc) {
                    ret = SSL_read(sock->ssl, buffer, size);
                } else {
                    ret = recv(sock->socket, buffer, size, 0);
                }
                if (ret < 0) {
                    lua_pushnil(L);
                    if (sock->enc) {
                        lua_pushstring(L, multi_ssl_get_error(sock->ssl, ret));
                    } else {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            lua_pushstring(L, "timeout");
                        } else if (errno == ECONNRESET) {
                            lua_pushstring(L, "closed");
                        } else {
                            lua_pushstring(L, strerror(errno));
                        }
                    }
                    luaL_pushresult(&str);
                    return 3; // Return nil, [String] error, [String] partData
                }
                sock->recB += size;
                sock->lastT = getcurrenttime();
            }
        }

        if (poll(ufds, 1, 0) == -1) {
            lua_pushnil(L);
            lua_pushstring(L, strerror(errno));
            lua_pushstring(L, "");
            return 3; // Return nil, [String] error, [String] partData
        }

        /*if ((ufds[0].revents & POLLIN) == 0) {
            if (mode == 0) {
                luaL_pushresult(&str);
                if ((ufds[0].revents & POLLHUP) || (ufds[0].revents & POLLERR)) {
                    lua_pushstring(L, "closed");
                } else {
                    lua_pushnil(L);
                }
                lua_pushnil(L);
                return 3; // Return [String] data, [String] error, nil
            } else {
                lua_pushnil(L);
                if ((ufds[0].revents & POLLHUP) || (ufds[0].revents & POLLERR)) {
                    lua_pushstring(L, "closed");
                } else if (ufds[0].revents & POLLOUT) {
                    lua_pushstring(L, "want_write");
                } else {
                    lua_pushstring(L, "error");
                }
                luaL_pushresult(&str);
                return 3; // Return nil, [String] error, [String] partData
            }
        }*/
    }
}

/**
 * Lua Method
 * Send data to the peer
 * @param0 [Multisocket] socket (TCP)
 * @param1 [String] data
 * @return1 [Integer] byteNum / nil
 * @return2 nil / [String] error
 * @return3 nil / [Integer] partByteNum
 */
static int multi_tcp_send(lua_State *L) {
    // Check if there are two parameters and if they have valid values
    if (lua_gettop(L) != 2) {
        lua_pushnil(L);
        lua_pushstring(L, "Wrong number of arguments");
        lua_pushinteger(L, 0);
        return 3; // Return nil, [String] error, [Integer] partByteNum
    } else if (!lua_isuserdata(L, 1) || !luaL_checkudata(L, 1, "multisocket_tcp")) {
        lua_pushnil(L);
        lua_pushstring(L, "Argument #0 has to be [Multisocket] socket (TCP)");
        lua_pushinteger(L, 0);
        return 3; // Return nil, [String] error, [Integer] partByteNum
    } else if (!lua_tostring(L, 2)) {
        lua_pushnil(L);
        lua_pushfstring(L, "Argument #1 has to be [String] data");
        lua_pushinteger(L, 0);
        return 3; // Return nil, [String] error, [Integer] partByteNum
    }

    // Cast userdata to Multisocket
    Multisocket *sock = (Multisocket *) lua_touserdata(L, 1);
    long dataSize = 0;
    long pos = 0;
    const char *data = lua_tolstring(L, 2, &dataSize);

    // Init poll filedescriptor(s) to
    struct pollfd ufds[1];
    ufds[0].fd = sock->socket;
    ufds[0].events = POLLIN | POLLOUT | POLLERR | POLLHUP;

    int s = MULTISOCKET_BUFFER_SIZE;
    while (pos < dataSize) {
        if (s > dataSize - pos) {
            s = (int) (dataSize - pos);
        }

        long trans;
        if (sock->enc) {
            trans = SSL_write(sock->ssl, data+pos, s);
        } else {
            trans = send(sock->socket, data+pos, s, 0);
        }

        if (trans < 0 || sock->enc && trans == 0) {
            lua_pushnil(L);
            if (sock->enc) {
                lua_pushstring(L, multi_ssl_get_error(sock->ssl, (int) trans));
            } else {
                if (poll(ufds, 1, 0) == -1) {
                    lua_pushstring(L, strerror(errno));
                } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    lua_pushstring(L, "timeout");
                } else if (errno == ECONNRESET) {
                    lua_pushstring(L, "closed");
                } else if (!sock->enc && ufds[0].revents & POLLIN) {
                    lua_pushstring(L, "want_read");
                } else {
                    lua_pushstring(L, strerror(errno));
                }
            }
            lua_pushinteger(L, pos);
            return 3; // Return nil, [String] error, [Integer] partByteNum
        }
        pos += trans;
        sock->sndB += trans;
        sock->lastT = getcurrenttime();
    }

    lua_pushinteger(L, pos);
    lua_pushnil(L);
    lua_pushnil(L);
    return 3; // Return [Integer] byteNum, nil, nil
}

/**
 * Lua Method
 * Close the socket connection
 * @param0 [Multisocket] socket (TCP)
 * @return1 [Boolean] success / nil
 * @return2 nil / [String] error
 */
static int multi_tcp_close(lua_State *L) {
    // Check if there are any parameters
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
        multi_ssl_close(sock);
    }

    // Close the socket connection
    if (close(sock->socket) == -1) {
        lua_pushnil(L);
        lua_pushstring(L, strerror(errno));
        return 2; // Return nil, [String] error
    }

    lua_pushboolean(L, 1);
    return 1; // Return [Boolean] success (true)
}



#include "tcp_support.h"


