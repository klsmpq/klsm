all: release

debug:
	mkdir -p build
	cd build && CXX=/opt/intel/composer_xe_2013_sp1/bin/icpc cmake -DCMAKE_BUILD_TYPE="Debug" .. && make -j6

release:
	mkdir -p build
	cd build && CXX=/opt/intel/composer_xe_2013_sp1/bin/icpc cmake -DCMAKE_BUILD_TYPE="Release" .. && make -j6

check: debug
	cd build && ctest --output-on-failure

doc:
	mkdir -p build
	cd build; CXX=/opt/intel/composer_xe_2013_sp1/bin/icpc cmake -DCMAKE_BUILD_TYPE="Debug" ..; make doc

clean:
	rm -rf build

.PHONY: build check doc clean
