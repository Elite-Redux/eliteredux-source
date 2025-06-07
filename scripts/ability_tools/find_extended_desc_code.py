#!/usr/bin/env python3
"""
Search for existing extended description implementation by Riolu.
This helps us understand what's already been done.
"""

import os
import re
from pathlib import Path

def search_files(root_dir, patterns, extensions):
    """Search for patterns in files with given extensions"""
    matches = []
    
    for root, dirs, files in os.walk(root_dir):
        # Skip build directories and tools
        if any(skip in root for skip in ['build/', '.git/', '__pycache__']):
            continue
            
        for file in files:
            if any(file.endswith(ext) for ext in extensions):
                file_path = os.path.join(root, file)
                
                try:
                    with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                        content = f.read()
                        
                    for pattern in patterns:
                        if re.search(pattern, content, re.IGNORECASE):
                            # Find line numbers
                            lines = content.split('\n')
                            for i, line in enumerate(lines):
                                if re.search(pattern, line, re.IGNORECASE):
                                    matches.append({
                                        'file': file_path,
                                        'line': i + 1,
                                        'content': line.strip(),
                                        'pattern': pattern
                                    })
                except:
                    pass
    
    return matches

def main():
    # Define search patterns for extended descriptions
    patterns = [
        r'extended.*desc',
        r'long.*desc',
        r'detailed.*desc',
        r'ability.*detail',
        r'extendedDescription',
        r'extended_description',
        r'longDescription',
        r'long_description',
        # UI-related patterns
        r'ability.*popup',
        r'ability.*window',
        r'desc.*window',
        # Codegen-related
        r'ability.*text\.hh',
        r'generated.*abilit'
    ]
    
    # File extensions to search
    extensions = ['.c', '.cc', '.h', '.hh', '.inc', '.proto', '.kt', '.java']
    
    # Search directories
    search_dirs = [
        'src/',
        'include/',
        'proto/',
        'tools/codegen/'
    ]
    
    print("Searching for extended description implementation...")
    print("This may take a moment...\n")
    
    all_matches = []
    for dir in search_dirs:
        if os.path.exists(dir):
            print(f"Searching {dir}...")
            matches = search_files(dir, patterns, extensions)
            all_matches.extend(matches)
    
    # Group by file
    files_with_matches = {}
    for match in all_matches:
        file = match['file']
        if file not in files_with_matches:
            files_with_matches[file] = []
        files_with_matches[file].append(match)
    
    # Generate report
    print(f"\nFound {len(all_matches)} matches in {len(files_with_matches)} files:\n")
    
    # Sort by relevance (files with most matches first)
    sorted_files = sorted(files_with_matches.items(), 
                         key=lambda x: len(x[1]), 
                         reverse=True)
    
    report = []
    report.append("# Extended Description Implementation Search Results\n")
    
    for file, matches in sorted_files[:20]:  # Top 20 files
        report.append(f"\n## {file} ({len(matches)} matches)")
        
        # Show unique match contents
        shown = set()
        for match in matches[:5]:  # First 5 unique matches
            content = match['content']
            if content not in shown:
                report.append(f"- Line {match['line']}: `{content[:100]}...`")
                shown.add(content)
    
    # Save report
    report_path = Path(__file__).parent / 'extended_desc_search_results.md'
    with open(report_path, 'w') as f:
        f.write('\n'.join(report))
    
    print(f"Detailed results saved to: {report_path}")
    
    # Also check for specific ability struct fields
    print("\n\nChecking ability struct definition...")
    struct_patterns = [
        r'struct.*Ability',
        r'typedef.*ability',
        r'extendedDesc',
        r'extended_desc'
    ]
    
    struct_files = search_files('include/', struct_patterns, ['.h', '.hh'])
    struct_files.extend(search_files('src/', struct_patterns, ['.c', '.cc', '.h']))
    
    if struct_files:
        print("\nPotential ability struct definitions found:")
        for match in struct_files[:10]:
            print(f"- {match['file']}:{match['line']}")

if __name__ == '__main__':
    main()