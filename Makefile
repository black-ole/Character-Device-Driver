obj-m += mymodule.o

all:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) modules
	$(CC) readtest.c -o read
	$(CC) writetest.c -o write

clean:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) clean
	rm read
	rm write
