g++ -O3 -std=c++17 \
  $(find src/platform/test | grep 'cpp\b') $(find src -maxdepth 2 | grep 'cpp\b') \
  lib/tinyxml2/tinyxml2.cpp \
  -Ilib/cul/inc -Ilib/ecs3/inc -Ilib/tinyxml2 \
  -Wno-unqualified-std-cast-call \
  -o bin/.out-unit-tests
cd bin
./.out-unit-tests
