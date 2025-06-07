import re
import os

trainers_file = os.path.join('src', 'data', 'trainers.h')

# Regex patterns
trainer_start_regex = re.compile(r'^\s*\[(TRAINER_\w+)\] =')
trainer_end_regex = re.compile(r'^\s*\},')
# Find if InsaneDouble definitions EXIST
has_insane_double_size_regex = re.compile(r'^\s*\.partySizeInsaneDouble\s*=')
has_insane_double_party_regex = re.compile(r'^\s*\.partyInsaneDouble\s*=')
# Find the HellDouble lines we might need to remove
hell_double_size_line_regex = re.compile(r'^\s*\.partySizeHellDouble\s*=.*$')
hell_double_party_line_regex = re.compile(r'^\s*\.partyHellDouble\s*=.*$')


output_lines = []
current_block_lines = []
current_trainer_id = None
has_insane_double = False

print(f"Processing {trainers_file} to remove unnecessary HellDouble fields...")

try:
    with open(trainers_file, 'r', encoding='utf-8') as infile:
        lines = infile.readlines()

    i = 0
    while i < len(lines):
        line = lines[i]

        match_start = trainer_start_regex.match(line)
        if match_start:
            # Process the previous block before starting a new one
            if current_block_lines:
                if not has_insane_double:
                    # Remove HellDouble lines if InsaneDouble was missing
                    cleaned_block = [l for l in current_block_lines
                                     if not hell_double_size_line_regex.match(l) and
                                        not hell_double_party_line_regex.match(l)]
                    if len(cleaned_block) < len(current_block_lines):
                         print(f"Removed unnecessary HellDouble fields for {current_trainer_id}")
                    output_lines.extend(cleaned_block)
                else:
                    # Keep the block as is
                    output_lines.extend(current_block_lines)

            # Start new block
            current_trainer_id = match_start.group(1)
            current_block_lines = [line]
            has_insane_double = False
        elif current_trainer_id:
            # Add line to current block buffer
            current_block_lines.append(line)
            # Check if this block has InsaneDouble definitions
            if has_insane_double_size_regex.search(line) or has_insane_double_party_regex.search(line):
                has_insane_double = True
            # Check if we reached the end of the block
            if trainer_end_regex.match(line):
                 # Process the completed block
                if not has_insane_double:
                    cleaned_block = [l for l in current_block_lines
                                     if not hell_double_size_line_regex.match(l) and
                                        not hell_double_party_line_regex.match(l)]
                    if len(cleaned_block) < len(current_block_lines):
                         print(f"Removed unnecessary HellDouble fields for {current_trainer_id}")
                    output_lines.extend(cleaned_block)
                else:
                    output_lines.extend(current_block_lines)
                # Reset for next potential block
                current_block_lines = []
                current_trainer_id = None
                has_insane_double = False
        else:
            # Line outside any trainer block
            output_lines.append(line)

        i += 1

    # Add any remaining lines from the last block if file didn't end cleanly
    if current_block_lines:
         if not has_insane_double:
            cleaned_block = [l for l in current_block_lines
                             if not hell_double_size_line_regex.match(l) and
                                not hell_double_party_line_regex.match(l)]
            if len(cleaned_block) < len(current_block_lines):
                 print(f"Removed unnecessary HellDouble fields for {current_trainer_id}")
            output_lines.extend(cleaned_block)
         else:
            output_lines.extend(current_block_lines)


    # Write the modified content back to the original file
    with open(trainers_file, 'w', encoding='utf-8') as outfile:
        outfile.writelines(output_lines)

    print(f"Finished cleaning up {trainers_file}.")

except FileNotFoundError:
    print(f"Error: Input file not found at {trainers_file}")
except Exception as e:
    print(f"An error occurred: {e}")