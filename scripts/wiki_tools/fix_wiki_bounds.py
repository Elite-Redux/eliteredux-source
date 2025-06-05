#!/usr/bin/env python3
"""
Fix bounds checking issues in ui_information_menu.c
This prevents memory access errors when navigating the wiki
"""

import re
from pathlib import Path

def fix_wiki_bounds():
    file_path = Path("../../src/ui_information_menu.c")
    
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # Find the PrintToWindow function
    print("Fixing PrintToWindow function bounds checking...")
    
    # Pattern to find the problematic line
    pattern = r'(static void PrintToWindow\(void\) \{[^}]+?)(u8 numPages = gHelpArticles\[currentTab\]\.entries\[entryIdx\]\.numPages;)'
    
    # Find the function
    match = re.search(pattern, content, re.DOTALL)
    if not match:
        print("ERROR: Could not find the PrintToWindow function pattern!")
        return False
    
    # Extract parts
    before_numPages = match.group(1)
    numPages_line = match.group(2)
    
    # Find where we need to insert the fix
    # We need to replace the numPages line and add bounds checking
    replacement = """u8 numEntries = min(MAX_ENTRIES_ON_SCREEN, getCurrentTabEntries());
    u8 numPages = 0;
    
    // Validate bounds before accessing arrays
    if (currentTab >= ARRAY_COUNT(gHelpArticles))
        return;
    
    if (entryIdx < gHelpArticles[currentTab].numEntries)
        numPages = gHelpArticles[currentTab].entries[entryIdx].numPages;"""
    
    # Find the original numEntries line
    numEntries_pattern = r'(u8 numEntries = min\(MAX_ENTRIES_ON_SCREEN, getCurrentTabEntries\(\)\);)'
    
    # Replace in the before_numPages section
    new_before = re.sub(numEntries_pattern, replacement, before_numPages)
    
    # Now we need to fix the for loop bounds checking
    print("Adding bounds check in entry list display...")
    
    # Find the for loop that displays entries
    for_loop_pattern = r'(for \(i = 0; i < numEntries; i\+\+\) \{\s*)(y = SPACE_BETWEEN_INFORMATION_START)'
    for_loop_replacement = r'\1// Validate bounds before accessing\n            if (offset + i >= gHelpArticles[currentTab].numEntries)\n                break;\n                \n            \2'
    
    new_content = content[:match.start()] + new_before + content[match.end():]
    new_content = re.sub(for_loop_pattern, for_loop_replacement, new_content)
    
    # Add bounds check in the entry display section
    print("Adding bounds check for entry display...")
    
    entry_display_pattern = r'(\} else \{\s*)(x = \(2 \* 8\);)'
    entry_display_replacement = r'\1// Double-check bounds for safety\n        if (entryIdx >= gHelpArticles[currentTab].numEntries)\n            return;\n            \n        if (pageIdx >= numPages && numPages > 0)\n            pageIdx = numPages - 1;\n            \n        \2'
    
    new_content = re.sub(entry_display_pattern, entry_display_replacement, new_content)
    
    # Fix the Task_MenuMain function
    print("Adding bounds check in Task_MenuMain...")
    
    task_main_pattern = r'(if \(sMenuDataPtr->isEntryOpen\) \{\s*u8 numPages = gHelpArticles\[currentTab\]\.entries\[entryIdx\]\.numPages;)'
    task_main_replacement = r'\1\n        \n        if (numPages == 0)\n            numPages = 1;'
    
    new_content = re.sub(task_main_pattern, task_main_replacement, new_content)
    
    # Write the fixed content back
    with open(file_path, 'w', encoding='utf-8') as f:
        f.write(new_content)
    
    print("Successfully fixed bounds checking in ui_information_menu.c")
    return True

if __name__ == "__main__":
    if fix_wiki_bounds():
        print("\nDone! The wiki system should now have proper bounds checking.")
        print("Remember to rebuild the ROM to test the fix.")
    else:
        print("\nFailed to apply fixes. Please check the error messages above.")