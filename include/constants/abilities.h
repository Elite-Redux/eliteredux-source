#ifndef GUARD_CONSTANTS_ABILITIES_H
#define GUARD_CONSTANTS_ABILITIES_H

#define ABILITY_NONE 0 // Empty ability slot.
#define ABILITY_STENCH 1 // Attacks have a 10% chance to cause enemy to flinch.
#define ABILITY_DRIZZLE 2 // Summons rain on entry. Lasts 8 turns.
#define ABILITY_SPEED_BOOST 3 // Raises own Speed by one stage after every turn.
#define ABILITY_BATTLE_ARMOR 4 // Immune to critical hits. Takes 20% less damage from all attacks.
#define ABILITY_STURDY 5 // At full HP, cannot be KO in one hit, stays at 1 HP instead.
#define ABILITY_DAMP 6 // Makes foe Water-type on contact. Also works on offense.
#define ABILITY_LIMBER 7 // Immune to paralysis. Takes 50% less recoil damage.
#define ABILITY_SAND_VEIL 8 // Evasion is boosted by 1.25x while a sandstorm is active.
#define ABILITY_STATIC 9 // 30% chance to paralyze on contact. Also works on offense.
#define ABILITY_VOLT_ABSORB 10 // Heals 25% of max HP when hit by an Electric-type move.
#define ABILITY_WATER_ABSORB 11 // Heals 25% of max HP when hit by a Water-type move.
#define ABILITY_OBLIVIOUS 12 // Immune to infatuation, Scare, Intimidate and Taunt.
#define ABILITY_CLOUD_NINE 13 // Clears weather and prevents its effects.
#define ABILITY_COMPOUND_EYES 14 // Grants a 1.3x accuracy boost.
#define ABILITY_INSOMNIA 15 // Cannot fall asleep. Rest fails if used.
#define ABILITY_COLOR_CHANGE 16 // Changes type to a resist or an immunity before getting hit.
#define ABILITY_IMMUNITY 17 // Cannot be poisoned. Halves damage taken from Poison moves.
#define ABILITY_FLASH_FIRE 18 // Powers up Fire-type moves by 1.5x if hit by a Fire-type move.
#define ABILITY_SHIELD_DUST 19 // Immune to added move effects and all entry hazards.
#define ABILITY_OWN_TEMPO 20 // Immune to confusion, Intimidate and Scare.
#define ABILITY_SUCTION_CUPS 21 // Cannot be forced to switch out by an enemy's move.
#define ABILITY_INTIMIDATE 22 // Lowers foes' Atk by one stage on entry.
#define ABILITY_SHADOW_TAG 23 // Opponents can't be switched out. Ghosts aren't affected.
#define ABILITY_ROUGH_SKIN 24 // Enemies lose 1/8 of max HP if they use a contact move.
#define ABILITY_WONDER_GUARD 25 // Is only hit by Super-effective attacks or indirect damage.
#define ABILITY_LEVITATE 26 // Immune to Ground-type moves. Ups own Flying moves by 1.25x.
#define ABILITY_EFFECT_SPORE 27 // 30% chance to inflict SLP, PARA or PSN if hit by a contact move.
#define ABILITY_SYNCHRONIZE 28 // Enemies inflicting status on this Pokémon get same status.
#define ABILITY_CLEAR_BODY 29 // Immune to stat drops.
#define ABILITY_NATURAL_CURE 30 // Heals status condition upon switching out.
#define ABILITY_LIGHTNING_ROD 31 // Redirects Electric moves. Absorbs them, ups highest Atk.
#define ABILITY_SERENE_GRACE 32 // Doubles chance of secondary effects on its own moves.
#define ABILITY_SWIFT_SWIM 33 // This Pokémon's Speed gets a 1.5x boost if rain is active.
#define ABILITY_CHLOROPHYLL 34 // This Pokémon's Speed gets a 1.5x boost if sun is active.
#define ABILITY_ILLUMINATE 35 // Grants a 1.2x accuracy boost.
#define ABILITY_TRACE 36 // Copies the foe's ability. Does not copy innates.
#define ABILITY_HUGE_POWER 37 // Doubles own Attack stat. Boosts raw stat, not base stat.
#define ABILITY_POISON_POINT 38 // 30% chance to poison on contact. Also works on offense.
#define ABILITY_INNER_FOCUS 39 // Blocks flinch, Intimidate, Scare. Focus Blast never misses.
#define ABILITY_MAGMA_ARMOR 40 // Frostbite-immune. Takes 30% less dmg from Water/Ice-type moves.
#define ABILITY_WATER_VEIL 41 // Burn-immune. Casts Aqua Ring on entry.
#define ABILITY_MAGNET_PULL 42 // Traps opposing Steel-types. Ghosts aren't affected.
#define ABILITY_SOUNDPROOF 43 // Immune to sound-based moves.
#define ABILITY_RAIN_DISH 44 // Heals 1/8 of max HP every turn if rain is active.
#define ABILITY_SAND_STREAM 45 // Summons a sandstorm on entry. Lasts 8 turns.
#define ABILITY_PRESSURE 46 // Doubles foe's PP usage. Clears stat buffs on entry.
#define ABILITY_THICK_FAT 47 // Takes 1/2 damage from Fire-type and Ice-type attacks.
#define ABILITY_EARLY_BIRD 48 // Awakens twice as fast from sleep.
#define ABILITY_FLAME_BODY 49 // 30% chance to burn on contact. Also works on offense.
#define ABILITY_RUN_AWAY 50 // Guarantees fleeing. Raises Speed if stats lowered by an enemy.
#define ABILITY_KEEN_EYE 51 // Immune to accuracy drops. Grants a 1.2x accuracy boost.
#define ABILITY_HYPER_CUTTER 52 // Enemies can't lower Atk/Sp. Atk. Crit rate of contact moves: +1.
#define ABILITY_PICKUP 53 // Removes all hazards on entry. Not immune to hazards.
#define ABILITY_TRUANT 54 // Can only attack every other turn. Can use status moves every turn.
#define ABILITY_HUSTLE 55 // 0.9x accuracy. Raises Atk & SpAtk by 1.4x.
#define ABILITY_CUTE_CHARM 56 // 30% chance to charm attacker on contact, which halves its power.
#define ABILITY_PLUS 57 // Deals double damage if an ally Pokémon has Minus or Plus.
#define ABILITY_MINUS 58 // Deals double damage if an ally Pokémon has Minus or Plus.
#define ABILITY_FORECAST 59 // Changes form with the weather. Weather setting triggers attack.
#define ABILITY_STICKY_HOLD 60 // Can't lose its item.
#define ABILITY_SHED_SKIN 61 // 30% chance to heal its status condition at the end of a turn.
#define ABILITY_GUTS 62 // Ups Atk by 1.5x if suffering from a status condition.
#define ABILITY_MARVEL_SCALE 63 // Ups Def by 1.5x if suffering from a status condition.
#define ABILITY_LIQUID_OOZE 64 // Draining causes harm to enemies instead of healing them.
#define ABILITY_OVERGROW 65 // Boosts Grass-type moves by 1.2x, or 1.5x when under 1/3 HP.
#define ABILITY_BLAZE 66 // Boosts Fire-type moves by 1.2x, or 1.5x when under 1/3 HP.
#define ABILITY_TORRENT 67 // Boosts Water-type moves by 1.2x, or 1.5x when under 1/3 HP.
#define ABILITY_SWARM 68 // Boosts Bug-type moves by 1.2x, or 1.5x when under 1/3 HP.
#define ABILITY_ROCK_HEAD 69 // Immune to recoil damage, but not immune to Explosion/crash dmg.
#define ABILITY_DROUGHT 70 // Summons sun on entry. Lasts 8 turns.
#define ABILITY_ARENA_TRAP 71 // Enemies can't flee. Ghosts and ungrounded Pokémon are immune.
#define ABILITY_VITAL_SPIRIT 72 // Can't fall asleep. Heals status after using Fighting-type moves.
#define ABILITY_WHITE_SMOKE 73 // Sets Smokescreen on switch-in.
#define ABILITY_PURE_POWER 74 // Doubles own Attack stat. Boosts raw stat, not base stat.
#define ABILITY_SHELL_ARMOR 75 // Immune to critical hits. Takes 20% less damage from all attacks.
#define ABILITY_AIR_LOCK 76 // Clears weather and prevents its effects.

#define ABILITIES_COUNT_GEN3 (ABILITY_AIR_LOCK + 1)

#define ABILITY_TANGLED_FEET 77 // Doubles Evasion when confused.
#define ABILITY_MOTOR_DRIVE 78 // Boosts Speed instead of being hit by Electric-type moves.
#define ABILITY_RIVALRY 79 // Deals 1.25x to same gender. Takes .75x from opposite gender.
#define ABILITY_STEADFAST 80 // Raises Speed by one stage if this Pokémon flinches.
#define ABILITY_SNOW_CLOAK 81 // Evasion is boosted by 1.25x under hail.
#define ABILITY_GLUTTONY 82 // Eats berries early. Berries also restore 1/3 of max HP.
#define ABILITY_ANGER_POINT 83 // Getting hit raises Atk by +1. Critical hits maximize Attack.
#define ABILITY_UNBURDEN 84 // Consuming its held item doubles Speed until switched out.
#define ABILITY_HEATPROOF 85 // Halves damage taken from Fire- type moves. Takes no burn damage.
#define ABILITY_SIMPLE 86 // Doubles all stat changes on this Pokémon.
#define ABILITY_DRY_SKIN 87 // Water/Rain heals. Fire/Sun hurts.
#define ABILITY_DOWNLOAD 88 // Raises Atk/Sp. Atk by one stage depending on opponent.
#define ABILITY_IRON_FIST 89 // Boosts the power of punching moves by 1.3x.
#define ABILITY_POISON_HEAL 90 // Restores 1/8 of max HP after each turn if poisoned.
#define ABILITY_ADAPTABILITY 91 // Increases STAB from 1.5x to 2x.
#define ABILITY_SKILL_LINK 92 // Multi-hit moves always hit the maximum number of times.
#define ABILITY_HYDRATION 93 // Cures own status at the end of every turn in rain.
#define ABILITY_SOLAR_POWER 94 // Ups highest attacking stat by 1.5x in sun.
#define ABILITY_QUICK_FEET 95 // Ups Speed by 1.5x if suffering from a status condition.
#define ABILITY_NORMALIZE 96 // Its moves become Normal-type, get 1.1x boost, ignore resists.
#define ABILITY_SNIPER 97 // Critical hits have a 2.25x dmg multiplier instead of 1.5x.
#define ABILITY_MAGIC_GUARD 98 // Only damaged by attacks.
#define ABILITY_NO_GUARD 99 // Attacks used by and on this Pokémon bypass accuracy checks.
#define ABILITY_STALL 100 // Takes 30% damage if it hasn't moved.
#define ABILITY_TECHNICIAN 101 // Moves with 60 BP or less get a 1.5x boost.
#define ABILITY_LEAF_GUARD 102 // Immune to status conditions if sun is active.
#define ABILITY_KLUTZ 103 // Own held item has no effect. Mega Stones are unaffected.
#define ABILITY_MOLD_BREAKER 104 // Moves hit through abilities. Also affects innates.
#define ABILITY_SUPER_LUCK 105 // Raises critical-hit ratio of own moves by +1.
#define ABILITY_AFTERMATH 106 // If faints by a contact move, attacker takes 25% of max HP.
#define ABILITY_ANTICIPATION 107 // Senses Super-effective moves. Blocks one Super-effective hit.
#define ABILITY_FOREWARN 108 // Casts a 50 BP Future Sight on entry.
#define ABILITY_UNAWARE 109 // Ignores foes' stat changes, both positive and negative ones.
#define ABILITY_TINTED_LENS 110 // Attacks deal double damage if resisted.
#define ABILITY_FILTER 111 // Takes 35% less damage from Super-effective moves.
#define ABILITY_SLOW_START 112 // Halves Attack and Speed during the first 5 turns out.
#define ABILITY_SCRAPPY 113 // Normal/Fighting can hit Ghosts. Immune to Intimidate/Scare.
#define ABILITY_STORM_DRAIN 114 // Redirects Water moves. Absorbs them, ups highest Atk.
#define ABILITY_ICE_BODY 115 // Heals 1/8 of max HP every turn in hail.
#define ABILITY_SOLID_ROCK 116 // Takes 35% less damage from Super-effective moves.
#define ABILITY_SNOW_WARNING 117 // Summons hail on entry. Lasts 8 turns.
#define ABILITY_HONEY_GATHER 118 // Has a 50% chance to find Honey each turn.
#define ABILITY_FRISK 119 // Checks foes' item and disables their items for two turns.
#define ABILITY_RECKLESS 120 // Moves causing recoil damage deal 1.2x more damage.
#define ABILITY_MULTITYPE 121 // Held Plate item decides holder's type.
#define ABILITY_FLOWER_GIFT 122 // Increases the party's SpAtk and SpDef by 1.5x in Sun.
#define ABILITY_BAD_DREAMS 123 // Sleeping Pokémon lose 1/4 of max HP at the end of each turn.

#define ABILITIES_COUNT_GEN4 (ABILITY_BAD_DREAMS + 1)

#define ABILITY_PICKPOCKET 124 // Steals the foe's held item on contact.
#define ABILITY_SHEER_FORCE 125 // Exchanges added effects on its moves for 1.3x more power.
#define ABILITY_CONTRARY 126 // Stat raises turn into stat drops for this Pokémon and vice versa.
#define ABILITY_UNNERVE 127 // Foes can't eat Berries as long as this Pokémon is in battle.
#define ABILITY_DEFIANT 128 // Raises Attack by two stages if stats are lowered by an enemy.
#define ABILITY_DEFEATIST 129 // Halves Atk and Sp. Atk stats if user is below 1/3 of max HP.
#define ABILITY_CURSED_BODY 130 // 30% chance to disable moves if enemy makes contact.
#define ABILITY_HEALER 131 // 30% chance to heal user or ally's status at the end of each turn.
#define ABILITY_FRIEND_GUARD 132 // Reduces damage that ally takes by 50% in double battles.
#define ABILITY_WEAK_ARMOR 133 // If hit by a contact attack: -1 Defense and +2 Speed.
#define ABILITY_HEAVY_METAL 134 // Doubles this Pokémon's weight.
#define ABILITY_LIGHT_METAL 135 // Boosts Speed by 1.3x and halves this Pokémon's weight.
#define ABILITY_MULTISCALE 136 // At full HP, halves damage taken from attacks
#define ABILITY_TOXIC_BOOST 137 // Ups Atk by 1.5x if poisoned. Immune to Poison status damage.
#define ABILITY_FLARE_BOOST 138 // Ups Sp. Atk by 1.5x if burned. Ignites in fog.
#define ABILITY_HARVEST 139 // 50% chance to recycle a used Berry every turn, 100% in sun.
#define ABILITY_TELEPATHY 140 // Can't be damaged by ally attacks.
#define ABILITY_MOODY 141 // Lowers a random stat by -1 and raises another by +2 every turn.
#define ABILITY_OVERCOAT 142 // Blocks weather dmg, powder moves. 20% Special damage reduction.
#define ABILITY_POISON_TOUCH 143 // 30% chance to poison on contact. Also works on offense.
#define ABILITY_REGENERATOR 144 // Heals 1/3 of max HP upon switching out.
#define ABILITY_BIG_PECKS 145 // Boosts the power of contact moves by 1.3x.
#define ABILITY_SAND_RUSH 146 // This Pokémon's Speed gets a 1.5x boost in a sandstorm.
#define ABILITY_WONDER_SKIN 147 // Opposing status moves have their accuracy halved.
#define ABILITY_ANALYTIC 148 // Attacks get a 1.3x power boost if it moves last.
#define ABILITY_ILLUSION 149 // Appears as last party slot and boosts power by 1.3x until hit.
#define ABILITY_IMPOSTER 150 // Transforms into the foe on entry.
#define ABILITY_INFILTRATOR 151 // Own moves bypass Substitutes and damage reduction screens.
#define ABILITY_MUMMY 152 // If hit, makes the attacker's ability Mummy.
#define ABILITY_MOXIE 153 // Dealing a KO raises Attack by one stage.
#define ABILITY_JUSTIFIED 154 // Boosts Attack instead of being hit by Dark-type moves.
#define ABILITY_RATTLED 155 // If hit by Bug, Dark or Ghost move, or flinches: +1 Speed.
#define ABILITY_MAGIC_BOUNCE 156 // Bounces back the effect of status moves to their user.
#define ABILITY_SAP_SIPPER 157 // Boosts highest Atk instead of being hit by Grass-type moves.
#define ABILITY_PRANKSTER 158 // Status moves have +1 priority but fail on opposing Dark-types.
#define ABILITY_SAND_FORCE 159 // Ups highest attacking stat by 1.5x in sand.
#define ABILITY_IRON_BARBS 160 // Enemies lose 1/8 of max HP if they use a contact move.
#define ABILITY_ZEN_MODE 161 // Transforms into Zen Mode on entry until end of battle.
#define ABILITY_VICTORY_STAR 162 // Gives 1.2x accuracy boost to its own and its allies' moves.
#define ABILITY_TURBOBLAZE 163 // Moves hit through abilities. Adds Fire type to itself.
#define ABILITY_TERAVOLT 164 // Moves hit through abilities. Adds Electric type to itself.

#define ABILITIES_COUNT_GEN5 (ABILITY_TERAVOLT + 1)

#define ABILITY_AROMA_VEIL 165 // Immune to Encore, Attract, Taunt, Torment, Disable, Heal Block.
#define ABILITY_FLOWER_VEIL 166 // Grass-types on this Pokémon's side are immune to stat drops.
#define ABILITY_CHEEK_POUCH 167 // This ability has no effect.
#define ABILITY_PROTEAN 168 // Changes type depending on the move it's about to use.
#define ABILITY_FUR_COAT 169 // Halves damage taken by Physical moves. Does NOT double Defense.
#define ABILITY_MAGICIAN 170 // Steals the foe's held item after using a non-contact move.
#define ABILITY_BULLETPROOF 171 // Immune to projectile, ball, or bomb-based moves.
#define ABILITY_COMPETITIVE 172 // Raises Sp. Atk by two stages if stats are lowered by an enemy.
#define ABILITY_STRONG_JAW 173 // Boosts the power of bite/fang moves by 1.3x.
#define ABILITY_REFRIGERATE 174 // Normal-type moves become Ice- type moves and get a 1.1x boost.
#define ABILITY_SWEET_VEIL 175 // This Pokémon and its ally are immune to sleep.
#define ABILITY_STANCE_CHANGE 176 // Turns into Blade or Shield form depending on move used.
#define ABILITY_GALE_WINGS 177 // At full HP, gives +1 priority to this Pokémon's Flying-type moves.
#define ABILITY_MEGA_LAUNCHER 178 // Boosts Beam/Pump/Cannon/Shot/ Gun/Pulse, etc. moves by 1.3x.
#define ABILITY_GRASS_PELT 179 // This Pokémon's Defense gets a 1.5x boost in Grassy Terrain.
#define ABILITY_SYMBIOSIS 180 // Passes own item to its ally if said ally consumes its item.
#define ABILITY_TOUGH_CLAWS 181 // Boosts the power of contact moves by 1.3x.
#define ABILITY_PIXILATE 182 // Normal-type moves become Fairy- type moves and get a 1.1x boost.
#define ABILITY_GOOEY 183 // Lowers Speed of enemies that make contact with this Pokémon.
#define ABILITY_AERILATE 184 // Normal-type moves become Flying- type moves and get a 1.1x boost.
#define ABILITY_PARENTAL_BOND 185 // Moves hit twice. 1st hit at 100% power, 2nd hit at 25%.
#define ABILITY_DARK_AURA 186 // Boosts Dark moves by 1.33x for all while this Pokémon is out.
#define ABILITY_FAIRY_AURA 187 // Boosts Fairy moves by 1.33x for all while this Pokémon is out.
#define ABILITY_AURA_BREAK 188 // Cancels aura abilities and makes them 25% weaker instead.
#define ABILITY_PRIMORDIAL_SEA 189 // Heavy Rain until switched out. Fire-type moves are unusable.
#define ABILITY_DESOLATE_LAND 190 // Intense Sun until switched out. Water-type moves are unusable.
#define ABILITY_DELTA_STREAM 191 // Strong Winds until switched out. Weather-based moves not usable.

#define ABILITIES_COUNT_GEN6 (ABILITY_DELTA_STREAM + 1)

#define ABILITY_STAMINA 192 // Getting hit raises Def by +1. Critical hits maximize Defense.
#define ABILITY_WIMP_OUT 193 // At 1/2 of max HP or below, instantly switches out.
#define ABILITY_EMERGENCY_EXIT 194 // At 1/2 of max HP or below, instantly switches out.
#define ABILITY_WATER_COMPACTION 195 // Takes 1/2 dmg from Water-type moves. +2 Def when hit by those.
#define ABILITY_MERCILESS 196 // 100% crit if targetting slowed, poisoned or paralyzed foes.
#define ABILITY_SHIELDS_DOWN 197 // At 1/2 of max HP or below, transforms into Core form.
#define ABILITY_STAKEOUT 198 // Deals double damage to opponents being switched in.
#define ABILITY_WATER_BUBBLE 199 // Halves Fire dmg taken, no burns, doubles power of its Water moves.
#define ABILITY_STEELWORKER 200 // Boosts the power of Steel-type moves by 1.3x.
#define ABILITY_BERSERK 201 // Boosts Sp. Atk by one stage when at 1/2 of max HP or lower.
#define ABILITY_SLUSH_RUSH 202 // This Pokémon's Speed gets a 1.5x boost in hail.
#define ABILITY_LONG_REACH 203 // Doesn't make contact. Boosts Phys. non-contact moves by 1.2x.
#define ABILITY_LIQUID_VOICE 204 // Sound moves get a 1.2x boost and become Ground if Normal.
#define ABILITY_TRIAGE 205 // Moves that have a healing effect gain +3 priority.
#define ABILITY_GALVANIZE 206 // Normal-type moves become Elec.- type moves and get a 1.1x boost.
#define ABILITY_SURGE_SURFER 207 // If Electric Terrain is active, gets a 1.5x Speed boost.
#define ABILITY_SCHOOLING 208 // If Lv. 20 or more: changes into School form until 1/4 HP or less.
#define ABILITY_DISGUISE 209 // Protects once against an attack. Restores protection in fog.
#define ABILITY_BATTLE_BOND 210 // Transforms into Battle Bond form after dealing a KO.
#define ABILITY_POWER_CONSTRUCT 211 // At 1/2 of max HP or below, transforms into Complete form.
#define ABILITY_CORROSION 212 // Steel-types take Supereffective from Poison. Can poison any type.
#define ABILITY_COMATOSE 213 // Can move, but is always asleep. Immune to status conditions.
#define ABILITY_QUEENLY_MAJESTY 214 // Protects itself and ally from priority moves.
#define ABILITY_INNARDS_OUT 215 // If KO'd, deals as much damage as what the fatal attack dealt.
#define ABILITY_DANCER 216 // Copies dance moves used by others.
#define ABILITY_BATTERY 217 // Grants a 1.3x power boost to ally's Special attacks.
#define ABILITY_FLUFFY 218 // Takes 1/2 dmg from contact moves but Fire moves hurt it 2x more.
#define ABILITY_DAZZLING 219 // Protects itself and ally from priority moves.
#define ABILITY_SOUL_HEART 220 // KOs dealt anywhere on the field raise Sp. Atk by one stage.
#define ABILITY_TANGLING_HAIR 221 // Lowers Speed of enemies that make contact with this Pokémon.
#define ABILITY_RECEIVER 222 // In Double Battles, copies its fainting partner's ability.
#define ABILITY_POWER_OF_ALCHEMY 223 // Remove or turns enemy items into Black Sludge or Big Nugget on entry.
#define ABILITY_BEAST_BOOST 224 // Dealing a KO raises highest calculated stat by one stage.
#define ABILITY_RKS_SYSTEM 225 // Held Memory determines its type. Also has Protean + Adaptability.
#define ABILITY_ELECTRIC_SURGE 226 // Casts Electric Terrain on entry. Lasts 8 turns.
#define ABILITY_PSYCHIC_SURGE 227 // Casts Psychic Terrain on entry. Lasts 8 turns.
#define ABILITY_MISTY_SURGE 228 // Casts Misty Terrain on entry. Lasts 8 turns.
#define ABILITY_GRASSY_SURGE 229 // Casts Grassy Terrain on entry. Lasts 8 turns.
#define ABILITY_FULL_METAL_BODY 230 // Immune to stat drops.
#define ABILITY_SHADOW_SHIELD 231 // At full HP, halves damage taken from attacks
#define ABILITY_PRISM_ARMOR 232 // Takes 35% less damage from Super-effective moves.
#define ABILITY_NEUROFORCE 233 // Grants an additional 1.25x boost to Super-effective moves.

#define ABILITIES_COUNT_GEN7 (ABILITY_NEUROFORCE + 1)

#define ABILITY_INTREPID_SWORD 234 // On entry, raises Attack by one stage.
#define ABILITY_DAUNTLESS_SHIELD 235 // On entry, raises Defense by one stage.
#define ABILITY_LIBERO 236 // Before using a move, changes its type to the move's type.
#define ABILITY_BALL_FETCH 237 // No effect in battle.
#define ABILITY_COTTON_DOWN 238 // Lowers the Speed of all active Pokémon by one stage when hit.
#define ABILITY_PROPELLER_TAIL 239 // Isn't affected by target redirection.
#define ABILITY_MIRROR_ARMOR 240 // Bounces back any stat drops inflicted by an enemy.
#define ABILITY_GULP_MISSILE 241 // Gulps a prey after Dive/Surf. If hit, shoots prey at enemy.
#define ABILITY_STALWART 242 // Isn't affected by target redirection.
#define ABILITY_STEAM_ENGINE 243 // Maximizes Speed if hit by a Fire-type or Water-type attack.
#define ABILITY_PUNK_ROCK 244 // Sound moves deal 1.3x more dmg. Takes -50% dmg from sound moves.
#define ABILITY_SAND_SPIT 245 // If hit, summons a sandstorm that lasts 8 turns.
#define ABILITY_ICE_SCALES 246 // Halves damage taken by Special moves. Does NOT double SpDef.
#define ABILITY_RIPEN 247 // Doubles resistance, healing and stat raises provided by Berries.
#define ABILITY_ICE_FACE 248 // Protects once against an attack. Restores protection under hail.
#define ABILITY_POWER_SPOT 249 // Grants a 1.3x boost to ally's attacks.
#define ABILITY_MIMICRY 250 // Changes type depending on active Terrain.
#define ABILITY_SCREEN_CLEANER 251 // Clears screens and Aurora Veil from both sides on entry.
#define ABILITY_STEELY_SPIRIT 252 // Boosts own & ally's Steel-type moves by 1.3x.
#define ABILITY_PERISH_BODY 253 // If hit, casts Perish Song.
#define ABILITY_WANDERING_SPIRIT 254 // Trades ability with attacker on contact.
#define ABILITY_GORILLA_TACTICS 255 // Raises own Atk by 1.5x, but can only use the first chosen move.
#define ABILITY_NEUTRALIZING_GAS 256 // All abilities are nullified.
#define ABILITY_PASTEL_VEIL 257 // Casts Safeguard on entry.
#define ABILITY_HUNGER_SWITCH 258 // Changes between Full and Hangry forms after each turn.
#define ABILITY_QUICK_DRAW 259 // 30% chance to move first.
#define ABILITY_UNSEEN_FIST 260 // Its contact moves hit enemies, even if they protect themselves.
#define ABILITY_CURIOUS_MEDICINE 261 // Resets its ally's stat changes on entry.
#define ABILITY_TRANSISTOR 262 // Boosts the power of Electric- type moves by 1.5x.
#define ABILITY_DRAGONS_MAW 263 // Boosts the power of Dragon-type moves by 1.5x.
#define ABILITY_CHILLING_NEIGH 264 // KOs raise Attack by one stage.
#define ABILITY_GRIM_NEIGH 265 // KOs raise Sp. Atk by one stage.
#define ABILITY_AS_ONE_ICE_RIDER 266 // Unnerve + Chilling Neigh.
#define ABILITY_AS_ONE_SHADOW_RIDER 267 // Unnerve + Grim Neigh.

#define ABILITIES_COUNT_LATEST_GEN (ABILITY_AS_ONE_SHADOW_RIDER + 1)

#define ABILITY_CHLOROPLAST 268 // Weather Ball, Solar Beam/Blade, Growth act as if used in sun.
#define ABILITY_WHITEOUT 269 // Grants a 1.5x power boost to Ice-type moves under hail.
#define ABILITY_PYROMANCY 270 // Moves inflict burn 5x as often.
#define ABILITY_KEEN_EDGE 271 // Boosts the power of slashing moves by 1.3x.
#define ABILITY_PRISM_SCALES 272 // Takes 30% less damage from Special attacks.
#define ABILITY_POWER_FISTS 273 // Iron Fist moves target Special Defense and get a 1.3x boost.
#define ABILITY_SAND_SONG 274 // Sound moves get a 1.2x boost and become Ground if Normal.
#define ABILITY_RAMPAGE 275 // No recharge after a KO, if it usually would need to recharge.
#define ABILITY_VENGEANCE 276 // Boosts Ghost-type moves by 1.2x, or 1.5x when below 1/3 HP.
#define ABILITY_BLITZ_BOXER 277 // At full HP, gives +1 priority to this Pokémon's punching moves.
#define ABILITY_ANTARCTIC_BIRD 278 // Ice-type and Flying-type moves get a 1.3x power boost.
#define ABILITY_IMMOLATE 279 // Normal-type moves become Fire- type moves and get a 1.1x boost.
#define ABILITY_CRYSTALLIZE 280 // Rock-type moves become Ice-type moves and get a 1.1x boost.
#define ABILITY_ELECTROCYTES 281 // Boosts the power of Electric- type moves by 1.25x.
#define ABILITY_AERODYNAMICS 282 // Boosts Speed instead of being hit by Flying-type moves.
#define ABILITY_CHRISTMAS_SPIRIT 283 // Takes 50% less damage if hail is active.
#define ABILITY_EXPLOIT_WEAKNESS 284 // Moves are 1.25x stronger on foes affected by a status condition.
#define ABILITY_GROUND_SHOCK 285 // Target Grounds aren't immune to Electric but resist it instead.
#define ABILITY_ANCIENT_IDOL 286 // Uses Def and Sp. Def instead of Atk and Sp. Atk when attacking.
#define ABILITY_MYSTIC_POWER 287 // All moves gain the 1.5x power boost from STAB.
#define ABILITY_PERFECTIONIST 288 // Move BP < 51 BP: +1 to crit rate. Move BP < 26 BP: +1 priority too.
#define ABILITY_GROWING_TOOTH 289 // Raises Attack by one stage after using a biting move.
#define ABILITY_INFLATABLE 290 // Ups Def and Sp. Def by one stage if hit by Flying or Fire moves.
#define ABILITY_AURORA_BOREALIS 291 // Boosts the power of Ice-type moves by 1.5x (due to STAB).
#define ABILITY_AVENGER 292 // If a party Pokémon fainted last turn, next move gets 1.5x boost.
#define ABILITY_LETS_ROLL 293 // Casts Defense Curl on entry.
#define ABILITY_AQUATIC 294 // Adds Water type to itself.
#define ABILITY_LOUD_BANG 295 // Sound-based moves have 50% chance to confuse the foe.
#define ABILITY_LEAD_COAT 296 // Takes 40% less from Phys. moves. This Pokémon's Speed is 0.9x.
#define ABILITY_AMPHIBIOUS 297 // Boosts the power of Water-type moves by 1.5x (due to STAB).
#define ABILITY_GROUNDED 298 // Adds Ground type to itself.
#define ABILITY_EARTHBOUND 299 // Boosts the power of Ground-type moves by 1.25x.
#define ABILITY_FIGHT_SPIRIT 300 // Normal-type moves become Fight.- type moves and get a 1.1x boost.
#define ABILITY_FELINE_PROWESS 301 // Doubles own Sp. Atk stat. Boosts raw stat, not base stat.
#define ABILITY_COIL_UP 302 // On entry, gives +1 priority once to the first biting move used.
#define ABILITY_FOSSILIZED 303 // Halves dmg taken by Rock moves. Boosts own Rock moves by 1.2x.
#define ABILITY_MAGICAL_DUST 304 // If hit by a contact move, gives Psychic type to the attacker.
#define ABILITY_DREAMCATCHER 305 // Doubles move power if anyone on the field is asleep.
#define ABILITY_NOCTURNAL 306 // Boosts own Dark moves by 1.25x. Takes -25% dmg from Dark/Fairy.
#define ABILITY_SELF_SUFFICIENT 307 // Recovers 1/16 of max HP at the end of each turn.
#define ABILITY_TECTONIZE 308 // Normal-type moves become Ground- type moves and get a 1.1x boost.
#define ABILITY_ICE_AGE 309 // Adds Ice type to itself.
#define ABILITY_HALF_DRAKE 310 // Adds Dragon type to itself.
#define ABILITY_LIQUIFIED 311 // Takes 1/2 dmg from contact moves but Water moves hurt it 2x more.
#define ABILITY_DRAGONFLY 312 // Adds Dragon type to itself. Avoids Ground attacks.
#define ABILITY_DRAGONSLAYER 313 // Deals 1.5x damage to Dragons. Takes .5x damage from Dragons.
#define ABILITY_MOUNTAINEER 314 // Immune to Rock-type attacks and Stealth Rock damage.
#define ABILITY_HYDRATE 315 // Normal-type moves become Water- type moves and get a 1.1x boost.
#define ABILITY_METALLIC 316 // Adds Steel type to itself.
#define ABILITY_PERMAFROST 317 // Takes 25% less damage from Super-effective moves.
#define ABILITY_PRIMAL_ARMOR 318 // Takes 50% less damage from Super-effective moves.
#define ABILITY_RAGING_BOXER 319 // Punching moves hit twice. 1st hit at 100% power, 2nd hit at 40%.
#define ABILITY_AIR_BLOWER 320 // Casts a 3-turn Tailwind on entry.
#define ABILITY_JUGGERNAUT 321 // Paralysis-immune. Uses 20% of its Def when using a contact move.
#define ABILITY_SHORT_CIRCUIT 322 // Boosts Elec.-type moves by 1.2x, or 1.5x when below 1/3 HP.
#define ABILITY_MAJESTIC_BIRD 323 // Boosts own Sp. Atk by 1.5x. Boosts raw stat, not base stat.
#define ABILITY_PHANTOM 324 // Adds Ghost type to itself.
#define ABILITY_INTOXICATE 325 // Normal-type moves become Poison- type moves and get a 1.1x boost.
#define ABILITY_IMPENETRABLE 326 // Only damaged by attacks.
#define ABILITY_HYPNOTIST 327 // Hypnosis accuracy is 90% when used by this Pokémon.
#define ABILITY_OVERWHELM 328 // Hits Fairies with Dragon moves. Immune to Intimidate and Scare.
#define ABILITY_SCARE 329 // Lowers foes' Sp. Atk by one stage on entry.
#define ABILITY_MAJESTIC_MOTH 330 // On entry, raises highest calculated stat by one stage.
#define ABILITY_SOUL_EATER 331 // Dealing a KO heals 1/4 of this Pokémon's max HP.
#define ABILITY_SOUL_LINKER 332 // Enemies take all the damage they deal, same for this Pokémon.
#define ABILITY_SWEET_DREAMS 333 // Heals 1/8 of max HP every turn if asleep. Immune to Bad Dreams.
#define ABILITY_BAD_LUCK 334 // Foes hit the lowest damage roll, have 5% less acc. and can't crit.
#define ABILITY_HAUNTED_SPIRIT 335 // When this Pokémon is KO'd, casts a Curse on the attacker.
#define ABILITY_ELECTRIC_BURST 336 // Boosts own Elec. moves by 1.35x, takes 10% of dmg dealt as recoil.
#define ABILITY_RAW_WOOD 337 // Halves dmg taken by Grass moves. Boosts own Grass moves by 1.2x.
#define ABILITY_SOLENOGLYPHS 338 // Biting moves have a 50% chance to badly poison the target.
#define ABILITY_SPIDER_LAIR 339 // Casts Sticky Web on entry. Lasts 5 turns.
#define ABILITY_FATAL_PRECISION 340 // Super-effective moves never miss and get a 1.2x boost.
#define ABILITY_FORT_KNOX 341 // Boosts Defense by three stages if stats are lowered by an enemy.
#define ABILITY_SEAWEED 342 // Takes 1/2 dmg from Fire if Grass, doubles Grass dmg on Fire-types.
#define ABILITY_PSYCHIC_MIND 343 // Boosts Psychic-type moves by 1.2x, or 1.5x when under 1/3 HP.
#define ABILITY_POISON_ABSORB 344 // Heals 25% of max HP when hit by a Poison-type move.
#define ABILITY_SCAVENGER 345 // Dealing a KO heals 1/4 of this Pokémon's max HP.
#define ABILITY_TWISTED_DIMENSION 346 // Sets up Trick Room on entry, lasts 3 turns.
#define ABILITY_MULTI_HEADED 347 // Hits as many times, as it has heads.
#define ABILITY_NORTH_WIND 348 // 3 turns Aurora Veil on entry. Immune to Hail damage.
#define ABILITY_OVERCHARGE 349 // Electric is super effective vs Electric. Can paralyze Electric.
#define ABILITY_VIOLENT_RUSH 350 // Boosts Speed by 50% + Attack by 20% on first turn.
#define ABILITY_FLAMING_SOUL 351 // At full HP, gives +1 priority to this Pokémon's Fire-type moves.
#define ABILITY_SAGE_POWER 352 // Ups Special Attack by 50% and locks move.
#define ABILITY_BONE_ZONE 353 // Bone moves ignore immunities and deal 2x on not very effective.
#define ABILITY_WEATHER_CONTROL 354 // Negates all weather based moves from enemies.
#define ABILITY_SPEED_FORCE 355 // Contact moves use 20% of its Speed stat additionally.
#define ABILITY_SEA_GUARDIAN 356 // Ups highest stat by +1 on entry when it rains.
#define ABILITY_MOLTEN_DOWN 357 // Fire-type is super effective against Rock-type.
#define ABILITY_HYPER_AGGRESSIVE 358 // Moves hit twice. Second hit does 25% damage.
#define ABILITY_FLOCK 359 // Boosts Flying-type moves by 1.2x, or 1.5x when below 1/3 HP.
#define ABILITY_FIELD_EXPLORER 360 // Boosts field moves by 50%. Cut, Surf, Strength etc.
#define ABILITY_STRIKER 361 // Boosts the power of kicking moves by 1.3x.
#define ABILITY_FROZEN_SOUL 362 // At full HP, gives +1 priority to this Pokémon's Ice-type moves.
#define ABILITY_PREDATOR 363 // Dealing a KO heals 1/4 of this Pokémon's max HP.
#define ABILITY_LOOTER 364 // Dealing a KO heals 1/4 of this Pokémon's max HP.
#define ABILITY_LUNAR_ECLIPSE 365 // Fairy & Dark gains STAB. Hypnosis has 1.5x accuracy.
#define ABILITY_SOLAR_FLARE 366 // Chloroplast + Immolate. Fire moves gain STAB.
#define ABILITY_POWER_CORE 367 // The Pokémon uses +20% of its Defense or SpDef during moves.
#define ABILITY_SIGHTING_SYSTEM 368 // Moves always hit. Moves last for moves less than 80% accuracy.
#define ABILITY_BAD_COMPANY 369 // Not implemented right now. Has no effect.
#define ABILITY_OPPORTUNIST 370 // If target has less than 1/2 HP, single-target moves get +1 prio.
#define ABILITY_GIANT_WINGS 371 // Boosts the power of wing, wind or air-based moves by 1.3x.
#define ABILITY_MOMENTUM 372 // Contact moves use the Speed stat for damage calculation.
#define ABILITY_GRIP_PINCER 373 // 50% chance to trap. Then ignores Defense & accuracy checks.
#define ABILITY_BIG_LEAVES 374 // Chloroplast + Chlorophyll + Leaf Guard + Harvest + Solar Power.
#define ABILITY_PRECISE_FIST 375 // Punching moves get +1 crit and 5x effect chance.
#define ABILITY_DEADEYE 376 // Never misses.
#define ABILITY_ARTILLERY 377 // Mega Launcher moves always hit. Single-target now hits both foes.
#define ABILITY_AMPLIFIER 378 // Ups sound moves by 30% and makes them hit both foes.
#define ABILITY_ICE_DEW 379 // Boosts SpAtk instead of being hit by Ice-type moves.
#define ABILITY_SUN_WORSHIP 380 // Ups highest stat by +1 on entry when sunny.
#define ABILITY_POLLINATE 381 // Normal-type moves become Bug- type moves and get a 1.1x boost.
#define ABILITY_VOLCANO_RAGE 382 // Triggers 50 BP Eruption after using a Fire-type move.
#define ABILITY_COLD_REBOUND 383 // Attacks with Icy Wind when hit by a contact move.
#define ABILITY_LOW_BLOW 384 // Attacks with 40BP Feint Attack on switch-in.
#define ABILITY_NOSFERATU 385 // Contact moves do +20% damage and heal 1/2 of damage dealt.
#define ABILITY_SPECTRAL_SHROUD 386 // Spectralize + 30% chance to badly poison the foe.
#define ABILITY_DISCIPLINE 387 // Rampage moves no longer trap you. Can't be confused or intimidated.
#define ABILITY_THUNDERCALL 388 // Triggers Smite at 20% power when using an Electric move.
#define ABILITY_MARINE_APEX 389 // 50% more damage to Water- types + Infiltrator.
#define ABILITY_MIGHTY_HORN 390 // Boosts the power of horn and drill-based by 1.3x.
#define ABILITY_HARDENED_SHEATH 391 // Ups Attack by +1 when using horn moves.
#define ABILITY_ARCTIC_FUR 392 // Weakens incoming physical and special moves by 35%.
#define ABILITY_SPECTRALIZE 393 // Normal-type moves become Ghost- type moves and get a 1.1x boost.
#define ABILITY_LETHARGY 394 // Damage drops 20% each turn to 20%. Resets on switch-in.
#define ABILITY_IRON_BARRAGE 395 // Combines Mega Launcher with Sighting System.
#define ABILITY_STEEL_BARREL 396 // Immune to recoil damage, but not immune to Explosion/crash dmg.
#define ABILITY_PYRO_SHELLS 397 // Triggers 50 BP Outburst after using a Mega Launcher move.
#define ABILITY_FUNGAL_INFECTION 398 // Contact moves inflict Leech Seed on the target.
#define ABILITY_PARRY 399 // Counters contact with Mach Punch. Takes 20% less damage.
#define ABILITY_SCRAPYARD 400 // Sets a layer of Spikes when hit (contact move).
#define ABILITY_LOOSE_QUILLS 401 // Sets a layer of Spikes when hit (contact move).
#define ABILITY_TOXIC_DEBRIS 402 // Sets a layer of Toxic Spikes when hit by contact moves.
#define ABILITY_ROUNDHOUSE 403 // Kicks always hit. Damages foes' weaker defenses.
#define ABILITY_MINERALIZE 404 // Normal-type moves become Rock- type moves and get a 1.1x boost.
#define ABILITY_LOOSE_ROCKS 405 // Deploys Stealth Rocks when hit by contact.
#define ABILITY_SPINNING_TOP 406 // Fighting moves up speed +1 and clear hazards.
#define ABILITY_RETRIBUTION_BLOW 407 // Uses Hyper Beam if any foe uses an stat boosting move.
#define ABILITY_FEARMONGER 408 // Intimidate + Scare; 10% para chance on contact moves.
#define ABILITY_KINGS_WRATH 409 // Lowering any stats on its side raises Atk and Def.
#define ABILITY_QUEENS_MOURNING 410 // Lowering any stats on its side raises SpAtk and SpDef.
#define ABILITY_TOXIC_SPILL 411 // Non-Poison-types take 1/8 dmg every turn when on field.
#define ABILITY_DESERT_CLOAK 412 // Protects its side from status and secondary effects in sand.
#define ABILITY_DRACONIZE 413 // Normal-type moves become Dragon- type moves and get a 1.1x boost.
#define ABILITY_PRETTY_PRINCESS 414 // Does 50% more damage if the target has any lowered stat.
#define ABILITY_SELF_REPAIR 415 // Combines Self Sufficient & Natural Cure.
#define ABILITY_ATOMIC_BURST 416 // When hit super-effectively, triggers 50 BP Hyper Beam.
#define ABILITY_HELLBLAZE 417 // Boosts Fire-type moves by 1.3x, or 1.8x when below 1/3 HP.
#define ABILITY_RIPTIDE 418 // Boosts Water-type moves by 1.3x, or 1.8x when below 1/3 HP.
#define ABILITY_FOREST_RAGE 419 // Boosts Grass-type moves by 1.3x, or 1.8x when below 1/3 HP.
#define ABILITY_PRIMAL_MAW 420 // Biting moves hit twice. 2nd hit does 0.4x damage.
#define ABILITY_SWEEPING_EDGE 421 // Keen Edge moves always hit. Single-target now hits both foes.
#define ABILITY_GIFTED_MIND 422 // Nulls Psychic weakness; status moves always hit.
#define ABILITY_HYDRO_CIRCUIT 423 // Electric moves +50%; Water moves siphon 25% damage.
#define ABILITY_EQUINOX 424 // Boosts Atk or SpAtk to match the higher value.
#define ABILITY_ABSORBANT 425 // Drain moves recover +50% HP & apply Leech Seed.
#define ABILITY_CLUELESS 426 // Negates Weather, Rooms and Terrains.
#define ABILITY_CHEATING_DEATH 427 // Gets no damage for the first two hits.
#define ABILITY_CHEAP_TACTICS 428 // Attacks with Scratch on switch-in.
#define ABILITY_COWARD 429 // Sets up Protect on switch-in. Only works once.
#define ABILITY_VOLT_RUSH 430 // At full HP, gives +1 priority to its Electric-type moves.
#define ABILITY_DUNE_TERROR 431 // Sand reduces damage by 35%. Boosts Ground moves by 20%.
#define ABILITY_INFERNAL_RAGE 432 // Fire-type moves are boosted by 35% with 5% recoil.
#define ABILITY_DUAL_WIELD 433 // Mega Launcher and Keen Edge moves hit twice for 70% damage.
#define ABILITY_ELEMENTAL_CHARGE 434 // 20% chance to BRN/FRZ/PARA with respective types.
#define ABILITY_AMBUSH 435 // Guaranteed critical hit on first turn.
#define ABILITY_ATLAS 436 // Sets Gravity on entry for 8 turns.
#define ABILITY_RADIANCE 437 // +20% accuracy; Dark moves fail when user is present.
#define ABILITY_JAWS_OF_CARNAGE 438 // Devours 1/2 of the foe when defeating it.
#define ABILITY_ANGELS_WRATH 439 // Drastically alters all of the users moves.
#define ABILITY_PRISMATIC_FUR 440 // Color Change + Protean + Fur Coat + Ice Scales.
#define ABILITY_SHOCKING_JAWS 441 // Biting moves have 50% chance to paralyze the target.
#define ABILITY_FAE_HUNTER 442 // Does 50% more damage to Fairy-types.
#define ABILITY_GRAVITY_WELL 443 // Sets Gravity on entry for 5 turns.
#define ABILITY_EVAPORATE 444 // Takes no damage and sets Mist if hit by water.
#define ABILITY_LUMBERJACK 445 // 1.5x damage to Grass types.
#define ABILITY_WELL_BAKED_BODY 446 // Boosts Defense sharply instead of being hit by Fire-type moves.
#define ABILITY_FURNACE 447 // User gains +2 Speed when when hit by rocks.
#define ABILITY_ELECTROMORPHOSIS 448 // Charges up when getting hit.
#define ABILITY_ROCKY_PAYLOAD 449 // Boosts the power of Rock-type and throwing moves by 1.5x.
#define ABILITY_EARTH_EATER 450 // Heals 25% of max HP when hit by a Ground move.
#define ABILITY_LINGERING_AROMA 451 // If hit, makes the attacker's ability Lingering Aroma.
#define ABILITY_FAIRY_TALE 452 // Adds Fairy type to itself.
#define ABILITY_RAGING_MOTH 453 // Fire moves hits twice, both hits at 70% power.
#define ABILITY_ADRENALINE_RUSH 454 // KOs raise Speed by one stage.
#define ABILITY_ARCHMAGE 455 // 30% chance of adding a type related effect to each move.
#define ABILITY_CRYOMANCY 456 // Moves inflict frostbite 5x as often.
#define ABILITY_PHANTOM_PAIN 457 // Ghost type moves can hit normal type pokemon for neutral damage.
#define ABILITY_PURGATORY 458 // Boosts Ghost-type moves by 1.3x, or 1.8x when below 1/3 HP.
#define ABILITY_EMANATE 459 // Normal-type moves become Psy.- type moves and get a 1.1x boost.
#define ABILITY_KUNOICHI_BLADE 460 // Boosts weaker moves and increases the frequency of multi-hit moves.
#define ABILITY_MONKEY_BUSINESS 461 // Uses Tickle on entry.
#define ABILITY_COMBAT_SPECIALIST 462 // Boosts the power of punching and kicking moves by 1.3x.
#define ABILITY_JUNGLES_GUARD 463 // Grass-types on user side: immune to status/stat drops from enemy.
#define ABILITY_HUNTERS_HORN 464 // Boost horn moves and heals 1/4 HP when defeating an enemy.
#define ABILITY_PIXIE_POWER 465 // 1.2x accuracy. Boosts Fairy moves by 1.33x for all.
#define ABILITY_PLASMA_LAMP 466 // Boost accuracy & power of Fire & Electric type moves by 1.2x.
#define ABILITY_MAGMA_EATER 467 // Predator + Molten Down.
#define ABILITY_SUPER_HOT_GOO 468 // Inflicts burn and lowers Speed on contact.
#define ABILITY_NIKA 469 // Iron fist + Water moves function normally under sun.
#define ABILITY_ARCHER 470 // Boosts the power of arrow moves by 1.3x.
#define ABILITY_COLD_PLASMA 471 // Electric type moves now inflict burn instead of paralysis.
#define ABILITY_SUPER_SLAMMER 472 // Boosts the power of hammer and slamming moves by 1.3x.
#define ABILITY_INVERSE_ROOM 473 // Sets up the Inverse field condition for 3 turns upon entry.
#define ABILITY_ACCELERATE 474 // Moves that need a charge turn are now used instantly.
#define ABILITY_FROST_BURN 475 // Triggers 40BP Ice Beam after using a Fire-type move.
#define ABILITY_ITCHY_DEFENSE 476 // Causes infestation when hit by a contact move.
#define ABILITY_GENERATOR 477 // Charges up once on entry or when electric terrain is active.
#define ABILITY_MOON_SPIRIT 478 // Fairy & Dark gains STAB. Moonlight recovers 75% HP.
#define ABILITY_DUST_CLOUD 479 // Attacks with Sand Attack on switch-in.
#define ABILITY_BERSERKER_RAGE 480 // Berserk + Rampage.
#define ABILITY_TRICKSTER 481 // Uses Disable on switch-in.
#define ABILITY_SAND_GUARD 482 // Blocks priority and reduces special damage by 1/2 in sand.
#define ABILITY_NATURAL_RECOVERY 483 // Combines Natural Cure & Regenerator.
#define ABILITY_WIND_RIDER 484 // Increases attack in tailwind or when hit by wind move.
#define ABILITY_SOOTHING_AROMA 485 // Cures party status on entry.
#define ABILITY_PRIM_AND_PROPER 486 // Wonder Skin + Cute Charm.
#define ABILITY_SUPER_STRAIN 487 // KOs lower Attack by +1. Take 25% recoil damage.
#define ABILITY_TIPPING_POINT 488 // Getting hit raises SpAtk. Critical hits maximize SpAtk.
#define ABILITY_ENLIGHTENED 489 // Emanate + Inner Focus.
#define ABILITY_PEACEFUL_SLUMBER 490 // Sweet Dreams + Self Sufficient.
#define ABILITY_AFTERSHOCK 491 // Triggers Magnitude 4-7 after using a damaging move.
#define ABILITY_FREEZING_POINT 492 // 30% chance to get frostbitten on contact.
#define ABILITY_CRYO_PROFICIENCY 493 // Triggers hail when hit. 30% chance to frostbite on contact.
#define ABILITY_ARCANE_FORCE 494 // All moves gain STAB. Ups “supereffective” by 10%.
#define ABILITY_DOOMBRINGER 495 // Uses Doom Desire on switch-in.
#define ABILITY_WISHMAKER 496 // Uses Wish on switch-in. Three uses per battle.
#define ABILITY_YUKI_ONNA 497 // Scare + Intimidate. 10% chance to infatuate on hit.
#define ABILITY_SUPPRESS 498 // Casts Torment on entry.
#define ABILITY_REFRIGERATOR 499 // Combines Refrigerate & Illuminate.
#define ABILITY_HEAVEN_ASUNDER 500 // Spacial Rend always crits. Ups crit level by +1.
#define ABILITY_PURIFYING_WATERS 501 // Hydration + Water Veil.
#define ABILITY_SEABORNE 502 // Drizzle + Swift Swim.
#define ABILITY_HIGH_TIDE 503 // Triggers 50 BP Surf after using a Water-type move.
#define ABILITY_CHANGE_OF_HEART 504 // Uses Heart Swap on switch-in.
#define ABILITY_MYSTIC_BLADES 505 // Keen edge moves become special and deal 30% more damage.
#define ABILITY_DETERMINATION 506 // Ups Special Attack by 50% if suffering.
#define ABILITY_FERTILIZE 507 // Normal-type moves become Grass- type moves and get a 1.1x boost.
#define ABILITY_PURE_LOVE 508 // Infatuates on contact. Heal 25% damage vs infatuated.
#define ABILITY_FIGHTER 509 // Boosts Fight.-type moves by 1.2x, or 1.5x when below 1/3 HP.
#define ABILITY_MYCELIUM_MIGHT 510 // Status moves ignore immunities but go last.
#define ABILITY_TELEKINETIC 511 // Casts Telekinesis on entry.
#define ABILITY_COMBUSTION 512 // Boosts the power of Fire-type moves by 1.5x.
#define ABILITY_PONY_POWER 513 // Keen Edge + Mystic Blades.
#define ABILITY_POWDER_BURST 514 // Casts Powder on entry.
#define ABILITY_RETRIEVER 515 // Retrieves item on switch-out.
#define ABILITY_MONSTER_MASH 516 // Casts Trick-or-Treat on entry.
#define ABILITY_TWO_STEP 517 // Triggers 50BP Revelation Dance after using a Dance move.
#define ABILITY_SPITEFUL 518 // Reduces attacker's PP on contact.
#define ABILITY_FORTITUDE 519 // Boosts SpDef +1 when hit. Maxes SpDef on crit.
#define ABILITY_DEVOURER 520 // Strong Jaw + Primal Maw.
#define ABILITY_PHANTOM_THIEF 521 // Attacks with 40BP Spectral Thief on switch-in.
#define ABILITY_EARLY_GRAVE 522 // At full HP, gives +1 priority to this Pokémon's Ghost-type moves.
#define ABILITY_GRAPPLER 523 // Trapping moves last 6 turns. Trapping deals 1/6 HP.
#define ABILITY_BASS_BOOSTED 524 // Amplifier + Punk Rock.
#define ABILITY_FLAMING_JAWS 525 // Biting moves have 50% chance to burn the target.
#define ABILITY_MONSTER_HUNTER 526 // Deals 1.5x more damage to Dark-types.
#define ABILITY_CROWNED_SWORD 527 // Intrepid Sword + Anger Point.
#define ABILITY_CROWNED_SHIELD 528 // Dauntless Shield + Stamina.
#define ABILITY_BERSERK_DNA 529 // Sharply ups highest attacking stat but confuses on entry.
#define ABILITY_CROWNED_KING 530 // Unnerve + Grim Neigh + Chilling Neigh.
#define ABILITY_SNAP_TRAP_WHEN_HIT 531 // Counters contact with 50BP Snap Trap.
#define ABILITY_PERMANENCE 532 // Foes can't heal in any way.
#define ABILITY_HUBRIS 533 // KOs raise SpAtk by one stage.
#define ABILITY_COSMIC_DAZE 534 // 2x damage vs confused. Enemies take 2x confusion damage.
#define ABILITY_MINDS_EYE 535 // Hits Ghost-type Pokémon. Accuracy can't be lowered.
#define ABILITY_BLOOD_PRICE 536 // Does 30% more damage but lose 10% HP when attacking.
#define ABILITY_SPIKE_ARMOR 537 // 30% chance to bleed on contact or offense.
#define ABILITY_VOODOO_POWER 538 // 30% chance to bleed when hit by special attacks.
#define ABILITY_CHROME_COAT 539 // Reduces special damage taken by 40%, but decreases Speed by 10%.
#define ABILITY_BANSHEE 540 // Sound moves get a 1.2x boost and become Ghost if Normal.
#define ABILITY_WEB_SPINNER 541 // Uses String Shot on switch-in.
#define ABILITY_SHOWDOWN_MODE 542 // Ambush + Violent Rush.
#define ABILITY_SEED_SOWER 543 // Sets Grassy Terrain when hit. Heals party status when it does.
#define ABILITY_AIRBORNE 544 // Boosts own & ally's Flying-type moves by 1.3x.
#define ABILITY_PARROTING 545 // Copies sound moves used by others. Immune to sound.
#define ABILITY_SALT_CIRCLE 546 // Prevents opposing pokemon from fleeing on entry.
#define ABILITY_PURIFYING_SALT 547 // Immune to status conditions. Take 1/2 damage from Ghost.
#define ABILITY_PROTOSYNTHESIS 548 // Boosts highest stat in Sun or with Booster Energy.
#define ABILITY_QUARK_DRIVE 549 // Boosts highest stat in Electric Terrain or with Booster Energy.
#define ABILITY_WIND_POWER 550 // Charges up when hit by wind moves or Tailwind starts.
#define ABILITY_IMPULSE 551 // Non-contact moves use the Speed stat for damage.
#define ABILITY_TERMINAL_VELOCITY 552 // Special moves use 20% of its Speed stat additionally.
#define ABILITY_GUARD_DOG 553 // Can't be forced out. Inverts Intimidate effects.
#define ABILITY_ANGER_SHELL 554 // Applies Shell Smash when reduced below 1/2 HP.
#define ABILITY_EGOIST 555 // Raises its own stats when foes raise theirs.
#define ABILITY_SUBDUE 556 // Doubles the power of stat dropping moves.
#define ABILITY_READIED_ACTION 557 // Doubles attack on first turn.
#define ABILITY_DARK_GALE_WINGS 558 // At full HP, gives +1 priority to this Pokémon's Dark-type moves.
#define ABILITY_GUILT_TRIP 559 // Sharply lowers attacker's Attack and SpAtk when fainting.
#define ABILITY_WATER_GALE_WINGS 560 // At full HP, gives +1 priority to this Pokémon's Water-type moves.
#define ABILITY_ZERO_TO_HERO 561 // Changes forms after switching out.
#define ABILITY_COSTAR 562 // Copies its ally's stat changes on switch-in.
#define ABILITY_COMMANDER 563 // Hops inside an allied Dondozo. Boosts its ally but can't act.
#define ABILITY_EJECT_PACK_ABILITY 564 // Flees when stats are lowered.
#define ABILITY_VENGEFUL_SPIRIT 565 // Combines Haunted Spirit & Vengeance.
#define ABILITY_CUD_CHEW 566 // Eats berries again at the end of the next turn.
#define ABILITY_ARMOR_TAIL 567 // Protects itself and ally from priority moves.
#define ABILITY_MIND_CRUSH 568 // Biting moves use SpAtk and deal 30% more damage.
#define ABILITY_SUPREME_OVERLORD 569 // Each fainted ally increases Attack and SpAtk by 10%.
#define ABILITY_ILL_WILL 570 // Deletes the PP of the move that faints this Pokemon.
#define ABILITY_FIRE_SCALES 571 // Halves damage taken by Special moves. Does NOT double SpDef.
#define ABILITY_WATCH_YOUR_STEP 572 // Spreads two layers of Spikes on switch-in.
#define ABILITY_RAPID_RESPONSE 573 // Boosts Speed by 50% + SpAtk by 20% on first turn.
#define ABILITY_DOUBLE_IRON_BARBS 574 // 1/6 HP damage when touched.
#define ABILITY_THERMAL_EXCHANGE 575 // Ups Attack when hit by Fire. Immune to burn.
#define ABILITY_GOOD_AS_GOLD 576 // Immune to all Status moves, unless whole field is affected.
#define ABILITY_SHARING_IS_CARING 577 // Stat changes are shared between all battlers.
#define ABILITY_TABLETS_OF_RUIN 578 // Lowers the Attack of other Pokemon by 25%.
#define ABILITY_SWORD_OF_RUIN 579 // Lowers the Defense of other Pokemon by 25%.
#define ABILITY_VESSEL_OF_RUIN 580 // Lowers the Special Attack of other Pokemon by 25%.
#define ABILITY_BEADS_OF_RUIN 581 // Lowers the Special Defense of other Pokemon by 25%.
#define ABILITY_PERMAFROST_CLONE 582 // Takes 25% less damage from Super-effective moves.
#define ABILITY_GALLANTRY 583 // Gets no damage for first hit.
#define ABILITY_ORICHALCUM_PULSE 584 // Summons sun on entry. Raises Atk by 1.33x in sun.
#define ABILITY_LEAF_GUARD_CLONE 585 // Immune to status conditions if sun is active.
#define ABILITY_WINGED_KING 586 // Ups “supereffective” by 33%.
#define ABILITY_HADRON_ENGINE 587 // Field becomes Electric. +33% SpAtk in Electric Terrain.
#define ABILITY_IRON_SERPENT 588 // Ups “supereffective” by 33%.
#define ABILITY_WEATHER_DOUBLE_BOOST 589 // Sun boosts Water. Rain boosts Fire.
#define ABILITY_SWEEPING_EDGE_PLUS 590 // Sweeping Edge + Keen Edge.
#define ABILITY_CELESTIAL_BLESSING 591 // Recovers 1/12 of its health each turn under Misty Terrain.
#define ABILITY_MINION_CONTROL 592 // Moves hit an extra time for each healthy party member.
#define ABILITY_MOLTEN_BLADES 593 // Keen Edge + Keen Edge moves have a 20% chance to burn.
#define ABILITY_HAUNTING_FRENZY 594 // 20% chance to flinch the opponent. +1 speed on kill.
#define ABILITY_NOISE_CANCEL 595 // Protects the party from sound- based moves.
#define ABILITY_RADIO_JAM 596 // Sound-based moves inflict disable.
#define ABILITY_OLE 597 // 30% chance to evade single- target moves.
#define ABILITY_MALICIOUS 598 // Lowers the foe's highest Attack and Defense stat.
#define ABILITY_DEAD_POWER 599 // 1.5x Attack boost. 20% chance to curse on contact moves.
#define ABILITY_BRAWLING_WYVERN 600 // No guard + Dragon type moves become punching moves.
#define ABILITY_MYTHICAL_ARROWS 601 // Arrow moves become special and deal 30% more damage.
#define ABILITY_LAWNMOWER 602 // Removes terrain on switch-in. Stat up if terrain removed.
#define ABILITY_FLOURISH 603 // Boosts Grass moves by 50% in grassy terrain.
#define ABILITY_DESERT_SPIRIT 604 // Summons sand on entry. Ground moves hit airborne in sand.
#define ABILITY_CONTEMPT 605 // Ignores opposing stat changes. Boosts Attack when stat lowered.
#define ABILITY_AERIALIST 606 // Levitate + Flock.
#define ABILITY_TERA_SHELL 607 // All hits will be not very effective while at full HP.
#define ABILITY_TOXIC_CHAIN 608 // Moves have a 30% chance to badly poison the foe.
#define ABILITY_PARASITIC_SPORES 609 // Deals 1/8 HP damage to non- Ghost. Spreads on contact.
#define ABILITY_POISON_PUPPETEER 610 // Poison also inflicts confusion.
#define ABILITY_ENTRANCE 611 // Confusion also inflicts infatuation.
#define ABILITY_REJECTION 612 // Applies Quash on switch-in.
#define ABILITY_APPLE_ENLIGHTENMENT 613 // Fur coat + Magic Guard.
#define ABILITY_BALLOON_BOMBER 614 // Aftermath + Inflatable
#define ABILITY_FLAMING_MAW 615 // Strong Jaw + Flaming Jaws
#define ABILITY_DEMOLITIONIST 616 // Readied Action + Ignores Protect + screens break on readied turn
#define ABILITY_ROCKHARD_WILL 617 // Boosts Rock-type moves by 1.2x, or 1.5x when under 1/3 HP.
#define ABILITY_FRAGRANT_DAZE 618 // 30% chance to confuse on contact.
#define ABILITY_LOW_VISIBILITY 619 // Summons Eerie Fog on entry.
#define ABILITY_OLD_MARINER 620 // Seaweed + Water Absorb.
#define ABILITY_ECTOPLASM 621 // Ups highest attacking stat by 1.5x in fog.
#define ABILITY_BEAUTIFUL_MUSIC 622 // Sound-based moves have 50% chance to infatuate the foe.
#define ABILITY_SURPRISE 623 // Astonishes enemy priority users in fog.
#define ABILITY_SNOW_SONG 624 // Sound moves get a 1.2x boost and become Ice if Normal.
#define ABILITY_GREATER_SPIRIT 625 // Ups highest stat by +1 on entry in fog.
#define ABILITY_RESONANCE 626 // Sound moves cause the target to  bleed.
#define ABILITY_ETHEREAL_RUSH 627 // This Pokémon's Speed gets a 1.5x boost in fog.
#define ABILITY_CUTE_ANTECEDENCE 628 // At full HP, gives +1 priority to its Fairy-type moves.
#define ABILITY_RECURRING_NIGHTMARE 629 // Revives at 25% HP once after fainting in fog.
#define ABILITY_MENACING_SITUATION 630 // 20% chance to Fear on contact. Also works on offense.
#define ABILITY_SHINY_LIGHTNING 631 // Grants a 1.2x accuracy boost. Thunder never misses.
#define ABILITY_TERRIFY 632 // Lowers foes' Sp. Atk by two stages on entry.
#define ABILITY_ICE_DOWNFALL 633 // Counters contact with 60BP Icicle Crash.
#define ABILITY_LAST_STAND 634 // Def and SpDef increase as HP drops. Max 1.6x.
#define ABILITY_PYROCLASTIC_FLOW 635 // Combines Molten Down & Corrosion.
#define ABILITY_BLOOD_BATH 636 // Immune to bleed. Inflict fear when inflicting bleed.
#define ABILITY_BATTLE_AURA 637 // Sharply increases the critical hit rate for all while on the field.
#define ABILITY_BLOODLUST 638 // Blood Bath + Jaws of Carnage.
#define ABILITY_PIERCING_SOLO 639 // Sound moves have a 30% chance to cause bleeding.
#define ABILITY_RHYTHMIC 640 // Deals 10% more damage for each repeated move use.
#define ABILITY_CHUNKY_BASS_LINE 641 // Triggers a 40BP Earthquake after using a sound move.
#define ABILITY_DUAL_HAMMER 642 // Super Slammer moves hit twice for 70% damage.
#define ABILITY_DENTING_BLOWS 643 // Hammer moves lower Defense.
#define ABILITY_ICE_COLD_HUNTER 644 // Combines Hyper Aggressive and Whiteout.
#define ABILITY_SOUL_CRUSHER 645 // Hammer moves become Special and get a 1.1x power boost.
#define ABILITY_ARC_FLASH 646 // 50% chance to burn when hit or paralyze when dealing damage.
#define ABILITY_UNICORN 647 // Mighty Horn + Dazzling.
#define ABILITY_ON_THE_PROWL 648 // +1 priority for the first turn. Negative priority becomes +0.
#define ABILITY_PRETENTIOUS 649 // Dealing a KO raises Crit by one stage.
#define ABILITY_VENOBLAZE_PINCERS 650 // 1.2x boost to physical moves and 20% chance to Burn or Poison.
#define ABILITY_ETERNAL_BLESSING 651 // Combines Celestial Blessing and Regenerator.
#define ABILITY_SUGAR_RUSH 652 // Sharply raises Speed when eating a berry.
#define ABILITY_PEACEFUL_REST 653 // Heals 1/8 of max HP every turn in fog.
#define ABILITY_WHITE_NOISE 654 // Combines Static and Rest in Peace.
#define ABILITY_SMOKEY_MANEUVERS 655 // Evasion is boosted by 1.25x in fog.
#define ABILITY_TAG 656 // Attacks switching opponents with a 20BP Pursuit.
#define ABILITY_POWER_METAL 657 // Sound moves get a 1.2x boost and become Steel if Normal.
#define ABILITY_POWER_EDGE 658 // Keen Edge moves target Special Defense and get a 1.3x boost.
#define ABILITY_SUPERCONDUCTOR 659 // Steel-type moves become Electric -type moves and get a 1.1x boost.
#define ABILITY_ULTRA_INSTINCT 660 // Counters contact with Vacuum Wave. Takes 20% less damage.
#define ABILITY_UNLOCKED_POTENTIAL 661 // Inner Focus + Berserk.
#define ABILITY_HIGHER_RANK 662 // Priority moves get a 1.2x boost.
#define ABILITY_FUNERAL_PYRE 663 // Non-Ghost and Dark-types take 1/4 damage every turn.
#define ABILITY_FLAME_BUBBLE 664 // Water Bubble + Flaming Soul.
#define ABILITY_ELEMENTAL_VORTEX 665 // Flash Fire + Water Absorb.
#define ABILITY_SNOWY_WRATH 666 // Snow Warning + Whiteout.
#define ABILITY_PATTERN_CHANGE 667 // Changes type depending on the move it's about to use.
#define ABILITY_NO_TURNING_BACK 668 // Boosts all stats but can't retreat when below 1/2 max HP.
#define ABILITY_FLAMMABLE_COAT 669 // Changes forms when using or hit by a Fire-type move.
#define ABILITY_DRACO_MORALE 670 // Uses Dragon Cheer on switch-in.
#define ABILITY_BAD_OMEN 671 // Foes min roll and may miss. Takes 1/4 damage from crits.
#define ABILITY_MOSH_PIT 672 // Ally's attacks get a 1.25x boost. 1.5x if attack causes recoil.
#define ABILITY_BLOOD_STAIN 673 // Bleeds if not immune. Can't get other status. Spreads on contact.
#define ABILITY_BLOOD_STIGMA 674 // Immune to status. Gets a 50% boost vs bleeding foes.
#define ABILITY_MAXIMUM_ACCELERATION 675 // Moves use the Speed stat for damage calculations.
#define ABILITY_SIDEWINDER 676 // First biting move each entry gets +1 priority. Resets on KO.
#define ABILITY_PETRIFY 677 // Clears stat buffs then lowers speed by one stage on entry.
#define ABILITY_FLUFFIEST 678 // Quarters contact damage taken. 4x weak to fire.
#define ABILITY_WAY_OF_PRECISION 679 // Inner Focus + Precise Fist.
#define ABILITY_WAY_OF_SWIFTNESS 680 // Pretentious + Swift Swim.
#define ABILITY_ATOMIC_PUNCH 681 // Iron Fist + Steelworker.
#define ABILITY_IRON_GIANT 682 // Heatproof + Juggernaut.
#define ABILITY_MASTER_HAND 683 // Mega Launcher + Rampage.
#define ABILITY_FINAL_BLOW 684 // Unseen Fist + Fatal Precision.
#define ABILITY_HOSPITALITY 685 // Heals partner for 25% of its max HP on switch-in.
#define ABILITY_BUTTER_UP 686 // Combines Hospitality & Soothing Aroma
#define ABILITY_VITALITY_STRIKE 687 // Heals for 10% of the damage dealt by punching moves.
#define ABILITY_HUGE_WINGS 688 // Giant Wings + Levitate.
#define ABILITY_SWORD_OF_DAMNATION 689 // Unaware + Sword of Ruin.
#define ABILITY_RESTRAINING_ORDER 690 // Forces the attacker when hit once each switch-in.
#define ABILITY_ASSASSINS_TOOLS 691 // Contact moves have a 30% chance to PSN, PRLZ, or BLD.
#define ABILITY_FROSTMAW 692 // Biting moves have a 50% chance to inflict frostbite.
#define ABILITY_PATCHWORK 693 // Disguise + curses the opponent when its Disguise breaks.
#define ABILITY_BLIND_RAGE 694 // Combines Scrappy & Mold Breaker.
#define ABILITY_SLIPSTREAM 695 // Moves use 20% of its Speed stat additionally.
#define ABILITY_APEX_PREDATOR 696 // Combines Tough Claws & Predator.
#define ABILITY_DRAGONS_RITUAL 697 // Dealing a KO raises Attack and Speed by one stage.
#define ABILITY_PINNACLE_BLADE 698 // Slashing moves always hit and break protection and barriers.
#define ABILITY_ENERGIZED 699 // Generator + charges up on KO with an Electric-type move.
#define ABILITY_COLOR_SPECTRUM 700 // Same-type attacks get a 1.2x boost. Changes type each turn.
#define ABILITY_STEEL_BEETLE 701 // Raging Boxer + Pollinate.
#define ABILITY_FROM_THE_SHADOWS 702 // Attacks trap and have a 20% flinch chance when moving first.
#define ABILITY_RAGE_POINT 703 // Gets a 1.5x boost while statused. Raises offenses when crit.
#define ABILITY_HOT_COALS 704 // Sets a trap that burns the next foe that switches in.
#define ABILITY_TERASTAL_TREASURE 705 // Reduces damage taken by 40%, but lowers speed by 20%.
#define ABILITY_SHOCKING_MAW 706 // Strong Jaw + Bite moves have 50% paralysis chance.
#define ABILITY_GLEAM_EYES 707 // Frisk + Scare.
#define ABILITY_ROUSED_FANGS 708 // Biting moves use SpAtk and deal 30% more damage.
#define ABILITY_DREAM_STATE 709 // Immune to critical hits. Takes 20% less damage from all attacks.
#define ABILITY_DREAM_WHIMSY 710 // Uses Yawn on switch-in.
#define ABILITY_LUNAR_AFFINITY 711 // Copies lunar moves used by others.
#define ABILITY_FLAME_SHIELD 712 // Takes 35% less damage from Super-effective moves.
#define ABILITY_AQUATIC_DWELLER 713 // Boosts the power of Water-type moves by 1.5x.
#define ABILITY_APPLE_PIE 714 // Self Sufficient + Ripen.
#define ABILITY_HOVER 715 // Adds Psychic type to itself. Avoids Ground attacks.
#define ABILITY_DEPRAVITY 716 // Combines Merciless & Overcharge.
#define ABILITY_WILDFIRE 717 // Attacks with 20BP Fire Spin when hit by a contact move.
#define ABILITY_JUMP_SCARE 718 // Attacks with Astonish on switch-in.
#define ABILITY_TAR_TOSS 719 // Uses Tar Shot on switch-in.
#define ABILITY_STUN_SHOCK 720 // Attacks have a 60% chance to Paralyze or Poison.
#define ABILITY_RAGING_GODDESS 721 // Combines Rampage & Hyper Aggressive.
#define ABILITY_WHIPLASH 722 // Physical attacks have a 50% chance to lower Defense.
#define ABILITY_SUPERSWEET_SYRUP 723 // Can't lose its item. Disables foe's item for 2 turns on contact.
#define ABILITY_LUCKY_HALO 724 // Negates self stat drops. Survives the first hit that would KO it.
#define ABILITY_TRASH_HEAP 725 // Combines Corrosion & Toxic Spill.
#define ABILITY_SLUDGY_MIX 726 // Combines Intoxicate & Punk Rock.
#define ABILITY_OVERWATCH 727 // Combines On the Prowl & Stakeout.
#define ABILITY_WIND_RAGE 728 // Uses Defog on switch-in. Air- based moves get a 1.3x boost.
#define ABILITY_VICTORY_BOMB 729 // Attacks with a 100BP Fire-type Explosion on fainting.
#define ABILITY_RAZOR_SHARP 730 // Critical hits also inflict bleeding.
#define ABILITY_TO_THE_BONE 731 // Critical hits get a 1.5x boost and inflict bleeding.
#define ABILITY_BLADE_DANCE 732 // Triggers 50 BP Leaf Blade after using a dance move.
#define ABILITY_TAEKKYEON 733 // All attacks are dances.
#define ABILITY_APE_SHIFT 734 // Anger Point + Heal status and always crits with below 50% HP.
#define ABILITY_KNOW_YOUR_PLACE 735 // Contact attacks make foes move last for 5 turns.
#define ABILITY_DEEP_CUTS 736 // Slashing moves have a 50% chance to inflict bleeding.
#define ABILITY_LIFE_STEAL 737 // Steals 1/10 HP from foes each turn.
#define ABILITY_RUDE_AWAKENING 738 // Raises all stats becomes immune to sleep after waking up.
#define ABILITY_TERAFORM_ZERO 739 // Tera Shell + clears weather and terrain on first entry.
#define ABILITY_SET_ABLAZE 740 // Inflicting burn also inflicts fear.

#define ABILITIES_COUNT_CUSTOM (ABILITY_SET_ABLAZE + 1)

#define ABILITIES_COUNT ABILITIES_COUNT_CUSTOM

#endif  // GUARD_CONSTANTS_ABILITIES_H
