CC=clang

list = PNG.o SGI.o main.c

sgi_png: $(list)
	$(CC) -o sgi_png -lz $(list)

# ====
PNG.o: PNG.c PNG.h
	$(CC) -c PNG.c

SGI.o: SGI.c SGI.h
	$(CC) -c SGI.c

main.o: main.c
	$(CC) -c main.c

# ==== CLEAN
clean:	
	rm $(list)
