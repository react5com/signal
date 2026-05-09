
```
mkdir build
cd build
cmake ..
cmake --build . --config Debug
cmake --install . --config Debug --prefix ../dist

ctest
```