#!/bin/bash

# another thing I want: build the tests in wasm
# (actually deceptively simply to do!)
emcc -std=c++17 \
  $(find src/platform/test | grep 'cpp\b') $(find src -maxdepth 2 | grep 'cpp\b') \
  -Ilib/cul/inc -Ilib/ecs3/inc \
 -Wno-unqualified-std-cast-call \
 -sNO_EXIT_RUNTIME=1 -sNO_DISABLE_EXCEPTION_CATCHING \
 -DMACRO_NEW_20220728_VECTORS \
 -sEXPORTED_FUNCTIONS=_main \
 -sEXPORTED_RUNTIME_METHODS=cwrap,ccall,UTF8ToString \
 -o bin/out-unit-tests.js

node bin/out-unit-tests.js

emcc -std=c++17 \
  src/platform/wasm/wasm-main.cpp $(find src -maxdepth 2 | grep 'cpp\b') \
  --shell-file src/platform/wasm/shell.html \
 -Ilib/cul/inc -Ilib/ecs3/inc \
 -Wno-unqualified-std-cast-call \
 -sNO_EXIT_RUNTIME=1 -sNO_DISABLE_EXCEPTION_CATCHING \
 -DMACRO_NEW_20220728_VECTORS \
 -sEXPORTED_FUNCTIONS=_main,_to_js_start_up,_to_js_press_key,_to_js_release_key,_to_js_update \
 -sEXPORTED_RUNTIME_METHODS=cwrap,ccall,UTF8ToString \
 -o bin/out.html

# could use cp when debugging these modules

esbuild --minify src/platform/wasm/jsPlatform.js > bin/out-jsPlatform.js
esbuild --minify src/platform/wasm/driver.js > bin/out-driver.js
