#!/bin/python3
import re

trainer_parties_filepath = "./src/data/trainer_parties.h"
#the fact you can't read and write a file in the same loop without having corruption drove me insane
python_is_garbage = []
with open(trainer_parties_filepath, 'r') as fp:
    lines = fp.readlines()
    python_is_garbage = lines
    line_index = -1
    for line in lines:
        line_index += 1
        if re.search('.evs', line):
            start_line = re.match('[\s]+.evs[\s]+=[\s]+', line)[0]
            evs_txt = line.replace(start_line, '')
            evs_txt = re.search(r"(?<={).*(?=})", evs_txt)[0]
            evs_stats = re.findall('\d+', evs_txt)
            if len(evs_stats) > 6:
                bruh = start_line + '{'
                bruh += ", ".join(evs_stats[:-1]) + '},\n'
                python_is_garbage[line_index] = bruh

with open(trainer_parties_filepath, 'w') as fp:
    fp.writelines(python_is_garbage)
