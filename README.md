# libcpp-gc
Header-Only Very Simple Garbage Collector Library for C++

**PROJECT IS JUST FOR FUN**

## Usage

1. Copy `GarbageCollector.h` to your project
2. Use it :)

## Example

```cpp
#include "Example.h"
#include "GarbageCollector.h"

int main() {
    // You can change interval of garbage collector working
    GarbageCollector::gcInterval = 5s;

    // Start garbage collection
    return GarbageCollector::runtime([]() {
        auto example = gc(new Example());

        // Some useful code...

        return 0;
    });
}

```
