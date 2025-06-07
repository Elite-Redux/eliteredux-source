import re
import os

target_file = os.path.join('src', 'data', 'trainers.h')

# Lines to add for single battles
hell_single_lines = [
    '        .partySizeHell = 0, // Placeholder, update when party is added\n',
    '        .partyHell = {.ItemCustomMoves = NULL}, // Placeholder, update when party is added\n'
]

# Lines to add for double battles (in addition to single)
hell_double_lines = [
    '        .partySizeHellDouble = 0, // Placeholder, update when party is added\n',
    '        .partyHellDouble = {.ItemCustomMoves = NULL}, // Placeholder, update when party is added\n'
]

# Regex to find the start of a trainer definition
trainer_start_regex = re.compile(r'^\s*\[(TRAINER_\w+)\] =')
# Regex to find the end of a trainer definition
trainer_end_regex = re.compile(r'^\s*\},')
# Regex to find the double battle flag
double_battle_regex = re.compile(r'^\s*\.doubleBattle\s*=\s*TRUE')
# Regex to find potential insertion points (adjust if needed based on actual file structure)
# We'll insert after the last party definition (usually partyInsane or partyInsaneDouble)
# Updated regex to better handle different last party field names
last_party_field_regex = re.compile(r'^\s*\.(party|partyInsane|partyDouble|partyInsaneDouble)\s*=\s*\{')

in_trainer_block = False
is_double_battle = False
hell_lines_added = False
lines_to_insert = []
buffer_lines = [] # Buffer lines within a trainer block to insert before the closing brace
output_lines = [] # Store all lines to write at the end

print(f"Processing {target_file} to add Hell placeholder fields...")

try:
    with open(target_file, 'r', encoding='utf-8') as infile:
        lines = infile.readlines()

    i = 0
    while i < len(lines):
        line = lines[i]

        if trainer_start_regex.match(line):
            # Start of a new block, process buffered lines from previous block
            for buf_line in buffer_lines:
                output_lines.append(buf_line)
            buffer_lines = []

            # Reset flags and start buffering the new block
            in_trainer_block = True
            is_double_battle = False
            hell_lines_added = False # Reset this flag for each new trainer
            lines_to_insert = []
            buffer_lines.append(line) # Add the start line to buffer

        elif in_trainer_block:
            buffer_lines.append(line) # Add current line to buffer

            if double_battle_regex.search(line):
                is_double_battle = True

            # If we reach the end of the block
            if trainer_end_regex.match(line):
                # Prepare lines to insert based on double battle status ONLY if not already added
                if not hell_lines_added:
                    lines_to_insert.extend(hell_single_lines)
                    if is_double_battle:
                        lines_to_insert.extend(hell_double_lines)
                    hell_lines_added = True # Mark as prepared/added for this block

                # Insert the new lines *before* the closing brace line
                output_lines.extend(buffer_lines[:-1]) # Add all buffered lines except the last one
                output_lines.extend(lines_to_insert)   # Add the new Hell lines
                output_lines.append(buffer_lines[-1])  # Add the closing brace line

                # Reset flags and buffer for the next block
                in_trainer_block = False
                is_double_battle = False
                # hell_lines_added = False # Already reset at start of block
                lines_to_insert = []
                buffer_lines = []
        else:
             # Line outside any trainer block, add directly to output
             output_lines.append(line)

        i += 1

    # Add any remaining buffered lines if the file ends unexpectedly within a block
    for buf_line in buffer_lines:
        output_lines.append(buf_line)

    # Write the modified content back to the original file
    with open(target_file, 'w', encoding='utf-8') as outfile:
        outfile.writelines(output_lines)

    print(f"Successfully added placeholder fields to {target_file}.")

except FileNotFoundError:
    print(f"Error: Input file not found at {target_file}")
except Exception as e:
    print(f"An error occurred: {e}")