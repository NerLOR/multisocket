
# Lua Multisocket
A library written in C to provide simple IPv4/TCP and IPv6/TCP sockets in Lua. 
The library uses the [OpenSSL](https://www.openssl.org/) library to encrypt connections.
UDP is currently not supported.
With the HTTP module, it is possible to wrap a TCP/SSL connection to create a HTTP object.

## Compatibility
The library is designed to be used in **Lua 5.3**, 
but it may also work with older and newer versions.

#### Features:
* IPv4 TCP connections
* IPv6 TCP connections
* SSL/TLS Encryption on TCP connections
* HTTP wrapper for TCP connections

#### Work in progress:
* Get information from X509 Certificates

#### Planned:
* IPv4/IPv6 UDP support

## Documentation
The full documentation and reference can be found on the [Wiki page](https://github.com/NerLOR/multisocket/wiki).

## Examples
Some examples can also be found on the [Examples page](https://github.com/NerLOR/multisocket/wiki/Examples).

## Multithreading
If you want to set up a server, capable of running several threads simultaneously, 
I recommend working with [Effil](https://github.com/effil/effil),
a module for multithreading support in Lua.
To use all these multithreading modules, you have to work with [pointers](https://github.com/NerLOR/multisocket/wiki#Pointers), but
**POINTERS ARE DANGEROUS** in a high-level languages like Lua.
**USE THEM ONLY IF YOU KNOW WHAT YOU ARE DOING!** If you are not careful enough,
you could create some serious security weak spots.

And don't forget: ***POINTERS ARE EVIL!***
