setup:
    #!/usr/bin/env bash
    ln -s build/compile_commands.json .

build:
    #!/usr/bin/env bash
    cmake -B build -S .
    cmake --build build

run: build
    #!/usr/bin/env bash
    ./build/kaleidoscope

run_debug: build
    #!/usr/bin/env bash
    gdb -batch -ex "run" -ex "bt" ./build/kaleidoscope

clean:
    #!/usr/bin/env bash
    rm -rf ./build
