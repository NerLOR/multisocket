#!/usr/bin/lua5.3

package.cpath = package.cpath..";/home/lorenz/Documents/Projects/lua/?/?.so"


local multisocket = require("multisocket")


local sock = multisocket.tcp4()

local host = "www.lua.org"

print(sock:bind("*",0))
print(sock:connect(host,443))

print(sock:encrypt())

print(sock:send("GET / HTTP/1.1\r\nHost: "..host.."\r\n\r\n"))
print(sock:receive("\r\n\r\n"))
