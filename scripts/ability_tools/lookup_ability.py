#!/usr/bin/env python3
"""
Quick lookup tool for ability mechanics.
Helps verify ability implementations before writing extended descriptions.
"""

import sys
import subprocess
import re
from pathlib import Path

def search_ability(ability_name):
    """Search for ability implementation across key files"""
    
    # Remove ABILITY_ prefix if provided
    ability_clean = ability_name.replace("ABILITY_", "")
    
    print(f"\n=== Searching for {ability_name} ===\n")
    
    # Key files to search
    search_locations = [
        ("abilities.cc", f"grep -n -i -A 10 -B 2 'drizzle\\|{ability_name}' src/abilities.cc || true"),
        ("battle scripts", f"grep -n -i '{ability_name}' src/battle_*.c || true"),
        ("ability enum", f"grep -n '{ability_name}' include/generated/constants/abilities.h || true"),
        ("proto definition", f"grep -n -A 5 'id: {ability_name}' proto/AbilityList.textproto || true"),
        ("ability handlers", f"grep -n -i '{ability_clean}' src/abilities.cc || true"),
    ]
    
    results = {}
    
    for location, command in search_locations:
        print(f"Checking {location}...")
        try:
            result = subprocess.run(command, shell=True, capture_output=True, text=True)
            if result.stdout:
                results[location] = result.stdout
                print(f"✓ Found in {location}")
            else:
                print(f"✗ Not found in {location}")
        except Exception as e:
            print(f"✗ Error searching {location}: {e}")
    
    return results

def display_results(results):
    """Display search results in readable format"""
    
    print("\n" + "="*60 + "\n")
    
    for location, content in results.items():
        print(f"\n### {location.upper()} ###")
        print(content[:1000])  # Limit output length
        if len(content) > 1000:
            print("... (truncated)")
    
    # Look for key information
    print("\n" + "="*60 + "\n")
    print("KEY INFORMATION TO VERIFY:")
    print("- Effect percentages/numbers")
    print("- Trigger conditions (on entry, on contact, etc.)")
    print("- Interactions with other abilities")
    print("- Special cases or exceptions")
    print("- Weather/terrain/field interactions")

def main():
    if len(sys.argv) < 2:
        print("Usage: python lookup_ability.py ABILITY_NAME")
        print("Example: python lookup_ability.py ABILITY_DRIZZLE")
        sys.exit(1)
    
    ability_name = sys.argv[1].upper()
    if not ability_name.startswith("ABILITY_"):
        ability_name = "ABILITY_" + ability_name
    
    results = search_ability(ability_name)
    display_results(results)
    
    # Save results to file for reference
    output_file = Path(__file__).parent / f"lookup_{ability_name}.txt"
    with open(output_file, 'w') as f:
        f.write(f"Lookup results for {ability_name}\n")
        f.write("="*60 + "\n\n")
        for location, content in results.items():
            f.write(f"\n### {location.upper()} ###\n")
            f.write(content)
            f.write("\n" + "-"*40 + "\n")
    
    print(f"\nResults saved to: {output_file}")

if __name__ == "__main__":
    main()