install:
	@echo "Start compiling..."
	gcc -o multisocket.so src/multisocket.c --shared -fPIC -lssl -lcrypt -std=c11 -I/usr/include
	@echo "Finished compiling!"