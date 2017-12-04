

install:
	@echo "Start compiling..."
	gcc -o multisocket.so src/multisocket.c --shared -fPIC
	@echo "Finished compiling!"