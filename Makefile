default:run

all:native web

sources = main.c editor.c menu.c easing.c

web:
	source emsdk_portable/emsdk_set_env.sh;\
	emcc -O3 $(sources) -o ld.html -s USE_SDL=2 -s USE_SDL_TTF=2 \
	-s ALLOW_MEMORY_GROWTH=1 --preload-file data/

native:
	clang $(sources) -g -o ld -lSDL2 -lSDL2_ttf -lm

run:native
	./ld
