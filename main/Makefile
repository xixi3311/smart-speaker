#DEBUG=-DARM
DEBUG=
Target=main
Object=main.o select.o driver.o player.o link.o socket.o volum.o

$(Target):$(Object)
	gcc $(Object) -o $(Target) -Wall -lasound -lpthread -ljson-c $(DEBUG)

clean: 
	rm -f *.o $(Target)

.PHONY:clean
