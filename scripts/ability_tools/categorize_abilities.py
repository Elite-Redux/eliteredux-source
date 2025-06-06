#!/usr/bin/env python3
"""
Categorize abilities by their effects/types for batch processing.
This helps organize the 700-800 abilities into manageable groups.
"""

import re
from collections import defaultdict
from pathlib import Path

def load_abilities_from_proto(proto_path):
    """Load abilities from AbilityList.textproto"""
    abilities = []
    current_ability = {}
    
    with open(proto_path, 'r') as f:
        content = f.read()
    
    # Simple parser for protobuf text format
    ability_blocks = re.findall(r'ability\s*{([^}]+)}', content, re.DOTALL)
    
    for block in ability_blocks:
        ability = {}
        
        # Extract fields
        name_match = re.search(r'name:\s*"([^"]+)"', block)
        desc_match = re.search(r'description:\s*"([^"]+)"', block)
        id_match = re.search(r'id:\s*(\w+)', block)
        
        if name_match:
            ability['name'] = name_match.group(1)
        if desc_match:
            ability['description'] = desc_match.group(1)
        if id_match:
            ability['id'] = id_match.group(1)
            
        if ability:
            abilities.append(ability)
    
    return abilities

def categorize_abilities(abilities):
    """Categorize abilities based on their descriptions and effects"""
    categories = defaultdict(list)
    
    # Define categorization rules
    categorization_rules = {
        'Weather/Terrain': [
            'weather', 'rain', 'sun', 'sand', 'hail', 'snow',
            'terrain', 'grassy', 'electric', 'psychic', 'misty'
        ],
        'Stat Modification': [
            'stat', 'attack', 'defense', 'speed', 'accuracy', 'evasion',
            'boost', 'increase', 'decrease', 'reduce', 'raise', 'lower'
        ],
        'Type-Based': [
            'type', 'normal', 'fire', 'water', 'grass', 'electric',
            'ice', 'fighting', 'poison', 'ground', 'flying', 'psychic',
            'bug', 'rock', 'ghost', 'dragon', 'dark', 'steel', 'fairy'
        ],
        'Damage Calculation': [
            'damage', 'power', 'critical', 'super effective', 'stab',
            'recoil', 'contact', 'physical', 'special'
        ],
        'Status/Condition': [
            'status', 'burn', 'poison', 'paralyze', 'sleep', 'freeze',
            'confuse', 'flinch', 'infatuat', 'trap'
        ],
        'Item/Berry': [
            'item', 'berry', 'hold', 'consume', 'heal'
        ],
        'HP/Healing': [
            'hp', 'heal', 'recover', 'restore', 'drain', 'absorb'
        ],
        'Priority/Speed': [
            'priority', 'first', 'speed', 'quick', 'slow'
        ],
        'Switch/Pivot': [
            'switch', 'swap', 'pivot', 'enter', 'send out'
        ],
        'Protection/Immunity': [
            'immune', 'protect', 'prevent', 'block', 'resist'
        ],
        'Multi-hit/Strike': [
            'multi', 'twice', 'multiple', 'consecutive'
        ],
        'Form/Transformation': [
            'form', 'transform', 'change', 'mega', 'primal'
        ],
        'Field Effects': [
            'reflect', 'light screen', 'safeguard', 'tailwind',
            'trick room', 'gravity', 'wonder room', 'magic room'
        ],
        'Custom/Signature': [
            # We'll identify these by looking for unique patterns
        ]
    }
    
    # Categorize each ability
    for ability in abilities:
        if 'description' not in ability:
            categories['No Description'].append(ability)
            continue
        
        desc_lower = ability['description'].lower()
        categorized = False
        
        # Check against each category
        for category, keywords in categorization_rules.items():
            if any(keyword in desc_lower for keyword in keywords):
                categories[category].append(ability)
                categorized = True
                # Don't break - abilities can belong to multiple categories
        
        # If not categorized, check if it's a custom ability
        if not categorized:
            if ability.get('id', '').startswith('ABILITY_CUSTOM_') or \
               'CUSTOM' in ability.get('id', ''):
                categories['Custom/Signature'].append(ability)
            else:
                categories['Uncategorized'].append(ability)
    
    return categories

def generate_category_report(categories):
    """Generate a report of categorized abilities"""
    report = []
    report.append("# Ability Categorization Report\n")
    report.append(f"Total categories: {len(categories)}\n")
    
    # Sort categories by size
    sorted_cats = sorted(categories.items(), key=lambda x: len(x[1]), reverse=True)
    
    for category, abilities in sorted_cats:
        report.append(f"\n## {category} ({len(abilities)} abilities)")
        
        # Show first 10 abilities as examples
        for i, ability in enumerate(abilities[:10]):
            name = ability.get('name', 'Unknown')
            desc = ability.get('description', 'No description')[:60] + '...'
            report.append(f"{i+1}. **{name}**: {desc}")
        
        if len(abilities) > 10:
            report.append(f"... and {len(abilities) - 10} more")
    
    return '\n'.join(report)

def generate_batch_plan(categories):
    """Generate a batch processing plan"""
    plan = []
    plan.append("# Batch Processing Plan for Extended Descriptions\n")
    
    # Prioritize categories
    priority_order = [
        'Custom/Signature',  # ER custom abilities first
        'Weather/Terrain',   # Important meta abilities
        'Stat Modification', # Common competitive abilities
        'Type-Based',       # Type-changing abilities
        'Damage Calculation',
        'Status/Condition',
        'Protection/Immunity',
        'HP/Healing',
        'Item/Berry',
        'Priority/Speed',
        'Switch/Pivot',
        'Multi-hit/Strike',
        'Form/Transformation',
        'Field Effects',
        'Uncategorized',
        'No Description'
    ]
    
    batch_size = 50
    batch_num = 1
    
    for category in priority_order:
        if category not in categories:
            continue
            
        abilities = categories[category]
        num_batches = (len(abilities) + batch_size - 1) // batch_size
        
        plan.append(f"\n## {category}")
        plan.append(f"Total abilities: {len(abilities)}")
        plan.append(f"Number of batches: {num_batches}")
        
        for i in range(num_batches):
            start = i * batch_size
            end = min(start + batch_size, len(abilities))
            plan.append(f"- Batch {batch_num}: abilities {start+1}-{end}")
            batch_num += 1
    
    plan.append(f"\n**Total batches: {batch_num - 1}**")
    
    return '\n'.join(plan)

def main():
    # Path to AbilityList.textproto
    proto_path = Path(__file__).parent.parent.parent / 'proto' / 'AbilityList.textproto'
    
    print("Loading abilities...")
    abilities = load_abilities_from_proto(proto_path)
    print(f"Loaded {len(abilities)} abilities")
    
    print("\nCategorizing abilities...")
    categories = categorize_abilities(abilities)
    
    # Generate reports
    category_report = generate_category_report(categories)
    batch_plan = generate_batch_plan(categories)
    
    # Save reports
    report_path = Path(__file__).parent / 'ability_categories.md'
    with open(report_path, 'w') as f:
        f.write(category_report)
    
    plan_path = Path(__file__).parent / 'batch_processing_plan.md'
    with open(plan_path, 'w') as f:
        f.write(batch_plan)
    
    print(f"\nReports saved:")
    print(f"- Category report: {report_path}")
    print(f"- Batch plan: {plan_path}")
    
    # Save categorized abilities as JSON for later use
    import json
    
    # Convert to JSON-serializable format
    categories_json = {}
    for cat, abs in categories.items():
        categories_json[cat] = [
            {
                'id': a.get('id', ''),
                'name': a.get('name', ''),
                'description': a.get('description', '')
            }
            for a in abs
        ]
    
    json_path = Path(__file__).parent / 'categorized_abilities.json'
    with open(json_path, 'w') as f:
        json.dump(categories_json, f, indent=2)
    
    print(f"- Categorized data: {json_path}")

if __name__ == '__main__':
    main()