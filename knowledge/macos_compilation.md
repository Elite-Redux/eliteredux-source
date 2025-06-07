# macOS Compilation Guide for Elite Redux

## Prerequisites

1. **Xcode Command Line Tools**
   ```bash
   xcode-select --install
   ```

2. **Homebrew**
   - Install from https://brew.sh/

3. **Required packages via Homebrew**
   ```bash
   brew install libpng
   ```

4. **devkitARM**
   - Download devkitpro-pacman-installer.pkg from GitHub
   - Install and run:
   ```bash
   sudo dkp-pacman -Sy
   sudo dkp-pacman -S gba-dev
   sudo dkp-pacman -S devkitarm-rules
   ```

5. **Environment variables** (add to ~/.bashrc or ~/.zshrc)
   ```bash
   export DEVKITPRO=/opt/devkitpro
   export DEVKITARM=$DEVKITPRO/devkitARM
   ```

6. **Java/JDK and Kotlin**
   ```bash
   # Install SDKMAN
   curl -s "https://get.sdkman.io" | bash
   source "$HOME/.sdkman/bin/sdkman-init.sh"
   
   # Install Kotlin
   sdk install kotlin
   ```

## Building Elite Redux

### First-time setup

1. **Build agbcc compiler**
   ```bash
   git clone https://github.com/pret/agbcc
   cd agbcc
   ./build.sh
   ./install.sh ../eliteredux-source
   cd ..
   ```

2. **Build tools**
   ```bash
   make tools
   ```

   If you encounter libpng errors with gbagfx/rsfont:
   ```bash
   cd tools/gbagfx && cc -Wall -Wextra -Werror -Wno-sign-compare -std=c11 -O2 -DPNG_SKIP_SETJMP_CHECK -I/opt/homebrew/include main.c convert_png.c gfx.c jasc_pal.c lz.c rl.c util.c font.c huff.c -o gbagfx -L/opt/homebrew/lib -lpng -lz
   cd ../rsfont && cc -Wall -Wextra -Werror -std=c11 -O2 -DPNG_SKIP_SETJMP_CHECK -I/opt/homebrew/include main.c convert_png.c util.c font.c -o rsfont -L/opt/homebrew/lib -lpng -lz
   ```

### Building the ROM

#### Using Smart Scripts (Recommended)
```bash
# Clean without removing tools
./scripts/clean_build.sh

# Build with automatic tool checking
./scripts/smart_build.sh     # Uses 4 cores by default
./scripts/smart_build.sh 10  # Specify core count
```

#### Manual Build Commands
```bash
# Clean previous build (preserves tools)
make mostlyclean

# Build with appropriate parallelism
# For M4 Pro (10 cores), but use fewer if memory issues occur
make -j4  # Recommended for poryscript memory usage
# OR
make -j10  # Full cores if memory permits
```

#### Full Clean Build (rarely needed)
```bash
make clean  # WARNING: This removes tools too
make tools  # Rebuild all tools
make -j4    # Build ROM
```

## Common Issues and Solutions

### 1. Permission denied for protoc
```bash
chmod +x tools/codegen/protoc
```

### 2. Missing tools (scaninc, preproc, etc.)
```bash
make tools
```

### 3. Poryscript memory issues ("Killed: 9")
- Use fewer parallel jobs: `make -j4` instead of `make -j10`
- This happens because poryscript uses significant memory when processing map scripts
- macOS kills processes that consume too much memory

### 4. Wrong poryscript architecture
- Ensure you're using the macOS ARM64 version for Apple Silicon
- Download from the official poryscript releases page
- Replace in `tools/poryscript/`

### 5. macOS Gatekeeper blocking tools
- When downloading tools like poryscript, macOS adds quarantine flags
- You'll see popups saying the app "may cause harm"
- Fix permanently: `xattr -d com.apple.quarantine tools/poryscript/poryscript`
- This removes the quarantine flag and stops the security warnings

### 6. libpng warnings
- "libpng warning: bKGD: invalid index" warnings are harmless
- These occur due to palette issues in some PNG files but don't affect the build

### 7. Zero-byte object files
- If you see linker errors about missing functions, check for 0-byte .o files
- This can happen with corrupted builds
- Solution: `make mostlyclean` then rebuild

## Build System Notes

- Both `make` and `make modern` are equivalent - they both use modern GCC
- The build creates files in `build/modern/` directory
- Clean builds recommended when switching between major changes
- Tool errors during `make clean` about subdirectories can be ignored

## CPU Core Recommendations

- Mac Mini M4 Pro: 10 cores (6 performance + 4 efficiency)
- Use `sysctl -n hw.ncpu` to check your core count
- Start with `-j4` for stability, increase if build succeeds

## Key Learnings Summary

1. **NEVER use `make clean`** - it removes tools and causes build failures
   - Always use `make mostlyclean` instead
   - Or use the provided scripts: `./scripts/clean_build.sh`

2. **macOS Security (Gatekeeper)**
   - Downloaded tools trigger "may cause harm" popups
   - Fix: `xattr -d com.apple.quarantine tools/poryscript/poryscript`

3. **Memory Management**
   - poryscript uses lots of memory with parallel builds
   - Use `-j4` instead of `-j10` to avoid "Killed: 9" errors

4. **libpng on Apple Silicon**
   - Homebrew installs to `/opt/homebrew` not `/usr/local`
   - Some tools need manual compilation with correct paths

5. **Successful Build Process**
   ```bash
   # One-time setup
   xattr -d com.apple.quarantine tools/poryscript/poryscript
   
   # For all builds
   make mostlyclean
   make -j4
   ```