//Different type of generic cries
enum{
    CRY_GENERIC,
    //Species Specific
    CRY_KYOGRE,
    CRY_GROUDON,
    CRY_RAYQUAZA,
    CRY_MANAPHY,
    CRY_PIKACHU,
    CRY_ROTOM,
    CRY_HOOPA,
    CRY_RHYHORN,
    //Fairy
    CRY_FAIRY_SMALL,
    CRY_FAIRY_MEDIUM,
    CRY_FAIRY_BIG,
    //Fish
    CRY_FISH_SMALL,
    CRY_FISH_MEDIUM,
    CRY_FISH_BIG,
    //Mineral
    CRY_MINERAL_SMALL,
    CRY_MINERAL_MEDIUM,
    CRY_MINERAL_BIG,
    //Dragon
    CRY_DRAGON_SMALL,
    CRY_DRAGON_MEDIUM,
    CRY_DRAGON_BIG,
    //Bird
    CRY_BIRD_SMALL,
    CRY_BIRD_MEDIUM,
    CRY_BIRD_BIG,
    //Turtle
    CRY_TURTLE_SMALL,
    CRY_TURTLE_MEDIUM,
    CRY_TURTLE_BIG,
    //Bug
    CRY_BUG_SMALL,
    CRY_BUG_MEDIUM,
    CRY_BUG_BIG,
    //Grass
    CRY_GRASS_SMALL,
    CRY_GRASS_MEDIUM,
    CRY_GRASS_BIG,
    //Monster
    CRY_MONSTER_SMALL,
    CRY_MONSTER_MEDIUM,
    CRY_MONSTER_BIG,
    //Dog
    CRY_DOG_SMALL,
    CRY_DOG_MEDIUM,
    CRY_DOG_BIG,
    //Cat
    CRY_CAT_SMALL,
    CRY_CAT_MEDIUM,
    CRY_CAT_BIG,
    //Monkey
    CRY_MONKEY_SMALL,
    CRY_MONKEY_MEDIUM,
    CRY_MONKEY_BIG,
    //Fighting
    CRY_FIGHTING_SMALL,
    CRY_FIGHTING_MEDIUM,
    CRY_FIGHTING_BIG,
    //Roddent
    CRY_RODDENT_SMALL,
    CRY_RODDENT_MEDIUM,
    CRY_RODDENT_BIG,
    //Crab
    CRY_CRAB_SMALL,
    CRY_CRAB_MEDIUM,
    CRY_CRAB_BIG,
    //Dinosaur
    CRY_DINOSAUR_SMALL,
    CRY_DINOSAUR_MEDIUM,
    CRY_DINOSAUR_BIG,
    //Bat
    CRY_BAT_SMALL,
    CRY_BAT_MEDIUM,
    CRY_BAT_BIG,
    //Snake
    CRY_SNAKE_SMALL,
    CRY_SNAKE_MEDIUM,
    CRY_SNAKE_BIG,
    //Frog
    CRY_FROG_SMALL,
    CRY_FROG_MEDIUM,
    CRY_FROG_BIG,
    //Flower
    CRY_FLOWER_SMALL,
    CRY_FLOWER_MEDIUM,
    CRY_FLOWER_BIG,
    //Squid
    CRY_SQUID_SMALL,
    CRY_SQUID_MEDIUM,
    CRY_SQUID_BIG,
    //Elephant
    CRY_ELEPHANT_SMALL,
    CRY_ELEPHANT_MEDIUM,
    CRY_ELEPHANT_BIG,
    //Mollusk
    CRY_MOLLUSK_SMALL,
    CRY_MOLLUSK_MEDIUM,
    CRY_MOLLUSK_BIG,
    //Imp
    CRY_IMP_SMALL,
    CRY_IMP_MEDIUM,
    CRY_IMP_BIG,
    //Crocodile
    CRY_CROCODILE_SMALL,
    CRY_CROCODILE_MEDIUM,
    CRY_CROCODILE_BIG,
    //Lizard
    CRY_LIZARD_SMALL,
    CRY_LIZARD_MEDIUM,
    CRY_LIZARD_BIG,
    //Lizard
    CRY_FOX_SMALL,
    CRY_FOX_MEDIUM,
    CRY_FOX_BIG,
    //Psy
    CRY_PSY_SMALL,
    CRY_PSY_MEDIUM,
    CRY_PSY_BIG,
    //Duck
    CRY_DUCK_SMALL,
    CRY_DUCK_MEDIUM,
    //Pig
    CRY_PIG_SMALL,
    CRY_PIG_MEDIUM,
    //Horse
    CRY_HORSE_SMALL,
    CRY_HORSE_MEDIUM,
    //Thing
    CRY_THING_SMALL,
    CRY_THING_MEDIUM,
    //Misc
    CRY_BABY,
    CRY_BUTTERFLY,
    CRY_MOLE,
    CRY_CHICKEN,

    CRY_EGG_GROUP_WATER_1,
    CRY_EGG_GROUP_WATER_2,
    CRY_EGG_GROUP_WATER_3,
    CRY_EGG_GROUP_DRAGON,
    CRY_EGG_GROUP_GRASS,
    CRY_EGG_GROUP_BUG,
    CRY_EGG_GROUP_MINERAL,
    CRY_EGG_GROUP_FIELD,
    CRY_EGG_GROUP_HUMAN_LIKE,
    CRY_EGG_GROUP_AMORPHOUS,
    CRY_EGG_GROUP_FAIRY,
    CRY_EGG_GROUP_DITTO,
    CRY_EGG_GROUP_MONSTER,
    CRY_EGG_GROUP_FLYING,
    NUM_CRY_TYPES,
};

//Here you can define directly what cry should use each species, if its not defined it will try to use one depending on its type, egg group and evolution stage, this causes more RAM usage when calculating so its advised to define all of them
const u8 sSpeciesCryNum[NUM_SPECIES] = {
    [SPECIES_BULBASAUR] = CRY_MONSTER_SMALL,
    [SPECIES_IVYSAUR] = CRY_MONSTER_MEDIUM,
    [SPECIES_VENUSAUR] = CRY_MONSTER_BIG,
    [SPECIES_CHARMANDER] = CRY_LIZARD_SMALL,
    [SPECIES_CHARMELEON] = CRY_LIZARD_MEDIUM,
    [SPECIES_CHARIZARD] = CRY_RHYHORN,
    [SPECIES_SQUIRTLE] = CRY_TURTLE_SMALL,
    [SPECIES_WARTORTLE] = CRY_TURTLE_MEDIUM,
    [SPECIES_BLASTOISE] = CRY_TURTLE_BIG,
    [SPECIES_CATERPIE] = CRY_BUG_SMALL,
    [SPECIES_METAPOD] = CRY_BUG_SMALL,
    [SPECIES_BUTTERFREE] = CRY_BUTTERFLY,
    [SPECIES_BUTTERFREE_MEGA] = CRY_BUTTERFLY,
    [SPECIES_WEEDLE] = CRY_BUG_SMALL,
    [SPECIES_KAKUNA] = CRY_BUG_SMALL,
    [SPECIES_BEEDRILL] = CRY_BUG_BIG,
    [SPECIES_PIDGEY] = CRY_BIRD_SMALL,
    [SPECIES_PIDGEOTTO] = CRY_BIRD_MEDIUM,
    [SPECIES_PIDGEOT] = CRY_BIRD_BIG,
    [SPECIES_RATTATA] = CRY_RODDENT_SMALL,
    [SPECIES_RATICATE] = CRY_RODDENT_MEDIUM,
    [SPECIES_SPEAROW] = CRY_BIRD_MEDIUM,
    [SPECIES_FEAROW] = CRY_BIRD_BIG,
    [SPECIES_EKANS] = CRY_SNAKE_SMALL,
    [SPECIES_ARBOK] = CRY_SNAKE_MEDIUM,
    [SPECIES_PICHU] = CRY_PIKACHU,
    [SPECIES_PIKACHU] = CRY_PIKACHU,
    [SPECIES_RAICHU] = CRY_PIKACHU,
    [SPECIES_SANDSHREW] = CRY_RODDENT_SMALL,
    [SPECIES_SANDSLASH] = CRY_RODDENT_BIG,
    [SPECIES_NIDORAN_F] = CRY_RODDENT_SMALL,
    [SPECIES_NIDORINA] = CRY_MONSTER_MEDIUM,
    [SPECIES_NIDOQUEEN] = CRY_MONSTER_BIG,
    [SPECIES_NIDORAN_M] = CRY_RODDENT_SMALL,
    [SPECIES_NIDORINO] = CRY_MONSTER_MEDIUM,
    [SPECIES_NIDOKING] = CRY_MONSTER_BIG,
    [SPECIES_CLEFFA] = CRY_BABY,
    [SPECIES_CLEFAIRY] = CRY_FAIRY_MEDIUM,
    [SPECIES_CLEFABLE] = CRY_FAIRY_MEDIUM,
    [SPECIES_CLEFABLE_MEGA] = CRY_FAIRY_MEDIUM,
    [SPECIES_VULPIX] = CRY_FOX_SMALL,
    [SPECIES_NINETALES] = CRY_FOX_MEDIUM,
    [SPECIES_IGGLYBUFF] = CRY_BABY,
    [SPECIES_JIGGLYPUFF] = CRY_FAIRY_MEDIUM,
    [SPECIES_WIGGLYTUFF] = CRY_FAIRY_MEDIUM,
    [SPECIES_ZUBAT] = CRY_BAT_SMALL,
    [SPECIES_GOLBAT] = CRY_BAT_MEDIUM,
    [SPECIES_CROBAT] = CRY_BAT_BIG,
    [SPECIES_ODDISH] = CRY_GRASS_SMALL,
    [SPECIES_GLOOM] = CRY_GRASS_SMALL,
    [SPECIES_VILEPLUME] = CRY_GRASS_MEDIUM,
    [SPECIES_BELLOSSOM] = CRY_FLOWER_MEDIUM,
    [SPECIES_PARAS] = CRY_CRAB_SMALL,
    [SPECIES_PARASECT] = CRY_CRAB_MEDIUM,
    [SPECIES_VENONAT] = CRY_BUG_SMALL,
    [SPECIES_VENOMOTH] = CRY_BUG_SMALL,
    [SPECIES_DIGLETT] = CRY_MOLE,
    [SPECIES_DUGTRIO] = CRY_MOLE,
    [SPECIES_MEOWTH] = CRY_CAT_SMALL,
    [SPECIES_PERSIAN] = CRY_CAT_MEDIUM,
    [SPECIES_PSYDUCK] = CRY_DUCK_SMALL,
    [SPECIES_GOLDUCK] = CRY_DUCK_MEDIUM,
    [SPECIES_MANKEY] = CRY_MONKEY_MEDIUM,
    [SPECIES_PRIMEAPE] = CRY_MONKEY_BIG,
    [SPECIES_ANNIHILAPE] = CRY_MONKEY_BIG,
    [SPECIES_GROWLITHE] = CRY_DOG_MEDIUM,
    [SPECIES_ARCANINE] = CRY_DOG_BIG,
    [SPECIES_POLIWAG] = CRY_FROG_SMALL,
    [SPECIES_POLIWHIRL] = CRY_FROG_MEDIUM,
    [SPECIES_POLIWRATH] = CRY_FROG_BIG,
    [SPECIES_POLITOED] = CRY_FROG_BIG,
    [SPECIES_ABRA] = CRY_PSY_SMALL,
    [SPECIES_KADABRA] = CRY_PSY_MEDIUM,
    [SPECIES_ALAKAZAM] = CRY_PSY_BIG,
    [SPECIES_MACHOP] = CRY_FIGHTING_SMALL,
    [SPECIES_MACHOKE] = CRY_FIGHTING_MEDIUM,
    [SPECIES_MACHAMP] = CRY_FIGHTING_BIG,
    [SPECIES_MACHAMP_MEGA] = CRY_FIGHTING_BIG,
    [SPECIES_BELLSPROUT] = CRY_GRASS_SMALL,
    [SPECIES_WEEPINBELL] = CRY_GRASS_SMALL,
    [SPECIES_VICTREEBEL] = CRY_GRASS_MEDIUM,
    [SPECIES_TENTACOOL] = CRY_SQUID_SMALL,
    [SPECIES_TENTACRUEL] = CRY_SQUID_MEDIUM,
    [SPECIES_GEODUDE] = CRY_MINERAL_SMALL,
    [SPECIES_GRAVELER] = CRY_MINERAL_MEDIUM,
    [SPECIES_GOLEM] = CRY_MINERAL_BIG,
    [SPECIES_PONYTA] = CRY_HORSE_SMALL,
    [SPECIES_RAPIDASH] = CRY_HORSE_MEDIUM,
    [SPECIES_SLOWPOKE] = CRY_PIG_SMALL,
    [SPECIES_SLOWBRO] = CRY_PIG_MEDIUM,
    [SPECIES_SLOWKING] = CRY_PIG_MEDIUM,
    [SPECIES_MAGNEMITE] = CRY_MINERAL_SMALL,
    [SPECIES_MAGNETON] = CRY_MINERAL_MEDIUM,
    [SPECIES_MAGNEZONE] = CRY_MINERAL_BIG,
    [SPECIES_FARFETCHD] = CRY_DUCK_SMALL,
    [SPECIES_DODUO] = CRY_BIRD_MEDIUM,
    [SPECIES_DODRIO] = CRY_BIRD_SMALL,
    [SPECIES_SEEL] = CRY_PIG_SMALL,
    [SPECIES_DEWGONG] = CRY_PIG_MEDIUM,
    [SPECIES_GRIMER] = CRY_THING_SMALL,
    [SPECIES_MUK] = CRY_THING_MEDIUM,
    [SPECIES_SHELLDER] = CRY_PIG_SMALL,
    [SPECIES_CLOYSTER] = CRY_PIG_MEDIUM,
    [SPECIES_GASTLY] = CRY_PSY_SMALL,
    [SPECIES_HAUNTER] = CRY_PSY_MEDIUM,
    [SPECIES_GENGAR] = CRY_PSY_BIG,
    [SPECIES_ONIX] = CRY_MINERAL_BIG,
    [SPECIES_STEELIX] = CRY_MINERAL_BIG,
    [SPECIES_DROWZEE] = CRY_PIG_MEDIUM,
    [SPECIES_HYPNO] = CRY_PSY_MEDIUM,
    [SPECIES_KRABBY] = CRY_CRAB_SMALL,
    [SPECIES_KINGLER] = CRY_CRAB_MEDIUM,
    [SPECIES_KINGLER_MEGA] = CRY_CRAB_MEDIUM,
    [SPECIES_VOLTORB] = CRY_THING_SMALL,
    [SPECIES_ELECTRODE] = CRY_THING_MEDIUM,
    [SPECIES_EXEGGCUTE] = CRY_GRASS_SMALL,
    [SPECIES_EXEGGUTOR] = CRY_PIG_MEDIUM,
    [SPECIES_CUBONE] = CRY_DINOSAUR_SMALL,
    [SPECIES_MAROWAK] = CRY_DINOSAUR_MEDIUM,
    [SPECIES_TYROGUE] = CRY_FIGHTING_SMALL,
    [SPECIES_HITMONLEE] = CRY_FIGHTING_MEDIUM,
    [SPECIES_HITMONCHAN] = CRY_FIGHTING_MEDIUM,
    [SPECIES_HITMONTOP] = CRY_FIGHTING_MEDIUM,
    [SPECIES_LICKITUNG] = CRY_PIG_SMALL,
    [SPECIES_LICKILICKY] = CRY_PIG_MEDIUM,
    [SPECIES_KOFFING] = CRY_THING_SMALL,
    [SPECIES_WEEZING] = CRY_THING_MEDIUM,
    [SPECIES_RHYHORN] = CRY_RHYHORN,
    [SPECIES_RHYDON] = CRY_RHYHORN,
    [SPECIES_RHYPERIOR] = CRY_MONSTER_BIG,
    [SPECIES_HAPPINY] = CRY_BABY,
    [SPECIES_CHANSEY] = CRY_FAIRY_MEDIUM,
    [SPECIES_BLISSEY] = CRY_FAIRY_MEDIUM,
    [SPECIES_TANGELA] = CRY_GRASS_MEDIUM,
    [SPECIES_TANGROWTH] = CRY_GRASS_BIG,
    [SPECIES_KANGASKHAN] = CRY_MONSTER_BIG,
    [SPECIES_HORSEA] = CRY_SQUID_SMALL,
    [SPECIES_SEADRA] = CRY_SQUID_MEDIUM,
    [SPECIES_KINGDRA] = CRY_DRAGON_MEDIUM,
    [SPECIES_GOLDEEN] = CRY_FISH_SMALL,
    [SPECIES_SEAKING] = CRY_FISH_MEDIUM,
    [SPECIES_STARYU] = CRY_THING_SMALL,
    [SPECIES_STARMIE] = CRY_THING_MEDIUM,
    [SPECIES_MIME_JR] = CRY_PIG_SMALL,
    [SPECIES_MR_MIME] = CRY_PIG_MEDIUM,
    [SPECIES_SCYTHER] = CRY_BUG_BIG,
    [SPECIES_SCIZOR] = CRY_BUG_BIG,
    [SPECIES_SMOOCHUM] = CRY_BABY,
    [SPECIES_JYNX] = CRY_PSY_MEDIUM,
    [SPECIES_ELEKID] = CRY_FIGHTING_SMALL,
    [SPECIES_ELECTABUZZ] = CRY_FIGHTING_MEDIUM,
    [SPECIES_ELECTIVIRE] = CRY_FIGHTING_BIG,
    [SPECIES_MAGBY] = CRY_FIGHTING_SMALL,
    [SPECIES_MAGMAR] = CRY_FIGHTING_MEDIUM,
    [SPECIES_MAGMORTAR] = CRY_FIGHTING_BIG,
    [SPECIES_PINSIR] = CRY_BUG_BIG,
    [SPECIES_TAUROS] = CRY_MONSTER_MEDIUM,
    [SPECIES_MAGIKARP] = CRY_FISH_SMALL,
    [SPECIES_GYARADOS] = CRY_MONSTER_BIG,
    [SPECIES_LAPRAS] = CRY_TURTLE_BIG,
    [SPECIES_LAPRAS_MEGA] = CRY_TURTLE_BIG,
    [SPECIES_DITTO] = CRY_EGG_GROUP_DITTO,
    [SPECIES_EEVEE] = CRY_FOX_SMALL,
    [SPECIES_VAPOREON] = CRY_FOX_MEDIUM,
    [SPECIES_JOLTEON] = CRY_FOX_MEDIUM,
    [SPECIES_FLAREON] = CRY_FOX_MEDIUM,
    [SPECIES_ESPEON] = CRY_FOX_MEDIUM,
    [SPECIES_UMBREON] = CRY_FOX_MEDIUM,
    [SPECIES_LEAFEON] = CRY_FOX_MEDIUM,
    [SPECIES_GLACEON] = CRY_FOX_MEDIUM,
    [SPECIES_SYLVEON] = CRY_FOX_MEDIUM,
    [SPECIES_PORYGON] = CRY_THING_SMALL,
    [SPECIES_PORYGON2] = CRY_THING_SMALL,
    [SPECIES_PORYGON_Z] = CRY_THING_SMALL,
    [SPECIES_OMANYTE] = CRY_SQUID_SMALL,
    [SPECIES_OMASTAR] = CRY_SQUID_MEDIUM,
    [SPECIES_KABUTO] = CRY_SQUID_SMALL,
    [SPECIES_KABUTOPS] = CRY_SQUID_MEDIUM,
    [SPECIES_AERODACTYL] = CRY_DINOSAUR_BIG,
    [SPECIES_MUNCHLAX] = CRY_PIG_SMALL,
    [SPECIES_SNORLAX] = CRY_PIG_MEDIUM,
    [SPECIES_SNORLAX_MEGA] = CRY_PIG_MEDIUM,
    [SPECIES_ARTICUNO] = CRY_BIRD_BIG,
    [SPECIES_ZAPDOS] = CRY_BIRD_BIG,
    [SPECIES_MOLTRES] = CRY_BIRD_BIG,
    [SPECIES_DRATINI] = CRY_DRAGON_SMALL,
    [SPECIES_DRAGONAIR] = CRY_DRAGON_MEDIUM,
    [SPECIES_DRAGONITE] = CRY_DRAGON_BIG,
    [SPECIES_MEWTWO] = CRY_PSY_BIG,
    [SPECIES_MEW] = CRY_FAIRY_SMALL,
    [SPECIES_CHIKORITA] = CRY_GRASS_SMALL,
    [SPECIES_BAYLEEF] = CRY_DINOSAUR_MEDIUM,
    [SPECIES_MEGANIUM] = CRY_MONSTER_MEDIUM,
    [SPECIES_CYNDAQUIL] = CRY_RODDENT_SMALL,
    [SPECIES_QUILAVA] = CRY_RODDENT_SMALL,
    [SPECIES_TYPHLOSION] = CRY_MONSTER_MEDIUM,
    [SPECIES_TOTODILE] = CRY_LIZARD_SMALL,
    [SPECIES_CROCONAW] = CRY_LIZARD_MEDIUM,
    [SPECIES_FERALIGATR] = CRY_LIZARD_BIG,
    [SPECIES_SENTRET] = CRY_RODDENT_SMALL,
    [SPECIES_FURRET] = CRY_RODDENT_SMALL,
    [SPECIES_HOOTHOOT] = CRY_BIRD_SMALL,
    [SPECIES_NOCTOWL] = CRY_BIRD_MEDIUM,
    [SPECIES_LEDYBA] = CRY_BUG_SMALL,
    [SPECIES_LEDIAN] = CRY_BUTTERFLY,
    [SPECIES_SPINARAK] = CRY_BUG_SMALL,
    [SPECIES_ARIADOS] = CRY_BUG_MEDIUM,
    [SPECIES_CHINCHOU] = CRY_MINERAL_SMALL,
    [SPECIES_LANTURN] = CRY_FISH_MEDIUM,
    [SPECIES_TOGEPI] = CRY_BABY,
    [SPECIES_TOGETIC] = CRY_FAIRY_MEDIUM,
    [SPECIES_TOGEKISS] = CRY_FAIRY_BIG,
    [SPECIES_NATU] = CRY_BIRD_MEDIUM,
    [SPECIES_XATU] = CRY_PSY_MEDIUM,
    [SPECIES_MAREEP] = CRY_HORSE_SMALL,
    [SPECIES_FLAAFFY] = CRY_HORSE_MEDIUM,
    [SPECIES_AMPHAROS] = CRY_DRAGON_MEDIUM,
    [SPECIES_AZURILL] = CRY_BABY,
    [SPECIES_MARILL] = CRY_RODDENT_SMALL,
    [SPECIES_AZUMARILL] = CRY_FAIRY_SMALL,
    [SPECIES_BONSLY] = CRY_GRASS_MEDIUM,
    [SPECIES_SUDOWOODO] = CRY_PIG_SMALL,
    [SPECIES_HOPPIP] = CRY_GRASS_SMALL,
    [SPECIES_SKIPLOOM] = CRY_GRASS_MEDIUM,
    [SPECIES_JUMPLUFF] = CRY_GRASS_MEDIUM,
    [SPECIES_AIPOM] = CRY_MONKEY_SMALL,
    [SPECIES_AMBIPOM] = CRY_MONKEY_SMALL,
    [SPECIES_SUNKERN] = CRY_GRASS_SMALL,
    [SPECIES_SUNFLORA] = CRY_FLOWER_MEDIUM,
    [SPECIES_YANMA] = CRY_BUG_MEDIUM,
    [SPECIES_YANMEGA] = CRY_BUG_MEDIUM,
    [SPECIES_WOOPER] = CRY_PIG_SMALL,
    [SPECIES_QUAGSIRE] = CRY_PIG_MEDIUM,
    [SPECIES_MURKROW] = CRY_BIRD_SMALL,
    [SPECIES_HONCHKROW] = CRY_BIRD_MEDIUM,
    [SPECIES_MISDREAVUS] = CRY_FAIRY_SMALL,
    [SPECIES_MISMAGIUS] = CRY_FAIRY_MEDIUM,
    [SPECIES_UNOWN] = CRY_THING_SMALL,
    [SPECIES_WYNAUT] = CRY_THING_SMALL,
    [SPECIES_WOBBUFFET] = CRY_PIG_SMALL,
    [SPECIES_GIRAFARIG] = CRY_HORSE_SMALL,
    [SPECIES_FARIGIRAF] = CRY_HORSE_SMALL,
    [SPECIES_PINECO] = CRY_MINERAL_SMALL,
    [SPECIES_FORRETRESS] = CRY_MINERAL_MEDIUM,
    [SPECIES_DUNSPARCE] = CRY_SNAKE_SMALL,
    [SPECIES_DUDUNSPARCE] = CRY_SNAKE_SMALL,
    [SPECIES_DUDUNSPARCE_THREE_SEGMENT] = CRY_SNAKE_SMALL,
    [SPECIES_GLIGAR] = CRY_BUG_MEDIUM,
    [SPECIES_GLISCOR] = CRY_BUG_MEDIUM,
    [SPECIES_SNUBBULL] = CRY_DOG_SMALL,
    [SPECIES_GRANBULL] = CRY_DOG_BIG,
    [SPECIES_QWILFISH] = CRY_MOLLUSK_SMALL,
    [SPECIES_SHUCKLE] = CRY_MOLLUSK_SMALL,
    [SPECIES_HERACROSS] = CRY_BUG_BIG,
    [SPECIES_SNEASEL] = CRY_FIGHTING_SMALL,
    [SPECIES_WEAVILE] = CRY_FIGHTING_MEDIUM,
    [SPECIES_TEDDIURSA] = CRY_RODDENT_SMALL,
    [SPECIES_URSARING] = CRY_MONSTER_MEDIUM,
    [SPECIES_URSALUNA] = CRY_MONSTER_BIG,
    [SPECIES_URSALUNA_BLOODMOON] = CRY_MONSTER_BIG,
    [SPECIES_SLUGMA] = CRY_MOLLUSK_SMALL,
    [SPECIES_MAGCARGO] = CRY_MOLLUSK_BIG,
    [SPECIES_SWINUB] = CRY_ELEPHANT_SMALL,
    [SPECIES_PILOSWINE] = CRY_ELEPHANT_MEDIUM,
    [SPECIES_MAMOSWINE] = CRY_ELEPHANT_BIG,
    [SPECIES_CORSOLA] = CRY_MOLLUSK_SMALL,
    [SPECIES_REMORAID] = CRY_FISH_SMALL,
    [SPECIES_OCTILLERY] = CRY_SQUID_BIG,
    [SPECIES_DELIBIRD] = CRY_BIRD_SMALL,
    [SPECIES_MANTYKE] = CRY_MOLLUSK_SMALL,
    [SPECIES_MANTINE] = CRY_MOLLUSK_MEDIUM,
    [SPECIES_SKARMORY] = CRY_BIRD_MEDIUM,
    [SPECIES_HOUNDOUR] = CRY_DOG_MEDIUM,
    [SPECIES_HOUNDOOM] = CRY_DOG_BIG,
    [SPECIES_PHANPY] = CRY_ELEPHANT_SMALL,
    [SPECIES_DONPHAN] = CRY_ELEPHANT_MEDIUM,
    [SPECIES_STANTLER] = CRY_HORSE_SMALL,
    [SPECIES_WYRDEER] = CRY_HORSE_MEDIUM,
    [SPECIES_SMEARGLE] = CRY_PIG_SMALL,
    [SPECIES_MILTANK] = CRY_PIG_MEDIUM,
    [SPECIES_RAIKOU] = CRY_CAT_MEDIUM,
    [SPECIES_ENTEI] = CRY_MONSTER_MEDIUM,
    [SPECIES_SUICUNE] = CRY_PSY_MEDIUM,
    [SPECIES_LARVITAR] = CRY_DINOSAUR_SMALL,
    [SPECIES_PUPITAR] = CRY_MINERAL_BIG,
    [SPECIES_TYRANITAR] = CRY_DINOSAUR_BIG,
    [SPECIES_LUGIA] = CRY_PSY_MEDIUM,
    [SPECIES_HO_OH] = CRY_BIRD_BIG,
    [SPECIES_CELEBI] = CRY_FAIRY_SMALL,
    [SPECIES_TORCHIC] = CRY_CHICKEN,
    [SPECIES_RALTS] = CRY_FAIRY_SMALL,
    [SPECIES_KIRLIA] = CRY_FAIRY_SMALL,
    [SPECIES_GARDEVOIR] = CRY_FAIRY_MEDIUM,
    [SPECIES_GALLADE] = CRY_FIGHTING_MEDIUM,
    [SPECIES_MAWILE] = CRY_FAIRY_SMALL,
    [SPECIES_ARON] = CRY_MINERAL_SMALL,
    [SPECIES_LAIRON] = CRY_MINERAL_MEDIUM,
    [SPECIES_AGGRON] = CRY_MINERAL_BIG,
    [SPECIES_PLUSLE] = CRY_PIKACHU,
    [SPECIES_MINUN] = CRY_PIKACHU,
    [SPECIES_SWABLU] = CRY_EGG_GROUP_FLYING,
    [SPECIES_KYOGRE] = CRY_KYOGRE,
    [SPECIES_GROUDON] = CRY_GROUDON,
    [SPECIES_RAYQUAZA] = CRY_RAYQUAZA,
    [SPECIES_JIRACHI] = CRY_FAIRY_SMALL,
    [SPECIES_PACHIRISU] = CRY_PIKACHU,
    [SPECIES_ROTOM] = CRY_ROTOM,
    [SPECIES_PHIONE] = CRY_MANAPHY,
    [SPECIES_MANAPHY] = CRY_MANAPHY,
    [SPECIES_VICTINI] = CRY_FAIRY_SMALL,
    [SPECIES_LILLIPUP] = CRY_DOG_SMALL,
    [SPECIES_HERDIER] = CRY_DOG_MEDIUM,
    [SPECIES_STOUTLAND] = CRY_DOG_BIG,
    [SPECIES_EMOLGA] = CRY_PIKACHU,
    [SPECIES_FLABEBE] = CRY_FAIRY_SMALL,
    [SPECIES_FLOETTE] = CRY_FAIRY_SMALL,
    [SPECIES_FLORGES] = CRY_FAIRY_SMALL,
    [SPECIES_HAWLUCHA] = CRY_BIRD_MEDIUM,
    [SPECIES_DEDENNE] = CRY_PIKACHU,
    [SPECIES_DIANCIE] = CRY_FAIRY_SMALL,
    [SPECIES_HOOPA] = CRY_HOOPA,
    [SPECIES_TOGEDEMARU] = CRY_PIKACHU,
    [SPECIES_ROOKIDEE] = CRY_BIRD_SMALL,
    [SPECIES_CORVISQUIRE] = CRY_BIRD_MEDIUM,
    [SPECIES_CORVIKNIGHT] = CRY_BIRD_BIG,
    [SPECIES_CORVIKNIGHT_MEGA] = CRY_BIRD_BIG,
    [SPECIES_ROLYCOLY] = CRY_MINERAL_SMALL,
    [SPECIES_CARKOL] = CRY_MINERAL_MEDIUM,
    [SPECIES_COALOSSAL] = CRY_MINERAL_BIG,
    [SPECIES_COALOSSAL_MEGA] = CRY_MINERAL_BIG,
    [SPECIES_APPLIN] = CRY_EGG_GROUP_GRASS,
    [SPECIES_FLAPPLE] = CRY_DRAGON_MEDIUM,
    [SPECIES_APPLETUN] = CRY_DRAGON_MEDIUM,
    [SPECIES_DIPPLIN] = CRY_DRAGON_MEDIUM,
    [SPECIES_HYDRAPPLE] = CRY_DRAGON_BIG,
    [SPECIES_MILCERY] = CRY_FAIRY_SMALL,
    [SPECIES_ALCREMIE] = CRY_FAIRY_MEDIUM,
    [SPECIES_MORPEKO] = CRY_PIKACHU,
    [SPECIES_GROWLITHE_HISUIAN] = CRY_DOG_MEDIUM,
    [SPECIES_ARCANINE_HISUIAN] = CRY_DOG_BIG,
    [SPECIES_RAYQUAZA_MEGA] = CRY_RAYQUAZA,
    [SPECIES_KYOGRE_PRIMAL] = CRY_KYOGRE,
    [SPECIES_GROUDON_PRIMAL] = CRY_GROUDON,
    [SPECIES_RAICHU_ALOLAN] = CRY_PIKACHU,
    [SPECIES_GEODUDE_ALOLAN] = CRY_MINERAL_SMALL,
    [SPECIES_GRAVELER_ALOLAN] = CRY_MINERAL_MEDIUM,
    [SPECIES_GOLEM_ALOLAN] = CRY_MINERAL_BIG,
    [SPECIES_PICHU_SPIKY_EARED] = CRY_PIKACHU,
    [SPECIES_ROTOM_HEAT] = CRY_ROTOM,
    [SPECIES_ROTOM_WASH] = CRY_ROTOM,
    [SPECIES_ROTOM_FROST] = CRY_ROTOM,
    [SPECIES_ROTOM_FAN] = CRY_ROTOM,
    [SPECIES_ROTOM_MOW] = CRY_ROTOM,
    [SPECIES_HOOPA_UNBOUND] = CRY_HOOPA,
    [SPECIES_MORPEKO_HANGRY] = CRY_PIKACHU,
    [SPECIES_TYPHLOSION_MEGA] = CRY_MONSTER_MEDIUM,
};


//Cry Tables Definitions
#define TONE_DIRECTSOUND    0x20
#define TONE_DIRECTSOUND_2  0x30
#define TONE_UNCOMPRESSED   0x0
#define TONE_UNCOMPRESSED_2 0x10

#define POKEMON_CRY(sample)         {TONE_DIRECTSOUND,    60, 0, 0, (struct WaveData*)&sample, 0xFF, 0, 0xFF, 0}
#define POKEMON_CRY_2(sample)       {TONE_DIRECTSOUND_2,  60, 0, 0, (struct WaveData*)&sample, 0xFF, 0, 0xFF, 0}
#define POKEMON_CRY_NOT(sample)     {TONE_UNCOMPRESSED,   60, 0, 0, (struct WaveData*)&sample, 0xFF, 0, 0xFF, 0}
#define POKEMON_CRY_NOT_2(sample)   {TONE_UNCOMPRESSED_2, 60, 0, 0, (struct WaveData*)&sample, 0xFF, 0, 0xFF, 0}

#define CRY_DEFINE(cryName)         ALIGNED(4) const s8 cryName[]
//#define CRY_DEFINE_2(species)     INCBIN_S8("sound/direct_sound_samples/cries/species.bin");

#define CRY_MON_SIZE_SMALL  0
#define CRY_MON_SIZE_MEDIUM 1
#define CRY_MON_SIZE_BIG    2

//Sound Data
CRY_DEFINE(Cry_Generic)                = INCBIN_S8("sound/direct_sound_samples/cries/rhydon.bin");
CRY_DEFINE(Cry_Kyogre)                 = INCBIN_S8("sound/direct_sound_samples/cries/kyogre.bin");
CRY_DEFINE(Cry_Groudon)                = INCBIN_S8("sound/direct_sound_samples/cries/groudon.bin");
CRY_DEFINE(Cry_Rayquaza)               = INCBIN_S8("sound/direct_sound_samples/cries/rayquaza.bin");
CRY_DEFINE(Cry_Manaphy)                = INCBIN_S8("sound/direct_sound_samples/cries/manaphy.bin");
CRY_DEFINE(Cry_Rotom)                  = INCBIN_S8("sound/direct_sound_samples/cries/rotom.bin");
CRY_DEFINE(Cry_Hoopa)                  = INCBIN_S8("sound/direct_sound_samples/cries/uncomp_hoopa.bin");
CRY_DEFINE(Cry_Pikachu)                = INCBIN_S8("sound/direct_sound_samples/cries/pikachu.bin");
//Fairy
CRY_DEFINE(Cry_Fairy_Small)            = INCBIN_S8("sound/direct_sound_samples/cries/uncomp_flabebe.bin");
CRY_DEFINE(Cry_Fairy_Medium)           = INCBIN_S8("sound/direct_sound_samples/cries/uncomp_floette.bin");
CRY_DEFINE(Cry_Fairy_Strong)           = INCBIN_S8("sound/direct_sound_samples/cries/uncomp_florges.bin");
//Fish
CRY_DEFINE(Cry_Fish_Small)             = INCBIN_S8("sound/direct_sound_samples/cries/goldeen.bin");
CRY_DEFINE(Cry_Fish_Medium)            = INCBIN_S8("sound/direct_sound_samples/cries/seaking.bin");
CRY_DEFINE(Cry_Fish_Strong)            = INCBIN_S8("sound/direct_sound_samples/cries/wailord.bin");
//Mineral
CRY_DEFINE(Cry_Mineral_Small)          = INCBIN_S8("sound/direct_sound_samples/cries/roggenrola.bin");
CRY_DEFINE(Cry_Mineral_Medium)         = INCBIN_S8("sound/direct_sound_samples/cries/boldore.bin");
CRY_DEFINE(Cry_Mineral_Strong)         = INCBIN_S8("sound/direct_sound_samples/cries/gigalith.bin");
//Dragon
CRY_DEFINE(Cry_Dragon_Small)           = INCBIN_S8("sound/direct_sound_samples/cries/bagon.bin");
CRY_DEFINE(Cry_Dragon_Medium)          = INCBIN_S8("sound/direct_sound_samples/cries/shelgon.bin");
CRY_DEFINE(Cry_Dragon_Strong)          = INCBIN_S8("sound/direct_sound_samples/cries/salamence.bin");
//Bird
CRY_DEFINE(Cry_Bird_Small)             = INCBIN_S8("sound/direct_sound_samples/cries/swablu.bin");
CRY_DEFINE(Cry_Bird_Medium)            = INCBIN_S8("sound/direct_sound_samples/cries/pidgeotto.bin");
CRY_DEFINE(Cry_Bird_Strong)            = INCBIN_S8("sound/direct_sound_samples/cries/pidgeot.bin");
//Turtle
CRY_DEFINE(Cry_Turtle_Small)           = INCBIN_S8("sound/direct_sound_samples/cries/squirtle.bin");
CRY_DEFINE(Cry_Turtle_Medium)          = INCBIN_S8("sound/direct_sound_samples/cries/wartortle.bin");
CRY_DEFINE(Cry_Turtle_Strong)          = INCBIN_S8("sound/direct_sound_samples/cries/blastoise.bin");
//Bug
CRY_DEFINE(Cry_Bug_Small)              = INCBIN_S8("sound/direct_sound_samples/cries/venonat.bin");
CRY_DEFINE(Cry_Bug_Medium)             = INCBIN_S8("sound/direct_sound_samples/cries/galvantula.bin");
CRY_DEFINE(Cry_Bug_Strong)             = INCBIN_S8("sound/direct_sound_samples/cries/scyther.bin");
//Grass
CRY_DEFINE(Cry_Grass_Small)            = INCBIN_S8("sound/direct_sound_samples/cries/oddish.bin");
CRY_DEFINE(Cry_Grass_Medium)           = INCBIN_S8("sound/direct_sound_samples/cries/tangela.bin");
CRY_DEFINE(Cry_Grass_Strong)           = INCBIN_S8("sound/direct_sound_samples/cries/tangrowth.bin");
//Monster
CRY_DEFINE(Cry_Monster_Small)          = INCBIN_S8("sound/direct_sound_samples/cries/trapinch.bin");
CRY_DEFINE(Cry_Monster_Medium)         = INCBIN_S8("sound/direct_sound_samples/cries/tropius.bin");
CRY_DEFINE(Cry_Monster_Strong)         = INCBIN_S8("sound/direct_sound_samples/cries/rhyperior.bin");
//Dog
CRY_DEFINE(Cry_Dog_Small)              = INCBIN_S8("sound/direct_sound_samples/cries/lillipup.bin");
CRY_DEFINE(Cry_Dog_Medium)             = INCBIN_S8("sound/direct_sound_samples/cries/herdier.bin");
CRY_DEFINE(Cry_Dog_Strong)             = INCBIN_S8("sound/direct_sound_samples/cries/stoutland.bin");
// Cat
CRY_DEFINE(Cry_Cat_Small)              = INCBIN_S8("sound/direct_sound_samples/cries/meowth.bin");
CRY_DEFINE(Cry_Cat_Medium)             = INCBIN_S8("sound/direct_sound_samples/cries/persian.bin");
CRY_DEFINE(Cry_Cat_Big)                = INCBIN_S8("sound/direct_sound_samples/cries/purrloin.bin");
//Fox
CRY_DEFINE(Cry_Fox_Small)              = INCBIN_S8("sound/direct_sound_samples/cries/vulpix.bin");
CRY_DEFINE(Cry_Fox_Medium)             = INCBIN_S8("sound/direct_sound_samples/cries/ninetales.bin");
CRY_DEFINE(Cry_Fox_Big)                = INCBIN_S8("sound/direct_sound_samples/cries/zoroark.bin");
//Psy
CRY_DEFINE(Cry_Psy_Small)              = INCBIN_S8("sound/direct_sound_samples/cries/abra.bin");
CRY_DEFINE(Cry_Psy_Medium)             = INCBIN_S8("sound/direct_sound_samples/cries/kadabra.bin");
CRY_DEFINE(Cry_Psy_Big)                = INCBIN_S8("sound/direct_sound_samples/cries/alakazam.bin");
//Horse
CRY_DEFINE(Cry_Horse_Small)            = INCBIN_S8("sound/direct_sound_samples/cries/ponyta.bin");
CRY_DEFINE(Cry_Horse_Medium)           = INCBIN_S8("sound/direct_sound_samples/cries/rapidash.bin");
//Duck
CRY_DEFINE(Cry_Duck_Small)             = INCBIN_S8("sound/direct_sound_samples/cries/psyduck.bin");
CRY_DEFINE(Cry_Duck_Medium)            = INCBIN_S8("sound/direct_sound_samples/cries/golduck.bin");
//Duck
CRY_DEFINE(Cry_Pig_Small)              = INCBIN_S8("sound/direct_sound_samples/cries/slowpoke.bin");
CRY_DEFINE(Cry_Pig_Medium)             = INCBIN_S8("sound/direct_sound_samples/cries/slowking.bin");
//Thin
CRY_DEFINE(Cry_Thing_Small)            = INCBIN_S8("sound/direct_sound_samples/cries/grimer.bin");
CRY_DEFINE(Cry_Thing_Medium)           = INCBIN_S8("sound/direct_sound_samples/cries/muk.bin");
// Monkey
CRY_DEFINE(Cry_Monkey_Small)           = INCBIN_S8("sound/direct_sound_samples/cries/aipom.bin");
CRY_DEFINE(Cry_Monkey_Medium)          = INCBIN_S8("sound/direct_sound_samples/cries/mankey.bin");
CRY_DEFINE(Cry_Monkey_Big)             = INCBIN_S8("sound/direct_sound_samples/cries/primeape.bin");
// Fighting
CRY_DEFINE(Cry_Fighting_Small)         = INCBIN_S8("sound/direct_sound_samples/cries/machop.bin");
CRY_DEFINE(Cry_Fighting_Medium)        = INCBIN_S8("sound/direct_sound_samples/cries/machoke.bin");
CRY_DEFINE(Cry_Fighting_Big)           = INCBIN_S8("sound/direct_sound_samples/cries/machamp.bin");
// Rodent
CRY_DEFINE(Cry_Rodent_Small)           = INCBIN_S8("sound/direct_sound_samples/cries/nidoran_m.bin");
CRY_DEFINE(Cry_Rodent_Medium)          = INCBIN_S8("sound/direct_sound_samples/cries/raticate.bin");
CRY_DEFINE(Cry_Rodent_Big)             = INCBIN_S8("sound/direct_sound_samples/cries/sandslash.bin");
// Crab
CRY_DEFINE(Cry_Crab_Small)             = INCBIN_S8("sound/direct_sound_samples/cries/krabby.bin");
CRY_DEFINE(Cry_Crab_Medium)            = INCBIN_S8("sound/direct_sound_samples/cries/kingler.bin");
CRY_DEFINE(Cry_Crab_Big)               = INCBIN_S8("sound/direct_sound_samples/cries/uncomp_crabominable.bin");
// Dinosaur
CRY_DEFINE(Cry_Dinosaur_Small)         = INCBIN_S8("sound/direct_sound_samples/cries/larvitar.bin");
CRY_DEFINE(Cry_Dinosaur_Medium)        = INCBIN_S8("sound/direct_sound_samples/cries/uncomp_tyrunt.bin");
CRY_DEFINE(Cry_Dinosaur_Big)           = INCBIN_S8("sound/direct_sound_samples/cries/uncomp_tyrantrum.bin");
// Bat
CRY_DEFINE(Cry_Bat_Small)              = INCBIN_S8("sound/direct_sound_samples/cries/zubat.bin");
CRY_DEFINE(Cry_Bat_Medium)             = INCBIN_S8("sound/direct_sound_samples/cries/golbat.bin");
CRY_DEFINE(Cry_Bat_Big)                = INCBIN_S8("sound/direct_sound_samples/cries/crobat.bin");
// Snake
CRY_DEFINE(Cry_Snake_Small)            = INCBIN_S8("sound/direct_sound_samples/cries/ekans.bin");
CRY_DEFINE(Cry_Snake_Medium)           = INCBIN_S8("sound/direct_sound_samples/cries/arbok.bin");
CRY_DEFINE(Cry_Snake_Big)              = INCBIN_S8("sound/direct_sound_samples/cries/seviper.bin");
// Frog
CRY_DEFINE(Cry_Frog_Small)             = INCBIN_S8("sound/direct_sound_samples/cries/poliwag.bin");
CRY_DEFINE(Cry_Frog_Medium)            = INCBIN_S8("sound/direct_sound_samples/cries/poliwhirl.bin");
CRY_DEFINE(Cry_Frog_Big)               = INCBIN_S8("sound/direct_sound_samples/cries/poliwrath.bin");
// Flower
CRY_DEFINE(Cry_Flower_Small)           = INCBIN_S8("sound/direct_sound_samples/cries/bellsprout.bin");
CRY_DEFINE(Cry_Flower_Medium)          = INCBIN_S8("sound/direct_sound_samples/cries/petilil.bin");
CRY_DEFINE(Cry_Flower_Big)             = INCBIN_S8("sound/direct_sound_samples/cries/lilligant.bin");
// Squid
CRY_DEFINE(Cry_Squid_Small)            = INCBIN_S8("sound/direct_sound_samples/cries/tentacool.bin");
CRY_DEFINE(Cry_Squid_Medium)           = INCBIN_S8("sound/direct_sound_samples/cries/tentacool.bin");
CRY_DEFINE(Cry_Squid_Big)              = INCBIN_S8("sound/direct_sound_samples/cries/octillery.bin");
// Elephant
CRY_DEFINE(Cry_Elephant_Small)         = INCBIN_S8("sound/direct_sound_samples/cries/phanpy.bin");
CRY_DEFINE(Cry_Elephant_Medium)        = INCBIN_S8("sound/direct_sound_samples/cries/donphan.bin");
CRY_DEFINE(Cry_Elephant_Big)           = INCBIN_S8("sound/direct_sound_samples/cries/mamoswine.bin");
// Mollusk
CRY_DEFINE(Cry_Mollusk_Small)          = INCBIN_S8("sound/direct_sound_samples/cries/shellos.bin");
CRY_DEFINE(Cry_Mollusk_Medium)         = INCBIN_S8("sound/direct_sound_samples/cries/gastrodon.bin");
CRY_DEFINE(Cry_Mollusk_Big)            = INCBIN_S8("sound/direct_sound_samples/cries/magcargo.bin");
// Imp
CRY_DEFINE(Cry_Imp_Small)              = INCBIN_S8("sound/direct_sound_samples/cries/sableye.bin");
CRY_DEFINE(Cry_Imp_Medium)             = INCBIN_S8("sound/direct_sound_samples/cries/golett.bin");
CRY_DEFINE(Cry_Imp_Big)                = INCBIN_S8("sound/direct_sound_samples/cries/golurk.bin");
// Crocodile
CRY_DEFINE(Cry_Crocodile_Small)        = INCBIN_S8("sound/direct_sound_samples/cries/sandile.bin");
CRY_DEFINE(Cry_Crocodile_Medium)       = INCBIN_S8("sound/direct_sound_samples/cries/krokorok.bin");
CRY_DEFINE(Cry_Crocodile_Big)          = INCBIN_S8("sound/direct_sound_samples/cries/krookodile.bin");
// Lizard
CRY_DEFINE(Cry_Lizard_Small)           = INCBIN_S8("sound/direct_sound_samples/cries/totodile.bin");
CRY_DEFINE(Cry_Lizard_Medium)          = INCBIN_S8("sound/direct_sound_samples/cries/croconaw.bin");
CRY_DEFINE(Cry_Lizard_Big)             = INCBIN_S8("sound/direct_sound_samples/cries/feraligatr.bin");

CRY_DEFINE(Cry_Baby)                   = INCBIN_S8("sound/direct_sound_samples/cries/cleffa.bin");
CRY_DEFINE(Cry_Butterfly)              = INCBIN_S8("sound/direct_sound_samples/cries/beautifly.bin");
CRY_DEFINE(Cry_Mole)                   = INCBIN_S8("sound/direct_sound_samples/cries/diglett.bin");
CRY_DEFINE(Cry_Chicken)                = INCBIN_S8("sound/direct_sound_samples/cries/torchic.bin");

CRY_DEFINE(Cry_Egg_Group_Water_1)      = INCBIN_S8("sound/direct_sound_samples/cries/seel.bin");
CRY_DEFINE(Cry_Egg_Group_Water_2)      = INCBIN_S8("sound/direct_sound_samples/cries/seel.bin");
CRY_DEFINE(Cry_Egg_Group_Water_3)      = INCBIN_S8("sound/direct_sound_samples/cries/seel.bin");
CRY_DEFINE(Cry_Egg_Group_Flying)       = INCBIN_S8("sound/direct_sound_samples/cries/swablu.bin");
CRY_DEFINE(Cry_Egg_Group_Monster)      = INCBIN_S8("sound/direct_sound_samples/cries/tropius.bin");
CRY_DEFINE(Cry_Egg_Group_Grass)        = INCBIN_S8("sound/direct_sound_samples/cries/oddish.bin");
CRY_DEFINE(Cry_Egg_Group_Bug)          = INCBIN_S8("sound/direct_sound_samples/cries/venomoth.bin");
CRY_DEFINE(Cry_Egg_Group_Mineral)      = INCBIN_S8("sound/direct_sound_samples/cries/geodude.bin");
CRY_DEFINE(Cry_Egg_Group_Field)        = INCBIN_S8("sound/direct_sound_samples/cries/aipom.bin");
CRY_DEFINE(Cry_Egg_Group_Human_Like)   = INCBIN_S8("sound/direct_sound_samples/cries/meditite.bin");
CRY_DEFINE(Cry_Egg_Group_Amorphous)    = INCBIN_S8("sound/direct_sound_samples/cries/koffing.bin");
CRY_DEFINE(Cry_Egg_Group_Fairy)        = INCBIN_S8("sound/direct_sound_samples/cries/skitty.bin");
CRY_DEFINE(Cry_Egg_Group_Ditto)        = INCBIN_S8("sound/direct_sound_samples/cries/ditto.bin");
CRY_DEFINE(Cry_Egg_Group_Dragon)       = INCBIN_S8("sound/direct_sound_samples/cries/salamence.bin");

const struct ToneData gCryTable[NUM_CRY_TYPES] = {
    [CRY_GENERIC]               = POKEMON_CRY(Cry_Generic),
    [CRY_KYOGRE]                = POKEMON_CRY(Cry_Kyogre),
    [CRY_GROUDON]               = POKEMON_CRY(Cry_Groudon),
    [CRY_RAYQUAZA]              = POKEMON_CRY(Cry_Rayquaza),
    [CRY_MANAPHY]               = POKEMON_CRY_NOT(Cry_Manaphy),
    [CRY_HOOPA]                 = POKEMON_CRY_NOT(Cry_Hoopa),
    [CRY_PIKACHU]               = POKEMON_CRY(Cry_Pikachu),
    [CRY_ROTOM]                 = POKEMON_CRY_NOT(Cry_Rotom),
    [CRY_RHYHORN]               = POKEMON_CRY_NOT(Cry_Generic),

    [CRY_FAIRY_SMALL]           = POKEMON_CRY_NOT(Cry_Fairy_Small),
    [CRY_FAIRY_MEDIUM]          = POKEMON_CRY_NOT(Cry_Fairy_Medium),
    [CRY_FAIRY_BIG]             = POKEMON_CRY_NOT(Cry_Fairy_Strong),

    [CRY_FISH_SMALL]            = POKEMON_CRY(Cry_Fish_Small),
    [CRY_FISH_MEDIUM]           = POKEMON_CRY(Cry_Fish_Medium),
    [CRY_FISH_BIG]              = POKEMON_CRY(Cry_Fish_Strong),

    [CRY_MINERAL_SMALL]         = POKEMON_CRY_NOT(Cry_Mineral_Small),
    [CRY_MINERAL_MEDIUM]        = POKEMON_CRY_NOT(Cry_Mineral_Medium),
    [CRY_MINERAL_BIG]           = POKEMON_CRY_NOT(Cry_Mineral_Strong),

    [CRY_DRAGON_SMALL]          = POKEMON_CRY(Cry_Dragon_Small),
    [CRY_DRAGON_MEDIUM]         = POKEMON_CRY(Cry_Dragon_Medium),
    [CRY_DRAGON_BIG]            = POKEMON_CRY(Cry_Dragon_Strong),

    [CRY_BIRD_SMALL]            = POKEMON_CRY(Cry_Bird_Small),
    [CRY_BIRD_MEDIUM]           = POKEMON_CRY(Cry_Bird_Medium),
    [CRY_BIRD_BIG]              = POKEMON_CRY(Cry_Bird_Strong),

    [CRY_TURTLE_SMALL]          = POKEMON_CRY(Cry_Turtle_Small),
    [CRY_TURTLE_MEDIUM]         = POKEMON_CRY(Cry_Turtle_Medium),
    [CRY_TURTLE_BIG]            = POKEMON_CRY(Cry_Turtle_Strong),

    [CRY_BUG_SMALL]             = POKEMON_CRY(Cry_Bug_Small),
    [CRY_BUG_MEDIUM]            = POKEMON_CRY_NOT(Cry_Bug_Medium),
    [CRY_BUG_BIG]               = POKEMON_CRY(Cry_Bug_Strong),

    [CRY_GRASS_SMALL]           = POKEMON_CRY(Cry_Grass_Small),
    [CRY_GRASS_MEDIUM]          = POKEMON_CRY(Cry_Grass_Medium),
    [CRY_GRASS_BIG]             = POKEMON_CRY_NOT(Cry_Grass_Strong),

    [CRY_MONSTER_SMALL]         = POKEMON_CRY(Cry_Monster_Small),
    [CRY_MONSTER_MEDIUM]        = POKEMON_CRY(Cry_Monster_Medium),
    [CRY_MONSTER_BIG]           = POKEMON_CRY_NOT(Cry_Monster_Strong),

    [CRY_DOG_SMALL]             = POKEMON_CRY_NOT(Cry_Dog_Small),
    [CRY_DOG_MEDIUM]            = POKEMON_CRY_NOT(Cry_Dog_Medium),
    [CRY_DOG_BIG]               = POKEMON_CRY_NOT(Cry_Dog_Strong),

    [CRY_CAT_SMALL]             = POKEMON_CRY(Cry_Cat_Small),
    [CRY_CAT_MEDIUM]            = POKEMON_CRY(Cry_Cat_Medium),
    [CRY_CAT_BIG]               = POKEMON_CRY_NOT(Cry_Cat_Big),

    [CRY_FOX_SMALL]             = POKEMON_CRY(Cry_Fox_Small),
    [CRY_FOX_MEDIUM]            = POKEMON_CRY(Cry_Fox_Medium),
    [CRY_FOX_BIG]               = POKEMON_CRY_NOT(Cry_Fox_Big),

    [CRY_PSY_SMALL]             = POKEMON_CRY(Cry_Psy_Small),
    [CRY_PSY_MEDIUM]            = POKEMON_CRY(Cry_Psy_Medium),
    [CRY_PSY_BIG]               = POKEMON_CRY(Cry_Psy_Big),

    [CRY_HORSE_SMALL]           = POKEMON_CRY(Cry_Horse_Small),
    [CRY_HORSE_MEDIUM]          = POKEMON_CRY(Cry_Horse_Medium),

    [CRY_DUCK_SMALL]            = POKEMON_CRY(Cry_Duck_Small),
    [CRY_DUCK_MEDIUM]           = POKEMON_CRY(Cry_Duck_Medium),

    [CRY_PIG_SMALL]             = POKEMON_CRY(Cry_Pig_Small),
    [CRY_PIG_MEDIUM]            = POKEMON_CRY(Cry_Pig_Medium),

    [CRY_THING_SMALL]           = POKEMON_CRY(Cry_Thing_Small),
    [CRY_THING_MEDIUM]          = POKEMON_CRY(Cry_Thing_Medium),

    [CRY_BABY]                  = POKEMON_CRY(Cry_Baby),
    [CRY_BUTTERFLY]             = POKEMON_CRY(Cry_Butterfly),
    [CRY_MOLE]                  = POKEMON_CRY(Cry_Mole),
    [CRY_CHICKEN]               = POKEMON_CRY(Cry_Chicken),

    // Monkey
    [CRY_MONKEY_SMALL]          = POKEMON_CRY(Cry_Monkey_Small),
    [CRY_MONKEY_MEDIUM]         = POKEMON_CRY(Cry_Monkey_Medium),
    [CRY_MONKEY_BIG]            = POKEMON_CRY(Cry_Monkey_Big),

    // Fighting
    [CRY_FIGHTING_SMALL]        = POKEMON_CRY(Cry_Fighting_Small),
    [CRY_FIGHTING_MEDIUM]       = POKEMON_CRY(Cry_Fighting_Medium),
    [CRY_FIGHTING_BIG]          = POKEMON_CRY(Cry_Fighting_Big),

    // Rodent
    [CRY_RODDENT_SMALL]         = POKEMON_CRY(Cry_Rodent_Small),
    [CRY_RODDENT_MEDIUM]        = POKEMON_CRY(Cry_Rodent_Medium),
    [CRY_RODDENT_BIG]           = POKEMON_CRY_NOT(Cry_Rodent_Big),

    // Crab
    [CRY_CRAB_SMALL]            = POKEMON_CRY(Cry_Crab_Small),
    [CRY_CRAB_MEDIUM]           = POKEMON_CRY(Cry_Crab_Medium),
    [CRY_CRAB_BIG]              = POKEMON_CRY_NOT(Cry_Crab_Big),

    // Dinosaur
    [CRY_DINOSAUR_SMALL]        = POKEMON_CRY(Cry_Dinosaur_Small),
    [CRY_DINOSAUR_MEDIUM]       = POKEMON_CRY_NOT(Cry_Dinosaur_Medium),
    [CRY_DINOSAUR_BIG]          = POKEMON_CRY_NOT(Cry_Dinosaur_Big),

    // Bat
    [CRY_BAT_SMALL]             = POKEMON_CRY(Cry_Bat_Small),
    [CRY_BAT_MEDIUM]            = POKEMON_CRY(Cry_Bat_Medium),
    [CRY_BAT_BIG]               = POKEMON_CRY(Cry_Bat_Big),

    // Snake
    [CRY_SNAKE_SMALL]           = POKEMON_CRY(Cry_Snake_Small),
    [CRY_SNAKE_MEDIUM]          = POKEMON_CRY(Cry_Snake_Medium),
    [CRY_SNAKE_BIG]             = POKEMON_CRY(Cry_Snake_Big),

    // Frog
    [CRY_FROG_SMALL]            = POKEMON_CRY(Cry_Frog_Small),
    [CRY_FROG_MEDIUM]           = POKEMON_CRY(Cry_Frog_Medium),
    [CRY_FROG_BIG]              = POKEMON_CRY(Cry_Frog_Big),

    // Flower
    [CRY_FLOWER_SMALL]          = POKEMON_CRY(Cry_Flower_Small),
    [CRY_FLOWER_MEDIUM]         = POKEMON_CRY_NOT(Cry_Flower_Medium),
    [CRY_FLOWER_BIG]            = POKEMON_CRY_NOT(Cry_Flower_Big),

    // Squid
    [CRY_SQUID_SMALL]           = POKEMON_CRY(Cry_Squid_Small),
    [CRY_SQUID_MEDIUM]          = POKEMON_CRY(Cry_Squid_Medium),
    [CRY_SQUID_BIG]             = POKEMON_CRY(Cry_Squid_Big),

    // Elephant
    [CRY_ELEPHANT_SMALL]        = POKEMON_CRY(Cry_Elephant_Small),
    [CRY_ELEPHANT_MEDIUM]       = POKEMON_CRY(Cry_Elephant_Medium),
    [CRY_ELEPHANT_BIG]          = POKEMON_CRY_NOT(Cry_Elephant_Big),

    // Mollusk
    [CRY_MOLLUSK_SMALL]         = POKEMON_CRY_NOT(Cry_Mollusk_Small),
    [CRY_MOLLUSK_MEDIUM]        = POKEMON_CRY_NOT(Cry_Mollusk_Medium),
    [CRY_MOLLUSK_BIG]           = POKEMON_CRY(Cry_Mollusk_Big),

    // Imp
    [CRY_IMP_SMALL]             = POKEMON_CRY(Cry_Imp_Small),
    [CRY_IMP_MEDIUM]            = POKEMON_CRY_NOT(Cry_Imp_Medium),
    [CRY_IMP_BIG]               = POKEMON_CRY_NOT(Cry_Imp_Big),

    // Crocodile
    [CRY_CROCODILE_SMALL]       = POKEMON_CRY_NOT(Cry_Crocodile_Small),
    [CRY_CROCODILE_MEDIUM]      = POKEMON_CRY_NOT(Cry_Crocodile_Medium),
    [CRY_CROCODILE_BIG]         = POKEMON_CRY_NOT(Cry_Crocodile_Big),

    // Lizard
    [CRY_LIZARD_SMALL]          = POKEMON_CRY(Cry_Lizard_Small),
    [CRY_LIZARD_MEDIUM]         = POKEMON_CRY(Cry_Lizard_Medium),
    [CRY_LIZARD_BIG]            = POKEMON_CRY(Cry_Lizard_Big),

    [CRY_EGG_GROUP_WATER_1]    = POKEMON_CRY(Cry_Egg_Group_Water_1),
    [CRY_EGG_GROUP_WATER_2]    = POKEMON_CRY(Cry_Egg_Group_Water_2),
    [CRY_EGG_GROUP_WATER_3]    = POKEMON_CRY(Cry_Egg_Group_Water_3),
    [CRY_EGG_GROUP_DRAGON]     = POKEMON_CRY(Cry_Egg_Group_Flying),
    [CRY_EGG_GROUP_FLYING]     = POKEMON_CRY(Cry_Egg_Group_Flying),
    [CRY_EGG_GROUP_MONSTER]    = POKEMON_CRY(Cry_Egg_Group_Monster),
    [CRY_EGG_GROUP_GRASS]      = POKEMON_CRY(Cry_Egg_Group_Grass),
    [CRY_EGG_GROUP_BUG]        = POKEMON_CRY(Cry_Egg_Group_Bug),
    [CRY_EGG_GROUP_MINERAL]    = POKEMON_CRY(Cry_Egg_Group_Mineral),
    [CRY_EGG_GROUP_FIELD]      = POKEMON_CRY(Cry_Egg_Group_Field),
    [CRY_EGG_GROUP_HUMAN_LIKE] = POKEMON_CRY(Cry_Egg_Group_Human_Like),
    [CRY_EGG_GROUP_AMORPHOUS]  = POKEMON_CRY(Cry_Egg_Group_Amorphous),
    [CRY_EGG_GROUP_FAIRY]      = POKEMON_CRY(Cry_Egg_Group_Fairy),
    [CRY_EGG_GROUP_DITTO]      = POKEMON_CRY(Cry_Egg_Group_Ditto),
};

const struct ToneData gCryTable2[NUM_CRY_TYPES] = {
    [CRY_GENERIC]              = POKEMON_CRY(Cry_Generic),
    [CRY_KYOGRE]               = POKEMON_CRY(Cry_Kyogre),
    [CRY_GROUDON]              = POKEMON_CRY(Cry_Groudon),
    [CRY_RAYQUAZA]             = POKEMON_CRY(Cry_Rayquaza),
    [CRY_MANAPHY]              = POKEMON_CRY_NOT(Cry_Manaphy),
    [CRY_HOOPA]                = POKEMON_CRY_NOT(Cry_Hoopa),
    [CRY_PIKACHU]              = POKEMON_CRY(Cry_Pikachu),
    [CRY_ROTOM]                = POKEMON_CRY_NOT(Cry_Rotom),

    [CRY_FAIRY_SMALL]          = POKEMON_CRY_NOT(Cry_Fairy_Small),
    [CRY_FAIRY_MEDIUM]         = POKEMON_CRY_NOT(Cry_Fairy_Medium),
    [CRY_FAIRY_BIG]            = POKEMON_CRY_NOT(Cry_Fairy_Strong),

    [CRY_FISH_SMALL]           = POKEMON_CRY(Cry_Fish_Small),
    [CRY_FISH_MEDIUM]          = POKEMON_CRY(Cry_Fish_Medium),
    [CRY_FISH_BIG]             = POKEMON_CRY(Cry_Fish_Strong),

    [CRY_MINERAL_SMALL]        = POKEMON_CRY(Cry_Mineral_Small),
    [CRY_MINERAL_MEDIUM]       = POKEMON_CRY(Cry_Mineral_Medium),
    [CRY_MINERAL_BIG]          = POKEMON_CRY(Cry_Mineral_Strong),

    [CRY_DRAGON_SMALL]         = POKEMON_CRY(Cry_Dragon_Small),
    [CRY_DRAGON_MEDIUM]        = POKEMON_CRY(Cry_Dragon_Medium),
    [CRY_DRAGON_BIG]           = POKEMON_CRY(Cry_Dragon_Strong),

    [CRY_BIRD_SMALL]           = POKEMON_CRY(Cry_Bird_Small),
    [CRY_BIRD_MEDIUM]          = POKEMON_CRY(Cry_Bird_Medium),
    [CRY_BIRD_BIG]             = POKEMON_CRY(Cry_Bird_Strong),

    [CRY_TURTLE_SMALL]         = POKEMON_CRY(Cry_Turtle_Small),
    [CRY_TURTLE_MEDIUM]        = POKEMON_CRY(Cry_Turtle_Medium),
    [CRY_TURTLE_BIG]           = POKEMON_CRY(Cry_Turtle_Strong),

    [CRY_BUG_SMALL]            = POKEMON_CRY(Cry_Bug_Small),
    [CRY_BUG_MEDIUM]           = POKEMON_CRY_NOT(Cry_Bug_Medium),
    [CRY_BUG_BIG]              = POKEMON_CRY(Cry_Bug_Strong),

    [CRY_GRASS_SMALL]          = POKEMON_CRY(Cry_Grass_Small),
    [CRY_GRASS_MEDIUM]         = POKEMON_CRY(Cry_Grass_Medium),
    [CRY_GRASS_BIG]            = POKEMON_CRY_NOT(Cry_Grass_Strong),

    [CRY_MONSTER_SMALL]        = POKEMON_CRY_NOT(Cry_Monster_Small),
    [CRY_MONSTER_MEDIUM]       = POKEMON_CRY(Cry_Monster_Medium),
    [CRY_MONSTER_BIG]          = POKEMON_CRY_NOT(Cry_Monster_Strong),

    [CRY_EGG_GROUP_WATER_1]    = POKEMON_CRY(Cry_Egg_Group_Water_1),
    [CRY_EGG_GROUP_WATER_2]    = POKEMON_CRY(Cry_Egg_Group_Water_2),
    [CRY_EGG_GROUP_WATER_3]    = POKEMON_CRY(Cry_Egg_Group_Water_3),
    [CRY_EGG_GROUP_DRAGON]     = POKEMON_CRY(Cry_Egg_Group_Flying),
    [CRY_EGG_GROUP_FLYING]     = POKEMON_CRY(Cry_Egg_Group_Flying),
    [CRY_EGG_GROUP_MONSTER]    = POKEMON_CRY(Cry_Egg_Group_Monster),
    [CRY_EGG_GROUP_GRASS]      = POKEMON_CRY(Cry_Egg_Group_Grass),
    [CRY_EGG_GROUP_BUG]        = POKEMON_CRY(Cry_Egg_Group_Bug),
    [CRY_EGG_GROUP_MINERAL]    = POKEMON_CRY(Cry_Egg_Group_Mineral),
    [CRY_EGG_GROUP_FIELD]      = POKEMON_CRY(Cry_Egg_Group_Field),
    [CRY_EGG_GROUP_HUMAN_LIKE] = POKEMON_CRY(Cry_Egg_Group_Human_Like),
    [CRY_EGG_GROUP_AMORPHOUS]  = POKEMON_CRY(Cry_Egg_Group_Amorphous),
    [CRY_EGG_GROUP_FAIRY]      = POKEMON_CRY(Cry_Egg_Group_Fairy),
    [CRY_EGG_GROUP_DITTO]      = POKEMON_CRY(Cry_Egg_Group_Ditto),
};

static bool8 CanEvolve(u16 species)
{
    u32 i;

    for (i = 0; i < EVOS_PER_MON; i++)
    {
        if (gEvolutionTable[species][i].method && gEvolutionTable[species][i].method != EVO_MEGA_EVOLUTION)
            return TRUE;
    }
    return FALSE;
}

#define POKEMON_STAGES_FIND 3
static SpeciesEnum GetFirstStageFromSpecies(SpeciesEnum species)
{
    //We need to define fist stages for every Pokémon to make things faster
    u16 i, j, k;
    bool8 found;
    u32 numChecks = 0;

    for (i = 0; i < POKEMON_STAGES_FIND; i++)
    {
        found = FALSE;
        for (j = 1; j < NUM_SPECIES; j++)
        {
            if(!isSpeciesPlaceholderMon(j)){
                for (k = 0; k < POKEMON_STAGES_FIND; k++)
                {
                    if (gEvolutionTable[j][k].targetSpecies == species)
                    {
                        species = j;
                        found = TRUE;
                        break;
                    }
                    numChecks++;
                }
            }

            if (found)
                break;
        }

        if (j == NUM_SPECIES)
            break;
    }

    //MGBA_PRINT_DEBUG("GetFirstStageFromSpecies species: %d numChecks: %d", species, numChecks)

    return species;
}

static u8 GetSpeciesEvolutionStage(u16 species, u16 firstStage){
    u16 middleStage = gEvolutionTable[firstStage][0].targetSpecies;

    switch(species){
        case SPECIES_VILEPLUME:
        case SPECIES_BELLOSSOM:
        case SPECIES_POLIWRATH:
        case SPECIES_POLITOED:
        case SPECIES_BEAUTIFLY:
        case SPECIES_DUSTOX:
        case SPECIES_GARDEVOIR:
        case SPECIES_GALLADE:
        case SPECIES_HYDRAPPLE:
            return 2;
        break;
        case SPECIES_SLOWBRO:
        case SPECIES_SLOWBRO_GALARIAN:
        case SPECIES_SLOWKING:
        case SPECIES_SLOWKING_GALARIAN:
        case SPECIES_KLEAVOR:
        case SPECIES_HITMONLEE:
        case SPECIES_HITMONCHAN:
        case SPECIES_HITMONTOP:
        case SPECIES_NINJASK:
        case SPECIES_SHEDINJA:
        case SPECIES_GLALIE:
        case SPECIES_FROSLASS:
        case SPECIES_GOREBYSS:
        case SPECIES_WORMADAM:
        case SPECIES_WORMADAM_SANDY_CLOAK:
        case SPECIES_WORMADAM_TRASH_CLOAK:
        case SPECIES_MOTHIM:
        case SPECIES_FLAPPLE:
        case SPECIES_APPLETUN:
        case SPECIES_DIPPLIN:
        case SPECIES_ARMAROUGE:
        case SPECIES_CERULEDGE:
            return 1;
        break;
        default:
            if(firstStage == SPECIES_EEVEE && firstStage != species)
                return 1;
        break;
    }

    //Is First Stage
    if(firstStage == species)
        return 0;
    else if(middleStage == species)
        return 1;
    else
        return 2;
}

#define POKEMON_STAGE_BABY_MON       0 //Ex: Pichu, Igglybuff
#define POKEMON_STAGE_FIRST_STAGE    1 //Ex: Bulbasaur, Charmander
#define POKEMON_STAGE_SECOND_STAGE   2 //Ex: Gloom, Weepinbell
#define POKEMON_STAGE_THIRD_STAGE    3 //Ex: Blaziken, Swampert
#define POKEMON_STAGE_SINGLE_STAGE   4 //Ex: Skarmory, Spinda
#define POKEMON_STAGE_MEGA           5 //Ex: Mega Mawile, Mega Blaziken
#define POKEMON_STAGE_LEGENDARY      6 //Ex: Deoxys, Jirachi
#define POKEMON_STAGE_MEGA_LEGENDARY 7 //Ex: Mega Rayquaza, Mega Latias

static u8 getSpeciesCategory(SpeciesEnum species){
    u16 firstStage = GetFirstStageFromSpecies(species);
    bool8 isFirstStage = (firstStage == species);
    bool8 canEvolve = CanEvolve(species);
    bool8 canBreed = (gBaseStats[species].eggGroup1 != EGG_GROUP_UNDISCOVERED);
    bool8 isMegaEvolution = FALSE; //IsMegaEvolution(species);
    bool8 isLegendary = FALSE; //IsLegendary(species);
    u8 evolutionStages = GetSpeciesEvolutionStage(species, firstStage);

    if (isFirstStage && canEvolve) {
        if (!canBreed)
            return POKEMON_STAGE_BABY_MON;
        else
            return POKEMON_STAGE_FIRST_STAGE;
    }
    else if (canEvolve && !isFirstStage) {
        return POKEMON_STAGE_SECOND_STAGE;
    }
    else if (!canEvolve && firstStage != species && evolutionStages == 2) {
        return POKEMON_STAGE_THIRD_STAGE;
    }
    else if (isFirstStage && !canEvolve && canBreed) {
        return POKEMON_STAGE_SINGLE_STAGE;
    }
    else if (isMegaEvolution) {
        if (isLegendary)
            return POKEMON_STAGE_MEGA_LEGENDARY;
        else
            return POKEMON_STAGE_MEGA;
    }
    else if (isLegendary) {
        return POKEMON_STAGE_LEGENDARY;
    }

    return POKEMON_STAGE_SINGLE_STAGE;
}

static u8 getCrySize(SpeciesEnum species){
    u8 speciesCategory = getSpeciesCategory(species);

    switch(speciesCategory){
        case POKEMON_STAGE_BABY_MON:
        case POKEMON_STAGE_FIRST_STAGE:
            return CRY_MON_SIZE_SMALL;
        break;
        case POKEMON_STAGE_SECOND_STAGE:
        case POKEMON_STAGE_SINGLE_STAGE:
            return CRY_MON_SIZE_MEDIUM;
        break;
        case POKEMON_STAGE_THIRD_STAGE:
        case POKEMON_STAGE_MEGA:
        case POKEMON_STAGE_LEGENDARY:
        case POKEMON_STAGE_MEGA_LEGENDARY:
            return CRY_MON_SIZE_BIG;
        break;
    }

    return CRY_MON_SIZE_SMALL;
}

u16 GetSpeciesCry(SpeciesEnum species, bool32 v0){
    u16 speciesCry = sSpeciesCryNum[species];
    u8 monSize;
    
    if(speciesCry == CRY_GENERIC){
        monSize = getCrySize(species);

        if(gBaseStats[species].eggGroup2 == EGG_GROUP_WATER_1)
            return CRY_FISH_SMALL + monSize;
        else if(gBaseStats[species].eggGroup2 == EGG_GROUP_WATER_2)
            return CRY_FISH_SMALL + monSize;
        else if(gBaseStats[species].eggGroup2 == EGG_GROUP_WATER_2)
            return CRY_FISH_SMALL + monSize;
        else if(gBaseStats[species].eggGroup2 == EGG_GROUP_DRAGON)
            return CRY_DRAGON_SMALL + monSize;
        else if(gBaseStats[species].eggGroup2 == EGG_GROUP_FLYING)
            return CRY_BIRD_SMALL + monSize;
        else if(gBaseStats[species].eggGroup2 == EGG_GROUP_MONSTER)
            return CRY_MONSTER_SMALL + monSize;
        else if(gBaseStats[species].eggGroup1 == EGG_GROUP_GRASS)
            return CRY_EGG_GROUP_GRASS;
        else if(gBaseStats[species].eggGroup2 == EGG_GROUP_BUG || gBaseStats[species].eggGroup1 == EGG_GROUP_BUG)
            return CRY_BUG_SMALL + monSize;
        else if(gBaseStats[species].eggGroup2 == EGG_GROUP_MINERAL)
            return CRY_MINERAL_SMALL + monSize;
        else if(gBaseStats[species].eggGroup2 == EGG_GROUP_FIELD || gBaseStats[species].eggGroup1 == EGG_GROUP_FIELD)
            return CRY_EGG_GROUP_FIELD;
        else if(gBaseStats[species].eggGroup2 == EGG_GROUP_HUMAN_LIKE)
            return CRY_EGG_GROUP_HUMAN_LIKE;
        else if(gBaseStats[species].eggGroup2 == EGG_GROUP_AMORPHOUS)
            return CRY_EGG_GROUP_AMORPHOUS;
        else if(gBaseStats[species].eggGroup2 == EGG_GROUP_FAIRY)
            return CRY_FAIRY_SMALL + monSize;
        else if(gBaseStats[species].eggGroup2 == EGG_GROUP_DITTO)
            return CRY_EGG_GROUP_DITTO;
        else if(gBaseStats[species].eggGroup2 == EGG_GROUP_GRASS)
            return CRY_GRASS_SMALL + monSize;
    }

    return speciesCry;
}