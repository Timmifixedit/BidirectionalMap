sudo apt update
if [[ $CXX == g++* ]]; then
  sudo apt install -y "$CXX"
elif [[ $CXX == clang* ]]; then
  sudo apt install -y wget
  wget https://apt.llvm.org/llvm.sh
  chmod +x llvm.sh
  CLANG_VERSION=$(echo "$CXX" | tr -d -c 0-9)
  sudo ./llvm.sh "$CLANG_VERSION"
fi