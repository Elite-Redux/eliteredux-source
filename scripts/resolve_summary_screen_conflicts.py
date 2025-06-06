#!/usr/bin/env python3
"""
Script to help resolve merge conflicts in pokemon_summary_screen.c
This script identifies key patterns and provides guidance for the merge.
"""

import re
import sys

def analyze_conflicts(file_path):
    """Analyze merge conflicts in the file and provide resolution guidance."""
    
    with open(file_path, 'r') as f:
        content = f.read()
    
    # Find all conflicts
    conflict_pattern = r'<<<<<<< HEAD(.*?)=======(.*?)>>>>>>> origin/SummaryScreenChanges'
    conflicts = list(re.finditer(conflict_pattern, content, re.DOTALL))
    
    print(f"Found {len(conflicts)} conflicts in {file_path}")
    print("=" * 80)
    
    resolution_guide = []
    
    for i, match in enumerate(conflicts):
        start_pos = match.start()
        line_num = content[:start_pos].count('\n') + 1
        
        head_content = match.group(1).strip()
        branch_content = match.group(2).strip()
        
        print(f"\nConflict {i+1} at line {line_num}:")
        print("-" * 40)
        
        # Analyze the conflict and provide guidance
        if "expandedAbilityMode" in branch_content:
            print("✓ KEEP BRANCH VERSION - Contains expandedAbilityMode functionality")
            resolution_guide.append({
                'line': line_num,
                'action': 'keep_branch',
                'reason': 'Contains expandedAbilityMode functionality'
            })
        elif "LoadCurrentPageTilemap" in branch_content:
            print("✓ KEEP BRANCH VERSION - Contains LoadCurrentPageTilemap function")
            resolution_guide.append({
                'line': line_num,
                'action': 'keep_branch',
                'reason': 'Contains LoadCurrentPageTilemap function'
            })
        elif "ChangeIntoExpandedAbilityMode" in branch_content:
            print("✓ KEEP BRANCH VERSION - Contains ChangeIntoExpandedAbilityMode call")
            resolution_guide.append({
                'line': line_num,
                'action': 'keep_branch',
                'reason': 'Contains ChangeIntoExpandedAbilityMode functionality'
            })
        elif "sMonSummaryScreen->ModifyMode" in branch_content and "ModifyMode" in head_content:
            print("⚠ MERGE REQUIRED - Update global ModifyMode to struct member")
            print("  Replace: ModifyMode")
            print("  With: sMonSummaryScreen->ModifyMode")
            resolution_guide.append({
                'line': line_num,
                'action': 'merge',
                'reason': 'Update global variable to struct member'
            })
        elif re.search(r'static void \(\*const sTextPrinterFunctions', head_content):
            print("✓ KEEP BRANCH VERSION - Better formatting")
            resolution_guide.append({
                'line': line_num,
                'action': 'keep_branch',
                'reason': 'Better code formatting'
            })
        else:
            # Check for specific patterns
            if "gCurrentModifyIndex" in head_content and "sCurrentModifyIndex" in branch_content:
                print("⚠ MERGE REQUIRED - Update global to struct member")
                resolution_guide.append({
                    'line': line_num,
                    'action': 'merge',
                    'reason': 'Update global variable to struct member'
                })
            else:
                print("⚠ MANUAL REVIEW REQUIRED")
                resolution_guide.append({
                    'line': line_num,
                    'action': 'manual',
                    'reason': 'Requires careful analysis'
                })
        
        # Show a snippet of the conflict
        head_lines = head_content.split('\n')[:3]
        branch_lines = branch_content.split('\n')[:3]
        
        print("\nHEAD version (first 3 lines):")
        for line in head_lines:
            print(f"  {line}")
        
        print("\nBranch version (first 3 lines):")
        for line in branch_lines:
            print(f"  {line}")
    
    print("\n" + "=" * 80)
    print("SUMMARY OF RESOLUTIONS:")
    print("=" * 80)
    
    keep_branch = sum(1 for g in resolution_guide if g['action'] == 'keep_branch')
    merge_needed = sum(1 for g in resolution_guide if g['action'] == 'merge')
    manual_needed = sum(1 for g in resolution_guide if g['action'] == 'manual')
    
    print(f"Keep branch version: {keep_branch} conflicts")
    print(f"Merge required: {merge_needed} conflicts")
    print(f"Manual review required: {manual_needed} conflicts")
    
    print("\nKEY CHANGES TO PRESERVE:")
    print("1. expandedAbilityMode functionality")
    print("2. LoadCurrentPageTilemap() function")
    print("3. ChangeIntoExpandedAbilityMode() function")
    print("4. BufferMonPokemonExpandedAbilityAndInnates() function")
    print("5. gSummaryScreenPageAbilityExpandedTilemap references")
    print("6. Update global variables to struct members:")
    print("   - ModifyMode → sMonSummaryScreen->ModifyMode")
    print("   - gCurrentModifyIndex → sMonSummaryScreen->sCurrentModifyIndex")
    
    return resolution_guide

def main():
    file_path = "src/pokemon_summary_screen.c"
    
    print("Summary Screen Merge Conflict Analyzer")
    print("=" * 80)
    
    try:
        analyze_conflicts(file_path)
    except FileNotFoundError:
        print(f"Error: {file_path} not found!")
        print("Make sure you run this script from the project root directory")
        sys.exit(1)
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()