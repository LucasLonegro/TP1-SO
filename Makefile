CC = gcc
CFLAGS = -O2
ALL_CFLAGS = -pedantic -Wall -std=gnu17 $(CFLAGS)

OUT_DIR = bin

TEST_FILES = Makefile

all: pre-build md5 esclavo vista

md5: aplicacion.c
	$(CC) $(ALL_CFLAGS) $< -o $(OUT_DIR)/$@

esclavo: esclavo.c
	$(CC) $(ALL_CFLAGS) $< -o $(OUT_DIR)/$@

vista: vista.c
	$(CC) $(ALL_CFLAGS) $< -o $(OUT_DIR)/$@

debug: CFLAGS = -g
debug: all

valgrind: debug
	@valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes $(OUT_DIR)/md5 $(TEST_FILES)

pre-build:
	@mkdir -p $(OUT_DIR)

clean:
	@rm -rf $(OUT_DIR)/*

.PHONY: all clean
