all: build

build:
	mkdir -p build
	cd build && cmake -DCMAKE_BUILD_TYPE="Debug" .. && make -j6

check: build
	cd build && ctest --output-on-failure

doc:
	mkdir -p build
	cd build; cmake -DCMAKE_BUILD_TYPE="Debug" ..; make doc

clean:
	rm -rf build

.PHONY: build check doc clean
