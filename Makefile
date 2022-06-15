.PHONY: emu
emu: ./bin/nes.o ./bin/gui.o ./bin/interpreter.o ./bin/memory.o
	gcc -o ./bin/emu $^ -L ./lib/SDL/SDL/lib -lmingw32 -lSDL2main -lSDL2

./bin/%.o: ./src/%.c
	gcc -c $< -o $@ -Wall -Wextra -Wno-unused-parameter

.PHONY: clean
clean:
	rm -f ./bin/*.o ./bin/emu