rm -rf build
git stash
git pull
mkdir build
cd build
cmake ..
make client
