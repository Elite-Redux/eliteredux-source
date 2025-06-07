#!/bin/bash
# Clean build script for Elite Redux (works on macOS/Linux/WSL)
# This cleans ROM build files without removing compiled tools

echo "Cleaning ROM build files (keeping tools intact)..."
make mostlyclean

echo "Build cleaned. Tools are preserved."
echo "Run 'make -j4' to rebuild the ROM"