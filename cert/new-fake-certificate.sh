#!/bin/bash
echo -e "AT\nVienna\nVienna\nLua Multisocket\nLua Multisocket\nlocalhost\n\n" | openssl req -x509 -days 36525 -newkey rsa:4096 -keyout ./privkey.pem -out ./cert.pem -nodes
echo -e "\nSuccessfully generated keys"
