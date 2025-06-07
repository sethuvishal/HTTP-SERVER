objects = build/request.o build/response.o build/server.o build/config.o build/utils.o

INCLUDES = -Iinclude
CFLAGS = -Wall -Wextra -fPIC $(INCLUDES) `pkg-config --cflags glib-2.0`
LDLIBS = `pkg-config --libs glib-2.0`

all: libhttpserver.so

libhttpserver.so: $(objects)
	gcc -shared -o libhttpserver.so $(objects) $(LDLIBS)

build/%.o: src/%.c
	mkdir -p build
	gcc $(CFLAGS) -c $< -o $@

clean:
	rm -f build/*.o libhttpserver.so