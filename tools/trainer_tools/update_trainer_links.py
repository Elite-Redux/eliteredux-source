import re
import os

trainers_file = os.path.join('src', 'data', 'trainers.h')

# Regex patterns
trainer_start_regex = re.compile(r'^\s*\[(TRAINER_\w+)\] =')
# Regex to find any existing party definition to extract base name
party_line_regex = re.compile(r'^\s*\.(party|partyInsane|partyDouble|partyInsaneDouble)\s*=\s*\{\.ItemCustomMoves\s*=\s*(\w+)\}')
# Regex to find the specific Insane/InsaneDouble lines
insane_size_line_regex = re.compile(r'^(\s*\.partySizeInsane\s*=\s*)(.*)(,\s*(//.*)?)$')
insane_party_line_regex = re.compile(r'^(\s*\.partyInsane\s*=\s*)(\{.*\})(\s*,\s*(//.*)?)$')
insane_double_size_line_regex = re.compile(r'^(\s*\.partySizeInsaneDouble\s*=\s*)(.*)(,\s*(//.*)?)$')
insane_double_party_line_regex = re.compile(r'^(\s*\.partyInsaneDouble\s*=\s*)(\{.*\})(\s*,\s*(//.*)?)$')
# Regex to find the placeholder Hell definitions
hell_party_null_regex = re.compile(r'^(\s*\.partyHell\s*=\s*\{\.ItemCustomMoves\s*=\s*)NULL(\s*\},.*)$')
hell_size_zero_regex = re.compile(r'^(\s*\.partySizeHell\s*=\s*)0(,\s*(//.*)?)$')
hell_double_party_null_regex = re.compile(r'^(\s*\.partyHellDouble\s*=\s*\{\.ItemCustomMoves\s*=\s*)NULL(\s*\},.*)$')
hell_double_size_zero_regex = re.compile(r'^(\s*\.partySizeHellDouble\s*=\s*)0(,\s*(//.*)?)$')
trainer_end_regex = re.compile(r'^\s*\},')

output_lines = []
current_trainer_id = None
base_party_name = None
# Store the exact lines found for Insane/InsaneDouble
insane_size_line_content = None
insane_party_line_content = None
insane_double_size_line_content = None
insane_double_party_line_content = None

print(f"Processing {trainers_file} to update Hell party links mirroring Insane definitions...")

try:
    with open(trainers_file, 'r', encoding='utf-8') as infile:
        lines = infile.readlines()

    i = 0
    while i < len(lines):
        line = lines[i]
        output_lines.append(line) # Add line initially, may be replaced later

        match_start = trainer_start_regex.match(line)
        if match_start:
            # Start of a new trainer block, reset context
            current_trainer_id = match_start.group(1)
            base_party_name = None
            insane_size_line_content = None
            insane_party_line_content = None
            insane_double_size_line_content = None
            insane_double_party_line_content = None
            # Scan ahead within this block to find the base party name and Insane definitions
            j = i + 1
            while j < len(lines) and not trainer_end_regex.match(lines[j]):
                # Find base name (only once)
                if base_party_name is None:
                    match_party = party_line_regex.search(lines[j])
                    if match_party:
                        temp_name = match_party.group(2)
                        if temp_name.endswith('InsaneDouble'):
                            base_party_name = temp_name[:-len('InsaneDouble')]
                        elif temp_name.endswith('Double'):
                             base_party_name = temp_name[:-len('Double')]
                        elif temp_name.endswith('Insane'):
                             base_party_name = temp_name[:-len('Insane')]
                        elif temp_name != 'NULL':
                             base_party_name = temp_name
                        # if base_party_name: print(f"Found base name for {current_trainer_id}: {base_party_name}") # Debug

                # Find Insane/InsaneDouble definitions
                if insane_size_line_content is None:
                    match_is = insane_size_line_regex.match(lines[j])
                    if match_is: insane_size_line_content = lines[j]
                if insane_party_line_content is None:
                     match_ip = insane_party_line_regex.match(lines[j])
                     if match_ip: insane_party_line_content = lines[j]
                if insane_double_size_line_content is None:
                     match_ids = insane_double_size_line_regex.match(lines[j])
                     if match_ids: insane_double_size_line_content = lines[j]
                if insane_double_party_line_content is None:
                     match_idp = insane_double_party_line_regex.match(lines[j])
                     if match_idp: insane_double_party_line_content = lines[j]
                j += 1
            if base_party_name is None and current_trainer_id != "TRAINER_NONE":
                 print(f"Warning: Could not determine base party name for {current_trainer_id}")

        elif current_trainer_id: # Inside a trainer block
            # Check and replace Hell party pointer using Insane party line
            match_hell_null = hell_party_null_regex.match(line)
            if match_hell_null and insane_party_line_content:
                new_line = insane_party_line_content.replace(".partyInsane", ".partyHell", 1).replace("Insane", "Hell", 1) # Replace in name and pointer
                output_lines[-1] = new_line

            # Check and replace Hell party size using Insane size line
            match_hell_size = hell_size_zero_regex.match(line)
            if match_hell_size and insane_size_line_content:
                new_line = insane_size_line_content.replace(".partySizeInsane", ".partySizeHell", 1).replace("Insane", "Hell", 1) # Replace in name and pointer/macro arg
                output_lines[-1] = new_line

            # Check and replace HellDouble party pointer using InsaneDouble party line
            match_double_null = hell_double_party_null_regex.match(line)
            if match_double_null and insane_double_party_line_content:
                new_line = insane_double_party_line_content.replace(".partyInsaneDouble", ".partyHellDouble", 1).replace("InsaneDouble", "HellDouble", 1)
                output_lines[-1] = new_line

            # Check and replace HellDouble party size using InsaneDouble size line
            match_double_size = hell_double_size_zero_regex.match(line)
            if match_double_size and insane_double_size_line_content: # Check if InsaneDouble size line was actually found
                new_line = insane_double_size_line_content.replace(".partySizeInsaneDouble", ".partySizeHellDouble", 1).replace("InsaneDouble", "HellDouble", 1)
                output_lines[-1] = new_line
            elif match_double_size and not insane_double_size_line_content:
                 # If placeholder exists but no InsaneDouble size to mirror, keep placeholder but log warning
                 print(f"Warning: Found placeholder for partySizeHellDouble for {current_trainer_id} but no corresponding partySizeInsaneDouble to mirror.")


            # Reset trainer context if end of block is reached
            if trainer_end_regex.match(line):
                current_trainer_id = None
                insane_size_line_content = None
                insane_party_line_content = None
                insane_double_size_line_content = None
                insane_double_party_line_content = None

        i += 1

    # Write the modified content back to the original file
    with open(trainers_file, 'w', encoding='utf-8') as outfile:
        outfile.writelines(output_lines)

    print(f"Finished updating links in {trainers_file} (mirroring Insane definitions).")

except FileNotFoundError:
    print(f"Error: Input file not found at {trainers_file}")
except Exception as e:
    print(f"An error occurred: {e}")