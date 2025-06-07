#!/usr/bin/env python3
"""
Analyze the writing style of current ability descriptions in Elite Redux.
This will help us maintain consistency when writing extended descriptions.
"""

import re
from collections import defaultdict, Counter
from pathlib import Path

def load_ability_descriptions(proto_path):
    """Load ability descriptions from AbilityList.textproto"""
    abilities = []
    current_ability = {}
    
    with open(proto_path, 'r') as f:
        lines = f.readlines()
    
    for line in lines:
        line = line.strip()
        
        if line.startswith('ability {'):
            if current_ability:
                abilities.append(current_ability)
            current_ability = {}
        elif line.startswith('name:'):
            current_ability['name'] = line.split('"')[1]
        elif line.startswith('description:'):
            # Handle multi-line descriptions
            desc_match = re.search(r'description:\s*"([^"]*)"', line)
            if desc_match:
                current_ability['description'] = desc_match.group(1)
        elif line.startswith('}') and current_ability:
            abilities.append(current_ability)
            current_ability = {}
    
    return abilities

def analyze_style(abilities):
    """Analyze writing patterns in ability descriptions"""
    analysis = {
        'total_abilities': len(abilities),
        'sentence_patterns': defaultdict(int),
        'length_stats': {},
        'common_phrases': Counter(),
        'number_formats': Counter(),
        'voice_analysis': {'active': 0, 'passive': 0},
        'punctuation': Counter(),
        'terminology': Counter()
    }
    
    lengths = []
    
    for ability in abilities:
        if 'description' not in ability:
            continue
            
        desc = ability['description']
        lengths.append(len(desc))
        
        # Sentence structure
        if desc.startswith(('Boosts', 'Increases', 'Decreases', 'Reduces')):
            analysis['sentence_patterns']['action_verb_start'] += 1
        elif desc.startswith(('The ', 'This ')):
            analysis['sentence_patterns']['article_start'] += 1
        elif 'when' in desc.lower():
            analysis['sentence_patterns']['conditional'] += 1
        
        # Number formats
        numbers = re.findall(r'\d+[%x]?', desc)
        for num in numbers:
            analysis['number_formats'][num] += 1
        
        # Common phrases
        phrases = [
            'power', 'damage', 'stat', 'type', 'move', 'turn',
            'boost', 'increase', 'decrease', 'reduce', 'prevent',
            'immune', 'resist', 'super effective', 'STAB'
        ]
        for phrase in phrases:
            if phrase.lower() in desc.lower():
                analysis['common_phrases'][phrase] += 1
        
        # Voice analysis
        passive_indicators = ['is ', 'are ', 'was ', 'were ', 'been ', 'being ']
        if any(indicator in desc for indicator in passive_indicators):
            analysis['voice_analysis']['passive'] += 1
        else:
            analysis['voice_analysis']['active'] += 1
    
    # Length statistics
    if lengths:
        analysis['length_stats'] = {
            'min': min(lengths),
            'max': max(lengths),
            'avg': sum(lengths) / len(lengths),
            'common_range': f"{sorted(lengths)[len(lengths)//4]}-{sorted(lengths)[3*len(lengths)//4]}"
        }
    
    return analysis

def generate_style_guide(analysis):
    """Generate a style guide based on the analysis"""
    guide = []
    guide.append("# Elite Redux Ability Description Style Guide\n")
    guide.append("Based on analysis of existing ability descriptions:\n")
    
    guide.append("\n## Key Statistics")
    guide.append(f"- Total abilities analyzed: {analysis['total_abilities']}")
    guide.append(f"- Average description length: {analysis['length_stats']['avg']:.0f} characters")
    guide.append(f"- Common length range: {analysis['length_stats']['common_range']} characters")
    
    guide.append("\n## Writing Style Patterns")
    guide.append(f"- Active voice preferred ({analysis['voice_analysis']['active']} vs {analysis['voice_analysis']['passive']} passive)")
    
    if analysis['sentence_patterns']['action_verb_start'] > analysis['sentence_patterns']['article_start']:
        guide.append("- Descriptions typically start with action verbs (Boosts, Increases, etc.)")
    else:
        guide.append("- Descriptions often start with articles (The, This)")
    
    guide.append("\n## Common Terminology")
    guide.append("Most frequently used terms:")
    for term, count in analysis['common_phrases'].most_common(10):
        guide.append(f"- {term}: {count} occurrences")
    
    guide.append("\n## Number Formatting")
    guide.append("Common number formats:")
    for format, count in analysis['number_formats'].most_common(5):
        guide.append(f"- {format}: {count} times")
    
    guide.append("\n## Style Guidelines for Extended Descriptions")
    guide.append("1. **Length**: Aim for detailed explanations while maintaining clarity")
    guide.append("2. **Voice**: Use active voice when possible")
    guide.append("3. **Structure**: Start with the main effect, then add conditions/exceptions")
    guide.append("4. **Numbers**: Use consistent formatting (e.g., '50%' not '50 percent')")
    guide.append("5. **Terminology**: Use established game terms consistently")
    
    return '\n'.join(guide)

def main():
    # Path to AbilityList.textproto
    proto_path = Path(__file__).parent.parent.parent / 'proto' / 'AbilityList.textproto'
    
    print("Loading ability descriptions...")
    abilities = load_ability_descriptions(proto_path)
    
    print(f"Analyzing {len(abilities)} abilities...")
    analysis = analyze_style(abilities)
    
    # Generate style guide
    style_guide = generate_style_guide(analysis)
    
    # Save style guide
    output_path = Path(__file__).parent / 'ability_style_guide.md'
    with open(output_path, 'w') as f:
        f.write(style_guide)
    
    print(f"\nStyle guide saved to: {output_path}")
    
    # Also save raw analysis for reference
    import json
    analysis_path = Path(__file__).parent / 'ability_analysis.json'
    
    # Convert Counter objects to dict for JSON serialization
    analysis_json = {
        'total_abilities': analysis['total_abilities'],
        'sentence_patterns': dict(analysis['sentence_patterns']),
        'length_stats': analysis['length_stats'],
        'common_phrases': dict(analysis['common_phrases'].most_common(20)),
        'number_formats': dict(analysis['number_formats'].most_common(10)),
        'voice_analysis': analysis['voice_analysis']
    }
    
    with open(analysis_path, 'w') as f:
        json.dump(analysis_json, f, indent=2)
    
    print(f"Detailed analysis saved to: {analysis_path}")

if __name__ == '__main__':
    main()