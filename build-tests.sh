g++ -O3 -std=c++17 \
  $(find src/platform/test | grep 'cpp\b') $(find src -maxdepth 2 | grep 'cpp\b') \
	$(find src/map-director/map-loader-task | grep 'cpp\b') \
	$(find src/map-director/slopes-group-filler | grep 'cpp\b') \
	$(find src/map-director/twist-loop-filler | grep 'cpp\b') \
	$(find tests | grep 'cpp\b') \
  lib/tinyxml2/tinyxml2.cpp \
  -Ilib/cul/inc -Ilib/ecs3/inc -Ilib/tinyxml2 \
  -Ilib/HashMap/include \
  -Wno-unqualified-std-cast-call \
  -o bin/.out-unit-tests
cd bin
./.out-unit-tests && echo "All tests Pass!"
cd ..

