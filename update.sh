rm -rf build
git stash
git pull
mkdir build
cd build
cmake ..
read -p "server/client: " resp
make ./$resp
