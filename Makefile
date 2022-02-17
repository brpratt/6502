.PHONY: example clean test

CC := gcc
CFLAGS := -Wall -Wextra -g

example: bin/compy bin/program.bin
	@./bin/compy ./bin/program.bin

clean:
	rm -rf obj/
	rm -rf bin/

obj/bus.o: src/bus.c src/bus.h
	@mkdir -p obj
	$(CC) $(CFLAGS) -c -o obj/bus.o $<

obj/cpu.o: src/cpu.c src/cpu.h src/bus.h
	@mkdir -p obj
	$(CC) $(CFLAGS) -c -o obj/cpu.o $<

obj/main.o: example/main.c src/bus.h src/cpu.h
	@mkdir -p obj
	$(CC) $(CFLAGS) -Isrc -c -o obj/main.o $<

bin/compy: obj/bus.o obj/cpu.o obj/main.o
	@mkdir -p bin
	$(CC) $(CFLAGS) $^ -o bin/compy

bin/program.bin: example/program.asm
	@mkdir -p bin
	dasm $< -f3 -obin/program.bin

test: bin/bus_test bin/cpu_test bin/6502_functional_test
	@./bin/bus_test
	@./bin/cpu_test
	@./bin/6502_functional_test

bin/bus_test: test/test.c test/test.h test/bus_test.c obj/bus.o
	@mkdir -p bin
	$(CC) -o bin/bus_test $(CFLAGS) -Isrc $^

bin/cpu_test: test/test.c test/test.h test/cpu_test.c obj/bus.o obj/cpu.o
	@mkdir -p bin
	$(CC) -o bin/cpu_test $(CLFAGS) -Isrc $^

bin/6502_functional_test: test/test.c test/test.h test/6502_functional_test.c obj/bus.o obj/cpu.o
	@mkdir -p bin
	$(CC) -o bin/6502_functional_test $(CLFAGS) -Isrc $^
