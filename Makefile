default:run

all:native web

sources = main.c editor.c menu.c easing.c game.c sprite.c assets.c

web:
	source emsdk_portable/emsdk_set_env.sh;\
	emcc -O3 $(sources) -o ld.html -s USE_SDL=2 -s USE_SDL_TTF=2 \
	-s ALLOW_MEMORY_GROWTH=1 --preload-file data/
	cp ld.html ld.data ld.js ld.html.mem site
	@rclone copy site dropbox:Public/sokoban

web-devel:
	source emsdk_portable/emsdk_set_env.sh;\
	emcc $(sources) -o ld.html -s USE_SDL=2 -s USE_SDL_TTF=2 \
	-s ALLOW_MEMORY_GROWTH=1 -s ASSERTIONS=2 --preload-file data/
	cp ld.html ld.data ld.js ld.html.mem site

native:
	clang $(sources) -g -O0 -o ld -lSDL2 -lSDL2_ttf -lm

run:native
	optirun ./ld;echo a
