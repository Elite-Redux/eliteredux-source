Build the Elite Redux ROM intelligently. 

Steps:
1. Check if there are any uncommitted changes that might affect the build
2. If there are build errors, run `make clean` first (ignore harmless tool subdirectory errors)
3. Run `make -j24` (or adjust core count based on system)
4. If the build fails:
   - Analyze the error messages
   - Provide specific guidance on how to fix common issues
   - Suggest relevant files to check based on the error
5. If successful, report the build time and confirm pokeemerald.gba was created

Common issues to handle:
- Missing agbcc compiler → Suggest installation steps
- Compilation errors → Identify the problematic file and line
- Linker errors → Check for symbol conflicts
- Tool build failures → These can often be ignored

Do not show the full build output as it's too verbose. Only show relevant errors or success confirmation.