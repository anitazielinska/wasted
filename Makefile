HEADERS=src/game.hh src/engine.hh src/util.hh
FILES=src/main.cc src/game.cc src/engine.cc src/util.cc lib/lodepng/*.cpp
FLAGS=-g -Wall --std=c++17
LIBS=-Ilib -lGL -lGLEW -lglfw -lGLU -lassimp
OUT=game
CC=g++

.PHONY: clean

build: $(FILES) $(HEADERS)
	@mkdir -p out
	$(CC) $(FLAGS) $(FILES) $(LIBS) -I. -o out/$(OUT)

run: build
	out/$(OUT)

clean:
	rm -rf out
