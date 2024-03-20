all: md5 esclavo vista

md5: md5.c
	gcc -Wall $< -o $@

esclavo: esclavo.c
	gcc -Wall $< -o $@

vista: vista.c
	gcc -Wall $< -o $@

clean:
	rm -f md5 esclavo vista

.PHONY: all clean