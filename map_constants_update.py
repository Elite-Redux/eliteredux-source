# This script will apply the same changes to your project as this PR: https://github.com/pret/pokeemerald/pull/1755/files
# You can run this before or after merging with upstream pret, so that your project's custom code and data are updated appropriately.
# To run this script:
#    - Place this file in the root of your project. For example, pokeemearld/map_constants_update.py 
#    - Run "python map_constants_update.py"

import re
import glob

# 1. In map.json files, convert "dest_warp_id" to string value instead of integer. 127 becomes "WARP_ID_DYNAMIC". 126 becomes "WARP_ID_SECRET_BASE."
# 2. In map.json files, convert "dest_map": "MAP_NONE" to "dest_map": "MAP_DYNAMIC".
# 3. In all script files (.inc and .pory), convert MAP_NONE to MAP_DYNAMIC.
# 5. In all C files, convert MAP_NUM(NONE) to MAP_NUM(DYNAMIC).
# 7. In all C files, convert MAP_GROUP(NONE) to MAP_GROUP(DYNAMIC).

for filepath in glob.iglob('data/maps/**/map.json', recursive=True):
    with open(filepath) as file:
        text = file.read()

    text = re.sub(r'\"dest_warp_id\": +127',    '"dest_warp_id": "WARP_ID_DYNAMIC"', text)
    text = re.sub(r'\"dest_warp_id\": +126',    '"dest_warp_id": "WARP_ID_SECRET_BASE"', text)
    text = re.sub(r'\"dest_warp_id\": +(\d+)', r'"dest_warp_id": "\1"', text)
    text = re.sub(r'\"dest_map\": +"MAP_NONE"', '"dest_map": "MAP_DYNAMIC"', text)

    with open(filepath, "w") as file:
        file.write(text)


for filepath in glob.iglob('data/**/*.inc', recursive=True):
    with open(filepath, encoding="utf-8") as file:
        text = file.read()

    text = re.sub(r'\bMAP_NONE\b',      'MAP_DYNAMIC', text)

    with open(filepath, "w", encoding="utf-8") as file:
        file.write(text)


for filepath in glob.iglob('data/**/*.pory', recursive=True):
    with open(filepath, encoding="utf-8") as file:
        text = file.read()

    text = re.sub(r'\bMAP_NONE\b',      'MAP_DYNAMIC', text)

    with open(filepath, "w", encoding="utf-8") as file:
        file.write(text)


for filepath in glob.iglob('src/**/*.c', recursive=True):
    with open(filepath, encoding="utf-8") as file:
        text = file.read()

    text = re.sub(r'MAP_NUM\(NONE\)',        'MAP_NUM(DYNAMIC)', text)
    text = re.sub(r'MAP_GROUP\(NONE\)',      'MAP_GROUP(DYNAMIC)', text)

    with open(filepath, "w", encoding="utf-8") as file:
        file.write(text)

