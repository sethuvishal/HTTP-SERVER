objects = build/request.o build/response.o build/main.o build/config.o build/utils.o
all: $(objects)
	cc $^ -o build/all

$(objects): build/%.o: src/%.c
	echo $^
	cc -c $^ -o $@

clean:
	rm -f build/*