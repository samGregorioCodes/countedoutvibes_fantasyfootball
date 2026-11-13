# Counted Out Football build
#
#   make          browser build (WebAssembly) into dist/
#   make native   console build with the system C++ compiler
#   make serve    serve dist/ at http://localhost:8000
#   make clean    remove build output
#
# The browser build needs Emscripten on PATH (source emsdk_env.sh first).

SRC      := src/main.cpp
DIST     := dist
WEB      := web/index.html web/style.css web/app.js

EMXX     ?= em++
CXX      ?= g++
CXXSTD   := -std=c++17
WARNINGS := -Wall -Wextra -Wpedantic

# Emscripten settings
#   -lembind                 C++ classes exposed to JS via EMSCRIPTEN_BINDINGS
#   MODULARIZE + EXPORT_ES6  game.js exports a factory: import createGameModule from './game.js'
#   INVOKE_RUN=0             don't run main(); the console game stays native-only
#   ENVIRONMENT=web          browser-only glue code (smaller output)
#   FILESYSTEM=0             no virtual file system needed
EMFLAGS := \
	-O2 \
	-lembind \
	-sMODULARIZE=1 \
	-sEXPORT_ES6=1 \
	-sEXPORT_NAME=createGameModule \
	-sINVOKE_RUN=0 \
	-sENVIRONMENT=web \
	-sFILESYSTEM=0 \
	-sALLOW_MEMORY_GROWTH=0

.PHONY: all web native serve clean

all: web

web: $(DIST)/game.js $(addprefix $(DIST)/,$(notdir $(WEB)))

$(DIST)/game.js: $(SRC) | $(DIST)
	$(EMXX) $(CXXSTD) $(WARNINGS) $(EMFLAGS) $(SRC) -o $@

$(DIST)/%: web/% | $(DIST)
	cp $< $@

$(DIST):
	mkdir -p $(DIST)

native: counted_out_football

counted_out_football: $(SRC)
	$(CXX) $(CXXSTD) $(WARNINGS) -O2 $(SRC) -o $@

serve: web
	python3 -m http.server 8000 --directory $(DIST)

clean:
	rm -rf $(DIST) counted_out_football
