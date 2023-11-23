CPPFLAGS :=$(CPPFLAGS) -g -W -Wall -Wno-unused-parameter -Werror
#INCLUDES = -I/usr/local/libevent/include 
CC=gcc
LD_FLAG=  -lrt -ldl -lpthread
#LD_FLAG= -levent -lrt -ldl -lpthread
SOURCE_CPP := $(wildcard *.c)
OBJECTS := $(SOURCE_CPP:%.c=%.o)

		
FAST_LIB=libfast.a

%.o: %.c
	$(CC) ${INCLUDES} -o $@ -c $< $(CPPFLAGS)

all:$(FAST_LIB)

$(FAST_LIB):$(OBJECTS)
	ar rcs $(FAST_LIB) $(OBJECTS)
clean:
	rm -rf $(FAST_LIB) *.o
install:
	mkdir  -p /usr/local/include/fast/
	cp  -f *.h /usr/local/include/fast/.
	mv ${FAST_LIB} /usr/local/lib/.

