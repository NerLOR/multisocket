


/**
 * Lua Method
 * Receive until CRLF and return the line without CRLF
 * @param0 [Multisocket] socket (TCP)
 * @return1 [String] data / nil
 * @return2 nil / [String] error
 * @return3 nil / [String] partData
 */
static int multi_tcp_receive_line(lua_State *L) {
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

    lua_pushcfunction(L, multi_tcp_receive);
    lua_pushvalue(L, 1);
    lua_pushstring(L, "\r\n");
    lua_call(L, 2, 3);
    return 3;
}


/**
 * Lua Method
 * @param0 [Multisocket] socket (TCP)
 * @param1 nil / [Number] timeout (seconds)
 * @return1 [Boolean] success / nil
 * @return2 nil / [String] error
 */
static int multi_tcp_set_timeout(lua_State *L) {
    // Check if there are two parameters and if they have valid values
    if (lua_gettop(L) != 2 && lua_gettop(L) != 1) {
        lua_pushnil(L);
        lua_pushstring(L, "Wrong number of arguments");
        return 2; // Return nil, [String] error
    } else if (!lua_isuserdata(L, 1) || !luaL_checkudata(L, 1, "multisocket_tcp")) {
        lua_pushnil(L);
        lua_pushstring(L, "Argument #0 has to be [Multisocket] socket (TCP)");
        return 2; // Return nil, [String] error
    } else if (lua_gettop(L) == 2 && (!lua_isnumber(L, 2) ||  lua_tonumber(L, 2) < 0)) {
        lua_pushnil(L);
        lua_pushstring(L, "Argument #1 has to be [Number] timeout");
        return 2; // Return nil, [String] error
    }

    // Cast userdata to Multisocket
    Multisocket *sock = (Multisocket *) lua_touserdata(L, 1);

    // Load parameters into variables
    // If timeval time is 0 then the timeout is unlimited
    // Therefore 1 will be added to tv_usec
    struct timeval time;
    bzero(&time, sizeof(time));
    if (lua_gettop(L) == 2) {
        double sec = lua_tonumber(L, 2);
        time.tv_sec = (long) sec;
        time.tv_usec = (long) ((sec - time.tv_sec) * 1000000) + 1;
    }

    if (setsockopt(sock->socket, SOL_SOCKET, SO_RCVTIMEO, &time, sizeof(time)) < 0 ||
        setsockopt(sock->socket, SOL_SOCKET, SO_SNDTIMEO, &time, sizeof(time)) < 0) {
        lua_pushnil(L);
        lua_pushstring(L, strerror(errno));
        return 2; // Return nil, [String] error
    }

    lua_pushboolean(L, 1);
    return 1; // Return [Boolean] success (true)
}

/**
 * Lua Method
 * Get the duration the socket is created
 * @param0 [Multisocket] socket
 * @return1 [Number] duration
 */
static int multi_get_duration(lua_State *L) {
    // Check if there are two parameters and if they have valid values
    if (lua_gettop(L) != 1) {
        lua_pushnil(L);
        lua_pushstring(L, "Wrong number of arguments");
        return 2; // Return nil, [String] error
    } else if (!lua_isuserdata(L, 1) || !luaL_checkudata(L, 1, "multisocket_tcp")) {
        lua_pushnil(L);
        lua_pushstring(L, "Argument #0 has to be [Multisocket] socket");
        return 2; // Return nil, [String] error
    }

    Multisocket *sock = (Multisocket *) lua_touserdata(L, 1);

    lua_pushnumber(L, ((double) getcurrenttime() - sock->startT) / 1000000000);
    return 1; // Return [Number] duration
}

/**
 * Lua Method
 * Get the time the socket was created
 * @param0 [Multisocket] socket
 * @return1 [Number] startTime (UNIX)
 */
static int multi_get_starttime(lua_State *L) {
    // Check if there are two parameters and if they have valid values
    if (lua_gettop(L) != 1) {
        lua_pushnil(L);
        lua_pushstring(L, "Wrong number of arguments");
        return 2; // Return nil, [String] error
    } else if (!lua_isuserdata(L, 1) || !luaL_checkudata(L, 1, "multisocket_tcp")) {
        lua_pushnil(L);
        lua_pushstring(L, "Argument #0 has to be [Multisocket] socket");
        return 2; // Return nil, [String] error
    }

    Multisocket *sock = (Multisocket *) lua_touserdata(L, 1);

    lua_pushnumber(L, ((double) sock->startT) / 1000000000);
    return 1; // Return [Number] startTime (UNIX)
}

/**
 * Lua Method
 * Get the time the socket got the last signal from the peer
 * @param0 [Multisocket] socket
 * @return1 [Number] lastTime (UNIX)
 */
static int multi_get_lasttime(lua_State *L) {
    // Check if there are two parameters and if they have valid values
    if (lua_gettop(L) != 1) {
        lua_pushnil(L);
        lua_pushstring(L, "Wrong number of arguments");
        return 2; // Return nil, [String] error
    } else if (!lua_isuserdata(L, 1) || !luaL_checkudata(L, 1, "multisocket_tcp")) {
        lua_pushnil(L);
        lua_pushstring(L, "Argument #0 has to be [Multisocket] socket");
        return 2; // Return nil, [String] error
    }

    Multisocket *sock = (Multisocket *) lua_touserdata(L, 1);

    lua_pushnumber(L, ((double) sock->lastT) / 1000000000);
    return 1; // Return [Number] lastSignal (UNIX)
}

/**
 * Lua Method
 * Get the number of bytes sent by the socket
 * @param0 [Multisocket] socket
 * @return1 [Integer] numBytesSent
 */
static int multi_get_sent(lua_State *L) {
    // Check if there are two parameters and if they have valid values
    if (lua_gettop(L) != 1) {
        lua_pushnil(L);
        lua_pushstring(L, "Wrong number of arguments");
        return 2; // Return nil, [String] error
    } else if (!lua_isuserdata(L, 1) || !luaL_checkudata(L, 1, "multisocket_tcp")) {
        lua_pushnil(L);
        lua_pushstring(L, "Argument #0 has to be [Multisocket] socket");
        return 2; // Return nil, [String] error
    }

    Multisocket *sock = (Multisocket *) lua_touserdata(L, 1);

    lua_pushinteger(L, sock->sndB);
    return 1; // Return [Integer] numBytesSent
}

/**
 * Lua Method
 * Get the number of bytes received by the socket
 * @param0 [Multisocket] socket
 * @return1 [Integer] numBytesReceived
 */
static int multi_get_received(lua_State *L) {
    // Check if there are two parameters and if they have valid values
    if (lua_gettop(L) != 1) {
        lua_pushnil(L);
        lua_pushstring(L, "Wrong number of arguments");
        return 2; // Return nil, [String] error
    } else if (!lua_isuserdata(L, 1) || !luaL_checkudata(L, 1, "multisocket_tcp")) {
        lua_pushnil(L);
        lua_pushstring(L, "Argument #0 has to be [Multisocket] socket");
        return 2; // Return nil, [String] error
    }

    Multisocket *sock = (Multisocket *) lua_touserdata(L, 1);

    lua_pushinteger(L, sock->recB);
    return 1; // Return [Integer] numBytesReceived
}


/**
 * Lua Method
 * Get the address of the socket
 * @param0 [Multisocket] socket
 * @return1 [String] address / nil
 * @return2 nil / [String] error
 */
static int multi_tcp_get_sockaddr(lua_State *L) {
    // Check if there are two parameters and if they have valid values
    if (lua_gettop(L) != 1) {
        lua_pushnil(L);
        lua_pushstring(L, "Wrong number of arguments");
        return 2; // Return nil, [String] error
    } else if (!lua_isuserdata(L, 1) || !luaL_checkudata(L, 1, "multisocket_tcp")) {
        lua_pushnil(L);
        lua_pushstring(L, "Argument #0 has to be [Multisocket] socket");
        return 2; // Return nil, [String] error
    }

    Multisocket *sock = (Multisocket *) lua_touserdata(L, 1);

    struct sockaddr_in6 address6;
    struct sockaddr_in address4;
    char string[INET6_ADDRSTRLEN+INET_ADDRSTRLEN];
    bzero(&address6, sizeof(address6));
    bzero(&address4, sizeof(address4));
    bzero(string, sizeof(string));
    socklen_t addrLen;

    struct sockaddr *address;
    if (sock->ipv6) {
        address = (struct sockaddr *) &address6;
        addrLen = sizeof(address6);
    } else if (sock->ipv4) {
        address = (struct sockaddr *) &address4;
        addrLen = sizeof(address4);
    }

    if (getsockname(sock->socket, address, &addrLen) < 0) {
        lua_pushnil(L);
        lua_pushstring(L, strerror(errno));
        return 2; // Return nil, [String] error
    }

    if (sock->ipv6) {
        if (inet_ntop(AF_INET6, &address6.sin6_addr, string, sizeof(string)) == 0) {
            lua_pushnil(L);
            lua_pushstring(L, strerror(errno));
            return 2; // Return nil, [String] error
        }
    } else if (sock->ipv4) {
        if (inet_ntop(AF_INET, &address4.sin_addr, string, sizeof(string)) == 0) {
            lua_pushnil(L);
            lua_pushstring(L, strerror(errno));
            return 2; // Return nil, [String] error
        }
    }

    lua_pushstring(L, string);
    return 1; // Return [String] address
}

/**
 * Lua Method
 * Get the port of the socket
 * @param0 [Multisocket] socket
 * @return1 [Integer] port / nil
 * @return2 nil / [String] error
 */
static int multi_tcp_get_sockport(lua_State *L) {
    // Check if there are two parameters and if they have valid values
    if (lua_gettop(L) != 1) {
        lua_pushnil(L);
        lua_pushstring(L, "Wrong number of arguments");
        return 2; // Return nil, [String] error
    } else if (!lua_isuserdata(L, 1) || !luaL_checkudata(L, 1, "multisocket_tcp")) {
        lua_pushnil(L);
        lua_pushstring(L, "Argument #0 has to be [Multisocket] socket");
        return 2; // Return nil, [String] error
    }

    Multisocket *sock = (Multisocket *) lua_touserdata(L, 1);

    struct sockaddr_in6 address6;
    struct sockaddr_in address4;
    bzero(&address6, sizeof(address6));
    bzero(&address4, sizeof(address4));
    socklen_t addrLen;

    struct sockaddr *address;
    if (sock->ipv6) {
        address = (struct sockaddr *) &address6;
        addrLen = sizeof(address6);
    } else if (sock->ipv4) {
        address = (struct sockaddr *) &address4;
        addrLen = sizeof(address4);
    }

    if (getsockname(sock->socket, address, &addrLen) < 0) {
        lua_pushnil(L);
        lua_pushstring(L, strerror(errno));
        return 2;
    }

    if (sock->ipv6) {
        lua_pushinteger(L, ntohs(((struct sockaddr_in6 *) address)->sin6_port));
    } else if (sock->ipv4) {
        lua_pushinteger(L, ntohs(((struct sockaddr_in *) address)->sin_port));
    }
    return 1; // Return [Integer] port
}

/**
 * Lua Method
 * Get the address and port of the socket
 * @param0 [Multisocket] socket (TCP)
 * @return1 [String] name / nil
 * @return2 nil / [String] error
 */
static int multi_tcp_get_sockname(lua_State *L) {
    // Check if there are two parameters and if they have valid values
    if (lua_gettop(L) != 1) {
        lua_pushnil(L);
        lua_pushstring(L, "Wrong number of arguments");
        return 2; // Return nil, [String] error
    } else if (!lua_isuserdata(L, 1) || !luaL_checkudata(L, 1, "multisocket_tcp")) {
        lua_pushnil(L);
        lua_pushstring(L, "Argument #0 has to be [Multisocket] socket");
        return 2; // Return nil, [String] error
    }

    Multisocket *sock = (Multisocket *) lua_touserdata(L, 1);

    luaL_Buffer str;
    luaL_buffinit(L, &str);

    lua_pushcfunction(L, multi_tcp_get_sockaddr);
    lua_pushvalue(L, 1);
    lua_call(L, 1, 2);
    if (lua_isnil(L, 2)) {
        lua_pushnil(L);
        lua_pushvalue(L, 3);
        return 2;
    }
    if (sock->ipv6) {
        luaL_addstring(&str, "[");
        luaL_addstring(&str, lua_tostring(L, 2));
        luaL_addstring(&str, "]:");
    } else if (sock->ipv4) {
        luaL_addstring(&str, lua_tostring(L, 2));
        luaL_addstring(&str, ":");
    }

    lua_pushcfunction(L, multi_tcp_get_sockport);
    lua_pushvalue(L, 1);
    lua_call(L, 1, 2);
    if (lua_isnil(L, 2)) {
        lua_pushnil(L);
        lua_pushvalue(L, 3);
        return 2;
    }
    luaL_addstring(&str, lua_tostring(L, 0));

    luaL_pushresult(&str);
    return 1; // Return [String] name
}

/**
 * Lua Method
 * Get the address of the peer
 * @param0 [Multisocket] socket
 * @return1 [String] address / nil
 * @return2 nil / [String] error
 */
static int multi_tcp_get_peeraddr(lua_State *L) {
    // Check if there are two parameters and if they have valid values
    if (lua_gettop(L) != 1) {
        lua_pushnil(L);
        lua_pushstring(L, "Wrong number of arguments");
        return 2; // Return nil, [String] error
    } else if (!lua_isuserdata(L, 1) || !luaL_checkudata(L, 1, "multisocket_tcp")) {
        lua_pushnil(L);
        lua_pushstring(L, "Argument #0 has to be [Multisocket] socket");
        return 2; // Return nil, [String] error
    }

    Multisocket *sock = (Multisocket *) lua_touserdata(L, 1);

    struct sockaddr_in6 address6;
    struct sockaddr_in address4;
    char string[INET6_ADDRSTRLEN+INET_ADDRSTRLEN];
    bzero(&address6, sizeof(address6));
    bzero(&address4, sizeof(address4));
    bzero(string, sizeof(string));
    socklen_t addrLen;

    struct sockaddr *address;
    if (sock->ipv6) {
        address = (struct sockaddr *) &address6;
        addrLen = sizeof(address6);
    } else if (sock->ipv4) {
        address = (struct sockaddr *) &address4;
        addrLen = sizeof(address4);
    }

    if (getpeername(sock->socket, address, &addrLen) < 0) {
        lua_pushnil(L);
        lua_pushstring(L, strerror(errno));
        return 2; // Return nil, [String] error
    }

    if (sock->ipv6) {
        if (inet_ntop(AF_INET6, &address6.sin6_addr, string, sizeof(string)) == 0) {
            lua_pushnil(L);
            lua_pushstring(L, strerror(errno));
            return 2; // Return nil, [String] error
        }
    } else if (sock->ipv4) {
        if (inet_ntop(AF_INET, &address4.sin_addr, string, sizeof(string)) == 0) {
            lua_pushnil(L);
            lua_pushstring(L, strerror(errno));
            return 2; // Return nil, [String] error
        }
    }

    lua_pushstring(L, string);
    return 1; // Return [String] address
}

/**
 * Lua Method
 * Get the port of the peer
 * @param0 [Multisocket] socket
 * @return1 [Integer] port / nil
 * @return2 nil / [String] error
 */
static int multi_tcp_get_peerport(lua_State *L) {
    // Check if there are two parameters and if they have valid values
    if (lua_gettop(L) != 1) {
        lua_pushnil(L);
        lua_pushstring(L, "Wrong number of arguments");
        return 2; // Return nil, [String] error
    } else if (!lua_isuserdata(L, 1) || !luaL_checkudata(L, 1, "multisocket_tcp")) {
        lua_pushnil(L);
        lua_pushstring(L, "Argument #0 has to be [Multisocket] socket");
        return 2; // Return nil, [String] error
    }

    Multisocket *sock = (Multisocket *) lua_touserdata(L, 1);

    struct sockaddr_in6 address6;
    struct sockaddr_in address4;
    bzero(&address6, sizeof(address6));
    bzero(&address4, sizeof(address4));
    socklen_t addrLen;

    struct sockaddr *address;
    if (sock->ipv6) {
        address = (struct sockaddr *) &address6;
        addrLen = sizeof(address6);
    } else if (sock->ipv4) {
        address = (struct sockaddr *) &address4;
        addrLen = sizeof(address4);
    }

    if (getpeername(sock->socket, address, &addrLen) < 0) {
        lua_pushnil(L);
        lua_pushstring(L, strerror(errno));
        return 2;
    }

    if (sock->ipv6) {
        lua_pushinteger(L, ntohs(((struct sockaddr_in6 *) address)->sin6_port));
    } else if (sock->ipv4) {
        lua_pushinteger(L, ntohs(((struct sockaddr_in *) address)->sin_port));
    }
    return 1; // Return [Integer] port
}

/**
 * Lua Method
 * Get the address and port of the peer
 * @param0 [Multisocket] socket (TCP)
 * @return1 [String] name / nil
 * @return2 nil / [String] error
 */
static int multi_tcp_get_peername(lua_State *L) {
    // Check if there are two parameters and if they have valid values
    if (lua_gettop(L) != 1) {
        lua_pushnil(L);
        lua_pushstring(L, "Wrong number of arguments");
        return 2; // Return nil, [String] error
    } else if (!lua_isuserdata(L, 1) || !luaL_checkudata(L, 1, "multisocket_tcp")) {
        lua_pushnil(L);
        lua_pushstring(L, "Argument #0 has to be [Multisocket] socket");
        return 2; // Return nil, [String] error
    }

    Multisocket *sock = (Multisocket *) lua_touserdata(L, 1);

    luaL_Buffer str;
    luaL_buffinit(L, &str);

    lua_pushcfunction(L, multi_tcp_get_peeraddr);
    lua_pushvalue(L, 1);
    lua_call(L, 1, 2);
    if (lua_isnil(L, 2)) {
        lua_pushnil(L);
        lua_pushvalue(L, 3);
        return 2;
    }
    if (sock->ipv6) {
        luaL_addstring(&str, "[");
        luaL_addstring(&str, lua_tostring(L, 2));
        luaL_addstring(&str, "]:");
    } else if (sock->ipv4) {
        luaL_addstring(&str, lua_tostring(L, 2));
        luaL_addstring(&str, ":");
    }

    lua_pushcfunction(L, multi_tcp_get_peerport);
    lua_pushvalue(L, 1);
    lua_call(L, 1, 2);
    if (lua_isnil(L, 2)) {
        lua_pushnil(L);
        lua_pushvalue(L, 3);
        return 2;
    }
    luaL_addstring(&str, lua_tostring(L, 0));

    luaL_pushresult(&str);
    return 1; // Return [String] name
}


/**
 * Is the socket a IPv6 socket?
 * @param0 [Multisocket] socket
 * @return1 [Boolean] ipv6
 */
static int multi_is_ipv6(lua_State *L) {
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

    lua_pushboolean(L, sock->ipv6);
    return 1;
}

/**
 * Is the socket a IPv4 socket?
 * @param0 [Multisocket] socket
 * @return1 [Boolean] ipv4
 */
static int multi_is_ipv4(lua_State *L) {
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

    lua_pushboolean(L, sock->ipv4);
    return 1;
}

/**
 * Lua Method
 * Is the socket on the server side?
 * @param0 [Multisocket] socket (TCP)
 * @return1 [Boolean] serverside
 */
static int multi_tcp_is_server_side(lua_State *L) {
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

    lua_pushboolean(L, sock->servers);
    return 1;
}

/**
 * Lua Method
 * Is the socket on the client side?
 * @param0 [Multisocket] socket (TCP)
 * @return1 [Boolean] clientside
 */
static int multi_tcp_is_client_side(lua_State *L) {
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

    lua_pushboolean(L, sock->clients);
    return 1;
}

/**
 * Lua Method
 * Is the socket encrypted?
 * @param0 [Multisocket] socket (TCP)
 * @return1 [Boolean] encrypted
 */
static int multi_tcp_is_encrypted(lua_State *L) {
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

    lua_pushboolean(L, sock->enc);
    return 1;
}


