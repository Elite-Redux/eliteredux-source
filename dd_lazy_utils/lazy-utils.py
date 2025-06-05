#!/usr/bin/env python3

# Import the necessary packages for the UI
from consolemenu import *
from consolemenu.items import *
from colors import color
# import for the logic
import re

#### UTILS PART ####
def formatDefine(str):
    return (re.sub('[^0-9a-zA-Z _]+', '', str.upper().replace(' ', '_')))

def manipulateFile(filepath, func):
    buffer = "pysucks" # copy the file in memory a second time because python sucks
    with open(filepath, 'r') as fp:
        lines = fp.readlines()
        buffer = lines
        line_index = -1
        for line in lines:
            line_index += 1
            func(line, line_index, buffer)
    with open(filepath, 'w') as fp:
        fp.writelines(buffer) 
#### LOGIC PART ####

class DefineTarget:
    def __init__(self) -> None:
        self.macros = {}

    def parseLineForMacro(self, line: str, buffer: list[str]):
        if not re.search('#define', line):
            return
        line = line.replace('#define', '').strip()
        macro = re.match('\w+', line)[0]
        value = line.replace(macro, '').strip()
        self.macros[macro] = value
        print(macro, value)

    

def addBattleEvent():
    global battleEventAdd
    name = 'BATTLE_EVENT_{}'.format(formatDefine(battleEventAdd["NAME"]))

#### UI INTERACTION PART ####

def AskInput(question, attr, proxy):
    proxy[attr] = input(question)
    


#### UI PART #### 
# ask the user for a series of details
battleEventAdd = {
    "NAME": "",
}

    
def initBattleEvent(parent):
    global battleEventAdd
    submenu = ConsoleMenu()
    def currName():
        global battleEventAdd
        name = battleEventAdd["NAME"]
        if not name:
            return "Set a name for the battle event: "
        name = 'BATTLE_EVENT_{}'.format(formatDefine(name))
        return 'current: {} [reselect to change it]'.format(color(name, fg="blue"))
    def currStatus():
        global battleEventAdd
        name = battleEventAdd["NAME"]
        if not name:
            return ""
        return "Add it!"

    items = [
        FunctionItem(currName, AskInput, args=["enter battle event name: ", "NAME", battleEventAdd]),
        FunctionItem(currStatus, addBattleEvent)
    ]
    for item in items:
        submenu.append_item(item)
    menu_item = SubmenuItem("Add battle events", submenu=submenu)
    menu_item.set_menu(parent)
    return menu_item

# create appending stuff submenu
def initAppendingStuff():
    global menu
    submenu = ConsoleMenu(title="Add auto some stuff")
    items = [
        initBattleEvent(submenu)
    ]
    for item in items:
        submenu.append_item(item)
    menu_item = SubmenuItem("Append stuff", submenu=submenu)
    menu_item.set_menu(menu)
    return menu_item

# Create the root menu
menu = ConsoleMenu("Redux Lazy Utils",)

def initBaseMenu():
    global menu
    # Create some items

    menu_base = [
        # A FunctionItem runs a Python function when selected
        initAppendingStuff()
    ]
    # Once we're done creating them, we just add the items to the menu
    for item in menu_base:
        menu.append_item(item)
    menu.show()

if __name__ == "__main__":
    definetarget = DefineTarget()
    def pysucks(s, f, seed):
        definetarget.parseLineForMacro(s, f)
    manipulateFile('./include/constants/battle_events.h', pysucks)
    # initBaseMenu()

# Finally, we call show to show the menu and allow the user to interact


