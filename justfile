build:
    #!/usr/bin/env bash
    cmake -B build -S .
    cmake --build build

run:
    #!/usr/bin/env bash
    ./build/kaleidoscope

clean:
    #!/usr/bin/env bash
    rm -rf ./build
