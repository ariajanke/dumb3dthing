#!/bin/bash

emcc -std=c++17 \
  src/platform/wasm/wasm-main.cpp $(find src -maxdepth 2 | grep 'cpp\b') \
  --shell-file src/platform/wasm/shell.html \
 -sEXPORTED_FUNCTIONS=_to_js_start_up,_to_js_press_key,_to_js_release_key,_to_js_update \
 -sEXPORTED_RUNTIME_METHODS=cwrap,ccall \
 -o src/platform/wasm/out.html
