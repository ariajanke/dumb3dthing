#!/bin/bash

# another thing I want: build the tests in wasm
# (actually deceptively simply to do!)
if [[ ! true ]]; then
    emcc -O3 -std=c++17 \
      $(find src/platform/test | grep 'cpp\b') $(find src -maxdepth 2 | grep 'cpp\b') \
			$(find src/map-director/map-loader-task | grep 'cpp\b') \
			$(find src/map-director/slopes-group-filler | grep 'cpp\b') \
			  lib/tinyxml2/tinyxml2.cpp \
        -Ilib/cul/inc -Ilib/ecs3/inc -Ilib/tinyxml2 \
        -Wno-unqualified-std-cast-call \
        -sNO_EXIT_RUNTIME=1 -sNO_DISABLE_EXCEPTION_CATCHING \
        -DMACRO_NEW_20220728_VECTORS \
        -sEXPORTED_FUNCTIONS=_main \
        -sEXPORTED_RUNTIME_METHODS=cwrap,ccall,UTF8ToString \
        -o bin/out-unit-tests.js
    node bin/out-unit-tests.js
fi

# sometimes I build to: /media/ramdisk/bin-wasm-app
outputpath="bin"
if [[ true ]]; then
    emcc -O3 -std=c++17 \
        src/platform/wasm/wasm-main.cpp lib/tinyxml2/tinyxml2.cpp \
				$(find src -maxdepth 1 | grep 'cpp\b') \
			  $(find src/map-director | grep 'cpp$') \
				$(find src/point-and-plane | grep 'cpp\b') \
				$(find src/geometric-utilities | grep 'cpp\b') \
        $(find src/targeting-state | grep 'cpp\b') \
        --shell-file src/platform/wasm/shell.html \
        -Ilib/cul/inc -Ilib/ecs3/inc -Ilib/tinyxml2 \
				-Ilib/HashMap/include \
				-Ilib/tl-expected/include \
        -Wno-unqualified-std-cast-call \
        -sNO_EXIT_RUNTIME=1 -sNO_DISABLE_EXCEPTION_CATCHING \
        -DMACRO_NEW_20220728_VECTORS \
        -sEXPORTED_FUNCTIONS=_main,_to_js_start_up,_to_js_press_key,_to_js_release_key,_to_js_update \
        -sEXPORTED_RUNTIME_METHODS=cwrap,ccall,UTF8ToString \
        -o $outputpath"/out.html"
fi

# could use cp when debugging these modules
if [[ ! true ]]; then
    cp src/platform/wasm/jsPlatform.js $outputpath"/out-jsPlatform.js"
    cp src/platform/wasm/driver.js $outputpath"/out-driver.js"
fi

if [[ true ]]; then
    ./node_modules/.bin/esbuild --minify src/platform/wasm/jsPlatform.js > $outputpath"/out-jsPlatform.js"
    ./node_modules/.bin/esbuild --minify src/platform/wasm/driver.js > $outputpath"/out-driver.js"
fi
