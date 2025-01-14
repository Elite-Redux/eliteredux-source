enum{
    INFORMATION_ENTRIES_BASIC_INFO,
    INFORMATION_ENTRIES_BATTLE_INFO,
    INFORMATION_ENTRIES_ABILITIES_INFO,
    INFORMATION_ENTRIES_MISC_INFO,
    NUM_INFORMATION_ENTRIES,
};

//Basic Information
enum{
    BASIC_INFORMATION_1,
    BASIC_INFORMATION_2,
    BASIC_INFORMATION_3,
    BASIC_INFORMATION_4,
    BASIC_INFORMATION_5,
    BASIC_INFORMATION_6,
    BASIC_INFORMATION_7,
    BASIC_INFORMATION_8,
    BASIC_INFORMATION_9,
    BASIC_INFORMATION_10,
    NUM_BASIC_ENTRIES,
};

//Battle Information
enum{
    BATTLE_INFORMATION_1,
    BATTLE_INFORMATION_2,
    BATTLE_INFORMATION_3,
    NUM_BATTLE_ENTRIES,
};

//Abilities Information
enum{
    ABILITIES_INFORMATION_1,
    ABILITIES_INFORMATION_2,
    ABILITIES_INFORMATION_3,
    NUM_ABILITIES_ENTRIES,
};

//Misc Information
enum{
    MISC_INFORMATION_1,
    MISC_INFORMATION_2,
    MISC_INFORMATION_3,
    MISC_INFORMATION_4,
    MISC_INFORMATION_5,
    NUM_MISC_ENTRIES,
};

const struct MenuEntry sMenuEntry[NUM_INFORMATION_ENTRIES] = {
    [INFORMATION_ENTRIES_BASIC_INFO] = {
        .title = _("Basic Information"),
        .numEntries = NUM_BASIC_ENTRIES,
        .paletteNum = MENU_COLOR_BLUE,
    },
    [INFORMATION_ENTRIES_BATTLE_INFO] = {
        .title = _("Battle Information"),
        .numEntries = NUM_BATTLE_ENTRIES,
        .paletteNum = MENU_COLOR_RED,
    },
    [INFORMATION_ENTRIES_ABILITIES_INFO] = {
        .title = _("Abilities Information"),
        .numEntries = NUM_ABILITIES_ENTRIES,
        .paletteNum = MENU_COLOR_YELLOW,
    },
    [INFORMATION_ENTRIES_MISC_INFO] = {
        .title = _("Misc Information"),
        .numEntries = NUM_MISC_ENTRIES,
        .paletteNum = MENU_COLOR_GREEN,
    },
};

const struct InformationEntry sInformationEntry[NUM_INFORMATION_ENTRIES][MAX_NUM_ENTRIES_PER_TAB] = {
    [INFORMATION_ENTRIES_BASIC_INFO] = {
        //Basic Information
        [BASIC_INFORMATION_1] = {
            .title = _("Basic Information 1"),
            .description = {
            //Page 1
            _("Lorem ipsum dolor sit amet, consectetur\n"
                "adipiscing elit, sed do eiusmod tempor\n"
                "aliqua. Morbi quis commodo odio aenean\n"
                "sed  adipiscing diam donec. Diam vel quam\n"
                "elementum pulvinar.\n"),
            //Page 2
            _("Aliquam ut porttitor leo a diam\n"
                "sollicitudin tempor id. Vehicula ipsum a\n"
                "arcu cursus vitae congue mauris\n"
                "rhoncus aenean.\n"),
            //Page 3
            _("Vivamus ac purus interdum risus\n:"
                "vestibulum egestas ut sed massa. Class\n"
                "aptent taciti sociosqu ad litora torquent\n"
                "per conubia nostra, per inceptos himenaeos."),
            },
            .numPages = 3
        },
        [BASIC_INFORMATION_2] = {
            .title = _("Basic Information 2"),
            .description = _("Description 2"),
            .numPages = 1
        },
        [BASIC_INFORMATION_3] = {
            .title = _("Basic Information 3"),
            .description = _("Description 3"),
            .numPages = 1
        },
        [BASIC_INFORMATION_4] = {
            .title = _("Basic Information 4"),
            .description = _("Description 4"),
            .numPages = 1
        },
        [BASIC_INFORMATION_5] = {
            .title = _("Basic Information 5"),
            .description = _("Description 5"),
            .numPages = 1
        },
        [BASIC_INFORMATION_6] = {
            .title = _("Basic Information 6"),
            .description = _("Description 6"),
            .numPages = 1
        },
        [BASIC_INFORMATION_7] = {
            .title = _("Basic Information 7"),
            .description = _("Description 7"),
            .numPages = 1
        },
        [BASIC_INFORMATION_8] = {
            .title = _("Basic Information 8"),
            .description = _("Description 8"),
            .numPages = 1
        },
        [BASIC_INFORMATION_9] = {
            .title = _("Basic Information 9"),
            .description = _("Description 9"),
            .numPages = 1
        },
        [BASIC_INFORMATION_10] = {
            .title = _("Basic Information 10"),
            .description = _("Description 10"),
            .numPages = 1
        },
    },
    
    [INFORMATION_ENTRIES_BATTLE_INFO] = {
        //Abilities Information
        [BATTLE_INFORMATION_1] = {
            .title = _("Battle Information 1"),
            .description = {
            //Page 1
            _("Page 1"),
            //Page 2
            _("Page 2"),
            //Page 3
            _("Page 3"),
            },
            .numPages = 3
        },
        [BATTLE_INFORMATION_2] = {
            .title = _("Battle Information 2"),
            .description = {
            //Page 1
            _("Page 1"),
            //Page 2
            _("Page 2"),
            //Page 3
            _("Page 3"),
            },
            .numPages = 3
        },
        [BATTLE_INFORMATION_3] = {
            .title = _("Battle Information 3"),
            .description = {
            //Page 1
            _("Page 1"),
            //Page 2
            _("Page 2"),
            //Page 3
            _("Page 3"),
            },
            .numPages = 3
        },
    },
    
    [INFORMATION_ENTRIES_ABILITIES_INFO] = {
        //Abilities Information
        [ABILITIES_INFORMATION_1] = {
            .title = _("Abilities and Innates"),
            .description = 
            //Page 1
            _(
                "Pokemon within this game have 3 passive        \n"
                "Abilities which are called Innates, these can't\n"
                "be changed. Only Abilities are changed from    \n"
                "within the Pokemon summary screen."
            ),
            .numPages = 1
        },
        [ABILITIES_INFORMATION_2] = {
            .title = _("Abilities Information 2"),
            .description = {
            //Page 1
            _("Lorem ipsum dolor sit amet, consectetur\n"
                "adipiscing elit, sed do eiusmod tempor\n"
                "aliqua. Morbi quis commodo odio aenean\n"
                "sed  adipiscing diam donec. Diam vel quam\n"
                "elementum pulvinar.\n"),
            //Page 2
            _("Aliquam ut porttitor leo a diam\n"
                "sollicitudin tempor id. Vehicula ipsum a\n"
                "arcu cursus vitae congue mauris\n"
                "rhoncus aenean.\n"),
            //Page 3
            _("Vivamus ac purus interdum risus\n:"
                "vestibulum egestas ut sed massa. Class\n"
                "aptent taciti sociosqu ad litora torquent\n"
                "per conubia nostra, per inceptos himenaeos."),
            },
            .numPages = 3
        },
        [ABILITIES_INFORMATION_3] = {
            .title = _("Abilities Information 3"),
            .description = _("Description 3"),
            .numPages = 1
        },
    },
    [INFORMATION_ENTRIES_MISC_INFO] = {
        //Misc Information
        [MISC_INFORMATION_1] = {
            .title = _("Misc Information 1"),
            .description = {
            //Page 1
            _("Lorem ipsum dolor sit amet, consectetur\n"
                "adipiscing elit, sed do eiusmod tempor\n"
                "aliqua. Morbi quis commodo odio aenean\n"
                "sed  adipiscing diam donec. Diam vel quam\n"
                "elementum pulvinar.\n"),
            //Page 2
            _("Aliquam ut porttitor leo a diam\n"
                "sollicitudin tempor id. Vehicula ipsum a\n"
                "arcu cursus vitae congue mauris\n"
                "rhoncus aenean.\n"),
            //Page 3
            _("Vivamus ac purus interdum risus\n:"
                "vestibulum egestas ut sed massa. Class\n"
                "aptent taciti sociosqu ad litora torquent\n"
                "per conubia nostra, per inceptos himenaeos."),
            },
            .numPages = 3
        },
        [MISC_INFORMATION_2] = {
            .title = _("Misc Information 2"),
            .description = _("Description 2"),
            .numPages = 1
        },
        [MISC_INFORMATION_3] = {
            .title = _("Misc Information 3"),
            .description = _("Description 3"),
            .numPages = 1
        },
        [MISC_INFORMATION_4] = {
            .title = _("Misc Information 4"),
            .description = _("Description 4"),
            .numPages = 1
        },
        [MISC_INFORMATION_5] = {
            .title = _("Misc Information 5"),
            .description = _("Description 5"),
            .numPages = 1
        },
    }
};