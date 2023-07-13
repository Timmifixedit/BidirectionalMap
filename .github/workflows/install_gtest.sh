git clone https://github.com/google/googletest.git && cd googletest
mkdir build && cd build
cmake -DBUILD_SHARED_LIBS=ON -DINSTALL_GTEST=ON -DCMAKE_INSTALL_PREFIX:PATH=/usr ..
make -j$(nproc)
sudo make install
sudo ldconfig
