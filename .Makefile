CC = gcc
CFLAGS = -O2
ALL_CFLAGS = -pedantic -Wall -std=c99 $(CFLAGS)

OUT_DIR = bin

all: pre-build md5 esclavo vista

md5: aplicacion.c
	$(CC) $(ALL_CFLAGS) $< -o $(OUT_DIR)/$@

esclavo: esclavo.c
	$(CC) $(ALL_CFLAGS) $< -o $(OUT_DIR)/$@

vista: vista.c
	$(CC) $(ALL_CFLAGS) $< -o $(OUT_DIR)/$@

pre-build:
	@mkdir -p $(OUT_DIR)

clean:
	@rm -rf $(OUT_DIR)/*

.PHONY: all clean
