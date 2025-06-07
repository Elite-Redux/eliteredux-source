#!/bin/bash
# Smart build script for Elite Redux (works on macOS/Linux/WSL)
# Automatically builds tools if needed, then builds ROM

# Check if essential tools exist
TOOLS_NEEDED=false
if [ ! -f "tools/scaninc/scaninc" ] || [ ! -f "tools/preproc/preproc" ] || [ ! -f "tools/gbagfx/gbagfx" ]; then
    TOOLS_NEEDED=true
fi

if [ "$TOOLS_NEEDED" = true ]; then
    echo "Some tools are missing. Building tools first..."
    make tools
    
    # Handle libpng issues on macOS if gbagfx/rsfont failed
    if [ ! -f "tools/gbagfx/gbagfx" ]; then
        echo "Building gbagfx manually for macOS..."
        cd tools/gbagfx && cc -Wall -Wextra -Werror -Wno-sign-compare -std=c11 -O2 -DPNG_SKIP_SETJMP_CHECK -I/opt/homebrew/include main.c convert_png.c gfx.c jasc_pal.c lz.c rl.c util.c font.c huff.c -o gbagfx -L/opt/homebrew/lib -lpng -lz
        cd ../..
    fi
    
    if [ ! -f "tools/rsfont/rsfont" ]; then
        echo "Building rsfont manually for macOS..."
        cd tools/rsfont && cc -Wall -Wextra -Werror -std=c11 -O2 -DPNG_SKIP_SETJMP_CHECK -I/opt/homebrew/include main.c convert_png.c util.c font.c -o rsfont -L/opt/homebrew/lib -lpng -lz
        cd ../..
    fi
fi

# Determine number of cores (default to 4 for safety with poryscript)
CORES=${1:-4}
echo "Building ROM with $CORES parallel jobs..."
make -j$CORES