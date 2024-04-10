rm -rf build
git stash
git pull
cmake -B build -G Ninja .
cd build
read -p "server/client: " resp
ninja $resp
