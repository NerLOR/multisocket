

install:
	@echo "Start compiling..."
	gcc -o multisocket.so src/multisocket.c --shared -fPIC -lssl -lcrypt
	@echo "Finished compiling!"