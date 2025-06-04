import re
import os

parties_file = os.path.join('src', 'data', 'trainer_parties.h')

# Regex to find the start of an Insane or InsaneDouble party definition
# Captures: 1=Full Name (sParty_XInsane), 2=Base Name (sParty_X), 3=Suffix (Insane or InsaneDouble)
insane_party_start_regex = re.compile(r'^\s*static const struct TrainerMonItemCustomMoves\s+((sParty_\w+?)(Insane|InsaneDouble))\[\]\s*=')
# Regex to find the end of a party definition block
party_end_regex = re.compile(r'^\s*\};')

output_lines = []
current_block_buffer = []
current_insane_party_name = None
suffix_to_replace = None
base_name_part = None

print(f"Processing {parties_file} to insert Hell placeholders...")

try:
    with open(parties_file, 'r', encoding='utf-8') as infile:
        lines = infile.readlines() # Read all lines at once for easier checking

    i = 0
    while i < len(lines):
        line = lines[i]

        if current_insane_party_name is None:
            match_start = insane_party_start_regex.match(line)
            if match_start:
                current_insane_party_name = match_start.group(1)
                base_name_part = match_start.group(2)
                suffix_to_replace = match_start.group(3)
                current_block_buffer = [line] # Start buffering
            else:
                output_lines.append(line) # Line outside any block we care about
            i += 1
        else:
            # We are inside an Insane block, keep buffering
            current_block_buffer.append(line)
            if party_end_regex.match(line):
                # Found the end of the Insane block

                # 1. Add the original Insane block to output
                output_lines.extend(current_block_buffer)

                # 2. Check if the corresponding Hell block already exists immediately after
                hell_suffix = "Hell" if suffix_to_replace == "Insane" else "HellDouble"
                expected_hell_name = f"{base_name_part}{hell_suffix}"
                hell_start_pattern = re.compile(r'^\s*static const struct TrainerMonItemCustomMoves\s+' + re.escape(expected_hell_name) + r'\[\]\s*=')

                already_exists = False
                if i + 1 < len(lines): # Check if there is a next line
                    # Also check if the next line is just whitespace before the definition
                    if hell_start_pattern.match(lines[i+1].lstrip()):
                        already_exists = True
                        print(f"Skipping insertion for {expected_hell_name}, already exists.")

                # 3. If it doesn't exist, create and add the Hell block
                if not already_exists:
                    # Ensure a newline precedes the new block
                    if not output_lines[-1].endswith('\n'):
                         output_lines[-1] += '\n' # Add newline to the last line added (the '};')
                    output_lines.append('\n') # Add an extra blank line for separation

                    hell_block_lines = list(current_block_buffer) # Copy the buffer
                    # Modify the first line to rename the array
                    first_line_modified = hell_block_lines[0].replace(current_insane_party_name, expected_hell_name, 1)
                    hell_block_lines[0] = first_line_modified
                    output_lines.extend(hell_block_lines) # Add the new Hell block
                    print(f"Inserted placeholder for {expected_hell_name}")

                # 4. Reset for next search
                current_insane_party_name = None
                current_block_buffer = []
                suffix_to_replace = None
                base_name_part = None
            # else: still inside the block, continue buffering
            i += 1

    # If file ended while buffering (shouldn't happen with correct structure)
    if current_block_buffer:
        output_lines.extend(current_block_buffer)

    # Write the modified content back to the original file
    with open(parties_file, 'w', encoding='utf-8') as outfile:
        outfile.writelines(output_lines)

    print(f"Finished processing {parties_file}.")

except FileNotFoundError:
    print(f"Error: Input file not found at {parties_file}")
except Exception as e:
    print(f"An error occurred: {e}")