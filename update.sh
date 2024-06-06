rm -rf build
git pull
mkdir build            
cd build
cmake .. -G Ninja
ninja
