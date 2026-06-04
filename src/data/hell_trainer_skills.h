//This table is used for hell mode trainer skill definitions for each trainer, you can also give them extra abilities for their whole team to avoid having to code a lot of new abilities
const u16 sTrainerSkillList[TRAINERS_COUNT][MAX_HELL_TRAINERS_GYM_SKILLS][BATTLE_EVENT_ARGUMENTS] = {
    [TRAINER_BRENDAN_ROUTE_103_MUDKIP] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EVIOLITE, 0, 0},
    },
    [TRAINER_BRENDAN_ROUTE_103_TORCHIC] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EVIOLITE, 0, 0},
    },
    [TRAINER_BRENDAN_ROUTE_103_TREECKO] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EVIOLITE, 0, 0},
    },
    [TRAINER_MAY_ROUTE_103_MUDKIP] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EVIOLITE, 0, 0},
    },
    [TRAINER_MAY_ROUTE_103_TORCHIC] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EVIOLITE, 0, 0},
    },
    [TRAINER_MAY_ROUTE_103_TREECKO] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EVIOLITE, 0, 0},
    },
    [TRAINER_CALVIN_1] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_NO_PROTECT, 0, 0},
        [HELL_MODE_TRAINER_SKILL_2] = {BATTLE_EVENT_EXTRA_ABILITIES_1, ABILITY_UNNERVE, 0},
        [HELL_MODE_TRAINER_SKILL_3] = {BATTLE_EVENT_LAST_STAND_STAGES, 5, 2},
    },
    [TRAINER_ALLEN] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_SPIKES, 1, 0},
    },
    [TRAINER_BILLY] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EXTRA_ABILITIES_1, ABILITY_UNNERVE, 0},
    },
    [TRAINER_DARIAN] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EXTRA_ABILITIES_1, ABILITY_INNARDS_OUT, 0},
    },
    [TRAINER_CINDY_1] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EXTRA_ABILITIES_1, ABILITY_CUTE_CHARM, 0},
    },
    [TRAINER_RICK] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_PERMA_STICKY_WEB, 0, 0},
    },
    [TRAINER_LYLE] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EXTRA_ABILITIES_1, ABILITY_SWORD_OF_RUIN, 0},
    },
    [TRAINER_WINSTON_1] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EXTRA_ABILITIES_1, ABILITY_CHEAP_TACTICS, 0},
    },
    [TRAINER_JAMES_1] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_TOXIC_SPIKES, 0, 0}, //Always sets 2 layers
    },
    [TRAINER_HALEY_1] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EXTRA_ABILITIES_1, ABILITY_SHELL_ARMOR, 0},
    },
    [TRAINER_IVAN] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EXTRA_ABILITIES_1, ABILITY_SUPER_LUCK, 0},
    },
    [TRAINER_GRUNT_PETALBURG_WOODS] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EXTRA_ABILITIES_1, ABILITY_FILTER, 0},
    },
    [TRAINER_TIANA] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_ONSTAY_LEECH_SEED, 0, 0},
    },
    [TRAINER_JOSH] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_POSTURE_DEFENSE, 4, 0},
    },
    [TRAINER_TOMMY] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_STEALTH_ROCK, 0, 0},
        [HELL_MODE_TRAINER_SKILL_2] = {BATTLE_EVENT_LAST_BLEED, 2, 0}, // Last bleed does not affect the first Pokémon
    },
    [TRAINER_GINA_AND_MIA_1] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EXTRA_ABILITIES_1, ABILITY_FLASH_FIRE, 0},
    },
    [TRAINER_MARC] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_LAST_STAND_STAGES, 5, 2},
    },
    [TRAINER_ROXANNE_1] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EXTRA_ABILITIES_1, ABILITY_PRESSURE, 0},
        [HELL_MODE_TRAINER_SKILL_2] = {BATTLE_EVENT_EXTRA_ABILITIES_2, ABILITY_SHED_SKIN, 0},
        [HELL_MODE_TRAINER_SKILL_3] = {BATTLE_EVENT_EXTRA_ABILITIES_3, ABILITY_SELF_SUFFICIENT, 0},
    },
    // ==== Route 116 ====
    [TRAINER_JOSE] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EXTRA_ABILITIES_1, ABILITY_SHIELD_DUST, 0},
    },
    [TRAINER_JOEY] = {
        [HELL_MODE_TRAINER_SKILL_2] = {BATTLE_EVENT_FIRST_BURNED, 3, 0},
    },
    [TRAINER_JERRY_1] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_STEADY_SPDEF, 0, 0},
    },
    //[TRAINER_DAWSON] = {
        // TODO: Permanent sun (can be overwritted by player)
    //},
    //[TRAINER_SARAH] = {
        // TODO: Permanent inverse room
    //},
    [TRAINER_CLARK] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EVIOLITE, 0, 0},
    },
    [TRAINER_JANICE] = {
        // Doc says Focus Sash effect, Sturdy probably good enough (only difference is Mold Breaker?)
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EXTRA_ABILITIES_1, ABILITY_STURDY, 0},
    },
    [TRAINER_KAREN_1] = {
        // Not implemented yet
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_STICKY_WEB, 0, 0},
    },
    [TRAINER_JOHNSON] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_RAINBOW, 0, 0},
    },
    //[TRAINER_DEVAN] = {
        // TODO: Permanent sand (can be overwritted by player)
    //},
    [TRAINER_NIDO_MEGAS_ROUTE116_RUSTURF] = {
        // Emperor's Wrath should be the same as Queen's Mourning + King's Wrath
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EXTRA_ABILITIES_1, ABILITY_EMPERORS_WRATH, 0},
        // TODO: All mons use Tickle each turn
        // TODO: AI uses Tailwind after death
    },
    // ==== Rusturf Tunnel ====
    [TRAINER_GRUNT_RUSTURF_TUNNEL] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EXTRA_ABILITIES_1, ABILITY_SMOKEY_MANEUVERS, 0},
    },
    //[TRAINER_MIKE_2] = {
        // TODO: All mons use Power Trip on entry
    //},
    // ==== Rustboro ====
    [TRAINER_BRENDAN_RUSTBORO_TREECKO] = {
        // Doc says Focus Sash effect, Sturdy probably good enough (only difference is Mold Breaker?)
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EXTRA_ABILITIES_1, ABILITY_STURDY, 0},
    },
    [TRAINER_BRENDAN_RUSTBORO_TORCHIC] = {
        // Doc says Focus Sash effect, Sturdy probably good enough (only difference is Mold Breaker?)
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EXTRA_ABILITIES_1, ABILITY_STURDY, 0},
    },
    [TRAINER_BRENDAN_RUSTBORO_MUDKIP] = {
        // Doc says Focus Sash effect, Sturdy probably good enough (only difference is Mold Breaker?)
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EXTRA_ABILITIES_1, ABILITY_STURDY, 0},
    },
    [TRAINER_MAY_RUSTBORO_TREECKO] = {
        // Doc says Focus Sash effect, Sturdy probably good enough (only difference is Mold Breaker?)
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EXTRA_ABILITIES_1, ABILITY_STURDY, 0},
    },
    [TRAINER_MAY_RUSTBORO_TORCHIC] = {
        // Doc says Focus Sash effect, Sturdy probably good enough (only difference is Mold Breaker?)
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EXTRA_ABILITIES_1, ABILITY_STURDY, 0},
    },
    [TRAINER_MAY_RUSTBORO_MUDKIP] = {
        // Doc says Focus Sash effect, Sturdy probably good enough (only difference is Mold Breaker?)
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EXTRA_ABILITIES_1, ABILITY_STURDY, 0},
    },
    // ==== Route 105 ====
    [TRAINER_IMANI] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EXTRA_ABILITIES_1, ABILITY_COMATOSE, 0},
        [HELL_MODE_TRAINER_SKILL_2] = {BATTLE_EVENT_EXTRA_ABILITIES_2, ABILITY_SWEET_DREAMS, 0},
        // TODO: Inflicts sleep when hit by attack (assume only for AI?)
    },
    [TRAINER_DOMINIK] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EXTRA_ABILITIES_1, ABILITY_WEATHER_DOUBLE_BOOST, 0},
        [HELL_MODE_TRAINER_SKILL_2] = {BATTLE_EVENT_EXTRA_ABILITIES_2, ABILITY_SUN_WORSHIP, 0},
        // TODO: Permanent sun (not sure if changeable by player)
    },
    [TRAINER_FOSTER] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EXTRA_ABILITIES_1, ABILITY_ACCELERATE, 0},
        [HELL_MODE_TRAINER_SKILL_2] = {BATTLE_EVENT_EXTRA_ABILITIES_2, ABILITY_ANALYTIC, 0},
        [HELL_MODE_TRAINER_SKILL_3] = {BATTLE_EVENT_LAST_STAND_STAGES, 5, 1},
    },
    //[TRAINER_BEVERLY] = {
        // TODO: Power of Water moves is doubled (all or just opponent?)
        // TODO: Player's abilities are disabled after 2 turns on field
        // TODO: Speed increased by one stage when falling under half HP
    //},
    [TRAINER_JOSUE] = {
        // Forced Tag Battle with Ruin Maniac Andres
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_RAINBOW, 0, 0},
        // TODO: Stat changes are shared between allies (not Sharing and Caring ability since that's all mons, not just allies)
        // TODO: Reduce ally damage by 50% (pre-nerf Friend Guard)
    },
    [TRAINER_LUIS] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EXTRA_ABILITIES_1, ABILITY_KNOW_YOUR_PLACE, 0},
        // TODO: Power of Fighting moves is doubled (all or just opponent?)
        // TODO: Last mon sent out gets +4 Def and Sp. Def
    },
    //[TRAINER_ANDRES_1] = {
        // Forced Tag Battle with Bird Keeper Josue
        // TODO: Pebble Shower used after attacking
        // TODO: Ancient Power used after every turn
        // TODO: Speed increases by +2 stages when hit by contact move
    //},
    [TRAINER_FERALIGATRITEX_ROUTE105] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_SWAMP, 0, 0},
        // TODO: Supereffective moves deal 50% less damage
        // TODO: Recovers 50% HP on KO
    },
    // ==== ROUTE 106 ====
    //[TRAINER_ELLIOT_1] = {
        // TODO: Every AI mon uses Mean Look on entry
    //},
    //[TRAINER_NED] = {
        // TODO: Rain
    //},
    [TRAINER_DOUGLAS] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EXTRA_ABILITIES_1, ABILITY_STICKY_HOLD, 0},
        // TODO: AI uses recycle at end of turn
    },
    [TRAINER_KAYLA] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EXTRA_ABILITIES_1, ABILITY_MYSTIC_POWER, 0},
        // TODO: Hazards cannot be removed by player
    },
    // ==== GRANITE CAVE ====
    [TRAINER_STEVEN_GRANITE_CAVE] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EXTRA_ABILITIES_1, ABILITY_PRIMORDIAL_SEA, 0},
    },
    // TODO: Steven Stone Post Brawly?
    // TODO: Black Belt Hitmonstone
    // ==== ROUTE 107 ====
    [TRAINER_DENISE] = {
        // Doc says passive leftovers, this is same except it can be suppressed
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EXTRA_ABILITIES_1, ABILITY_SELF_SUFFICIENT, 0},
        // TODO: Every turn 50% chance to use Trick/Wonder/Magic/Inverse Room. Check 50% roll for each individually.
    },
    //[TRAINER_TONY_1] = {
        // TODO: Uses Night Shade after every attack
        // TODO: Permanent rain
        // TODO: AI uses Water Pledge every 2 turns automatically
    //},
    //[TRAINER_BETH] = {
        // TODO: Permanent Mist
        // TODO: AI mons use Magnet Rise on entry
        // TODO: Passive Expert Belt
    //},
    //[TRAINER_CAMRON] = {
        // TODO: AI has Quick Claw effect on every move
        // TODO: STAB moves are flagged as status moves?
        // TODO: Player takes 1/18th (should this be 1/16th?) damage every turn
    //},
    //[TRAINER_DARRIN] = {
        // TODO: AI uses Refresh at end of every turn
        // TODO: All mons get Toxic poisoned at start of every turn
        // TODO: Player mons get Heal Block on entry
    //},
    //[TRAINER_LISA_AND_RAY] = {
        // TODO: AI uses Scratch after player uses stat boosting move
        // TODO: Every pokemon on field copies Dance moves
        // TODO: Player has permanent Ring Target effect
    //},
    // ==== ROUTE 108 ====
    [TRAINER_MATTHEW] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EXTRA_ABILITIES_1, ABILITY_COMATOSE, 0},
        // TODO: Suppress player abilities
        // TODO: Permanent tailwind
    },
    //[TRAINER_TARA] = {
        // TODO: Player's mons inflicted with Salt Cure and Curse
        // TODO: Player's accurate moves (95%-100% acc) have accuracy lowered by 25% (-25% acc or acc * 0.75?)
        // TODO: Passive Torment effect
    //},
    //[TRAINER_CAROLINA] = {
        // TODO: If AI mons have no items, attacks deal x1.5 damage
        // TODO: When AI mons switch out, they get item back
        // TODO: If AI consumes item, speed x2
    //},
    //[TRAINER_JEROME] = {
        // TODO: Permanent electric terrain
        // TODO: Permanent Primordial Sea
        // TODO: Electric type moves deal supereffective damage to Ground Types
    //},
    //[TRAINER_MISSY] = {
        // TODO: AI stat raises are doubled
        // TODO: AI stat drops are immediately reset to 0
        // TODO: AI hits lowest defense
    //},
    [TRAINER_CORY_1] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EXTRA_ABILITIES_1, ABILITY_IMMUNITY, 0},
        [HELL_MODE_TRAINER_SKILL_2] = {BATTLE_EVENT_SWAMP, 0, 0},
        // TODO: Red Card on hit for AI (once per AI mon)
    },
    // ==== ROUTE 108 ABANDONED SHIP ====
    [TRAINER_THALIA_1] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EXTRA_ABILITIES_1, ABILITY_PURE_LOVE, 0},
        [HELL_MODE_TRAINER_SKILL_2] = {BATTLE_EVENT_EXTRA_ABILITIES_2, ABILITY_PIXIE_POWER, 0},
        // TODO: Opponent uses Sweet Kiss when attacked
    },
    [TRAINER_DEMETRIUS] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EXTRA_ABILITIES_1, ABILITY_ANGELS_WRATH, 0},
        // TODO: Opponents deal x1.25 damage
        // TODO: Opponents hit lowest defense vs statused foes
        // TODO: Opponent's status moves ignore abilities
    },
    [TRAINER_DUNCAN] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EXTRA_ABILITIES_1, ABILITY_POISON_HEAL, 0},
        // TODO: Both trainers' teams start Badly Poisoned
        // TODO: Opponent uses Anchor Shot if player tries to switch out
    },
    [TRAINER_CHARLIE] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EXTRA_ABILITIES_1, ABILITY_COSMIC_DAZE, 0},
        // TODO: Opponent uses Whirlwind every other turn
        // TODO: Opponent uses Confuse Ray when player switches in
    },
    [TRAINER_GARRISON] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EXTRA_ABILITIES_1, ABILITY_BONE_ZONE, 0},
        // TODO: Every move opponent uses is a Bone move
        // TODO: 20% chance to flinch Player
        // TODO: Opponent's Speed raises on KO
    },
    [TRAINER_JANI] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EXTRA_ABILITIES_1, ABILITY_RESTRAINING_ORDER, 0},
        [HELL_MODE_TRAINER_SKILL_2] = {BATTLE_EVENT_EXTRA_ABILITIES_2, ABILITY_LEVITATE, 0},
        // TODO: Use 50BP Explosion/Outburst when fainting
        // TODO: Ups Def and Sp. Def one stage if hit by Fire or Flying moves
    },
    [TRAINER_KIRA_AND_DAN_1] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_FIRE_SEA, 0, 0},
        // TODO: Opponent attacks with Burning Jealousy every 2 turns
        // TODO: Opponent attacks with Rage Fist upon fainting (targets mon that fainted it)
    },
    [TRAINER_SHEDINJITE_ABANDONED_SHIP] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EXTRA_ABILITIES_1, ABILITY_TERA_SHELL, 0},
        // TODO: Opponent's mons enter with a substitute
        // TODO: Passive Heavy Duty Boots effect
    },
    // ==== ROUTE 109 ====
    [TRAINER_LOLA_1] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EXTRA_ABILITIES_1, ABILITY_COMATOSE, 0},
    },
    [TRAINER_CHANDLER] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_STEALTH_ROCK, 0, 0},
    },
    [TRAINER_ELIJAH] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EXTRA_ABILITIES_1, ABILITY_MOUNTAINEER, 0},
    },
    //[TRAINER_CARTER] = {
        // TODO: Opponents use sub on switch in
    //},
    //[TRAINER_GWEN] = {
        // TODO: Sun
    //},
    //[TRAINER_EDMOND] = {
        // TODO: Use 10 BP Bubble on switch in
    //},
    //[TRAINER_HUEY] = {
        // TODO: Huey's team inflicted with frostbite
    //},
    //[TRAINER_HAILEY] = {
        // TODO: Passive Focus Band effect for all of opponent's mons, with activation chance at 30% instead of 10%
    //},
    //[TRAINER_RICKY_1] = {
        // TODO: Soaks all player mons?
    //},
    [TRAINER_MEL_AND_PAUL] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EXTRA_ABILITIES_1, ABILITY_VOLT_ABSORB, 0},
        [HELL_MODE_TRAINER_SKILL_2] = {BATTLE_EVENT_EXTRA_ABILITIES_2, ABILITY_WATER_ABSORB, 0},
    },
    //[TRAINER_ALICE] = {
        // TODO: Opponent has passive Leech Seed effect on player
        // TODO: Passive Leftover effect
    //},
    [TRAINER_DAVID] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EXTRA_ABILITIES_1, ABILITY_REGENERATOR, 0},
        // TODO: Opponent uses Teleport every 2 turns
    },
    // ==== ROUTE 109 SEAHOUSE ====
    //[TRAINER_SIMON] = {
        // TODO: Perma Mud Sport (doc says old effect? not sure which one)
    //},
    [TRAINER_JOHANNA] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EXTRA_ABILITIES_1, ABILITY_CUTE_CHARM, 0},
    },
    [TRAINER_DWAYNE] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EXTRA_ABILITIES_1, ABILITY_MERCILESS, 0},
    },
    // ==== DEWFORD MANOR ====
    [TRAINER_CROBATITE_DEWFORD_MANOR] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EXTRA_ABILITIES_1, ABILITY_TRICKSTER, 0},
        // TODO: Moves use Speed for damage calculation
        // TODO: Player moves are Normalized
    },
    // ==== DEWFORD GYM ====
    //[TRAINER_TAKAO] = {
        // TODO: All mons in fight are frostbitten
    //},
    //[TRAINER_LAURA] = {
        // TODO: Iron Fist and Striker flagged moves get additional x1.5
    //},
    //[TRAINER_CRISTIAN] = {
        // TODO: Increase highest stat by +2 at end of turn
    //},
    [TRAINER_JOCELYN] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EXTRA_ABILITIES_1, ABILITY_SCRAPPY, 0},
    },
    //[TRAINER_BRENDEN] = {
        // TODO: Perma rain
    //},
    //[TRAINER_LILITH] = {
        // TODO: Buff ally's mon's Sp. Def by 25%
    //},
    [TRAINER_BRAWLY_1] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EXTRA_ABILITIES_1, ABILITY_OLE, 0},
        // TODO: Priority moves fail if used by player (player can select another move after selecting a priority move though)
        // TODO: Passive Brightpowder
    },
    // ==== SLATEPORT CITY ====
    //[TRAINER_GRETA_SLATEPORT] = {
        // TODO: Player cannot switch by ANY means
        // TODO: Player's active Pokemon faints if it does not get a KO in 3 turns
        // TODO: Player Pokemon get sent out in slot order (for example: Pokemon in slot 2 always sent out after slot 1 faints)
    //},
    // ==== SLATEPORT MUSEUM ====
    //[TRAINER_GRUNT_MUSEUM_1] = {
        // TODO: Sets caltrops on player's side
    //},
    //[TRAINER_GRUNT_MUSEUM_2] = {
        // TODO: Permanent hail
    //},
    //[TRAINER_ARCHIE_SLATEPORT] = {
        // TODO: Permanent rain
    //},
    // ==== ROUTE 103 ====
    [TRAINER_ISABELLE] = {
        // Test Primordial Sea (might be a little weird constantly activating/deactivating when Trainer Pokemon faint/switch?)
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EXTRA_ABILITIES_1, ABILITY_PRIMORDIAL_SEA, 0},
        [HELL_MODE_TRAINER_SKILL_2] = {BATTLE_EVENT_SWAMP, 0, 0},
        // TODO: Rooms don't work
    },
    [TRAINER_PETE] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EXTRA_ABILITIES_1, ABILITY_PRIMORDIAL_SEA, 0},
        [HELL_MODE_TRAINER_SKILL_2] = {BATTLE_EVENT_EXTRA_ABILITIES_2, ABILITY_SWIFT_SWIM, 0},
        [HELL_MODE_TRAINER_SKILL_3] = {BATTLE_EVENT_EXTRA_ABILITIES_3, ABILITY_SAP_SIPPER, 0},
    },
    [TRAINER_RHETT] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EXTRA_ABILITIES_1, ABILITY_LONG_REACH, 0},
    },
    //[TRAINER_MARCOS] = {
        // TODO: Soundproof does not work
    //},
    //[TRAINER_AMY_AND_LIV_1] = {
        // TODO: Life Dew used on switch on
    //},
    //[TRAINER_ANDREW] = {
        // TODO: Permanent Rain
    //},
    //[TRAINER_DAISY] = {
        // TODO: Passive Lax Incense (Bright Powder effect)
    //},
    [TRAINER_MIGUEL_1] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EXTRA_ABILITIES_1, ABILITY_PRETTY_PRINCESS, 0},
    },
    // ==== ALTERING CAVE ====
    //[TRAINER_LEAF_ALTERING_CAVE] = {
        // TODO: The player can only use mons from Kanto (excluding forms)
        // TODO: Both sides use Sp. Def for dealing/taking special damage
        // TODO: Suppress player's abilities
    //},
    // ==== ROUTE 110 ====
    [TRAINER_ISABEL_1] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EXTRA_ABILITIES_1, ABILITY_FRIEND_GUARD, 0},
    },
    //[TRAINER_KALEB] = {
        // TODO: Permanent Quash
    //},
    //[TRAINER_TIMMY] = {
        // TODO: One layer of Spikes on entry
        // TODO: Moves heal 200% of damage dealt (only healing moves or all?)
    //},
    //[TRAINER_DALE] = {
        // TODO: Electric immunity
        // TODO: Switch in with Helping Hand boost
    //},
    [TRAINER_JOSEPH] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EXTRA_ABILITIES_1, ABILITY_MOLD_BREAKER, 0},
    },
    [TRAINER_ALYSSA] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EXTRA_ABILITIES_1, ABILITY_SPEED_BOOST, 0},
    },
    //[TRAINER_EDWIN_1] = {
        // TODO: Trainer mons ignore player's immunities
    //},
    //[TRAINER_EDWARD] = {
        // TODO: Trick room
    //},
    //[TRAINER_BRENDAN_ROUTE_110_TREECKO] = {
        // TODO: Enemy team benefits from fog effects
        // TODO: Permanent fog
    //},
    //[TRAINER_BRENDAN_ROUTE_110_TORCHIC] = {
        // TODO: Enemy team benefits from fog effects
        // TODO: Permanent fog
    //},
    //[TRAINER_BRENDAN_ROUTE_110_MUDKIP] = {
        // TODO: Enemy team benefits from fog effects
        // TODO: Permanent fog
    //},
    //[TRAINER_MAY_ROUTE_110_TREECKO] = {
        // TODO: Enemy team benefits from fog effects
        // TODO: Permanent fog
    //},
    //[TRAINER_MAY_ROUTE_110_TORCHIC] = {
        // TODO: Enemy team benefits from fog effects
        // TODO: Permanent fog
    //},
    //[TRAINER_MAY_ROUTE_110_MUDKIP] = {
        // TODO: Enemy team benefits from fog effects
        // TODO: Permanent fog
    //},
    [TRAINER_JACLYN] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EXTRA_ABILITIES_1, ABILITY_GIFTED_MIND, 0},
    },
    //[TRAINER_ABIGAIL_1] = {
        // TODO: Boost attack by 50% and speed by 20% on entry for AI's mons
    //},
    //[TRAINER_BENJAMIN_1] = {
        // TODO: Hadron Engine but permanent electric terrain
    //},
    [TRAINER_JACOB] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EXTRA_ABILITIES_1, ABILITY_GUARD_DOG, 0},
    },
    //[TRAINER_ANTHONY] = {
        // TODO: Permanent Misty Terrain
    //},
    //[TRAINER_JASMINE] = {
        // TODO: +2 Speed on entry for AI's mons and Speed cannot be reduced
    //},
    // ==== MAUVILLE CITY ====
    //[TRAINER_WALLY_MAUVILLE] = {
        // TODO: Passive Ability Shield for AI's mons
        // TODO: Changes player's abilities to Lethargy (blockable by Ability Shield)
    //},
    // ==== MAUVILLE GAME CORNER ====
    //[TRAINER_SINNOH_MEGAS_MAUVILLE_GAME_CORNER] = {
        // TODO: Enemies are immune to stat drops
        // TODO: Enemies gain +1 to best attack stat on entry
        // TODO: Enemy moves can never miss
        // TODO: Immune to rooms
    //},
    // ==== ROUTE 111 South ====
    //[TRAINER_BIANCA] = {
        // TODO: Player's mons get insta-KOed (!!) by critical hits
    //},
    //[TRAINER_CELINA] = {
        // TODO: Player's moves have a 30% chance to miss
    //},
    //[TRAINER_HAYDEN] = {
        // TODO: Permanent sun
    //},
    //[TRAINER_TYRON] = {
        // TODO: Flower Gift ability, but always only buffs the user (won't buff allies in double battles)
    //},
    [TRAINER_VICTOR] = {
        // Self Sufficient in place of passive leftovers (for now maybe)
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EXTRA_ABILITIES_1, ABILITY_SELF_SUFFICIENT, 0},
        [HELL_MODE_TRAINER_SKILL_2] = {BATTLE_EVENT_EXTRA_ABILITIES_2, ABILITY_INNER_FOCUS, 0},
        // TODO: Permanent sandstorm
    },
    [TRAINER_VICTORIA] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EXTRA_ABILITIES_1, ABILITY_GRASS_PELT, 0},
        [HELL_MODE_TRAINER_SKILL_2] = {BATTLE_EVENT_EXTRA_ABILITIES_2, ABILITY_FLOURISH, 0},
        [HELL_MODE_TRAINER_SKILL_3] = {BATTLE_EVENT_EXTRA_ABILITIES_2, ABILITY_SELF_SUFFICIENT, 0},
    },
    [TRAINER_VIVI] = {
        [HELL_MODE_TRAINER_SKILL_2] = {BATTLE_EVENT_EXTRA_ABILITIES_2, ABILITY_BIOFILM, 0},
        [HELL_MODE_TRAINER_SKILL_3] = {BATTLE_EVENT_EXTRA_ABILITIES_2, ABILITY_POISON_ABSORB, 0},
        // TODO: Permanent Toxic Terrain
    },
    //[TRAINER_VICKY] = {
        // TODO: Suppress player's abilities
        // TODO: Toxic Spikes (on entry I assume?)
        // TODO: Spikes (on entry I assume?)
    //},
    // ==== ROUTE 118 West ====
    [TRAINER_DEANDRE] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EXTRA_ABILITIES_1, ABILITY_GALLANTRY, 0},
    },
    //[TRAINER_WADE] = {
        // TODO: Rain sets Aurora Veil
    //},
    //[TRAINER_DALTON_1] = {
        // TODO: Sound moves always crit
    //},
    //[TRAINER_ROSE_1] = {
        // TODO: Lowers player's highest attacking stat each turn
    //},
    // ==== ROUTE 117 ====
    [TRAINER_DEREK] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EXTRA_ABILITIES_1, ABILITY_TINTED_LENS, 0},
    },
    [TRAINER_AISHA] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EXTRA_ABILITIES_1, ABILITY_HYPER_AGGRESSIVE, 0},
    },
    [TRAINER_ANNA_AND_MEG_1] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EXTRA_ABILITIES_1, ABILITY_FRIEND_GUARD, 0},
    },
    [TRAINER_DYLAN_1] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EXTRA_ABILITIES_1, ABILITY_MOMENTUM, 0},
    },
    [TRAINER_MARIA_1] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EXTRA_ABILITIES_1, ABILITY_IMPULSE, 0},
    },
    [TRAINER_ISAAC_1] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EXTRA_ABILITIES_1, ABILITY_HUBRIS, 0},
    },
    [TRAINER_LYDIA_1] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EXTRA_ABILITIES_1, ABILITY_MOXIE, 0},
    },
    [TRAINER_BRANDI] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EXTRA_ABILITIES_1, ABILITY_PSYCHIC_MIND, 0},
    },
    //[TRAINER_MELINA] = {
        // TODO: Tailwind on entry (permanent?)
    //},
    // ==== VERDANTURF MEADOWS ====
    [TRAINER_MEGANIUMITE_VERDANTURF_MEADOW] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EXTRA_ABILITIES_1, ABILITY_TRIAGE, 0},
        [HELL_MODE_TRAINER_SKILL_2] = {BATTLE_EVENT_EXTRA_ABILITIES_2, ABILITY_DAZZLING, 0},
        // TODO: Permanent Safeguard
    },
    // ==== MAUVILLE GYM ====
    // Permanent electric terrain for all Mauville Gym battles?
    //[TRAINER_VIVIAN] = {
        // TODO: Flourish ability but for Electric Terrain 
    //},
    [TRAINER_BEN] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EXTRA_ABILITIES_1, ABILITY_SURGE_SURFER, 0},
    },
    [TRAINER_KIRK] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EXTRA_ABILITIES_1, ABILITY_TRANSISTOR, 0},
    },
    [TRAINER_SHAWN] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EXTRA_ABILITIES_1, ABILITY_FRIEND_GUARD, 0},
    },
    [TRAINER_ANGELO] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EXTRA_ABILITIES_1, ABILITY_POWER_SPOT, 0},
    },
    [TRAINER_WATTSON_1] = {
        [HELL_MODE_TRAINER_SKILL_1] = {BATTLE_EVENT_EXTRA_ABILITIES_1, ABILITY_SURGE_SURFER, 0},
        // TODO: Player's entire party is paralyzed
        // TODO: Ignores rooms
    },
};
