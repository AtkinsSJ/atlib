# AtLib

The base game-dev code I've been developing on and off since 2015. It is not good but it is mine.

## Building

The build uses CMake. I don't know a lot about CMake, so these instructions may be lacking.

Set up the CMake build using something like this:

```shell
cmake -DCMAKE_BUILD_TYPE=Debug -G Ninja -B ./Build/Debug
```

Then compile like this:
```shell
cd ./Build/Debug
ninja
```
