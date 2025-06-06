objects = build/request.o build/response.o build/main.o build/config.o build/utils.o
CFLAGS = `pkg-config --cflags glib-2.0`
LDLIBS = `pkg-config --libs glib-2.0`

all: $(objects)
	cc $^ -o build/all $(LDLIBS)

$(objects): build/%.o: src/%.c
	echo $^
	cc -c $^ -o $@ $(CFLAGS)

clean:
	rm -f build/*