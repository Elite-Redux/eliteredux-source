#ifndef GUARD_TRAINERS_H
#define GUARD_TRAINERS_H

#include "constants/opponents.h"
#include "constants/battle_frontier_trainers.h"
#include "generated/constants/trainer_field_enums.h"

// Special Trainer Ids.
//      0-299 are frontier trainers
#define TRAINER_RECORD_MIXING_FRIEND        FRONTIER_TRAINERS_COUNT
#define TRAINER_RECORD_MIXING_APPRENTICE    400
#define TRAINER_EREADER                     500
#define TRAINER_FRONTIER_BRAIN              1022
#define TRAINER_PLAYER                      1023
#define TRAINER_SECRET_BASE                 1024
#define TRAINER_LINK_OPPONENT               2048
#define TRAINER_UNION_ROOM                  3072
#define TRAINER_STEVEN_PARTNER              3075
#define TRAINER_CUSTOM_PARTNER              3076

#define TRAINER_BACK_PIC_BRENDAN                0
#define TRAINER_BACK_PIC_MAY                    1
#define TRAINER_BACK_PIC_RED                    2
#define TRAINER_BACK_PIC_LEAF                   3
#define TRAINER_BACK_PIC_RUBY_SAPPHIRE_BRENDAN  4
#define TRAINER_BACK_PIC_RUBY_SAPPHIRE_MAY      5
#define TRAINER_BACK_PIC_WALLY                  6
#define TRAINER_BACK_PIC_STEVEN                 7

#define FACILITY_CLASS_HIKER                 0x0
#define FACILITY_CLASS_AQUA_GRUNT_M          0x1
#define FACILITY_CLASS_PKMN_BREEDER_F        0x2
#define FACILITY_CLASS_COOLTRAINER_M         0x3
#define FACILITY_CLASS_BIRD_KEEPER           0x4
#define FACILITY_CLASS_COLLECTOR             0x5
#define FACILITY_CLASS_AQUA_GRUNT_F          0x6
#define FACILITY_CLASS_SWIMMER_M             0x7
#define FACILITY_CLASS_MAGMA_GRUNT_M         0x8
#define FACILITY_CLASS_EXPERT_M              0x9
#define FACILITY_CLASS_BLACK_BELT            0xa
#define FACILITY_CLASS_AQUA_LEADER_ARCHIE    0xb
#define FACILITY_CLASS_HEX_MANIAC            0xc
#define FACILITY_CLASS_AROMA_LADY            0xd
#define FACILITY_CLASS_RUIN_MANIAC           0xe
#define FACILITY_CLASS_INTERVIEWER           0xf
#define FACILITY_CLASS_TUBER_F               0x10
#define FACILITY_CLASS_TUBER_M               0x11
#define FACILITY_CLASS_COOLTRAINER_F         0x12
#define FACILITY_CLASS_LADY                  0x13
#define FACILITY_CLASS_BEAUTY                0x14
#define FACILITY_CLASS_RICH_BOY              0x15
#define FACILITY_CLASS_EXPERT_F              0x16
#define FACILITY_CLASS_POKEMANIAC            0x17
#define FACILITY_CLASS_MAGMA_GRUNT_F         0x18
#define FACILITY_CLASS_GUITARIST             0x19
#define FACILITY_CLASS_KINDLER               0x1a
#define FACILITY_CLASS_CAMPER                0x1b
#define FACILITY_CLASS_PICNICKER             0x1c
#define FACILITY_CLASS_BUG_MANIAC            0x1d
#define FACILITY_CLASS_PSYCHIC_M             0x1e
#define FACILITY_CLASS_PSYCHIC_F             0x1f
#define FACILITY_CLASS_GENTLEMAN             0x20
#define FACILITY_CLASS_ELITE_FOUR_SIDNEY     0x21
#define FACILITY_CLASS_ELITE_FOUR_PHOEBE     0x22
#define FACILITY_CLASS_LEADER_ROXANNE        0x23
#define FACILITY_CLASS_LEADER_BRAWLY         0x24
#define FACILITY_CLASS_LEADER_TATE_AND_LIZA  0x25
#define FACILITY_CLASS_SCHOOL_KID_M          0x26
#define FACILITY_CLASS_SCHOOL_KID_F          0x27
#define FACILITY_CLASS_SR_AND_JR             0x28
#define FACILITY_CLASS_POKEFAN_M             0x29
#define FACILITY_CLASS_POKEFAN_F             0x2a
#define FACILITY_CLASS_YOUNGSTER             0x2b
#define FACILITY_CLASS_CHAMPION_WALLACE      0x2c
#define FACILITY_CLASS_FISHERMAN             0x2d
#define FACILITY_CLASS_CYCLING_TRIATHLETE_M  0x2e
#define FACILITY_CLASS_CYCLING_TRIATHLETE_F  0x2f
#define FACILITY_CLASS_RUNNING_TRIATHLETE_M  0x30
#define FACILITY_CLASS_RUNNING_TRIATHLETE_F  0x31
#define FACILITY_CLASS_SWIMMING_TRIATHLETE_M 0x32
#define FACILITY_CLASS_SWIMMING_TRIATHLETE_F 0x33
#define FACILITY_CLASS_DRAGON_TAMER          0x34
#define FACILITY_CLASS_NINJA_BOY             0x35
#define FACILITY_CLASS_BATTLE_GIRL           0x36
#define FACILITY_CLASS_PARASOL_LADY          0x37
#define FACILITY_CLASS_SWIMMER_F             0x38
#define FACILITY_CLASS_TWINS                 0x39
#define FACILITY_CLASS_SAILOR                0x3a
#define FACILITY_CLASS_WALLY                 0x3b
#define FACILITY_CLASS_BRENDAN               0x3c
#define FACILITY_CLASS_BRENDAN_2             0x3d
#define FACILITY_CLASS_BRENDAN_3             0x3e
#define FACILITY_CLASS_MAY                   0x3f
#define FACILITY_CLASS_MAY_2                 0x40
#define FACILITY_CLASS_MAY_3                 0x41
#define FACILITY_CLASS_PKMN_BREEDER_M        0x42
#define FACILITY_CLASS_BUG_CATCHER           0x43
#define FACILITY_CLASS_PKMN_RANGER_M         0x44
#define FACILITY_CLASS_PKMN_RANGER_F         0x45
#define FACILITY_CLASS_MAGMA_LEADER_MAXIE    0x46
#define FACILITY_CLASS_LASS                  0x47
#define FACILITY_CLASS_YOUNG_COUPLE          0x48
#define FACILITY_CLASS_OLD_COUPLE            0x49
#define FACILITY_CLASS_SIS_AND_BRO           0x4a
#define FACILITY_CLASS_STEVEN                0x4b
#define FACILITY_CLASS_SALON_MAIDEN_ANABEL   0x4c
#define FACILITY_CLASS_DOME_ACE_TUCKER       0x4d
#define FACILITY_CLASS_RED                   0x4e
#define FACILITY_CLASS_LEAF                  0x4f
#define FACILITY_CLASS_RS_BRENDAN            0x50
#define FACILITY_CLASS_RS_MAY                0x51

#define FACILITY_CLASSES_COUNT               0x52

#define RS_FACILITY_CLASS_AQUA_LEADER_ARCHIE    0x0
#define RS_FACILITY_CLASS_AQUA_GRUNT_M          0x1
#define RS_FACILITY_CLASS_AQUA_GRUNT_F          0x2
#define RS_FACILITY_CLASS_AROMA_LADY            0x3
#define RS_FACILITY_CLASS_RUIN_MANIAC           0x4
#define RS_FACILITY_CLASS_INTERVIEWER           0x5
#define RS_FACILITY_CLASS_TUBER_F               0x6
#define RS_FACILITY_CLASS_TUBER_M               0x7
#define RS_FACILITY_CLASS_COOLTRAINER_M         0x8
#define RS_FACILITY_CLASS_COOLTRAINER_F         0x9
#define RS_FACILITY_CLASS_HEX_MANIAC            0xA
#define RS_FACILITY_CLASS_LADY                  0xB
#define RS_FACILITY_CLASS_BEAUTY                0xC
#define RS_FACILITY_CLASS_RICH_BOY              0xD
#define RS_FACILITY_CLASS_POKEMANIAC            0xE
#define RS_FACILITY_CLASS_SWIMMER_M             0xF
#define RS_FACILITY_CLASS_BLACK_BELT            0x10
#define RS_FACILITY_CLASS_GUITARIST             0x11
#define RS_FACILITY_CLASS_KINDLER               0x12
#define RS_FACILITY_CLASS_CAMPER                0x13
#define RS_FACILITY_CLASS_BUG_MANIAC            0x14
#define RS_FACILITY_CLASS_PSYCHIC_M             0x15
#define RS_FACILITY_CLASS_PSYCHIC_F             0x16
#define RS_FACILITY_CLASS_GENTLEMAN             0x17
#define RS_FACILITY_CLASS_ELITE_FOUR_M          0x18
#define RS_FACILITY_CLASS_ELITE_FOUR_F          0x19
#define RS_FACILITY_CLASS_LEADER_F              0x1A
#define RS_FACILITY_CLASS_LEADER_M              0x1B
#define RS_FACILITY_CLASS_LEADER_MF             0x1C
#define RS_FACILITY_CLASS_SCHOOL_KID_M          0x1D
#define RS_FACILITY_CLASS_SCHOOL_KID_F          0x1E
#define RS_FACILITY_CLASS_SR_AND_JR             0x1F
#define RS_FACILITY_CLASS_POKEFAN_M             0x20
#define RS_FACILITY_CLASS_POKEFAN_F             0x21
#define RS_FACILITY_CLASS_EXPERT_M              0x22
#define RS_FACILITY_CLASS_EXPERT_F              0x23
#define RS_FACILITY_CLASS_YOUNGSTER             0x24
#define RS_FACILITY_CLASS_CHAMPION              0x25
#define RS_FACILITY_CLASS_FISHERMAN             0x26
#define RS_FACILITY_CLASS_CYCLING_TRIATHLETE_M  0x27
#define RS_FACILITY_CLASS_CYCLING_TRIATHLETE_F  0x28
#define RS_FACILITY_CLASS_RUNNING_TRIATHLETE_M  0x29
#define RS_FACILITY_CLASS_RUNNING_TRIATHLETE_F  0x2A
#define RS_FACILITY_CLASS_SWIMMING_TRIATHLETE_M 0x2B
#define RS_FACILITY_CLASS_SWIMMING_TRIATHLETE_F 0x2C
#define RS_FACILITY_CLASS_DRAGON_TAMER          0x2D
#define RS_FACILITY_CLASS_BIRD_KEEPER           0x2E
#define RS_FACILITY_CLASS_NINJA_BOY             0x2F
#define RS_FACILITY_CLASS_BATTLE_GIRL           0x30
#define RS_FACILITY_CLASS_PARASOL_LADY          0x31
#define RS_FACILITY_CLASS_SWIMMER_F             0x32
#define RS_FACILITY_CLASS_PICNICKER             0x33
#define RS_FACILITY_CLASS_TWINS                 0x34
#define RS_FACILITY_CLASS_SAILOR                0x35
#define RS_FACILITY_CLASS_BOARDER_1             0x36
#define RS_FACILITY_CLASS_BOARDER_2             0x37
#define RS_FACILITY_CLASS_COLLECTOR             0x38
#define RS_FACILITY_CLASS_WALLY                 0x39
#define RS_FACILITY_CLASS_BRENDAN_1             0x3A
#define RS_FACILITY_CLASS_BRENDAN_2             0x3B
#define RS_FACILITY_CLASS_BRENDAN_3             0x3C
#define RS_FACILITY_CLASS_MAY_1                 0x3D
#define RS_FACILITY_CLASS_MAY_2                 0x3E
#define RS_FACILITY_CLASS_MAY_3                 0x3F
#define RS_FACILITY_CLASS_PKMN_BREEDER_M        0x40
#define RS_FACILITY_CLASS_PKMN_BREEDER_F        0x41
#define RS_FACILITY_CLASS_PKMN_RANGER_M         0x42
#define RS_FACILITY_CLASS_PKMN_RANGER_F         0x43
#define RS_FACILITY_CLASS_MAGMA_LEADER          0x44
#define RS_FACILITY_CLASS_MAGMA_GRUNT_M         0x45
#define RS_FACILITY_CLASS_MAGMA_GRUNT_F         0x46
#define RS_FACILITY_CLASS_LASS                  0x47
#define RS_FACILITY_CLASS_BUG_CATCHER           0x48
#define RS_FACILITY_CLASS_HIKER                 0x49
#define RS_FACILITY_CLASS_YOUNG_COUPLE          0x4A
#define RS_FACILITY_CLASS_OLD_COUPLE            0x4B
#define RS_FACILITY_CLASS_SIS_AND_BRO           0x4C

#define RS_FACILITY_CLASSES_COUNT               0x4D

#define F_TRAINER_FEMALE (1 << 7)

// All trainer parties specify the IV, level, and species for each Pokémon in the
// party. Some trainer parties also specify held items and custom moves for each
// Pokémon.
#define F_TRAINER_PARTY_CUSTOM_MOVESET (1 << 0)
#define F_TRAINER_PARTY_HELD_ITEM      (1 << 1)

#endif  // GUARD_TRAINERS_H
