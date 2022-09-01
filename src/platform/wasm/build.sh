#!/bin/bash
#emcc -std=c++17 helloworld.cpp --shell-file shell.html \
# -sEXPORTED_FUNCTIONS=_to_js_sum_array,_main,_to_js_reserve_temporary_buffer,_to_js_get_secret_array \
# -sEXPORTED_RUNTIME_METHODS=cwrap,ccall -o out.html

emcc -std=c++17 wasm-main.cpp  \
  --shell-file shell.html \
 -sEXPORTED_FUNCTIONS=_to_js_sum_array,_main,_to_js_reserve_temporary_buffer,_to_js_get_secret_array \
 -sEXPORTED_RUNTIME_METHODS=cwrap,ccall -o out.html
