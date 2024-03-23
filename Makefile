CC = gcc
CFLAGS = -O2
ALL_CFLAGS = -pedantic -Wall -std=gnu17 -fdiagnostics-color=always $(CFLAGS)

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

pvs:
	@pvs-studio-analyzer trace -o $(OUT_DIR)/strace_out -- make
	@pvs-studio-analyzer analyze -f $(OUT_DIR)/strace_out -o $(OUT_DIR)/project-analysis.log
	@plog-converter -a GA:1,2 -t tasklist -o report.tasks $(OUT_DIR)/project-analysis.log

pre-build:
	@mkdir -p $(OUT_DIR)

clean:
	@rm -rf $(OUT_DIR)/* report.tasks

.PHONY: all clean
