rm -rf build
git stash
git pull

cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release .
cmake --build build