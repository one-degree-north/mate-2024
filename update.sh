rm -rf build
git stash
git pull

read -p "Install server or client (s/c)? " resp
case $resp in
    s )
        mkdir build
        cd build
        cmake ..
        make server ;;
    c )
        cmake -B build -G Ninja .
        cd build
	ninja client ;;
    * ) echo "Invalid response." ;;
esac
