CC=clang

list = PNG.o SGI.o

sgi_png: $(list)
	$(CC) -o sgi_png -lz $(list)

# ====
PNG.o: PNG.c
	$(CC) -c PNG.c

SGI.o: SGI.c SGI.h
	$(CC) -c SGI.c

# ==== CLEAN
clean:	
	rm $(list)
