#include "constants/battle_script_commands.h"

#include "abilities.hh"
#include "battle.h"
#include "battle_ai_main.h"
#include "battle_ai_new.h"
#include "battle_ai_util.h"
#include "battle_anim.h"
#include "battle_arena.h"
#include "battle_controllers.h"
#include "battle_events.h"
#include "battle_interface.h"
#include "battle_main.h"
#include "battle_message.h"
#include "battle_pike.h"
#include "battle_pyramid.h"
#include "battle_scripts.h"
#include "battle_setup.h"
#include "battle_util.h"
#include "bg.h"
#include "constants/abilities.h"
#include "constants/battle_anim.h"
#include "constants/battle_config.h"
#include "constants/battle_move_effects.h"
#include "constants/battle_string_ids.h"
#include "constants/hold_effects.h"
#include "constants/items.h"
#include "constants/map_types.h"
#include "constants/moves.h"
#include "constants/party_menu.h"
#include "constants/rgb.h"
#include "constants/songs.h"
#include "constants/species.h"
#include "constants/trainers.h"
#include "data.h"
#include "event_data.h"
#include "field_specials.h"
#include "global.h"
#include "item.h"
#include "m4a.h"
#include "mail.h"
#include "main.h"
#include "menu_specialized.h"
#include "mgba_printf/mgba.h"
#include "mgba_printf/mini_printf.h"
#include "money.h"
#include "naming_screen.h"
#include "overworld.h"
#include "palette.h"
#include "party_menu.h"
#include "pokedex.h"
#include "pokemon.h"
#include "pokemon_icon.h"
#include "pokemon_storage_system.h"
#include "pokemon_summary_screen.h"
#include "pokenav.h"
#include "random.h"
#include "recorded_battle.h"
#include "reshow_battle_screen.h"
#include "rtc.h"
#include "sound.h"
#include "string_util.h"
#include "task.h"
#include "text.h"
#include "util.h"
#include "wild_encounter.h"
#include "window.h"

extern struct MusicPlayerInfo gMPlayInfo_BGM;

extern const u8* const gBattleScriptsForMoveEffects[];

// table to avoid ugly powing on gba (courtesy of doesnt)
// this returns (i^2.5)/4
// the quarters cancel so no need to re-quadruple them in actual calculation
static const s32 sExperienceScalingFactors[] = {
    0,      0,      1,      3,      8,      13,     22,     32,     45,     60,     79,     100,    124,    152,    183,    217,    256,    297,
    343,    393,    447,    505,    567,    634,    705,    781,    861,    946,    1037,   1132,   1232,   1337,   1448,   1563,   1685,   1811,
    1944,   2081,   2225,   2374,   2529,   2690,   2858,   3031,   3210,   3396,   3587,   3786,   3990,   4201,   4419,   4643,   4874,   5112,
    5357,   5608,   5866,   6132,   6404,   6684,   6971,   7265,   7566,   7875,   8192,   8515,   8847,   9186,   9532,   9886,   10249,  10619,
    10996,  11382,  11776,  12178,  12588,  13006,  13433,  13867,  14310,  14762,  15222,  15690,  16167,  16652,  17146,  17649,  18161,  18681,
    19210,  19748,  20295,  20851,  21417,  21991,  22574,  23166,  23768,  24379,  25000,  25629,  26268,  26917,  27575,  28243,  28920,  29607,
    30303,  31010,  31726,  32452,  33188,  33934,  34689,  35455,  36231,  37017,  37813,  38619,  39436,  40262,  41099,  41947,  42804,  43673,
    44551,  45441,  46340,  47251,  48172,  49104,  50046,  50999,  51963,  52938,  53924,  54921,  55929,  56947,  57977,  59018,  60070,  61133,
    62208,  63293,  64390,  65498,  66618,  67749,  68891,  70045,  71211,  72388,  73576,  74777,  75989,  77212,  78448,  79695,  80954,  82225,
    83507,  84802,  86109,  87427,  88758,  90101,  91456,  92823,  94202,  95593,  96997,  98413,  99841,  101282, 102735, 104201, 105679, 107169,
    108672, 110188, 111716, 113257, 114811, 116377, 117956, 119548, 121153, 122770, 124401, 126044, 127700, 129369, 131052, 132747, 134456, 136177,
    137912, 139660, 141421, 143195, 144983, 146784, 148598, 150426, 152267, 154122, 155990, 157872, 159767,
};

static const u16 sTrappingMoves[] = {
    MOVE_BIND,
    MOVE_WRAP,
    MOVE_FIRE_SPIN,
    MOVE_CLAMP,
    MOVE_WHIRLPOOL,
    MOVE_SAND_TOMB,
    MOVE_MAGMA_STORM,
    MOVE_INFESTATION,
    MOVE_THUNDER_CAGE,
    MOVE_SNAP_TRAP,
    0xFFFF,
};

// this file's functions
static bool8 IsTwoTurnsMove(u16 move);
static void TrySetDestinyBondToHappen(void);
static u8 AttacksThisTurn(u8 battlerId, u16 move);  // Note: returns 1 if it's a charging turn, otherwise 2.
static bool32 IsMonGettingExpSentOut(void);
static void sub_804F17C(void);
static bool8 sub_804F1CC(void);
static void DrawLevelUpWindow1(void);
static void DrawLevelUpWindow2(void);
static bool8 sub_804F344(void);
static void PutMonIconOnLvlUpBox(void);
static void PutLevelAndGenderOnLvlUpBox(void);
static bool32 CriticalCapture(u32 odds);
static bool8 DisableLastUsedMove(u8 battlerTarget);

static void SpriteCB_MonIconOnLvlUpBox(struct Sprite* sprite);

static void Cmd_attackcanceler(void);
static void Cmd_accuracycheck(void);
static void Cmd_attackstring(void);
static void Cmd_ppreduce(void);
static void Cmd_critcalc(void);
static void Cmd_damagecalc(void);
static void Cmd_typecalc(void);
static void Cmd_adjustdamage(void);
static void Cmd_multihitresultmessage(void);
static void Cmd_attackanimation(void);
static void Cmd_waitanimation(void);
static void Cmd_healthbarupdate(void);
static void Cmd_datahpupdate(void);
static void Cmd_critmessage(void);
static void Cmd_effectivenesssound(void);
static void Cmd_resultmessage(void);
static void Cmd_printstring(void);
static void Cmd_printselectionstring(void);
static void Cmd_waitmessage(void);
static void Cmd_printfromtable(void);
static void Cmd_printselectionstringfromtable(void);
static void Cmd_seteffectwithchance(void);
static void Cmd_seteffectprimary(void);
static void Cmd_seteffectsecondary(void);
static void Cmd_clearstatusfromeffect(void);
static void Cmd_tryfaintmon(void);
static void Cmd_dofaintanimation(void);
static void Cmd_cleareffectsonfaint(void);
static void Cmd_jumpifstatus(void);
static void Cmd_jumpifstatus2(void);
static void Cmd_jumpifability(void);
static void Cmd_jumpifsideaffecting(void);
static void Cmd_jumpifstat(void);
static void Cmd_jumpifstatus3condition(void);
static void Cmd_jumpbasedontype(void);
static void Cmd_getexp(void);
static void Cmd_checkteamslost(void);
static void Cmd_movevaluescleanup(void);
static void Cmd_setmultihit(void);
static void Cmd_decrementmultihit(void);
static void Cmd_goto(void);
static void Cmd_jumpifbyte(void);
static void Cmd_jumpifhalfword(void);
static void Cmd_jumpifword(void);
static void Cmd_jumpifarrayequal(void);
static void Cmd_jumpifarraynotequal(void);
static void Cmd_setbyte(void);
static void Cmd_addbyte(void);
static void Cmd_subbyte(void);
static void Cmd_copyarray(void);
static void Cmd_copyarraywithindex(void);
static void Cmd_orbyte(void);
static void Cmd_orhalfword(void);
static void Cmd_orword(void);
static void Cmd_bicbyte(void);
static void Cmd_bichalfword(void);
static void Cmd_bicword(void);
static void Cmd_pause(void);
static void Cmd_waitstate(void);
static void Cmd_healthbar_update(void);
static void Cmd_return(void);
static void Cmd_end(void);
static void Cmd_end2(void);
static void Cmd_end3(void);
static void Cmd_jumpifaffectedbyprotect(void);
static void Cmd_call(void);
static void Cmd_setroost(void);
static void Cmd_jumpifabilitypresent(void);
static void Cmd_endselectionscript(void);
static void Cmd_playanimation(void);
static void Cmd_playanimation2(void);
static void Cmd_setgraphicalstatchangevalues(void);
static void Cmd_playstatchangeanimation(void);
static void Cmd_moveend(void);
static void Cmd_sethealblock(void);
static void Cmd_returnatktoball(void);
static void Cmd_getswitchedmondata(void);
static void Cmd_switchindataupdate(void);
static void Cmd_switchinanim(void);
static void Cmd_jumpifcantswitch(void);
static void Cmd_openpartyscreen(void);
static void Cmd_switchhandleorder(void);
static void Cmd_switchineffects(void);
static void Cmd_trainerslidein(void);
static void Cmd_playse(void);
static void Cmd_fanfare(void);
static void Cmd_playfaintcry(void);
static void Cmd_endlinkbattle(void);
static void Cmd_returntoball(void);
static void Cmd_handlelearnnewmove(void);
static void Cmd_yesnoboxlearnmove(void);
static void Cmd_yesnoboxstoplearningmove(void);
static void Cmd_hitanimation(void);
static void Cmd_getmoneyreward(void);
static void Cmd_unknown_5E(void);
static void Cmd_swapattackerwithtarget(void);
static void Cmd_incrementgamestat(void);
static void Cmd_drawpartystatussummary(void);
static void Cmd_hidepartystatussummary(void);
static void Cmd_jumptocalledmove(void);
static void Cmd_statusanimation(void);
static void Cmd_status2animation(void);
static void Cmd_chosenstatusanimation(void);
static void Cmd_yesnobox(void);
static void Cmd_cancelallactions(void);
static void Cmd_setgravity(void);
static void Cmd_removeitem(void);
static void Cmd_atknameinbuff1(void);
static void Cmd_drawlvlupbox(void);
static void Cmd_resetsentmonsvalue(void);
static void Cmd_setatktoplayer0(void);
static void Cmd_makevisible(void);
static void Cmd_recordability(void);
static void Cmd_buffermovetolearn(void);
static void Cmd_jumpifplayerran(void);
static void Cmd_hpthresholds(void);
static void Cmd_hpthresholds2(void);
static void Cmd_useitemonopponent(void);
static void Cmd_various(void);
static void Cmd_setprotectlike(void);
static void Cmd_faintifabilitynotdamp(void);
static void Cmd_setatkhptozero(void);
static void Cmd_jumpifnexttargetvalid(void);
static void Cmd_tryhealhalfhealth(void);
static void Cmd_trymirrormove(void);
static void Cmd_setrain(void);
static void Cmd_setreflect(void);
static void Cmd_setseeded(void);
static void Cmd_manipulatedamage(void);
static void Cmd_trysetrest(void);
static void Cmd_jumpifnotfirstturn(void);
static void Cmd_setmiracleeye(void);
static void Cmd_jumpifcantmakeasleep(void);
static void Cmd_stockpile(void);
static void Cmd_stockpiletobasedamage(void);
static void Cmd_stockpiletohpheal(void);
static void Cmd_checkcondition(void);
static void Cmd_statbuffchange(void);
static void Cmd_normalisebuffs(void);
static void Cmd_setbide(void);
static void Cmd_confuseifrepeatingattackends(void);
static void Cmd_setmultihitcounter(void);
static void Cmd_initmultihitstring(void);
static void Cmd_forcerandomswitch(void);
static void Cmd_tryconversiontypechange(void);
static void Cmd_givepaydaymoney(void);
static void Cmd_setlightscreen(void);
static void Cmd_battlemacros(void);
static void Cmd_jumpifabilityonside(void);
static void Cmd_setsandstorm(void);
static void Cmd_weatherdamage(void);
static void Cmd_tryinfatuating(void);
static void Cmd_updatestatusicon(void);
static void Cmd_setmist(void);
static void Cmd_setfocusenergy(void);
static void Cmd_transformdataexecution(void);
static void Cmd_setsubstitute(void);
static void Cmd_mimicattackcopy(void);
static void Cmd_metronome(void);
static void Cmd_calculatesetdamage(void);
static void Cmd_trytoapplymoveeffect(void);
static void Cmd_counterdamagecalculator(void);
static void Cmd_mirrorcoatdamagecalculator(void);
static void Cmd_disablelastusedattack(void);
static void Cmd_trysetencore(void);
static void Cmd_painsplitdmgcalc(void);
static void Cmd_settypetorandomresistance(void);
static void Cmd_setalwayshitflag(void);
static void Cmd_copymovepermanently(void);
static void Cmd_trychoosesleeptalkmove(void);
static void Cmd_setdestinybond(void);
static void Cmd_trysetdestinybondtohappen(void);
static void Cmd_settailwind(void);
static void Cmd_tryspiteppreduce(void);
static void Cmd_healpartystatus(void);
static void Cmd_cursetarget(void);
static void Cmd_trysetspikes(void);
static void Cmd_setforesight(void);
static void Cmd_trysetperishsong(void);
static void Cmd_handlerollout(void);
static void Cmd_jumpifconfusedandstatmaxed(void);
static void Cmd_handlefurycutter(void);
static void Cmd_setembargo(void);
static void Cmd_presentdamagecalculation(void);
static void Cmd_setsafeguard(void);
static void Cmd_magnitudedamagecalculation(void);
static void Cmd_jumpifnopursuitswitchdmg(void);
static void Cmd_setsunny(void);
static void Cmd_maxattackhalvehp(void);
static void Cmd_copyfoestats(void);
static void Cmd_rapidspinfree(void);
static void Cmd_setdefensecurlbit(void);
static void Cmd_recoverbasedonsunlight(void);
static void Cmd_setstickyweb(void);
static void Cmd_selectfirstvalidtarget(void);
static void Cmd_trysetfutureattack(void);
static void Cmd_trydobeatup(void);
static void Cmd_setsemiinvulnerablebit(void);
static void Cmd_clearsemiinvulnerablebit(void);
static void Cmd_setminimize(void);
static void Cmd_sethail(void);
static void Cmd_jumpifattackandspecialattackcannotfall(void);
static void Cmd_setforcedtarget(void);
static void Cmd_setcharge(void);
static void Cmd_callterrainattack(void);
static void Cmd_cureifburnedparalysedorpoisoned(void);
static void Cmd_settorment(void);
static void Cmd_jumpifnodamage(void);
static void Cmd_settaunt(void);
static void Cmd_trysethelpinghand(void);
static void Cmd_tryswapitems(void);
static void Cmd_trycopyability(void);
static void Cmd_trywish(void);
static void Cmd_settoxicspikes(void);
static void Cmd_setgastroacid(void);
static void Cmd_setyawn(void);
static void Cmd_setdamagetohealthdifference(void);
static void Cmd_setroom(void);
static void Cmd_tryswapabilities(void);
static void Cmd_tryimprison(void);
static void Cmd_setstealthrock(void);
static void Cmd_setuserstatus3(void);
static void Cmd_assistattackselect(void);
static void Cmd_trysetmagiccoat(void);
static void Cmd_trysetsnatch(void);
static void Cmd_trygetintimidatetarget(void);
static void Cmd_switchoutabilities(void);
static void Cmd_jumpifhasnohp(void);
static void Cmd_getsecretpowereffect(void);
static void Cmd_pickup(void);
static void Cmd_docastformchangeanimation(void);
static void Cmd_trycastformdatachange(void);
static void Cmd_settypebasedhalvers(void);
static void Cmd_jumpifsubstituteblocks(void);
static void Cmd_tryrecycleitem(void);
static void Cmd_settypetoterrain(void);
static void Cmd_unused_pursuitrelated(void);
static void Cmd_snatchsetbattlers(void);
static void Cmd_removelightscreenreflect(void);
static void Cmd_handleballthrow(void);
static void Cmd_givecaughtmon(void);
static void Cmd_trysetcaughtmondexflags(void);
static void Cmd_displaydexinfo(void);
static void Cmd_trygivecaughtmonnick(void);
static void Cmd_subattackerhpbydmg(void);
static void Cmd_removeattackerstatus1(void);
static void Cmd_finishaction(void);
static void Cmd_finishturn(void);
static void Cmd_trainerslideout(void);
static void Cmd_settelekinesis(void);
static void Cmd_swapstatstages(void);
static void Cmd_averagestats(void);
static void Cmd_jumpifoppositegenders(void);
static void Cmd_trygetbaddreamstarget(void);
static void Cmd_tryworryseed(void);
static void Cmd_metalburstdamagecalculator(void);
static int TryUseStockpile(int battler);
extern u8 gMaxPartyLevel;

static const u16 sBadgeFlags[8] = {
    FLAG_BADGE01_GET,
    FLAG_BADGE02_GET,
    FLAG_BADGE03_GET,
    FLAG_BADGE04_GET,
    FLAG_BADGE05_GET,
    FLAG_BADGE06_GET,
    FLAG_BADGE07_GET,
    FLAG_BADGE08_GET,
};
static const u16 sWhiteOutBadgeMoney[9] = {8, 16, 24, 36, 48, 60, 80, 100, 120};

void (*const gBattleScriptingCommandsTable[])(void) = {
    Cmd_attackcanceler,                          // 0x0
    Cmd_accuracycheck,                           // 0x1
    Cmd_attackstring,                            // 0x2
    Cmd_ppreduce,                                // 0x3
    Cmd_critcalc,                                // 0x4
    Cmd_damagecalc,                              // 0x5
    Cmd_typecalc,                                // 0x6
    Cmd_adjustdamage,                            // 0x7
    Cmd_multihitresultmessage,                   // 0x8
    Cmd_attackanimation,                         // 0x9
    Cmd_waitanimation,                           // 0xA
    Cmd_healthbarupdate,                         // 0xB
    Cmd_datahpupdate,                            // 0xC
    Cmd_critmessage,                             // 0xD
    Cmd_effectivenesssound,                      // 0xE
    Cmd_resultmessage,                           // 0xF
    Cmd_printstring,                             // 0x10
    Cmd_printselectionstring,                    // 0x11
    Cmd_waitmessage,                             // 0x12
    Cmd_printfromtable,                          // 0x13
    Cmd_printselectionstringfromtable,           // 0x14
    Cmd_seteffectwithchance,                     // 0x15
    Cmd_seteffectprimary,                        // 0x16
    Cmd_seteffectsecondary,                      // 0x17
    Cmd_clearstatusfromeffect,                   // 0x18
    Cmd_tryfaintmon,                             // 0x19
    Cmd_dofaintanimation,                        // 0x1A
    Cmd_cleareffectsonfaint,                     // 0x1B
    Cmd_jumpifstatus,                            // 0x1C
    Cmd_jumpifstatus2,                           // 0x1D
    Cmd_jumpifability,                           // 0x1E
    Cmd_jumpifsideaffecting,                     // 0x1F
    Cmd_jumpifstat,                              // 0x20
    Cmd_jumpifstatus3condition,                  // 0x21
    Cmd_jumpbasedontype,                         // 0x22
    Cmd_getexp,                                  // 0x23
    Cmd_checkteamslost,                          // 0x24
    Cmd_movevaluescleanup,                       // 0x25
    Cmd_setmultihit,                             // 0x26
    Cmd_decrementmultihit,                       // 0x27
    Cmd_goto,                                    // 0x28
    Cmd_jumpifbyte,                              // 0x29
    Cmd_jumpifhalfword,                          // 0x2A
    Cmd_jumpifword,                              // 0x2B
    Cmd_jumpifarrayequal,                        // 0x2C
    Cmd_jumpifarraynotequal,                     // 0x2D
    Cmd_setbyte,                                 // 0x2E
    Cmd_addbyte,                                 // 0x2F
    Cmd_subbyte,                                 // 0x30
    Cmd_copyarray,                               // 0x31
    Cmd_copyarraywithindex,                      // 0x32
    Cmd_orbyte,                                  // 0x33
    Cmd_orhalfword,                              // 0x34
    Cmd_orword,                                  // 0x35
    Cmd_bicbyte,                                 // 0x36
    Cmd_bichalfword,                             // 0x37
    Cmd_bicword,                                 // 0x38
    Cmd_pause,                                   // 0x39
    Cmd_waitstate,                               // 0x3A
    Cmd_healthbar_update,                        // 0x3B
    Cmd_return,                                  // 0x3C
    Cmd_end,                                     // 0x3D
    Cmd_end2,                                    // 0x3E
    Cmd_end3,                                    // 0x3F
    Cmd_jumpifaffectedbyprotect,                 // 0x40
    Cmd_call,                                    // 0x41
    Cmd_setroost,                                // 0x42
    Cmd_jumpifabilitypresent,                    // 0x43
    Cmd_endselectionscript,                      // 0x44
    Cmd_playanimation,                           // 0x45
    Cmd_playanimation2,                          // 0x46
    Cmd_setgraphicalstatchangevalues,            // 0x47
    Cmd_playstatchangeanimation,                 // 0x48
    Cmd_moveend,                                 // 0x49
    Cmd_sethealblock,                            // 0x4A
    Cmd_returnatktoball,                         // 0x4B
    Cmd_getswitchedmondata,                      // 0x4C
    Cmd_switchindataupdate,                      // 0x4D
    Cmd_switchinanim,                            // 0x4E
    Cmd_jumpifcantswitch,                        // 0x4F
    Cmd_openpartyscreen,                         // 0x50
    Cmd_switchhandleorder,                       // 0x51
    Cmd_switchineffects,                         // 0x52
    Cmd_trainerslidein,                          // 0x53
    Cmd_playse,                                  // 0x54
    Cmd_fanfare,                                 // 0x55
    Cmd_playfaintcry,                            // 0x56
    Cmd_endlinkbattle,                           // 0x57
    Cmd_returntoball,                            // 0x58
    Cmd_handlelearnnewmove,                      // 0x59
    Cmd_yesnoboxlearnmove,                       // 0x5A
    Cmd_yesnoboxstoplearningmove,                // 0x5B
    Cmd_hitanimation,                            // 0x5C
    Cmd_getmoneyreward,                          // 0x5D
    Cmd_unknown_5E,                              // 0x5E
    Cmd_swapattackerwithtarget,                  // 0x5F
    Cmd_incrementgamestat,                       // 0x60
    Cmd_drawpartystatussummary,                  // 0x61
    Cmd_hidepartystatussummary,                  // 0x62
    Cmd_jumptocalledmove,                        // 0x63
    Cmd_statusanimation,                         // 0x64
    Cmd_status2animation,                        // 0x65
    Cmd_chosenstatusanimation,                   // 0x66
    Cmd_yesnobox,                                // 0x67
    Cmd_cancelallactions,                        // 0x68
    Cmd_setgravity,                              // 0x69
    Cmd_removeitem,                              // 0x6A
    Cmd_atknameinbuff1,                          // 0x6B
    Cmd_drawlvlupbox,                            // 0x6C
    Cmd_resetsentmonsvalue,                      // 0x6D
    Cmd_setatktoplayer0,                         // 0x6E
    Cmd_makevisible,                             // 0x6F
    Cmd_recordability,                           // 0x70
    Cmd_buffermovetolearn,                       // 0x71
    Cmd_jumpifplayerran,                         // 0x72
    Cmd_hpthresholds,                            // 0x73
    Cmd_hpthresholds2,                           // 0x74
    Cmd_useitemonopponent,                       // 0x75
    Cmd_various,                                 // 0x76
    Cmd_setprotectlike,                          // 0x77
    Cmd_faintifabilitynotdamp,                   // 0x78
    Cmd_setatkhptozero,                          // 0x79
    Cmd_jumpifnexttargetvalid,                   // 0x7A
    Cmd_tryhealhalfhealth,                       // 0x7B
    Cmd_trymirrormove,                           // 0x7C
    Cmd_setrain,                                 // 0x7D
    Cmd_setreflect,                              // 0x7E
    Cmd_setseeded,                               // 0x7F
    Cmd_manipulatedamage,                        // 0x80
    Cmd_trysetrest,                              // 0x81
    Cmd_jumpifnotfirstturn,                      // 0x82
    Cmd_setmiracleeye,                           // 0x83
    Cmd_jumpifcantmakeasleep,                    // 0x84
    Cmd_stockpile,                               // 0x85
    Cmd_stockpiletobasedamage,                   // 0x86
    Cmd_stockpiletohpheal,                       // 0x87
    Cmd_checkcondition,                          // 0x88
    Cmd_statbuffchange,                          // 0x89
    Cmd_normalisebuffs,                          // 0x8A
    Cmd_setbide,                                 // 0x8B
    Cmd_confuseifrepeatingattackends,            // 0x8C
    Cmd_setmultihitcounter,                      // 0x8D
    Cmd_initmultihitstring,                      // 0x8E
    Cmd_forcerandomswitch,                       // 0x8F
    Cmd_tryconversiontypechange,                 // 0x90
    Cmd_givepaydaymoney,                         // 0x91
    Cmd_setlightscreen,                          // 0x92
    Cmd_battlemacros,                            // 0x93
    Cmd_jumpifabilityonside,                     // 0x94
    Cmd_setsandstorm,                            // 0x95
    Cmd_weatherdamage,                           // 0x96
    Cmd_tryinfatuating,                          // 0x97
    Cmd_updatestatusicon,                        // 0x98
    Cmd_setmist,                                 // 0x99
    Cmd_setfocusenergy,                          // 0x9A
    Cmd_transformdataexecution,                  // 0x9B
    Cmd_setsubstitute,                           // 0x9C
    Cmd_mimicattackcopy,                         // 0x9D
    Cmd_metronome,                               // 0x9E
    Cmd_calculatesetdamage,                      // 0x9F
    Cmd_trytoapplymoveeffect,                    // 0xA0
    Cmd_counterdamagecalculator,                 // 0xA1
    Cmd_mirrorcoatdamagecalculator,              // 0xA2
    Cmd_disablelastusedattack,                   // 0xA3
    Cmd_trysetencore,                            // 0xA4
    Cmd_painsplitdmgcalc,                        // 0xA5
    Cmd_settypetorandomresistance,               // 0xA6
    Cmd_setalwayshitflag,                        // 0xA7
    Cmd_copymovepermanently,                     // 0xA8
    Cmd_trychoosesleeptalkmove,                  // 0xA9
    Cmd_setdestinybond,                          // 0xAA
    Cmd_trysetdestinybondtohappen,               // 0xAB
    Cmd_settailwind,                             // 0xAC
    Cmd_tryspiteppreduce,                        // 0xAD
    Cmd_healpartystatus,                         // 0xAE
    Cmd_cursetarget,                             // 0xAF
    Cmd_trysetspikes,                            // 0xB0
    Cmd_setforesight,                            // 0xB1
    Cmd_trysetperishsong,                        // 0xB2
    Cmd_handlerollout,                           // 0xB3
    Cmd_jumpifconfusedandstatmaxed,              // 0xB4
    Cmd_handlefurycutter,                        // 0xB5
    Cmd_setembargo,                              // 0xB6
    Cmd_presentdamagecalculation,                // 0xB7
    Cmd_setsafeguard,                            // 0xB8
    Cmd_magnitudedamagecalculation,              // 0xB9
    Cmd_jumpifnopursuitswitchdmg,                // 0xBA
    Cmd_setsunny,                                // 0xBB
    Cmd_maxattackhalvehp,                        // 0xBC
    Cmd_copyfoestats,                            // 0xBD
    Cmd_rapidspinfree,                           // 0xBE
    Cmd_setdefensecurlbit,                       // 0xBF
    Cmd_recoverbasedonsunlight,                  // 0xC0
    Cmd_setstickyweb,                            // 0xC1
    Cmd_selectfirstvalidtarget,                  // 0xC2
    Cmd_trysetfutureattack,                      // 0xC3
    Cmd_trydobeatup,                             // 0xC4
    Cmd_setsemiinvulnerablebit,                  // 0xC5
    Cmd_clearsemiinvulnerablebit,                // 0xC6
    Cmd_setminimize,                             // 0xC7
    Cmd_sethail,                                 // 0xC8
    Cmd_jumpifattackandspecialattackcannotfall,  // 0xC9
    Cmd_setforcedtarget,                         // 0xCA
    Cmd_setcharge,                               // 0xCB
    Cmd_callterrainattack,                       // 0xCC
    Cmd_cureifburnedparalysedorpoisoned,         // 0xCD
    Cmd_settorment,                              // 0xCE
    Cmd_jumpifnodamage,                          // 0xCF
    Cmd_settaunt,                                // 0xD0
    Cmd_trysethelpinghand,                       // 0xD1
    Cmd_tryswapitems,                            // 0xD2
    Cmd_trycopyability,                          // 0xD3
    Cmd_trywish,                                 // 0xD4
    Cmd_settoxicspikes,                          // 0xD5
    Cmd_setgastroacid,                           // 0xD6
    Cmd_setyawn,                                 // 0xD7
    Cmd_setdamagetohealthdifference,             // 0xD8
    Cmd_setroom,                                 // 0xD9
    Cmd_tryswapabilities,                        // 0xDA
    Cmd_tryimprison,                             // 0xDB
    Cmd_setstealthrock,                          // 0xDC
    Cmd_setuserstatus3,                          // 0xDD
    Cmd_assistattackselect,                      // 0xDE
    Cmd_trysetmagiccoat,                         // 0xDF
    Cmd_trysetsnatch,                            // 0xE0
    Cmd_trygetintimidatetarget,                  // 0xE1
    Cmd_switchoutabilities,                      // 0xE2
    Cmd_jumpifhasnohp,                           // 0xE3
    Cmd_getsecretpowereffect,                    // 0xE4
    Cmd_pickup,                                  // 0xE5
    Cmd_docastformchangeanimation,               // 0xE6
    Cmd_trycastformdatachange,                   // 0xE7
    Cmd_settypebasedhalvers,                     // 0xE8
    Cmd_jumpifsubstituteblocks,                  // 0xE9
    Cmd_tryrecycleitem,                          // 0xEA
    Cmd_settypetoterrain,                        // 0xEB
    Cmd_unused_pursuitrelated,                   // 0xEC
    Cmd_snatchsetbattlers,                       // 0xED
    Cmd_removelightscreenreflect,                // 0xEE
    Cmd_handleballthrow,                         // 0xEF
    Cmd_givecaughtmon,                           // 0xF0
    Cmd_trysetcaughtmondexflags,                 // 0xF1
    Cmd_displaydexinfo,                          // 0xF2
    Cmd_trygivecaughtmonnick,                    // 0xF3
    Cmd_subattackerhpbydmg,                      // 0xF4
    Cmd_removeattackerstatus1,                   // 0xF5
    Cmd_finishaction,                            // 0xF6
    Cmd_finishturn,                              // 0xF7
    Cmd_trainerslideout,                         // 0xF8
    Cmd_settelekinesis,                          // 0xF9
    Cmd_swapstatstages,                          // 0xFA
    Cmd_averagestats,                            // 0xFB
    Cmd_jumpifoppositegenders,                   // 0xFC
    Cmd_trygetbaddreamstarget,                   // 0xFD
    Cmd_tryworryseed,                            // 0xFE
    Cmd_metalburstdamagecalculator,              // 0xFF
};

const struct StatFractions gAccuracyStageRatios[] = {
    {1, 3},  // -6
    {3, 8},  // -5
    {3, 7},  // -4
    {1, 2},  // -3
    {2, 3},  // -2
    {3, 4},  // -1
    {1, 1},  //  0
    {4, 3},  // +1
    {3, 2},  // +2
    {2, 1},  // +3
    {7, 3},  // +4
    {8, 3},  // +5
    {3, 1},  // +6
};

static const u32 sStatusFlagsForMoveEffects[NUM_MOVE_EFFECTS] = {
    [MOVE_EFFECT_SLEEP] = STATUS1_SLEEP,         [MOVE_EFFECT_POISON] = STATUS1_POISON,       [MOVE_EFFECT_BURN] = STATUS1_BURN,
    [MOVE_EFFECT_FREEZE] = STATUS1_FREEZE,       [MOVE_EFFECT_PARALYSIS] = STATUS1_PARALYSIS, [MOVE_EFFECT_TOXIC] = STATUS1_TOXIC_POISON,
    [MOVE_EFFECT_FROSTBITE] = STATUS1_FROSTBITE, [MOVE_EFFECT_BLEED] = STATUS1_BLEED,         [MOVE_EFFECT_CONFUSION] = STATUS2_CONFUSION,
    [MOVE_EFFECT_FLINCH] = STATUS2_FLINCHED,     [MOVE_EFFECT_UPROAR] = STATUS2_UPROAR,       [MOVE_EFFECT_CHARGING] = STATUS2_MULTIPLETURNS,
    [MOVE_EFFECT_WRAP] = STATUS2_WRAPPED,        [MOVE_EFFECT_RECHARGE] = STATUS2_RECHARGE,   [MOVE_EFFECT_PREVENT_ESCAPE] = STATUS2_ESCAPE_PREVENTION,
    [MOVE_EFFECT_NIGHTMARE] = STATUS2_NIGHTMARE, [MOVE_EFFECT_THRASH] = STATUS2_LOCK_CONFUSE, [MOVE_EFFECT_ATTRACT] = STATUS2_INFATUATION,
    [MOVE_EFFECT_CURSE] = STATUS2_CURSED,
};

static const u8* const sMoveEffectBS_Ptrs[] = {
    [MOVE_EFFECT_SLEEP] = BattleScript_MoveEffectSleep,
    [MOVE_EFFECT_POISON] = BattleScript_MoveEffectPoison,
    [MOVE_EFFECT_BURN] = BattleScript_MoveEffectBurn,
    [MOVE_EFFECT_FREEZE] = BattleScript_MoveEffectFreeze,
    [MOVE_EFFECT_PARALYSIS] = BattleScript_MoveEffectParalysis,
    [MOVE_EFFECT_TOXIC] = BattleScript_MoveEffectToxic,
    [MOVE_EFFECT_CONFUSION] = BattleScript_MoveEffectConfusion,
    [MOVE_EFFECT_UPROAR] = BattleScript_MoveEffectUproar,
    [MOVE_EFFECT_PAYDAY] = BattleScript_MoveEffectPayDay,
    [MOVE_EFFECT_WRAP] = BattleScript_MoveEffectWrap,
    [MOVE_EFFECT_FROSTBITE] = BattleScript_MoveEffectFrostbite,
    [MOVE_EFFECT_BLEED] = BattleScript_MoveEffectBleed,
    [MOVE_EFFECT_ATTRACT] = BattleScript_MoveEffectAttract,
    [MOVE_EFFECT_CURSE] = BattleScript_MoveEffectCurse,
};

static const struct WindowTemplate sUnusedWinTemplate = {0, 1, 3, 7, 0xF, 0x1F, 0x3F};

static const u16 sUnknown_0831C2C8[] = INCBIN_U16("graphics/battle_interface/unk_battlebox.gbapal");
static const u32 sUnknown_0831C2E8[] = INCBIN_U32("graphics/battle_interface/unk_battlebox.4bpp.lz");

#define MON_ICON_LVLUP_BOX_TAG 0xD75A

static const struct OamData sOamData_MonIconOnLvlUpBox = {
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = 0,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(32x32),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(32x32),
    .tileNum = 0,
    .priority = 0,
    .paletteNum = 0,
    .affineParam = 0,
};

static const struct SpriteTemplate sSpriteTemplate_MonIconOnLvlUpBox = {
    .tileTag = MON_ICON_LVLUP_BOX_TAG,
    .paletteTag = MON_ICON_LVLUP_BOX_TAG,
    .oam = &sOamData_MonIconOnLvlUpBox,
    .anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCB_MonIconOnLvlUpBox,
};

static const u16 sProtectSuccessRates[] = {USHRT_MAX, USHRT_MAX / 2, USHRT_MAX / 4, USHRT_MAX / 8};

#define FORBIDDEN_MIMIC (1 << 0)
#define FORBIDDEN_METRONOME (1 << 1)
#define FORBIDDEN_ASSIST (1 << 2)
#define FORBIDDEN_COPYCAT (1 << 3)
#define FORBIDDEN_SLEEP_TALK (1 << 4)

#define FORBIDDEN_INSTRUCT_END 0xFFFF

static const u16 sMoveEffectsForbiddenToInstruct[] = {
    EFFECT_ASSIST,
    // EFFECT_BEAK_BLAST,
    EFFECT_BELCH,
    EFFECT_BIDE,
    // EFFECT_CELEBRATE,
    // EFFECT_CHATTER,
    EFFECT_COPYCAT,
    // EFFECT_DYNAMAX_CANNON,
    EFFECT_FOCUS_PUNCH,
    EFFECT_GEOMANCY,
    // EFFECT_HOLD_HANDS,
    EFFECT_INSTRUCT,
    EFFECT_ME_FIRST,
    EFFECT_METRONOME,
    EFFECT_MIMIC,
    EFFECT_MIRROR_MOVE,
    EFFECT_NATURE_POWER,
    // EFFECT_OBSTRUCT,
    EFFECT_RAMPAGE,
    EFFECT_RECHARGE,
    EFFECT_RECOIL_25,
    EFFECT_ROLLOUT,
    EFFECT_SEMI_INVULNERABLE,
    // EFFECT_SHELL_TRAP,
    EFFECT_SKETCH,
    // EFFECT_SKY_DROP,
    EFFECT_SKULL_BASH,
    EFFECT_SLEEP_TALK,
    EFFECT_SOLARBEAM,
    EFFECT_TRANSFORM,
    EFFECT_TWO_TURNS_ATTACK,
    EFFECT_UPROAR,
    FORBIDDEN_INSTRUCT_END,
};

static const u16 sNaturePowerMoves[BATTLE_TERRAIN_COUNT] = {
#if B_NATURE_POWER_MOVES >= GEN_7
    [BATTLE_TERRAIN_GRASS] = MOVE_ENERGY_BALL,     [BATTLE_TERRAIN_LONG_GRASS] = MOVE_ENERGY_BALL,
    [BATTLE_TERRAIN_SAND] = MOVE_EARTH_POWER,      [BATTLE_TERRAIN_WATER] = MOVE_HYDRO_PUMP,
    [BATTLE_TERRAIN_POND] = MOVE_HYDRO_PUMP,       [BATTLE_TERRAIN_MOUNTAIN] = MOVE_EARTH_POWER,
    [BATTLE_TERRAIN_CAVE] = MOVE_EARTH_POWER,      [BATTLE_TERRAIN_BUILDING] = MOVE_TRI_ATTACK,
    [BATTLE_TERRAIN_PLAIN] = MOVE_TRI_ATTACK,      [BATTLE_TERRAIN_SNOW] = MOVE_ICE_BEAM,
#elif B_NATURE_POWER_MOVES == GEN_6
    [BATTLE_TERRAIN_GRASS] = MOVE_ENERGY_BALL,     [BATTLE_TERRAIN_LONG_GRASS] = MOVE_ENERGY_BALL,
    [BATTLE_TERRAIN_SAND] = MOVE_EARTH_POWER,      [BATTLE_TERRAIN_WATER] = MOVE_HYDRO_PUMP,
    [BATTLE_TERRAIN_POND] = MOVE_HYDRO_PUMP,       [BATTLE_TERRAIN_MOUNTAIN] = MOVE_EARTH_POWER,
    [BATTLE_TERRAIN_CAVE] = MOVE_EARTH_POWER,      [BATTLE_TERRAIN_BUILDING] = MOVE_TRI_ATTACK,
    [BATTLE_TERRAIN_PLAIN] = MOVE_TRI_ATTACK,      [BATTLE_TERRAIN_SNOW] = MOVE_FROST_BREATH,
#elif B_NATURE_POWER_MOVES == GEN_5
    [BATTLE_TERRAIN_GRASS] = MOVE_SEED_BOMB,       [BATTLE_TERRAIN_LONG_GRASS] = MOVE_SEED_BOMB,
    [BATTLE_TERRAIN_SAND] = MOVE_EARTHQUAKE,       [BATTLE_TERRAIN_WATER] = MOVE_HYDRO_PUMP,
    [BATTLE_TERRAIN_POND] = MOVE_HYDRO_PUMP,       [BATTLE_TERRAIN_MOUNTAIN] = MOVE_EARTHQUAKE,
    [BATTLE_TERRAIN_CAVE] = MOVE_EARTHQUAKE,       [BATTLE_TERRAIN_BUILDING] = MOVE_TRI_ATTACK,
    [BATTLE_TERRAIN_PLAIN] = MOVE_EARTHQUAKE,      [BATTLE_TERRAIN_SNOW] = MOVE_BLIZZARD,
#elif B_NATURE_POWER_MOVES == GEN_4
    [BATTLE_TERRAIN_GRASS] = MOVE_SEED_BOMB,       [BATTLE_TERRAIN_LONG_GRASS] = MOVE_SEED_BOMB,
    [BATTLE_TERRAIN_SAND] = MOVE_EARTHQUAKE,       [BATTLE_TERRAIN_WATER] = MOVE_HYDRO_PUMP,
    [BATTLE_TERRAIN_POND] = MOVE_HYDRO_PUMP,       [BATTLE_TERRAIN_MOUNTAIN] = MOVE_ROCK_SLIDE,
    [BATTLE_TERRAIN_CAVE] = MOVE_ROCK_SLIDE,       [BATTLE_TERRAIN_BUILDING] = MOVE_TRI_ATTACK,
    [BATTLE_TERRAIN_PLAIN] = MOVE_EARTHQUAKE,      [BATTLE_TERRAIN_SNOW] = MOVE_BLIZZARD,
#else  // Gen 1-3
    [BATTLE_TERRAIN_GRASS] = MOVE_STUN_SPORE,      [BATTLE_TERRAIN_LONG_GRASS] = MOVE_RAZOR_LEAF,
    [BATTLE_TERRAIN_SAND] = MOVE_EARTHQUAKE,       [BATTLE_TERRAIN_WATER] = MOVE_SURF,
    [BATTLE_TERRAIN_POND] = MOVE_BUBBLE_BEAM,      [BATTLE_TERRAIN_MOUNTAIN] = MOVE_ROCK_SLIDE,
    [BATTLE_TERRAIN_CAVE] = MOVE_SHADOW_BALL,      [BATTLE_TERRAIN_BUILDING] = MOVE_SWIFT,
    [BATTLE_TERRAIN_PLAIN] = MOVE_SWIFT,           [BATTLE_TERRAIN_SNOW] = MOVE_BLIZZARD,
#endif
    [BATTLE_TERRAIN_UNDERWATER] = MOVE_HYDRO_PUMP, [BATTLE_TERRAIN_SOARING] = MOVE_AIR_SLASH,
    [BATTLE_TERRAIN_SKY_PILLAR] = MOVE_AIR_SLASH,  [BATTLE_TERRAIN_BURIAL_GROUND] = MOVE_SHADOW_BALL,
    [BATTLE_TERRAIN_PUDDLE] = MOVE_MUD_BOMB,       [BATTLE_TERRAIN_MARSH] = MOVE_MUD_BOMB,
    [BATTLE_TERRAIN_SWAMP] = MOVE_MUD_BOMB,        [BATTLE_TERRAIN_ICE] = MOVE_ICE_BEAM,
    [BATTLE_TERRAIN_VOLCANO] = MOVE_LAVA_PLUME,    [BATTLE_TERRAIN_DISTORTION_WORLD] = MOVE_TRI_ATTACK,
    [BATTLE_TERRAIN_SPACE] = MOVE_DRACO_METEOR,    [BATTLE_TERRAIN_ULTRA_SPACE] = MOVE_PSYSHOCK,
};

static const u16 sPickupItems[] = {
    ITEM_SUPER_POTION,
    ITEM_FULL_HEAL,
    ITEM_GREAT_BALL,
    ITEM_REPEL,
    ITEM_REVIVE,
    ITEM_QUICK_BALL,
    ITEM_ETHER,
    ITEM_HYPER_POTION,
    ITEM_CHERISH_BALL,
    ITEM_MAX_ETHER,
    ITEM_RARE_CANDY,
    ITEM_BOTTLE_CAP,
    ITEM_MAX_REVIVE,
    ITEM_LEFTOVERS,
    ITEM_FULL_RESTORE,
    ITEM_PP_UP,
    ITEM_PP_MAX,
    ITEM_MAX_ELIXIR,
};

static const u16 sRarePickupItems[] = {
    ITEM_HYPER_POTION,
    ITEM_NUGGET,
    ITEM_KINGS_ROCK,
    ITEM_FULL_RESTORE,
    ITEM_PEARL_STRING,
    ITEM_BALM_MUSHROOM,
    ITEM_PP_MAX,
    ITEM_ELIXIR,
    ITEM_BIG_NUGGET,
    ITEM_GOLD_BOTTLE_CAP,
    ITEM_COMET_SHARD,
};

static const u8 sPickupProbabilities[] = {30, 40, 50, 60, 70, 80, 90, 94, 98};

static const u8 sTerrainToType[BATTLE_TERRAIN_COUNT] = {
    [BATTLE_TERRAIN_GRASS] = TYPE_GRASS,       [BATTLE_TERRAIN_LONG_GRASS] = TYPE_GRASS,    [BATTLE_TERRAIN_SAND] = TYPE_GROUND,
    [BATTLE_TERRAIN_UNDERWATER] = TYPE_WATER,  [BATTLE_TERRAIN_WATER] = TYPE_WATER,         [BATTLE_TERRAIN_POND] = TYPE_WATER,
    [BATTLE_TERRAIN_CAVE] = TYPE_ROCK,         [BATTLE_TERRAIN_BUILDING] = TYPE_NORMAL,     [BATTLE_TERRAIN_SOARING] = TYPE_FLYING,
    [BATTLE_TERRAIN_SKY_PILLAR] = TYPE_FLYING, [BATTLE_TERRAIN_BURIAL_GROUND] = TYPE_GHOST, [BATTLE_TERRAIN_PUDDLE] = TYPE_GROUND,
    [BATTLE_TERRAIN_MARSH] = TYPE_GROUND,      [BATTLE_TERRAIN_SWAMP] = TYPE_GROUND,        [BATTLE_TERRAIN_SNOW] = TYPE_ICE,
    [BATTLE_TERRAIN_ICE] = TYPE_ICE,           [BATTLE_TERRAIN_VOLCANO] = TYPE_FIRE,        [BATTLE_TERRAIN_DISTORTION_WORLD] = TYPE_NORMAL,
    [BATTLE_TERRAIN_SPACE] = TYPE_DRAGON,      [BATTLE_TERRAIN_ULTRA_SPACE] = TYPE_PSYCHIC,
#if B_CAMOUFLAGE_TYPES >= GEN_5
    [BATTLE_TERRAIN_MOUNTAIN] = TYPE_GROUND,   [BATTLE_TERRAIN_PLAIN] = TYPE_GROUND,
#elif B_CAMOUFLAGE_TYPES == GEN_4
    [BATTLE_TERRAIN_MOUNTAIN] = TYPE_ROCK,     [BATTLE_TERRAIN_PLAIN] = TYPE_GROUND,
#else
    [BATTLE_TERRAIN_MOUNTAIN] = TYPE_ROCK,     [BATTLE_TERRAIN_PLAIN] = TYPE_NORMAL,
#endif
};

// In Battle Palace, moves are chosen based on the pokemons nature rather than by the player
// Moves are grouped into "Attack", "Defense", or "Support" (see PALACE_MOVE_GROUP_*)
// Each nature has a certain percent chance of selecting a move from a particular group
// and a separate percent chance for each group when below 50% HP
// The table below doesn't list percentages for Support because you can subtract the other two
// Support percentages are listed in comments off to the side instead
#define PALACE_STYLE(atk, def, atkLow, defLow) {atk, atk + def, atkLow, atkLow + defLow}

const ALIGNED(4) u8 gBattlePalaceNatureToMoveGroupLikelihood[NUM_NATURES][4] = {
    [NATURE_HARDY] = PALACE_STYLE(61, 7, 61, 7),      // 32% support >= 50% HP, 32% support < 50% HP
    [NATURE_LONELY] = PALACE_STYLE(20, 25, 84, 8),    // 55%,  8%
    [NATURE_BRAVE] = PALACE_STYLE(70, 15, 32, 60),    // 15%,  8%
    [NATURE_ADAMANT] = PALACE_STYLE(38, 31, 70, 15),  // 31%, 15%
    [NATURE_NAUGHTY] = PALACE_STYLE(20, 70, 70, 22),  // 10%,  8%
    [NATURE_BOLD] = PALACE_STYLE(30, 20, 32, 58),     // 50%, 10%
    [NATURE_DOCILE] = PALACE_STYLE(56, 22, 56, 22),   // 22%, 22%
    [NATURE_RELAXED] = PALACE_STYLE(25, 15, 75, 15),  // 60%, 10%
    [NATURE_IMPISH] = PALACE_STYLE(69, 6, 28, 55),    // 25%, 17%
    [NATURE_LAX] = PALACE_STYLE(35, 10, 29, 6),       // 55%, 65%
    [NATURE_TIMID] = PALACE_STYLE(62, 10, 30, 20),    // 28%, 50%
    [NATURE_HASTY] = PALACE_STYLE(58, 37, 88, 6),     //  5%,  6%
    [NATURE_SERIOUS] = PALACE_STYLE(34, 11, 29, 11),  // 55%, 60%
    [NATURE_JOLLY] = PALACE_STYLE(35, 5, 35, 60),     // 60%,  5%
    [NATURE_NAIVE] = PALACE_STYLE(56, 22, 56, 22),    // 22%, 22%
    [NATURE_MODEST] = PALACE_STYLE(35, 45, 34, 60),   // 20%,  6%
    [NATURE_MILD] = PALACE_STYLE(44, 50, 34, 6),      //  6%, 60%
    [NATURE_QUIET] = PALACE_STYLE(56, 22, 56, 22),    // 22%, 22%
    [NATURE_BASHFUL] = PALACE_STYLE(30, 58, 30, 58),  // 12%, 12%
    [NATURE_RASH] = PALACE_STYLE(30, 13, 27, 6),      // 57%, 67%
    [NATURE_CALM] = PALACE_STYLE(40, 50, 25, 62),     // 10%, 13%
    [NATURE_GENTLE] = PALACE_STYLE(18, 70, 90, 5),    // 12%,  5%
    [NATURE_SASSY] = PALACE_STYLE(88, 6, 22, 20),     //  6%, 58%
    [NATURE_CAREFUL] = PALACE_STYLE(42, 50, 42, 5),   //  8%, 53%
    [NATURE_QUIRKY] = PALACE_STYLE(56, 22, 56, 22),   // 22%, 22%
};

static const u8 sBattlePalaceNatureToFlavorTextId[NUM_NATURES] = {
    [NATURE_HARDY] = B_MSG_EAGER_FOR_MORE,   [NATURE_LONELY] = B_MSG_GLINT_IN_EYE,  [NATURE_BRAVE] = B_MSG_GETTING_IN_POS,
    [NATURE_ADAMANT] = B_MSG_GLINT_IN_EYE,   [NATURE_NAUGHTY] = B_MSG_GLINT_IN_EYE, [NATURE_BOLD] = B_MSG_GETTING_IN_POS,
    [NATURE_DOCILE] = B_MSG_EAGER_FOR_MORE,  [NATURE_RELAXED] = B_MSG_GLINT_IN_EYE, [NATURE_IMPISH] = B_MSG_GETTING_IN_POS,
    [NATURE_LAX] = B_MSG_GROWL_DEEPLY,       [NATURE_TIMID] = B_MSG_GROWL_DEEPLY,   [NATURE_HASTY] = B_MSG_GLINT_IN_EYE,
    [NATURE_SERIOUS] = B_MSG_EAGER_FOR_MORE, [NATURE_JOLLY] = B_MSG_GETTING_IN_POS, [NATURE_NAIVE] = B_MSG_EAGER_FOR_MORE,
    [NATURE_MODEST] = B_MSG_GETTING_IN_POS,  [NATURE_MILD] = B_MSG_GROWL_DEEPLY,    [NATURE_QUIET] = B_MSG_EAGER_FOR_MORE,
    [NATURE_BASHFUL] = B_MSG_EAGER_FOR_MORE, [NATURE_RASH] = B_MSG_GROWL_DEEPLY,    [NATURE_CALM] = B_MSG_GETTING_IN_POS,
    [NATURE_GENTLE] = B_MSG_GLINT_IN_EYE,    [NATURE_SASSY] = B_MSG_GROWL_DEEPLY,   [NATURE_CAREFUL] = B_MSG_GROWL_DEEPLY,
    [NATURE_QUIRKY] = B_MSG_EAGER_FOR_MORE,
};

#define IS_THREE_HEADED(battlerAttacker) (gBaseStats[gBattleMons[battlerAttacker].species].flags & F_THREE_HEADED)

static bool32 NoTargetPresent(u32 move) {
    switch (move) {
        case MOVE_SUNNY_DAY:
        case MOVE_RAIN_DANCE:
        case MOVE_SANDSTORM:
        case MOVE_HAIL:
        case MOVE_EERIE_FOG:
            return FALSE;
    }

    if (!IsBattlerAlive(gBattlerTarget)) gBattlerTarget = GetMoveTarget(move, 0);

    switch (GetBattlerBattleMoveTargetFlags(move, gBattlerAttacker)) {
        case MOVE_TARGET_SELECTED:
        case MOVE_TARGET_DEPENDS:
        case MOVE_TARGET_RANDOM:
            if (!IsBattlerAlive(gBattlerTarget)) return TRUE;
            break;
        case MOVE_TARGET_BOTH:
            if (!IsBattlerAlive(gBattlerTarget) && !IsBattlerAlive(BATTLE_PARTNER(gBattlerTarget))) return TRUE;
            break;
        case MOVE_TARGET_FOES_AND_ALLY:
            if (!IsBattlerAlive(gBattlerTarget) && !IsBattlerAlive(BATTLE_PARTNER(gBattlerTarget)) && !IsBattlerAlive(BATTLE_PARTNER(gBattlerAttacker)))
                return TRUE;
            break;
    }

    return FALSE;
}

static bool32 TryAegiFormChange(void) {
    u16 newSpecies = 0;
    // Only Aegislash with Stance Change can transform, transformed mons cannot.
    if (!BATTLER_HAS_ABILITY(gBattlerAttacker, ABILITY_STANCE_CHANGE) || gBattleMons[gBattlerAttacker].status2 & STATUS2_TRANSFORMED) return FALSE;

    switch (gBattleMons[gBattlerAttacker].species) {
        default:
            return FALSE;
        case SPECIES_AEGISLASH:  // Shield -> Blade
            if (gBattleMoves[gCurrentMove].power > 0) newSpecies = SPECIES_AEGISLASH_BLADE;
            break;
        case SPECIES_AEGISLASH_BLADE:  // Blade -> Shield
            if (gCurrentMove == MOVE_KINGS_SHIELD) newSpecies = SPECIES_AEGISLASH;
            break;
        case SPECIES_AEGISLASH_BLADE_REDUX:  // Special -> Physical
            if (gBattleMoves[gCurrentMove].split == SPLIT_PHYSICAL && !gBattleMoves[gCurrentMove].arrowBased) newSpecies = SPECIES_AEGISLASH_REDUX;
            break;
        case SPECIES_AEGISLASH_BLADE_REDUX_MEGA:  // Special -> Physical
            if (gBattleMoves[gCurrentMove].split == SPLIT_PHYSICAL && !gBattleMoves[gCurrentMove].arrowBased) newSpecies = SPECIES_AEGISLASH_REDUX_MEGA;
            break;
        case SPECIES_AEGISLASH_REDUX:  // Physical -> Special
            if (gBattleMoves[gCurrentMove].split == SPLIT_SPECIAL || gBattleMoves[gCurrentMove].arrowBased) newSpecies = SPECIES_AEGISLASH_BLADE_REDUX;
            break;
        case SPECIES_AEGISLASH_REDUX_MEGA:  // Physical -> Special
            if (gBattleMoves[gCurrentMove].split == SPLIT_SPECIAL || gBattleMoves[gCurrentMove].arrowBased) newSpecies = SPECIES_AEGISLASH_BLADE_REDUX_MEGA;
            break;
    }

    if (!newSpecies) return FALSE;
    gBattleScripting.abilityPopupOverwrite = ABILITY_STANCE_CHANGE;

    UpdateAbilityStateIndicesForNewSpecies(gBattlerAttacker, newSpecies);
    gBattleMons[gBattlerAttacker].species = newSpecies;
    BattleScriptCall(BattleScript_AttackerFormChange);
    BattleScriptCall(BattleScript_AbilityPopUp);
    return TRUE;
}

bool8 PartyIsMaxLevel(void) {
    int i;

    for (i = 0; i < PARTY_SIZE; i++) {
        if (GetMonData(&gPlayerParty[i], MON_DATA_LEVEL) != 100 && !GetMonData(&gPlayerParty[i], MON_DATA_IS_EGG)) return FALSE;
    }
    return TRUE;
}

int ShouldSetMoldBreaker(int battler, int move) {
    if (gBattleMoves[gCurrentMove].flags & FLAG_TARGET_ABILITY_IGNORED) return TRUE;
    if (getMonotypeChampType() == TYPE_STEEL && GetBattlerSide(battler) != B_SIDE_PLAYER) return TRUE;
    if (BattlerHasAbility(gBattlerAttacker, ABILITY_MOLD_BREAKER, FALSE)) return TRUE;
    if (BattlerHasAbility(gBattlerAttacker, ABILITY_TERAVOLT, FALSE)) return TRUE;
    if (BattlerHasAbility(gBattlerAttacker, ABILITY_TURBOBLAZE, FALSE)) return TRUE;
    if (BattlerHasAbility(gBattlerAttacker, ABILITY_BLIND_RAGE, FALSE)) return TRUE;
    if (BattlerHasAbility(gBattlerAttacker, ABILITY_MYCELIUM_MIGHT, FALSE) && IS_MOVE_STATUS(gCurrentMove)) return TRUE;
    return FALSE;
}

MultihitType GetParentalBondType(int battler, int target, int move, int moveType) {
    if (!IsMoveAffectedByParentalBond(move, battler)) return MULTIHIT_SINGLE;

    int hasFortKnox = HasFortKnox(target);

    ON_ABILITY(battler,
               FALSE,
               gAbilities[ability].onParentalBond && (!hasFortKnox || gAbilities[ability].resistsFortKnox),
               int result = gAbilities[ability].onParentalBond(battler, move, moveType);
               if (result) return result)

    return ABILITY_NONE;
}

int HasFortKnox(int battler) {
    RETURN_ABILITY_IF_FLAG(battler, FALSE, fortKnox)
    return FALSE;
}

static int HasProtean(int battler) {
    RETURN_ABILITY_IF_FLAG(battler, FALSE, protean)
    return ABILITY_NONE;
}

static int HasColorChange(int battler) {
    RETURN_ABILITY_IF_FLAG(battler, FALSE, colorChange)
    return ABILITY_NONE;
}

int GetParentalBondCount(int battler, MultihitType parentalBondType) {
    switch (parentalBondType) {
        case PARENTAL_BOND_HYPER_AGGRESSIVE:
        case PARENTAL_BOND_PRIMAL_MAW:
        case PARENTAL_BOND_DUAL_WIELD:
        case PARENTAL_BOND_ICE_COLD_HUNTER:
            return 2;

        case PARENTAL_BOND_THREE_HEADED:
            return 3;

        case PARENTAL_BOND_MINION_CONTROL: {
            struct Pokemon* party;
            int i;
            u8 count = 1;
            if (GetBattlerSide(gBattlerAttacker) == B_SIDE_PLAYER)
                party = gPlayerParty;
            else
                party = gEnemyParty;

            for (i = 0; i < PARTY_SIZE; i++) {
                FILTER(gBattlerPartyIndexes[gBattlerAttacker] != i)
                FILTER(GetMonData(&party[i], MON_DATA_HP))
                FILTER(GetMonData(&party[i], MON_DATA_SPECIES2))
                FILTER(GetMonData(&party[i], MON_DATA_SPECIES2) != SPECIES_EGG)
                FILTER_NOT(GetMonData(&party[i], MON_DATA_STATUS))
                count++;
            }

            return count;
        }
    }

    return 1;
}

static void Cmd_attackcanceler(void) {
    s32 i;
    u8 moveType;

    GET_MOVE_TYPE(gCurrentMove, moveType);

    if (gBattleMoves[gCurrentMove].type2) {
        CalculateMoveDamage(gCurrentMove, gBattlerAttacker, gBattlerTarget, &moveType, 1, FALSE, TRUE, TRUE);
        gBattleStruct->dynamicMoveType = moveType | 0x80;
    }

    if (moveType == TYPE_FIRE && (gBattleWeather & WEATHER_RAIN_PRIMAL) && WEATHER_HAS_EFFECT && gBattleMoves[gCurrentMove].power) {
        BattleScriptCall(BattleScript_PrimordialSeaFizzlesOutFireTypeMoves);
        CancelMultiTurnMoves(gBattlerAttacker);
        return;
    }

    if (moveType == TYPE_WATER && (gBattleWeather & WEATHER_SUN_PRIMAL) && WEATHER_HAS_EFFECT && gBattleMoves[gCurrentMove].power) {
        BattleScriptCall(BattleScript_DesolateLandEvaporatesWaterTypeMoves);
        CancelMultiTurnMoves(gBattlerAttacker);
        return;
    }

    if (gBattleOutcome != 0) {
        gCurrentActionFuncId = B_ACTION_FINISHED;
        return;
    }
    if (!IsBattlerAlive(gBattlerAttacker) && !(gHitMarker & HITMARKER_NO_ATTACKSTRING) &&
        !(gProcessingExtraAttacks && gQueuedExtraAttackData[0].ability == ABILITY_VICTORY_BOMB)) {
        gHitMarker |= HITMARKER_UNABLE_TO_USE_MOVE;
        gBattlescriptCurrInstr = BattleScript_MoveEnd;
        return;
    }
#if (B_STANCE_CHANGE_FAIL <= GEN_6)
    if (TryAegiFormChange()) return;
#endif
    if (AtkCanceller_UnableToUseMove()) return;

    if (!gTurnStructs[gBattlerAttacker].multiHitCounter) {
        gTurnStructs[gBattlerAttacker].parentalBondTrigger = GetParentalBondType(gBattlerAttacker, gBattlerTarget, gCurrentMove, moveType);
        i = GetParentalBondCount(gBattlerAttacker, gTurnStructs[gBattlerAttacker].parentalBondTrigger);
        if (i > 1) {
            gTurnStructs[gBattlerAttacker].multiHitCounter = gTurnStructs[gBattlerAttacker].parentalBondOn =
                gTurnStructs[gBattlerAttacker].parentalBondInitialCount = i;
            PREPARE_BYTE_NUMBER_BUFFER(gBattleScripting.multihitString, 1, 0)
        }
    }

    int ability;
    if ((ability = HasProtean(gBattlerAttacker)) && !IS_BATTLER_OF_TYPE(gBattlerAttacker, moveType) && gCurrentMove != MOVE_STRUGGLE) {
        gBattleScripting.abilityPopupOverwrite = ability;
        SET_BATTLER_TYPE(gBattlerAttacker, moveType);
        PREPARE_TYPE_BUFFER(gBattleTextBuff1, moveType);
        gBattlerAbility = gBattlerAttacker;
        BattleScriptCall(BattleScript_ProteanActivates);
        return;
    }

    if ((ability = HasColorChange(gBattlerTarget)) && gBattlerAttacker != gBattlerTarget && CheckAndSetOncePerTurnAbility(gBattlerTarget, ability)) {
        u32 bestType = gBattleMons[gBattlerTarget].type1;
        u16 bestModifier = GetTypeModifier(moveType, bestType, gBattlerAttacker, gBattlerTarget);

        for (int currentType = TYPE_NORMAL; currentType < NUMBER_OF_MON_TYPES; ++currentType) {
            u16 currentModifier = GetTypeModifier(moveType, currentType, gBattlerAttacker, gBattlerTarget);
            if (currentModifier < bestModifier) {
                bestModifier = currentModifier;
                bestType = currentType;
            }
            if (bestModifier == UQ_4_12(0.0)) break;
        }

        if (gBattleMons[gBattlerTarget].type1 != bestType) {
            gBattleScripting.abilityPopupOverwrite = ability;
            SET_BATTLER_TYPE(gBattlerTarget, bestType);
            PREPARE_TYPE_BUFFER(gBattleTextBuff1, bestType);
            gBattlerAbility = gBattlerTarget;
            BattleScriptCall(BattleScript_ColorChangeActivates);
            return;
        }
    }

    if (AtkCanceller_UnableToUseMove2()) return;

    if (AbilityBattleEffects(ABILITYEFFECT_MOVES_BLOCK, gBattlerTarget, 0, 0, 0)) {
        CancelMultiTurnMoves(gBattlerAttacker);
        gRoundStructs[gBattlerAttacker].attackCancelled = TRUE;
        return;
    }
    if (!gBattleMons[gBattlerAttacker].pp[gCurrMovePos] && gCurrentMove != MOVE_STRUGGLE &&
        !(gHitMarker & (HITMARKER_x800000 | HITMARKER_NO_ATTACKSTRING | HITMARKER_NO_PPDEDUCT)) &&
        !(gBattleMons[gBattlerAttacker].status2 & STATUS2_MULTIPLETURNS) &&
        !((gBattleMoves[gCurrentMove].effect == EFFECT_SPIT_UP || gBattleMoves[gCurrentMove].effect == EFFECT_SWALLOW) &&
          gVolatileStructs[gBattlerAttacker].stockpileCounter)) {
        gBattlescriptCurrInstr = BattleScript_NoPPForMove;
        gMoveResultFlags |= MOVE_RESULT_MISSED;
        return;
    }
#if (B_STANCE_CHANGE_FAIL >= GEN_7)
    if (TryAegiFormChange()) return;
#endif

    if (BATTLER_HAS_ABILITY(gBattlerAttacker, ABILITY_FLAMMABLE_COAT) && moveType == TYPE_FIRE &&
        gBattleMons[gBattlerAttacker].species == SPECIES_LUMBERING_SLOTH && !(gBattleMons[gBattlerAttacker].status2 & STATUS2_TRANSFORMED)) {
        gBattleScripting.abilityPopupOverwrite = ABILITY_FLAMMABLE_COAT;
        UpdateAbilityStateIndicesForNewSpecies(gBattlerAttacker, SPECIES_LUMBERING_SLOTH_ENGULFED);
        gBattleMons[gBattlerAttacker].species = SPECIES_LUMBERING_SLOTH_ENGULFED;
        BattleScriptCall(BattleScript_AttackerFormChange);
        BattleScriptCall(BattleScript_AbilityPopUp);
        return;
    }

    gHitMarker &= ~(HITMARKER_x800000);
    if (!(gHitMarker & HITMARKER_OBEYS) && !(gBattleMons[gBattlerAttacker].status2 & STATUS2_MULTIPLETURNS)) {
        switch (IsMonDisobedient()) {
            case 0:
                break;
            case 2:
                gHitMarker |= HITMARKER_OBEYS;
                return;
            default:
                gMoveResultFlags |= MOVE_RESULT_MISSED;
                return;
        }
    }

    gHitMarker |= HITMARKER_OBEYS;
    if (NoTargetPresent(gCurrentMove) && (!IsTwoTurnsMove(gCurrentMove) || (gBattleMons[gBattlerAttacker].status2 & STATUS2_MULTIPLETURNS))) {
        gBattlescriptCurrInstr = BattleScript_ButItFailedAtkStringPpReduce;
        if (!IsTwoTurnsMove(gCurrentMove) || (gBattleMons[gBattlerAttacker].status2 & STATUS2_MULTIPLETURNS)) CancelMultiTurnMoves(gBattlerAttacker);
        return;
    }

    if (gBattleMoves[gCurrentMove].flags & FLAG_MAGIC_COAT_AFFECTED && !gRoundStructs[gBattlerAttacker].usesBouncedMove) {
        if (gRoundStructs[gBattlerTarget].bounceMove) {
            gRoundStructs[gBattlerTarget].bounceMove = FALSE;
            gRoundStructs[gBattlerTarget].usesBouncedMove = TRUE;
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_PKMNMOVEBOUNCED;
            if (BlocksPrankster(gCurrentMove, gBattlerTarget, gBattlerAttacker, TRUE)) {
                // Opponent used a prankster'd magic coat -> reflected status move should fail against a dark-type attacker
                gBattlerTarget = gBattlerAttacker;
                gBattlescriptCurrInstr = BattleScript_MagicCoatBouncePrankster;
            } else {
                BattleScriptCall(BattleScript_MagicCoatBounce);
            }
            return;
        }

        ON_ABILITY(gBattlerTarget, TRUE, gAbilities[ability].magicBounce, gBattleScripting.abilityPopupOverwrite = ability;
                   gRoundStructs[gBattlerTarget].usesBouncedMove = TRUE;
                   gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_PKMNMOVEBOUNCEDABILITY;
                   BattleScriptCall(BattleScript_MagicCoatBounce);
                   return)
    }

    for (i = 0; i < gBattlersCount; i++) {
        if ((gRoundStructs[gBattlerByTurnOrder[i]].stealMove) && gBattleMoves[gCurrentMove].flags & FLAG_SNATCH_AFFECTED) {
            gRoundStructs[gBattlerByTurnOrder[i]].stealMove = FALSE;
            gBattleScripting.battler = gBattlerByTurnOrder[i];
            BattleScriptCall(BattleScript_SnatchedMove);
            return;
        }
    }

    if (gTurnStructs[gBattlerTarget].redirectedAbility) {
        gBattleScripting.abilityPopupOverwrite = gTurnStructs[gBattlerTarget].redirectedAbility;
        gTurnStructs[gBattlerTarget].redirectedAbility = ABILITY_NONE;
        BattleScriptCall(BattleScript_TookAttack);
    } else if (IsBattlerProtected(gBattlerTarget, gCurrentMove) && (gCurrentMove != MOVE_CURSE || IS_BATTLER_OF_TYPE(gBattlerAttacker, TYPE_GHOST)) &&
               ((!IsTwoTurnsMove(gCurrentMove) || (gBattleMons[gBattlerAttacker].status2 & STATUS2_MULTIPLETURNS))) &&
               gBattleMoves[gCurrentMove].effect != EFFECT_SUCKER_PUNCH) {
        if (IsMoveMakingContact(gCurrentMove, gBattlerAttacker) || gRoundStructs[gBattlerTarget].merculight)
            gRoundStructs[gBattlerAttacker].touchedProtectLike = TRUE;
        CancelMultiTurnMoves(gBattlerAttacker);
        gMoveResultFlags |= MOVE_RESULT_MISSED;
        gLastLandedMoves[gBattlerTarget] = 0;
        gLastHitByType[gBattlerTarget] = 0;
        gBattleCommunication[MISS_TYPE] = B_MSG_PROTECTED;
        gBattlescriptCurrInstr++;
    } else {
        gBattlescriptCurrInstr++;
    }
}

static bool32 JumpIfMoveFailed(u8 adder, u16 move) {
    if (gMoveResultFlags & MOVE_RESULT_NO_EFFECT) {
        gTurnStructs[gBattlerAttacker].multiHitCounter = 0;
        gLastLandedMoves[gBattlerTarget] = 0;
        gLastHitByType[gBattlerTarget] = 0;
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
        return TRUE;
    } else {
        TrySetDestinyBondToHappen();
        if (AbilityBattleEffects(ABILITYEFFECT_ABSORBING, gBattlerTarget, 0, 0, move)) {
            gMoveResultFlags |= MOVE_RESULT_DOESNT_AFFECT_FOE;
            return TRUE;
        }
    }
    gBattlescriptCurrInstr += adder;
    return FALSE;
}

static void Cmd_jumpifaffectedbyprotect(void) {
    if (IsBattlerProtected(gBattlerTarget, gCurrentMove)) {
        gMoveResultFlags |= MOVE_RESULT_MISSED;
        JumpIfMoveFailed(5, 0);
        gBattleCommunication[MISS_TYPE] = B_MSG_PROTECTED;
    } else {
        gBattlescriptCurrInstr += 5;
    }
}

bool8 JumpIfMoveAffectedByProtect(u16 move) {
    bool8 affected = FALSE;
    if (IsBattlerProtected(gBattlerTarget, move)) {
        gMoveResultFlags |= MOVE_RESULT_MISSED;
        JumpIfMoveFailed(7, move);
        gBattleCommunication[MISS_TYPE] = B_MSG_PROTECTED;
        affected = TRUE;
    }
    return affected;
}

u32 GetTotalAccuracy(u32 battlerAtk, u32 battlerDef, u32 move, struct MoveState* moveState) {
    s8 buff, accStage, evasionStage;
    u8 atkParam = GetBattlerHoldEffectParam(battlerAtk);
    u8 defParam = GetBattlerHoldEffectParam(battlerDef);
    u32 atkHoldEffect = GetBattlerHoldEffect(battlerAtk, TRUE);
    u32 defHoldEffect = GetBattlerHoldEffect(battlerDef, TRUE);
    u8 moveType;
    AccuracyPriority prio = 0;

    GET_MOVE_TYPE(move, moveType)

    if (gStatuses3[battlerDef] & STATUS3_ALWAYS_HITS && gVolatileStructs[battlerDef].battlerWithSureHit == battlerAtk) return 101;
    if (gStatuses3[battlerDef] & STATUS3_TELEKINESIS && !IsBattlerGrounded(battlerDef)) return 101;
    if (B_TOXIC_NEVER_MISS >= GEN_6 && gBattleMoves[move].effect == EFFECT_TOXIC && IS_BATTLER_OF_TYPE(battlerAtk, TYPE_POISON)) return 101;
    if (IsMyceliumMightActive(battlerAtk)) return 101;

    if ((gStatuses3[battlerDef] & STATUS3_PHANTOM_FORCE) || (!(gBattleMoves[move].flags & FLAG_DMG_IN_AIR) && gStatuses3[battlerDef] & STATUS3_ON_AIR) ||
        (!(gBattleMoves[move].flags & FLAG_DMG_2X_IN_AIR) && gStatuses3[battlerDef] & STATUS3_ON_AIR) ||
        (!(gBattleMoves[move].flags & FLAG_DMG_UNDERGROUND) && gStatuses3[battlerDef] & STATUS3_UNDERGROUND) ||
        (!(gBattleMoves[move].flags & FLAG_DMG_UNDERWATER) && gStatuses3[battlerDef] & STATUS3_UNDERWATER)) {
        if (moveState)
            moveState->missesThisTurn = AI_MISSES_THIS_TURN_IF_FIRST;
        else
            prio = ACCURACY_ALWAYS_MISSES;
    }

    if (gVolatileStructs[battlerAtk].trepidation && moveType == TYPE_PSYCHIC) {
        if (moveState && gVolatileStructs[battlerAtk].trepidation == 1)
            moveState->missesThisTurn = AI_MISSES_THIS_TURN;
        else
            return ACCURACY_ALWAYS_MISSES;
    }

    int moveAcc = gBattleMoves[move].accuracy;

    if (prio < ACCURACY_ALWAYS_MISSES) {
        if (moveAcc == 0)
            prio = ACCURACY_HITS_IF_POSSIBLE;
        else if (IsBattlerWeatherAffected(battlerDef, WEATHER_RAIN_ANY) &&
                 (gBattleMoves[move].effect == EFFECT_THUNDER || gBattleMoves[move].effect == EFFECT_HURRICANE))
            prio = ACCURACY_HITS_IF_POSSIBLE;
        else if (IsBattlerWeatherAffected(battlerDef, WEATHER_HAIL_ANY) &&
                 (gBattleMoves[move].effect == EFFECT_FREEZE_DRY || gBattleMoves[move].effect == MOVE_SHEER_COLD || move == MOVE_BLIZZARD))
            prio = ACCURACY_HITS_IF_POSSIBLE;
        else if (IsBattlerWeatherAffected(battlerDef, WEATHER_FOG_ANY) && (move == MOVE_EERIE_SPELL || move == MOVE_VEXING_VOID))
            prio = ACCURACY_HITS_IF_POSSIBLE;
    }

    gPotentialItemEffectBattler = battlerDef;
    accStage = gBattleMons[battlerAtk].statStages[STAT_ACC];
    evasionStage = gBattleMons[battlerDef].statStages[STAT_EVASION];
    if (gBattleMoves[move].flags & FLAG_STAT_STAGES_IGNORED || BATTLER_HAS_ABILITY(battlerAtk, ABILITY_KEEN_EYE) ||
        BATTLER_HAS_ABILITY(battlerAtk, ABILITY_MINDS_EYE))
        evasionStage = min(evasionStage, DEFAULT_STAT_STAGE);
    else if (IsUnaware(battlerAtk))
        evasionStage = DEFAULT_STAT_STAGE;

    if (gBattleMons[battlerDef].status2 & STATUS2_FORESIGHT)
        buff = accStage;
    else
        buff = accStage + DEFAULT_STAT_STAGE - evasionStage;

    if (buff < 0) buff = 0;
    if (buff >= ARRAY_COUNT(gAccuracyStageRatios)) buff = ARRAY_COUNT(gAccuracyStageRatios) - 1;

    // Check Thunder and Hurricane on sunny weather.
    if (IsBattlerWeatherAffected(battlerDef, WEATHER_SUN_ANY) &&
        (gBattleMoves[move].effect == EFFECT_THUNDER || gBattleMoves[move].effect == EFFECT_HURRICANE || move == MOVE_EERIE_SPELL || move == MOVE_VEXING_VOID))
        moveAcc = 50;

    moveAcc *= gAccuracyStageRatios[buff].dividend;
    moveAcc /= gAccuracyStageRatios[buff].divisor;

    for (int sourceBattler = 0; sourceBattler < gBattlersCount; sourceBattler++) {
        ON_ABILITY(sourceBattler,
                   TRUE,
                   gAbilities[ability].onAccuracy &&
                       IsTargettedApplyOnFlagAppropriate(battlerAtk, sourceBattler, battlerAtk, battlerDef, gAbilities[ability].onAccuracyFor),
                   int result = gAbilities[ability].onAccuracy(ability, battlerAtk, battlerDef, move, moveType, &moveAcc);
                   prio = max(prio, result))
    }
    switch (prio) {
        case ACCURACY_ALWAYS_HITS:
        case ACCURACY_HITS_IF_POSSIBLE:
            return 101;

        case ACCURACY_ALWAYS_MISSES:
            return 0;
    }

    if (defHoldEffect == HOLD_EFFECT_EVASION_UP) moveAcc = (moveAcc * (100 - defParam)) / 100;

    if (atkHoldEffect == HOLD_EFFECT_WIDE_LENS)
        moveAcc = (moveAcc * (100 + atkParam)) / 100;
    else if (atkHoldEffect == HOLD_EFFECT_ZOOM_LENS && GetBattlerTurnOrderNum(battlerAtk) > GetBattlerTurnOrderNum(battlerDef))
        moveAcc = (moveAcc * (100 + atkParam)) / 100;

    if (gRoundStructs[battlerAtk].usedMicleBerry) {
        gRoundStructs[battlerAtk].usedMicleBerry = FALSE;
        if (HasRipenEffect(battlerAtk))
            moveAcc = (moveAcc * 140) / 100;  // ripen gives 40% acc boost
        else
            moveAcc = (moveAcc * 120) / 100;  // 20% acc boost
    }

    if (IsGravityActive()) moveAcc = (moveAcc * 5) / 3;  // 1.66 Gravity acc boost

    if (gSideTimers[GET_BATTLER_SIDE(battlerDef)].smokescreenTimer) moveAcc *= .75;

    return min(moveAcc, 100);
}

static void Cmd_accuracycheck(void) {
    u16 type, move = T2_READ_16(gBattlescriptCurrInstr + 5);

    if (move == ACC_CURR_MOVE) move = gCurrentMove;

    if (move == NO_ACC_CALC_CHECK_LOCK_ON) {
        if (GetAbilityState(gBattlerTarget, ABILITY_COMMANDER) >= COMMANDER_ACTIVE)
            gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
        else if (gStatuses3[gBattlerTarget] & STATUS3_ALWAYS_HITS && gVolatileStructs[gBattlerTarget].battlerWithSureHit == gBattlerAttacker)
            gBattlescriptCurrInstr += 7;
        else if (gStatuses3[gBattlerTarget] & (STATUS3_SEMI_INVULNERABLE))
            gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
        else if (!JumpIfMoveAffectedByProtect(0))
            gBattlescriptCurrInstr += 7;
    } else if (gTurnStructs[gBattlerAttacker].parentalBondOn < gTurnStructs[gBattlerAttacker].parentalBondInitialCount ||
               (gTurnStructs[gBattlerAttacker].multiHitsUsed &&
                (!(gBattleMoves[move].effect == EFFECT_TRIPLE_KICK || gBattleMoves[move].effect == EFFECT_TEN_HITS) ||
                 BATTLER_HAS_ABILITY(gBattlerAttacker, ABILITY_KUNOICHI_BLADE) || BATTLER_HAS_ABILITY(gBattlerAttacker, ABILITY_SKILL_LINK) ||
                 GetBattlerHoldEffect(gBattlerAttacker, TRUE) == HOLD_EFFECT_LOADED_DICE))) {
        // No acc checks for second hit of Parental Bond or multi hit moves
        JumpIfMoveFailed(7, move);
    } else {
        u32 accuracy;
        GET_MOVE_TYPE(move, type);
        if (JumpIfMoveAffectedByProtect(move)) return;

        accuracy = GetTotalAccuracy(gBattlerAttacker, gBattlerTarget, move, NULL);

        // final calculation
        if (accuracy <= 100 && BATTLER_HAS_ABILITY(gBattlerTarget, ABILITY_ANTICIPATION) && !GetSingleUseAbilityCounter(gBattlerTarget, ABILITY_ANTICIPATION) &&
            CalcTypeEffectivenessMultiplier(move, type, gBattlerAttacker, gBattlerTarget, TRUE) >= UQ_4_12(2.0)) {
            SetSingleUseAbilityCounter(gBattlerTarget, ABILITY_ANTICIPATION, TRUE);
            gBattleScripting.abilityPopupOverwrite = ABILITY_ANTICIPATION;
            gMoveResultFlags |= MOVE_RESULT_MISSED;
            gBattleCommunication[MISS_TYPE] = B_MSG_AVOIDED_DMG;
        } else if ((Random() % 100) >= accuracy) {
            gMoveResultFlags |= MOVE_RESULT_MISSED;
            if (GetBattlerHoldEffect(gBattlerAttacker, TRUE) == HOLD_EFFECT_BLUNDER_POLICY)
                gBattleStruct->blunderPolicy = TRUE;  // Only activates from missing through acc/evasion checks

            if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE &&
                GetBattlerBattleMoveTargetFlags(move, gBattlerAttacker) & (MOVE_TARGET_BOTH | MOVE_TARGET_FOES_AND_ALLY))
                gBattleCommunication[MISS_TYPE] = B_MSG_AVOIDED_ATK;
            else
                gBattleCommunication[MISS_TYPE] = B_MSG_MISSED;

            if (gBattleMoves[move].power) CalcTypeEffectivenessMultiplier(move, type, gBattlerAttacker, gBattlerTarget, TRUE);
        }
        JumpIfMoveFailed(7, move);
    }
}

static void Cmd_attackstring(void) {
    if (gBattleControllerExecFlags) return;
    if (!(gHitMarker & (HITMARKER_NO_ATTACKSTRING | HITMARKER_ATTACKSTRING_PRINTED))) {
        PrepareStringBattle(STRINGID_USEDMOVE, gBattlerAttacker);
        gHitMarker |= HITMARKER_ATTACKSTRING_PRINTED;
    }
    gBattlescriptCurrInstr++;
    gBattleCommunication[MSG_DISPLAY] = 0;
}

static void Cmd_ppreduce(void) {
    s32 ppToDeduct = 1;

    if (gBattleControllerExecFlags) return;

    gBattlescriptCurrInstr++;

    if (gHitMarker & HITMARKER_NO_PPDEDUCT) {
        gHitMarker &= ~HITMARKER_NO_PPDEDUCT;
        return;
    }

    if (gHitMarker & HITMARKER_NO_ATTACKSTRING) return;

    if (!BATTLER_HAS_ABILITY(gBattlerAttacker, ABILITY_PRESSURE) && IsAbilityOnSide(GetBattlerSide(BATTLE_OPPOSITE(gBattlerAttacker)), ABILITY_PRESSURE))
        ppToDeduct++;

    if ((gBattleMoves[gCurrentMove].effect == EFFECT_SPIT_UP || gBattleMoves[gCurrentMove].effect == EFFECT_SWALLOW)) {
        while (ppToDeduct) {
            if (!TryUseStockpile(gBattlerAttacker)) break;
            ppToDeduct--;
        }

        if (!ppToDeduct) return;
    }

    if (gBattleMons[gBattlerAttacker].pp[gCurrMovePos]) {
        gRoundStructs[gBattlerAttacker].notFirstStrike = TRUE;
        // For item Metronome, echoed voice
        if (gCurrentMove == gLastResultingMoves[gBattlerAttacker] && !(gMoveResultFlags & MOVE_RESULT_NO_EFFECT) && !WasUnableToUseMove(gBattlerAttacker) &&
            (gTurnStructs[gBattlerAttacker].parentalBondOn > 0 &&
             gTurnStructs[gBattlerAttacker].parentalBondOn != gTurnStructs[gBattlerAttacker].parentalBondInitialCount))  // Don't increment counter on first hit
            gBattleStruct->sameMoveTurns[gBattlerAttacker]++;
        else
            gBattleStruct->sameMoveTurns[gBattlerAttacker] = 0;

        if (gBattleMons[gBattlerAttacker].pp[gCurrMovePos] > ppToDeduct)
            gBattleMons[gBattlerAttacker].pp[gCurrMovePos] -= ppToDeduct;
        else
            gBattleMons[gBattlerAttacker].pp[gCurrMovePos] = 0;

        if (!(gBattleMons[gBattlerAttacker].status2 & STATUS2_TRANSFORMED) && !((gVolatileStructs[gBattlerAttacker].mimickedMoves) & gBitTable[gCurrMovePos])) {
            gActiveBattler = gBattlerAttacker;
            BtlController_EmitSetMonData(0, REQUEST_PPMOVE1_BATTLE + gCurrMovePos, 0, 1, &gBattleMons[gBattlerAttacker].pp[gCurrMovePos]);
            MarkBattlerForControllerExec(gBattlerAttacker);
        }
    }
}

// The chance is 1/N for each stage.
#if B_CRIT_CHANCE >= GEN_7
static const u8 sCriticalHitChance[] = {24, 8, 2, 1, 1};
#elif B_CRIT_CHANCE == GEN_6
static const u8 sCriticalHitChance[] = {16, 8, 2, 1, 1};
#else
static const u8 sCriticalHitChance[] = {16, 8, 4, 3, 2};  // Gens 2,3,4,5
#endif  // B_CRIT_CHANCE

#define BENEFITS_FROM_LEEK(battler, holdEffect)                                                                     \
    ((holdEffect == HOLD_EFFECT_LEEK) && (GET_BASE_SPECIES_ID(gBattleMons[battler].species) == SPECIES_FARFETCHD || \
                                          gBattleMons[battler].species == SPECIES_FARFETCHD_GALARIAN || gBattleMons[battler].species == SPECIES_SIRFETCHD))
s32 CalcCritChanceStage(u8 battlerAtk, u8 battlerDef, u32 move, bool32 recordAbility) {
    u32 holdEffectAtk = GetBattlerHoldEffect(battlerAtk, TRUE);

    if (gSideStatuses[battlerDef] & SIDE_STATUS_LUCKY_CHANT || gStatuses3[gBattlerAttacker] & STATUS3_CANT_SCORE_A_CRIT) {
        return NEVER_CRIT;
    }

    int critChance = 0;

    for (int battler = 0; battler < gBattlersCount; battler++) {
        ON_ABILITY(battler,
                   TRUE,
                   gAbilities[ability].onCrit && IsTargettedApplyOnFlagAppropriate(battlerAtk, battler, battlerAtk, battlerDef, gAbilities[ability].onCritFor),
                   int result = gAbilities[ability].onCrit(battler, battlerDef, move);
                   if (result == NEVER_CRIT) return NEVER_CRIT;
                   critChance += result)
    }

    // Always Critical
    if (gStatuses3[battlerAtk] & STATUS3_LASER_FOCUS || gBattleMoves[move].effect == EFFECT_ALWAYS_CRIT || (gBattleMoves[move].alwaysCrit) ||
        (gBattleMoves[move].effect == EFFECT_FLAIL && gBattleMons[battlerAtk].hp <= gBattleMons[battlerAtk].maxHP / 2) ||
        (gVolatileStructs[battlerAtk].showdownMode)) {
        return ALWAYS_CRIT;
    }

    // Boost Critical Chance
    critChance += ((gBattleMoves[gCurrentMove].flags & FLAG_HIGH_CRIT) != 0) + (holdEffectAtk == HOLD_EFFECT_SCOPE_LENS) +
                  2 * (holdEffectAtk == HOLD_EFFECT_LUCKY_PUNCH && (GET_BASE_SPECIES_ID(gBattleMons[gBattlerAttacker].species) == SPECIES_HAPPINY ||
                                                                    GET_BASE_SPECIES_ID(gBattleMons[gBattlerAttacker].species) == SPECIES_CHANSEY ||
                                                                    GET_BASE_SPECIES_ID(gBattleMons[gBattlerAttacker].species) == SPECIES_HAPPINY_REDUX ||
                                                                    GET_BASE_SPECIES_ID(gBattleMons[gBattlerAttacker].species) == SPECIES_CHANSEY_REDUX ||
                                                                    GET_BASE_SPECIES_ID(gBattleMons[gBattlerAttacker].species) == SPECIES_BLISSEY_REDUX ||
                                                                    GET_BASE_SPECIES_ID(gBattleMons[gBattlerAttacker].species) == SPECIES_BLISSEY)) +
                  BENEFITS_FROM_LEEK(battlerAtk, holdEffectAtk) + gVolatileStructs[battlerAtk].critBoost + (move == MOVE_VISE_GRIP);

    return min(critChance, ALWAYS_CRIT);
}
#undef BENEFITS_FROM_LEEK

s8 GetInverseCritChance(u8 battlerAtk, u8 battlerDef, u32 move) {
    s32 critChanceIndex = CalcCritChanceStage(battlerAtk, battlerDef, move, FALSE);
    if (critChanceIndex <= NEVER_CRIT)
        return -1;
    else
        return sCriticalHitChance[min(critChanceIndex, ALWAYS_CRIT)];
}

static void Cmd_critcalc(void) {
    s32 critChance = CalcCritChanceStage(gBattlerAttacker, gBattlerTarget, gCurrentMove, TRUE);
    gPotentialItemEffectBattler = gBattlerAttacker;

    if (gBattleTypeFlags & (BATTLE_TYPE_WALLY_TUTORIAL | BATTLE_TYPE_FIRST_BATTLE))
        gIsCriticalHit = FALSE;
    else if (critChance <= NEVER_CRIT)
        gIsCriticalHit = FALSE;
    else if (critChance >= ALWAYS_CRIT)
        gIsCriticalHit = TRUE;
    else if (Random() % sCriticalHitChance[critChance] == 0)
        gIsCriticalHit = TRUE;
    else
        gIsCriticalHit = FALSE;

    gBattlescriptCurrInstr++;
}

static void Cmd_damagecalc(void) {
    u8 moveType;
    u8 movePower = 0;

    if (gProcessingExtraAttacks) {
        movePower = gQueuedExtraAttackData[0].movePower;
    }

    // to enable changing the power of a Future Sight
    if (gCurrentMove == gWishFutureKnock.futureSightMove[gBattlerTarget] && gWishFutureKnock.futureSightCounter[gBattlerTarget] == 0) {
        movePower = gWishFutureKnock.futureSightPower[gBattlerTarget];
        gWishFutureKnock.futureSightMove[gBattlerTarget] = MOVE_NONE;
    }

    GET_MOVE_TYPE(gCurrentMove, moveType);
    gBattleMoveDamage = CalculateMoveDamage(gCurrentMove, gBattlerAttacker, gBattlerTarget, &moveType, movePower, gIsCriticalHit, TRUE, TRUE);
    gBattleStruct->dynamicMoveType = moveType | 0x80;

    gBattlescriptCurrInstr++;
}

static void Cmd_typecalc(void) {
    u8 moveType;

    GET_MOVE_TYPE(gCurrentMove, moveType);
    CalcTypeEffectivenessMultiplier(gCurrentMove, moveType, gBattlerAttacker, gBattlerTarget, TRUE);

    gBattlescriptCurrInstr++;
}

static void Cmd_adjustdamage(void) {
    u8 holdEffect, param;
    u32 moveType;

    GET_MOVE_TYPE(gCurrentMove, moveType);

    if (DoesSubstituteBlockMove(gBattlerAttacker, gBattlerTarget, gCurrentMove)) goto END;
    if (DoesDisguiseBlockMove(gBattlerAttacker, gBattlerTarget, gCurrentMove)) goto END;
    if (RemainingNoDamageHits(gBattlerTarget) > 0) goto END;
    if (gBattleMons[gBattlerTarget].hp > gBattleMoveDamage) goto END;

    holdEffect = GetBattlerHoldEffect(gBattlerTarget, TRUE);
    param = GetBattlerHoldEffectParam(gBattlerTarget);

    gPotentialItemEffectBattler = gBattlerTarget;

    if (holdEffect == HOLD_EFFECT_FOCUS_BAND && (Random() % 100) < param) {
        RecordItemEffectBattle(gBattlerTarget, holdEffect);
        gTurnStructs[gBattlerTarget].focusBanded = TRUE;
    } else if (holdEffect == HOLD_EFFECT_FOCUS_SASH && !IsUnnerveAbilityOnOpposingSide(gBattlerTarget) && BATTLER_MAX_HP(gBattlerTarget)) {
        RecordItemEffectBattle(gBattlerTarget, holdEffect);
        gTurnStructs[gBattlerTarget].focusSashed = TRUE;
    } else if (BATTLER_HAS_ABILITY(gBattlerTarget, ABILITY_STURDY) && BATTLER_MAX_HP(gBattlerTarget)) {
        gTurnStructs[gBattlerTarget].sturdied = TRUE;
    } else if (BATTLER_HAS_ABILITY(gBattlerTarget, ABILITY_IMMOVABLE_OBJECT) && BATTLER_MAX_HP(gBattlerTarget)) {
        gTurnStructs[gBattlerTarget].sturdied = TRUE;
    } else if (BATTLER_HAS_ABILITY(gBattlerTarget, ABILITY_LUCKY_HALO) && !GetSingleUseAbilityCounter(gBattlerTarget, ABILITY_LUCKY_HALO)) {
        gTurnStructs[gBattlerTarget].haloed = TRUE;
    }

    if ((gBattleMoves[gCurrentMove].effect != EFFECT_FALSE_SWIPE && !gBattleScripting.forceFalseSwipeEffect) &&
        !(gProcessingExtraAttacks && gQueuedExtraAttackData[0].falseSwipe) && !gRoundStructs[gBattlerTarget].endured &&
        !gTurnStructs[gBattlerTarget].focusBanded && !gTurnStructs[gBattlerTarget].focusSashed && !gTurnStructs[gBattlerTarget].sturdied &&
        !gTurnStructs[gBattlerTarget].haloed)
        goto END;

    // Handle reducing the dmg to 1 hp.
    gBattleMoveDamage = gBattleMons[gBattlerTarget].hp - 1;

    if (gRoundStructs[gBattlerTarget].endured) {
        gMoveResultFlags |= MOVE_RESULT_FOE_ENDURED;
    } else if (gTurnStructs[gBattlerTarget].focusBanded || gTurnStructs[gBattlerTarget].focusSashed) {
        gMoveResultFlags |= MOVE_RESULT_FOE_HUNG_ON;
        gLastUsedItem = gBattleMons[gBattlerTarget].item;
    } else if (gTurnStructs[gBattlerTarget].sturdied) {
        gMoveResultFlags |= MOVE_RESULT_STURDIED;
        gBattleScripting.abilityPopupOverwrite = ABILITY_STURDY;
    } else if (gTurnStructs[gBattlerTarget].haloed) {
        gMoveResultFlags |= MOVE_RESULT_STURDIED;
        gBattleScripting.abilityPopupOverwrite = ABILITY_LUCKY_HALO;
    }

END:
    gBattlescriptCurrInstr++;

    if (!(gMoveResultFlags & MOVE_RESULT_NO_EFFECT) && gBattleMoveDamage >= 1) gTurnStructs[gBattlerAttacker].damagedMons |= gBitTable[gBattlerTarget];

    // Check gems and damage reducing berries.
    if (gTurnStructs[gBattlerTarget].berryReduced && !(gMoveResultFlags & MOVE_RESULT_NO_EFFECT) && gBattleMons[gBattlerTarget].item) {
        BattleScriptCall(BattleScript_TargetAteItem);
        gLastUsedItem = gBattleMons[gBattlerTarget].item;
    }
    if (gTurnStructs[gBattlerAttacker].gemBoost && !(gMoveResultFlags & MOVE_RESULT_NO_EFFECT) && gBattleMons[gBattlerAttacker].item) {
        BattleScriptCall(BattleScript_GemActivates);
        gLastUsedItem = gBattleMons[gBattlerAttacker].item;
    }

    // WEATHER_STRONG_WINDS prints a string when it's about to reduce the power
    // of a move that is Super Effective against a Flying-type Pokémon.
    if (gBattleWeather & WEATHER_STRONG_WINDS) {
        if ((gBattleMons[gBattlerTarget].type1 == TYPE_FLYING &&
             GetTypeModifier(moveType, gBattleMons[gBattlerTarget].type1, gBattlerAttacker, gBattlerTarget) >= UQ_4_12(2.0)) ||
            (gBattleMons[gBattlerTarget].type2 == TYPE_FLYING &&
             GetTypeModifier(moveType, gBattleMons[gBattlerTarget].type2, gBattlerAttacker, gBattlerTarget) >= UQ_4_12(2.0)) ||
            (gBattleMons[gBattlerTarget].type3 == TYPE_FLYING &&
             GetTypeModifier(moveType, gBattleMons[gBattlerTarget].type3, gBattlerAttacker, gBattlerTarget) >= UQ_4_12(2.0))) {
            gBattlerAbility = gBattlerTarget;
            BattleScriptCall(BattleScript_AttackWeakenedByStrongWinds);
        }
    }
}

static void Cmd_multihitresultmessage(void) {
    if (gBattleControllerExecFlags) return;

    if (!(gMoveResultFlags & MOVE_RESULT_FAILED) && !(gMoveResultFlags & MOVE_RESULT_FOE_ENDURED)) {
        if (gMoveResultFlags & MOVE_RESULT_STURDIED) {
            gMoveResultFlags &= ~(MOVE_RESULT_STURDIED | MOVE_RESULT_FOE_HUNG_ON);
            gTurnStructs[gBattlerTarget].sturdied = FALSE;  // Delete this line to make Sturdy last for the duration of the whole move turn.
            if (gTurnStructs[gBattlerTarget].haloed) SetSingleUseAbilityCounter(gBattlerTarget, ABILITY_LUCKY_HALO, TRUE);
            gTurnStructs[gBattlerTarget].haloed = FALSE;
            BattleScriptCall(BattleScript_SturdiedMsg);
            return;
        } else if (gMoveResultFlags & MOVE_RESULT_FOE_HUNG_ON) {
            gLastUsedItem = gBattleMons[gBattlerTarget].item;
            gPotentialItemEffectBattler = gBattlerTarget;
            gMoveResultFlags &= ~(MOVE_RESULT_STURDIED | MOVE_RESULT_FOE_HUNG_ON);
            BattleScriptCall(BattleScript_HangedOnMsg);
            return;
        }
    }
    gBattlescriptCurrInstr++;

    // Print berry reducing message after result message.
    if (gTurnStructs[gBattlerTarget].berryReduced && !(gMoveResultFlags & MOVE_RESULT_NO_EFFECT)) {
        gTurnStructs[gBattlerTarget].berryReduced = FALSE;
        BattleScriptCall(BattleScript_PrintBerryReduceString);
    }
}

static void Cmd_attackanimation(void) {
    if (gBattleControllerExecFlags) return;

    if ((gHitMarker & HITMARKER_NO_ANIMATIONS) && gCurrentMove != MOVE_TRANSFORM &&
        gCurrentMove != MOVE_SUBSTITUTE
        // In a wild double battle gotta use the teleport animation if two wild pokemon are alive.
        && !(gCurrentMove == MOVE_TELEPORT && WILD_DOUBLE_BATTLE && GetBattlerSide(gBattlerAttacker) == B_SIDE_OPPONENT &&
             IsBattlerAlive(BATTLE_PARTNER(gBattlerAttacker)))) {
        BattleScriptPush(gBattlescriptCurrInstr + 1);
        gBattlescriptCurrInstr = BattleScript_Pausex20;
        gBattleScripting.animTurn++;
        gBattleScripting.animTargetsHit++;
    } else {
        if (gTurnStructs[gBattlerAttacker].parentalBondOn < gTurnStructs[gBattlerAttacker].parentalBondInitialCount)  // No animation on second hit
        {
            gBattlescriptCurrInstr++;
            return;
        }

        if ((GetBattlerBattleMoveTargetFlags(gCurrentMove, gBattlerAttacker) & MOVE_TARGET_BOTH ||
             GetBattlerBattleMoveTargetFlags(gCurrentMove, gBattlerAttacker) & MOVE_TARGET_FOES_AND_ALLY ||
             GetBattlerBattleMoveTargetFlags(gCurrentMove, gBattlerAttacker) & MOVE_TARGET_DEPENDS) &&
            gBattleScripting.animTargetsHit) {
            gBattlescriptCurrInstr++;
            return;
        }
        if (!(gMoveResultFlags & MOVE_RESULT_NO_EFFECT)) {
            u8 multihit;

            gActiveBattler = gBattlerAttacker;

            if (gBattleMons[gBattlerTarget].status2 & STATUS2_SUBSTITUTE)
                multihit = gTurnStructs[gBattlerAttacker].multiHitCounter;
            else if (gTurnStructs[gBattlerAttacker].multiHitCounter != 0 && gTurnStructs[gBattlerAttacker].multiHitCounter != 1) {
                if (gBattleMons[gBattlerTarget].hp <= gBattleMoveDamage)
                    multihit = 1;
                else
                    multihit = gTurnStructs[gBattlerAttacker].multiHitCounter;
            } else
                multihit = gTurnStructs[gBattlerAttacker].multiHitCounter;

            BtlController_EmitMoveAnimation(0,
                                            gCurrentMove,
                                            gBattleScripting.animTurn,
                                            gBattleMovePower,
                                            gBattleMoveDamage,
                                            gBattleMons[gBattlerAttacker].friendship,
                                            &gVolatileStructs[gBattlerAttacker],
                                            multihit);
            gBattleScripting.animTurn += 1;
            gBattleScripting.animTargetsHit += 1;
            MarkBattlerForControllerExec(gBattlerAttacker);
            gBattlescriptCurrInstr++;
        } else {
            BattleScriptPush(gBattlescriptCurrInstr + 1);
            gBattlescriptCurrInstr = BattleScript_Pausex20;
        }
    }
}

static void Cmd_waitanimation(void) {
    if (gBattleControllerExecFlags == 0) gBattlescriptCurrInstr++;
}

static void Cmd_healthbarupdate(void) {
    if (gBattleControllerExecFlags) return;

    if (!(gMoveResultFlags & MOVE_RESULT_NO_EFFECT) || (gHitMarker & HITMARKER_PASSIVE_DAMAGE) || gBattleMoveDamage < 0) {
        gActiveBattler = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);

        if (gBattleMoveDamage >= 0 && DoesSubstituteBlockMove(gBattlerAttacker, gActiveBattler, gCurrentMove) &&
            gVolatileStructs[gActiveBattler].substituteHP && !(gHitMarker & HITMARKER_IGNORE_SUBSTITUTE)) {
            PrepareStringBattle(STRINGID_SUBSTITUTEDAMAGED, gActiveBattler);
            FlagSet(FLAG_SYS_DISABLE_DAMAGE_DONE);
        } else if (gBattleMoveDamage > 0 && RemainingNoDamageHits(gActiveBattler) > 0) {
            s8 noDamageHits = RemainingNoDamageHits(gActiveBattler) - 1;

            if (noDamageHits == 0)
                PrepareStringBattle(STRINGID_BATTLERCANNOLONGERENDUREHITS, gActiveBattler);
            else if (noDamageHits == 1)
                PrepareStringBattle(STRINGID_BATTLERCANSTILLENDUREASINGLEHIT, gActiveBattler);
            else {
                ConvertIntToDecimalStringN(gBattleTextBuff4, noDamageHits, STR_CONV_MODE_LEFT_ALIGN, 2);
                PrepareStringBattle(STRINGID_BATTLERCANSTILLENDUREHITS, gActiveBattler);
            }

            FlagSet(FLAG_SYS_DISABLE_DAMAGE_DONE);
        } else if (gBattleMoveDamage < 0 || !DoesDisguiseBlockMove(gBattlerAttacker, gActiveBattler, gCurrentMove)) {
            s16 healthValue = min(gBattleMoveDamage, 10000);  // Max damage (10000) not present in R/S, ensures that huge damage values don't change sign

            if (!(gHitMarker & HITMARKER_IGNORE_SUBSTITUTE) && !(gMoveResultFlags & MOVE_RESULT_ONE_HIT_KO) && !(gMoveResultFlags & MOVE_RESULT_FOE_ENDURED) &&
                !(gMoveResultFlags & MOVE_RESULT_FAILED) && !(gMoveResultFlags & MOVE_RESULT_DOESNT_AFFECT_FOE) && !IS_BATTLER_PROTECTED(gActiveBattler) &&
                gVolatileStructs[gActiveBattler].substituteHP == 0 && gBattleMoves[gCurrentMove].split != SPLIT_STATUS &&
                gBattleMoves[gCurrentMove].power > 0 && gBattleMoveDamage > 0 && gSaveBlock2Ptr->damageDone) {
                VarSet(VAR_DAMAGE_DONE, gBattleMoveDamage);
                FlagClear(FLAG_SYS_DISABLE_DAMAGE_DONE);
            }

            BtlController_EmitHealthBarUpdate(0, healthValue);
            MarkBattlerForControllerExec(gActiveBattler);

            if (GetBattlerSide(gActiveBattler) == B_SIDE_PLAYER && gBattleMoveDamage > 0) gBattleResults.playerMonWasDamaged = TRUE;
        }
    } else {
        FlagClear(FLAG_SYS_DISABLE_DAMAGE_DONE);
    }

    gBattlescriptCurrInstr += 2;
}

void IncrementTimesTookDamage(u8 battler) {
    u8* timesDamaged = &gBattleStruct->timesDamaged[gBattlerPartyIndexes[battler]][GetBattlerSide(battler)];
    *timesDamaged = min(100, *timesDamaged + 1);
}

static void Cmd_datahpupdate(void) {
    int battlerType;

    if (gBattleControllerExecFlags) return;

    battlerType = READ_FIRST_8_INC;

    if (!(gMoveResultFlags & MOVE_RESULT_NO_EFFECT) || gHitMarker & HITMARKER_PASSIVE_DAMAGE || gBattleMoveDamage < 0) {
        gActiveBattler = GetBattlerForBattleScript(battlerType);
        if (gBattleMoveDamage >= 0 && DoesSubstituteBlockMove(gBattlerAttacker, gActiveBattler, gCurrentMove) &&
            gVolatileStructs[gActiveBattler].substituteHP && !(gHitMarker & HITMARKER_IGNORE_SUBSTITUTE)) {
            if (gVolatileStructs[gActiveBattler].substituteHP >= gBattleMoveDamage) {
                if (gTurnStructs[gActiveBattler].dmg == 0) gTurnStructs[gActiveBattler].dmg = gBattleMoveDamage;
                gVolatileStructs[gActiveBattler].substituteHP -= gBattleMoveDamage;
                gHpDealt = gBattleMoveDamage;
            } else {
                if (gTurnStructs[gActiveBattler].dmg == 0) gTurnStructs[gActiveBattler].dmg = gVolatileStructs[gActiveBattler].substituteHP;
                gHpDealt = gVolatileStructs[gActiveBattler].substituteHP;
                gVolatileStructs[gActiveBattler].substituteHP = 0;
            }
            // check substitute fading
            if (gVolatileStructs[gActiveBattler].substituteHP == 0) {
                BattleScriptCall(BattleScript_SubstituteFade);
                return;
            }
        } else if (gBattleMoveDamage > 0 && !(gHitMarker & HITMARKER_PASSIVE_DAMAGE) && RemainingNoDamageHits(gActiveBattler) > 0) {
            IncrementSingleUseAbilityCounter(gActiveBattler, GetNoDamageAbility(gActiveBattler), 1);
            if (RemainingNoDamageHits(gActiveBattler) <= 0) {
                BattleScriptCall(BattleScript_BattlerCanNoLongerEndureHits);
            }
        } else if (gBattleMoveDamage > 0 && DoesDisguiseBlockMove(gBattlerAttacker, gActiveBattler, gCurrentMove)) {
            ON_ABILITY(gActiveBattler, TRUE, gAbilities[ability].onDisguise, int newSpecies = gAbilities[ability].onDisguise(gActiveBattler, FALSE);
                       FILTER(newSpecies);
                       gBattleScripting.abilityPopupOverwrite = ability;
                       UpdateAbilityStateIndicesForNewSpecies(gActiveBattler, newSpecies);
                       gBattleMons[gActiveBattler].species = newSpecies;
                       BattleScriptCall(BattleScript_TargetFormChange);
                       break;)
        } else {
            gHitMarker &= ~(HITMARKER_IGNORE_SUBSTITUTE);
            if (gBattleMoveDamage < 0)  // hp goes up
            {
                gBattleMons[gActiveBattler].hp -= gBattleMoveDamage;
                if (gBattleMons[gActiveBattler].hp > gBattleMons[gActiveBattler].maxHP) gBattleMons[gActiveBattler].hp = gBattleMons[gActiveBattler].maxHP;

            } else  // hp goes down
            {
                if (gHitMarker & HITMARKER_SKIP_DMG_TRACK) {
                    gHitMarker &= ~(HITMARKER_SKIP_DMG_TRACK);
                } else {
                    gTakenDmg[gActiveBattler] += gBattleMoveDamage;
                    if (battlerType == BS_TARGET)
                        gTakenDmgByBattler[gActiveBattler] = gBattlerAttacker;
                    else
                        gTakenDmgByBattler[gActiveBattler] = gBattlerTarget;
                }

                if (gBattleMons[gActiveBattler].hp > gBattleMoveDamage) {
                    gBattleMons[gActiveBattler].hp -= gBattleMoveDamage;
                    gHpDealt = gBattleMoveDamage;
                } else {
                    gHpDealt = gBattleMons[gActiveBattler].hp;
                    gBattleMons[gActiveBattler].hp = 0;
                }

                if (!gTurnStructs[gActiveBattler].dmg && !(gHitMarker & HITMARKER_PASSIVE_DAMAGE)) gTurnStructs[gActiveBattler].dmg = gHpDealt;

                if (IS_MOVE_PHYSICAL(gCurrentMove) && !(gHitMarker & HITMARKER_PASSIVE_DAMAGE) && gCurrentMove != MOVE_PAIN_SPLIT) {
                    gRoundStructs[gActiveBattler].physicalDmg = gHpDealt;
                    gTurnStructs[gActiveBattler].physicalDmg = gHpDealt;
                    if (battlerType == BS_TARGET) {
                        gRoundStructs[gActiveBattler].physicalBattlerId = gBattlerAttacker;
                        gTurnStructs[gActiveBattler].physicalBattlerId = gBattlerAttacker;
                    } else {
                        gRoundStructs[gActiveBattler].physicalBattlerId = gBattlerTarget;
                        gTurnStructs[gActiveBattler].physicalBattlerId = gBattlerTarget;
                    }
                    IncrementTimesTookDamage(gActiveBattler);
                } else if (!IS_MOVE_PHYSICAL(gCurrentMove) && !(gHitMarker & HITMARKER_PASSIVE_DAMAGE)) {
                    gRoundStructs[gActiveBattler].specialDmg = gHpDealt;
                    gTurnStructs[gActiveBattler].specialDmg = gHpDealt;
                    if (battlerType == BS_TARGET) {
                        gRoundStructs[gActiveBattler].specialBattlerId = gBattlerAttacker;
                        gTurnStructs[gActiveBattler].specialBattlerId = gBattlerAttacker;
                    } else {
                        gRoundStructs[gActiveBattler].specialBattlerId = gBattlerTarget;
                        gTurnStructs[gActiveBattler].specialBattlerId = gBattlerTarget;
                    }
                    IncrementTimesTookDamage(gActiveBattler);
                }
                gRoundStructs[gActiveBattler].damaged = TRUE;
            }
            gHitMarker &= ~(HITMARKER_PASSIVE_DAMAGE);
            BtlController_EmitSetMonData(0, REQUEST_HP_BATTLE, 0, 2, &gBattleMons[gActiveBattler].hp);
            MarkBattlerForControllerExec(gActiveBattler);
        }
    } else {
        gActiveBattler = GetBattlerForBattleScript(battlerType);
        if (gTurnStructs[gActiveBattler].dmg == 0) gTurnStructs[gActiveBattler].dmg = 0xFFFF;
    }
}

static void Cmd_critmessage(void) {
    if (gBattleControllerExecFlags == 0) {
        if (gIsCriticalHit == TRUE && !(gMoveResultFlags & MOVE_RESULT_NO_EFFECT)) {
            PrepareStringBattle(STRINGID_CRITICALHIT, gBattlerAttacker);
            gBattleCommunication[MSG_DISPLAY] = 1;
        }
        gBattlescriptCurrInstr++;
    }
}

static void Cmd_effectivenesssound(void) {
    if (gBattleControllerExecFlags) return;

    gActiveBattler = gBattlerTarget;
    if (!(gMoveResultFlags & MOVE_RESULT_MISSED)) {
        switch (gMoveResultFlags & (~(MOVE_RESULT_MISSED))) {
            case MOVE_RESULT_SUPER_EFFECTIVE:
                BtlController_EmitPlaySE(0, SE_SUPER_EFFECTIVE);
                MarkBattlerForControllerExec(gActiveBattler);
                break;
            case MOVE_RESULT_NOT_VERY_EFFECTIVE:
                BtlController_EmitPlaySE(0, SE_NOT_EFFECTIVE);
                MarkBattlerForControllerExec(gActiveBattler);
                break;
            case MOVE_RESULT_DOESNT_AFFECT_FOE:
            case MOVE_RESULT_FAILED:
                // no sound
                break;
            case MOVE_RESULT_FOE_ENDURED:
            case MOVE_RESULT_ONE_HIT_KO:
            case MOVE_RESULT_FOE_HUNG_ON:
            case MOVE_RESULT_STURDIED:
            default:
                if (gMoveResultFlags & MOVE_RESULT_SUPER_EFFECTIVE) {
                    BtlController_EmitPlaySE(0, SE_SUPER_EFFECTIVE);
                    MarkBattlerForControllerExec(gActiveBattler);
                } else if (gMoveResultFlags & MOVE_RESULT_NOT_VERY_EFFECTIVE) {
                    BtlController_EmitPlaySE(0, SE_NOT_EFFECTIVE);
                    MarkBattlerForControllerExec(gActiveBattler);
                } else if (!(gMoveResultFlags & (MOVE_RESULT_DOESNT_AFFECT_FOE | MOVE_RESULT_FAILED))) {
                    BtlController_EmitPlaySE(0, SE_EFFECTIVE);
                    MarkBattlerForControllerExec(gActiveBattler);
                }
                break;
        }
    }
    gBattlescriptCurrInstr++;
}

static void Cmd_resultmessage(void) {
    u32 stringId = 0;

    if (gBattleControllerExecFlags) return;

    if (gMoveResultFlags & MOVE_RESULT_MISSED && (!(gMoveResultFlags & MOVE_RESULT_DOESNT_AFFECT_FOE) || gBattleCommunication[MISS_TYPE] > B_MSG_AVOIDED_ATK)) {
        if (gBattleCommunication[MISS_TYPE] > B_MSG_AVOIDED_ATK)  // Wonder Guard or Levitate - show the ability pop-up
            CreateAbilityPopUp(gBattlerTarget, GetBattlerAbility(gBattlerTarget), (gBattleTypeFlags & BATTLE_TYPE_DOUBLE) != 0);
        stringId = gMissStringIds[gBattleCommunication[MISS_TYPE]];
        gBattleCommunication[MSG_DISPLAY] = 1;
    } else {
        gBattleCommunication[MSG_DISPLAY] = 1;
        switch (gMoveResultFlags & (~MOVE_RESULT_MISSED)) {
            case MOVE_RESULT_SUPER_EFFECTIVE:
                if (!gTurnStructs[gBattlerAttacker].multiHitCounter)  // Don't print effectiveness on each hit in a multi hit attack
                    stringId = STRINGID_SUPEREFFECTIVE;
                break;
            case MOVE_RESULT_NOT_VERY_EFFECTIVE:
                if (!gTurnStructs[gBattlerAttacker].multiHitCounter) stringId = STRINGID_NOTVERYEFFECTIVE;
                break;
            case MOVE_RESULT_ONE_HIT_KO:
                stringId = STRINGID_ONEHITKO;
                break;
            case MOVE_RESULT_DOESNT_AFFECT_FOE:
                stringId = STRINGID_ITDOESNTAFFECT;
                break;
            case MOVE_RESULT_FOE_ENDURED:
                if (!gTurnStructs[gBattlerAttacker].multiHitCounter) stringId = STRINGID_PKMNENDUREDHIT;
                break;
            case MOVE_RESULT_FAILED:
                stringId = STRINGID_BUTITFAILED;
                break;
            case MOVE_RESULT_FOE_HUNG_ON:
                gLastUsedItem = gBattleMons[gBattlerTarget].item;
                gPotentialItemEffectBattler = gBattlerTarget;
                gMoveResultFlags &= ~(MOVE_RESULT_FOE_ENDURED | MOVE_RESULT_FOE_HUNG_ON);
                BattleScriptCall(BattleScript_HangedOnMsg);
                return;
            default:
                if (gMoveResultFlags & MOVE_RESULT_DOESNT_AFFECT_FOE) {
                    stringId = STRINGID_ITDOESNTAFFECT;
                } else if (gMoveResultFlags & MOVE_RESULT_ONE_HIT_KO) {
                    gMoveResultFlags &= ~(MOVE_RESULT_ONE_HIT_KO);
                    gMoveResultFlags &= ~(MOVE_RESULT_SUPER_EFFECTIVE);
                    gMoveResultFlags &= ~(MOVE_RESULT_NOT_VERY_EFFECTIVE);
                    BattleScriptCall(BattleScript_OneHitKOMsg);
                    return;
                } else if (gMoveResultFlags & MOVE_RESULT_STURDIED) {
                    gMoveResultFlags &= ~(MOVE_RESULT_STURDIED | MOVE_RESULT_FOE_ENDURED | MOVE_RESULT_FOE_HUNG_ON);
                    gTurnStructs[gBattlerTarget].sturdied = FALSE;
                    if (gTurnStructs[gBattlerTarget].haloed) SetSingleUseAbilityCounter(gBattlerTarget, ABILITY_LUCKY_HALO, TRUE);
                    gTurnStructs[gBattlerTarget].haloed = FALSE;
                    BattleScriptCall(BattleScript_SturdiedMsg);
                    return;
                } else if (gMoveResultFlags & MOVE_RESULT_FOE_ENDURED && !gTurnStructs[gBattlerAttacker].multiHitCounter) {
                    gMoveResultFlags &= ~(MOVE_RESULT_FOE_ENDURED | MOVE_RESULT_FOE_HUNG_ON);
                    BattleScriptCall(BattleScript_EnduredMsg);
                    return;
                } else if (gMoveResultFlags & MOVE_RESULT_FOE_HUNG_ON) {
                    gLastUsedItem = gBattleMons[gBattlerTarget].item;
                    gPotentialItemEffectBattler = gBattlerTarget;
                    gMoveResultFlags &= ~(MOVE_RESULT_FOE_ENDURED | MOVE_RESULT_FOE_HUNG_ON);
                    BattleScriptCall(BattleScript_HangedOnMsg);
                    return;
                } else if (gMoveResultFlags & MOVE_RESULT_FAILED) {
                    stringId = STRINGID_BUTITFAILED;
                } else {
                    gBattleCommunication[MSG_DISPLAY] = 0;
                }
        }
    }

    if (stringId) PrepareStringBattle(stringId, gBattlerAttacker);

    gBattlescriptCurrInstr++;

    // Print berry reducing message after result message.
    if (gTurnStructs[gBattlerTarget].berryReduced && !(gMoveResultFlags & MOVE_RESULT_NO_EFFECT)) {
        gTurnStructs[gBattlerTarget].berryReduced = FALSE;
        BattleScriptCall(BattleScript_PrintBerryReduceString);
    }

    if (!(gHitMarker & HITMARKER_IGNORE_SUBSTITUTE) && !(gMoveResultFlags & MOVE_RESULT_ONE_HIT_KO) && !(gMoveResultFlags & MOVE_RESULT_FOE_ENDURED) &&
        !(gMoveResultFlags & MOVE_RESULT_FAILED) && !(gMoveResultFlags & MOVE_RESULT_DOESNT_AFFECT_FOE) && gBattleMoves[gCurrentMove].split != SPLIT_STATUS &&
        gBattleMoves[gCurrentMove].power > 0 && gBattleMoveDamage > 0 && (gTurnStructs[gBattlerAttacker].multiHitCounter == 0) &&
        !FlagGet(FLAG_SYS_DISABLE_DAMAGE_DONE) && gSaveBlock2Ptr->damageDone) {
        PREPARE_HWORD_NUMBER_BUFFER(gBattleTextBuff4, 4, VarGet(VAR_DAMAGE_DONE));
        BattleScriptCall(BattleScript_PrintDamageDoneString);
    }
}

static void Cmd_printstring(void) {
    if (gBattleControllerExecFlags == 0) {
        u16 var = READ_FIRST_16_INC;

        PrepareStringBattle(var, gBattlerAttacker);
        gBattleCommunication[MSG_DISPLAY] = 1;
    }
}

static void Cmd_printselectionstring(void) {
    gActiveBattler = gBattlerAttacker;

    BtlController_EmitPrintSelectionString(0, T2_READ_16(gBattlescriptCurrInstr + 1));
    MarkBattlerForControllerExec(gActiveBattler);

    gBattlescriptCurrInstr += 3;
    gBattleCommunication[MSG_DISPLAY] = 1;
}

static void Cmd_waitmessage(void) {
    if (gBattleControllerExecFlags == 0) {
        if (!gBattleCommunication[MSG_DISPLAY]) {
            gBattlescriptCurrInstr += 3;
        } else {
            u16 toWait = T2_READ_16(gBattlescriptCurrInstr + 1);
            if (++gPauseCounterBattle >= toWait) {
                gPauseCounterBattle = 0;
                gBattlescriptCurrInstr += 3;
                gBattleCommunication[MSG_DISPLAY] = 0;
            }
        }
    }
}

static void Cmd_printfromtable(void) {
    if (gBattleControllerExecFlags == 0) {
        const u16* ptr = (const u16*)T1_READ_PTR(gBattlescriptCurrInstr + 1);
        ptr += gBattleCommunication[MULTISTRING_CHOOSER];

        gBattlescriptCurrInstr += 5;
        PrepareStringBattle(*ptr, gBattlerAttacker);
        gBattleCommunication[MSG_DISPLAY] = 1;
    }
}

static void Cmd_printselectionstringfromtable(void) {
    if (gBattleControllerExecFlags == 0) {
        const u16* ptr = (const u16*)T1_READ_PTR(gBattlescriptCurrInstr + 1);
        ptr += gBattleCommunication[MULTISTRING_CHOOSER];

        gActiveBattler = gBattlerAttacker;
        BtlController_EmitPrintSelectionString(0, *ptr);
        MarkBattlerForControllerExec(gActiveBattler);

        gBattlescriptCurrInstr += 5;
        gBattleCommunication[MSG_DISPLAY] = 1;
    }
}

u8 GetBattlerTurnOrderNum(u8 battlerId) {
    s32 i;
    for (i = 0; i < gBattlersCount; i++) {
        if (gBattlerByTurnOrder[i] == battlerId) break;
    }
    return i;
}

void ClearPowerOfAlchemyState(int alchemyBattler, int battler) {
    int state;
    if (!BattlerHasAbility(alchemyBattler, ABILITY_POWER_OF_ALCHEMY, FALSE)) return;
    alchemyBattler--;
    state = GetAbilityState(alchemyBattler, ABILITY_POWER_OF_ALCHEMY);
    state &= ~(3 << (2 * battler));
    SetAbilityState(alchemyBattler, ABILITY_POWER_OF_ALCHEMY, state);
}

void SetPowerOfAlchemyState(int alchemyBattler, int battler, int item) {
    int state;
    state = GetAbilityState(alchemyBattler, ABILITY_POWER_OF_ALCHEMY);
    state &= ~(3 << (2 * battler));
    state |= ((item == ITEM_BLACK_SLUDGE || item == ITEM_BIG_NUGGET ? 2 : 1) << (2 * battler));
    SetAbilityState(alchemyBattler, ABILITY_POWER_OF_ALCHEMY, state);
}

int UpdateBattlerItem(int battler, int newItem) {
    int oldItem = gBattleMons[battler].item;
    int alchemyBattler;
    if (oldItem == newItem) return 0;
    if (oldItem == ITEM_NONE)
        SetAbilityState(battler, ABILITY_UNBURDEN, FALSE);
    else if (newItem == ITEM_NONE)
        SetAbilityState(battler, ABILITY_UNBURDEN, TRUE);

    if (newItem == ITEM_NONE && oldItem != ITEM_BIG_NUGGET && (alchemyBattler = IsAbilityOnField(ABILITY_POWER_OF_ALCHEMY)))
        SetPowerOfAlchemyState(alchemyBattler - 1, battler, oldItem);

    if (newItem == ITEM_NONE) gBattleStruct->choicedMove[battler] = 0;

    gActiveBattler = battler;
    gBattleMons[battler].item = newItem;
    BtlController_EmitSetMonData(0, REQUEST_HELDITEM_BATTLE, 0, 2, &gBattleMons[battler].item);  // remove target item
    MarkBattlerForControllerExec(battler);
    return oldItem;
}

// battlerStealer steals the item of battlerItem
void StealTargetItem(u8 battlerStealer, u8 battlerItem) {
    gLastUsedItem = UpdateBattlerItem(battlerItem, ITEM_NONE);
    UpdateBattlerItem(battlerStealer, gLastUsedItem);

    TrySaveExchangedItem(battlerItem, gLastUsedItem);
}

void RemoveItem(u8 battler) { gLastUsedItem = UpdateBattlerItem(battler, ITEM_NONE); }

#define RESET_RETURN                                    \
    {                                                   \
        gBattleScripting.moveEffect = 0;                \
        gBattleScripting.moveSecondaryEffectChance = 0; \
        return;                                         \
    }

bool8 IsPreventableSecondaryEffect(u8 moveEffect) {
    switch (moveEffect) {
        case MOVE_EFFECT_SLEEP:
        case MOVE_EFFECT_POISON:
        case MOVE_EFFECT_BURN:
        case MOVE_EFFECT_FREEZE:
        case MOVE_EFFECT_PARALYSIS:
        case MOVE_EFFECT_TOXIC:
        case MOVE_EFFECT_FROSTBITE:
        case MOVE_EFFECT_BLEED:
        case MOVE_EFFECT_CONFUSION:
        case MOVE_EFFECT_FLINCH:
        case MOVE_EFFECT_ATK_PLUS_1:
        case MOVE_EFFECT_DEF_PLUS_1:
        case MOVE_EFFECT_SPD_PLUS_1:
        case MOVE_EFFECT_SP_ATK_PLUS_1:
        case MOVE_EFFECT_SP_DEF_PLUS_1:
        case MOVE_EFFECT_ACC_PLUS_1:
        case MOVE_EFFECT_EVS_PLUS_1:
        case MOVE_EFFECT_ATK_MINUS_1:
        case MOVE_EFFECT_DEF_MINUS_1:
        case MOVE_EFFECT_SPD_MINUS_1:
        case MOVE_EFFECT_SP_ATK_MINUS_1:
        case MOVE_EFFECT_SP_DEF_MINUS_1:
        case MOVE_EFFECT_ACC_MINUS_1:
        case MOVE_EFFECT_EVS_MINUS_1:
        case MOVE_EFFECT_ATK_PLUS_2:
        case MOVE_EFFECT_DEF_PLUS_2:
        case MOVE_EFFECT_SPD_PLUS_2:
        case MOVE_EFFECT_SP_ATK_PLUS_2:
        case MOVE_EFFECT_SP_DEF_PLUS_2:
        case MOVE_EFFECT_ACC_PLUS_2:
        case MOVE_EFFECT_EVS_PLUS_2:
        case MOVE_EFFECT_ATK_MINUS_2:
        case MOVE_EFFECT_DEF_MINUS_2:
        case MOVE_EFFECT_SPD_MINUS_2:
        case MOVE_EFFECT_SP_ATK_MINUS_2:
        case MOVE_EFFECT_SP_DEF_MINUS_2:
        case MOVE_EFFECT_ACC_MINUS_2:
        case MOVE_EFFECT_EVS_MINUS_2:
        case MOVE_EFFECT_ATTRACT:
        case MOVE_EFFECT_CURSE:
        case MOVE_EFFECT_DISABLE:
        case MOVE_EFFECT_SALT_CURE:
        case MOVE_EFFECT_PREVENT_ESCAPE:
        case MOVE_EFFECT_NIGHTMARE:
        case MOVE_EFFECT_SYRUP:
            return TRUE;
        default:
            return FALSE;
    }
}

void SetMoveEffect(bool32 primary, u32 certain) {
    s32 i, byTwo = 0, affectsUser = 0;
    bool32 statusChanged = FALSE;
    bool32 mirrorArmorReflected = BattlerHasAbility(gBattlerTarget, ABILITY_MIRROR_ARMOR, TRUE);
    u32 flags = 0;
    bool16 ignoreTypeImmunities = gBattleScripting.moveEffect & MOVE_EFFECT_IGNORE_TYPE_IMMUNITIES;

    gBattleScripting.moveEffect &= ~MOVE_EFFECT_IGNORE_TYPE_IMMUNITIES;

    switch (gBattleScripting.moveEffect)  // Set move effects which happen later on
    {
        case MOVE_EFFECT_KNOCK_OFF:
        case MOVE_EFFECT_SMACK_DOWN:
        case MOVE_EFFECT_REMOVE_STATUS:
        case MOVE_EFFECT_BURN_UP:
        case MOVE_EFFECT_STEAL_ITEM:
        case MOVE_EFFECT_MAKE_IT_RAIN:
        case MOVE_EFFECT_WYRM_WIND:
        case MOVE_EFFECT_SCALE_SHOT:
        case MOVE_EFFECT_BUG_BITE:
            gBattleStruct->moveEffect2 = gBattleScripting.moveEffect;
            return;
    }

    if (gBattleScripting.moveEffect & MOVE_EFFECT_AFFECTS_USER) {
        gEffectBattler = gBattlerAttacker;  // battlerId that effects get applied on
        gBattleScripting.moveEffect &= ~(MOVE_EFFECT_AFFECTS_USER);
        affectsUser = MOVE_EFFECT_AFFECTS_USER;
        gBattleScripting.battler = gBattlerTarget;  // theoretically the attacker
    } else {
        gEffectBattler = gBattlerTarget;
        gBattleScripting.battler = gBattlerAttacker;
    }
    // Just in case this flag is still set
    gBattleScripting.moveEffect &= ~(MOVE_EFFECT_CERTAIN);

    if ((BATTLER_HAS_ABILITY(gEffectBattler, ABILITY_SHIELD_DUST) || GetBattlerHoldEffect(gEffectBattler, TRUE) == HOLD_EFFECT_COVERT_CLOAK) &&
        !(gHitMarker & HITMARKER_IGNORE_SAFEGUARD) && !primary && !affectsUser && IsPreventableSecondaryEffect(gBattleScripting.moveEffect))
        RESET_RETURN

    if (gSideStatuses[GET_BATTLER_SIDE(gEffectBattler)] & SIDE_STATUS_SAFEGUARD && !(gHitMarker & HITMARKER_IGNORE_SAFEGUARD) && !primary &&
        gBattleScripting.moveEffect <= MOVE_EFFECT_CONFUSION)
        RESET_RETURN

    if (TestSheerForceFlag(gBattlerAttacker, gCurrentMove)) RESET_RETURN

    if (gBattleMons[gEffectBattler].hp == 0) {
        switch (gBattleScripting.moveEffect) {
            case MOVE_EFFECT_PAYDAY:
            case MOVE_EFFECT_WATER_PLEDGE:
            case MOVE_EFFECT_FIRE_PLEDGE:
            case MOVE_EFFECT_GRASS_PLEDGE:
            case MOVE_EFFECT_SWAMP:
            case MOVE_EFFECT_RAINBOW:
            case MOVE_EFFECT_FIRE_SEA:
            case MOVE_EFFECT_SPECTRAL_THIEF:
                break;

            default:
                RESET_RETURN;
        }
    }

    if (DoesSubstituteBlockMove(gBattlerAttacker, gEffectBattler, gCurrentMove) && affectsUser != MOVE_EFFECT_AFFECTS_USER) RESET_RETURN

    if (gBattleScripting.moveEffect <= PRIMARY_STATUS_MOVE_EFFECT)  // status change
    {
        switch (sStatusFlagsForMoveEffects[gBattleScripting.moveEffect]) {
            case STATUS1_SLEEP:
                // check active uproar
                if (!IsSoundproof(gEffectBattler)) {
                    for (gActiveBattler = 0; gActiveBattler < gBattlersCount; gActiveBattler++)
                        if (gBattleMons[gActiveBattler].status2 & STATUS2_UPROAR) break;

                    if (gActiveBattler != gBattlersCount) break;
                }

                if (!CanSleep(gEffectBattler)) break;

                CancelMultiTurnMoves(gEffectBattler);
                statusChanged = TRUE;
                break;
            case STATUS1_POISON:
                if (BattlerHasAbility(gEffectBattler, ABILITY_IMMUNITY, TRUE) && (primary == TRUE || certain == MOVE_EFFECT_CERTAIN)) {
                    gBattleScripting.abilityPopupOverwrite = ABILITY_IMMUNITY;

                    BattleScriptCall(BattleScript_PSNPrevention);

                    if (gHitMarker & HITMARKER_IGNORE_SAFEGUARD) {
                        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_ABILITY_PREVENTS_ABILITY_STATUS;
                        gHitMarker &= ~(HITMARKER_IGNORE_SAFEGUARD);
                    } else {
                        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_ABILITY_PREVENTS_MOVE_STATUS;
                    }
                    RESET_RETURN
                }
                if (!CanPoisonType(gBattleScripting.battler, gEffectBattler) && (gHitMarker & HITMARKER_IGNORE_SAFEGUARD) &&
                    (primary == TRUE || certain == MOVE_EFFECT_CERTAIN)) {
                    BattleScriptCall(BattleScript_PSNPrevention);

                    gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_STATUS_HAD_NO_EFFECT;
                    RESET_RETURN
                }
                if (!CanBePoisoned(gBattleScripting.battler, gEffectBattler)) break;

                statusChanged = TRUE;
                break;
            case STATUS1_BURN: {
                u16 blockingAbility = 0;

                if (gBattleMons[gEffectBattler].status1 & STATUS1_ANY || BattlerHasAbility(gEffectBattler, ABILITY_COMATOSE, TRUE) ||
                    BattlerHasAbility(gEffectBattler, ABILITY_BLOOD_STAIN, TRUE))
                    break;

                if (BattlerHasAbility(gEffectBattler, ABILITY_WATER_VEIL, TRUE))
                    blockingAbility = ABILITY_WATER_VEIL;
                else if (BattlerHasAbility(gEffectBattler, ABILITY_PURIFYING_WATERS, TRUE))
                    blockingAbility = ABILITY_PURIFYING_WATERS;
                else if (BattlerHasAbility(gEffectBattler, ABILITY_THERMAL_EXCHANGE, TRUE))
                    blockingAbility = ABILITY_THERMAL_EXCHANGE;
                else if (BattlerHasAbility(gEffectBattler, ABILITY_WATER_BUBBLE, TRUE))
                    blockingAbility = ABILITY_WATER_BUBBLE;

                if (blockingAbility && (primary == TRUE || certain == MOVE_EFFECT_CERTAIN)) {
                    gBattleScripting.abilityPopupOverwrite = blockingAbility;

                    BattleScriptCall(BattleScript_BRNPrevention);
                    if (gHitMarker & HITMARKER_IGNORE_SAFEGUARD) {
                        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_ABILITY_PREVENTS_ABILITY_STATUS;
                        gHitMarker &= ~(HITMARKER_IGNORE_SAFEGUARD);
                    } else {
                        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_ABILITY_PREVENTS_MOVE_STATUS;
                    }
                    RESET_RETURN
                }
                if (IS_BATTLER_OF_TYPE(gEffectBattler, TYPE_FIRE) && (gHitMarker & HITMARKER_IGNORE_SAFEGUARD) &&
                    (primary == TRUE || certain == MOVE_EFFECT_CERTAIN)) {
                    BattleScriptCall(BattleScript_BRNPrevention);

                    gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_STATUS_HAD_NO_EFFECT;
                    RESET_RETURN
                }

                if (!CanBeBurned(gEffectBattler)) break;

                statusChanged = TRUE;
            } break;
            case STATUS1_FREEZE:
                if (!CanBeFrozen(gEffectBattler)) break;

                CancelMultiTurnMoves(gEffectBattler);
                statusChanged = TRUE;
                break;
            case STATUS1_PARALYSIS: {
                u16 blockingAbility = 0;

                if (gBattleMons[gEffectBattler].status1 & STATUS1_ANY || BattlerHasAbility(gEffectBattler, ABILITY_COMATOSE, TRUE) ||
                    BattlerHasAbility(gEffectBattler, ABILITY_BLOOD_STAIN, TRUE))
                    break;

                if (BattlerHasAbility(gEffectBattler, ABILITY_LIMBER, TRUE))
                    blockingAbility = ABILITY_LIMBER;
                else if (BattlerHasAbility(gEffectBattler, ABILITY_JUGGERNAUT, TRUE))
                    blockingAbility = ABILITY_JUGGERNAUT;

                if (blockingAbility) {
                    if (primary == TRUE || certain == MOVE_EFFECT_CERTAIN) {
                        gBattleScripting.abilityPopupOverwrite = blockingAbility;

                        BattleScriptCall(BattleScript_PRLZPrevention);

                        if (gHitMarker & HITMARKER_IGNORE_SAFEGUARD) {
                            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_ABILITY_PREVENTS_ABILITY_STATUS;
                            gHitMarker &= ~(HITMARKER_IGNORE_SAFEGUARD);
                        } else {
                            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_ABILITY_PREVENTS_MOVE_STATUS;
                        }
                        RESET_RETURN
                    } else
                        break;
                }
                if (ignoreTypeImmunities) {
                    statusChanged = TRUE;
                    break;
                }
                if (!CanParalyzeType(gBattleScripting.battler, gEffectBattler) && (gHitMarker & HITMARKER_IGNORE_SAFEGUARD) &&
                    (primary == TRUE || certain == MOVE_EFFECT_CERTAIN)) {
                    BattleScriptCall(BattleScript_PRLZPrevention);

                    gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_STATUS_HAD_NO_EFFECT;
                    RESET_RETURN
                }
                if (!CanBeParalyzed(gBattleScripting.battler, gEffectBattler)) break;

                statusChanged = TRUE;
            } break;
            case STATUS1_TOXIC_POISON:
                if (BattlerHasAbility(gEffectBattler, ABILITY_IMMUNITY, TRUE) && (primary == TRUE || certain == MOVE_EFFECT_CERTAIN)) {
                    gBattleScripting.abilityPopupOverwrite = ABILITY_IMMUNITY;

                    BattleScriptCall(BattleScript_PSNPrevention);

                    if (gHitMarker & HITMARKER_IGNORE_SAFEGUARD) {
                        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_ABILITY_PREVENTS_ABILITY_STATUS;
                        gHitMarker &= ~(HITMARKER_IGNORE_SAFEGUARD);
                    } else {
                        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_ABILITY_PREVENTS_MOVE_STATUS;
                    }
                    RESET_RETURN
                }
                if (!CanPoisonType(gBattleScripting.battler, gEffectBattler) && (gHitMarker & HITMARKER_IGNORE_SAFEGUARD) &&
                    (primary == TRUE || certain == MOVE_EFFECT_CERTAIN)) {
                    BattleScriptCall(BattleScript_PSNPrevention);

                    gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_STATUS_HAD_NO_EFFECT;
                    RESET_RETURN
                }
                if (gBattleMons[gEffectBattler].status1) break;
                if (CanBePoisoned(gBattleScripting.battler, gEffectBattler)) {
                    // It's redundant, because at this point we know the status1 value is 0.
                    gBattleMons[gEffectBattler].status1 &= ~(STATUS1_TOXIC_POISON);
                    gBattleMons[gEffectBattler].status1 &= ~(STATUS1_POISON);
                    statusChanged = TRUE;
                    break;
                } else {
                    gMoveResultFlags |= MOVE_RESULT_DOESNT_AFFECT_FOE;
                }
                break;
            case STATUS1_FROSTBITE:
                if (!CanGetFrostbite(gEffectBattler)) break;

                statusChanged = TRUE;
                break;
            case STATUS1_BLEED:
                if (!CanBleed(gEffectBattler)) break;

                statusChanged = TRUE;
        }
        if (statusChanged == TRUE) {
            if (sStatusFlagsForMoveEffects[gBattleScripting.moveEffect] == STATUS1_SLEEP)
                gBattleMons[gEffectBattler].status1 |= (B_SLEEP_TURNS >= GEN_5) ? ((Random() % 3) + 2) : ((Random() % 4) + 3);
            else
                gBattleMons[gEffectBattler].status1 |= sStatusFlagsForMoveEffects[gBattleScripting.moveEffect];

            BattleScriptCall(sMoveEffectBS_Ptrs[gBattleScripting.moveEffect]);

            gActiveBattler = gEffectBattler;
            BtlController_EmitSetMonData(0, REQUEST_STATUS_BATTLE, 0, 4, &gBattleMons[gEffectBattler].status1);
            MarkBattlerForControllerExec(gActiveBattler);

            if (gHitMarker & HITMARKER_IGNORE_SAFEGUARD) {
                gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_STATUSED_BY_ABILITY;
                gHitMarker &= ~(HITMARKER_IGNORE_SAFEGUARD);
            } else {
                gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_STATUSED;
            }

            // for synchronize

            if (gBattleScripting.moveEffect == MOVE_EFFECT_POISON || gBattleScripting.moveEffect == MOVE_EFFECT_TOXIC ||
                gBattleScripting.moveEffect == MOVE_EFFECT_PARALYSIS || gBattleScripting.moveEffect == MOVE_EFFECT_BURN) {
                gBattleStruct->synchronizeMoveEffect = gBattleScripting.moveEffect;
                gHitMarker |= HITMARKER_SYNCHRONISE_EFFECT;
            }

            if (gBattleScripting.moveEffect == MOVE_EFFECT_POISON || gBattleScripting.moveEffect == MOVE_EFFECT_TOXIC) {
                SetBattlerAffectedFlag(gBattleScripting.battler, gEffectBattler, ABILITY_POISON_PUPPETEER);
                SetBattlerAffectedFlag(gBattleScripting.battler, gEffectBattler, ABILITY_NEUROTOXIN);
            } else if (gBattleScripting.moveEffect == MOVE_EFFECT_BURN)
                SetBattlerAffectedFlag(gBattleScripting.battler, gEffectBattler, ABILITY_SET_ABLAZE);
            return;
        } else if (statusChanged == FALSE) {
            gBattleScripting.moveEffect = 0;
            gBattleScripting.moveSecondaryEffectChance = 0;
            return;
        }
        return;
    } else {
        if (!(gBattleMons[gEffectBattler].status2 & sStatusFlagsForMoveEffects[gBattleScripting.moveEffect])) {
#define SET_MOVE_EFFECT_AS(effect)        \
    gBattleScripting.moveEffect = effect; \
    SetMoveEffect(primary, certain);

            switch (gBattleScripting.moveEffect) {
                case MOVE_EFFECT_ORDER_UP:
                    if (gBattleMons[gBattlerAttacker].species == SPECIES_DONDOZO && IsBattlerAlive(BATTLE_PARTNER(gBattlerAttacker)) &&
                        GetAbilityState(BATTLE_PARTNER(gBattlerAttacker), ABILITY_COMMANDER) == COMMANDER_ACTIVE) {
                        switch (gBattleMons[BATTLE_PARTNER(gBattlerAttacker)].species) {
                            case SPECIES_TATSUGIRI:
                            case SPECIES_TATSUGIRI_CURLY:
                                SET_MOVE_EFFECT_AS(MOVE_EFFECT_ATK_PLUS_1 | affectsUser)
                                break;
                            case SPECIES_TATSUGIRI_DROOPY:
                                SET_MOVE_EFFECT_AS(MOVE_EFFECT_DEF_PLUS_1 | affectsUser)
                                break;
                            case SPECIES_TATSUGIRI_STRETCHY:
                                SET_MOVE_EFFECT_AS(MOVE_EFFECT_SPD_PLUS_1 | affectsUser)
                                break;
                        }
                    }
                    break;
                case MOVE_EFFECT_WATER_PLEDGE:
                    if (IsWeatherActive(WEATHER_SUN_ANY) || HasChloroplast(gBattlerAttacker)) {
                        SET_MOVE_EFFECT_AS(MOVE_EFFECT_RAINBOW | MOVE_EFFECT_AFFECTS_USER)
                    }
                    if (IsTerrainActive(STATUS_FIELD_GRASSY_TERRAIN)) {
                        SET_MOVE_EFFECT_AS(MOVE_EFFECT_SWAMP)
                    }
                    break;
                case MOVE_EFFECT_FIRE_PLEDGE:
                    if (IsWeatherActive(WEATHER_RAIN_ANY)) {
                        SET_MOVE_EFFECT_AS(MOVE_EFFECT_RAINBOW | MOVE_EFFECT_AFFECTS_USER)
                    }
                    if (IsTerrainActive(STATUS_FIELD_GRASSY_TERRAIN)) {
                        SET_MOVE_EFFECT_AS(MOVE_EFFECT_FIRE_SEA)
                    }
                    break;
                case MOVE_EFFECT_GRASS_PLEDGE:
                    if (IsWeatherActive(WEATHER_SUN_ANY) || HasChloroplast(gBattlerAttacker)) {
                        SET_MOVE_EFFECT_AS(MOVE_EFFECT_FIRE_SEA)
                    }
                    if (IsWeatherActive(WEATHER_RAIN_ANY)) {
                        SET_MOVE_EFFECT_AS(MOVE_EFFECT_SWAMP)
                    }
                    break;
                case MOVE_EFFECT_RAINBOW:
                    if (!gSideTimers[GetBattlerSide(gEffectBattler)].rainbowTimer) {
                        gSideTimers[GetBattlerSide(gEffectBattler)].started.rainbow = TRUE;
                        gSideTimers[GetBattlerSide(gEffectBattler)].rainbowTimer = PLEDGE_DURATION;
                        BattleScriptCall(BattleScript_RainbowStart);
                    }
                    break;
                case MOVE_EFFECT_SWAMP:
                    if (!gSideTimers[GetBattlerSide(gEffectBattler)].swampTimer) {
                        gSideTimers[GetBattlerSide(gEffectBattler)].started.swamp = TRUE;
                        gSideTimers[GetBattlerSide(gEffectBattler)].swampTimer = PLEDGE_DURATION;
                        BattleScriptCall(BattleScript_SwampStart);
                    }
                    break;
                case MOVE_EFFECT_FIRE_SEA:
                    if (!gSideTimers[GetBattlerSide(gEffectBattler)].fireSeaTimer) {
                        gSideTimers[GetBattlerSide(gEffectBattler)].started.fireSea = TRUE;
                        gSideTimers[GetBattlerSide(gEffectBattler)].fireSeaTimer = PLEDGE_DURATION;
                        BattleScriptCall(BattleScript_SeaOfFireStart);
                    }
                    break;
                case MOVE_EFFECT_CONFUSION:
                    if (CanBeConfused(gEffectBattler)) {
                        SetBattlerAffectedFlag(gBattleScripting.battler, gEffectBattler, ABILITY_ENTRANCE);
                        gBattleMons[gEffectBattler].status2 |= STATUS2_CONFUSION_TURN(((Random()) % 4) + 2);  // 2-5 turns

                        BattleScriptCall(sMoveEffectBS_Ptrs[gBattleScripting.moveEffect]);
                    }
                    break;
                case MOVE_EFFECT_FLINCH:
                    if (!BATTLER_HAS_ABILITY(gEffectBattler, ABILITY_INNER_FOCUS) && !BATTLER_HAS_ABILITY(gEffectBattler, ABILITY_ENLIGHTENED) &&
                        !BATTLER_HAS_ABILITY(gEffectBattler, ABILITY_UNLOCKED_POTENTIAL) && !BATTLER_HAS_ABILITY(gEffectBattler, ABILITY_WAY_OF_PRECISION)) {
                        gBattleMons[gEffectBattler].status2 |= sStatusFlagsForMoveEffects[gBattleScripting.moveEffect];
                    }
                    break;
                case MOVE_EFFECT_UPROAR:
                    if (!(gBattleMons[gEffectBattler].status2 & STATUS2_UPROAR) && !gProcessingExtraAttacks) {
                        gBattleMons[gEffectBattler].status2 |= STATUS2_MULTIPLETURNS;
                        gLockedMoves[gEffectBattler] = gCurrentMove;
                        gBattleMons[gEffectBattler].status2 |= STATUS2_UPROAR_TURN(B_UPROAR_TURNS >= GEN_5 ? 3 : ((Random() & 3) + 2));

                        BattleScriptCall(sMoveEffectBS_Ptrs[gBattleScripting.moveEffect]);
                    }
                    break;
                case MOVE_EFFECT_PAYDAY:
                    if (GET_BATTLER_SIDE(gBattlerAttacker) == B_SIDE_PLAYER) {
                        u16 PayDay = gPaydayMoney;
                        gPaydayMoney += (gBattleMons[gBattlerAttacker].level * 5);
                        if (PayDay > gPaydayMoney) gPaydayMoney = 0xFFFF;
                    }
                    BattleScriptCall(sMoveEffectBS_Ptrs[gBattleScripting.moveEffect]);
                    if (IsBattlerAlive(gBattlerTarget) && GetBattlerHoldEffect(gBattlerAttacker, TRUE) == HOLD_EFFECT_AMULET_COIN &&
                        (gBattleMons[gBattlerAttacker].species == SPECIES_MEOWTH_PARTNER ||
                         gBattleMons[gBattlerAttacker].species == SPECIES_MEOWTH_PARTNER_MEGA)) {
                        SET_MOVE_EFFECT_AS(MOVE_EFFECT_MAKE_IT_RAIN)
                    }
                    break;
                case MOVE_EFFECT_HAPPY_HOUR:
                    if (GET_BATTLER_SIDE(gBattlerAttacker) == B_SIDE_PLAYER && !gBattleStruct->moneyMultiplierMove) {
                        gBattleStruct->moneyMultiplier *= 2;
                        gBattleStruct->moneyMultiplierMove = 1;
                    }
                    break;
                case MOVE_EFFECT_TRI_ATTACK:
                    if (!gBattleMons[gEffectBattler].status1) {
#if B_USE_FROSTBITE == TRUE
                        static const u8 sTriAttackEffects[] = {MOVE_EFFECT_BURN, MOVE_EFFECT_FROSTBITE, MOVE_EFFECT_PARALYSIS};
#else
                        static const u8 sTriAttackEffects[] = {MOVE_EFFECT_BURN, MOVE_EFFECT_FREEZE, MOVE_EFFECT_PARALYSIS};
#endif
                        SET_MOVE_EFFECT_AS(sTriAttackEffects[Random() % 3])
                    }
                    break;
                case MOVE_EFFECT_DIRE_CLAW:
                    if (!gBattleMons[gEffectBattler].status1) {
                        static const u8 sDireClawEffects[] = {MOVE_EFFECT_POISON, MOVE_EFFECT_BLEED, MOVE_EFFECT_PARALYSIS};
                        SET_MOVE_EFFECT_AS(sDireClawEffects[Random() % 3])
                    }
                    break;
                case MOVE_EFFECT_CHARGING:
                    if (!gProcessingExtraAttacks) {
                        gBattleMons[gEffectBattler].status2 |= STATUS2_MULTIPLETURNS;
                        gLockedMoves[gEffectBattler] = gCurrentMove;
                        gRoundStructs[gEffectBattler].chargingTurn = TRUE;
                    }
                    break;
                case MOVE_EFFECT_CURSE:
                    if (!(gBattleMons[gEffectBattler].status2 & STATUS2_CURSED)) {
                        gBattleMons[gEffectBattler].status2 |= STATUS2_CURSED;
                        BattleScriptCall(sMoveEffectBS_Ptrs[gBattleScripting.moveEffect]);
                    }
                    break;
                case MOVE_EFFECT_ATTRACT:
                    if (CanInfatuate(gBattleScripting.battler, gEffectBattler)) {
                        gBattleMons[gEffectBattler].status2 |= STATUS2_INFATUATED_WITH(gBattleScripting.battler);
                        BattleScriptCall(sMoveEffectBS_Ptrs[gBattleScripting.moveEffect]);
                    }
                    break;
                case MOVE_EFFECT_WRAP:
                    if (!(gBattleMons[gEffectBattler].status2 & STATUS2_WRAPPED)) {
                        bool8 hasGrappler = BATTLER_HAS_ABILITY(gBattlerAttacker, ABILITY_GRAPPLER);
                        bool8 hasGripClaw = GetBattlerHoldEffect(gBattlerAttacker, TRUE) == HOLD_EFFECT_GRIP_CLAW;
                        gBattleMons[gEffectBattler].status2 |= STATUS2_WRAPPED;
                        if (hasGrappler && hasGripClaw)
                            gVolatileStructs[gEffectBattler].wrapTurns = 8;
                        else if (hasGrappler)
                            gVolatileStructs[gEffectBattler].wrapTurns = 6;
                        else if (hasGripClaw)
                            gVolatileStructs[gEffectBattler].wrapTurns = 7;
                        else
                            gVolatileStructs[gEffectBattler].wrapTurns = (Random() % 2) + 4;

                        gBattleStruct->wrappedMove[gEffectBattler] = gCurrentMove;
                        gBattleStruct->wrappedBy[gEffectBattler] = gBattlerAttacker;

                        for (gBattleCommunication[MULTISTRING_CHOOSER] = 0;; gBattleCommunication[MULTISTRING_CHOOSER]++) {
                            if (gBattleCommunication[MULTISTRING_CHOOSER] > ARRAY_COUNT(sTrappingMoves) - 1) break;
                            if (sTrappingMoves[gBattleCommunication[MULTISTRING_CHOOSER]] == gCurrentMove) break;
                        }

                        BattleScriptCall(sMoveEffectBS_Ptrs[gBattleScripting.moveEffect]);
                    }
                    break;
                case MOVE_EFFECT_ATK_PLUS_1:
                case MOVE_EFFECT_DEF_PLUS_1:
                case MOVE_EFFECT_SPD_PLUS_1:
                case MOVE_EFFECT_SP_ATK_PLUS_1:
                case MOVE_EFFECT_SP_DEF_PLUS_1:
                case MOVE_EFFECT_ACC_PLUS_1:
                case MOVE_EFFECT_EVS_PLUS_1:
                    if (!NoAliveMonsForEitherParty() &&
                        ChangeStatBuffsImplicit(1, gBattleScripting.moveEffect - MOVE_EFFECT_ATK_PLUS_1 + 1, affectsUser | STAT_BUFF_UPDATE_MOVE_EFFECT, 0)) {
                        gBattleScripting.animArg1 = gBattleScripting.moveEffect & ~(MOVE_EFFECT_AFFECTS_USER | MOVE_EFFECT_CERTAIN);
                        gBattleScripting.animArg2 = 0;
                        BattleScriptCall(BattleScript_StatUp);
                    }
                    break;
                case MOVE_EFFECT_ATK_MINUS_1:
                case MOVE_EFFECT_DEF_MINUS_1:
                case MOVE_EFFECT_SPD_MINUS_1:
                case MOVE_EFFECT_SP_ATK_MINUS_1:
                case MOVE_EFFECT_SP_DEF_MINUS_1:
                case MOVE_EFFECT_ACC_MINUS_1:
                case MOVE_EFFECT_EVS_MINUS_1:
                    flags = affectsUser;
                    if (mirrorArmorReflected && !affectsUser) flags |= STAT_BUFF_ALLOW_PTR;

                    if (ChangeStatBuffsImplicit(
                            -1, gBattleScripting.moveEffect - MOVE_EFFECT_ATK_MINUS_1 + 1, flags | STAT_BUFF_UPDATE_MOVE_EFFECT, gBattlescriptCurrInstr)) {
                        gBattleScripting.animArg1 = gBattleScripting.moveEffect & ~(MOVE_EFFECT_AFFECTS_USER | MOVE_EFFECT_CERTAIN);
                        gBattleScripting.animArg2 = 0;
                        BattleScriptCall(BattleScript_StatDown);
                    }
                    break;
                case MOVE_EFFECT_ATK_PLUS_2:
                case MOVE_EFFECT_DEF_PLUS_2:
                case MOVE_EFFECT_SPD_PLUS_2:
                case MOVE_EFFECT_SP_ATK_PLUS_2:
                case MOVE_EFFECT_SP_DEF_PLUS_2:
                case MOVE_EFFECT_ACC_PLUS_2:
                case MOVE_EFFECT_EVS_PLUS_2:
                    if (!NoAliveMonsForEitherParty() &&
                        ChangeStatBuffsImplicit(2, gBattleScripting.moveEffect - MOVE_EFFECT_ATK_PLUS_2 + 1, affectsUser | STAT_BUFF_UPDATE_MOVE_EFFECT, 0)) {
                        gBattleScripting.animArg1 = gBattleScripting.moveEffect & ~(MOVE_EFFECT_AFFECTS_USER | MOVE_EFFECT_CERTAIN);
                        gBattleScripting.animArg2 = 0;
                        BattleScriptCall(BattleScript_StatUp);
                    }
                    break;
                case MOVE_EFFECT_ATK_MINUS_2:
                case MOVE_EFFECT_DEF_MINUS_2:
                case MOVE_EFFECT_SPD_MINUS_2:
                case MOVE_EFFECT_SP_ATK_MINUS_2:
                case MOVE_EFFECT_SP_DEF_MINUS_2:
                case MOVE_EFFECT_ACC_MINUS_2:
                case MOVE_EFFECT_EVS_MINUS_2:
                    flags = affectsUser;
                    if (mirrorArmorReflected && !affectsUser) flags |= STAT_BUFF_ALLOW_PTR;

                    if (ChangeStatBuffsImplicit(
                            -2, gBattleScripting.moveEffect - MOVE_EFFECT_ATK_MINUS_2 + 1, flags | STAT_BUFF_UPDATE_MOVE_EFFECT, gBattlescriptCurrInstr)) {
                        gBattleScripting.animArg1 = gBattleScripting.moveEffect & ~(MOVE_EFFECT_AFFECTS_USER | MOVE_EFFECT_CERTAIN);
                        gBattleScripting.animArg2 = 0;
                        BattleScriptCall(BattleScript_StatDown);
                    }
                    break;
                case MOVE_EFFECT_RECHARGE:
                    if (!gProcessingExtraAttacks) {
                        gBattleMons[gEffectBattler].status2 |= STATUS2_RECHARGE;
                        gVolatileStructs[gEffectBattler].rechargeTimer = 2;
                        gLockedMoves[gEffectBattler] = gCurrentMove;
                    }
                    break;
                case MOVE_EFFECT_RAGE:
                    gBattleMons[gBattlerAttacker].status2 |= STATUS2_RAGE;
                    break;
                case MOVE_EFFECT_PREVENT_ESCAPE:
                    gBattleMons[gBattlerTarget].status2 |= STATUS2_ESCAPE_PREVENTION;
                    gVolatileStructs[gBattlerTarget].battlerPreventingEscape = gBattlerAttacker;
                    break;
                case MOVE_EFFECT_NIGHTMARE:
                    if (gBattleMons[gBattlerTarget].status2 && STATUS2_NIGHTMARE) break;
                    gBattleMons[gBattlerTarget].status2 |= STATUS2_NIGHTMARE;
                    BattleScriptCall(BattleScript_NightmareStarts);
                    break;
                case MOVE_EFFECT_ALL_STATS_UP:
                    if (!NoAliveMonsForEitherParty()) {
                        BattleScriptCall(BattleScript_AllStatsUp);
                    }
                    break;
                case MOVE_EFFECT_RAPIDSPIN:
                    BattleScriptCall(BattleScript_RapidSpinAway);
                    break;
                case MOVE_EFFECT_ATK_DEF_DOWN:  // SuperPower
                    BattleScriptCall(BattleScript_AtkDefDown);
                    break;
                case MOVE_EFFECT_DEF_SPDEF_DOWN:  // Close Combat
                    if (!BATTLER_HAS_ABILITY(gBattlerAttacker, ABILITY_BAD_COMPANY)) {
                        BattleScriptCall(BattleScript_DefSpDefDown);
                    }
                    break;
                case MOVE_EFFECT_RECOIL_HP_25:  // Struggle
                    gBattleMoveDamage = (gBattleMons[gEffectBattler].maxHP) / 4;
                    if (gBattleMoveDamage == 0) gBattleMoveDamage = 1;

                    BattleScriptCall(BattleScript_MoveEffectRecoil);
                    break;
                case MOVE_EFFECT_THRASH:
                    if (gBattleMons[gEffectBattler].status2 & STATUS2_LOCK_CONFUSE || gProcessingExtraAttacks) {
                        break;
                    } else {
                        gBattleMons[gEffectBattler].status2 |= STATUS2_MULTIPLETURNS;
                        gLockedMoves[gEffectBattler] = gCurrentMove;
                        gBattleMons[gEffectBattler].status2 |= STATUS2_LOCK_CONFUSE_TURN((Random() & 1) + 2);  // thrash for 2-3 turns
                    }
                    break;
                case MOVE_EFFECT_SP_ATK_TWO_DOWN:  // Overheat
                    break;
                case MOVE_EFFECT_CLEAR_SMOG:
                    for (i = 0; i < NUM_BATTLE_STATS; i++) {
                        if (gBattleMons[gEffectBattler].statStages[i] != 6) break;
                    }
                    if ((gTurnStructs[gEffectBattler].physicalDmg || gTurnStructs[gEffectBattler].specialDmg) && i != NUM_BATTLE_STATS) {
                        for (i = 0; i < NUM_BATTLE_STATS; i++) gBattleMons[gEffectBattler].statStages[i] = 6;
                        BattleScriptCall(BattleScript_MoveEffectClearSmog);
                    }
                    break;
                case MOVE_EFFECT_FLAME_BURST:
                    if (IsBattlerAlive(BATTLE_PARTNER(gBattlerTarget)) && !IsMagicGuardProtected(BATTLE_PARTNER(gBattlerTarget))) {
                        gBattleMoveDamage = gBattleMons[BATTLE_PARTNER(gBattlerTarget)].hp / 4;
                        if (gBattleMoveDamage == 0) gBattleMoveDamage = 1;
                        gBattlescriptCurrInstr = BattleScript_MoveEffectFlameBurst;
                    }
                    break;
                case MOVE_EFFECT_FEINT:
                    if (IS_BATTLER_PROTECTED(gBattlerTarget)) {
                        gRoundStructs[gBattlerTarget].isProtected = FALSE;
                        gRoundStructs[gBattlerTarget].protectedThisTurn = FALSE;
                        gSideStatuses[GetBattlerSide(gBattlerTarget)] &= ~(SIDE_STATUS_WIDE_GUARD);
                        gSideStatuses[GetBattlerSide(gBattlerTarget)] &= ~(SIDE_STATUS_CRAFTY_SHIELD);
                        gSideStatuses[GetBattlerSide(gBattlerTarget)] &= ~(SIDE_STATUS_MAT_BLOCK);
                        gRoundStructs[gBattlerTarget].spikyShielded = FALSE;
                        gRoundStructs[gBattlerTarget].kingsShielded = FALSE;
                        gRoundStructs[gBattlerTarget].angelsWrathProtected = FALSE;
                        gRoundStructs[gBattlerTarget].banefulBunkered = FALSE;
                        gRoundStructs[gBattlerTarget].obstructed = FALSE;
                        gRoundStructs[gBattlerTarget].silkTrapped = FALSE;
                        gRoundStructs[gBattlerTarget].burningBulwark = FALSE;
                        gRoundStructs[gBattlerTarget].tanglingHusked = FALSE;
                        if (gCurrentMove == MOVE_FEINT) {
                            BattleScriptCall(BattleScript_MoveEffectFeint);
                        } else if (gCurrentMove == MOVE_HYPERSPACE_FURY) {
                            BattleScriptCall(BattleScript_HyperspaceFuryRemoveProtect);
                        } else {
                            BattleScriptCall(BattleScript_AttackerRemovesProtect);
                        }
                    }
                    break;
                case MOVE_EFFECT_SPECTRAL_THIEF:
                    gBattleStruct->stolenStats[0] = 0;  // Stats to steal.
                    gBattleScripting.animArg1 = 0;
                    for (i = STAT_ATK; i < NUM_BATTLE_STATS; i++) {
                        if (gBattleMons[gBattlerTarget].statStages[i] > 6 && gBattleMons[gBattlerAttacker].statStages[i] != 12) {
                            gBattleStruct->stolenStats[0] |= gBitTable[i];
                            // Store by how many stages to raise the stat.
                            gBattleStruct->stolenStats[i] = gBattleMons[gBattlerTarget].statStages[i] - 6;
                            while (gBattleMons[gBattlerAttacker].statStages[i] + gBattleStruct->stolenStats[i] > 12) gBattleStruct->stolenStats[i]--;
                            gBattleMons[gBattlerTarget].statStages[i] = 6;

                            if (gBattleStruct->stolenStats[i] >= 2) byTwo++;

                            if (gBattleScripting.animArg1 == 0) {
                                if (byTwo)
                                    gBattleScripting.animArg1 = STAT_ANIM_PLUS2 - 1 + i;
                                else
                                    gBattleScripting.animArg1 = STAT_ANIM_PLUS1 - 1 + i;
                            } else {
                                if (byTwo)
                                    gBattleScripting.animArg1 = STAT_ANIM_MULTIPLE_PLUS2;
                                else
                                    gBattleScripting.animArg1 = STAT_ANIM_MULTIPLE_PLUS1;
                            }
                        }
                    }

                    if (gBattleStruct->stolenStats[0] != 0) {
                        BattleScriptCall(BattleScript_SpectralThiefSteal);
                    }
                    break;
                case MOVE_EFFECT_V_CREATE:
                    BattleScriptCall(BattleScript_VCreateStatLoss);
                    break;
                case MOVE_EFFECT_CORE_ENFORCER:
                    if (GetBattlerTurnOrderNum(gBattlerAttacker) > GetBattlerTurnOrderNum(gBattlerTarget)) {
                        BattleScriptCall(BattleScript_MoveEffectCoreEnforcer);
                    }
                    break;
                case MOVE_EFFECT_THROAT_CHOP:
                    gVolatileStructs[gEffectBattler].throatChopTimer = 2;
                    break;
                case MOVE_EFFECT_INCINERATE:
                    if ((B_INCINERATE_GEMS >= GEN_6 && GetBattlerHoldEffect(gEffectBattler, FALSE) == HOLD_EFFECT_GEMS) ||
                        (gBattleMons[gEffectBattler].item >= FIRST_BERRY_INDEX && gBattleMons[gEffectBattler].item <= LAST_BERRY_INDEX)) {
                        gLastUsedItem = UpdateBattlerItem(gEffectBattler, ITEM_NONE);
                        BattleScriptCall(BattleScript_MoveEffectIncinerate);
                    }
                    break;
                case MOVE_EFFECT_TRAP_BOTH:
                    if (!(gBattleMons[gBattlerTarget].status2 & STATUS2_ESCAPE_PREVENTION) &&
                        !(gBattleMons[gBattlerAttacker].status2 & STATUS2_ESCAPE_PREVENTION)) {
                        BattleScriptCall(BattleScript_BothCanNoLongerEscape);
                    }
                    if (!(gBattleMons[gBattlerTarget].status2 & STATUS2_ESCAPE_PREVENTION))
                        gVolatileStructs[gBattlerTarget].battlerPreventingEscape = gBattlerAttacker;

                    if (!(gBattleMons[gBattlerAttacker].status2 & STATUS2_ESCAPE_PREVENTION))
                        gVolatileStructs[gBattlerAttacker].battlerPreventingEscape = gBattlerTarget;

                    gBattleMons[gBattlerTarget].status2 |= STATUS2_ESCAPE_PREVENTION;
                    gBattleMons[gBattlerAttacker].status2 |= STATUS2_ESCAPE_PREVENTION;
                    break;
                case MOVE_EFFECT_DISABLE:
                    if (CanBeDisabled(gBattlerTarget)) {
                        DisableLastUsedMove(gBattlerTarget);
                        BattleScriptCall(BattleScript_MoveWasDisabledMessage);
                    }
                    break;
                case MOVE_EFFECT_GLAIVE_RUSH:
                    gRoundStructs[gBattlerAttacker].glaiveRush = TRUE;
                    break;
                case MOVE_EFFECT_SALT_CURE:
                    if (!(gStatuses4[gBattlerTarget] & STATUS4_SALT_CURE)) {
                        gStatuses4[gBattlerTarget] |= STATUS4_SALT_CURE;
                        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_SALT_CURE;
                        BattleScriptCall(BattleScript_AnnounceStatus);
                    }
                    break;
                case MOVE_EFFECT_SYRUP:
                    if (!gVolatileStructs[gEffectBattler].syrupTimer) {
                        if ((gBattleMons[gBattlerAttacker].species == SPECIES_DIPPLIN || gBattleMons[gBattlerAttacker].species == SPECIES_HYDRAPPLE)) {
                            gVolatileStructs[gEffectBattler].syrupBombIsShiny =
                                GetMonData(&GetSideParty(GetBattlerSide(gBattlerAttacker))[gBattlerPartyIndexes[gBattlerAttacker]], MON_DATA_IS_SHINY, NULL);
                        } else
                            gVolatileStructs[gEffectBattler].syrupBombIsShiny = FALSE;
                        gVolatileStructs[gEffectBattler].syrupTimer = 3;
                        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_SYRUP;
                        BattleScriptCall(BattleScript_AnnounceStatus);
                    }
                    break;
                case MOVE_EFFECT_SMOKESCREEN:
                    if (!gSideTimers[GET_BATTLER_SIDE(gBattlerAttacker)].smokescreenTimer) {
                        int side = GET_BATTLER_SIDE(gBattlerAttacker);
                        gSideTimers[side].smokescreenTimer =
                            GetBattlerHoldEffect(gBattlerAttacker, TRUE) == HOLD_EFFECT_LIGHT_CLAY ? SCREEN_DURATION_EXTENDED : SCREEN_DURATION;
                        gSideTimers[side].started.smokescreen = TRUE;
                        gSideTimers[side].smokescreenBattler = gBattlerAttacker;
                        gSideStatuses[side] |= SIDE_STATUS_SMOKESCREEN;
                        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_SMOKESCREEN;
                        BattleScriptCall(BattleScript_AnnounceStatus);
                    }
                    break;
                case MOVE_EFFECT_FEAR:
                    if (!gVolatileStructs[gEffectBattler].fear) {
                        BattleScriptCall(BattleScript_SetFearMoveEffect);
                    }
                    break;
                case MOVE_EFFECT_YAWN:
                    if (!(gStatuses3[gEffectBattler] & STATUS3_YAWN) && CanSleep(gEffectBattler)) {
                        BattleScriptCall(BattleScript_SetYawnMoveEffect);
                    }
                    break;
                case MOVE_EFFECT_PSYCHIC_NOISE:
                    if (!(gStatuses3[gEffectBattler] & STATUS3_HEAL_BLOCK)) {
                        BattleScriptCall(BattleScript_AnnounceHealBlock);
                        gStatuses3[gEffectBattler] |= STATUS3_HEAL_BLOCK;
                        gVolatileStructs[gEffectBattler].healBlockTimer = 2;
                    }
                    break;
                case MOVE_EFFECT_HIGHEST_STAT_EXCEPT_SPEED_PLUS_1:
                    SET_MOVE_EFFECT_AS((MOVE_EFFECT_ATK_PLUS_1 - STAT_ATK + GetHighestStatIdExcept(gEffectBattler, FALSE, STAT_SPEED)) | affectsUser)
                    break;
                case MOVE_EFFECT_DOUBLESLAP:
                    REQUIRE(gTurnStructs[gBattlerAttacker].multiHitsUsed >= 2)
                    SET_MOVE_EFFECT_AS(MOVE_EFFECT_CONFUSION | affectsUser)
                    break;
            }
        }
    }

    gBattleScripting.moveEffect = 0;
    gBattleScripting.moveSecondaryEffectChance = 0;
}

int GetMoveEffectChance(int battler, int move, int moveEffect, int baseChance) {
    switch (moveEffect) {
        case MOVE_EFFECT_BURN:
            if (BATTLER_HAS_ABILITY(battler, ABILITY_PYROMANCY)) baseChance *= 5;
            break;

        case MOVE_EFFECT_FROSTBITE:
            if (BATTLER_HAS_ABILITY(battler, ABILITY_CRYOMANCY)) baseChance *= 5;
            if (BATTLER_HAS_ABILITY(battler, ABILITY_SNOWY_WRATH)) baseChance *= 5;
            break;

        case MOVE_EFFECT_FLINCH:
            if (gTurnStructs[gBattlerAttacker].parentalBondOn < gTurnStructs[gBattlerAttacker].parentalBondInitialCount) return 0;
    }

    if (BATTLER_HAS_ABILITY(battler, ABILITY_SERENE_GRACE)) baseChance *= 2;

    if (gSideTimers[GetBattlerSide(battler)].rainbowTimer) baseChance *= 2;

    if (BATTLER_HAS_ABILITY(battler, ABILITY_PRECISE_FIST) && IsIronFistBoosted(battler, move)) baseChance *= 5;

    if (BATTLER_HAS_ABILITY(battler, ABILITY_WAY_OF_PRECISION) && IsIronFistBoosted(battler, move)) baseChance *= 5;

    // Angel's Wrath
    if (BATTLER_HAS_ABILITY(battler, ABILITY_ANGELS_WRATH) && move == MOVE_POISON_STING) baseChance = 100;

    return min(baseChance, 100);
}

static void Cmd_seteffectwithchance(void) {
    u32 percentChance = gBattleScripting.moveSecondaryEffectChance
                            ? (gBattleScripting.moveSecondaryEffectChance == 0xFF ? 0 : gBattleScripting.moveSecondaryEffectChance)
                            : gBattleMoves[gCurrentMove].secondaryEffectChance;
    u8 moveEffect;

    gBattlescriptCurrInstr++;

    FlagClear(FLAG_LAST_MOVE_SECONDARY_EFFECT_ACTIVATED);

    moveEffect = gBattleScripting.moveEffect & 0xFF;

    percentChance = GetMoveEffectChance(gBattlerAttacker, gCurrentMove, moveEffect, percentChance);

    if (gBattleScripting.moveEffect & MOVE_EFFECT_CERTAIN && !(gMoveResultFlags & MOVE_RESULT_NO_EFFECT)) {
        gBattleScripting.moveEffect &= ~(MOVE_EFFECT_CERTAIN);
        SetMoveEffect(FALSE, MOVE_EFFECT_CERTAIN);
    } else if (gTurnStructs[gBattlerAttacker].parentalBondTrigger == ABILITY_MINION_CONTROL &&
               gTurnStructs[gBattlerAttacker].parentalBondOn < gTurnStructs[gBattlerAttacker].parentalBondInitialCount) {
        // No-op
    } else if (Random() % 100 < percentChance && gBattleScripting.moveEffect && !(gMoveResultFlags & MOVE_RESULT_NO_EFFECT)) {
        if (percentChance >= 100)
            SetMoveEffect(FALSE, MOVE_EFFECT_CERTAIN);
        else
            SetMoveEffect(FALSE, 0);

        FlagSet(FLAG_LAST_MOVE_SECONDARY_EFFECT_ACTIVATED);
    }

    gBattleScripting.moveEffect = 0;
    gBattleScripting.multihitMoveEffect = 0;
    gBattleScripting.moveSecondaryEffectChance = 0;
}

static void Cmd_seteffectprimary(void) {
    gBattlescriptCurrInstr++;
    SetMoveEffect(TRUE, 0);
}

static void Cmd_seteffectsecondary(void) {
    gBattlescriptCurrInstr++;
    SetMoveEffect(FALSE, 0);
}

static void Cmd_clearstatusfromeffect(void) {
    gActiveBattler = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);

    if (gBattleScripting.moveEffect <= PRIMARY_STATUS_MOVE_EFFECT)
        gBattleMons[gActiveBattler].status1 &= (~sStatusFlagsForMoveEffects[gBattleScripting.moveEffect]);
    else
        gBattleMons[gActiveBattler].status2 &= (~sStatusFlagsForMoveEffects[gBattleScripting.moveEffect]);

    if (gBattleScripting.moveEffect == MOVE_EFFECT_CHARGING) {
        if (gBattleMoves[gCurrentMove].effect == EFFECT_SOLARBEAM &&
            (IsBattlerWeatherAffected(gActiveBattler, WEATHER_SUN_ANY) || HasChloroplast(gActiveBattler)))
            gRoundStructs[gActiveBattler].chargingTurn = FALSE;
        else if (BattlerHasAbility(gActiveBattler, ABILITY_ACCELERATE, FALSE))
            gRoundStructs[gActiveBattler].chargingTurn = FALSE;
    }

    gBattleScripting.moveEffect = 0;
    gBattlescriptCurrInstr += 2;
    gBattleScripting.multihitMoveEffect = 0;
    gBattleScripting.moveSecondaryEffectChance = 0;
}

static void Cmd_tryfaintmon(void) {
    const u8* BS_ptr;
    int recurringNightmare = FALSE;
    gActiveBattler = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);

    if (gActiveBattler == gBattlerAttacker && gProcessingExtraAttacks && gQueuedExtraAttackData[0].ability == ABILITY_VICTORY_BOMB) {
        gBattlescriptCurrInstr += 7;
        return;
    }

    if (BattlerHasAbility(gActiveBattler, ABILITY_RECURRING_NIGHTMARE, TRUE) && !IsBattlerAlive(gActiveBattler) &&
        !GetSingleUseAbilityCounter(gActiveBattler, ABILITY_RECURRING_NIGHTMARE) && IsBattlerWeatherAffected(gActiveBattler, WEATHER_FOG_ANY)) {
        SetSingleUseAbilityCounter(gActiveBattler, ABILITY_RECURRING_NIGHTMARE, 1);
        recurringNightmare = TRUE;
    }

    if (gBattlescriptCurrInstr[2] != 0) {
        if (gHitMarker & HITMARKER_FAINTED(gActiveBattler)) {
            BS_ptr = T1_READ_PTR(gBattlescriptCurrInstr + 3);

            BattleScriptPop();
            gBattlescriptCurrInstr = BS_ptr;
            gSideStatuses[GetBattlerSide(gActiveBattler)] &=
                ~(SIDE_STATUS_SPIKES_DAMAGED | SIDE_STATUS_TOXIC_SPIKES_DAMAGED | SIDE_STATUS_STEALTH_ROCK_DAMAGED | SIDE_STATUS_STICKY_WEB_DAMAGED);
        } else {
            gBattlescriptCurrInstr += 7;
        }
    } else {
        u8 battlerId;

        if (gActiveBattler == gBattlerAttacker) {
            battlerId = gBattlerTarget;
            BS_ptr = BattleScript_FaintAttacker;
        } else {
            battlerId = gBattlerAttacker;
            BS_ptr = BattleScript_FaintTarget;
        }

        SetActiveStackBattler(gActiveBattler, 1);

        if (recurringNightmare) {
            SetActiveMultistringChooser(B_MSG_FADE_OUT);
            SetActiveAbilityPopupOverride(ABILITY_RECURRING_NIGHTMARE);
        } else
            SetActiveMultistringChooser(B_MSG_MON_FAINTED);

        if (!(gAbsentBattlerFlags & gBitTable[gActiveBattler]) && gBattleMons[gActiveBattler].hp == 0) {
            gHitMarker |= HITMARKER_FAINTED(gActiveBattler);
            BattleScriptPush(gBattlescriptCurrInstr + 7);
            gBattlescriptCurrInstr = BS_ptr;
            gFaintedMonCount[GetBattlerSide(gActiveBattler)]++;
            if (GetBattlerSide(gActiveBattler) == B_SIDE_PLAYER) {
                gHitMarker |= HITMARKER_x400000;
                if (gBattleResults.playerFaintCounter < 0xFF) gBattleResults.playerFaintCounter++;
                AdjustFriendshipOnBattleFaint(gActiveBattler);
                gSideTimers[0].retaliateTimer = 2;
            } else {
                if (gBattleResults.opponentFaintCounter < 0xFF) gBattleResults.opponentFaintCounter++;
                gBattleResults.lastOpponentSpecies = GetMonData(&gEnemyParty[gBattlerPartyIndexes[gActiveBattler]], MON_DATA_SPECIES, NULL);
                gSideTimers[1].retaliateTimer = 2;
            }
            if ((gHitMarker & HITMARKER_DESTINYBOND) && gBattleMons[gBattlerAttacker].hp != 0) {
                gHitMarker &= ~(HITMARKER_DESTINYBOND);
                BattleScriptCall(BattleScript_DestinyBondTakesLife);
                gBattleMoveDamage = gBattleMons[battlerId].hp;
                // Destiny Bond disables recurring nightmare
                SetSingleUseAbilityCounter(gBattlerAttacker, ABILITY_RECURRING_NIGHTMARE, 2);
                SetSingleUseAbilityCounter(gBattlerTarget, ABILITY_RECURRING_NIGHTMARE, 2);
            }
            if ((gStatuses3[gBattlerTarget] & STATUS3_GRUDGE) && !(gHitMarker & HITMARKER_GRUDGE) &&
                GetBattlerSide(gBattlerAttacker) != GetBattlerSide(gBattlerTarget) && gBattleMons[gBattlerAttacker].hp != 0 && gCurrentMove != MOVE_STRUGGLE) {
                u8 moveIndex = gBattleStruct->chosenMovePositions[gBattlerAttacker];

                gBattleMons[gBattlerAttacker].pp[moveIndex] = 0;
                BattleScriptCall(BattleScript_GrudgeTakesPp);
                gActiveBattler = gBattlerAttacker;
                BtlController_EmitSetMonData(0, moveIndex + REQUEST_PPMOVE1_BATTLE, 0, 1, &gBattleMons[gActiveBattler].pp[moveIndex]);
                MarkBattlerForControllerExec(gActiveBattler);

                PREPARE_MOVE_BUFFER(gBattleTextBuff1, gBattleMons[gBattlerAttacker].moves[moveIndex])
            }
        } else {
            gBattlescriptCurrInstr += 7;
        }
    }
}

static void Cmd_dofaintanimation(void) {
    if (gBattleControllerExecFlags == 0) {
        gActiveBattler = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);
        BtlController_EmitFaintAnimation(0);
        MarkBattlerForControllerExec(gActiveBattler);
        gBattlescriptCurrInstr += 2;
    }
}

static void Cmd_cleareffectsonfaint(void) {
    if (gBattleControllerExecFlags == 0) {
        gActiveBattler = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);

        if (!(gBattleTypeFlags & BATTLE_TYPE_ARENA) || gBattleMons[gActiveBattler].hp == 0) {
            gBattleMons[gActiveBattler].status1 = 0;
            BtlController_EmitSetMonData(0, REQUEST_STATUS_BATTLE, 0, 0x4, &gBattleMons[gActiveBattler].status1);
            MarkBattlerForControllerExec(gActiveBattler);
        }
        gBattlescriptCurrInstr += 2;

        FaintClearSetData();  // Effects like attractions, trapping, etc.
    }
}

static void Cmd_jumpifstatus(void) {
    u8 battlerId = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);
    u32 flags = T2_READ_32(gBattlescriptCurrInstr + 2);
    const u8* jumpPtr = T2_READ_PTR(gBattlescriptCurrInstr + 6);

    if (gBattleMons[battlerId].status1 & flags && gBattleMons[battlerId].hp)
        gBattlescriptCurrInstr = jumpPtr;
    else
        gBattlescriptCurrInstr += 10;
}

static void Cmd_jumpifstatus2(void) {
    u8 battlerId = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);
    u32 flags = T2_READ_32(gBattlescriptCurrInstr + 2);
    const u8* jumpPtr = T2_READ_PTR(gBattlescriptCurrInstr + 6);

    if (gBattleMons[battlerId].status2 & flags && gBattleMons[battlerId].hp)
        gBattlescriptCurrInstr = jumpPtr;
    else
        gBattlescriptCurrInstr += 10;
}

static void Cmd_jumpifability(void) {
    u32 battlerId;
    bool32 hasAbility = FALSE;
    u32 ability = T2_READ_16(gBattlescriptCurrInstr + 2);

    switch (gBattlescriptCurrInstr[1]) {
        default:
            battlerId = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);
            if (BATTLER_HAS_ABILITY(battlerId, ability)) hasAbility = TRUE;
            break;
        case BS_ATTACKER_SIDE:
            battlerId = IsAbilityOnSide(gBattlerAttacker, ability);
            if (battlerId) {
                battlerId--;
                hasAbility = TRUE;
            }
            break;
        case BS_TARGET_SIDE:
            battlerId = IsAbilityOnOpposingSide(gBattlerAttacker, ability);
            if (battlerId) {
                battlerId--;
                hasAbility = TRUE;
            }
            break;
    }

    if (hasAbility) {
        gBattleScripting.abilityPopupOverwrite = ability;
        gBattlescriptCurrInstr = T2_READ_PTR(gBattlescriptCurrInstr + 4);
        gBattlerAbility = battlerId;
    } else {
        gBattlescriptCurrInstr += 8;
    }
}

static void Cmd_jumpifsideaffecting(void) {
    u8 side;
    u32 flags;
    const u8* jumpPtr;

    if (gBattlescriptCurrInstr[1] == BS_ATTACKER)
        side = GET_BATTLER_SIDE(gBattlerAttacker);
    else
        side = GET_BATTLER_SIDE(gBattlerTarget);

    flags = T2_READ_32(gBattlescriptCurrInstr + 2);
    jumpPtr = T2_READ_PTR(gBattlescriptCurrInstr + 6);

    if (gSideStatuses[side] & flags)
        gBattlescriptCurrInstr = jumpPtr;
    else
        gBattlescriptCurrInstr += 10;
}

static void Cmd_jumpifstat(void) {
    bool32 ret = 0;
    u8 battlerId = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);
    u8 statId = gBattlescriptCurrInstr[3];
    u8 cmpTo = gBattlescriptCurrInstr[4];
    u8 cmpKind = gBattlescriptCurrInstr[2];

    ret = CompareStat(battlerId, statId, cmpTo, cmpKind);

    if (ret)
        gBattlescriptCurrInstr = T2_READ_PTR(gBattlescriptCurrInstr + 5);
    else
        gBattlescriptCurrInstr += 9;
}

static void Cmd_jumpifstatus3condition(void) {
    u32 flags;
    const u8* jumpPtr;

    gActiveBattler = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);
    flags = T2_READ_32(gBattlescriptCurrInstr + 2);
    jumpPtr = T2_READ_PTR(gBattlescriptCurrInstr + 7);

    if (gBattlescriptCurrInstr[6]) {
        if ((gStatuses3[gActiveBattler] & flags) != 0)
            gBattlescriptCurrInstr += 11;
        else
            gBattlescriptCurrInstr = jumpPtr;
    } else {
        if ((gStatuses3[gActiveBattler] & flags) != 0)
            gBattlescriptCurrInstr = jumpPtr;
        else
            gBattlescriptCurrInstr += 11;
    }
}

static void Cmd_jumpbasedontype(void) {
    u8 battlerId = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);
    u8 type = gBattlescriptCurrInstr[2];
    const u8* jumpPtr = T2_READ_PTR(gBattlescriptCurrInstr + 4);

    if (type == TYPE_CURRENT_MOVE) type = gBattleMoves[gCurrentMove].type;

    // jumpiftype
    if (gBattlescriptCurrInstr[3]) {
        if (IS_BATTLER_OF_TYPE(battlerId, type))
            gBattlescriptCurrInstr = jumpPtr;
        else
            gBattlescriptCurrInstr += 8;
    }
    // jumpifnottype
    else {
        if (!IS_BATTLER_OF_TYPE(battlerId, type))
            gBattlescriptCurrInstr = jumpPtr;
        else
            gBattlescriptCurrInstr += 8;
    }
}

static void Cmd_getexp(void) {
    u16 item;
    s32 i;  // also used as stringId
    u8 holdEffect;
    s32 sentIn;
    u32* exp = &gBattleStruct->expValue;
    u8 highestLevel;

    gBattlerFainted = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);
    sentIn = gSentPokesToOpponent[(gBattlerFainted & 2) >> 1];

    switch (gBattleScripting.getexpState) {
        case 0:  // check if should receive exp at all
            if (GetBattlerSide(gBattlerFainted) != B_SIDE_OPPONENT ||
                (gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_RECORDED_LINK | BATTLE_TYPE_TRAINER_HILL | BATTLE_TYPE_FRONTIER | BATTLE_TYPE_SAFARI |
                                     BATTLE_TYPE_BATTLE_TOWER | BATTLE_TYPE_EREADER_TRAINER))) {
                gBattleScripting.getexpState = 6;  // goto last case
            } else {
                // Print Exp gain message once, only after KO, and only if something can gain Exp
                if (!PartyIsMaxLevel() && gSaveBlock2Ptr->automaticExpGain) PrepareStringBattle(STRINGID_PKMNGAINEDEXP, gBattleStruct->expGetterBattlerId);

                gBattleScripting.getexpState++;
                gBattleStruct->givenExpMons |= gBitTable[gBattlerPartyIndexes[gBattlerFainted]];
            }
            break;
        case 1:  // calculate experience points to redistribute
        {
            u32 calculatedExp;
            s32 viaSentIn;

            for (viaSentIn = 0, i = 0; i < PARTY_SIZE; i++) {
                if (GetMonData(&gPlayerParty[i], MON_DATA_SPECIES) == SPECIES_NONE || GetMonData(&gPlayerParty[i], MON_DATA_HP) == 0) continue;
                if (gBitTable[i] & sentIn) viaSentIn++;

                item = GetMonData(&gPlayerParty[i], MON_DATA_HELD_ITEM);

                if (item == ITEM_ENIGMA_BERRY) {
#ifndef FREE_ENIGMA_BERRY
                    holdEffect = gSaveBlock1Ptr->enigmaBerry.holdEffect;
#else
                    holdEffect = 0;
#endif
                } else
                    holdEffect = ItemId_GetHoldEffect(item);
            }
#if (B_SCALED_EXP >= GEN_5) && (B_SCALED_EXP != GEN_6)
            calculatedExp = gBaseStats[gBattleMons[gBattlerFainted].species].expYield * gBattleMons[gBattlerFainted].level / 5;
#else
            calculatedExp = gBaseStats[gBattleMons[gBattlerFainted].species].expYield * gBattleMons[gBattlerFainted].level / 7;
#endif

            // Exp share effect always on. Any Pokemon that was sent in gets 100% of the exp, the rest get 25%

            *exp = calculatedExp;  // * 3 / 4; // Portion of EXP given to Pokemon that appeared in battle
            if (*exp == 0) *exp = 1;

            gExpShareExp = calculatedExp / 4;  // Portion of EXP given to all Pokemon, whether they battled or not
            if (gExpShareExp == 0) gExpShareExp = 1;

            gBattleScripting.getexpState++;
            gBattleStruct->expGetterMonId = 0;
            gBattleStruct->sentInPokes = sentIn;
        }
            FALLTHROUGH
        case 2:  // set exp value to the poke in expgetter_id and print message
            if (gBattleControllerExecFlags == 0) {
                item = GetMonData(&gPlayerParty[gBattleStruct->expGetterMonId], MON_DATA_HELD_ITEM);

                if (item == ITEM_ENIGMA_BERRY) {
#ifndef FREE_ENIGMA_BERRY
                    holdEffect = gSaveBlock1Ptr->enigmaBerry.holdEffect;
#else
                    holdEffect = 0;
#endif
                } else
                    holdEffect = ItemId_GetHoldEffect(item);

                if (GetMonData(&gPlayerParty[gBattleStruct->expGetterMonId], MON_DATA_LEVEL) == MAX_LEVEL || !gSaveBlock2Ptr->automaticExpGain) {
                    *(&gBattleStruct->sentInPokes) >>= 1;
                    MonGainEVs(&gPlayerParty[gBattleStruct->expGetterMonId], gBattleMons[gBattlerFainted].species);
                    gBattleScripting.getexpState = 5;
                    gBattleMoveDamage = 0;  // used for exp
                } else if (GetMonData(&gPlayerParty[gBattleStruct->expGetterMonId], MON_DATA_LEVEL) >= GetLevelCap()) {
                    gBattleMoveDamage = 1;  // If mon is above level cap, it gets 1 exp, but still gains EVs
                    MonGainEVs(&gPlayerParty[gBattleStruct->expGetterMonId], gBattleMons[gBattlerFainted].species);
                    // EVs won't be applied until next level up. TODO: Update this mechanic to match newer games
                    gBattleStruct->sentInPokes >>= 1;
                    gBattleScripting.getexpState++;
                } else {
                    // Music change in a wild battle after fainting opposing pokemon.
                    if (!(gBattleTypeFlags & BATTLE_TYPE_TRAINER) && (gBattleMons[0].hp || (gBattleTypeFlags & BATTLE_TYPE_DOUBLE && gBattleMons[2].hp)) &&
                        !IsBattlerAlive(GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT)) && !IsBattlerAlive(GetBattlerAtPosition(B_POSITION_OPPONENT_RIGHT)) &&
                        !gBattleStruct->wildVictorySong) {
                        BattleStopLowHpSound();
                        PlayBGM(MUS_VICTORY_WILD);
                        gBattleStruct->wildVictorySong++;
                    }

                    if (GetMonData(&gPlayerParty[gBattleStruct->expGetterMonId], MON_DATA_HP) &&
                        !GetMonData(&gPlayerParty[gBattleStruct->expGetterMonId], MON_DATA_IS_EGG)) {
                        if (gBattleStruct->sentInPokes & 1)
                            gBattleMoveDamage = *exp;
                        else {
                            gBattleMoveDamage = gExpShareExp;
                        }
                        if (holdEffect == HOLD_EFFECT_EXP_SHARE && !(gBattleStruct->sentInPokes & 1))
                            gBattleMoveDamage = gExpShareExp * 4;  // Determines how much EXP a Pokemon holding an EXP Share receives
                        if (holdEffect == HOLD_EFFECT_LUCKY_EGG) gBattleMoveDamage = (gBattleMoveDamage * 150) / 100;
                        if (holdEffect == HOLD_EFFECT_TRAINING_BAND) {
                            highestLevel = GetHighestLevelInPlayerParty();
                            if (GetMonData(&gPlayerParty[gBattleStruct->expGetterMonId], MON_DATA_LEVEL) < (highestLevel - 4)) {
                                gBattleMoveDamage = gBattleMoveDamage * 5;
                            }
                        }
#if (B_SCALED_EXP >= GEN_5) && (B_SCALED_EXP != GEN_6)
                        {
                            // Note: There is an edge case where if a pokemon receives a large amount of exp, it wouldn't be properly calculated
                            //       because of multiplying by scaling factor(the value would simply be larger than an u32 can hold). Hence u64 is needed.
                            u64 value = gBattleMoveDamage;
                            value *= sExperienceScalingFactors[(gBattleMons[gBattlerFainted].level * 2) + 10];
                            value /= sExperienceScalingFactors[gBattleMons[gBattlerFainted].level +
                                                               GetMonData(&gPlayerParty[gBattleStruct->expGetterMonId], MON_DATA_LEVEL) + 10];
                            gBattleMoveDamage = value + 1;
                        }
#endif

                        /*                     if (IsTradedMon(&gPlayerParty[gBattleStruct->expGetterMonId]))
                                            {
                                                // check if the pokemon doesn't belong to the player
                                                if (gBattleTypeFlags & BATTLE_TYPE_INGAME_PARTNER && gBattleStruct->expGetterMonId >= 3)
                                                {
                                                    i = STRINGID_EMPTYSTRING4;
                                                }
                                                else
                                                {
                                                    gBattleMoveDamage = (gBattleMoveDamage * 150) / 100;
                                                    i = STRINGID_ABOOSTED;
                                                }
                                            }
                                            else
                                            {
                                                i = STRINGID_EMPTYSTRING4;
                                            } */

                        // get exp getter battlerId
                        if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE) {
                            if (gBattlerPartyIndexes[2] == gBattleStruct->expGetterMonId && !(gAbsentBattlerFlags & gBitTable[2]))
                                gBattleStruct->expGetterBattlerId = 2;
                            else {
                                if (!(gAbsentBattlerFlags & gBitTable[0]))
                                    gBattleStruct->expGetterBattlerId = 0;
                                else
                                    gBattleStruct->expGetterBattlerId = 2;
                            }
                        } else {
                            gBattleStruct->expGetterBattlerId = 0;
                        }
                        // Uncomment this and change the exp gained string to debug exp gains (src\battle_script_commands.c)

                        /*                     PREPARE_MON_NICK_WITH_PREFIX_BUFFER(gBattleTextBuff1, gBattleStruct->expGetterBattlerId,
                           gBattleStruct->expGetterMonId);
                                            // buffer 'gained' or 'gained a boosted'
                                            PREPARE_STRING_BUFFER(gBattleTextBuff2, i);
                                            PREPARE_WORD_NUMBER_BUFFER(gBattleTextBuff4, 6, gBattleMoveDamage);

                                            PrepareStringBattle(STRINGID_PKMNGAINEDEXP, gBattleStruct->expGetterBattlerId);  */
                        MonGainEVs(&gPlayerParty[gBattleStruct->expGetterMonId], gBattleMons[gBattlerFainted].species);
                    }
                    gBattleStruct->sentInPokes >>= 1;
                    gBattleScripting.getexpState++;
                }
            }
            break;
        case 3:  // Set stats and give exp
            if (gBattleControllerExecFlags == 0) {
                gBattleResources->bufferB[gBattleStruct->expGetterBattlerId][0] = 0;
                if (GetMonData(&gPlayerParty[gBattleStruct->expGetterMonId], MON_DATA_HP) &&
                    GetMonData(&gPlayerParty[gBattleStruct->expGetterMonId], MON_DATA_LEVEL) != MAX_LEVEL &&
                    !GetMonData(&gPlayerParty[gBattleStruct->expGetterMonId], MON_DATA_IS_EGG)) {
                    gBattleResources->beforeLvlUp->stats[STAT_HP] = GetMonData(&gPlayerParty[gBattleStruct->expGetterMonId], MON_DATA_MAX_HP);
                    gBattleResources->beforeLvlUp->stats[STAT_ATK] = GetMonData(&gPlayerParty[gBattleStruct->expGetterMonId], MON_DATA_ATK);
                    gBattleResources->beforeLvlUp->stats[STAT_DEF] = GetMonData(&gPlayerParty[gBattleStruct->expGetterMonId], MON_DATA_DEF);
                    gBattleResources->beforeLvlUp->stats[STAT_SPEED] = GetMonData(&gPlayerParty[gBattleStruct->expGetterMonId], MON_DATA_SPEED);
                    gBattleResources->beforeLvlUp->stats[STAT_SPATK] = GetMonData(&gPlayerParty[gBattleStruct->expGetterMonId], MON_DATA_SPATK);
                    gBattleResources->beforeLvlUp->stats[STAT_SPDEF] = GetMonData(&gPlayerParty[gBattleStruct->expGetterMonId], MON_DATA_SPDEF);

                    gActiveBattler = gBattleStruct->expGetterBattlerId;
                    BtlController_EmitExpUpdate(0, gBattleStruct->expGetterMonId, gBattleMoveDamage);
                    MarkBattlerForControllerExec(gActiveBattler);
                }
                gBattleScripting.getexpState++;
            }
            break;
        case 4:  // lvl up if necessary
            if (gBattleControllerExecFlags == 0) {
                gActiveBattler = gBattleStruct->expGetterBattlerId;
                if (gBattleResources->bufferB[gActiveBattler][0] == CONTROLLER_TWORETURNVALUES &&
                    gBattleResources->bufferB[gActiveBattler][1] == RET_VALUE_LEVELED_UP) {
                    u16 temp, battlerId = 0xFF;
                    if (gBattleTypeFlags & BATTLE_TYPE_TRAINER && gBattlerPartyIndexes[gActiveBattler] == gBattleStruct->expGetterMonId)
                        HandleLowHpMusicChange(&gPlayerParty[gBattlerPartyIndexes[gActiveBattler]], gActiveBattler);

                    gStackBattler1 = gActiveBattler;
                    PREPARE_BYTE_NUMBER_BUFFER(gBattleTextBuff2, 3, GetMonData(&gPlayerParty[gBattleStruct->expGetterMonId], MON_DATA_LEVEL));

                    BattleScriptCall(BattleScript_LevelUp);
                    gLeveledUpInBattle |= gBitTable[gBattleStruct->expGetterMonId];
                    gBattleMoveDamage = T1_READ_32(&gBattleResources->bufferB[gActiveBattler][2]);
                    AdjustFriendship(&gPlayerParty[gBattleStruct->expGetterMonId], FRIENDSHIP_EVENT_GROW_LEVEL);

                    // update battle mon structure after level up
                    if (gBattlerPartyIndexes[0] == gBattleStruct->expGetterMonId && gBattleMons[0].hp)
                        battlerId = 0;
                    else if (gBattlerPartyIndexes[2] == gBattleStruct->expGetterMonId && gBattleMons[2].hp && (gBattleTypeFlags & BATTLE_TYPE_DOUBLE))
                        battlerId = 2;

                    if (battlerId != 0xFF) {
                        gBattleMons[battlerId].level = GetMonData(&gPlayerParty[gBattleStruct->expGetterMonId], MON_DATA_LEVEL);
                        gBattleMons[battlerId].hp = GetMonData(&gPlayerParty[gBattleStruct->expGetterMonId], MON_DATA_HP);
                        gBattleMons[battlerId].maxHP = GetMonData(&gPlayerParty[gBattleStruct->expGetterMonId], MON_DATA_MAX_HP);
                        gBattleMons[battlerId].attack = GetMonData(&gPlayerParty[gBattleStruct->expGetterMonId], MON_DATA_ATK);
                        gBattleMons[battlerId].defense = GetMonData(&gPlayerParty[gBattleStruct->expGetterMonId], MON_DATA_DEF);
                        gBattleMons[battlerId].speed = GetMonData(&gPlayerParty[gBattleStruct->expGetterMonId], MON_DATA_SPEED);
                        gBattleMons[battlerId].spAttack = GetMonData(&gPlayerParty[gBattleStruct->expGetterMonId], MON_DATA_SPATK);
                        gBattleMons[battlerId].spDefense = GetMonData(&gPlayerParty[gBattleStruct->expGetterMonId], MON_DATA_SPDEF);

                        if (gStatuses3[battlerId] & STATUS3_POWER_TRICK) SWAP(gBattleMons[battlerId].attack, gBattleMons[battlerId].defense, temp);
                    }

                    gBattleScripting.getexpState = 5;
                } else {
                    gBattleMoveDamage = 0;
                    gBattleScripting.getexpState = 5;
                }
            }
            break;
        case 5:                     // looper increment
            if (gBattleMoveDamage)  // there is exp to give, goto case 3 that gives exp
            {
                gBattleScripting.getexpState = 3;
            } else {
                gBattleStruct->expGetterMonId++;
                if (gBattleStruct->expGetterMonId < PARTY_SIZE)
                    gBattleScripting.getexpState = 2;  // loop again
                else
                    gBattleScripting.getexpState = 6;  // we're done
            }
            break;
        case 6:  // increment instruction
            if (gBattleControllerExecFlags == 0) {
                // not sure why gf clears the item and ability here
                gBattlescriptCurrInstr += 2;
            }
            break;
    }
}

static bool32 NoAliveMonsForPlayer(void) {
    u32 i;
    u32 HP_count = 0;

    if (gBattleTypeFlags & BATTLE_TYPE_INGAME_PARTNER && (gPartnerTrainerId == TRAINER_STEVEN_PARTNER || gPartnerTrainerId >= TRAINER_CUSTOM_PARTNER)) {
        for (i = 0; i < MULTI_PARTY_SIZE; i++) {
            if (GetMonData(&gPlayerParty[i], MON_DATA_SPECIES) && !GetMonData(&gPlayerParty[i], MON_DATA_IS_EGG))
                HP_count += GetMonData(&gPlayerParty[i], MON_DATA_HP);
        }
    } else {
        for (i = 0; i < PARTY_SIZE; i++) {
            if (GetMonData(&gPlayerParty[i], MON_DATA_SPECIES) && !GetMonData(&gPlayerParty[i], MON_DATA_IS_EGG) &&
                (!(gBattleTypeFlags & BATTLE_TYPE_ARENA) || !(gBattleStruct->arenaLostPlayerMons & gBitTable[i]))) {
                HP_count += GetMonData(&gPlayerParty[i], MON_DATA_HP);
            }
        }
    }

    return (HP_count == 0);
}

static bool32 NoAliveMonsForOpponent(void) {
    u32 i;
    u32 HP_count = 0;

    for (i = 0; i < PARTY_SIZE; i++) {
        if (GetMonData(&gEnemyParty[i], MON_DATA_SPECIES) && !GetMonData(&gEnemyParty[i], MON_DATA_IS_EGG) &&
            (!(gBattleTypeFlags & BATTLE_TYPE_ARENA) || !(gBattleStruct->arenaLostOpponentMons & gBitTable[i]))) {
            HP_count += GetMonData(&gEnemyParty[i], MON_DATA_HP);
        }
    }

    return (HP_count == 0);
}

bool32 NoAliveMonsForEitherParty(void) { return (NoAliveMonsForPlayer() || NoAliveMonsForOpponent()); }

// For battles that aren't BATTLE_TYPE_LINK or BATTLE_TYPE_RECORDED_LINK, the only thing this
// command does is check whether the player has won/lost by totaling each team's HP. It then
// sets gBattleOutcome accordingly, if necessary.
static void Cmd_checkteamslost(void) {
    if (gBattleControllerExecFlags) return;

    if (NoAliveMonsForPlayer()) gBattleOutcome |= B_OUTCOME_LOST;
    if (NoAliveMonsForOpponent()) gBattleOutcome |= B_OUTCOME_WON;

    if (gBattleOutcome == 0 && (gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_RECORDED_LINK))) {
        s32 i, foundPlayer, foundOpponent;

        for (foundPlayer = 0, i = 0; i < gBattlersCount; i += 2) {
            if ((gHitMarker & HITMARKER_FAINTED2(i)) && (!gTurnStructs[i].flag40)) foundPlayer++;
        }

        foundOpponent = 0;

        for (i = 1; i < gBattlersCount; i += 2) {
            if ((gHitMarker & HITMARKER_FAINTED2(i)) && (!gTurnStructs[i].flag40)) foundOpponent++;
        }

        if (gBattleTypeFlags & BATTLE_TYPE_MULTI) {
            if (foundOpponent + foundPlayer > 1)
                gBattlescriptCurrInstr = T2_READ_PTR(gBattlescriptCurrInstr + 1);
            else
                gBattlescriptCurrInstr += 5;
        } else {
            if (foundOpponent != 0 && foundPlayer != 0)
                gBattlescriptCurrInstr = T2_READ_PTR(gBattlescriptCurrInstr + 1);
            else
                gBattlescriptCurrInstr += 5;
        }
    } else {
        gBattlescriptCurrInstr += 5;
    }
}

static void MoveValuesCleanUp(void) {
    gMoveResultFlags = 0;
    gIsCriticalHit = FALSE;
    gBattleScripting.moveEffect = 0;
    gBattleScripting.moveSecondaryEffectChance = 0;
    gBattleCommunication[MISS_TYPE] = 0;
    gHitMarker &= ~(HITMARKER_DESTINYBOND);
    gHitMarker &= ~(HITMARKER_SYNCHRONISE_EFFECT);
}

static void Cmd_movevaluescleanup(void) {
    MoveValuesCleanUp();
    gBattlescriptCurrInstr += 1;
}

static void Cmd_setmultihit(void) {
    gTurnStructs[gBattlerAttacker].multiHitCounter = gBattlescriptCurrInstr[1];
    gBattlescriptCurrInstr += 2;
}

static void Cmd_decrementmultihit(void) {
    if (--gTurnStructs[gBattlerAttacker].multiHitCounter == 0)
        gBattlescriptCurrInstr += 5;
    else
        gBattlescriptCurrInstr = T2_READ_PTR(gBattlescriptCurrInstr + 1);
}

static void Cmd_goto(void) { gBattlescriptCurrInstr = T2_READ_PTR(gBattlescriptCurrInstr + 1); }

static void Cmd_jumpifbyte(void) {
    u8 caseID = READ_FIRST_8_INC;
    const u8* memByte = READ_PTR_INC;
    u8 value = READ_8_INC;
    const u8* jumpPtr = READ_PTR_INC;

    switch (caseID) {
        case CMP_EQUAL:
            if (*memByte == value) gBattlescriptCurrInstr = jumpPtr;
            break;
        case CMP_NOT_EQUAL:
            if (*memByte != value) gBattlescriptCurrInstr = jumpPtr;
            break;
        case CMP_GREATER_THAN:
            if (*memByte > value) gBattlescriptCurrInstr = jumpPtr;
            break;
        case CMP_LESS_THAN:
            if (*memByte < value) gBattlescriptCurrInstr = jumpPtr;
            break;
        case CMP_COMMON_BITS:
            if (*memByte & value) gBattlescriptCurrInstr = jumpPtr;
            break;
        case CMP_NO_COMMON_BITS:
            if (!(*memByte & value)) gBattlescriptCurrInstr = jumpPtr;
            break;
    }
}

static void Cmd_jumpifhalfword(void) {
    u8 caseID = READ_FIRST_8_INC;
    const u16* memHword = READ_PTR_INC;
    u16 value = READ_16_INC;
    const u8* jumpPtr = READ_PTR_INC;

    switch (caseID) {
        case CMP_EQUAL:
            if (*memHword == value) gBattlescriptCurrInstr = jumpPtr;
            break;
        case CMP_NOT_EQUAL:
            if (*memHword != value) gBattlescriptCurrInstr = jumpPtr;
            break;
        case CMP_GREATER_THAN:
            if (*memHword > value) gBattlescriptCurrInstr = jumpPtr;
            break;
        case CMP_LESS_THAN:
            if (*memHword < value) gBattlescriptCurrInstr = jumpPtr;
            break;
        case CMP_COMMON_BITS:
            if (*memHword & value) gBattlescriptCurrInstr = jumpPtr;
            break;
        case CMP_NO_COMMON_BITS:
            if (!(*memHword & value)) gBattlescriptCurrInstr = jumpPtr;
            break;
    }
}

static void Cmd_jumpifword(void) {
    u8 caseID = READ_FIRST_8_INC;
    const u32* memWord = READ_PTR_INC;
    u32 value = READ_32_INC;
    const u8* jumpPtr = READ_PTR_INC;

    switch (caseID) {
        case CMP_EQUAL:
            if (*memWord == value) gBattlescriptCurrInstr = jumpPtr;
            break;
        case CMP_NOT_EQUAL:
            if (*memWord != value) gBattlescriptCurrInstr = jumpPtr;
            break;
        case CMP_GREATER_THAN:
            if (*memWord > value) gBattlescriptCurrInstr = jumpPtr;
            break;
        case CMP_LESS_THAN:
            if (*memWord < value) gBattlescriptCurrInstr = jumpPtr;
            break;
        case CMP_COMMON_BITS:
            if (*memWord & value) gBattlescriptCurrInstr = jumpPtr;
            break;
        case CMP_NO_COMMON_BITS:
            if (!(*memWord & value)) gBattlescriptCurrInstr = jumpPtr;
            break;
    }
}

static void Cmd_jumpifarrayequal(void) {
    const u8* mem1 = READ_FIRST_PTR_INC;
    const u8* mem2 = READ_PTR_INC;
    u32 size = READ_8_INC;
    const u8* jumpPtr = READ_PTR_INC;

    u8 i;
    for (i = 0; i < size; i++) {
        if (mem1[i] != mem2[i]) return;
    }

    gBattlescriptCurrInstr = jumpPtr;
}

static void Cmd_jumpifarraynotequal(void) {
    const u8* mem1 = READ_FIRST_PTR_INC;
    const u8* mem2 = READ_PTR_INC;
    u32 size = READ_8_INC;
    const u8* jumpPtr = READ_PTR_INC;

    u8 i;
    for (i = 0; i < size; i++) {
        if (mem1[i] != mem2[i]) {
            gBattlescriptCurrInstr = jumpPtr;
            return;
        }
    }
}

static void Cmd_setbyte(void) {
    u8* memByte = READ_FIRST_PTR_INC;
    *memByte = READ_8_INC;
}

static void Cmd_addbyte(void) {
    u8* memByte = READ_FIRST_PTR_INC;
    *memByte += READ_8_INC;
}

static void Cmd_subbyte(void) {
    u8* memByte = READ_FIRST_PTR_INC;
    *memByte -= READ_8_INC;
}

static void Cmd_copyarray(void) {
    u8* dest = READ_FIRST_PTR_INC;
    const u8* src = READ_PTR_INC;
    s32 size = READ_8_INC;

    memcpy(dest, src, size);
}

static void Cmd_copyarraywithindex(void) {
    u8* dest = READ_FIRST_PTR_INC;
    const u8* src = READ_PTR_INC;
    const u8* index = READ_PTR_INC;
    s32 size = READ_8_INC;

    memcpy(dest, &src[*index], size);
}

static void Cmd_orbyte(void) {
    u8* memByte = READ_FIRST_PTR_INC;
    *memByte |= READ_8_INC;
}

static void Cmd_orhalfword(void) {
    u16* memHword = READ_FIRST_PTR_INC;
    u16 val = READ_16_INC;

    *memHword |= val;
}

static void Cmd_orword(void) {
    u32* memWord = READ_FIRST_PTR_INC;
    u32 val = READ_32_INC;

    *memWord |= val;
}

static void Cmd_bicbyte(void) {
    u8* memByte = READ_FIRST_PTR_INC;
    *memByte &= ~(READ_8_INC);
}

static void Cmd_bichalfword(void) {
    u16* memHword = READ_FIRST_PTR_INC;
    u16 val = READ_16_INC;

    *memHword &= ~val;
}

static void Cmd_bicword(void) {
    u32* memWord = READ_FIRST_PTR_INC;
    u32 val = READ_32_INC;

    *memWord &= ~val;
}

static void Cmd_pause(void) {
    if (gBattleControllerExecFlags == 0) {
        u16 value = T2_READ_16(gBattlescriptCurrInstr + 1);
        if (++gPauseCounterBattle >= value) {
            gPauseCounterBattle = 0;
            gBattlescriptCurrInstr += 3;
        }
    }
}

static void Cmd_waitstate(void) {
    if (gBattleControllerExecFlags == 0) gBattlescriptCurrInstr++;
}

static void Cmd_healthbar_update(void) {
    if (gBattlescriptCurrInstr[1] == BS_TARGET)
        gActiveBattler = gBattlerTarget;
    else
        gActiveBattler = gBattlerAttacker;

    BtlController_EmitHealthBarUpdate(0, gBattleMoveDamage);
    MarkBattlerForControllerExec(gActiveBattler);
    gBattlescriptCurrInstr += 2;
}

static void Cmd_return(void) { BattleScriptPop(); }

static void Cmd_end(void) {
    if (gBattleTypeFlags & BATTLE_TYPE_ARENA) BattleArena_AddSkillPoints(gBattlerAttacker);

    gMoveResultFlags = 0;
    gActiveBattler = 0;
    gCurrentActionFuncId = B_ACTION_TRY_FINISH;
}

static void Cmd_end2(void) {
    gActiveBattler = 0;
    gCurrentActionFuncId = B_ACTION_TRY_FINISH;
}

static void Cmd_end3(void)  // pops the main function stack
{
    BattleScriptPop();
    if (gBattleResources->battleCallbackStack->size != 0) gBattleResources->battleCallbackStack->size--;
    gBattleMainFunc = gBattleResources->battleCallbackStack->function[gBattleResources->battleCallbackStack->size];
}

static void Cmd_call(void) { BattleScriptCall(READ_FIRST_PTR_INC); }

static void Cmd_setroost(void) {
    gBattleResources->flags->flags[gBattlerAttacker] |= RESOURCE_FLAG_ROOST;

    // Pure flying type.
    if (gBattleMons[gBattlerAttacker].type1 == TYPE_FLYING && gBattleMons[gBattlerAttacker].type2 == TYPE_FLYING) {
        gBattleStruct->roostTypes[gBattlerAttacker][0] = TYPE_FLYING;
        gBattleStruct->roostTypes[gBattlerAttacker][1] = TYPE_FLYING;
        gBattleStruct->roostTypes[gBattlerAttacker][2] = TYPE_FLYING;
        SET_BATTLER_TYPE(gBattlerAttacker, TYPE_NORMAL);
    }
    // Dual Type with Flying Type.
    else if ((gBattleMons[gBattlerAttacker].type1 == TYPE_FLYING && gBattleMons[gBattlerAttacker].type2 != TYPE_FLYING) ||
             (gBattleMons[gBattlerAttacker].type2 == TYPE_FLYING && gBattleMons[gBattlerAttacker].type1 != TYPE_FLYING)) {
        gBattleStruct->roostTypes[gBattlerAttacker][0] = gBattleMons[gBattlerAttacker].type1;
        gBattleStruct->roostTypes[gBattlerAttacker][1] = gBattleMons[gBattlerAttacker].type2;
        if (gBattleMons[gBattlerAttacker].type1 == TYPE_FLYING) gBattleMons[gBattlerAttacker].type1 = TYPE_MYSTERY;
        if (gBattleMons[gBattlerAttacker].type2 == TYPE_FLYING) gBattleMons[gBattlerAttacker].type2 = TYPE_MYSTERY;
    }
    // Non-flying type.
    else if (!IS_BATTLER_OF_TYPE(gBattlerAttacker, TYPE_FLYING)) {
        gBattleStruct->roostTypes[gBattlerAttacker][0] = gBattleMons[gBattlerAttacker].type1;
        gBattleStruct->roostTypes[gBattlerAttacker][1] = gBattleMons[gBattlerAttacker].type2;
    }

    gBattlescriptCurrInstr++;
}

static void Cmd_jumpifabilitypresent(void) {
    if (IsAbilityOnField(T1_READ_16(gBattlescriptCurrInstr + 1))) {
        if (!gBattlescriptCurrInstr[7]) {
            gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 3);
        } else {
            BattleScriptPush(gBattlescriptCurrInstr + 8);
            gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 3);
        }
    } else
        gBattlescriptCurrInstr += 8;
}

static void Cmd_endselectionscript(void) {
    *(gBattlerAttacker + gBattleStruct->selectionScriptFinished) = TRUE;
    //
}

static void Cmd_playanimation(void) {
    const u16* argumentPtr;
    u8 animId = gBattlescriptCurrInstr[2];

    gActiveBattler = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);
    argumentPtr = T2_READ_PTR(gBattlescriptCurrInstr + 3);

#if B_TERRAIN_BG_CHANGE == FALSE
    if (animId == B_ANIM_RESTORE_BG) {
        // workaround for .if not working
        gBattlescriptCurrInstr += 7;
        return;
    }
#endif

    if (animId == B_ANIM_STATS_CHANGE || animId == B_ANIM_SNATCH_MOVE || animId == B_ANIM_MEGA_EVOLUTION || animId == B_ANIM_ILLUSION_OFF ||
        animId == B_ANIM_FORM_CHANGE || animId == B_ANIM_SUBSTITUTE_FADE || animId == B_ANIM_PRIMAL_REVERSION) {
        BtlController_EmitBattleAnimation(0, animId, *argumentPtr);
        MarkBattlerForControllerExec(gActiveBattler);
        gBattlescriptCurrInstr += 7;
    } else if (gHitMarker & HITMARKER_NO_ANIMATIONS && animId != B_ANIM_RESTORE_BG) {
        BattleScriptPush(gBattlescriptCurrInstr + 7);
        gBattlescriptCurrInstr = BattleScript_Pausex20;
    } else if (animId == B_ANIM_RAIN_CONTINUES || animId == B_ANIM_SUN_CONTINUES || animId == B_ANIM_SANDSTORM_CONTINUES || animId == B_ANIM_HAIL_CONTINUES ||
               animId == B_ANIM_FOG_CONTINUES) {
        BtlController_EmitBattleAnimation(0, animId, *argumentPtr);
        MarkBattlerForControllerExec(gActiveBattler);
        gBattlescriptCurrInstr += 7;
    } else if (gStatuses3[gActiveBattler] & STATUS3_SEMI_INVULNERABLE) {
        gBattlescriptCurrInstr += 7;
    } else if (animId == B_ANIM_RESTORE_BG) {
        DrawTerrainTypeBattleBackground();
        gBattlescriptCurrInstr += 7;
    } else {
        BtlController_EmitBattleAnimation(0, animId, *argumentPtr);
        MarkBattlerForControllerExec(gActiveBattler);
        gBattlescriptCurrInstr += 7;
    }
}

static void Cmd_playanimation2(void)  // animation Id is stored in the first pointer
{
    const u16* argumentPtr;
    const u8* animationIdPtr;

    gActiveBattler = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);
    animationIdPtr = T2_READ_PTR(gBattlescriptCurrInstr + 2);
    argumentPtr = T2_READ_PTR(gBattlescriptCurrInstr + 6);

    if (*animationIdPtr == B_ANIM_STATS_CHANGE || *animationIdPtr == B_ANIM_SNATCH_MOVE || *animationIdPtr == B_ANIM_MEGA_EVOLUTION ||
        *animationIdPtr == B_ANIM_ILLUSION_OFF || *animationIdPtr == B_ANIM_FORM_CHANGE || *animationIdPtr == B_ANIM_SUBSTITUTE_FADE ||
        *animationIdPtr == B_ANIM_PRIMAL_REVERSION) {
        BtlController_EmitBattleAnimation(0, *animationIdPtr, *argumentPtr);
        MarkBattlerForControllerExec(gActiveBattler);
        gBattlescriptCurrInstr += 10;
    } else if (gHitMarker & HITMARKER_NO_ANIMATIONS) {
        gBattlescriptCurrInstr += 10;
    } else if (*animationIdPtr == B_ANIM_RAIN_CONTINUES || *animationIdPtr == B_ANIM_SUN_CONTINUES || *animationIdPtr == B_ANIM_SANDSTORM_CONTINUES ||
               *animationIdPtr == B_ANIM_HAIL_CONTINUES) {
        BtlController_EmitBattleAnimation(0, *animationIdPtr, *argumentPtr);
        MarkBattlerForControllerExec(gActiveBattler);
        gBattlescriptCurrInstr += 10;
    } else if (gStatuses3[gActiveBattler] & STATUS3_SEMI_INVULNERABLE) {
        gBattlescriptCurrInstr += 10;
    } else {
        BtlController_EmitBattleAnimation(0, *animationIdPtr, *argumentPtr);
        MarkBattlerForControllerExec(gActiveBattler);
        gBattlescriptCurrInstr += 10;
    }
}

static void Cmd_setgraphicalstatchangevalues(void) {
    u8 value;

    switch (GET_STAT_BUFF_VALUE_WITH_SIGN(gBattleScripting.statChanger)) {
        case 1:
            value = STAT_ANIM_PLUS1;
            break;
        case 2:
            value = STAT_ANIM_PLUS2;
            break;
        case 3:
            value = STAT_ANIM_PLUS2;
            break;
        case -1:
            value = STAT_ANIM_MINUS1;
            break;
        case -2:
            value = STAT_ANIM_MINUS2;
            break;
        case -3:
            value = STAT_ANIM_MINUS2;
            break;
        default:  // <-12,-4> and <4, 12>
            if (gBattleScripting.statChanger.goesDown)
                value = STAT_ANIM_MINUS2;
            else
                value = STAT_ANIM_PLUS2;
            break;
    }
    gBattleScripting.animArg1 = gBattleScripting.statChanger.statId + value - 1;
    gBattleScripting.animArg2 = 0;
    gBattlescriptCurrInstr++;
}

static void PlayStatChangeAnimation(int battler, int statsToCheck, int flags, int rawStatChange) {
    u32 currStat = 0;
    u32 statAnimId = 0;
    u32 changeableStatsCount = 0;
    u32 startingStatAnimId = 0;

    gActiveBattler = battler;

    if (!rawStatChange) {
        // Handle Contrary and Simple
        if (BATTLER_HAS_ABILITY(gActiveBattler, ABILITY_CONTRARY)) {
            flags ^= STAT_CHANGE_NEGATIVE;
        }

        if (BATTLER_HAS_ABILITY(gActiveBattler, ABILITY_SIMPLE)) flags |= STAT_CHANGE_BY_TWO;

        if (gBattlerAttacker != gActiveBattler && BATTLER_HAS_ABILITY(gBattlerAttacker, ABILITY_SUBDUE)) flags |= STAT_CHANGE_BY_TWO;
    }

    if (flags & STAT_CHANGE_NEGATIVE)  // goes down
    {
        if (flags & STAT_CHANGE_BY_TWO)
            startingStatAnimId = STAT_ANIM_MINUS2 - 1;
        else
            startingStatAnimId = STAT_ANIM_MINUS1 - 1;

        while (statsToCheck != 0) {
            if (statsToCheck & 1) {
                if (rawStatChange || (flags & STAT_CHANGE_CANT_PREVENT && !BATTLER_HAS_ABILITY(gActiveBattler, ABILITY_CLEAR_BODY) &&
                                      !BATTLER_HAS_ABILITY(gActiveBattler, ABILITY_FULL_METAL_BODY) &&
                                      !(BATTLER_HAS_ABILITY(gActiveBattler, ABILITY_LUCKY_HALO) && flags & MOVE_EFFECT_AFFECTS_USER))) {
                    if (gBattleMons[gActiveBattler].statStages[currStat] > MIN_STAT_STAGE) {
                        statAnimId = startingStatAnimId + currStat;
                        changeableStatsCount++;
                    }
                } else if (!BATTLER_HAS_ABILITY(gActiveBattler, ABILITY_CLEAR_BODY) && !BATTLER_HAS_ABILITY(gActiveBattler, ABILITY_FULL_METAL_BODY) &&
                           !(!(flags & MOVE_EFFECT_AFFECTS_USER) &&
                             (gSideTimers[GET_BATTLER_SIDE(gActiveBattler)].mistTimer ||
                              (BATTLER_HAS_ABILITY(gActiveBattler, ABILITY_KEEN_EYE) && currStat == STAT_ACC) ||
                              (BATTLER_HAS_ABILITY(gActiveBattler, ABILITY_MINDS_EYE) && currStat == STAT_ACC) ||
                              (BATTLER_HAS_ABILITY(gActiveBattler, ABILITY_HYPER_CUTTER) && (currStat == STAT_ATK || currStat == STAT_SPATK)) ||
                              BATTLER_HAS_ABILITY(gActiveBattler, ABILITY_LUCKY_HALO) ||
                              GetBattlerHoldEffect(gActiveBattler, TRUE) == HOLD_EFFECT_CLEAR_AMULET))) {
                    if (gBattleMons[gActiveBattler].statStages[currStat] > MIN_STAT_STAGE) {
                        statAnimId = startingStatAnimId + currStat;
                        changeableStatsCount++;
                    }
                }
            }
            statsToCheck >>= 1, currStat++;
        }

        if (changeableStatsCount > 1)  // more than one stat, so the color is gray
        {
            if (flags & STAT_CHANGE_BY_TWO)
                statAnimId = STAT_ANIM_MULTIPLE_MINUS2;
            else
                statAnimId = STAT_ANIM_MULTIPLE_MINUS1;
        }
    } else  // goes up
    {
        if (flags & STAT_CHANGE_BY_TWO)
            startingStatAnimId = STAT_ANIM_PLUS2 - 1;
        else
            startingStatAnimId = STAT_ANIM_PLUS1 - 1;

        while (statsToCheck != 0) {
            if (statsToCheck & 1 && gBattleMons[gActiveBattler].statStages[currStat] < MAX_STAT_STAGE) {
                statAnimId = startingStatAnimId + currStat;
                changeableStatsCount++;
            }
            statsToCheck >>= 1, currStat++;
        }

        if (changeableStatsCount > 1)  // more than one stat, so the color is gray
        {
            if (flags & STAT_CHANGE_BY_TWO)
                statAnimId = STAT_ANIM_MULTIPLE_PLUS2;
            else
                statAnimId = STAT_ANIM_MULTIPLE_PLUS1;
        }
    }

    if (flags & STAT_CHANGE_MULTIPLE_STATS && changeableStatsCount < 2) {
        // NOOP
    } else if (changeableStatsCount != 0 && !gBattleScripting.statAnimPlayed) {
        BtlController_EmitBattleAnimation(0, B_ANIM_STATS_CHANGE, statAnimId);
        MarkBattlerForControllerExec(gActiveBattler);
        if (flags & STAT_CHANGE_MULTIPLE_STATS && changeableStatsCount > 1) gBattleScripting.statAnimPlayed = TRUE;
    }
}

static void Cmd_playstatchangeanimation(void) {
    PlayStatChangeAnimation(GetBattlerForBattleScript(gBattlescriptCurrInstr[1]), gBattlescriptCurrInstr[2], gBattlescriptCurrInstr[3], FALSE);
    gBattlescriptCurrInstr += 4;
}

static bool32 TryKnockOffBattleScript(u32 battlerDef) {
    if (!gBattleMons[battlerDef].item) return FALSE;
    if (!CanBattlerGetOrLoseItem(battlerDef, gBattleMons[battlerDef].item)) return FALSE;
    if (!ShouldApplyOnHitAffect(battlerDef)) return FALSE;
    if (NoAliveMonsForEitherParty()) return FALSE;

    if ((gBattleScripting.abilityPopupOverwrite = IsStickyHold(battlerDef))) {
        gBattlerAbility = battlerDef;
        BattleScriptCall(BattleScript_StickyHoldActivates);
        return TRUE;
    }

    gLastUsedItem = UpdateBattlerItem(battlerDef, ITEM_NONE);

    if (gBattleMoves[gCurrentMove].effect == EFFECT_CORROSIVE_GAS)
        BattleScriptCall(BattleScript_CorrosiveGas);
    else
        BattleScriptCall(BattleScript_KnockedOff);

    return TRUE;
}

static int CanPickpocket(int target, int attackerStealing) {
    int stealer = attackerStealing ? gBattlerAttacker : target;
    int stolen = attackerStealing ? target : gBattlerAttacker;
    int makesContact = IsMoveMakingContact(gCurrentMove, gBattlerAttacker);
    int ability = makesContact ? ABILITY_PICKPOCKET : ABILITY_MAGICIAN;
    if (!BATTLER_DAMAGED(target)) return FALSE;
    if (gMoveResultFlags & MOVE_RESULT_NO_EFFECT) return FALSE;
    if (!BATTLER_HAS_ABILITY(stealer, ability)) return FALSE;
    if (DoesSubstituteBlockMove(gBattlerAttacker, target, gCurrentMove)) return FALSE;
    if (!CanStealItem(stealer, stolen, gBattleMons[stolen].item)) return FALSE;
    if (IsStickyHold(target)) return FALSE;
    if (TestSheerForceFlag(gBattlerAttacker, gCurrentMove)) return FALSE;
    return ability;
}

static void Cmd_moveend(void) {
    s32 i, j;
    bool32 effect = FALSE;
    u32 moveType = 0;
    u32 holdEffectAtk = 0;
    u16* choicedMoveAtk = NULL;
    u32 arg1, arg2;
    u32 originallyUsedMove;

    if (gChosenMove == 0xFFFF)
        originallyUsedMove = 0;
    else
        originallyUsedMove = gChosenMove;

    arg1 = gBattlescriptCurrInstr[1];
    arg2 = gBattlescriptCurrInstr[2];

    holdEffectAtk = GetBattlerHoldEffect(gBattlerAttacker, TRUE);
    choicedMoveAtk = &gBattleStruct->choicedMove[gBattlerAttacker];
    GET_MOVE_TYPE(gCurrentMove, moveType);

    if (AbilityBattleEffects(ABILITYEFFECT_REACTIVE, 0, 0, ABILITY_BS_CALL, 0)) return;

    do {
        switch (gBattleScripting.moveendState) {
            case MOVEEND_SUM_DAMAGE:  // Sum and store damage dealt for multi strike recoil
                gTurnStructs[gBattlerAttacker].savedDmg += gHpDealt;
                gBattleScripting.moveendState++;

                gTurnStructs[gBattlerTarget].sturdied = 0;
                gTurnStructs[gBattlerTarget].haloed = 0;
                gTurnStructs[gBattlerTarget].focusBanded = 0;
                gTurnStructs[gBattlerTarget].focusSashed = 0;
                break;
            case MOVEEND_PROTECT_LIKE_EFFECT:
                if (gRoundStructs[gBattlerAttacker].touchedProtectLike) {
                    if (gRoundStructs[gBattlerTarget].spikyShielded) {
                        gRoundStructs[gBattlerAttacker].touchedProtectLike = FALSE;
                        gBattleScripting.moveEffect = MOVE_EFFECT_BLEED;
                        PREPARE_MOVE_BUFFER(gBattleTextBuff1, MOVE_SPIKY_SHIELD);
                        BattleScriptCall(BattleScript_KingsShieldEffect);
                        effect = 1;
                    } else if (gRoundStructs[gBattlerTarget].kingsShielded) {
                        gRoundStructs[gBattlerAttacker].touchedProtectLike = FALSE;
                        gBattleScripting.moveEffect = (B_KINGS_SHIELD_LOWER_ATK >= GEN_8) ? MOVE_EFFECT_ATK_MINUS_1 : MOVE_EFFECT_ATK_MINUS_2;
                        BattleScriptCall(BattleScript_KingsShieldEffect);
                        effect = 1;
                    } else if (gRoundStructs[gBattlerTarget].angelsWrathProtected) {
                        bool8 change = FALSE;
                        gRoundStructs[gBattlerAttacker].touchedProtectLike = FALSE;
                        gStackBattler3 = gBattlerAttacker;
                        gStackBattler4 = gBattlerTarget;
                        gBattlerAttacker = gStackBattler4;
                        gBattlerTarget = gStackBattler3;

                        for (j = 1; j < NUM_STATS; j++) {
                            if (gBattleMons[gBattlerTarget].statStages[j] > 0)
                                change = change || ChangeStatBuffs(gBattlerTarget, -1, j, STAT_BUFF_DONT_SET_BUFFERS, NULL);
                        }
                        if (change) {
                            SetStatChanger(STAT_ATK, -1);
                            BattleScriptCall(BattleScript_AngelsWrathProtectEffect);
                            effect = 1;
                        } else {
                            gBattlerAttacker = gStackBattler3;
                            gBattlerTarget = gStackBattler4;
                        }
                    } else if (gRoundStructs[gBattlerTarget].banefulBunkered) {
                        gRoundStructs[gBattlerAttacker].touchedProtectLike = FALSE;
                        gBattleScripting.moveEffect = MOVE_EFFECT_POISON;
                        PREPARE_MOVE_BUFFER(gBattleTextBuff1, MOVE_BANEFUL_BUNKER);
                        BattleScriptCall(BattleScript_KingsShieldEffect);
                        effect = 1;
                    } else if (gRoundStructs[gBattlerTarget].silkTrapped) {
                        gRoundStructs[gBattlerAttacker].touchedProtectLike = FALSE;
                        gBattleScripting.moveEffect = MOVE_EFFECT_SPD_MINUS_1;
                        PREPARE_MOVE_BUFFER(gBattleTextBuff1, MOVE_SILK_TRAP);
                        BattleScriptCall(BattleScript_KingsShieldEffect);
                        effect = 1;
                    } else if (gRoundStructs[gBattlerTarget].burningBulwark) {
                        gRoundStructs[gBattlerAttacker].touchedProtectLike = FALSE;
                        gBattleScripting.moveEffect = MOVE_EFFECT_BURN;
                        PREPARE_MOVE_BUFFER(gBattleTextBuff1, MOVE_BURNING_BULWARK);
                        BattleScriptCall(BattleScript_KingsShieldEffect);
                        effect = 1;
                    } else if (gRoundStructs[gBattlerTarget].merculight &&
                               CalcTypeEffectivenessMultiplier(MOVE_MERCULIGHT, TYPE_ELECTRIC, gBattlerTarget, gBattlerAttacker, TRUE)) {
                        gRoundStructs[gBattlerAttacker].touchedProtectLike = FALSE;
                        gBattleScripting.moveEffect = MOVE_EFFECT_PARALYSIS;
                        PREPARE_MOVE_BUFFER(gBattleTextBuff1, MOVE_MERCULIGHT);
                        BattleScriptCall(BattleScript_KingsShieldEffect);
                        effect = 1;
                    } else if (gRoundStructs[gBattlerTarget].iceBurnCharge) {
                        gRoundStructs[gBattlerAttacker].touchedProtectLike = FALSE;
                        gBattleScripting.moveEffect = MOVE_EFFECT_BURN;
                        PREPARE_MOVE_BUFFER(gBattleTextBuff1, MOVE_ICE_BURN);
                        BattleScriptCall(BattleScript_KingsShieldEffect);
                        effect = 1;
                    } else if (gRoundStructs[gBattlerTarget].freezeShockCharge) {
                        gRoundStructs[gBattlerAttacker].touchedProtectLike = FALSE;
                        gBattleScripting.moveEffect = MOVE_EFFECT_PARALYSIS;
                        PREPARE_MOVE_BUFFER(gBattleTextBuff1, MOVE_FREEZE_SHOCK);
                        BattleScriptCall(BattleScript_KingsShieldEffect);
                        effect = 1;
                    } else if (gRoundStructs[gBattlerTarget].obstructed && gCurrentMove != MOVE_SUCKER_PUNCH) {
                        gRoundStructs[gBattlerAttacker].touchedProtectLike = FALSE;
                        gBattleScripting.moveEffect = MOVE_EFFECT_DEF_MINUS_1;
                        PREPARE_MOVE_BUFFER(gBattleTextBuff1, MOVE_OBSTRUCT);
                        BattleScriptCall(BattleScript_KingsShieldEffect);
                        effect = 1;
                    } else if (gRoundStructs[gBattlerTarget].mindReader) {
                        gRoundStructs[gBattlerAttacker].touchedProtectLike = FALSE;
                        gBattleScripting.moveEffect = MOVE_EFFECT_SP_DEF_MINUS_1;
                        PREPARE_MOVE_BUFFER(gBattleTextBuff1, MOVE_MIND_READER);
                        BattleScriptCall(BattleScript_KingsShieldEffect);
                        effect = 1;
                    } else if (gRoundStructs[gBattlerTarget].tanglingHusked) {
                        gRoundStructs[gBattlerAttacker].touchedProtectLike = FALSE;
                        gBattleScripting.moveEffect = MOVE_EFFECT_SPD_MINUS_1;
                        PREPARE_MOVE_BUFFER(gBattleTextBuff1, MOVE_TANGLING_HUSK);
                        BattleScriptCall(BattleScript_KingsShieldEffect);
                        effect = 1;
                    }
                }
                gBattleScripting.moveendState++;
                break;
            case MOVEEND_RAGE:  // rage check
                if (gBattleMons[gBattlerTarget].status2 & STATUS2_RAGE && gBattleMons[gBattlerTarget].hp != 0 && gBattlerAttacker != gBattlerTarget &&
                    GetBattlerSide(gBattlerAttacker) != GetBattlerSide(gBattlerTarget) && !(gMoveResultFlags & MOVE_RESULT_NO_EFFECT) && TARGET_TURN_DAMAGED &&
                    gBattleMoves[gCurrentMove].power && CompareStat(gBattlerTarget, STAT_ATK, MAX_STAT_STAGE, CMP_LESS_THAN)) {
                    ChangeStatBuffs(gBattlerTarget, 1, STAT_ATK, MOVE_EFFECT_AFFECTS_USER | STAT_BUFF_DONT_SET_BUFFERS, NULL);
                    BattleScriptCall(BattleScript_RageIsBuilding);
                    effect = TRUE;
                }
                gBattleScripting.moveendState++;
                break;
            case MOVEEND_DEFROST:  // defrosting check
                if (gBattleMons[gBattlerTarget].status1 & STATUS1_FREEZE && gBattleMons[gBattlerTarget].hp != 0 && gBattlerAttacker != gBattlerTarget &&
                    !(gMoveResultFlags & MOVE_RESULT_NO_EFFECT) && (moveType == TYPE_FIRE || gBattleMoves[gCurrentMove].effect == EFFECT_SCALD)) {
                    gBattleMons[gBattlerTarget].status1 &= ~(STATUS1_FREEZE);
                    gActiveBattler = gBattlerTarget;
                    BtlController_EmitSetMonData(0, REQUEST_STATUS_BATTLE, 0, 4, &gBattleMons[gBattlerTarget].status1);
                    MarkBattlerForControllerExec(gActiveBattler);
                    BattleScriptCall(BattleScript_DefrostedViaFireMove);
                    effect = TRUE;
                }
                if (gBattleMons[gBattlerTarget].status1 & STATUS1_FROSTBITE && gBattleMons[gBattlerTarget].hp != 0 && gBattlerAttacker != gBattlerTarget &&
                    gBattleMoves[originallyUsedMove].flags & FLAG_THAW_USER && !(gMoveResultFlags & MOVE_RESULT_NO_EFFECT)) {
                    gBattleMons[gBattlerTarget].status1 &= ~STATUS1_FROSTBITE;
                    gActiveBattler = gBattlerTarget;
                    BtlController_EmitSetMonData(
                        0, REQUEST_STATUS_BATTLE, 0, sizeof(gBattleMons[gBattlerTarget].status1), &gBattleMons[gBattlerTarget].status1);
                    MarkBattlerForControllerExec(gActiveBattler);
                    BattleScriptCall(BattleScript_FrostbiteHealedViaFireMove);
                    effect = TRUE;
                }
                gBattleScripting.moveendState++;
                break;
            case MOVEEND_RECOIL:
                gBattleScripting.moveendState++;

                if (IS_MOVE_STATUS(gCurrentMove)) break;
                if (gMoveResultFlags & MOVE_RESULT_NO_EFFECT) break;
                if (gHitMarker & HITMARKER_UNABLE_TO_USE_MOVE) break;
                if (!gBattleMoveDamage) break;
                if (!IsBattlerAlive(gBattlerAttacker)) break;
                if (!gTurnStructs[gBattlerAttacker].savedDmg) break;
                if (gCurrentMove != MOVE_STRUGGLE) {
                    if (IsMagicGuardProtected(gBattlerAttacker)) break;
                    int blocked = FALSE;
                    ON_ABILITY(gBattlerAttacker, FALSE, gAbilities[ability].noRecoil, blocked = TRUE; break)
                    if (blocked) break;
                }

                switch (gBattleMoves[gCurrentMove].effect) {
                    case EFFECT_RECOIL_25_STATUS:
                    case EFFECT_RECOIL_25:
                        gBattleMoveDamage = max(1, gTurnStructs[gBattlerAttacker].savedDmg / 4);
                        break;
                    case EFFECT_RECOIL_33_STATUS:
                    case EFFECT_FLINCH_RECOIL_33:
                    case EFFECT_RECOIL_33:
                        gBattleMoveDamage = max(1, gTurnStructs[gBattlerAttacker].savedDmg / 3);
                        break;
                    case EFFECT_FLINCH_RECOIL_50:
                    case EFFECT_RECOIL_50:
                        gBattleMoveDamage = max(1, gTurnStructs[gBattlerAttacker].savedDmg / 2);
                        break;
                    default:
                        gBattleMoveDamage = 0;
                }

                if (gBattleMoveDamage) {
                    gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_RECOIL_NORMAL;
                    BattleScriptCall(BattleScript_MoveEffectRecoil);
                }

                ON_ABILITY(gBattlerAttacker,
                           FALSE,
                           gAbilities[ability].onRecoil,
                           int damage = gAbilities[ability].onRecoil(gTurnStructs[gBattlerAttacker].savedDmg, gBattlerAttacker, moveType);
                           FILTER(damage);
                           if (!gBattleMoveDamage) BattleScriptCall(BattleScript_MoveEffectRecoil);
                           gBattleScripting.abilityPopupOverwrite = ability;
                           BattleScriptCall(BattleScript_AbilityPopUp);
                           gBattleMoveDamage += damage;)

                if (gBattleMons[gBattlerAttacker].status2 & STATUS2_CONFUSION) {
                    if (!gBattleMoveDamage) {
                        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_RECOIL_CONFUSION;
                        BattleScriptCall(BattleScript_MoveEffectRecoil);
                    }
                    gBattleMoveDamage = max(1, gBattleMoveDamage + (gTurnStructs[gBattlerAttacker].savedDmg / 3));
                    BattleScriptCall(BattleScript_ConfusionAnimation);
                }

                if (!gBattleMoveDamage) break;

                ON_ABILITY(gBattlerAttacker, FALSE, gAbilities[ability].halfRecoil, gBattleMoveDamage = max(1, gBattleMoveDamage / 2))

                ReadActiveScriptInitialStackState();
                effect = TRUE;
                break;
            case MOVEEND_ABILITIES_AFTER_RECOIL:
                gHpDealt = gTurnStructs[gBattlerAttacker].savedDmg;
                if (AbilityBattleEffects(ABILITYEFFECT_AFTER_RECOIL, gBattlerAttacker, 0, 0, 0)) effect = TRUE;
                gBattleScripting.moveendState++;
                break;
            case MOVEEND_SYNCHRONIZE_TARGET:  // target synchronize
                if (AbilityBattleEffects(ABILITYEFFECT_SYNCHRONIZE, gBattlerTarget, 0, 0, 0)) effect = TRUE;
                gBattleScripting.moveendState++;
                break;
            case MOVEEND_ABILITIES:  // Such as abilities activating on contact(Poison Spore, Rough Skin, etc.).
                if (AbilityBattleEffects(ABILITYEFFECT_MOVE_END, gBattlerTarget, 0, 0, 0)) effect = TRUE;
                gBattleScripting.moveendState++;
                break;
            case MOVEEND_ABILITIES_ATTACKER:  // Poison Touch, possibly other in the future
                if (AbilityBattleEffects(ABILITYEFFECT_MOVE_END_ATTACKER, gBattlerAttacker, 0, 0, 0)) effect = TRUE;
                gBattleScripting.moveendState++;
                break;
            case MOVEEND_STATUS_IMMUNITY_ABILITIES:  // status immunities
                if (AbilityBattleEffects(ABILITYEFFECT_IMMUNITY, 0, 0, 0, 0))
                    effect = TRUE;  // it loops through all battlers, so we increment after its done with all battlers
                else
                    gBattleScripting.moveendState++;
                break;
            case MOVEEND_SYNCHRONIZE_ATTACKER:  // attacker synchronize
                if (AbilityBattleEffects(ABILITYEFFECT_ATK_SYNCHRONIZE, gBattlerAttacker, 0, 0, 0)) effect = TRUE;
                gBattleScripting.moveendState++;
                break;
            case MOVEEND_CHOICE_MOVE:  // update choice band move
                if (gHitMarker & HITMARKER_OBEYS && gChosenMove != MOVE_STRUGGLE && gChosenMove && !gProcessingExtraAttacks &&
                    gBattlerAttacker == GetTurnBattler() && (*choicedMoveAtk == 0 || *choicedMoveAtk == 0xFFFF) &&
                    (HOLD_EFFECT_CHOICE(holdEffectAtk) ||
                     (BATTLER_HAS_ABILITY(gBattlerAttacker, ABILITY_DISCIPLINE) && gBattleMoves[gChosenMove].effect == EFFECT_RAMPAGE) ||
                     BATTLER_HAS_ABILITY(gBattlerAttacker, ABILITY_GORILLA_TACTICS) || BATTLER_HAS_ABILITY(gBattlerAttacker, ABILITY_SAGE_POWER))) {
                    if (BATTLER_HAS_ABILITY(gBattlerAttacker, ABILITY_DISCIPLINE) && gBattleMoves[gChosenMove].effect == EFFECT_RAMPAGE)
                        gVolatileStructs[gBattlerAttacker].disciplineCounter = 3;

                    if ((gBattleMoves[gChosenMove].effect == EFFECT_BATON_PASS || gBattleMoves[gChosenMove].effect == EFFECT_HEALING_WISH) &&
                        !(gMoveResultFlags & MOVE_RESULT_FAILED)) {
                        ++gBattleScripting.moveendState;
                        break;
                    }
                    *choicedMoveAtk = gChosenMove;
                }
                for (i = 0; i < MAX_MON_MOVES; ++i) {
                    if (gBattleMons[gBattlerAttacker].moves[i] == *choicedMoveAtk) break;
                }
                if (i == MAX_MON_MOVES) *choicedMoveAtk = 0;
                ++gBattleScripting.moveendState;
                break;
            case MOVEEND_ITEM_EFFECTS_TARGET:
                if (ItemBattleEffects(ITEMEFFECT_TARGET, gBattlerTarget, FALSE)) effect = TRUE;
                gBattleScripting.moveendState++;
                break;
            case MOVEEND_MOVE_EFFECTS2_ON_EACH:  // For effects which should happen after target items, for example Knock Off after damage from Rocky
                                                 // Helmet.
            {
                int moveEffect = gBattleStruct->moveEffect2;
                gBattleStruct->moveEffect2 = 0;
                switch (moveEffect) {
                    case MOVE_EFFECT_STEAL_ITEM:
                        if (!gBattleMons[gBattlerAttacker].item) {
                            if (!CanStealItem(gBattlerAttacker, gBattlerTarget, gBattleMons[gBattlerTarget].item)) {
                                break;
                            }
                            StealTargetItem(gBattlerAttacker, gBattlerTarget);              // Attacker steals target item
                            gBattleMons[gBattlerAttacker].item = 0;                         // Item assigned later on with thief (see MOVEEND_CHANGED_ITEMS)
                            gBattleStruct->changedItems[gBattlerAttacker] = gLastUsedItem;  // Stolen item to be assigned later
                            BattleScriptCall(BattleScript_ItemSteal);
                            effect = TRUE;
                            break;
                        }
                        FALLTHROUGH
                    case MOVE_EFFECT_KNOCK_OFF:
                        effect = TryKnockOffBattleScript(gBattlerTarget);
                        break;
                    case MOVE_EFFECT_BUG_BITE:
                        effect = EatTargetBerry(gBattlerAttacker, gBattlerTarget);
                        break;
                    case MOVE_EFFECT_SMACK_DOWN:
                        if (!IsBattlerGrounded(gBattlerTarget) && IsBattlerAlive(gBattlerTarget)) {
                            gStatuses3[gBattlerTarget] |= STATUS3_SMACKED_DOWN;
                            gStatuses3[gBattlerTarget] &= ~(STATUS3_MAGNET_RISE | STATUS3_TELEKINESIS | STATUS3_ON_AIR);
                            effect = TRUE;
                            BattleScriptCall(BattleScript_MoveEffectSmackDown);
                        }
                        break;
                    case MOVE_EFFECT_REMOVE_STATUS:  // Smelling salts, Wake-Up Slap, Sparkling Aria
                        if ((gBattleMons[gBattlerTarget].status1 & gBattleMoves[gCurrentMove].argument) && IsBattlerAlive(gBattlerTarget)) {
                            gBattleMons[gBattlerTarget].status1 &= ~(gBattleMoves[gCurrentMove].argument);

                            gActiveBattler = gBattlerTarget;
                            BtlController_EmitSetMonData(0, REQUEST_STATUS_BATTLE, 0, 4, &gBattleMons[gActiveBattler].status1);
                            MarkBattlerForControllerExec(gActiveBattler);
                            effect = TRUE;
                            switch (gBattleMoves[gCurrentMove].argument) {
                                case STATUS1_PARALYSIS:
                                    BattleScriptCall(BattleScript_TargetPRLZHeal);
                                    break;
                                case STATUS1_SLEEP:
                                    BattleScriptCall(BattleScript_TargetWokeUp);
                                    break;
                                case STATUS1_BURN:
                                    BattleScriptCall(BattleScript_TargetBurnHeal);
                                    break;
                            }
                        }
                        break;  // MOVE_EFFECT_REMOVE_STATUS
                    default:
                        gBattleStruct->moveEffect2 = moveEffect;
                }
            }
                gBattleScripting.moveendState++;
                break;                   // MOVEEND_MOVE_EFFECTS2
            case MOVEEND_MOVE_EFFECTS2:  // For effects which should happen after target items, for example Knock Off after damage from Rocky Helmet.
                switch (gBattleStruct->moveEffect2) {
                    case MOVE_EFFECT_MAKE_IT_RAIN:
                        gBattleScripting.moveEffect = MOVE_EFFECT_SP_ATK_MINUS_1 | MOVE_EFFECT_AFFECTS_USER | MOVE_EFFECT_CERTAIN;
                        SetMoveEffect(FALSE, TRUE);
                        effect = TRUE;
                        break;
                    case MOVE_EFFECT_BURN_UP:
                        effect = TRUE;
                        BattleScriptCall(BattleScript_BurnUpRemoveType);
                        break;
                    case MOVE_EFFECT_SCALE_SHOT:
                        effect = TRUE;
                        BattleScriptCall(BattleScript_MoveEffectScaleShot);
                        break;
                    case MOVE_EFFECT_WYRM_WIND:
                        effect = TRUE;
                        BattleScriptCall(BattleScript_MoveEffectWyrmWind);
                        break;
                }
                gBattleStruct->moveEffect2 = 0;
                gBattleScripting.moveendState++;
                break;                   // MOVEEND_MOVE_EFFECTS2
            case MOVEEND_CHANGED_ITEMS:  // changed held items
                for (i = 0; i < gBattlersCount; i++) {
                    if (gBattleStruct->changedItems[i] != 0) {
                        gBattleMons[i].item = gBattleStruct->changedItems[i];
                        gBattleStruct->changedItems[i] = 0;
                        gTurnStructs[i].shouldTriggerSwitchItem = FALSE;
                    }
                }
                gBattleScripting.moveendState++;
                break;
            case MOVEEND_ITEM_EFFECTS_ALL:  // item effects for all battlers
                if (ItemBattleEffects(ITEMEFFECT_MOVE_END, 0, FALSE))
                    effect = TRUE;
                else
                    gBattleScripting.moveendState++;
                break;
            case MOVEEND_KINGSROCK:  // King's rock
                // These effects will occur at each hit in a multi-strike move
                if (ItemBattleEffects(ITEMEFFECT_KINGSROCK, 0, FALSE)) effect = TRUE;
                gBattleScripting.moveendState++;
                break;
            case MOVEEND_ATTACKER_INVISIBLE:  // make attacker sprite invisible
                if (gStatuses3[gBattlerAttacker] & (STATUS3_SEMI_INVULNERABLE) && gHitMarker & HITMARKER_NO_ANIMATIONS) {
                    gActiveBattler = gBattlerAttacker;
                    BtlController_EmitSpriteInvisibility(0, TRUE);
                    MarkBattlerForControllerExec(gActiveBattler);
                    gBattleScripting.moveendState++;
                    return;
                }
                gBattleScripting.moveendState++;
                break;
            case MOVEEND_ATTACKER_VISIBLE:  // make attacker sprite visible
                if (gMoveResultFlags & MOVE_RESULT_NO_EFFECT || !(gStatuses3[gBattlerAttacker] & (STATUS3_SEMI_INVULNERABLE)) ||
                    WasUnableToUseMove(gBattlerAttacker)) {
                    if (!gVolatileStructs[gBattlerAttacker].skyDropped) {
                        gActiveBattler = gBattlerAttacker;
                        BtlController_EmitSpriteInvisibility(0, FALSE);
                        MarkBattlerForControllerExec(gActiveBattler);
                        gStatuses3[gBattlerAttacker] &= ~STATUS3_SEMI_INVULNERABLE;
                        gTurnStructs[gBattlerAttacker].restoredBattlerSprite = TRUE;
                        gBattleScripting.moveendState++;
                        return;
                    }
                }
                gBattleScripting.moveendState++;
                break;
            case MOVEEND_TARGET_VISIBLE:  // make target sprite visible
                if (!gTurnStructs[gBattlerTarget].restoredBattlerSprite && gBattlerTarget < gBattlersCount &&
                    !(gStatuses3[gBattlerTarget] & STATUS3_SEMI_INVULNERABLE)) {
                    gActiveBattler = gBattlerTarget;
                    BtlController_EmitSpriteInvisibility(0, FALSE);
                    MarkBattlerForControllerExec(gActiveBattler);
                    gStatuses3[gBattlerTarget] &= ~STATUS3_SEMI_INVULNERABLE;
                    gBattleScripting.moveendState++;
                    return;
                }
                gBattleScripting.moveendState++;
                break;
            case MOVEEND_SUBSTITUTE:  // update substitute
                for (i = 0; i < gBattlersCount; i++) {
                    if (gVolatileStructs[i].substituteHP == 0 && (gBattleMons[i].status2 & STATUS2_SUBSTITUTE)) {
                        gBattleMons[i].status2 &= ~(STATUS2_SUBSTITUTE);
                        gVolatileStructs[i].substituteDestroyedThisTurn = TRUE;
                    }
                }
                gBattleScripting.moveendState++;
                break;
            case MOVEEND_UPDATE_LAST_MOVES:
                if (gMoveResultFlags & (MOVE_RESULT_FAILED | MOVE_RESULT_DOESNT_AFFECT_FOE))
                    gBattleStruct->lastMoveFailed |= gBitTable[gBattlerAttacker];
                else if (gBattleMoves[gCurrentMove].effect == EFFECT_FOCUS_PUNCH &&
                         (gRoundStructs[gBattlerAttacker].physicalDmg || gRoundStructs[gBattlerAttacker].specialDmg))
                    gBattleStruct->lastMoveFailed |= gBitTable[gBattlerAttacker];
                else
                    gBattleStruct->lastMoveFailed &= ~(gBitTable[gBattlerAttacker]);

                if (gHitMarker & HITMARKER_SWAP_ATTACKER_TARGET) {
                    gActiveBattler = gBattlerAttacker;
                    gBattlerAttacker = gBattlerTarget;
                    gBattlerTarget = gActiveBattler;
                    gHitMarker &= ~(HITMARKER_SWAP_ATTACKER_TARGET);
                }
                if (!gProcessingExtraAttacks) {
                    gVolatileStructs[gBattlerAttacker].usedMoves |= gBitTable[gCurrMovePos];
                    gBattleStruct->lastMoveTarget[gBattlerAttacker] = gBattlerTarget;
                    if (gHitMarker & HITMARKER_ATTACKSTRING_PRINTED) {
                        gLastPrintedMoves[gBattlerAttacker] = gChosenMove;
                        gLastUsedMove = gCurrentMove;
                    }
                }
                if (!(gAbsentBattlerFlags & gBitTable[gBattlerAttacker]) && !(gBattleStruct->field_91 & gBitTable[gBattlerAttacker]) &&
                    gBattleMoves[originallyUsedMove].effect != EFFECT_BATON_PASS && gBattleMoves[originallyUsedMove].effect != EFFECT_HEALING_WISH) {
                    if (gHitMarker & HITMARKER_OBEYS) {
                        if (!gProcessingExtraAttacks) {
                            gLastMoves[gBattlerAttacker] = gChosenMove;
                            gLastResultingMoves[gBattlerAttacker] = gCurrentMove;
                        }
                    } else {
                        gLastMoves[gBattlerAttacker] = 0xFFFF;
                        gLastResultingMoves[gBattlerAttacker] = 0xFFFF;
                    }

                    if (!(gHitMarker & HITMARKER_FAINTED(gBattlerTarget))) gLastHitBy[gBattlerTarget] = gBattlerAttacker;

                    if (gHitMarker & HITMARKER_OBEYS && !(gMoveResultFlags & MOVE_RESULT_NO_EFFECT)) {
                        if (gChosenMove == 0xFFFF) {
                            gLastLandedMoves[gBattlerTarget] = gChosenMove;
                        } else {
                            gLastLandedMoves[gBattlerTarget] = gCurrentMove;
                            GET_MOVE_TYPE(gCurrentMove, gLastHitByType[gBattlerTarget]);
                        }
                    } else {
                        gLastLandedMoves[gBattlerTarget] = 0xFFFF;
                    }
                }
                gBattleScripting.moveendState++;
                break;
            case MOVEEND_MIRROR_MOVE:  // mirror move
                if (!(gAbsentBattlerFlags & gBitTable[gBattlerAttacker]) && !(gBattleStruct->field_91 & gBitTable[gBattlerAttacker]) &&
                    gBattleMoves[originallyUsedMove].flags & FLAG_MIRROR_MOVE_AFFECTED && gHitMarker & HITMARKER_OBEYS && gBattlerAttacker != gBattlerTarget &&
                    !(gHitMarker & HITMARKER_FAINTED(gBattlerTarget)) && !(gMoveResultFlags & MOVE_RESULT_NO_EFFECT)) {
                    gBattleStruct->lastTakenMove[gBattlerTarget] = gChosenMove;
                    gBattleStruct->lastTakenMoveFrom[gBattlerTarget][gBattlerAttacker] = gChosenMove;
                }
                gBattleScripting.moveendState++;
                break;
            case MOVEEND_NEXT_TARGET:  // For moves hitting two opposing Pokemon.
                // Set a flag if move hits either target (for throat spray that can't check damage)
                if (!(gHitMarker & HITMARKER_UNABLE_TO_USE_MOVE) && !(gMoveResultFlags & MOVE_RESULT_NO_EFFECT))
                    gRoundStructs[gBattlerAttacker].targetAffected = TRUE;

                if (!(gHitMarker & HITMARKER_UNABLE_TO_USE_MOVE) && gBattleTypeFlags & BATTLE_TYPE_DOUBLE && !gRoundStructs[gBattlerAttacker].chargingTurn &&
                    (GetBattlerBattleMoveTargetFlags(gCurrentMove, gBattlerAttacker) == MOVE_TARGET_BOTH ||
                     GetBattlerBattleMoveTargetFlags(gCurrentMove, gBattlerAttacker) == MOVE_TARGET_FOES_AND_ALLY) &&
                    !(gHitMarker & HITMARKER_NO_ATTACKSTRING)) {
                    u8 battlerId;

                    if (GetBattlerBattleMoveTargetFlags(gCurrentMove, gBattlerAttacker) == MOVE_TARGET_FOES_AND_ALLY) {
                        gHitMarker |= HITMARKER_NO_PPDEDUCT;
                        for (battlerId = gBattlerTarget + 1; battlerId < gBattlersCount; battlerId++) {
                            if (battlerId == gBattlerAttacker) continue;
                            if (IsBattlerAlive(battlerId)) break;
                        }
                    } else {
                        battlerId = GetBattlerAtPosition(BATTLE_PARTNER(GetBattlerPosition(gBattlerTarget)));
                        gHitMarker |= HITMARKER_NO_ATTACKSTRING;
                    }

                    if (IsBattlerAlive(battlerId)) {
                        if (gBattleMoves[gCurrentMove].type2) {
                            SetTypeBeforeUsingMove(gCurrentMove, gBattlerAttacker);
                        }
                        gBattlerTarget = battlerId;
                        gBattleScripting.moveendState = 0;
                        MoveValuesCleanUp();
                        gBattleScripting.moveEffect = gBattleScripting.savedMoveEffect;
                        BattleScriptPush(gBattleScriptsForMoveEffects[gBattleMoves[gCurrentMove].effect]);
                        gBattlescriptCurrInstr = BattleScript_FlushMessageBox;
                        return;
                    } else {
                        gHitMarker |= HITMARKER_NO_ATTACKSTRING;
                        gHitMarker &= ~(HITMARKER_NO_PPDEDUCT);
                    }
                }
                RecordLastUsedMoveBy(gBattlerAttacker, gCurrentMove);
                gBattleScripting.moveendState++;
                break;
            case MOVEEND_EJECT_BUTTON:
                if (gBattleMoves[gCurrentMove].effect != EFFECT_HIT_SWITCH_TARGET && IsBattlerAlive(gBattlerAttacker) &&
                    !TestSheerForceFlag(gBattlerAttacker, gCurrentMove) &&
                    (GetBattlerSide(gBattlerAttacker) == B_SIDE_PLAYER || (gBattleTypeFlags & BATTLE_TYPE_TRAINER))) {
                    // Since we check if battler was damaged, we don't need to check move result.
                    // In fact, doing so actually prevents multi-target moves from activating eject button properly
                    u8 battlers[4] = {0, 1, 2, 3};
                    SortBattlersBySpeed(battlers, FALSE);
                    for (i = 0; i < gBattlersCount; i++) {
                        u8 battler = battlers[i];
                        if (battler == GetTurnBattler()) continue;
                        // Attacker is the damage-dealer, battler is mon to be switched out
                        if (gTurnStructs[battler].shouldTriggerSwitchItem && IsBattlerAlive(battler) &&
                            GetBattlerHoldEffect(battler, TRUE) == HOLD_EFFECT_EJECT_BUTTON && !IsUnnerveAbilityOnOpposingSide(battler) &&
                            CanBattlerSwitch(battler))  // Has mon to switch into
                        {
                            gStackBattler1 = battler;
                            gTurnStructs[battler].shouldTriggerSwitchItem = FALSE;
                            gLastUsedItem = gBattleMons[battler].item;
                            if (gBattleMoves[gCurrentMove].effect == EFFECT_HIT_ESCAPE)
                                gBattlescriptCurrInstr = BattleScript_MoveEnd;  // Prevent user switch-in selection
                            BattleScriptCall(BattleScript_EjectButtonActivates);
                            effect = TRUE;
                            break;  // Only the fastest Eject Button activates
                        }
                    }
                }
                gBattleScripting.moveendState++;
                break;
            case MOVEEND_RED_CARD:
                if (gBattleMoves[gCurrentMove].effect != EFFECT_HIT_SWITCH_TARGET && IsBattlerAlive(gBattlerAttacker) &&
                    !TestSheerForceFlag(gBattlerAttacker, gCurrentMove)) {
                    // Since we check if battler was damaged, we don't need to check move result.
                    // In fact, doing so actually prevents multi-target moves from activating red card properly
                    u8 battlers[4] = {0, 1, 2, 3};
                    SortBattlersBySpeed(battlers, FALSE);
                    for (i = 0; i < gBattlersCount; i++) {
                        u8 battler = battlers[i];
                        RestrainingOrderState restrainingOrderState = GetAbilityState(battler, ABILITY_RESTRAINING_ORDER);
                        // Search for fastest hit pokemon with a red card
                        // Attacker is the one to be switched out, battler is one with red card
                        if (restrainingOrderState == RESTRAINING_ORDER_ACTIVATING && IsBattlerAlive(battler) && CanBattlerSwitch(gBattlerAttacker)) {
                            SetAbilityState(battler, ABILITY_RESTRAINING_ORDER, RESTRAINING_ORDER_DONE);
                            gBattleScripting.abilityPopupOverwrite = ABILITY_RESTRAINING_ORDER;
                            gBattlerAbility = gStackBattler1 = battler;
                            if (gBattleMoves[gCurrentMove].effect == EFFECT_HIT_ESCAPE)
                                gBattlescriptCurrInstr = BattleScript_MoveEnd;  // Prevent user switch-in selection
                            BattleScriptCall(BattleScript_RestrainingOrderActivates);
                            effect = TRUE;
                            break;
                        } else if (gTurnStructs[battler].shouldTriggerSwitchItem && IsBattlerAlive(battler) &&
                                   GetBattlerHoldEffect(battler, TRUE) == HOLD_EFFECT_RED_CARD && !IsUnnerveAbilityOnOpposingSide(battler) &&
                                   CanBattlerSwitch(gBattlerAttacker))  // Has mon to switch into
                        {
                            gLastUsedItem = gBattleMons[battler].item;
                            gStackBattler1 = battler;
                            if (gBattleMoves[gCurrentMove].effect == EFFECT_HIT_ESCAPE)
                                gBattlescriptCurrInstr = BattleScript_MoveEnd;  // Prevent user switch-in selection
                            BattleScriptCall(BattleScript_RedCardActivates);
                            effect = TRUE;
                            break;  // Only fastest red card activates
                        }
                    }
                }

                for (i = 0; i < gBattlersCount; i++) {
                    if (GetAbilityState(i, ABILITY_RESTRAINING_ORDER) == RESTRAINING_ORDER_ACTIVATING)
                        SetAbilityState(i, ABILITY_RESTRAINING_ORDER, RESTRAINING_ORDER_NOT_TRIGGERED);
                }
                gBattleScripting.moveendState++;
                break;
            case MOVEEND_EJECT_PACK:
                if (!(gCurrentMove == MOVE_PARTING_SHOT && CanBattlerSwitch(gBattlerAttacker))) {
                    u8 battlers[4] = {0, 1, 2, 3};
                    SortBattlersBySpeed(battlers, FALSE);
                    for (i = 0; i < gBattlersCount; i++) {
                        u8 battler = battlers[i];
                        if (IsBattlerAlive(battler) && gRoundStructs[battler].statFell && gRoundStructs[battler].disableEjectPack == 0 &&
                            GetBattlerHoldEffect(battler, TRUE) == HOLD_EFFECT_EJECT_PACK && CountUsablePartyMons(battler) > 0)  // Has mon to switch into
                        {
                            gStackBattler1 = battler;
                            gRoundStructs[battler].statFell = FALSE;
                            gLastUsedItem = gBattleMons[battler].item;
                            BattleScriptCall(BattleScript_EjectPackActivates);
                            effect = TRUE;
                            break;  // Only fastest eject pack activates
                        }

                        if (IsBattlerAlive(battler) && gRoundStructs[battler].statFell && gRoundStructs[battler].disableEjectPack == 0 &&
                            BATTLER_HAS_ABILITY(battler, ABILITY_EJECT_PACK_ABILITY) && !GetSingleUseAbilityCounter(battler, ABILITY_EJECT_PACK_ABILITY) &&
                            CanBattlerSwitch(battler))  // Has mon to switch into
                        {
                            SetSingleUseAbilityCounter(battler, ABILITY_EJECT_PACK_ABILITY, TRUE);
                            gStackBattler1 = battler;
                            gRoundStructs[battler].statFell = FALSE;
                            gBattleScripting.abilityPopupOverwrite = ABILITY_EJECT_PACK_ABILITY;
                            BattleScriptCall(BattleScript_EmergencyExitPopupNoPause);
                            effect = TRUE;
                            break;  // Only fastest eject pack activates
                        }
                    }
                }
                gBattleScripting.moveendState++;
                break;
            case MOVEEND_LIFEORB_SHELLBELL:
                if (ItemBattleEffects(ITEMEFFECT_LIFEORB_SHELLBELL, 0, FALSE)) effect = TRUE;
                gBattleScripting.moveendState++;
                break;
            case MOVEEND_PICKPOCKET: {
                int ability;
                int checkOffense = gBattleMons[gBattlerAttacker].item == ITEM_NONE;
                u8 thieves[4] = {0};
                int thiefCount = 0;
                for (i = 0; i < gBattlersCount; i++) {
                    if (i == gBattlerAttacker) continue;
                    if ((ability = CanPickpocket(i, checkOffense))) thieves[thiefCount++] = i;
                }
                if (thiefCount > 0) {
                    if (checkOffense) {
                        gStackBattler1 = gBattlerAttacker;
                        gStackBattler2 = thieves[Random() % thiefCount];
                    } else {
                        gStackBattler1 = thieves[Random() % thiefCount];
                        gStackBattler2 = gBattlerAttacker;
                    }
                    StealTargetItem(gStackBattler1, gStackBattler2);
                    gBattleScripting.abilityPopupOverwrite = ability;
                    BattleScriptCall(BattleScript_Pickpocket);  // Includes sticky hold check to print separate string
                    effect = TRUE;
                }
            }
                gBattleScripting.moveendState++;
                break;
            case MOVEEND_DANCER:  // Special case because it's so annoying
                if (!(gMoveResultFlags & MOVE_RESULT_NO_EFFECT)) {
                    int i, dancersCount = 0, include = 0;
                    u8 battlers[MAX_BATTLERS_COUNT];

                    // Get list of battlers that can dance
                    for (i = 0; i < gBattlersCount; i++) {
                        FILTER(i != gBattlerAttacker)
                        FILTER_NOT(gTurnStructs[i].dancerUsedMove)
                        FILTER(IsBattlerAlive(i))
                        include |= 1 << i;
                        dancersCount++;
                    }

                    SortBattlersExcept(battlers, TRUE, ~include);

                    // Reverse order so faster battlers resolve first
                    for (i = dancersCount - 1; i >= 0; i--) {
                        // Out of turn moves do not use battle scripting so there's no point in pausing
                        AbilityBattleEffects(ABILITYEFFECT_MOVE_END_OTHER, battlers[i], 0, 0, 0);
                    }
                }
                gBattleScripting.moveendState++;
                break;
            case MOVEEND_EMERGENCY_EXIT:  // Special case, because moves hitting multiple opponents stop after switching out
                for (i = 0; i < gBattlersCount; i++) {
                    if (gBattleResources->flags->flags[i] & RESOURCE_FLAG_EMERGENCY_EXIT) {
                        gStackBattler1 = i;
                        gBattleResources->flags->flags[i] &= ~(RESOURCE_FLAG_EMERGENCY_EXIT);
                        BattleScriptPushCursor();
                        if (gBattleTypeFlags & BATTLE_TYPE_TRAINER || GetBattlerSide(i) == B_SIDE_PLAYER) {
                            if (B_ABILITY_POP_UP)
                                gBattlescriptCurrInstr = BattleScript_EmergencyExit;
                            else
                                gBattlescriptCurrInstr = BattleScript_EmergencyExitNoPopUp;
                        } else {
                            if (B_ABILITY_POP_UP)
                                gBattlescriptCurrInstr = BattleScript_EmergencyExitWild;
                            else
                                gBattlescriptCurrInstr = BattleScript_EmergencyExitWildNoPopUp;
                        }
                        return;
                    }
                }
                gBattleScripting.moveendState++;
                break;
            case MOVEEND_MULTIHIT_MOVE:
                if (!(gMoveResultFlags & MOVE_RESULT_NO_EFFECT) && !(gHitMarker & HITMARKER_UNABLE_TO_USE_MOVE) &&
                    gTurnStructs[gBattlerAttacker].multiHitCounter)  // Silly edge case
                {
                    gBattleScripting.multihitString[4]++;
                    if (--gTurnStructs[gBattlerAttacker].multiHitCounter == 0) {
                        BattleScriptCall(BattleScript_MultiHitPrintStrings);
                        effect = 1;
                    } else {
                        // Clear hitmarker flags from abilities
                        gHitMarker &= ~HITMARKER_IGNORE_SAFEGUARD;

                        if (gCurrentMove == MOVE_DRAGON_DARTS) {
                            if (IsBattlerAlive(BATTLE_PARTNER(gBattlerTarget))) gBattlerTarget = BATTLE_PARTNER(gBattlerTarget);
                        }

                        if (gBattleMons[gBattlerAttacker].hp && gBattleMons[gBattlerTarget].hp &&
                            (gChosenMove == MOVE_SLEEP_TALK || !(gBattleMons[gBattlerAttacker].status1 & STATUS1_SLEEP)) &&
                            !(gBattleMons[gBattlerAttacker].status1 & STATUS1_FREEZE)) {
                            if (gTurnStructs[gBattlerAttacker].parentalBondOn) gTurnStructs[gBattlerAttacker].parentalBondOn--;

                            gHitMarker |= (HITMARKER_NO_PPDEDUCT | HITMARKER_NO_ATTACKSTRING);
                            gBattleScripting.animTargetsHit = 0;
                            gBattleScripting.moveendState = 0;
                            gTurnStructs[gBattlerAttacker].multiHitsUsed++;
                            MoveValuesCleanUp();
                            BattleScriptPush(gBattleScriptsForMoveEffects[gBattleMoves[gCurrentMove].effect]);
                            gBattlescriptCurrInstr = BattleScript_FlushMessageBox;
                            return;
                        } else {
                            BattleScriptCall(BattleScript_MultiHitPrintStrings);
                            effect = 1;
                        }
                    }
                }
                gTurnStructs[gBattlerAttacker].multiHitCounter = 0;
                gTurnStructs[gBattlerAttacker].parentalBondOn = gTurnStructs[gBattlerAttacker].parentalBondInitialCount = 0;
                gTurnStructs[gBattlerAttacker].multiHitsUsed = 0;
                gBattleScripting.moveendState++;
                break;
            case MOVEEND_CHARGE: {
                u8 currentMoveType;
                GET_MOVE_TYPE(gCurrentMove, currentMoveType)
                if (currentMoveType == TYPE_ELECTRIC && gBattleMoves[gCurrentMove].power && !(gBattleMons[gBattlerAttacker].status2 & STATUS2_MULTIPLETURNS) &&
                    GetOncePerTurnAbilityCounter(gBattlerTarget, ABILITY_ENERGIZED) <= 0) {
                    gStatuses3[gBattlerAttacker] &= ~STATUS3_CHARGED_UP;
                }
                gBattleScripting.moveendState++;
                break;
            }
            case MOVEEND_CLEAR_BITS:  // Clear/Set bits for things like using a move for all targets and all hits.

#if B_RAMPAGE_CANCELLING >= GEN_5
                if (gBattleMoves[gCurrentMove].effect == EFFECT_RAMPAGE                                                 // If we're rampaging
                    && (gMoveResultFlags & MOVE_RESULT_NO_EFFECT)                                                       // And it is unusable
                    && (gBattleMons[gBattlerAttacker].status2 & STATUS2_LOCK_CONFUSE) != STATUS2_LOCK_CONFUSE_TURN(1))  // And won't end this turn
                    CancelMultiTurnMoves(gBattlerAttacker);                                                             // Cancel it
#endif

                if (!gProcessingExtraAttacks && gBattleMoves[gCurrentMove].effect != EFFECT_ROLLOUT) gVolatileStructs[gBattlerAttacker].rolloutCounter = 0;

                gRoundStructs[gBattlerAttacker].usesBouncedMove = FALSE;
                gRoundStructs[gBattlerAttacker].targetAffected = FALSE;
                gBattleStruct->ateBoost[gBattlerAttacker] = 0;
                gStatuses3[gBattlerAttacker] &= ~(STATUS3_ME_FIRST);
                gTurnStructs[gBattlerAttacker].gemBoost = FALSE;
                gTurnStructs[gBattlerAttacker].damagedMons = 0;
                gTurnStructs[gBattlerTarget].berryReduced = FALSE;
                gBattleScripting.moveEffect = 0;
                gBattleScripting.moveSecondaryEffectChance = 0;
                gBattleScripting.forceFalseSwipeEffect = FALSE;
                gBattleScripting.moveendState++;
                break;
            case MOVEEND_COUNT:
                break;
        }

        if (arg1 == 1 && effect == FALSE) gBattleScripting.moveendState = MOVEEND_COUNT;
        if (arg1 == 2 && arg2 == gBattleScripting.moveendState && effect == FALSE) gBattleScripting.moveendState = MOVEEND_COUNT;

    } while (gBattleScripting.moveendState != MOVEEND_COUNT && effect == FALSE);

    if (gBattleScripting.moveendState == MOVEEND_COUNT && effect == FALSE) gBattlescriptCurrInstr += 3;
}

static void Cmd_sethealblock(void) {
    if (gStatuses3[gBattlerTarget] & STATUS3_HEAL_BLOCK) {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    } else {
        gStatuses3[gBattlerTarget] |= STATUS3_HEAL_BLOCK;
        gVolatileStructs[gBattlerTarget].healBlockTimer = 5;
        gBattlescriptCurrInstr += 5;
    }
}

static void Cmd_returnatktoball(void) {
    if (gBattleScripting.switchInBattlerOverwrite != MAX_BATTLERS_COUNT) {  // Handles Neutralizing Gas
        gBattlerAttacker = gBattleScripting.switchInBattlerOverwrite;
        gBattleScripting.switchInBattlerOverwrite = MAX_BATTLERS_COUNT;
    }

    gActiveBattler = gBattlerAttacker;

    if (!(gHitMarker & HITMARKER_FAINTED(gActiveBattler))) {
        BtlController_EmitReturnMonToBall(0, 0);
        MarkBattlerForControllerExec(gActiveBattler);
    }
    gBattlescriptCurrInstr++;
}

static void Cmd_getswitchedmondata(void) {
    if (gBattleControllerExecFlags) return;

    gActiveBattler = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);

    gBattlerPartyIndexes[gActiveBattler] = *(gBattleStruct->monToSwitchIntoId + gActiveBattler);

    BtlController_EmitGetMonData(0, REQUEST_ALL_BATTLE, gBitTable[gBattlerPartyIndexes[gActiveBattler]]);
    MarkBattlerForControllerExec(gActiveBattler);

    gBattlescriptCurrInstr += 2;
}

static void Cmd_switchindataupdate(void) {
    struct BattlePokemon oldData;

    if (gBattleControllerExecFlags) return;

    gActiveBattler = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);
    oldData = gBattleMons[gActiveBattler];
    memcpy(&gBattleMons[gActiveBattler], &gBattleResources->bufferB[gActiveBattler][4], sizeof(gBattleMons[gActiveBattler]));

    gBattleMons[gActiveBattler].type1 = RandomizeType(
        gBaseStats[gBattleMons[gActiveBattler].species].type1, gBattleMons[gActiveBattler].species, gBattleMons[gActiveBattler].personality, TRUE);
    gBattleMons[gActiveBattler].type2 = RandomizeType(
        gBaseStats[gBattleMons[gActiveBattler].species].type2, gBattleMons[gActiveBattler].species, gBattleMons[gActiveBattler].personality, FALSE);
    gBattleMons[gActiveBattler].type3 = TYPE_MYSTERY;

    if (gBattleMoves[gCurrentMove].effect == EFFECT_BATON_PASS || gBattleMoves[gCurrentMove].effect == EFFECT_SHED_TAIL) {
        ARRAY_COPY(gBattleMons[gActiveBattler].statStages, oldData.statStages)
        gBattleMons[gActiveBattler].status2 = oldData.status2;
    }

    SwitchInClearSetData();

    if (gBattleTypeFlags & BATTLE_TYPE_PALACE && gBattleMons[gActiveBattler].maxHP / 2 >= gBattleMons[gActiveBattler].hp &&
        gBattleMons[gActiveBattler].hp != 0 && !(gBattleMons[gActiveBattler].status1 & STATUS1_SLEEP)) {
        gBattleStruct->palaceFlags |= gBitTable[gActiveBattler];
    }

    gBattleScripting.battler = gActiveBattler;

    PREPARE_MON_NICK_BUFFER(gBattleTextBuff1, gActiveBattler, gBattlerPartyIndexes[gActiveBattler]);

    gBattlescriptCurrInstr += 2;
}

static void Cmd_switchinanim(void) {
    if (gBattleControllerExecFlags) return;

    gActiveBattler = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);

    if (GetBattlerSide(gActiveBattler) == B_SIDE_OPPONENT &&
        !(gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_EREADER_TRAINER | BATTLE_TYPE_RECORDED_LINK | BATTLE_TYPE_TRAINER_HILL | BATTLE_TYPE_FRONTIER)))
        HandleSetPokedexFlag(SpeciesToNationalPokedexNum(gBattleMons[gActiveBattler].species), FLAG_SET_SEEN, gBattleMons[gActiveBattler].personality);

    gAbsentBattlerFlags &= ~(gBitTable[gActiveBattler]);

    BtlController_EmitSwitchInAnim(0, gBattlerPartyIndexes[gActiveBattler], gBattlescriptCurrInstr[2]);
    MarkBattlerForControllerExec(gActiveBattler);

    gBattlescriptCurrInstr += 3;

    if (gBattleTypeFlags & BATTLE_TYPE_ARENA) BattleArena_InitPoints();
}

bool32 CanBattlerSwitch(u32 battlerId) {
    s32 i, lastMonId, battlerIn1, battlerIn2;
    bool32 ret = FALSE;
    struct Pokemon* party;

    if (gStatuses4[battlerId] & STATUS4_COMMANDED) return FALSE;

    if (BATTLE_TWO_VS_ONE_OPPONENT && GetBattlerSide(battlerId) == B_SIDE_OPPONENT) {
        battlerIn1 = GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT);
        battlerIn2 = GetBattlerAtPosition(B_POSITION_OPPONENT_RIGHT);
        party = gEnemyParty;

        for (i = 0; i < PARTY_SIZE; i++) {
            if (GetMonData(&party[i], MON_DATA_HP) != 0 && GetMonData(&party[i], MON_DATA_SPECIES) != SPECIES_NONE && !GetMonData(&party[i], MON_DATA_IS_EGG) &&
                i != gBattlerPartyIndexes[battlerIn1] && i != gBattlerPartyIndexes[battlerIn2])
                break;
        }

        ret = (i != PARTY_SIZE);
    } else if (gBattleTypeFlags & BATTLE_TYPE_INGAME_PARTNER) {
        if (GetBattlerSide(battlerId) == B_SIDE_OPPONENT)
            party = gEnemyParty;
        else
            party = gPlayerParty;

        lastMonId = 0;
        if (battlerId & 2) lastMonId = 3;

        for (i = lastMonId; i < lastMonId + 3; i++) {
            if (GetMonData(&party[i], MON_DATA_SPECIES) != SPECIES_NONE && !GetMonData(&party[i], MON_DATA_IS_EGG) && GetMonData(&party[i], MON_DATA_HP) != 0 &&
                gBattlerPartyIndexes[battlerId] != i)
                break;
        }

        ret = (i != lastMonId + 3);
    } else if (gBattleTypeFlags & BATTLE_TYPE_MULTI) {
        if (gBattleTypeFlags & BATTLE_TYPE_TOWER_LINK_MULTI) {
            if (GetBattlerSide(battlerId) == B_SIDE_PLAYER) {
                party = gPlayerParty;

                lastMonId = 0;
                if (GetLinkTrainerFlankId(GetBattlerMultiplayerId(battlerId)) == TRUE) lastMonId = 3;
            } else {
                party = gEnemyParty;

                if (battlerId == 1)
                    lastMonId = 0;
                else
                    lastMonId = 3;
            }
        } else {
            if (GetBattlerSide(battlerId) == B_SIDE_OPPONENT)
                party = gEnemyParty;
            else
                party = gPlayerParty;

            lastMonId = 0;
            if (GetLinkTrainerFlankId(GetBattlerMultiplayerId(battlerId)) == TRUE) lastMonId = 3;
        }

        for (i = lastMonId; i < lastMonId + 3; i++) {
            if (GetMonData(&party[i], MON_DATA_SPECIES) != SPECIES_NONE && !GetMonData(&party[i], MON_DATA_IS_EGG) && GetMonData(&party[i], MON_DATA_HP) != 0 &&
                gBattlerPartyIndexes[battlerId] != i)
                break;
        }

        ret = (i != lastMonId + 3);
    } else if (gBattleTypeFlags & BATTLE_TYPE_TWO_OPPONENTS && GetBattlerSide(battlerId) == B_SIDE_OPPONENT) {
        party = gEnemyParty;

        lastMonId = 0;
        if (battlerId == B_POSITION_OPPONENT_RIGHT) lastMonId = 3;

        for (i = lastMonId; i < lastMonId + 3; i++) {
            if (GetMonData(&party[i], MON_DATA_SPECIES) != SPECIES_NONE && !GetMonData(&party[i], MON_DATA_IS_EGG) && GetMonData(&party[i], MON_DATA_HP) != 0 &&
                gBattlerPartyIndexes[battlerId] != i)
                break;
        }

        ret = (i != lastMonId + 3);
    } else {
        if (GetBattlerSide(battlerId) == B_SIDE_OPPONENT) {
            battlerIn1 = GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT);

            if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE)
                battlerIn2 = GetBattlerAtPosition(B_POSITION_OPPONENT_RIGHT);
            else
                battlerIn2 = battlerIn1;

            party = gEnemyParty;
        } else {
            // Check if attacker side has mon to switch into
            battlerIn1 = GetBattlerAtPosition(B_POSITION_PLAYER_LEFT);

            if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE)
                battlerIn2 = GetBattlerAtPosition(B_POSITION_PLAYER_RIGHT);
            else
                battlerIn2 = battlerIn1;

            party = gPlayerParty;
        }

        for (i = 0; i < PARTY_SIZE; i++) {
            if (GetMonData(&party[i], MON_DATA_HP) != 0 && GetMonData(&party[i], MON_DATA_SPECIES) != SPECIES_NONE && !GetMonData(&party[i], MON_DATA_IS_EGG) &&
                i != gBattlerPartyIndexes[battlerIn1] && i != gBattlerPartyIndexes[battlerIn2])
                break;
        }

        ret = (i != PARTY_SIZE);
    }
    return ret;
}

static void Cmd_jumpifcantswitch(void) {
    gActiveBattler = GetBattlerForBattleScript(gBattlescriptCurrInstr[1] & ~(SWITCH_IGNORE_ESCAPE_PREVENTION));

    if (!(gBattlescriptCurrInstr[1] & SWITCH_IGNORE_ESCAPE_PREVENTION) && !CanBattlerEscape(gActiveBattler)) {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 2);
    } else {
        if (CanBattlerSwitch(gActiveBattler))
            gBattlescriptCurrInstr += 6;
        else
            gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 2);
    }
}

// Opens the party screen to choose a new Pokémon to send out
// slotId is the Pokémon to replace
static void ChooseMonToSendOut(u8 slotId) {
    gBattleStruct->battlerPartyIndexes[gActiveBattler] = gBattlerPartyIndexes[gActiveBattler];
    gBattleStruct->monToSwitchIntoId[gActiveBattler] = PARTY_SIZE;
    gBattleStruct->field_93 &= ~(gBitTable[gActiveBattler]);

    BtlController_EmitChoosePokemon(0, PARTY_ACTION_SEND_OUT, slotId, ABILITY_NONE, gBattleStruct->battlerPartyOrders[gActiveBattler]);
    MarkBattlerForControllerExec(gActiveBattler);
}

static void Cmd_openpartyscreen(void) {
    u32 flags;
    u8 hitmarkerFaintBits;
    u8 battlerId;
    int mode = READ_FIRST_8_INC;
    const u8* jumpPtr = READ_PTR_INC;

    battlerId = 0;
    flags = 0;

    if (mode == BS_FAINTED_LINK_MULTIPLE_1) {
        if ((gBattleTypeFlags & (BATTLE_TYPE_DOUBLE | BATTLE_TYPE_MULTI)) != BATTLE_TYPE_DOUBLE) {
            for (gActiveBattler = 0; gActiveBattler < gBattlersCount; gActiveBattler++) {
                if (gHitMarker & HITMARKER_FAINTED(gActiveBattler)) {
                    if (HasNoMonsToSwitch(gActiveBattler, PARTY_SIZE, PARTY_SIZE)) {
                        gAbsentBattlerFlags |= gBitTable[gActiveBattler];
                        gHitMarker &= ~(HITMARKER_FAINTED(gActiveBattler));
                        BtlController_EmitLinkStandbyMsg(0, 2, FALSE);
                        MarkBattlerForControllerExec(gActiveBattler);
                    } else if (!gTurnStructs[gActiveBattler].flag40) {
                        ChooseMonToSendOut(PARTY_SIZE);
                        gTurnStructs[gActiveBattler].flag40 = 1;
                    }
                } else {
                    BtlController_EmitLinkStandbyMsg(0, 2, FALSE);
                    MarkBattlerForControllerExec(gActiveBattler);
                }
            }
        } else if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE) {
            u8 flag40_0, flag40_1, flag40_2, flag40_3;

            hitmarkerFaintBits = gHitMarker >> 28;

            if (gBitTable[0] & hitmarkerFaintBits) {
                gActiveBattler = 0;
                if (HasNoMonsToSwitch(0, PARTY_SIZE, PARTY_SIZE)) {
                    gAbsentBattlerFlags |= gBitTable[gActiveBattler];
                    gHitMarker &= ~(HITMARKER_FAINTED(gActiveBattler));
                    BtlController_EmitCantSwitch(0);
                    MarkBattlerForControllerExec(gActiveBattler);
                } else if (!gTurnStructs[gActiveBattler].flag40) {
                    ChooseMonToSendOut(gBattleStruct->monToSwitchIntoId[2]);
                    gTurnStructs[gActiveBattler].flag40 = 1;
                } else {
                    BtlController_EmitLinkStandbyMsg(0, 2, FALSE);
                    MarkBattlerForControllerExec(gActiveBattler);
                    flags |= 1;
                }
            }
            if (gBitTable[2] & hitmarkerFaintBits && !(gBitTable[0] & hitmarkerFaintBits)) {
                gActiveBattler = 2;
                if (HasNoMonsToSwitch(2, PARTY_SIZE, PARTY_SIZE)) {
                    gAbsentBattlerFlags |= gBitTable[gActiveBattler];
                    gHitMarker &= ~(HITMARKER_FAINTED(gActiveBattler));
                    BtlController_EmitCantSwitch(0);
                    MarkBattlerForControllerExec(gActiveBattler);
                } else if (!gTurnStructs[gActiveBattler].flag40) {
                    ChooseMonToSendOut(gBattleStruct->monToSwitchIntoId[0]);
                    gTurnStructs[gActiveBattler].flag40 = 1;
                } else if (!(flags & 1)) {
                    BtlController_EmitLinkStandbyMsg(0, 2, FALSE);
                    MarkBattlerForControllerExec(gActiveBattler);
                }
            }
            if (gBitTable[1] & hitmarkerFaintBits) {
                gActiveBattler = 1;
                if (HasNoMonsToSwitch(1, PARTY_SIZE, PARTY_SIZE)) {
                    gAbsentBattlerFlags |= gBitTable[gActiveBattler];
                    gHitMarker &= ~(HITMARKER_FAINTED(gActiveBattler));
                    BtlController_EmitCantSwitch(0);
                    MarkBattlerForControllerExec(gActiveBattler);
                } else if (!gTurnStructs[gActiveBattler].flag40) {
                    ChooseMonToSendOut(gBattleStruct->monToSwitchIntoId[3]);
                    gTurnStructs[gActiveBattler].flag40 = 1;
                } else {
                    BtlController_EmitLinkStandbyMsg(0, 2, FALSE);
                    MarkBattlerForControllerExec(gActiveBattler);
                    flags |= 2;
                }
            }
            if (gBitTable[3] & hitmarkerFaintBits && !(gBitTable[1] & hitmarkerFaintBits)) {
                gActiveBattler = 3;
                if (HasNoMonsToSwitch(3, PARTY_SIZE, PARTY_SIZE)) {
                    gAbsentBattlerFlags |= gBitTable[gActiveBattler];
                    gHitMarker &= ~(HITMARKER_FAINTED(gActiveBattler));
                    BtlController_EmitCantSwitch(0);
                    MarkBattlerForControllerExec(gActiveBattler);
                } else if (!gTurnStructs[gActiveBattler].flag40) {
                    ChooseMonToSendOut(gBattleStruct->monToSwitchIntoId[1]);
                    gTurnStructs[gActiveBattler].flag40 = 1;
                } else if (!(flags & 2)) {
                    BtlController_EmitLinkStandbyMsg(0, 2, FALSE);
                    MarkBattlerForControllerExec(gActiveBattler);
                }
            }

            flag40_0 = gTurnStructs[0].flag40;
            if (!flag40_0) {
                flag40_2 = gTurnStructs[2].flag40;
                if (!flag40_2 && hitmarkerFaintBits != 0) {
                    if (gAbsentBattlerFlags & gBitTable[0])
                        gActiveBattler = 2;
                    else
                        gActiveBattler = 0;

                    BtlController_EmitLinkStandbyMsg(0, 2, FALSE);
                    MarkBattlerForControllerExec(gActiveBattler);
                }
            }
            flag40_1 = gTurnStructs[1].flag40;
            if (!flag40_1) {
                flag40_3 = gTurnStructs[3].flag40;
                if (!flag40_3 && hitmarkerFaintBits != 0) {
                    if (gAbsentBattlerFlags & gBitTable[1])
                        gActiveBattler = 3;
                    else
                        gActiveBattler = 1;

                    BtlController_EmitLinkStandbyMsg(0, 2, FALSE);
                    MarkBattlerForControllerExec(gActiveBattler);
                }
            }
        }
    } else if (mode == BS_FAINTED_LINK_MULTIPLE_2) {
        if (!(gBattleTypeFlags & BATTLE_TYPE_MULTI)) {
            if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE) {
                hitmarkerFaintBits = gHitMarker >> 28;
                if (gBitTable[2] & hitmarkerFaintBits && gBitTable[0] & hitmarkerFaintBits) {
                    gActiveBattler = 2;
                    if (HasNoMonsToSwitch(2, gBattleResources->bufferB[0][1], PARTY_SIZE)) {
                        gAbsentBattlerFlags |= gBitTable[gActiveBattler];
                        gHitMarker &= ~(HITMARKER_FAINTED(gActiveBattler));
                        BtlController_EmitCantSwitch(0);
                        MarkBattlerForControllerExec(gActiveBattler);
                    } else if (!gTurnStructs[gActiveBattler].flag40) {
                        ChooseMonToSendOut(gBattleStruct->monToSwitchIntoId[0]);
                        gTurnStructs[gActiveBattler].flag40 = 1;
                    }
                }
                if (gBitTable[3] & hitmarkerFaintBits && hitmarkerFaintBits & gBitTable[1]) {
                    gActiveBattler = 3;
                    if (HasNoMonsToSwitch(3, gBattleResources->bufferB[1][1], PARTY_SIZE)) {
                        gAbsentBattlerFlags |= gBitTable[gActiveBattler];
                        gHitMarker &= ~(HITMARKER_FAINTED(gActiveBattler));
                        BtlController_EmitCantSwitch(0);
                        MarkBattlerForControllerExec(gActiveBattler);
                    } else if (!gTurnStructs[gActiveBattler].flag40) {
                        ChooseMonToSendOut(gBattleStruct->monToSwitchIntoId[1]);
                        gTurnStructs[gActiveBattler].flag40 = 1;
                    }
                }
            }
        }

        hitmarkerFaintBits = gHitMarker >> 28;

        gBattlerFainted = 0;
        while (!(gBitTable[gBattlerFainted] & hitmarkerFaintBits) && gBattlerFainted < gBattlersCount) gBattlerFainted++;

        if (gBattlerFainted == gBattlersCount) gBattlescriptCurrInstr = jumpPtr;
    } else {
        if (mode == BS_CHOOSE_FAINTED_MON)
            hitmarkerFaintBits = PARTY_ACTION_CHOOSE_FAINTED_MON;
        else if (mode & PARTY_SCREEN_OPTIONAL)
            hitmarkerFaintBits = PARTY_ACTION_CHOOSE_MON;  // Used here as the caseId for the EmitChoose function.
        else
            hitmarkerFaintBits = PARTY_ACTION_SEND_OUT;

        battlerId = GetBattlerForBattleScript(mode & ~(PARTY_SCREEN_OPTIONAL));
        if (gTurnStructs[battlerId].flag40) {
            return;
        }

        if (hitmarkerFaintBits == PARTY_ACTION_CHOOSE_FAINTED_MON && GetFirstFaintedPartyIndex(gBattlerAttacker) >= PARTY_SIZE) {
            gBattlescriptCurrInstr = jumpPtr;
            return;
        }

        if (hitmarkerFaintBits != PARTY_ACTION_CHOOSE_FAINTED_MON && HasNoMonsToSwitch(battlerId, PARTY_SIZE, PARTY_SIZE)) {
            gActiveBattler = battlerId;
            gAbsentBattlerFlags |= gBitTable[gActiveBattler];
            gHitMarker &= ~(HITMARKER_FAINTED(gActiveBattler));
            gBattlescriptCurrInstr = jumpPtr;
            return;
        }

        gActiveBattler = battlerId;
        *(gBattleStruct->battlerPartyIndexes + gActiveBattler) = gBattlerPartyIndexes[gActiveBattler];
        *(gBattleStruct->monToSwitchIntoId + gActiveBattler) = 6;
        gBattleStruct->field_93 &= ~(gBitTable[gActiveBattler]);

        BtlController_EmitChoosePokemon(
            0, hitmarkerFaintBits, *(gBattleStruct->monToSwitchIntoId + (gActiveBattler ^ 2)), ABILITY_NONE, gBattleStruct->battlerPartyOrders[gActiveBattler]);
        MarkBattlerForControllerExec(gActiveBattler);

        if (hitmarkerFaintBits != PARTY_ACTION_CHOOSE_FAINTED_MON && GetBattlerPosition(gActiveBattler) == 0 && gBattleResults.playerSwitchesCounter < 0xFF)
            gBattleResults.playerSwitchesCounter++;

        if (gBattleTypeFlags & BATTLE_TYPE_MULTI) {
            for (gActiveBattler = 0; gActiveBattler < gBattlersCount; gActiveBattler++) {
                if (gActiveBattler != battlerId) {
                    BtlController_EmitLinkStandbyMsg(0, 2, FALSE);
                    MarkBattlerForControllerExec(gActiveBattler);
                }
            }
        } else {
            gActiveBattler = GetBattlerAtPosition(GetBattlerPosition(battlerId) ^ BIT_SIDE);
            if (gAbsentBattlerFlags & gBitTable[gActiveBattler]) gActiveBattler ^= BIT_FLANK;

            if (gActiveBattler < gBattlersCount) {
                BtlController_EmitLinkStandbyMsg(0, 2, FALSE);
                MarkBattlerForControllerExec(gActiveBattler);
            }
        }
    }
}

static void Cmd_switchhandleorder(void) {
    s32 i;
    if (gBattleControllerExecFlags) return;

    gActiveBattler = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);

    switch (gBattlescriptCurrInstr[2]) {
        case 0:
            for (i = 0; i < gBattlersCount; i++) {
                if (gBattleResources->bufferB[i][0] == 0x22) {
                    *(gBattleStruct->monToSwitchIntoId + i) = gBattleResources->bufferB[i][1];
                    if (!(gBattleStruct->field_93 & gBitTable[i])) {
                        RecordedBattle_SetBattlerAction(i, gBattleResources->bufferB[i][1]);
                        gBattleStruct->field_93 |= gBitTable[i];
                    }
                }
            }
            break;
        case 1:
            if (!(gBattleTypeFlags & BATTLE_TYPE_MULTI)) SwitchPartyOrder(gActiveBattler);
            break;
        case 2:
            if (!(gBattleStruct->field_93 & gBitTable[gActiveBattler])) {
                RecordedBattle_SetBattlerAction(gActiveBattler, gBattleResources->bufferB[gActiveBattler][1]);
                gBattleStruct->field_93 |= gBitTable[gActiveBattler];
            }
            FALLTHROUGH
        case 3:
            gBattleCommunication[0] = gBattleResources->bufferB[gActiveBattler][1];
            *(gBattleStruct->monToSwitchIntoId + gActiveBattler) = gBattleResources->bufferB[gActiveBattler][1];

            if (gBattleTypeFlags & BATTLE_TYPE_LINK && gBattleTypeFlags & BATTLE_TYPE_MULTI) {
                *(gActiveBattler * 3 + (u8*)(gBattleStruct->battlerPartyOrders) + 0) &= 0xF;
                *(gActiveBattler * 3 + (u8*)(gBattleStruct->battlerPartyOrders) + 0) |= (gBattleResources->bufferB[gActiveBattler][2] & 0xF0);
                *(gActiveBattler * 3 + (u8*)(gBattleStruct->battlerPartyOrders) + 1) = gBattleResources->bufferB[gActiveBattler][3];

                *((gActiveBattler ^ BIT_FLANK) * 3 + (u8*)(gBattleStruct->battlerPartyOrders) + 0) &= (0xF0);
                *((gActiveBattler ^ BIT_FLANK) * 3 + (u8*)(gBattleStruct->battlerPartyOrders) + 0) |=
                    (gBattleResources->bufferB[gActiveBattler][2] & 0xF0) >> 4;
                *((gActiveBattler ^ BIT_FLANK) * 3 + (u8*)(gBattleStruct->battlerPartyOrders) + 2) = gBattleResources->bufferB[gActiveBattler][3];
            } else if (gBattleTypeFlags & BATTLE_TYPE_INGAME_PARTNER) {
                SwitchPartyOrderInGameMulti(gActiveBattler, *(gBattleStruct->monToSwitchIntoId + gActiveBattler));
            } else {
                SwitchPartyOrder(gActiveBattler);
            }

            PREPARE_SPECIES_BUFFER(gBattleTextBuff1, gBattleMons[gBattlerAttacker].species)
            PREPARE_MON_NICK_BUFFER(gBattleTextBuff2, gActiveBattler, gBattleResources->bufferB[gActiveBattler][1])
            break;
    }

    gBattlescriptCurrInstr += 3;
}

static void SetDmgHazardsBattlescript(u8 battlerId, u8 multistringId) {
    gStackBattler1 = battlerId;
    gBattleCommunication[MULTISTRING_CHOOSER] = multistringId;
    BattleScriptCall(BattleScript_DmgHazards);
}

static void Cmd_switchineffects(void) {
    s32 i;

    gBattlerAttacker = gActiveBattler = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);
    if (!IsBattlerAlive(gBattlerAttacker)) {
        gBattlescriptCurrInstr += 2;
        return;
    }
    UpdateSentPokesToOpponentValue(gActiveBattler);

    gHitMarker &= ~(HITMARKER_FAINTED(gActiveBattler));
    gTurnStructs[gActiveBattler].flag40 = 0;

    if (!IsBattlerAIControlled(gActiveBattler)) gBattleStruct->appearedInBattle |= gBitTable[gBattlerPartyIndexes[gActiveBattler]];

    // Neutralizing Gas announces itself before hazards
    if (!gFieldTimers.neutralizingGas && BattlerHasAbility(gActiveBattler, ABILITY_NEUTRALIZING_GAS, FALSE)) {
        for (i = 0; i < gBattlersCount; i++) {
            u16 abilities[TOTAL_ABILITY_COUNT];
            u8 j = 0;
            if (DoesBattlerHaveAbilityShield(i)) continue;
            ARRAY_COPY(abilities, gBattleMons[i].abilities)
            for (j = 0; j < TOTAL_ABILITY_COUNT; j++) {
                if (!IsPersistentOrUnsuppressableAbility(abilities[j])) abilities[j] = ABILITY_NONE;
            }
            UpdateAbilityStateIndices(i, abilities);
        }
        gFieldTimers.neutralizingGas = TRUE;
        gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_SWITCHIN_NEUTRALIZING_GAS;
        gBattlerAbility = gActiveBattler;
        gBattleScripting.abilityPopupOverwrite = ABILITY_NEUTRALIZING_GAS;
        BattleScriptCall(BattleScript_SwitchInAbilityMsgRet);
    }
    if (!gVolatileStructs[gActiveBattler].hazardDamaged) {
        gVolatileStructs[gActiveBattler].hazardDamaged = TRUE;
        gStackBattler1 = gActiveBattler;
        BattleScriptCall(BattleScript_ResolveAllHazards);
    } else {
        if (AbilityBattleEffects(ABILITYEFFECT_REACTIVE, 0, 0, ABILITY_BS_PUSH_CURSOR_AND_CALLBACK, 0)) return;

        if (TryPrimalReversion(gActiveBattler, TRUE)) return;

        while (gBattleScripting.abilityLoopCounter <= TOTAL_ABILITY_COUNT) {
            if (HandleSwitchInAbility(gBattleScripting.abilityLoopCounter++, gActiveBattler)) return;
        }

        if (ItemBattleEffects(ITEMEFFECT_ON_SWITCH_IN, gActiveBattler, FALSE) || AbilityBattleEffects(ABILITYEFFECT_TRACE2, 0, 0, 0, 0)) return;

        gSideStatuses[GetBattlerSide(gActiveBattler)] &=
            ~(SIDE_STATUS_SPIKES_DAMAGED | SIDE_STATUS_TOXIC_SPIKES_DAMAGED | SIDE_STATUS_STEALTH_ROCK_DAMAGED | SIDE_STATUS_STICKY_WEB_DAMAGED);

        for (i = 0; i < gBattlersCount; i++) {
            if (gBattlerByTurnOrder[i] == gActiveBattler) gActionsByTurnOrder[i] = B_ACTION_CANCEL_PARTNER;

            gBattleStruct->hpOnSwitchout[GetBattlerSide(i)] = gBattleMons[i].hp;
        }

        if (gBattlescriptCurrInstr[1] == 5) {
            u32 hitmarkerFaintBits = gHitMarker >> 28;

            gBattlerFainted++;
            while (1) {
                if (hitmarkerFaintBits & gBitTable[gBattlerFainted] && !(gAbsentBattlerFlags & gBitTable[gBattlerFainted])) break;
                if (gBattlerFainted >= gBattlersCount) break;
                gBattlerFainted++;
            }
        }
        gBattlescriptCurrInstr += 2;
    }
}

static void Cmd_trainerslidein(void) {
    gActiveBattler = GetBattlerAtPosition(gBattlescriptCurrInstr[1]);
    BtlController_EmitTrainerSlide(0);
    MarkBattlerForControllerExec(gActiveBattler);

    gBattlescriptCurrInstr += 2;
}

static void Cmd_playse(void) {
    gActiveBattler = gBattlerAttacker;
    BtlController_EmitPlaySE(0, T2_READ_16(gBattlescriptCurrInstr + 1));
    MarkBattlerForControllerExec(gActiveBattler);

    gBattlescriptCurrInstr += 3;
}

static void Cmd_fanfare(void) {
    gActiveBattler = gBattlerAttacker;
    BtlController_EmitPlayFanfareOrBGM(0, T2_READ_16(gBattlescriptCurrInstr + 1), FALSE);
    MarkBattlerForControllerExec(gActiveBattler);

    gBattlescriptCurrInstr += 3;
}

static void Cmd_playfaintcry(void) {
    gActiveBattler = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);
    BtlController_EmitFaintingCry(0);
    MarkBattlerForControllerExec(gActiveBattler);

    gBattlescriptCurrInstr += 2;
}

static void Cmd_endlinkbattle(void) {
    gActiveBattler = GetBattlerAtPosition(B_POSITION_PLAYER_LEFT);
    BtlController_EmitEndLinkBattle(0, gBattleOutcome);
    MarkBattlerForControllerExec(gActiveBattler);

    gBattlescriptCurrInstr += 1;
}

static void Cmd_returntoball(void) {
    gActiveBattler = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);
    BtlController_EmitReturnMonToBall(0, 1);
    MarkBattlerForControllerExec(gActiveBattler);

    gBattlescriptCurrInstr += 2;
}

static void Cmd_handlelearnnewmove(void) {
    const u8* jumpPtr1 = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    const u8* jumpPtr2 = T1_READ_PTR(gBattlescriptCurrInstr + 5);

    u16 learnMove = MonTryLearningNewMove(&gPlayerParty[gBattleStruct->expGetterMonId], gBattlescriptCurrInstr[9]);
    while (learnMove == MON_ALREADY_KNOWS_MOVE) learnMove = MonTryLearningNewMove(&gPlayerParty[gBattleStruct->expGetterMonId], FALSE);

    if (learnMove == 0) {
        gBattlescriptCurrInstr = jumpPtr2;
    } else if (learnMove == MON_HAS_MAX_MOVES) {
        gBattlescriptCurrInstr += 10;
    } else {
        gActiveBattler = GetBattlerAtPosition(B_POSITION_PLAYER_LEFT);

        if (gBattlerPartyIndexes[gActiveBattler] == gBattleStruct->expGetterMonId && !(gBattleMons[gActiveBattler].status2 & STATUS2_TRANSFORMED)) {
            GiveMoveToBattleMon(&gBattleMons[gActiveBattler], learnMove);
        }
        if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE) {
            gActiveBattler = GetBattlerAtPosition(B_POSITION_PLAYER_RIGHT);
            if (gBattlerPartyIndexes[gActiveBattler] == gBattleStruct->expGetterMonId && !(gBattleMons[gActiveBattler].status2 & STATUS2_TRANSFORMED)) {
                GiveMoveToBattleMon(&gBattleMons[gActiveBattler], learnMove);
            }
        }

        gBattlescriptCurrInstr = jumpPtr1;
    }
}

static void Cmd_yesnoboxlearnmove(void) {
    gActiveBattler = 0;

    switch (gBattleScripting.learnMoveState) {
        case 0:
            HandleBattleWindow(BATTLE_BOX_YES_NO_Y, 8, BATTLE_BOX_YES_NO_Y + BATTLE_BOX_YES_NO_WIDTH, 13, 0);
            BattlePutTextOnWindow(gText_BattleYesNoChoice, B_WIN_YESNO);
            gBattleScripting.learnMoveState++;
            gBattleCommunication[CURSOR_POSITION] = 0;
            BattleCreateYesNoCursorAt(0);
            break;
        case 1:
            if (JOY_NEW(DPAD_UP) && gBattleCommunication[CURSOR_POSITION] != 0) {
                PlaySE(SE_SELECT);
                BattleDestroyYesNoCursorAt(gBattleCommunication[CURSOR_POSITION]);
                gBattleCommunication[CURSOR_POSITION] = 0;
                BattleCreateYesNoCursorAt(0);
            }
            if (JOY_NEW(DPAD_DOWN) && gBattleCommunication[CURSOR_POSITION] == 0) {
                PlaySE(SE_SELECT);
                BattleDestroyYesNoCursorAt(gBattleCommunication[CURSOR_POSITION]);
                gBattleCommunication[CURSOR_POSITION] = 1;
                BattleCreateYesNoCursorAt(1);
            }
            if (JOY_NEW(A_BUTTON)) {
                PlaySE(SE_SELECT);
                if (gBattleCommunication[1] == 0) {
                    HandleBattleWindow(0x18, 0x8, 0x1D, 0xD, WINDOW_CLEAR);
                    BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 0x10, RGB_BLACK);
                    gBattleScripting.learnMoveState++;
                } else {
                    gBattleScripting.learnMoveState = 5;
                }
            } else if (JOY_NEW(B_BUTTON)) {
                PlaySE(SE_SELECT);
                gBattleScripting.learnMoveState = 5;
            }
            break;
        case 2:
            if (!gPaletteFade.active) {
                FreeAllWindowBuffers();
                ShowSelectMovePokemonSummaryScreen(
                    gPlayerParty, gBattleStruct->expGetterMonId, gPlayerPartyCount - 1, ReshowBattleScreenAfterMenu, gMoveToLearn);
                gBattleScripting.learnMoveState++;
            }
            break;
        case 3:
            if (!gPaletteFade.active && gMain.callback2 == BattleMainCB2) {
                gBattleScripting.learnMoveState++;
            }
            break;
        case 4:
            if (!gPaletteFade.active && gMain.callback2 == BattleMainCB2) {
                u8 movePosition = GetMoveSlotToReplace();
                if (movePosition == MAX_MON_MOVES) {
                    gBattleScripting.learnMoveState = 5;
                } else {
                    u16 moveId = GetMonData(&gPlayerParty[gBattleStruct->expGetterMonId], MON_DATA_MOVE1 + movePosition);
                    gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);

                    PREPARE_MOVE_BUFFER(gBattleTextBuff2, moveId)

                    RemoveMonPPBonus(&gPlayerParty[gBattleStruct->expGetterMonId], movePosition);
                    SetMonMoveSlot(&gPlayerParty[gBattleStruct->expGetterMonId], gMoveToLearn, movePosition);

                    if (gBattlerPartyIndexes[0] == gBattleStruct->expGetterMonId && !(gBattleMons[0].status2 & STATUS2_TRANSFORMED) &&
                        !(gVolatileStructs[0].mimickedMoves & gBitTable[movePosition])) {
                        RemoveBattleMonPPBonus(&gBattleMons[0], movePosition);
                        SetBattleMonMoveSlot(&gBattleMons[0], gMoveToLearn, movePosition);
                    }
                    if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE && gBattlerPartyIndexes[2] == gBattleStruct->expGetterMonId &&
                        !(gBattleMons[2].status2 & STATUS2_TRANSFORMED) && !(gVolatileStructs[2].mimickedMoves & gBitTable[movePosition])) {
                        RemoveBattleMonPPBonus(&gBattleMons[2], movePosition);
                        SetBattleMonMoveSlot(&gBattleMons[2], gMoveToLearn, movePosition);
                    }
                }
            }
            break;
        case 5:
            HandleBattleWindow(0x18, 8, 0x1D, 0xD, WINDOW_CLEAR);
            gBattlescriptCurrInstr += 5;
            break;
        case 6:
            if (gBattleControllerExecFlags == 0) {
                gBattleScripting.learnMoveState = 2;
            }
            break;
    }
}

static void Cmd_yesnoboxstoplearningmove(void) {
    switch (gBattleScripting.learnMoveState) {
        case 0:
            HandleBattleWindow(BATTLE_BOX_YES_NO_Y, 8, BATTLE_BOX_YES_NO_Y + BATTLE_BOX_YES_NO_WIDTH, 13, 0);
            BattlePutTextOnWindow(gText_BattleYesNoChoice, B_WIN_YESNO);
            gBattleScripting.learnMoveState++;
            gBattleCommunication[CURSOR_POSITION] = 0;
            BattleCreateYesNoCursorAt(0);
            break;
        case 1:
            if (JOY_NEW(DPAD_UP) && gBattleCommunication[CURSOR_POSITION] != 0) {
                PlaySE(SE_SELECT);
                BattleDestroyYesNoCursorAt(gBattleCommunication[CURSOR_POSITION]);
                gBattleCommunication[CURSOR_POSITION] = 0;
                BattleCreateYesNoCursorAt(0);
            }
            if (JOY_NEW(DPAD_DOWN) && gBattleCommunication[CURSOR_POSITION] == 0) {
                PlaySE(SE_SELECT);
                BattleDestroyYesNoCursorAt(gBattleCommunication[CURSOR_POSITION]);
                gBattleCommunication[CURSOR_POSITION] = 1;
                BattleCreateYesNoCursorAt(1);
            }
            if (JOY_NEW(A_BUTTON)) {
                PlaySE(SE_SELECT);

                if (gBattleCommunication[1] != 0)
                    gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
                else
                    gBattlescriptCurrInstr += 5;

                HandleBattleWindow(0x18, 0x8, 0x1D, 0xD, WINDOW_CLEAR);
            } else if (JOY_NEW(B_BUTTON)) {
                PlaySE(SE_SELECT);
                gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
                HandleBattleWindow(0x18, 0x8, 0x1D, 0xD, WINDOW_CLEAR);
            }
            break;
    }
}

static void Cmd_hitanimation(void) {
    gActiveBattler = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);

    if (gMoveResultFlags & MOVE_RESULT_NO_EFFECT) {
        gBattlescriptCurrInstr += 2;
    } else if (!(gHitMarker & HITMARKER_IGNORE_SUBSTITUTE) || !(DoesSubstituteBlockMove(gBattlerAttacker, gActiveBattler, gCurrentMove)) ||
               gVolatileStructs[gActiveBattler].substituteHP == 0) {
        BtlController_EmitHitAnimation(0);
        MarkBattlerForControllerExec(gActiveBattler);
        gBattlescriptCurrInstr += 2;
    } else {
        gBattlescriptCurrInstr += 2;
    }
}

static u32 GetTrainerMoneyToGive(u16 trainerId) {
    u32 i = 0;
    s32 lastMonLevel = 0;
    u32 moneyReward;

    if (trainerId == TRAINER_SECRET_BASE) {
        moneyReward = 20 * gBattleResources->secretBase->party.levels[0] * gBattleStruct->moneyMultiplier;
    } else {
        switch (gTrainers[trainerId].partyFlags) {
            case 0: {
                const struct TrainerMonNoItemDefaultMoves* party = gTrainers[trainerId].party.NoItemDefaultMoves;
                lastMonLevel = party[gTrainers[trainerId].partySize - 1].lvl;
            } break;
            case F_TRAINER_PARTY_CUSTOM_MOVESET: {
                const struct TrainerMonNoItemCustomMoves* party = gTrainers[trainerId].party.NoItemCustomMoves;
                lastMonLevel = party[gTrainers[trainerId].partySize - 1].lvl;
            } break;
            case F_TRAINER_PARTY_HELD_ITEM: {
                const struct TrainerMonItemDefaultMoves* party = gTrainers[trainerId].party.ItemDefaultMoves;
                lastMonLevel = party[gTrainers[trainerId].partySize - 1].lvl;
            } break;
            case F_TRAINER_PARTY_CUSTOM_MOVESET | F_TRAINER_PARTY_HELD_ITEM: {
                const struct TrainerMonItemCustomMoves* party = gTrainers[trainerId].party.ItemCustomMoves;
                lastMonLevel = GetHighestLevelInPlayerParty();

                if (lastMonLevel + party[gTrainers[trainerId].partySize - 1].lvl < 1) {
                    lastMonLevel = 1;
                } else if (lastMonLevel + party[gTrainers[trainerId].partySize - 1].lvl > 100) {
                    lastMonLevel = 100;
                } else {
                    lastMonLevel += party[gTrainers[trainerId].partySize - 1].lvl;
                }
            } break;
        }

        for (; gTrainerMoneyTable[i].classId != 0xFF; i++) {
            if (gTrainerMoneyTable[i].classId == gTrainers[trainerId].trainerClass) break;
        }

        if (gBattleTypeFlags & BATTLE_TYPE_TWO_OPPONENTS)
            moneyReward = 8 * lastMonLevel * gBattleStruct->moneyMultiplier * gTrainerMoneyTable[i].value;
        else if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE)
            moneyReward = 8 * lastMonLevel * gBattleStruct->moneyMultiplier * 2 * gTrainerMoneyTable[i].value;
        else
            moneyReward = 8 * lastMonLevel * gBattleStruct->moneyMultiplier * gTrainerMoneyTable[i].value;
    }

    return moneyReward;
}

static void Cmd_getmoneyreward(void) {
    if (VarGet(VAR_TRAINER_PRIZE_BP) == 0) {
        // if the battle is lost, the reward is 0, don't try to skip the reward, the game don't like that at all
        if (gBattleOutcome == B_OUTCOME_FORFEITED || gBattleOutcome == B_OUTCOME_LOST) {
            VarSet(VAR_TRAINER_PRIZE_BP, 0);
        } else {
            // if two opponnents are present give twice the BP amount
            if (gTrainerBattleOpponent_B) {
                VarSet(VAR_TRAINER_PRIZE_BP, DEFAULT_BP_GAIN_PER_TRAINER * 2);
            } else {
                VarSet(VAR_TRAINER_PRIZE_BP, DEFAULT_BP_GAIN_PER_TRAINER);
            }
        }

        gSpecialVar_0x8004 = VarGet(VAR_TRAINER_PRIZE_BP);
    }
    GiveFrontierBattlePoints();
    PREPARE_WORD_NUMBER_BUFFER(gBattleTextBuff1, 3, VarGet(VAR_TRAINER_PRIZE_BP));
    VarSet(VAR_TRAINER_PRIZE_BP, 0);
    gBattlescriptCurrInstr++;
}

static void Cmd_unknown_5E(void) {
    gActiveBattler = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);

    switch (gBattleCommunication[0]) {
        case 0:
            BtlController_EmitGetMonData(0, REQUEST_ALL_BATTLE, 0);
            MarkBattlerForControllerExec(gActiveBattler);
            gBattleCommunication[0]++;
            break;
        case 1:
            if (gBattleControllerExecFlags == 0) {
                struct BattlePokemon* bufferPoke = (struct BattlePokemon*)&gBattleResources->bufferB[gActiveBattler][4];
                ARRAY_COPY(gBattleMons[gActiveBattler].moves, bufferPoke->moves)
                ARRAY_COPY(gBattleMons[gActiveBattler].pp, bufferPoke->pp)
                gBattlescriptCurrInstr += 2;
            }
            break;
    }
}

static void Cmd_swapattackerwithtarget(void) {
    gActiveBattler = gBattlerAttacker;
    gBattlerAttacker = gBattlerTarget;
    gBattlerTarget = gActiveBattler;

    if (gHitMarker & HITMARKER_SWAP_ATTACKER_TARGET)
        gHitMarker &= ~(HITMARKER_SWAP_ATTACKER_TARGET);
    else
        gHitMarker |= HITMARKER_SWAP_ATTACKER_TARGET;

    gBattlescriptCurrInstr++;
}

static void Cmd_incrementgamestat(void) {
    if (GetBattlerSide(gBattlerAttacker) == B_SIDE_PLAYER) IncrementGameStat(gBattlescriptCurrInstr[1]);

    gBattlescriptCurrInstr += 2;
}

static void Cmd_drawpartystatussummary(void) {
    s32 i;
    struct Pokemon* party;
    struct HpAndStatus hpStatuses[PARTY_SIZE];

    if (gBattleControllerExecFlags) return;

    gActiveBattler = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);

    if (GetBattlerSide(gActiveBattler) == B_SIDE_PLAYER)
        party = gPlayerParty;
    else
        party = gEnemyParty;

    for (i = 0; i < PARTY_SIZE; i++) {
        if (GetMonData(&party[i], MON_DATA_SPECIES2) == SPECIES_NONE || GetMonData(&party[i], MON_DATA_SPECIES2) == SPECIES_EGG) {
            hpStatuses[i].hp = 0xFFFF;
            hpStatuses[i].status = 0;
        } else {
            hpStatuses[i].hp = GetMonData(&party[i], MON_DATA_HP);
            hpStatuses[i].status = GetMonData(&party[i], MON_DATA_STATUS);
        }
    }

    BtlController_EmitDrawPartyStatusSummary(0, hpStatuses, 1);
    MarkBattlerForControllerExec(gActiveBattler);

    gBattlescriptCurrInstr += 2;
}

static void Cmd_hidepartystatussummary(void) {
    gActiveBattler = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);
    BtlController_EmitHidePartyStatusSummary(0);
    MarkBattlerForControllerExec(gActiveBattler);

    gBattlescriptCurrInstr += 2;
}

static void Cmd_jumptocalledmove(void) {
    if (gBattlescriptCurrInstr[1])
        gCurrentMove = gCalledMove;
    else
        gChosenMove = gCurrentMove = gCalledMove;

    gBattlescriptCurrInstr = gBattleScriptsForMoveEffects[gBattleMoves[gCurrentMove].effect];
}

static void Cmd_statusanimation(void) {
    if (gBattleControllerExecFlags == 0) {
        gActiveBattler = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);
        if (!(gStatuses3[gActiveBattler] & STATUS3_SEMI_INVULNERABLE) && gVolatileStructs[gActiveBattler].substituteHP == 0 &&
            !(gHitMarker & HITMARKER_NO_ANIMATIONS)) {
            BtlController_EmitStatusAnimation(0, FALSE, gBattleMons[gActiveBattler].status1);
            MarkBattlerForControllerExec(gActiveBattler);
        }
        gBattlescriptCurrInstr += 2;
    }
}

static void Cmd_status2animation(void) {
    u32 wantedToAnimate;

    if (gBattleControllerExecFlags == 0) {
        gActiveBattler = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);
        wantedToAnimate = T1_READ_32(gBattlescriptCurrInstr + 2);
        if (!(gStatuses3[gActiveBattler] & STATUS3_SEMI_INVULNERABLE) && gVolatileStructs[gActiveBattler].substituteHP == 0 &&
            !(gHitMarker & HITMARKER_NO_ANIMATIONS)) {
            BtlController_EmitStatusAnimation(0, TRUE, gBattleMons[gActiveBattler].status2 & wantedToAnimate);
            MarkBattlerForControllerExec(gActiveBattler);
        }
        gBattlescriptCurrInstr += 6;
    }
}

static void Cmd_chosenstatusanimation(void) {
    u32 wantedStatus;

    if (gBattleControllerExecFlags == 0) {
        gActiveBattler = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);
        wantedStatus = T1_READ_32(gBattlescriptCurrInstr + 3);
        if (!(gStatuses3[gActiveBattler] & STATUS3_SEMI_INVULNERABLE) && gVolatileStructs[gActiveBattler].substituteHP == 0 &&
            !(gHitMarker & HITMARKER_NO_ANIMATIONS)) {
            BtlController_EmitStatusAnimation(0, gBattlescriptCurrInstr[2], wantedStatus);
            MarkBattlerForControllerExec(gActiveBattler);
        }
        gBattlescriptCurrInstr += 7;
    }
}

static void Cmd_yesnobox(void) {
    switch (gBattleCommunication[0]) {
        case 0:
            HandleBattleWindow(BATTLE_BOX_YES_NO_Y, 8, BATTLE_BOX_YES_NO_Y + BATTLE_BOX_YES_NO_WIDTH, 13, 0);
            BattlePutTextOnWindow(gText_BattleYesNoChoice, B_WIN_YESNO);
            gBattleCommunication[0]++;
            gBattleCommunication[CURSOR_POSITION] = 0;
            BattleCreateYesNoCursorAt(0);
            break;
        case 1:
            if (JOY_NEW(DPAD_UP) && gBattleCommunication[CURSOR_POSITION] != 0) {
                PlaySE(SE_SELECT);
                BattleDestroyYesNoCursorAt(gBattleCommunication[CURSOR_POSITION]);
                gBattleCommunication[CURSOR_POSITION] = 0;
                BattleCreateYesNoCursorAt(0);
            }
            if (JOY_NEW(DPAD_DOWN) && gBattleCommunication[CURSOR_POSITION] == 0) {
                PlaySE(SE_SELECT);
                BattleDestroyYesNoCursorAt(gBattleCommunication[CURSOR_POSITION]);
                gBattleCommunication[CURSOR_POSITION] = 1;
                BattleCreateYesNoCursorAt(1);
            }
            if (JOY_NEW(B_BUTTON)) {
                gBattleCommunication[CURSOR_POSITION] = 1;
                PlaySE(SE_SELECT);
                HandleBattleWindow(0x18, 8, 0x1D, 0xD, WINDOW_CLEAR);
                gBattlescriptCurrInstr++;
            } else if (JOY_NEW(A_BUTTON)) {
                PlaySE(SE_SELECT);
                HandleBattleWindow(0x18, 8, 0x1D, 0xD, WINDOW_CLEAR);
                gBattlescriptCurrInstr++;
            }
            break;
    }
}

static void Cmd_cancelallactions(void) {
    s32 i;

    for (i = 0; i < gBattlersCount; i++) gActionsByTurnOrder[i] = B_ACTION_CANCEL_PARTNER;

    gBattlescriptCurrInstr++;
}

static void Cmd_setgravity(void) {
    if (gFieldStatuses & STATUS_FIELD_GRAVITY) {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    } else {
        gFieldStatuses |= STATUS_FIELD_GRAVITY;
        gFieldTimers.started.gravity = TRUE;
        gFieldTimers.gravityTimer = GRAVITY_DURATION;
        gBattlescriptCurrInstr += 5;
    }
}

static void TryCheekPouch(u32 battlerId, u32 itemId) {
    int ability;
    if (ItemId_GetPocket(itemId) == POCKET_BERRIES &&
        ((ability = BattlerHasAbility(battlerId, ABILITY_GLUTTONY, FALSE)) || (ability = BattlerHasAbility(battlerId, ABILITY_SUGAR_RUSH, FALSE))) &&
        CanBattlerHeal(battlerId) && gBattleStruct->ateBerry[GetBattlerSide(battlerId)] & gBitTable[gBattlerPartyIndexes[battlerId]] &&
        !BATTLER_MAX_HP(battlerId)) {
        gBattleScripting.abilityPopupOverwrite = ability;
        gBattleMoveDamage = gBattleMons[battlerId].maxHP / 3;
        if (gBattleMoveDamage == 0) gBattleMoveDamage = 1;
        gBattleMoveDamage *= -1;
        gBattlerAbility = battlerId;
        BattleScriptCall(BattleScript_CheekPouchActivates);
    }
}

void SetCudChew(u32 battlerId, u32 itemId) {
    if (ItemId_GetPocket(itemId) == POCKET_BERRIES && BATTLER_HAS_ABILITY(battlerId, ABILITY_CUD_CHEW) &&
        gBattleStruct->ateBerry[GetBattlerSide(battlerId)] & gBitTable[gBattlerPartyIndexes[battlerId]]) {
        CudChewState state = GetAbilityStateAs(battlerId, ABILITY_CUD_CHEW).cudChewState;
        if (state.activating) {
            SetAbilityStateAs(battlerId, ABILITY_CUD_CHEW, (AbilityStates){.cudChewState = {0}});
        } else if (!state.itemId) {
            SetAbilityStateAs(battlerId, ABILITY_CUD_CHEW, (AbilityStates){.cudChewState = {.itemId = itemId, .setThisTurn = TRUE}});
        }
    }
}

static void Cmd_removeitem(void) {
    u16 itemId = 0;

    gActiveBattler = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);
    itemId = gBattleMons[gActiveBattler].item;

    // Popped Air Balloon cannot be restored by any means.
    if (GetBattlerHoldEffect(gActiveBattler, TRUE) != HOLD_EFFECT_AIR_BALLOON)
        gBattleStruct->usedHeldItems[gBattlerPartyIndexes[gActiveBattler]][GetBattlerSide(gActiveBattler)] = itemId;  // Remember if switched out

    gLastUsedItem = UpdateBattlerItem(gActiveBattler, ITEM_NONE);
    gBattlescriptCurrInstr += 2;
    TryCheekPouch(gActiveBattler, itemId);
}

static void Cmd_atknameinbuff1(void) {
    PREPARE_MON_NICK_BUFFER(gBattleTextBuff1, gBattlerAttacker, gBattlerPartyIndexes[gBattlerAttacker]);

    gBattlescriptCurrInstr++;
}

static void Cmd_drawlvlupbox(void) {
    if (gBattleScripting.drawlvlupboxState == 0) {
        if (IsMonGettingExpSentOut())
            gBattleScripting.drawlvlupboxState = 3;
        else
            gBattleScripting.drawlvlupboxState = 1;
    }

    switch (gBattleScripting.drawlvlupboxState) {
        case 1:
            gBattle_BG2_Y = 0x60;
            SetBgAttribute(2, BG_ATTR_PRIORITY, 0);
            ShowBg(2);
            sub_804F17C();
            gBattleScripting.drawlvlupboxState = 2;
            break;
        case 2:
            if (!sub_804F1CC()) gBattleScripting.drawlvlupboxState = 3;
            break;
        case 3:
            gBattle_BG1_X = 0;
            gBattle_BG1_Y = 0x100;
            SetBgAttribute(0, BG_ATTR_PRIORITY, 1);
            SetBgAttribute(1, BG_ATTR_PRIORITY, 0);
            ShowBg(0);
            ShowBg(1);
            HandleBattleWindow(0x12, 7, 0x1D, 0x13, WINDOW_x80);
            gBattleScripting.drawlvlupboxState = 4;
            break;
        case 4:
            DrawLevelUpWindow1();
            PutWindowTilemap(13);
            CopyWindowToVram(13, 3);
            gBattleScripting.drawlvlupboxState++;
            break;
        case 5:
        case 7:
            if (!IsDma3ManagerBusyWithBgCopy()) {
                gBattle_BG1_Y = 0;
                gBattleScripting.drawlvlupboxState++;
            }
            break;
        case 6:
            if (gMain.newKeys != 0) {
                PlaySE(SE_SELECT);
                DrawLevelUpWindow2();
                CopyWindowToVram(13, 2);
                gBattleScripting.drawlvlupboxState++;
            }
            break;
        case 8:
            if (gMain.newKeys != 0) {
                PlaySE(SE_SELECT);
                HandleBattleWindow(0x12, 7, 0x1D, 0x13, WINDOW_x80 | WINDOW_CLEAR);
                gBattleScripting.drawlvlupboxState++;
            }
            break;
        case 9:
            if (!sub_804F344()) {
                ClearWindowTilemap(14);
                CopyWindowToVram(14, 1);

                ClearWindowTilemap(13);
                CopyWindowToVram(13, 1);

                SetBgAttribute(2, BG_ATTR_PRIORITY, 2);
                ShowBg(2);

                gBattleScripting.drawlvlupboxState = 10;
            }
            break;
        case 10:
            if (!IsDma3ManagerBusyWithBgCopy()) {
                SetBgAttribute(0, BG_ATTR_PRIORITY, 0);
                SetBgAttribute(1, BG_ATTR_PRIORITY, 1);
                ShowBg(0);
                ShowBg(1);
                gBattlescriptCurrInstr++;
            }
            break;
    }
}

static void DrawLevelUpWindow1(void) {
    u16 currStats[NUM_STATS];

    GetMonLevelUpWindowStats(&gPlayerParty[gBattleStruct->expGetterMonId], currStats);
    DrawLevelUpWindowPg1(0xD, gBattleResources->beforeLvlUp->stats, currStats, TEXT_DYNAMIC_COLOR_5, TEXT_DYNAMIC_COLOR_4, TEXT_DYNAMIC_COLOR_6);
}

static void DrawLevelUpWindow2(void) {
    u16 currStats[NUM_STATS];

    GetMonLevelUpWindowStats(&gPlayerParty[gBattleStruct->expGetterMonId], currStats);
    DrawLevelUpWindowPg2(0xD, currStats, TEXT_DYNAMIC_COLOR_5, TEXT_DYNAMIC_COLOR_4, TEXT_DYNAMIC_COLOR_6);
}

static void sub_804F17C(void) {
    gBattle_BG2_Y = 0;
    gBattle_BG2_X = 0x1A0;

    LoadPalette(sUnknown_0831C2C8, 0x60, 0x20);
    CopyToWindowPixelBuffer(14, sUnknown_0831C2E8, 0, 0);
    PutWindowTilemap(14);
    CopyWindowToVram(14, 3);

    PutMonIconOnLvlUpBox();
}

static bool8 sub_804F1CC(void) {
    if (IsDma3ManagerBusyWithBgCopy()) return TRUE;

    if (gBattle_BG2_X == 0x200) return FALSE;

    if (gBattle_BG2_X == 0x1A0) PutLevelAndGenderOnLvlUpBox();

    gBattle_BG2_X += 8;
    if (gBattle_BG2_X >= 0x200) gBattle_BG2_X = 0x200;

    return (gBattle_BG2_X != 0x200);
}

static void PutLevelAndGenderOnLvlUpBox(void) {
    u16 monLevel;
    u8 monGender;
    struct TextPrinterTemplate printerTemplate;
    u8* txtPtr;
    u32 var;

    monLevel = GetMonData(&gPlayerParty[gBattleStruct->expGetterMonId], MON_DATA_LEVEL);
    monGender = GetMonGender(&gPlayerParty[gBattleStruct->expGetterMonId]);
    GetMonNickname(&gPlayerParty[gBattleStruct->expGetterMonId], gStringVar4);

    printerTemplate.currentChar = gStringVar4;
    printerTemplate.windowId = 14;
    printerTemplate.fontId = 0;
    printerTemplate.x = 32;
    printerTemplate.y = 0;
    printerTemplate.currentX = 32;
    printerTemplate.currentY = 0;
    printerTemplate.letterSpacing = 0;
    printerTemplate.lineSpacing = 0;
    printerTemplate.unk = 0;
    printerTemplate.fgColor = TEXT_COLOR_WHITE;
    printerTemplate.bgColor = TEXT_COLOR_TRANSPARENT;
    printerTemplate.shadowColor = TEXT_COLOR_DARK_GRAY;

    AddTextPrinter(&printerTemplate, 0xFF, NULL);

    txtPtr = gStringVar4;
    *(txtPtr)++ = CHAR_EXTRA_SYMBOL;
    *(txtPtr)++ = CHAR_LV_2;

    var = (u32)(txtPtr);
    txtPtr = ConvertIntToDecimalStringN(txtPtr, monLevel, STR_CONV_MODE_LEFT_ALIGN, 3);
    var = (u32)(txtPtr)-var;
    txtPtr = StringFill(txtPtr, CHAR_GENDERLESS, 4 - var);

    if (monGender != MON_GENDERLESS) {
        if (monGender == MON_MALE) {
            txtPtr = WriteColorChangeControlCode(txtPtr, 0, 0xC);
            txtPtr = WriteColorChangeControlCode(txtPtr, 1, 0xD);
            *(txtPtr++) = CHAR_MALE;
        } else {
            txtPtr = WriteColorChangeControlCode(txtPtr, 0, 0xE);
            txtPtr = WriteColorChangeControlCode(txtPtr, 1, 0xF);
            *(txtPtr++) = CHAR_FEMALE;
        }
        *(txtPtr++) = EOS;
    }

    printerTemplate.y = 10;
    printerTemplate.currentY = 10;
    AddTextPrinter(&printerTemplate, 0xFF, NULL);

    CopyWindowToVram(14, 2);
}

static bool8 sub_804F344(void) {
    if (gBattle_BG2_X == 0x1A0) return FALSE;

    if (gBattle_BG2_X - 16 < 0x1A0)
        gBattle_BG2_X = 0x1A0;
    else
        gBattle_BG2_X -= 16;

    return (gBattle_BG2_X != 0x1A0);
}

#define sDestroy data[0]
#define sSavedLvlUpBoxXPosition data[1]

static void PutMonIconOnLvlUpBox(void) {
    u8 spriteId;
    const u16* iconPal;
    struct SpriteSheet iconSheet;
    struct SpritePalette iconPalSheet;

    u16 species = GetMonData(&gPlayerParty[gBattleStruct->expGetterMonId], MON_DATA_SPECIES);
    u32 personality = GetMonData(&gPlayerParty[gBattleStruct->expGetterMonId], MON_DATA_PERSONALITY);

    const u8* iconPtr = GetMonIconPtr(species, personality);
    iconSheet.data = iconPtr;
    iconSheet.size = 0x200;
    iconSheet.tag = MON_ICON_LVLUP_BOX_TAG;

    iconPal = GetValidMonIconPalettePtr(species);
    iconPalSheet.data = iconPal;
    iconPalSheet.tag = MON_ICON_LVLUP_BOX_TAG;

    LoadSpriteSheet(&iconSheet);
    LoadSpritePalette(&iconPalSheet);

    spriteId = CreateSprite(&sSpriteTemplate_MonIconOnLvlUpBox, 256, 10, 0);
    gSprites[spriteId].sDestroy = FALSE;
    gSprites[spriteId].sSavedLvlUpBoxXPosition = gBattle_BG2_X;
}

static void SpriteCB_MonIconOnLvlUpBox(struct Sprite* sprite) {
    sprite->x2 = sprite->sSavedLvlUpBoxXPosition - gBattle_BG2_X;

    if (sprite->x2 != 0) {
        sprite->sDestroy = TRUE;
    } else if (sprite->sDestroy) {
        DestroySprite(sprite);
        FreeSpriteTilesByTag(MON_ICON_LVLUP_BOX_TAG);
        FreeSpritePaletteByTag(MON_ICON_LVLUP_BOX_TAG);
    }
}

#undef sDestroy
#undef sSavedLvlUpBoxXPosition

static bool32 IsMonGettingExpSentOut(void) {
    if (gBattlerPartyIndexes[0] == gBattleStruct->expGetterMonId) return TRUE;
    if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE && gBattlerPartyIndexes[2] == gBattleStruct->expGetterMonId) return TRUE;

    return FALSE;
}

static void Cmd_resetsentmonsvalue(void) {
    ResetSentPokesToOpponentValue();
    gBattlescriptCurrInstr++;
}

static void Cmd_setatktoplayer0(void) {
    gBattlerAttacker = GetBattlerAtPosition(B_POSITION_PLAYER_LEFT);
    gBattlescriptCurrInstr++;
}

static void Cmd_makevisible(void) {
    if (gBattleControllerExecFlags) return;

    gActiveBattler = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);
    BtlController_EmitSpriteInvisibility(0, FALSE);
    MarkBattlerForControllerExec(gActiveBattler);

    gBattlescriptCurrInstr += 2;
}

static void Cmd_recordability(void) {
    // u8 battler = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);
    gBattlescriptCurrInstr += 2;
}

void BufferMoveToLearnIntoBattleTextBuff2(void) { PREPARE_MOVE_BUFFER(gBattleTextBuff2, gMoveToLearn); }

static void Cmd_buffermovetolearn(void) {
    BufferMoveToLearnIntoBattleTextBuff2();
    gBattlescriptCurrInstr++;
}

static void Cmd_jumpifplayerran(void) {
    if (TryRunFromBattle(gBattlerFainted))
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    else
        gBattlescriptCurrInstr += 5;
}

static void Cmd_hpthresholds(void) {
    u8 opposingBank;
    s32 result;

    if (!(gBattleTypeFlags & BATTLE_TYPE_DOUBLE)) {
        gActiveBattler = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);
        opposingBank = gActiveBattler ^ BIT_SIDE;

        result = gBattleMons[opposingBank].hp * 100 / gBattleMons[opposingBank].maxHP;
        if (result == 0) result = 1;

        if (result > 69 || !gBattleMons[opposingBank].hp)
            gBattleStruct->hpScale = 0;
        else if (result > 39)
            gBattleStruct->hpScale = 1;
        else if (result > 9)
            gBattleStruct->hpScale = 2;
        else
            gBattleStruct->hpScale = 3;
    }

    gBattlescriptCurrInstr += 2;
}

static void Cmd_hpthresholds2(void) {
    u8 opposingBank;
    s32 result;
    u8 hpSwitchout;

    if (!(gBattleTypeFlags & BATTLE_TYPE_DOUBLE)) {
        gActiveBattler = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);
        opposingBank = gActiveBattler ^ BIT_SIDE;
        hpSwitchout = *(gBattleStruct->hpOnSwitchout + GetBattlerSide(opposingBank));
        result = (hpSwitchout - gBattleMons[opposingBank].hp) * 100 / hpSwitchout;

        if (gBattleMons[opposingBank].hp >= hpSwitchout)
            gBattleStruct->hpScale = 0;
        else if (result <= 29)
            gBattleStruct->hpScale = 1;
        else if (result <= 69)
            gBattleStruct->hpScale = 2;
        else
            gBattleStruct->hpScale = 3;
    }

    gBattlescriptCurrInstr += 2;
}

static void Cmd_useitemonopponent(void) {
    gBattlerInMenuId = gBattlerAttacker;
    PokemonUseItemEffects(&gEnemyParty[gBattlerPartyIndexes[gBattlerAttacker]], gLastUsedItem, gBattlerPartyIndexes[gBattlerAttacker], 0, TRUE);
    gBattlescriptCurrInstr += 1;
}

bool32 HasAttackerFaintedTarget(void) {
    if (!(gMoveResultFlags & MOVE_RESULT_NO_EFFECT) && gBattleMoves[gCurrentMove].power != 0 &&
        (gLastHitBy[gBattlerTarget] == 0xFF || gLastHitBy[gBattlerTarget] == gBattlerAttacker) && gBattlerTarget != gBattlerAttacker &&
        GetTurnBattler() == gBattlerAttacker &&
        (gChosenMove == gChosenMoveByBattler[gBattlerAttacker] || gChosenMove == gBattleMons[gBattlerAttacker].moves[gChosenMovePos]))
        return TRUE;
    else
        return FALSE;
}

static void HandleTerrainMove(u32 moveEffect) {
    u32 statusFlag = 0;
    u8* timer = NULL;
    u8 override = READ_8_INC;
    const u8* ptr = READ_PTR_INC;

    switch (override) {
        case B_MSG_TERRAINBECOMESMISTY:
            moveEffect = EFFECT_MISTY_TERRAIN;
            break;
        case B_MSG_TERRAINBECOMESGRASSY:
            moveEffect = EFFECT_GRASSY_TERRAIN;
            break;
        case B_MSG_TERRAINBECOMESELECTRIC:
            moveEffect = EFFECT_ELECTRIC_TERRAIN;
            break;
        case B_MSG_TERRAINBECOMESPSYCHIC:
            moveEffect = EFFECT_PSYCHIC_TERRAIN;
            break;
    }

    switch (moveEffect) {
        case EFFECT_MISTY_TERRAIN:
            statusFlag = STATUS_FIELD_MISTY_TERRAIN, timer = &gFieldTimers.terrainTimer;
            SetActiveMultistringChooser(B_MSG_TERRAINBECOMESMISTY);
            break;
        case EFFECT_GRASSY_TERRAIN:
            statusFlag = STATUS_FIELD_GRASSY_TERRAIN, timer = &gFieldTimers.terrainTimer;
            SetActiveMultistringChooser(B_MSG_TERRAINBECOMESGRASSY);
            break;
        case EFFECT_ELECTRIC_TERRAIN:
            statusFlag = STATUS_FIELD_ELECTRIC_TERRAIN, timer = &gFieldTimers.terrainTimer;
            SetActiveMultistringChooser(B_MSG_TERRAINBECOMESELECTRIC);
            break;
        case EFFECT_PSYCHIC_TERRAIN:
            statusFlag = STATUS_FIELD_PSYCHIC_TERRAIN, timer = &gFieldTimers.terrainTimer;
            SetActiveMultistringChooser(B_MSG_TERRAINBECOMESPSYCHIC);
            break;
    }

    if (gFieldStatuses & statusFlag || statusFlag == 0) {
        gBattlescriptCurrInstr = ptr;
    } else {
        gFieldStatuses &= ~STATUS_FIELD_TERRAIN_ANY;
        gFieldStatuses |= statusFlag;
        gFieldTimers.started.terrain = TRUE;
        if (GetBattlerHoldEffect(gBattlerAttacker, TRUE) == HOLD_EFFECT_TERRAIN_EXTENDER)
            *timer = TERRAIN_DURATION_EXTENDED;
        else
            *timer = TERRAIN_DURATION;
    }
}

bool32 CanPoisonType(u8 battlerAttacker, u8 battlerTarget) {
    if (BATTLER_HAS_ABILITY(battlerAttacker, ABILITY_CORROSION)) return TRUE;
    if (BATTLER_HAS_ABILITY(battlerAttacker, ABILITY_TOXIC_SPILL)) return TRUE;
    if (BATTLER_HAS_ABILITY(battlerAttacker, ABILITY_ANGELS_WRATH)) return TRUE;
    if (BATTLER_HAS_ABILITY(battlerAttacker, ABILITY_ACIDIC_SLIME)) return TRUE;
    if (BATTLER_HAS_ABILITY(battlerAttacker, ABILITY_PYROCLASTIC_FLOW)) return TRUE;
    if (IS_BATTLER_OF_TYPE(battlerTarget, TYPE_POISON)) return FALSE;
    if (IS_BATTLER_OF_TYPE(battlerTarget, TYPE_STEEL)) return FALSE;
    return TRUE;
}

bool32 CanParalyzeType(u8 battlerAttacker, u8 battlerTarget) {
    // New Code added for Ability Overcharge
    if (IS_BATTLER_OF_TYPE(battlerTarget, TYPE_ELECTRIC)) {
        if (BATTLER_HAS_ABILITY(battlerAttacker, ABILITY_OVERCHARGE)) return TRUE;
        if (BATTLER_HAS_ABILITY(battlerAttacker, ABILITY_DEPRAVITY)) return TRUE;
        if (B_PARALYZE_ELECTRIC >= GEN_6) return FALSE;
    }
    return TRUE;
}

bool32 CanUseLastResort(u8 battlerId) {
    u32 i;
    u32 knownMovesCount = 0, usedMovesCount = 0;

    for (i = 0; i < 4; i++) {
        if (gBattleMons[battlerId].moves[i] != MOVE_NONE) knownMovesCount++;
        if (i != gCurrMovePos && gVolatileStructs[battlerId].usedMoves & gBitTable[i])  // Increment used move count for all moves except current Last Resort.
            usedMovesCount++;
    }

    return (knownMovesCount >= 2 && usedMovesCount >= knownMovesCount - 1);
}

#define DEFOG_CLEAR(status, structField, battlescript, move)           \
    {                                                                  \
        if (*sideStatuses & status) {                                  \
            if (clear) {                                               \
                if (move) PREPARE_MOVE_BUFFER(gBattleTextBuff1, move); \
                *sideStatuses &= ~(status);                            \
                sideTimer->structField = 0;                            \
                BattleScriptCall(battlescript);                        \
            }                                                          \
            return TRUE;                                               \
        }                                                              \
    }

static bool32 ClearDefogHazards(u8 battlerAtk, bool32 clear) {
    s32 i;
    for (i = 0; i < 2; i++) {
        struct SideTimer* sideTimer = &gSideTimers[i];
        u32* sideStatuses = &gSideStatuses[i];

        gBattlerAttacker = i;
        if (GetBattlerSide(battlerAtk) != i) {
            DEFOG_CLEAR(SIDE_STATUS_REFLECT, reflectTimer, BattleScript_SideStatusWoreOffReturn, MOVE_REFLECT);
            DEFOG_CLEAR(SIDE_STATUS_LIGHTSCREEN, lightscreenTimer, BattleScript_SideStatusWoreOffReturn, MOVE_LIGHT_SCREEN);
            DEFOG_CLEAR(SIDE_STATUS_MIST, mistTimer, BattleScript_SideStatusWoreOffReturn, MOVE_MIST);
            DEFOG_CLEAR(SIDE_STATUS_AURORA_VEIL, auroraVeilTimer, BattleScript_SideStatusWoreOffReturn, MOVE_AURORA_VEIL);
            DEFOG_CLEAR(SIDE_STATUS_SAFEGUARD, safeguardTimer, BattleScript_SideStatusWoreOffReturn, MOVE_SAFEGUARD);
            DEFOG_CLEAR(SIDE_STATUS_LIGHTSCREEN, smokescreenTimer, BattleScript_SideStatusWoreOffReturn, MOVE_SMOKESCREEN);
        }
        DEFOG_CLEAR(SIDE_STATUS_SPIKES, spikesAmount, BattleScript_SpikesFree, 0);
        switch (sideTimer->stealthRockType) {
            case TYPE_ROCK:
                gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_STEALTH_ROCK_FREE;
                break;
            case TYPE_GRASS:
                gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_CREEPING_THORNS_FREE;
                break;
        }
        DEFOG_CLEAR(SIDE_STATUS_STEALTH_ROCK, stealthRockType, BattleScript_StealthRockFree, 0);
        DEFOG_CLEAR(SIDE_STATUS_TOXIC_SPIKES, toxicSpikesAmount, BattleScript_ToxicSpikesFree, 0);
        DEFOG_CLEAR(SIDE_STATUS_STICKY_WEB, stickyWebTimer, BattleScript_StickyWebFree, 0);
        if (gSideTimers[i].caltrops) {
            if (clear) {
                gSideTimers[i].caltrops = FALSE;
                BattleScriptCall(BattleScript_CaltropsFree);
            }
            return TRUE;
        }
        if (gSideTimers[i].hotCoals) {
            if (clear) {
                gSideTimers[i].hotCoals = FALSE;
                BattleScriptCall(BattleScript_HotCoalsFree);
            }
            return TRUE;
        }
    }
    if (gBattleWeather & WEATHER_FOG_ANY) {
        if (gBattleWeather & WEATHER_FOG_PERMANENT)
            gFieldTimers.fogReturnTimer = 99;
        else
            gFieldTimers.fogReturnTimer = gWishFutureKnock.weatherDuration - 1;
        gBattleWeather &= ~WEATHER_FOG_ANY;
        BattleScriptCall(BattleScript_FogBlownAway);
        return TRUE;
    }

    return FALSE;
}

u32 IsFlowerVeilProtected(u32 battler) {
    u16 isFlowerVeilProtected = FALSE;
    if (IsAbilityOnSide(battler, ABILITY_FLOWER_VEIL))
        isFlowerVeilProtected = ABILITY_FLOWER_VEIL;
    else if (IsAbilityOnSide(battler, ABILITY_JUNGLES_GUARD))
        isFlowerVeilProtected = ABILITY_JUNGLES_GUARD;

    if (isFlowerVeilProtected && IS_BATTLER_OF_TYPE(battler, TYPE_GRASS))
        return isFlowerVeilProtected;
    else
        return FALSE;
}

u32 IsLeafGuardProtected(u32 battler) {
    if (IsBattlerWeatherAffected(battler, WEATHER_SUN_ANY)) {
        if (BATTLER_HAS_ABILITY(battler, ABILITY_LEAF_GUARD)) return ABILITY_LEAF_GUARD;
        if (BATTLER_HAS_ABILITY(battler, ABILITY_BIG_LEAVES)) return ABILITY_BIG_LEAVES;
        if (BATTLER_HAS_ABILITY(battler, ABILITY_JUNGLES_GUARD)) return ABILITY_JUNGLES_GUARD;
    }

    return FALSE;
}

u32 IsDesertCloakProtected(u32 battler) {
    if (IsBattlerWeatherAffected(battler, WEATHER_SANDSTORM_ANY) &&
        (BATTLER_HAS_ABILITY(battler, ABILITY_DESERT_CLOAK) || BATTLER_HAS_ABILITY(BATTLE_PARTNER(battler), ABILITY_DESERT_CLOAK)))
        return TRUE;
    else
        return FALSE;
}

bool32 IsShieldsDownProtected(u32 battler) {
    switch (gBattleMons[battler].species) {
        case SPECIES_MINIOR:
        case SPECIES_MINIOR_METEOR_ORANGE:
        case SPECIES_MINIOR_METEOR_YELLOW:
        case SPECIES_MINIOR_METEOR_GREEN:
        case SPECIES_MINIOR_METEOR_BLUE:
        case SPECIES_MINIOR_METEOR_INDIGO:
        case SPECIES_MINIOR_METEOR_VIOLET:
            if (BATTLER_HAS_ABILITY(battler, ABILITY_SHIELDS_DOWN))  // Minior is not in core form
                return ABILITY_SHIELDS_DOWN;
            break;
    }
    return FALSE;
}

u32 IsAbilityStatusProtected(u32 battler) {
    u16 ability;
    if ((ability = IsFlowerVeilProtected(battler))) return ability;
    if ((ability = IsLeafGuardProtected(battler))) return ability;
    if (IsDesertCloakProtected(battler)) return ABILITY_DESERT_CLOAK;
    if (IsShieldsDownProtected(battler)) return ABILITY_SHIELDS_DOWN;
    if (BATTLER_HAS_ABILITY(battler, ABILITY_COMATOSE)) return ABILITY_COMATOSE;
    if (BATTLER_HAS_ABILITY(battler, ABILITY_BLOOD_STAIN)) return ABILITY_BLOOD_STAIN;
    if (BATTLER_HAS_ABILITY(battler, ABILITY_PURIFYING_SALT)) return ABILITY_PURIFYING_SALT;
    if (BATTLER_HAS_ABILITY(battler, ABILITY_BLOOD_STIGMA)) return ABILITY_BLOOD_STIGMA;
    return FALSE;
}

u32 JumpIfStandardStatusBlocking(u32 battler, bool32 affectsUser) {
    u16 ability;
    if (!affectsUser && gBattleMons[gActiveBattler].status1)
        gBattlescriptCurrInstr = BattleScript_ButItFailed;
    else if (!affectsUser && DoesSubstituteBlockMove(gBattlerAttacker, gActiveBattler, gCurrentMove))
        gBattlescriptCurrInstr = BattleScript_ButItFailed;
    else if ((ability = IsAbilityStatusProtected(gActiveBattler))) {
        if (!BATTLER_HAS_ABILITY(gActiveBattler, ability)) gBattleScripting.battlerPopupOverwrite = BATTLE_PARTNER(gActiveBattler);
        SetActiveAbilityPopupOverride(ability);
        gBattlescriptCurrInstr = BattleScript_LeafGuardProtects;
    } else if (IsBattlerTerrainAffected(gActiveBattler, STATUS_FIELD_MISTY_TERRAIN))
        gBattlescriptCurrInstr = BattleScript_MistyTerrainPrevents;
    else if (!affectsUser && gSideStatuses[GetBattlerSide(gActiveBattler)] & SIDE_STATUS_SAFEGUARD)
        gBattlescriptCurrInstr = BattleScript_SafeguardProtected;
    else
        return FALSE;
    return TRUE;
}

static void RecalcBattlerStats(u32 battler, struct Pokemon* mon) {
    CalculateMonStats(mon);
    gBattleMons[battler].level = GetMonData(mon, MON_DATA_LEVEL);
    gBattleMons[battler].hp = GetMonData(mon, MON_DATA_HP);
    gBattleMons[battler].maxHP = GetMonData(mon, MON_DATA_MAX_HP);
    gBattleMons[battler].attack = GetMonData(mon, MON_DATA_ATK);
    gBattleMons[battler].defense = GetMonData(mon, MON_DATA_DEF);
    gBattleMons[battler].speed = GetMonData(mon, MON_DATA_SPEED);
    gBattleMons[battler].spAttack = GetMonData(mon, MON_DATA_SPATK);
    gBattleMons[battler].spDefense = GetMonData(mon, MON_DATA_SPDEF);
    RepopulateAbilities(battler);

    gBattleMons[battler].type1 =
        RandomizeType(gBaseStats[gBattleMons[battler].species].type1, gBattleMons[battler].species, gBattleMons[battler].personality, TRUE);
    gBattleMons[battler].type2 =
        RandomizeType(gBaseStats[gBattleMons[battler].species].type2, gBattleMons[battler].species, gBattleMons[battler].personality, FALSE);
}

static bool32 IsRototillerAffected(u32 battlerId) {
    if (!IsBattlerAlive(battlerId)) return FALSE;
    if (!IsBattlerGrounded(battlerId)) return FALSE;                      // Only grounded battlers affected
    if (!IS_BATTLER_OF_TYPE(battlerId, TYPE_GRASS)) return FALSE;         // Only grass types affected
    if (gStatuses3[battlerId] & STATUS3_SEMI_INVULNERABLE) return FALSE;  // Rototiller doesn't affected semi-invulnerable battlers
    if (BlocksPrankster(MOVE_ROTOTILLER, gBattlerAttacker, battlerId, FALSE)) return FALSE;
    return TRUE;
}

static int ProtectSucceeds(int battler) {
    if (!(gBattleMoves[gLastResultingMoves[battler]].flags & FLAG_PROTECTION_MOVE)) gVolatileStructs[battler].protectUses = 0;

    if (gVolatileStructs[battler].protectUses > 3) return FALSE;
    if (sProtectSuccessRates[gVolatileStructs[battler].protectUses] >= Random()) return TRUE;
    return FALSE;
}

static bool32 CanTeleport(u8 battlerId) {
    struct Pokemon* party = NULL;
    u32 species, count = 0, i;

    if (GetBattlerSide(battlerId) == B_SIDE_PLAYER)
        party = gPlayerParty;
    else
        party = gEnemyParty;

    for (i = 0; i < PARTY_SIZE; i++) {
        species = GetMonData(&party[i], MON_DATA_SPECIES2);
        if (species != SPECIES_NONE && species != SPECIES_EGG && GetMonData(&party[i], MON_DATA_HP) != 0) count++;
    }

    switch (GetBattlerSide(battlerId)) {
        case B_SIDE_OPPONENT:
            if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE) return FALSE;
            break;
        case B_SIDE_PLAYER:
            if (count == 1 || (count <= 2 && gBattleTypeFlags & BATTLE_TYPE_DOUBLE)) return FALSE;
            break;
    }

    return TRUE;
}

static void Cmd_various(void) {
    struct Pokemon* mon;
    s32 i;
    u8 data[10];
    u32 side, bits;
    u8 increase;
    u8 statId;
    const u8* ptr;
    const u8* runAgain;
    int battlerType;
    int cmd;

    if (gBattleControllerExecFlags) return;

    runAgain = gBattlescriptCurrInstr++;
    battlerType = READ_8_INC;

    gActiveBattler = GetBattlerForBattleScript(battlerType);

    cmd = READ_8_INC;

    switch (cmd) {
        // Roar will fail in a double wild battle when used by the player against one of the two alive wild mons.
        // Also when an opposing wild mon uses it againt its partner.
        case VARIOUS_JUMP_IF_ROAR_FAILS:
            ptr = READ_PTR_INC;
            if (WILD_DOUBLE_BATTLE && GetBattlerSide(gBattlerAttacker) == B_SIDE_PLAYER && GetBattlerSide(gBattlerTarget) == B_SIDE_OPPONENT &&
                IS_WHOLE_SIDE_ALIVE(gBattlerTarget))
                gBattlescriptCurrInstr = ptr;
            else if (WILD_DOUBLE_BATTLE && GetBattlerSide(gBattlerAttacker) == B_SIDE_OPPONENT && GetBattlerSide(gBattlerTarget) == B_SIDE_OPPONENT)
                gBattlescriptCurrInstr = ptr;
            return;
        case VARIOUS_JUMP_IF_ABSENT:
            ptr = READ_PTR_INC;
            if (!IsBattlerAlive(gActiveBattler)) gBattlescriptCurrInstr = ptr;
            return;
        case VARIOUS_JUMP_IF_SHIELDS_DOWN_PROTECTED:
            ptr = READ_PTR_INC;
            if (IsShieldsDownProtected(gActiveBattler)) gBattlescriptCurrInstr = ptr;
            return;
        case VARIOUS_JUMP_IF_NO_HOLD_EFFECT: {
            int holdEffect = READ_8_INC;
            ptr = READ_PTR_INC;
            if (GetBattlerHoldEffect(gActiveBattler, TRUE) != holdEffect)
                gBattlescriptCurrInstr = ptr;
            else
                gLastUsedItem = gBattleMons[gActiveBattler].item;  // For B_LAST_USED_ITEM
        }
            return;
        case VARIOUS_JUMP_IF_NO_ALLY:
            ptr = READ_PTR_INC;
            if (!IsBattlerAlive(BATTLE_PARTNER(gActiveBattler))) gBattlescriptCurrInstr = ptr;
            return;
        case VARIOUS_INFATUATE_WITH_BATTLER: {
            int attacker = GetBattlerForBattleScript(READ_8_INC);
            gBattleScripting.battler = gActiveBattler;
            gBattleMons[gActiveBattler].status2 |= STATUS2_INFATUATED_WITH(attacker);
            return;
        }
        case VARIOUS_SET_LAST_USED_ITEM:
            gLastUsedItem = gBattleMons[gActiveBattler].item;
            break;
        case VARIOUS_TRY_FAIRY_LOCK:
            ptr = READ_PTR_INC;
            if (gFieldStatuses & STATUS_FIELD_FAIRY_LOCK) {
                gBattlescriptCurrInstr = ptr;
            } else {
                gFieldStatuses |= STATUS_FIELD_FAIRY_LOCK;
                gFieldTimers.started.fairyLock = TRUE;
                gFieldTimers.fairyLockTimer = 1;
            }
            return;
        case VARIOUS_GET_STAT_VALUE:
            i = READ_8_INC;
            gBattleMoveDamage = CalculateStat(gActiveBattler, i, 0, 0, TRUE, FALSE, IsUnaware(gBattlerAttacker), FALSE);
            return;
        case VARIOUS_JUMP_IF_FULL_HP:
            ptr = READ_PTR_INC;
            if (BATTLER_MAX_HP(gActiveBattler)) gBattlescriptCurrInstr = ptr;
            return;
        case VARIOUS_TRY_FRISK:
            while (gBattleStruct->friskedBattler < gBattlersCount) {
                gBattlerTarget = gBattleStruct->friskedBattler++;
                if (GET_BATTLER_SIDE2(gActiveBattler) != GET_BATTLER_SIDE2(gBattlerTarget) && IsBattlerAlive(gBattlerTarget) &&
                    gBattleMons[gBattlerTarget].item != ITEM_NONE) {
                    gLastUsedItem = gBattleMons[gBattlerTarget].item;
                    RecordItemEffectBattle(gBattlerTarget, GetBattlerHoldEffect(gBattlerTarget, FALSE));
                    BattleScriptPush(runAgain);
                    gBattlescriptCurrInstr = BattleScript_FriskMsg;

                    if (!(gStatuses3[gBattlerTarget] & STATUS3_EMBARGO)) {
                        gStatuses3[gBattlerTarget] |= STATUS3_EMBARGO;
                        gVolatileStructs[gBattlerTarget].embargoTimer = 2;
                    }
                    return;
                }
            }
            gBattleStruct->friskedBattler = 0;
            gBattleStruct->friskedAbility = FALSE;
            break;
        case VARIOUS_POISON_TYPE_IMMUNITY: {
            int target = GetBattlerForBattleScript(READ_8_INC);
            ptr = READ_PTR_INC;
            if (!CanPoisonType(gActiveBattler, target)) gBattlescriptCurrInstr = ptr;
        }
            return;
        case VARIOUS_PARALYZE_TYPE_IMMUNITY: {
            int target = GetBattlerForBattleScript(READ_8_INC);
            ptr = READ_PTR_INC;
            if (!CanParalyzeType(gActiveBattler, target)) gBattlescriptCurrInstr = ptr;
        }
            return;
        case VARIOUS_TRACE_ABILITY:
            if (DoesBattlerHaveAbilityShield(gActiveBattler)) break;
            UpdateAbilityStateIndicesForNewAbility(gActiveBattler, gBattleStruct->tracedAbility[gActiveBattler]);
            ReplaceAbility(gActiveBattler, gBattleStruct->tracedAbility[gActiveBattler]);
            break;
        case VARIOUS_TRY_ILLUSION_OFF:
            if (GetIllusionMonPtr(gActiveBattler) != NULL) {
                BattleScriptCall(BattleScript_IllusionOff);
            }
            return;
        case VARIOUS_SET_SPRITEIGNORE0HP:
            gBattleStruct->spriteIgnore0Hp = READ_8_INC;
            return;
        case VARIOUS_UPDATE_NICK:
            if (GetBattlerSide(gActiveBattler) == B_SIDE_PLAYER)
                mon = &gPlayerParty[gBattlerPartyIndexes[gActiveBattler]];
            else
                mon = &gEnemyParty[gBattlerPartyIndexes[gActiveBattler]];
            UpdateHealthboxAttribute(gHealthboxSpriteIds[gActiveBattler], mon, HEALTHBOX_NICK);
            break;
        case VARIOUS_JUMP_IF_NOT_BERRY:
            ptr = READ_PTR_INC;
            if (IsItemNegated(gActiveBattler))
                gBattlescriptCurrInstr = ptr;
            else if (ItemId_GetPocket(gBattleMons[gActiveBattler].item) != POCKET_BERRIES)
                gBattlescriptCurrInstr = ptr;
            return;
        case VARIOUS_CHECK_IF_GRASSY_TERRAIN_HEALS:
            ptr = READ_PTR_INC;
            if ((gStatuses3[gActiveBattler] & (STATUS3_SEMI_INVULNERABLE)) || BATTLER_MAX_HP(gActiveBattler) || !gBattleMons[gActiveBattler].hp ||
                !(IsBattlerGrounded(gActiveBattler))) {
                gBattlescriptCurrInstr = ptr;
            } else {
                gBattleMoveDamage = gBattleMons[gActiveBattler].maxHP / 16;
                if (gBattleMoveDamage == 0) gBattleMoveDamage = 1;
                gBattleMoveDamage *= -1;
            }
            return;
        case VARIOUS_GRAVITY_ON_AIRBORNE_MONS:
            if (gStatuses3[gActiveBattler] & STATUS3_ON_AIR) CancelMultiTurnMoves(gActiveBattler);

            gStatuses3[gActiveBattler] &= ~(STATUS3_MAGNET_RISE | STATUS3_TELEKINESIS | STATUS3_ON_AIR);
            break;
        case VARIOUS_SPECTRAL_THIEF:
            // Raise stats
            for (i = STAT_ATK; i < NUM_BATTLE_STATS; i++) {
                if (gBattleStruct->stolenStats[0] & gBitTable[i]) {
                    gBattleStruct->stolenStats[0] &= ~(gBitTable[i]);
                    SET_STATCHANGER(i, gBattleStruct->stolenStats[i], FALSE);
                    if (ChangeStatBuffsImplicit(
                            GET_STAT_BUFF_VALUE_WITH_SIGN(gBattleScripting.statChanger), i, MOVE_EFFECT_CERTAIN | MOVE_EFFECT_AFFECTS_USER, NULL)) {
                        BattleScriptCall(BattleScript_StatUpMsg);
                        return;
                    }
                }
            }
            break;
        case VARIOUS_SET_POWDER:
            gBattleMons[gActiveBattler].status2 |= STATUS2_POWDER;
            break;
        case VARIOUS_ACUPRESSURE:
            ptr = READ_PTR_INC;
            bits = 0;
            for (i = STAT_ATK; i < NUM_BATTLE_STATS; i++) {
                if (CompareStat(gActiveBattler, i, MAX_STAT_STAGE, CMP_LESS_THAN)) bits |= gBitTable[i];
            }
            if (bits) {
                u32 statId;
                do {
                    statId = (Random() % (NUM_BATTLE_STATS - 1)) + 1;
                } while (!(bits & gBitTable[statId]));

                SetActiveStatChanger(statId, 2);
            } else {
                gBattlescriptCurrInstr = ptr;
            }
            return;
        case VARIOUS_CANCEL_MULTI_TURN_MOVES:
            CancelMultiTurnMoves(gActiveBattler);
            break;
        case VARIOUS_SET_MAGIC_COAT_TARGET:
            gBattlerAttacker = gBattlerTarget;
            side = GetBattlerSide(gBattlerAttacker) ^ BIT_SIDE;
            if (IsAffectedByFollowMe(gBattlerAttacker, side, gCurrentMove))
                gBattlerTarget = gSideTimers[side].followmeTarget;
            else
                gBattlerTarget = gActiveBattler;
            break;
        case VARIOUS_IS_RUNNING_IMPOSSIBLE:
            gBattleCommunication[0] = IsRunningFromBattleImpossible();
            break;
        case VARIOUS_GET_MOVE_TARGET:
            gBattlerTarget = GetMoveTarget(gCurrentMove, 0);
            break;
        case VARIOUS_GET_BATTLER_FAINTED:
            if (gHitMarker & HITMARKER_FAINTED(gActiveBattler))
                gBattleCommunication[0] = TRUE;
            else
                gBattleCommunication[0] = FALSE;
            break;
        case VARIOUS_RESET_INTIMIDATE_TRACE_BITS:
            gTurnStructs[gActiveBattler].intimidatedMon = FALSE;
            gTurnStructs[gActiveBattler].scaredMon = FALSE;
            gTurnStructs[gActiveBattler].traced = FALSE;
            break;
        case VARIOUS_UPDATE_CHOICE_MOVE_ON_LVL_UP:
            if (gBattlerPartyIndexes[0] == gBattleStruct->expGetterMonId || gBattlerPartyIndexes[2] == gBattleStruct->expGetterMonId) {
                if (gBattlerPartyIndexes[0] == gBattleStruct->expGetterMonId)
                    gActiveBattler = 0;
                else
                    gActiveBattler = 2;

                for (i = 0; i < MAX_MON_MOVES; i++) {
                    if (gBattleMons[gActiveBattler].moves[i] == gBattleStruct->choicedMove[gActiveBattler]) break;
                }
                if (i == MAX_MON_MOVES) gBattleStruct->choicedMove[gActiveBattler] = 0;
            }
            break;
        case 7:
            if (!(gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_DOUBLE)) && gBattleTypeFlags & BATTLE_TYPE_TRAINER && gBattleMons[0].hp != 0 &&
                gBattleMons[1].hp != 0) {
                gHitMarker &= ~(HITMARKER_x400000);
            }
            break;
        case VARIOUS_PALACE_FLAVOR_TEXT:
            // Try and print end-of-turn Battle Palace flavor text (e.g. "A glint appears in mon's eyes")
            gBattleCommunication[0] = FALSE;  // whether or not msg should be printed
            gBattleScripting.battler = gActiveBattler = gBattleCommunication[1];
            if (!(gBattleStruct->palaceFlags & gBitTable[gActiveBattler]) && gBattleMons[gActiveBattler].maxHP / 2 >= gBattleMons[gActiveBattler].hp &&
                gBattleMons[gActiveBattler].hp != 0 && !(gBattleMons[gActiveBattler].status1 & STATUS1_SLEEP)) {
                gBattleStruct->palaceFlags |= gBitTable[gActiveBattler];
                gBattleCommunication[0] = TRUE;
                gBattleCommunication[MULTISTRING_CHOOSER] =
                    sBattlePalaceNatureToFlavorTextId[GetNatureFromPersonality(gBattleMons[gActiveBattler].personality)];
            }
            break;
        case VARIOUS_ARENA_JUDGMENT_WINDOW:
            i = BattleArena_ShowJudgmentWindow(&gBattleCommunication[0]);
            if (i == 0) return;

            gBattleCommunication[1] = i;
            break;
        case VARIOUS_ARENA_OPPONENT_MON_LOST:
            gBattleMons[1].hp = 0;
            gHitMarker |= HITMARKER_FAINTED(1);
            gBattleStruct->arenaLostOpponentMons |= gBitTable[gBattlerPartyIndexes[1]];
            break;
        case VARIOUS_ARENA_PLAYER_MON_LOST:
            gBattleMons[0].hp = 0;
            gHitMarker |= HITMARKER_FAINTED(0);
            gHitMarker |= HITMARKER_x400000;
            gBattleStruct->arenaLostPlayerMons |= gBitTable[gBattlerPartyIndexes[0]];
            break;
        case VARIOUS_ARENA_BOTH_MONS_LOST:
            gBattleMons[0].hp = 0;
            gBattleMons[1].hp = 0;
            gHitMarker |= HITMARKER_FAINTED(0);
            gHitMarker |= HITMARKER_FAINTED(1);
            gHitMarker |= HITMARKER_x400000;
            gBattleStruct->arenaLostPlayerMons |= gBitTable[gBattlerPartyIndexes[0]];
            gBattleStruct->arenaLostOpponentMons |= gBitTable[gBattlerPartyIndexes[1]];
            break;
        case VARIOUS_EMIT_YESNOBOX:
            BtlController_EmitYesNoBox(0);
            MarkBattlerForControllerExec(gActiveBattler);
            break;
        case 14:
            DrawArenaRefereeTextBox();
            break;
        case 15:
            RemoveArenaRefereeTextBox();
            break;
        case VARIOUS_ARENA_JUDGMENT_STRING:
            BattleStringExpandPlaceholdersToDisplayedString(gRefereeStringsTable[battlerType]);
            BattlePutTextOnWindow(gDisplayedStringBattle, ARENA_WIN_JUDGMENT_TEXT);
            break;
        case VARIOUS_ARENA_WAIT_STRING:
            if (IsTextPrinterActive(ARENA_WIN_JUDGMENT_TEXT)) return;
            break;
        case VARIOUS_WAIT_CRY:
            if (!IsCryFinished()) return;
            break;
        case VARIOUS_RETURN_OPPONENT_MON1:
            gActiveBattler = 1;
            if (gBattleMons[gActiveBattler].hp != 0) {
                BtlController_EmitReturnMonToBall(0, 0);
                MarkBattlerForControllerExec(gActiveBattler);
            }
            break;
        case VARIOUS_RETURN_OPPONENT_MON2:
            if (gBattlersCount > 3) {
                gActiveBattler = 3;
                if (gBattleMons[gActiveBattler].hp != 0) {
                    BtlController_EmitReturnMonToBall(0, 0);
                    MarkBattlerForControllerExec(gActiveBattler);
                }
            }
            break;
        case VARIOUS_VOLUME_DOWN:
            m4aMPlayVolumeControl(&gMPlayInfo_BGM, 0xFFFF, 0x55);
            break;
        case VARIOUS_VOLUME_UP:
            m4aMPlayVolumeControl(&gMPlayInfo_BGM, 0xFFFF, 0x100);
            break;
        case VARIOUS_SET_ALREADY_STATUS_MOVE_ATTEMPT:
            gBattleStruct->alreadyStatusedMoveAttempt |= gBitTable[gActiveBattler];
            break;
        case 24:
            if (sub_805725C(gActiveBattler)) return;
            break;
        case VARIOUS_SET_TELEPORT_OUTCOME:
            // Don't end the battle if one of the wild mons teleported from the wild double battle
            // and its partner is still alive.
            if (GetBattlerSide(gActiveBattler) == B_SIDE_OPPONENT && IsBattlerAlive(BATTLE_PARTNER(gActiveBattler))) {
                gAbsentBattlerFlags |= gBitTable[gActiveBattler];
                gHitMarker |= HITMARKER_FAINTED(gActiveBattler);
                gBattleMons[gActiveBattler].hp = 0;
                SetMonData(&gEnemyParty[gBattlerPartyIndexes[gActiveBattler]], MON_DATA_HP, &gBattleMons[gActiveBattler].hp);
                SetHealthboxSpriteInvisible(gHealthboxSpriteIds[gActiveBattler]);
                FaintClearSetData();
            } else if (GetBattlerSide(gActiveBattler) == B_SIDE_PLAYER) {
                gBattleOutcome = B_OUTCOME_PLAYER_TELEPORTED;
            } else {
                gBattleOutcome = B_OUTCOME_MON_TELEPORTED;
            }
            break;
        case VARIOUS_PLAY_TRAINER_DEFEATED_MUSIC:
            BtlController_EmitPlayFanfareOrBGM(0, MUS_VICTORY_TRAINER, TRUE);
            MarkBattlerForControllerExec(gActiveBattler);
            break;
        case VARIOUS_SET_ACTIVE_STAT_CHANGER: {
            StatChanger statChanger = {.value = READ_8_INC};
            SetActiveStatChanger(statChanger.statId, GET_STAT_BUFF_VALUE_WITH_SIGN(statChanger));
        } break;
        case VARIOUS_SWITCHIN_ABILITIES:
            REQUIRE(IsBattlerAlive(gActiveBattler))
            gBattlerAttacker = gActiveBattler;
            AbilityBattleEffects(ABILITYEFFECT_NEUTRALIZINGGAS, gActiveBattler, 0, ABILITY_BS_CALL, 0);
            ptr = gBattlescriptCurrInstr;
            gBattlescriptCurrInstr = runAgain;
            while (gBattleScripting.abilityLoopCounter <= NUM_ABILITY_SLOTS + 1) {
                if (HandleSwitchInAbility(gBattleScripting.abilityLoopCounter++, gActiveBattler)) return;
            }
            gBattlescriptCurrInstr = ptr;
            AbilityBattleEffects(ABILITYEFFECT_TRACE2, gActiveBattler, 0, 0, 0);
            return;
        case VARIOUS_SAVE_TARGET:
            SetActiveStackBattler(gBattlerTarget, 4);
            gSavedBattleScripting = gBattleScripting;
            break;
        case VARIOUS_RESTORE_TARGET:
            gBattlerTarget = gStackBattler4;
            gBattleScripting = gSavedBattleScripting;
            break;
        case VARIOUS_INSTANT_HP_DROP:
            BtlController_EmitHealthBarUpdate(0, INSTANT_HP_BAR_DROP);
            MarkBattlerForControllerExec(gActiveBattler);
            break;
        case VARIOUS_CLEAR_STATUS:
            gBattleMons[gActiveBattler].status1 = 0;
            BtlController_EmitSetMonData(0, REQUEST_STATUS_BATTLE, 0, 4, &gBattleMons[gActiveBattler].status1);
            MarkBattlerForControllerExec(gActiveBattler);
            break;
        case VARIOUS_RESTORE_PP:
            for (i = 0; i < 4; i++) {
                gBattleMons[gActiveBattler].pp[i] = CalculatePPWithBonus(gBattleMons[gActiveBattler].moves[i], gBattleMons[gActiveBattler].ppBonuses, i);
                data[i] = gBattleMons[gActiveBattler].pp[i];
            }
            data[i] = gBattleMons[gActiveBattler].ppBonuses;
            BtlController_EmitSetMonData(0, REQUEST_PP_DATA_BATTLE, 0, 5, data);
            MarkBattlerForControllerExec(gActiveBattler);
            break;
        case VARIOUS_TRY_ACTIVATE_RAMPAGE:
            break;
        case VARIOUS_ON_FAINTED_BY_ATTACKER:
            REQUIRE_NOT(NoAliveMonsForEitherParty())
            int moveType;
            GET_MOVE_TYPE(gCurrentMove, moveType)
            for (int i = 0; i < gBattlersCount; i++) {
                FILTER(IsBattlerAlive(i))
                ON_ABILITY(
                    i,
                    FALSE,
                    gAbilities[ability].onBattlerFaints &&
                        IsTargettedApplyOnFlagAppropriate(gActiveBattler, i, gBattlerAttacker, gActiveBattler, gAbilities[ability].onBattlerFaintsFor),
                    gStackBattler1 = i;
                    if (gAbilities[ability].onBattlerFaints(ability, i, gBattlerAttacker, gActiveBattler, gCurrentMove, moveType) & 1) {
                        gBattleScripting.abilityPopupOverwrite = ability;
                        BattleScriptCall(BattleScript_AbilityPopUpStack);
                    })
            }
            ReadActiveScriptInitialStackState();
            break;
        case VARIOUS_TRY_ACTIVATE_SUPER_STRAIN:  // and variants
            break;
        case VARIOUS_TRY_ACTIVATE_SOUL_EATER:
            break;
        case VARIOUS_TRY_ACTIVATE_GRIM_NEIGH:
            break;
        case VARIOUS_TRY_ACTIVATE_RECEIVER:  // Partner gets fainted's ally ability
            break;
        case VARIOUS_TRY_ACTIVATE_BEAST_BOOST:
            break;
        case VARIOUS_ON_FAINTED_BY_OTHER:
            REQUIRE_NOT(NoAliveMonsForEitherParty())
            for (int i = 0; i < gBattlersCount; i++) {
                FILTER(IsBattlerAlive(i))
                ON_ABILITY(
                    i,
                    FALSE,
                    gAbilities[ability].onBattlerFaints &&
                        IsTargettedApplyOnFlagAppropriate(gActiveBattler, i, MAX_BATTLERS_COUNT, gActiveBattler, gAbilities[ability].onBattlerFaintsFor),
                    gStackBattler1 = i;
                    if (gAbilities[ability].onBattlerFaints(ability, i, MAX_BATTLERS_COUNT, gActiveBattler, 0, 0) & 1) {
                        gBattleScripting.abilityPopupOverwrite = ability;
                        BattleScriptCall(BattleScript_AbilityPopUpStack);
                    })
            }
            ReadActiveScriptInitialStackState();
            break;
        case VARIOUS_TRY_ACTIVATE_FELL_STINGER:
            REQUIRE(IsBattlerAlive(gActiveBattler))
            if (!HasAttackerFaintedTarget()) break;
            if (NoAliveMonsForEitherParty()) break;
            if (gBattleMoves[gCurrentMove].effect == EFFECT_FELL_STINGER && CompareStat(gBattlerAttacker, STAT_ATK, MAX_STAT_STAGE, CMP_LESS_THAN)) {
                SetStatChanger(STAT_ATK, 3);

                BattleScriptCall(BattleScript_FellStingerRaisesStat);
                return;
            } else if (gBattleMoves[gCurrentMove].effect == EFFECT_MISC_HIT && gBattleMoves[gCurrentMove].argument == MISC_EFFECT_TRANSMUTE) {
                BattleScriptCall(BattleScript_TryRecycle);
                return;
            }
            break;
        case VARIOUS_PLAY_MOVE_ANIMATION:
            BtlController_EmitMoveAnimation(0,
                                            READ_16_INC,
                                            gBattleScripting.animTurn,
                                            0,
                                            0,
                                            gBattleMons[gActiveBattler].friendship,
                                            &gVolatileStructs[gActiveBattler],
                                            gTurnStructs[gBattlerAttacker].multiHitCounter);
            MarkBattlerForControllerExec(gActiveBattler);
            return;
        case VARIOUS_SET_LUCKY_CHANT:
            ptr = READ_PTR_INC;
            if (!(gSideStatuses[GET_BATTLER_SIDE(gActiveBattler)] & SIDE_STATUS_LUCKY_CHANT)) {
                int side = GET_BATTLER_SIDE(gActiveBattler);
                gSideTimers[side].started.luckyChant = TRUE;
                gSideStatuses[side] |= SIDE_STATUS_LUCKY_CHANT;
                gSideTimers[side].luckyChantBattlerId = gActiveBattler;
                gSideTimers[side].luckyChantTimer = SCREEN_DURATION;
            } else {
                gBattlescriptCurrInstr = ptr;
            }
            return;
        case VARIOUS_SUCKER_PUNCH_CHECK:
            ptr = READ_PTR_INC;
            if (gRoundStructs[gBattlerTarget].obstructed)
                gBattlescriptCurrInstr = ptr;
            else if (GetBattlerTurnOrderNum(gBattlerAttacker) > GetBattlerTurnOrderNum(gBattlerTarget))
                gBattlescriptCurrInstr = ptr;
            else if (gBattleMoves[gBattleMons[gBattlerTarget].moves[gBattleStruct->chosenMovePositions[gBattlerTarget]]].split == SPLIT_STATUS)
                gBattlescriptCurrInstr = ptr;
            return;
        case VARIOUS_SET_SIMPLE_BEAM:
            ptr = READ_PTR_INC;
            if (IsEntrainmentTargetOrSimpleBeamBannedAbility(GetBattlerAbility(gBattlerTarget)) ||
                HasAbilityIgnoringSuppression(gBattlerTarget, ABILITY_SIMPLE) || DoesBattlerHaveAbilityShield(gBattlerTarget)) {
                gBattlescriptCurrInstr = ptr;
            } else {
                UpdateAbilityStateIndicesForNewAbility(gBattlerTarget, ABILITY_SIMPLE);
                ReplaceAbility(gBattlerTarget, ABILITY_SIMPLE);
            }
            return;
        case VARIOUS_TRY_ENTRAINMENT:
            ptr = READ_PTR_INC;
            if (IsEntrainmentBannedAbilityAttacker(GetBattlerAbility(gBattlerAttacker)) ||
                IsEntrainmentTargetOrSimpleBeamBannedAbility(GetBattlerAbility(gBattlerTarget)) || DoesBattlerHaveAbilityShield(gBattlerTarget)) {
                gBattlescriptCurrInstr = ptr;
                return;
            }

            if (HasAbilityIgnoringSuppression(gBattlerTarget, GetBattlerAbility(gBattlerAttacker))) {
                gBattlescriptCurrInstr = ptr;
            } else {
                UpdateAbilityStateIndicesForNewAbility(gBattlerTarget, GetBattlerAbility(gBattlerAttacker));
                ReplaceAbility(gBattlerTarget, GetBattlerAbility(gBattlerAttacker));
            }
            return;
        case VARIOUS_COPY_ABILITY: {
            u8 battlerToGiveAbility = GetBattlerForBattleScript(READ_8_INC);
            ptr = READ_PTR_INC;
            if (IsEntrainmentBannedAbilityAttacker(GetBattlerAbility(gActiveBattler)) ||
                IsEntrainmentTargetOrSimpleBeamBannedAbility(GetBattlerAbility(battlerToGiveAbility)) ||
                HasAbilityIgnoringSuppression(gBattlerAttacker, GetBattlerAbility(gBattlerTarget)) || DoesBattlerHaveAbilityShield(battlerToGiveAbility)) {
                gBattlescriptCurrInstr = ptr;
                return;
            }
            UpdateAbilityStateIndicesForNewAbility(battlerToGiveAbility, GetBattlerAbility(gActiveBattler));
            ReplaceAbility(battlerToGiveAbility, GetBattlerAbility(gActiveBattler));
        }
            return;
        case VARIOUS_SET_LAST_USED_ABILITY:
            SetActiveAbilityPopupOverride(GetBattlerAbility(gActiveBattler));
            break;
        case VARIOUS_TRY_HEAL_PULSE:
            ptr = READ_PTR_INC;
            if (gBattleMons[gBattlerTarget].status1 & STATUS1_BLEED) {
                gBattleMoveDamage = 0;
            } else if (BATTLER_MAX_HP(gActiveBattler)) {
                gBattlescriptCurrInstr = ptr;
            } else {
                int megaLauncherBoosted = FALSE;
                if (IsMegaLauncherBoosted(gBattlerAttacker, gCurrentMove)) {
                    ON_ABILITY(gActiveBattler, FALSE, gAbilities[ability].megaLauncherBoost, megaLauncherBoosted = TRUE; break)
                }

                if (megaLauncherBoosted)
                    gBattleMoveDamage = -(gBattleMons[gActiveBattler].maxHP * 3 / 4);
                else
                    gBattleMoveDamage = -(gBattleMons[gActiveBattler].maxHP / 2);

                if (gBattleMoveDamage == 0) gBattleMoveDamage = -1;
            }
            return;
        case VARIOUS_TRY_QUASH:
            ptr = READ_PTR_INC;
            if (gFieldTimers.quashTimer) {
                gBattlescriptCurrInstr = ptr;
            } else {
                gFieldTimers.quashTimer = QUASH_DURATION;
                gFieldTimers.started.quash = TRUE;
            }
            return;
        case VARIOUS_INVERT_STAT_STAGES:
            for (i = 0; i < NUM_BATTLE_STATS; i++) {
                if (gBattleMons[gActiveBattler].statStages[i] < 6)  // Negative becomes positive.
                    gBattleMons[gActiveBattler].statStages[i] = 6 + (6 - gBattleMons[gActiveBattler].statStages[i]);
                else if (gBattleMons[gActiveBattler].statStages[i] > 6)  // Positive becomes negative.
                    gBattleMons[gActiveBattler].statStages[i] = 6 - (gBattleMons[gActiveBattler].statStages[i] - 6);
            }
            break;
        case VARIOUS_SET_TERRAIN:
            HandleTerrainMove(gBattleMoves[gCurrentMove].effect);
            return;
        case VARIOUS_TRY_ME_FIRST:
            ptr = READ_PTR_INC;
            if (GetBattlerTurnOrderNum(gBattlerAttacker) > GetBattlerTurnOrderNum(gBattlerTarget))
                gBattlescriptCurrInstr = ptr;
            else if (gBattleMoves[gBattleMons[gBattlerTarget].moves[gBattleStruct->chosenMovePositions[gBattlerTarget]]].power == 0)
                gBattlescriptCurrInstr = ptr;
            else {
                u16 move = gBattleMons[gBattlerTarget].moves[gBattleStruct->chosenMovePositions[gBattlerTarget]];
                switch (move) {
                    case MOVE_STRUGGLE:
                    case MOVE_CHATTER:
                    case MOVE_FOCUS_PUNCH:
                    case MOVE_THIEF:
                    case MOVE_COVET:
                    case MOVE_COUNTER:
                    case MOVE_MIRROR_COAT:
                    case MOVE_METAL_BURST:
                    case MOVE_ME_FIRST:
                    case MOVE_BEAK_BLAST:
                        gBattlescriptCurrInstr = ptr;
                        break;
                    default:
                        gQueuedExtraAttackData[++gQueuedAttackCount] = (struct ExtraAttackActionStruct){
                            .attacker = gBattlerAttacker,
                            .move = move,
                            .target = GetMoveTarget(gCalledMove, 0),
                            .movePos = MAX_MON_MOVES,
                            .prankster = BattlerHasAbility(gBattlerAttacker, ABILITY_PRANKSTER, FALSE),
                        };
                        gStatuses3[gBattlerAttacker] |= STATUS3_ME_FIRST;
                        break;
                }
            }
            return;
        case VARIOUS_JUMP_IF_BATTLE_END:
            ptr = READ_PTR_INC;
            if (NoAliveMonsForEitherParty()) gBattlescriptCurrInstr = ptr;
            return;
        case VARIOUS_TRY_ELECTRIFY:
            ptr = READ_PTR_INC;
            if (!ProtectSucceeds(gActiveBattler) || gCurrentTurnActionNumber >= GetBattlerTurnOrderNum(gBattlerTarget)) {
                gVolatileStructs[gActiveBattler].protectUses = 0;
                gBattlescriptCurrInstr = ptr;
            } else {
                gVolatileStructs[gActiveBattler].protectUses++;
                gStatuses4[gBattlerTarget] |= STATUS4_ELECTRIFIED;
            }
            return;
        case VARIOUS_TRY_REFLECT_TYPE:
            ptr = READ_PTR_INC;
            if (gBattleMons[gBattlerTarget].species == SPECIES_ARCEUS || gBattleMons[gBattlerTarget].species == SPECIES_SILVALLY) {
                gBattlescriptCurrInstr = ptr;
            } else if (gBattleMons[gBattlerTarget].type1 == TYPE_MYSTERY && gBattleMons[gBattlerTarget].type2 != TYPE_MYSTERY) {
                gBattleMons[gBattlerAttacker].type1 = gBattleMons[gBattlerTarget].type2;
                gBattleMons[gBattlerAttacker].type2 = gBattleMons[gBattlerTarget].type2;
            } else if (gBattleMons[gBattlerTarget].type1 != TYPE_MYSTERY && gBattleMons[gBattlerTarget].type2 == TYPE_MYSTERY) {
                gBattleMons[gBattlerAttacker].type1 = gBattleMons[gBattlerTarget].type1;
                gBattleMons[gBattlerAttacker].type2 = gBattleMons[gBattlerTarget].type1;
            } else if (gBattleMons[gBattlerTarget].type1 == TYPE_MYSTERY && gBattleMons[gBattlerTarget].type2 == TYPE_MYSTERY) {
                gBattlescriptCurrInstr = ptr;
            } else {
                gBattleMons[gBattlerAttacker].type1 = gBattleMons[gBattlerTarget].type1;
                gBattleMons[gBattlerAttacker].type2 = gBattleMons[gBattlerTarget].type2;
            }
            return;
        case VARIOUS_TRY_SOAK:
            ptr = READ_PTR_INC;
            if (gBattleMons[gBattlerTarget].type1 == gBattleMoves[gCurrentMove].type && gBattleMons[gBattlerTarget].type2 == gBattleMoves[gCurrentMove].type) {
                gBattlescriptCurrInstr = ptr;
            } else {
                SET_BATTLER_TYPE(gBattlerTarget, gBattleMoves[gCurrentMove].type);
                PREPARE_TYPE_BUFFER(gBattleTextBuff1, gBattleMoves[gCurrentMove].type);
            }
            return;
        case VARIOUS_HANDLE_MEGA_EVO:
            if (GetBattlerSide(gActiveBattler) == B_SIDE_OPPONENT)
                mon = &gEnemyParty[gBattlerPartyIndexes[gActiveBattler]];
            else
                mon = &gPlayerParty[gBattlerPartyIndexes[gActiveBattler]];

            // Change species.
            switch (READ_8_INC) {
                case 0: {
                    u16 megaSpecies;
                    gBattleStruct->mega.evolvedSpecies[gActiveBattler] = gBattleMons[gActiveBattler].species;
                    if (GetBattlerSide(gActiveBattler) == B_SIDE_PLAYER) {
                        gBattleStruct->mega.playerBaseSpecies[gBattlerPartyIndexes[gActiveBattler]] = gBattleMons[gActiveBattler].species;
                    }
                    if (GetBattlerPosition(gActiveBattler) == B_POSITION_PLAYER_LEFT ||
                        (GetBattlerPosition(gActiveBattler) == B_POSITION_PLAYER_RIGHT &&
                         !(gBattleTypeFlags & (BATTLE_TYPE_MULTI | BATTLE_TYPE_INGAME_PARTNER)))) {
                        gBattleStruct->mega.playerEvolvedSpecies = gBattleStruct->mega.evolvedSpecies[gActiveBattler];
                    }
                    // Checks regular Mega Evolution
                    megaSpecies = GetMegaEvolutionSpecies(gBattleStruct->mega.evolvedSpecies[gActiveBattler], gBattleMons[gActiveBattler].item);
                    // Checks Wish Mega Evolution
                    if (megaSpecies == SPECIES_NONE) {
                        megaSpecies = GetWishMegaEvolutionSpecies(gBattleStruct->mega.evolvedSpecies[gActiveBattler],
                                                                  gBattleMons[gActiveBattler].moves[0],
                                                                  gBattleMons[gActiveBattler].moves[1],
                                                                  gBattleMons[gActiveBattler].moves[2],
                                                                  gBattleMons[gActiveBattler].moves[3]);
                    }

                    UpdateAbilityStateIndicesForNewSpecies(gActiveBattler, megaSpecies);

                    gBattleMons[gActiveBattler].species = megaSpecies;
                    PREPARE_SPECIES_BUFFER(gBattleTextBuff1, gBattleMons[gActiveBattler].species);

                    BtlController_EmitSetMonData(
                        0, REQUEST_SPECIES_BATTLE, gBitTable[gBattlerPartyIndexes[gActiveBattler]], 2, &gBattleMons[gActiveBattler].species);
                    MarkBattlerForControllerExec(gActiveBattler);
                } break;
                // Change stats.
                case 1:
                    RecalcBattlerStats(gActiveBattler, mon);
                    if (ItemId_GetHoldEffect(gBattleMons[gActiveBattler].item) != HOLD_EFFECT_PRIMAL_ORB) {
                        gBattleStruct->mega.alreadyEvolved[GetBattlerPosition(gActiveBattler)] = TRUE;
                        gBattleStruct->mega.evolvedPartyIds[GetBattlerSide(gActiveBattler)] |= gBitTable[gBattlerPartyIndexes[gActiveBattler]];
                    }
                    break;
                // Update healthbox and elevation.
                case 2:
                    UpdateHealthboxAttribute(gHealthboxSpriteIds[gActiveBattler], mon, HEALTHBOX_ALL);
                    if (GetBattlerSide(gActiveBattler) == B_SIDE_OPPONENT) SetBattlerShadowSpriteCallback(gActiveBattler, gBattleMons[gActiveBattler].species);
                    break;
            }
            return;
        case VARIOUS_HANDLE_PRIMAL_REVERSION:
            if (GetBattlerSide(gActiveBattler) == B_SIDE_OPPONENT)
                mon = &gEnemyParty[gBattlerPartyIndexes[gActiveBattler]];
            else
                mon = &gPlayerParty[gBattlerPartyIndexes[gActiveBattler]];

            // Change species.
            switch (READ_8_INC) {
                case 0: {
                    u16 primalSpecies;
                    gBattleStruct->mega.primalRevertedSpecies[gActiveBattler] = gBattleMons[gActiveBattler].species;
                    if (GetBattlerSide(gActiveBattler) == B_SIDE_PLAYER) {
                        gBattleStruct->mega.playerBaseSpecies[gBattlerPartyIndexes[gActiveBattler]] = gBattleMons[gActiveBattler].species;
                    }

                    if (GetBattlerPosition(gActiveBattler) == B_POSITION_PLAYER_LEFT ||
                        (GetBattlerPosition(gActiveBattler) == B_POSITION_PLAYER_RIGHT &&
                         !(gBattleTypeFlags & (BATTLE_TYPE_MULTI | BATTLE_TYPE_INGAME_PARTNER)))) {
                        gBattleStruct->mega.playerPrimalRevertedSpecies = gBattleStruct->mega.primalRevertedSpecies[gActiveBattler];
                    }

                    // Checks Primal Reversion
                    primalSpecies = GetPrimalReversionSpecies(gBattleStruct->mega.primalRevertedSpecies[gActiveBattler], gBattleMons[gActiveBattler].item);

                    UpdateAbilityStateIndicesForNewSpecies(gActiveBattler, primalSpecies);

                    gBattleMons[gActiveBattler].species = primalSpecies;
                    PREPARE_SPECIES_BUFFER(gBattleTextBuff1, gBattleMons[gActiveBattler].species);

                    BtlController_EmitSetMonData(
                        0, REQUEST_SPECIES_BATTLE, gBitTable[gBattlerPartyIndexes[gActiveBattler]], 2, &gBattleMons[gActiveBattler].species);
                    MarkBattlerForControllerExec(gActiveBattler);

                    switch (primalSpecies) {
                        case SPECIES_GIRATINA_ORIGIN:
                        case SPECIES_PALKIA_ORIGIN:
                        case SPECIES_DIALGA_ORIGIN:
                            SetActiveMultistringChooser(B_MSG_ORIGIN_REVERSION);
                            break;

                        case SPECIES_ZAMAZENTA_CROWNED_SHIELD:
                        case SPECIES_ZACIAN_CROWNED_SWORD:
                            SetActiveMultistringChooser(B_MSG_CROWNED_REVERSION);
                            break;

                        default:
                            SetActiveMultistringChooser(B_MSG_PRIMAL_REVERSION);
                            break;
                    }
                    break;
                }
                // Change stats.
                case 1:
                    RecalcBattlerStats(gActiveBattler, mon);
                    gBattleStruct->mega.primalRevertedPartyIds[GetBattlerSide(gActiveBattler)] |= gBitTable[gBattlerPartyIndexes[gActiveBattler]];
                    break;
                // Update healthbox and elevation.
                default:
                    UpdateHealthboxAttribute(gHealthboxSpriteIds[gActiveBattler], mon, HEALTHBOX_ALL);
                    if (GetBattlerSide(gActiveBattler) == B_SIDE_OPPONENT) SetBattlerShadowSpriteCallback(gActiveBattler, gBattleMons[gActiveBattler].species);
                    break;
            }
            return;
        case VARIOUS_HANDLE_FORM_CHANGE:
            if (GetBattlerSide(gActiveBattler) == B_SIDE_OPPONENT)
                mon = &gEnemyParty[gBattlerPartyIndexes[gActiveBattler]];
            else
                mon = &gPlayerParty[gBattlerPartyIndexes[gActiveBattler]];

            // Change species.
            switch (READ_8_INC) {
                case 0:
                    if (!gBattleTextBuff1[0]) PREPARE_SPECIES_BUFFER(gBattleTextBuff1, gBattleMons[gActiveBattler].species);
                    BtlController_EmitSetMonData(
                        0, REQUEST_SPECIES_BATTLE, gBitTable[gBattlerPartyIndexes[gActiveBattler]], 2, &gBattleMons[gActiveBattler].species);
                    MarkBattlerForControllerExec(gActiveBattler);
                    break;
                // Change stats.
                case 1:
                    RecalcBattlerStats(gActiveBattler, mon);
                    break;
                // Update healthbox.
                default:
                    UpdateHealthboxAttribute(gHealthboxSpriteIds[gActiveBattler], mon, HEALTHBOX_ALL);
                    break;
            }
            return;
        case VARIOUS_TRY_LAST_RESORT:
            ptr = READ_PTR_INC;
            if (!CanUseLastResort(gActiveBattler)) gBattlescriptCurrInstr = ptr;
            return;
        case VARIOUS_TRY_HIT_SWITCH_TARGET:
            ptr = READ_PTR_INC;
            if (IsBattlerAlive(gBattlerAttacker) && IsBattlerAlive(gBattlerTarget) && !(gMoveResultFlags & MOVE_RESULT_NO_EFFECT) && TARGET_TURN_DAMAGED) {
                gBattleScripting.switchCase = B_SWITCH_HIT;
                gBattlescriptCurrInstr = BattleScript_ForceRandomSwitch;
            } else {
                gBattlescriptCurrInstr = ptr;
            }
            return;
        case VARIOUS_TRY_AUTOTOMIZE:
            ptr = READ_PTR_INC;
            if (GetBattlerWeight(gActiveBattler) > 1) {
                gVolatileStructs[gActiveBattler].autotomizeCount++;
            } else {
                gBattlescriptCurrInstr = ptr;
            }
            return;
        case VARIOUS_TRY_COPYCAT:
            ptr = READ_PTR_INC;
            switch (gBattleMoves[gLastUsedMove].effect) {
                case EFFECT_PLACEHOLDER:
                case EFFECT_PROTECT:
                case EFFECT_DO_NOTHING:
                case EFFECT_HIT_SWITCH_TARGET:
                case EFFECT_ROAR:
                case EFFECT_FOLLOW_ME:
                case EFFECT_TRICK:
                    gBattlescriptCurrInstr = ptr;
                    return;
            }

            if (gLastUsedMove == 0xFFFF || gBattleMoves[gLastUsedMove].copycatBanned) {
                gBattlescriptCurrInstr = ptr;
            } else {
                gCalledMove = gLastUsedMove;
                gHitMarker &= ~(HITMARKER_ATTACKSTRING_PRINTED);
                gBattlerTarget = GetMoveTarget(gCalledMove, 0);
            }
            return;
        case VARIOUS_TRY_INSTRUCT:
            ptr = READ_PTR_INC;
            for (i = 0; sMoveEffectsForbiddenToInstruct[i] != FORBIDDEN_INSTRUCT_END; i++) {
                if (sMoveEffectsForbiddenToInstruct[i] == gBattleMoves[gLastMoves[gActiveBattler]].effect) break;
            }
            if (gLastMoves[gActiveBattler] == 0 || gLastMoves[gActiveBattler] == 0xFFFF || sMoveEffectsForbiddenToInstruct[i] != FORBIDDEN_INSTRUCT_END ||
                gLastMoves[gActiveBattler] == MOVE_STRUGGLE || gLastMoves[gActiveBattler] == MOVE_KINGS_SHIELD) {
                gBattlescriptCurrInstr = ptr;
            } else {
                for (i = 0; i < MAX_MON_MOVES; i++) {
                    if (gBattleMons[gActiveBattler].moves[i] == gLastMoves[gActiveBattler]) {
                        break;
                    }
                }

                if (i >= MAX_MON_MOVES || gBattleMons[gActiveBattler].pp[i] == 0)
                    gBattlescriptCurrInstr = ptr;
                else {
                    gQueuedExtraAttackData[++gQueuedAttackCount] = (struct ExtraAttackActionStruct){
                        .attacker = gActiveBattler,
                        .target = GetMoveTarget(gLastMoves[gActiveBattler], 0),
                        .move = gLastMoves[gActiveBattler],
                        .movePos = i,
                    };
                }
            }
            return;
        case VARIOUS_ABILITY_POPUP:
            CreateAbilityPopUp(gActiveBattler, GetBattlerAbility(gActiveBattler), (gBattleTypeFlags & BATTLE_TYPE_DOUBLE) != 0);
            break;
        case VARIOUS_UPDATE_ABILITY_POPUP:
            UpdateAbilityPopup(gActiveBattler);
            break;
        case VARIOUS_EXTRASKILL_POPUP:
            CreateExtraSkillPopUp();
            break;
        case VARIOUS_DEFOG: {
            int clear = READ_8_INC;
            ptr = READ_PTR_INC;
            if (clear)  // Clear
            {
                const u8* continuePtr = gBattlescriptCurrInstr;
                gBattlescriptCurrInstr = runAgain;
                if (!ClearDefogHazards(gEffectBattler, TRUE)) gBattlescriptCurrInstr = continuePtr;
            } else {
                if (!ClearDefogHazards(gActiveBattler, FALSE)) gBattlescriptCurrInstr = ptr;
            }
        }
            return;
        case VARIOUS_JUMP_IF_TARGET_ALLY:
            ptr = READ_PTR_INC;
            if (GetBattlerSide(gBattlerAttacker) == GetBattlerSide(gBattlerTarget)) gBattlescriptCurrInstr = ptr;
            return;
        case VARIOUS_TRY_SYNCHRONOISE:
            ptr = READ_PTR_INC;
            if (!DoBattlersShareType(gBattlerAttacker, gBattlerTarget)) gBattlescriptCurrInstr = ptr;
            return;
        case VARIOUS_LOSE_TYPE: {
            int type = READ_8_INC;
            u8 typeToLose = type == TYPE_CURRENT_MOVE ? gBattleMoves[gCurrentMove].type : type;
            for (i = 0; i < 3; i++) {
                if (*(u8*)(&gBattleMons[gActiveBattler].type1 + i) == typeToLose) *(u8*)(&gBattleMons[gActiveBattler].type1 + i) = TYPE_MYSTERY;
            }
            switch (typeToLose) {
                case TYPE_ELECTRIC:
                    SetActiveMultistringChooser(B_MSG_BURNUP_ELECTRIC);
                    break;

                case TYPE_DARK:
                    SetActiveMultistringChooser(B_MSG_BURNUP_DARK);
                    break;

                default:
                    SetActiveMultistringChooser(B_MSG_BURNUP_FIRE);
                    break;
            }
            return;
        }
        case VARIOUS_PSYCHO_SHIFT:
            i = TRUE;
            ptr = READ_PTR_INC;
            if (gBattleMons[gBattlerAttacker].status1 & STATUS1_PARALYSIS && !(gBattleMons[gBattlerTarget].status1 & STATUS1_PARALYSIS)) {
                if (!CanBeParalyzed(gBattlerAttacker, gBattlerTarget)) {
                    BattleScriptPush(ptr);
                    gBattlescriptCurrInstr = BattleScript_PRLZPrevention;
                    i = FALSE;
                } else {
                    SetActiveMultistringChooser(B_MSG_PKMNWASPARALYZED);
                }
            } else if (gBattleMons[gBattlerAttacker].status1 & STATUS1_PSN_ANY && !(gBattleMons[gBattlerTarget].status1 & STATUS1_PSN_ANY)) {
                if (!CanBeParalyzed(gBattlerAttacker, gBattlerTarget)) {
                    BattleScriptPush(ptr);
                    gBattlescriptCurrInstr = BattleScript_PSNPrevention;
                    i = FALSE;
                } else {
                    if (gBattleMons[gBattlerAttacker].status1 & STATUS1_POISON)
                        SetActiveMultistringChooser(B_MSG_PKMNWASPOISONED);
                    else
                        SetActiveMultistringChooser(B_MSG_PKMNBADLYPOISONED);
                }
            } else if (gBattleMons[gBattlerAttacker].status1 & STATUS1_BURN && !(gBattleMons[gBattlerTarget].status1 & STATUS1_BURN)) {
                if (!CanBeBurned(gBattlerTarget)) {
                    BattleScriptPush(ptr);
                    gBattlescriptCurrInstr = BattleScript_BRNPrevention;
                    i = FALSE;
                } else {
                    SetActiveMultistringChooser(B_MSG_PKMNWASBURNED);
                }
            } else if (gBattleMons[gBattlerAttacker].status1 & STATUS1_SLEEP && CanSleep(gBattlerTarget)) {
                SetActiveMultistringChooser(B_MSG_PKMNFELLASLEEP);
            } else if ((gBattleMons[gBattlerAttacker].status1 & STATUS1_FROSTBITE) && CanGetFrostbite(gBattlerTarget)) {
                SetActiveMultistringChooser(B_MSG_PKMNGOTFROSTBITE);
            } else if ((gBattleMons[gBattlerAttacker].status1 & STATUS1_BLEED) && CanBleed(gBattlerTarget)) {
                SetActiveMultistringChooser(B_MSG_PKMNSTARTBLEED);
            }
            if (i == TRUE) {
                gBattleMons[gBattlerTarget].status1 = gBattleMons[gBattlerAttacker].status1 & STATUS1_ANY;
                gActiveBattler = gBattlerTarget;
                BtlController_EmitSetMonData(0, REQUEST_STATUS_BATTLE, 0, 4, &gBattleMons[gActiveBattler].status1);
                MarkBattlerForControllerExec(gActiveBattler);
            }
            return;
        case VARIOUS_CURE_STATUS:
            gBattleMons[gActiveBattler].status1 = 0;
            BtlController_EmitSetMonData(0, REQUEST_STATUS_BATTLE, 0, 4, &gBattleMons[gActiveBattler].status1);
            MarkBattlerForControllerExec(gActiveBattler);
            break;
        case VARIOUS_POWER_TRICK:
            gStatuses3[gActiveBattler] ^= STATUS3_POWER_TRICK;
            SWAP(gBattleMons[gActiveBattler].attack, gBattleMons[gActiveBattler].defense, i);
            SWAP(gBattleMons[gActiveBattler].statStages[STAT_ATK], gBattleMons[gActiveBattler].statStages[STAT_DEF], i);
            break;
        case VARIOUS_AFTER_YOU:
            ptr = READ_PTR_INC;
            if (gCurrentTurnActionNumber >= GetBattlerTurnOrderNum(gActiveBattler)) {
                gBattlescriptCurrInstr = ptr;
            } else {
                gRoundStructs[gActiveBattler].afterYou = TRUE;
            }
            return;
        case VARIOUS_BESTOW:
            ptr = READ_PTR_INC;
            if (gBattleMons[gBattlerAttacker].item == ITEM_NONE || !CanBattlerGetOrLoseItem(gBattlerAttacker, gBattleMons[gBattlerAttacker].item) ||
                !CanBattlerGetOrLoseItem(gBattlerTarget, gBattleMons[gBattlerAttacker].item)) {
                gBattlescriptCurrInstr = ptr;
            } else {
                gLastUsedItem = UpdateBattlerItem(gBattlerAttacker, ITEM_NONE);
                UpdateBattlerItem(gBattlerTarget, gLastUsedItem);
            }
            return;
        case VARIOUS_ARGUMENT_TO_MOVE_EFFECT:
            gBattleScripting.moveEffect = gBattleMoves[gCurrentMove].argument;
            break;
        case VARIOUS_JUMP_IF_NOT_GROUNDED:
            ptr = READ_PTR_INC;
            if (!IsBattlerGrounded(gActiveBattler)) gBattlescriptCurrInstr = ptr;
            return;
        case VARIOUS_HANDLE_TRAINER_SLIDE_MSG: {
            int state = READ_8_INC;
            if (state == 0) {
                gTurnStructs[gBattlerAttacker].savedDmg = gBattlerSpriteIds[gActiveBattler];
                HideBattlerShadowSprite(gActiveBattler);
            } else if (state == 1) {
                BtlController_EmitPrintString(0, STRINGID_TRAINERSLIDE);
                MarkBattlerForControllerExec(gActiveBattler);
            } else {
                gBattlerSpriteIds[gActiveBattler] = gTurnStructs[gBattlerAttacker].savedDmg;
                if (gBattleMons[gActiveBattler].hp != 0) {
                    SetBattlerShadowSpriteCallback(gActiveBattler, gBattleMons[gActiveBattler].species);
                    BattleLoadOpponentMonSpriteGfx(&gEnemyParty[gBattlerPartyIndexes[gActiveBattler]], gActiveBattler);
                }
            }
        }
            return;
        case VARIOUS_TRY_TRAINER_SLIDE_MSG_FIRST_OFF:
            if (ShouldDoTrainerSlide(gActiveBattler, gTrainerBattleOpponent_A, TRAINER_SLIDE_FIRST_DOWN)) {
                BattleScriptCall(BattleScript_TrainerSlideMsgRet);
                return;
            }
            break;
        case VARIOUS_TRY_TRAINER_SLIDE_MSG_LAST_ON:
            if (ShouldDoTrainerSlide(gActiveBattler, gTrainerBattleOpponent_A, TRAINER_SLIDE_LAST_SWITCHIN)) {
                BattleScriptCall(BattleScript_TrainerSlideMsgRet);
                return;
            }
            break;
        case VARIOUS_SET_AURORA_VEIL:
            if (gSideStatuses[GET_BATTLER_SIDE(gActiveBattler)] & SIDE_STATUS_AURORA_VEIL || !(WEATHER_HAS_EFFECT && gBattleWeather & WEATHER_HAIL_ANY)) {
                gMoveResultFlags |= MOVE_RESULT_MISSED;
                SetActiveMultistringChooser(B_MSG_SIDE_STATUS_FAILED);
            } else {
                int side = GET_BATTLER_SIDE(gActiveBattler);
                gSideTimers[side].started.auroraVeil = TRUE;
                gSideStatuses[side] |= SIDE_STATUS_AURORA_VEIL;
                if (GetBattlerHoldEffect(gActiveBattler, TRUE) == HOLD_EFFECT_LIGHT_CLAY)
                    gSideTimers[side].auroraVeilTimer = SCREEN_DURATION_EXTENDED;
                else
                    gSideTimers[side].auroraVeilTimer = SCREEN_DURATION;
                gSideTimers[side].auroraVeilBattlerId = gActiveBattler;

                SetActiveMultistringChooser(B_MSG_SET_SAFEGUARD);
            }
            break;
        case VARIOUS_TRY_THIRD_TYPE:
            ptr = READ_PTR_INC;
            if (IS_BATTLER_OF_TYPE(gActiveBattler, gBattleMoves[gCurrentMove].argument)) {
                if (gBattleMoves[gCurrentMove].argument == TYPE_GHOST) {
                    gVolatileStructs[gActiveBattler].trickOrTreat = TRUE;
                    gBattleMons[gActiveBattler].type3 = gBattleMoves[gCurrentMove].argument;
                    PREPARE_TYPE_BUFFER(gBattleTextBuff1, gBattleMoves[gCurrentMove].argument);
                } else
                    gBattlescriptCurrInstr = ptr;
            } else {
                gBattleMons[gActiveBattler].type3 = gBattleMoves[gCurrentMove].argument;
                PREPARE_TYPE_BUFFER(gBattleTextBuff1, gBattleMoves[gCurrentMove].argument);
            }
            return;
        case VARIOUS_DESTROY_ABILITY_POPUP:
            DestroyAbilityPopUp(gActiveBattler);
            break;
        case VARIOUS_TOTEM_BOOST:
            ptr = READ_PTR_INC;
            if (gTotemBoosts[gActiveBattler].stats) {
                for (i = 0; i < (NUM_BATTLE_STATS - 1); i++) {
                    if (gTotemBoosts[gActiveBattler].stats & (1 << i)) {
                        SetActiveStatChanger(i + 1, gTotemBoosts[gActiveBattler].statChanges[i]);

                        gTotemBoosts[gActiveBattler].stats &= ~(1 << i);
                        gBattleScripting.battler = gActiveBattler;
                        gBattlerTarget = gActiveBattler;
                        if (gTotemBoosts[gActiveBattler].stats & 0x80) {
                            gTotemBoosts[gActiveBattler].stats &= ~0x80;  // set 'aura flared to life' flag
                            gBattlescriptCurrInstr = BattleScript_TotemFlaredToLife;
                        } else {
                            gBattlescriptCurrInstr = ptr;  // do boost
                        }
                        return;
                    }
                }
            }
            return;
        case VARIOUS_MOVEEND_ITEM_EFFECTS:
            if (ItemBattleEffects(1, gActiveBattler, FALSE)) return;
            break;
        case VARIOUS_ROOM_SERVICE:
            if (GetBattlerHoldEffect(gActiveBattler, TRUE) == HOLD_EFFECT_ROOM_SERVICE && TryRoomService(gActiveBattler)) {
                gStackBattler1 = gActiveBattler;
                BattleScriptCall(BattleScript_BerryStatRaiseRet);
            }
            return;
        case VARIOUS_SET_BEAK_BLAST:
            if (gBattleMoves[gCurrentMove].argument == MOVE_EFFECT_PARALYSIS)
                gRoundStructs[gActiveBattler].freezeShockCharge = TRUE;
            else
                gRoundStructs[gActiveBattler].iceBurnCharge = TRUE;
            break;
        case VARIOUS_TERRAIN_SEED:
            if (GetBattlerHoldEffect(gActiveBattler, TRUE) == HOLD_EFFECT_SEEDS) {
                u16 item = gBattleMons[gActiveBattler].item;
                switch (GetBattlerHoldEffectParam(gActiveBattler)) {
                    case HOLD_EFFECT_PARAM_ELECTRIC_TERRAIN:
                        TryHandleSeed(gActiveBattler, STATUS_FIELD_ELECTRIC_TERRAIN, STAT_DEF, item, FALSE);
                        break;
                    case HOLD_EFFECT_PARAM_GRASSY_TERRAIN:
                        TryHandleSeed(gActiveBattler, STATUS_FIELD_GRASSY_TERRAIN, STAT_DEF, item, FALSE);
                        break;
                    case HOLD_EFFECT_PARAM_MISTY_TERRAIN:
                        TryHandleSeed(gActiveBattler, STATUS_FIELD_MISTY_TERRAIN, STAT_SPDEF, item, FALSE);
                        break;
                    case HOLD_EFFECT_PARAM_PSYCHIC_TERRAIN:
                        TryHandleSeed(gActiveBattler, STATUS_FIELD_PSYCHIC_TERRAIN, STAT_SPDEF, item, FALSE);
                        break;
                }
            }
            return;
        case VARIOUS_MAKE_INVISIBLE:
            if (gBattleControllerExecFlags) break;

            BtlController_EmitSpriteInvisibility(0, TRUE);
            MarkBattlerForControllerExec(gActiveBattler);
            break;
        case VARIOUS_JUMP_IF_TERRAIN_AFFECTED: {
            u32 flags = READ_32_INC;
            ptr = READ_PTR_INC;
            if (IsBattlerTerrainAffected(gActiveBattler, flags)) gBattlescriptCurrInstr = ptr;
        }
            return;
        case VARIOUS_EERIE_SPELL_PP_REDUCE:
            ptr = READ_PTR_INC;
            if (gLastMoves[gActiveBattler] != 0 && gLastMoves[gActiveBattler] != 0xFFFF) {
                s32 i;

                for (i = 0; i < MAX_MON_MOVES; i++) {
                    if (gLastMoves[gActiveBattler] == gBattleMons[gActiveBattler].moves[i]) break;
                }

                if (i != MAX_MON_MOVES && gBattleMons[gActiveBattler].pp[i] != 0) {
                    s32 ppToDeduct = gBattleMoves[gCurrentMove].argument;

                    if (gBattleMons[gActiveBattler].pp[i] < ppToDeduct) ppToDeduct = gBattleMons[gActiveBattler].pp[i];

                    PREPARE_MOVE_BUFFER(gBattleTextBuff1, gLastMoves[gActiveBattler])
                    ConvertIntToDecimalStringN(gBattleTextBuff2, ppToDeduct, STR_CONV_MODE_LEFT_ALIGN, 1);
                    PREPARE_BYTE_NUMBER_BUFFER(gBattleTextBuff2, 1, ppToDeduct)
                    gBattleMons[gActiveBattler].pp[i] -= ppToDeduct;
                    if (!(gVolatileStructs[gActiveBattler].mimickedMoves & gBitTable[i]) && !(gBattleMons[gActiveBattler].status2 & STATUS2_TRANSFORMED)) {
                        BtlController_EmitSetMonData(0, REQUEST_PPMOVE1_BATTLE + i, 0, 1, &gBattleMons[gActiveBattler].pp[i]);
                        MarkBattlerForControllerExec(gActiveBattler);
                    }

                    if (gBattleMons[gActiveBattler].pp[i] == 0) CancelMultiTurnMoves(gActiveBattler);
                } else {
                    gBattlescriptCurrInstr = ptr;  // cant reduce pp
                }
            } else {
                gBattlescriptCurrInstr = ptr;  // cant reduce pp
            }
            return;
        case VARIOUS_JUMP_IF_TEAM_HEALTHY:
            ptr = READ_PTR_INC;
            if ((gBattleTypeFlags & BATTLE_TYPE_DOUBLE) && IsBattlerAlive(BATTLE_PARTNER(gActiveBattler))) {
                u8 partner = BATTLE_PARTNER(gActiveBattler);
                if ((gBattleMons[gActiveBattler].hp == gBattleMons[gActiveBattler].maxHP && !(gBattleMons[gActiveBattler].status1 & STATUS1_ANY)) &&
                    (gBattleMons[partner].hp == gBattleMons[partner].maxHP && !(gBattleMons[partner].status1 & STATUS1_ANY)))
                    gBattlescriptCurrInstr = ptr;  // fail
            } else                                 // single battle
            {
                if (gBattleMons[gActiveBattler].hp == gBattleMons[gActiveBattler].maxHP && !(gBattleMons[gActiveBattler].status1 & STATUS1_ANY))
                    gBattlescriptCurrInstr = ptr;  // fail
            }
            return;
        case VARIOUS_TRY_HEAL_PERCENT_HP:
            increase = READ_8_INC;
            ptr = READ_PTR_INC;
            gBattleMoveDamage = gBattleMons[gActiveBattler].maxHP * increase / 100;
            if (gBattleMoveDamage == 0) gBattleMoveDamage = 1;
            gBattleMoveDamage *= -1;

            if (gBattleMons[gActiveBattler].hp == gBattleMons[gActiveBattler].maxHP)
                gBattlescriptCurrInstr = ptr;  // fail
            else if (!CanBattlerHeal(gActiveBattler))
                gBattlescriptCurrInstr = ptr;  // fail
            return;
        case VARIOUS_REMOVE_TERRAIN:
            gFieldTimers.terrainTimer = 0;
            switch (gFieldStatuses & STATUS_FIELD_TERRAIN_ANY) {
                case STATUS_FIELD_MISTY_TERRAIN:
                    SetActiveMultistringChooser(B_MSG_MISTYTERRAINENDS);
                    break;
                case STATUS_FIELD_GRASSY_TERRAIN:
                    SetActiveMultistringChooser(B_MSG_GRASSYTERRAINENDS);
                    break;
                case STATUS_FIELD_ELECTRIC_TERRAIN:
                    SetActiveMultistringChooser(B_MSG_ELECTRICTERRAINENDS);
                    break;
                case STATUS_FIELD_PSYCHIC_TERRAIN:
                    SetActiveMultistringChooser(B_MSG_PSYCHICTERRAINENDS);
                    break;
                default:
                    SetActiveMultistringChooser(B_MSG_PSYCHICTERRAINENDS + 1);  // failsafe
                    break;
            }
            gFieldStatuses &= ~STATUS_FIELD_TERRAIN_ANY;  // remove the terrain
            break;
        case VARIOUS_REMOVE_WEATHER:
            if (gBattleWeather & WEATHER_SUN_PRIMAL)
                SetActiveMultistringChooser(B_MSG_HARSH_SUNLIGHT_CANT_END);
            else if (gBattleWeather & WEATHER_RAIN_PRIMAL)
                SetActiveMultistringChooser(B_MSG_HEAVY_RAIN_WONT_END);
            else if (gBattleWeather & WEATHER_STRONG_WINDS)
                SetActiveMultistringChooser(B_MSG_WIND_WONT_END);
            else {
                gWishFutureKnock.weatherDuration = 0;
                if (gBattleWeather & WEATHER_SUN_ANY)
                    SetActiveMultistringChooser(B_MSG_SUN_ENDS);
                else if (gBattleWeather & WEATHER_RAIN_ANY)
                    SetActiveMultistringChooser(B_MSG_RAIN_ENDS);
                else if (gBattleWeather & WEATHER_SANDSTORM_ANY)
                    SetActiveMultistringChooser(B_MSG_SAND_ENDS);
                else if (gBattleWeather & WEATHER_HAIL_ANY)
                    SetActiveMultistringChooser(B_MSG_HAIL_ENDS);
                else
                    SetActiveMultistringChooser(B_MSG_REMOVE_WEATHER_FAILED);
                gBattleWeather = 0;
            }
            break;
        case VARIOUS_JUMP_IF_PRANKSTER_BLOCKED:
            ptr = READ_PTR_INC;
            if (BlocksPrankster(gCurrentMove, gBattlerAttacker, gActiveBattler, TRUE)) gBattlescriptCurrInstr = ptr;
            return;
        case VARIOUS_TRY_TO_CLEAR_PRIMAL_WEATHER: {
            if (!FlagGet(FLAG_PERMANENT_UNCHANGEABLE_WEATHER)) {  // Monotype Champ
                if (gBattleWeather & WEATHER_SUN_PRIMAL && !IsAbilityOnField(ABILITY_DESOLATE_LAND)) {
                    gBattleWeather &= ~WEATHER_SUN_PRIMAL;
                    PrepareStringBattle(STRINGID_EXTREMESUNLIGHTFADED, gActiveBattler);
                    gBattleCommunication[MSG_DISPLAY] = 1;
                } else if (gBattleWeather & WEATHER_RAIN_PRIMAL && !IsAbilityOnField(ABILITY_PRIMORDIAL_SEA)) {
                    gBattleWeather &= ~WEATHER_RAIN_PRIMAL;
                    PrepareStringBattle(STRINGID_HEAVYRAINLIFTED, gActiveBattler);
                    gBattleCommunication[MSG_DISPLAY] = 1;
                } else if (gBattleWeather & WEATHER_STRONG_WINDS && !IsAbilityOnField(ABILITY_DELTA_STREAM)) {
                    gBattleWeather &= ~WEATHER_STRONG_WINDS;
                    PrepareStringBattle(STRINGID_STRONGWINDSDISSIPATED, gActiveBattler);
                    gBattleCommunication[MSG_DISPLAY] = 1;
                }
            }
            break;
        }
        case VARIOUS_TRY_END_NEUTRALIZING_GAS:
            REQUIRE(gFieldTimers.neutralizingGas)
            REQUIRE(BattlerHasAbility(gActiveBattler, ABILITY_NEUTRALIZING_GAS, FALSE))
            REQUIRE_NOT(IsAbilityOnFieldExcept(gActiveBattler, ABILITY_NEUTRALIZING_GAS))
            gFieldTimers.neutralizingGas = FALSE;
            gBattleMons[gActiveBattler].abilities[GetAbilityIndex(gActiveBattler, ABILITY_NEUTRALIZING_GAS, FALSE)] = ABILITY_NONE;
            {
                u8 battlers[MAX_BATTLERS_COUNT - 1];
                int count = SortBattlersExcept(battlers, TRUE, 1 << gActiveBattler);
                for (i = count - 1; i >= 0; i--) {
                    gStackBattler1 = battlers[i];
                    BattleScriptCall(BattleScript_DoSingleSwitchIn);
                }
                BattleScriptCall(BattleScript_NeutralizingGasExits);
            }
            break;
        case VARIOUS_GET_ROTOTILLER_TARGETS:
            // Gets the battlers to be affected by rototiller. If there are none, print 'But it failed!'
            {
                u32 count = 0;
                ptr = READ_PTR_INC;
                for (i = 0; i < gBattlersCount; i++) {
                    gTurnStructs[i].rototillerAffected = FALSE;
                    if (IsRototillerAffected(i)) {
                        gTurnStructs[i].rototillerAffected = TRUE;
                        count++;
                    }
                }

                if (count == 0) gBattlescriptCurrInstr = ptr;  // Rototiller fails
            }
            return;
        case VARIOUS_JUMP_IF_NOT_ROTOTILLER_AFFECTED:
            ptr = READ_PTR_INC;
            if (gTurnStructs[gActiveBattler].rototillerAffected) {
                gTurnStructs[gActiveBattler].rototillerAffected = FALSE;
            } else {
                gBattlescriptCurrInstr = ptr;  // Unaffected by rototiller - print STRINGID_NOEFFECTONTARGET
            }
            return;
        case VARIOUS_TRY_ACTIVATE_BATTLE_BOND:
            break;
        case VARIOUS_CONSUME_BERRY: {
            int restoreItem = READ_8_INC;
            if (ItemId_GetHoldEffect(gBattleMons[gActiveBattler].item) == HOLD_EFFECT_NONE) {
                return;
            }

            gBattleScripting.battler = gEffectBattler = gBattlerTarget =
                gActiveBattler;  // Cover all berry effect battlerId cases. e.g. ChangeStatBuffs uses target ID
            // Do move end berry effects for just a single battler, instead of looping through all battlers
            if (ItemBattleEffects(ITEMEFFECT_BATTLER_MOVE_END, gActiveBattler, FALSE)) return;

            if (restoreItem) {
                gBattleMons[gActiveBattler].item = gBattleStruct->changedItems[gActiveBattler];
                gBattleStruct->changedItems[gActiveBattler] = ITEM_NONE;
                gBattleResources->flags->flags[gActiveBattler] &= ~(RESOURCE_FLAG_UNBURDEN);
            }
        }
            return;
        case VARIOUS_JUMP_IF_CANT_REVERT_TO_PRIMAL: {
            bool8 canDoPrimalReversion = FALSE;
            ptr = READ_PTR_INC;

            for (i = 0; i < EVOS_PER_MON; i++) {
                if (gEvolutionTable[gBattleMons[gActiveBattler].species][i].method == EVO_PRIMAL_REVERSION &&
                    gEvolutionTable[gBattleMons[gActiveBattler].species][i].param == gBattleMons[gActiveBattler].item)
                    canDoPrimalReversion = TRUE;
            }

            if (!canDoPrimalReversion) gBattlescriptCurrInstr = ptr;
            return;
        }
        case VARIOUS_JUMP_IF_WEATHER_AFFECTED: {
            u32 weatherFlags = READ_32_INC;
            ptr = READ_PTR_INC;
            if (IsBattlerWeatherAffected(gActiveBattler, weatherFlags)) gBattlescriptCurrInstr = ptr;
        }
            return;
        case VARIOUS_APPLY_PLASMA_FISTS:
            for (i = 0; i < gBattlersCount; i++) gStatuses4[i] |= STATUS4_PLASMA_FISTS;
            break;
        case VARIOUS_JUMP_IF_SPECIES: {
            int species = READ_16_INC;
            ptr = READ_PTR_INC;
            if (gBattleMons[gActiveBattler].species == species) gBattlescriptCurrInstr = ptr;
        }
            return;
        case VARIOUS_PHOTON_GEYSER_CHECK: {
            break;
        }
        case VARIOUS_SHELL_SIDE_ARM_CHECK:  // 0% chance GameFreak actually checks this way according to DaWobblefet, but this is the only functional
                                            // explanation at the moment
        {
            break;
        }
        case VARIOUS_JUMP_IF_LEAF_GUARD_PROTECTED:
            ptr = READ_PTR_INC;
            if (IsLeafGuardProtected(gActiveBattler)) {
                gBattlerAbility = gActiveBattler;
                gBattlescriptCurrInstr = ptr;
            }
            return;
        case VARIOUS_SET_ATTACKER_STICKY_WEB_USER:
            // For Mirror Armor: "If the Pokémon with this Ability is affected by Sticky Web, the effect is reflected back to the Pokémon which set it up.
            //  If Pokémon which set up Sticky Web is not on the field, no Pokémon have their Speed lowered."
            gBattlerAttacker = gBattlerTarget;  // Initialize 'fail' condition
            SetActiveStatChanger(STAT_SPEED, -1);
            if (gBattleStruct->stickyWebUser != 0xFF) gBattlerAttacker = gBattleStruct->stickyWebUser;
            break;
        case VARIOUS_CUT_1_3_HP_RAISE_STATS: {
            bool8 atLeastOneStatBoosted = FALSE;
            u16 hpFraction = max(1, gBattleMons[gBattlerAttacker].maxHP / 3);
            ptr = READ_PTR_INC;

            for (i = 1; i < NUM_STATS; i++) {
                if (CompareStat(gBattlerAttacker, i, MAX_STAT_STAGE, CMP_LESS_THAN)) {
                    atLeastOneStatBoosted = TRUE;
                    break;
                }
            }
            if (atLeastOneStatBoosted && gBattleMons[gBattlerAttacker].hp > hpFraction) {
                gBattleMoveDamage = hpFraction;
            } else {
                gBattlescriptCurrInstr = ptr;
            }
        }
            return;
        case VARIOUS_SET_OCTOLOCK:
            ptr = READ_PTR_INC;
            if (gVolatileStructs[gActiveBattler].octolock || !IsBattlerAlive(gActiveBattler)) {
                gBattlescriptCurrInstr = ptr;
            } else {
                gVolatileStructs[gActiveBattler].octolock = TRUE;
                gBattleMons[gActiveBattler].status2 |= STATUS2_ESCAPE_PREVENTION;
                gVolatileStructs[gActiveBattler].battlerPreventingEscape = gBattlerAttacker;
            }
            return;
        case VARIOUS_CHECK_POLTERGEIST:
            ptr = READ_PTR_INC;
            if (gBattleMons[gActiveBattler].item == ITEM_NONE || (gStatuses3[gActiveBattler] & STATUS3_SEMI_INVULNERABLE) ||
                IS_BATTLER_PROTECTED(gActiveBattler) || BattlerHasAbility(gActiveBattler, ABILITY_KLUTZ, FALSE)) {
                gBattlescriptCurrInstr = ptr;
            } else {
                gLastUsedItem = gBattleMons[gActiveBattler].item;
            }
            return;
        case VARIOUS_TRY_NO_RETREAT:
            ptr = READ_PTR_INC;
            if (gVolatileStructs[gActiveBattler].noRetreat || gBattleMons[gActiveBattler].status2 & STATUS2_FOCUS_ENERGY) {
                gBattlescriptCurrInstr = ptr;
            } else {
                gVolatileStructs[gActiveBattler].noRetreat = TRUE;
            }
            return;
        case VARIOUS_TRY_TAR_SHOT:
            ptr = READ_PTR_INC;
            if (gVolatileStructs[gActiveBattler].tarShot)
                gBattlescriptCurrInstr = ptr;
            else
                gVolatileStructs[gActiveBattler].tarShot = TRUE;
            return;
        case VARIOUS_CAN_TAR_SHOT_WORK:
            ptr = READ_PTR_INC;
            // Tar Shot will fail if it's already been used on the target and its speed can't be lowered further
            if (gVolatileStructs[gActiveBattler].tarShot || !CompareStat(gActiveBattler, STAT_SPEED, MAX_STAT_STAGE, CMP_LESS_THAN))
                gBattlescriptCurrInstr = ptr;
            return;
        case VARIOUS_TRY_TO_APPLY_MIMICRY:
            // Unused
            return;
        case VARIOUS_CAN_TELEPORT:
            gBattleCommunication[0] = CanTeleport(gActiveBattler);
            break;
        case VARIOUS_GET_BATTLER_SIDE:
            if (GetBattlerSide(gActiveBattler) == B_SIDE_PLAYER)
                gBattleCommunication[0] = B_SIDE_PLAYER;
            else
                gBattleCommunication[0] = B_SIDE_OPPONENT;
            break;
        case VARIOUS_RAISE_HIGHEST_ATTACKING_STAT:
            increase = READ_8_INC;
            ptr = READ_PTR_INC;

            statId = GetHighestAttackingStatId(gActiveBattler, TRUE);

            SetActiveStatChanger(statId, increase);

            if (!ChangeStatBuffs(gActiveBattler, increase, statId, MOVE_EFFECT_AFFECTS_USER, 0)) {
                gBattlescriptCurrInstr = ptr;
            }
            return;
        case VARIOUS_SET_DYNAMIC_TYPE:
            gBattleStruct->dynamicMoveType = READ_8_INC;
            if (gBattleStruct->dynamicMoveType == TYPE_MYSTERY) SetTypeBeforeUsingMove(gCurrentMove, gActiveBattler);
            return;
        case VARIOUS_GOTO_ACTUAL_MOVE:
            gBattlescriptCurrInstr = gBattleScriptsForMoveEffects[gBattleMoves[gCurrentMove].effect];
            return;
        case VARIOUS_SET_FEAR:
            gStatuses4[gActiveBattler] |= STATUS4_FEAR;
            gVolatileStructs[gActiveBattler].fear = gVolatileStructs[gActiveBattler].started.fear = TRUE;
            SetBattlerAffectedFlag(gBattlerAttacker, gActiveBattler, ABILITY_BLOOD_BATH);
            SetBattlerAffectedFlag(gBattlerAttacker, gActiveBattler, ABILITY_BLOODLUST);
            break;
        case VARIOUS_ON_WEATHER_CHANGE:
            ON_ABILITY(
                gActiveBattler, FALSE, gAbilities[ability].onWeather, if (gAbilities[ability].onWeather(ability, gActiveBattler)) {
                    gBattlerAbility = gActiveBattler;
                    gBattleScripting.abilityPopupOverwrite = ability;
                    BattleScriptCall(BattleScript_AbilityPopUp);
                })
            return;
        case VARIOUS_ON_TERRAIN_CHANGE:
            ON_ABILITY(
                gActiveBattler, FALSE, gAbilities[ability].onTerrain, if (gAbilities[ability].onTerrain(ability, gActiveBattler)) {
                    gBattlerAbility = gActiveBattler;
                    gBattleScripting.abilityPopupOverwrite = ability;
                    BattleScriptCall(BattleScript_AbilityPopUp);
                })
            return;
        case VARIOUS_GET_BATTLER:
            gBattleScripting.battler = gActiveBattler;
            break;
        case VARIOUS_DO_COPY_STAT_CHANGE:
            if (gBattleStruct->statStageCheckState == STAT_STAGE_CHECK_NOT_NEEDED) break;
            ptr = gBattlescriptCurrInstr;
            gBattlescriptCurrInstr = runAgain;
            for (i = 0; i < gBattlersCount; i++) {
                u8 battler = gBattlerAttacker = gBattlerByTurnOrder[i];
                u8 otherBattler;
                StatCopyState state;
                s8 change = 0;

                if (!IsBattlerAlive(battler)) continue;

                state = GetAbilityStateAs(battler, ABILITY_EGOIST).statCopyState;
                if (!state.inProgress) continue;

                for (state.stat++; state.stat < NUM_BATTLE_STATS; state.stat++) {
                    for (otherBattler = 0; otherBattler < gBattlersCount; otherBattler++) {
                        if (GetBattlerSide(otherBattler) == GetBattlerSide(battler)) continue;
                        if (gBattleStruct->statChangesToCheck[otherBattler][state.stat - 1] > 0)
                            change += gBattleStruct->statChangesToCheck[otherBattler][state.stat - 1];
                    }
                    if (change) {
                        if (ChangeStatBuffs(battler, change, state.stat, MOVE_EFFECT_AFFECTS_USER, NULL)) {
                            BattleScriptCall(BattleScript_PerformStatUp);
                            if (!state.announced) {
                                gBattleScripting.abilityPopupOverwrite = ABILITY_EGOIST;
                                gBattlerAbility = battler;
                                BattleScriptCall(BattleScript_AbilityPopUpAndWait);
                                state.announced = TRUE;
                            }
                            break;
                        }
                    }
                }

                if (state.stat >= NUM_BATTLE_STATS) state = (StatCopyState){0};

                SetAbilityStateAs(battler, ABILITY_EGOIST, (AbilityStates){.statCopyState = state});
                return;
            }

            gEffectBattler = IsAbilityOnField(ABILITY_SHARING_IS_CARING) - 1;
            if ((s8)gEffectBattler >= 0) {
                StatCopyState state = GetAbilityStateAs(gEffectBattler, ABILITY_SHARING_IS_CARING).statCopyState;
                if (state.inProgress) {
                    for (; state.battler < gBattlersCount; state.battler++) {
                        if (!IsBattlerAlive(state.battler)) continue;

                        for (state.stat++; state.stat < NUM_BATTLE_STATS; state.stat++) {
                            s8 change = 0;
                            u8 otherBattler;
                            for (otherBattler = 0; otherBattler < gBattlersCount; otherBattler++) {
                                if (otherBattler == state.battler) continue;
                                change += gBattleStruct->statChangesToCheck[otherBattler][state.stat - 1];
                            }
                            if (change) {
                                if (ChangeStatBuffs(state.battler, change, state.stat, MOVE_EFFECT_AFFECTS_USER, NULL)) {
                                    gBattlerAttacker = state.battler;
                                    BattleScriptCall(change > 0 ? BattleScript_PerformStatUp : BattleScript_PerformStatDown);
                                    if (!state.announced) {
                                        gBattlerAbility = gEffectBattler;
                                        gBattleScripting.abilityPopupOverwrite = ABILITY_SHARING_IS_CARING;
                                        BattleScriptCall(BattleScript_AbilityPopUpAndWait);
                                        state.announced = TRUE;
                                    }
                                    SetAbilityStateAs(gEffectBattler, ABILITY_SHARING_IS_CARING, (AbilityStates){.statCopyState = state});
                                    return;
                                }
                            }
                        }

                        if (state.stat >= NUM_BATTLE_STATS) state.stat = 0;
                    }
                    if (state.battler >= gBattlersCount) state = (StatCopyState){0};
                    SetAbilityStateAs(gEffectBattler, ABILITY_SHARING_IS_CARING, (AbilityStates){.statCopyState = state});
                    return;
                }
            }
            for (gEffectBattler = 0; gEffectBattler < gBattlersCount; gEffectBattler++) {
                u8 stat;
                FILTER(IsBattlerAlive(gEffectBattler))
                FILTER(gTurnStructs[gEffectBattler].mirrorHerbStat ||
                       (GetBattlerHoldEffect(gEffectBattler, TRUE) == HOLD_EFFECT_MIRROR_HERB && !IsUnnerveAbilityOnOpposingSide(gEffectBattler)))

                for (stat = max(gTurnStructs[gEffectBattler].mirrorHerbStat, STAT_ATK); stat < NUM_BATTLE_STATS; stat++) {
                    s8 change = 0;
                    for (i = 0; i < gBattlersCount; i++) {
                        if (GetBattlerSide(i) == GetBattlerSide(gEffectBattler)) continue;
                        if (gBattleStruct->statChangesToCheck[i][stat - 1] > 0) change += gBattleStruct->statChangesToCheck[i][stat - 1];
                    }
                    if (change) {
                        if (ChangeStatBuffs(gEffectBattler, change, stat, MOVE_EFFECT_AFFECTS_USER, NULL)) {
                            gBattlerAttacker = gEffectBattler;
                            BattleScriptCall(change > 0 ? BattleScript_PerformStatUp : BattleScript_PerformStatDown);
                            if (!gTurnStructs[gEffectBattler].mirrorHerbStat) {
                                BattleScriptCall(BattleScript_AttackerAteItem);
                            }
                            gTurnStructs[gEffectBattler].mirrorHerbStat = stat + 1;
                            return;
                        }
                    }
                }
                gTurnStructs[gEffectBattler].mirrorHerbStat = 0;
            }
            ZERO(gBattleStruct->statChangesToCheck)
            gBattleStruct->statStageCheckState = STAT_STAGE_CHECK_NOT_NEEDED;
            gBattlescriptCurrInstr = ptr;
            break;
        case VARIOUS_TRY_LOSE_PERCENT_HP: {
            u8 percentHp = READ_8_INC;
            u32 hpLost = gBattleMons[gActiveBattler].hp * percentHp / 100;
            ptr = READ_PTR_INC;

            if (gBattleMons[gActiveBattler].hp <= 1) {
                gBattlescriptCurrInstr = ptr;
                return;
            }

            if (!hpLost) hpLost = 1;

            gBattleMoveDamage = hpLost;
            return;
        }
        case VARIOUS_SWAP_SIDE_EFFECTS: {
            u32 temp;
            struct SideBeganThisTurn tempSide;
            u32 tempFlags = gSideStatuses[0] & SIDE_STATUS_SWAPPABLE;
            gSideStatuses[0] &= ~SIDE_STATUS_SWAPPABLE;
            gSideStatuses[0] |= (gSideStatuses[1] & SIDE_STATUS_SWAPPABLE);
            gSideStatuses[1] &= ~SIDE_STATUS_SWAPPABLE;
            gSideStatuses[1] |= tempFlags;

            SWAP(gSideTimers[0].reflectTimer, gSideTimers[1].reflectTimer, temp)
            SWAP(gSideTimers[0].lightscreenTimer, gSideTimers[1].lightscreenTimer, temp)
            SWAP(gSideTimers[0].mistTimer, gSideTimers[1].mistTimer, temp)
            SWAP(gSideTimers[0].safeguardTimer, gSideTimers[1].safeguardTimer, temp)
            SWAP(gSideTimers[0].spikesAmount, gSideTimers[1].spikesAmount, temp)
            SWAP(gSideTimers[0].toxicSpikesAmount, gSideTimers[1].toxicSpikesAmount, temp)
            SWAP(gSideTimers[0].stealthRockType, gSideTimers[1].stealthRockType, temp)
            SWAP(gSideTimers[0].stickyWebTimer, gSideTimers[1].stickyWebTimer, temp)
            SWAP(gSideTimers[0].auroraVeilTimer, gSideTimers[1].auroraVeilTimer, temp)
            SWAP(gSideTimers[0].tailwindTimer, gSideTimers[1].tailwindTimer, temp)
            SWAP(gSideTimers[0].luckyChantTimer, gSideTimers[1].luckyChantTimer, temp)
            SWAP(gSideTimers[0].swampTimer, gSideTimers[1].swampTimer, temp)
            SWAP(gSideTimers[0].fireSeaTimer, gSideTimers[1].fireSeaTimer, temp)
            SWAP(gSideTimers[0].rainbowTimer, gSideTimers[1].rainbowTimer, temp)
            SWAP(gSideTimers[0].smokescreenTimer, gSideTimers[1].smokescreenTimer, temp)
            SWAP(gSideTimers[0].hotCoals, gSideTimers[1].hotCoals, temp)
            SWAP(gSideTimers[0].caltrops, gSideTimers[1].caltrops, temp)
            SWAP(gSideTimers[0].started, gSideTimers[1].started, tempSide);

#define UPDATE_COURTCHANGED_BATTLER(structField)                                  \
    {                                                                             \
        temp = gSideTimers[0].structField;                                        \
        gSideTimers[0].structField = BATTLE_OPPOSITE(gSideTimers[1].structField); \
        gSideTimers[1].structField = BATTLE_OPPOSITE(temp);                       \
    }

            UPDATE_COURTCHANGED_BATTLER(reflectBattlerId);
            UPDATE_COURTCHANGED_BATTLER(lightscreenBattlerId);
            UPDATE_COURTCHANGED_BATTLER(mistBattlerId);
            UPDATE_COURTCHANGED_BATTLER(safeguardBattlerId);
            UPDATE_COURTCHANGED_BATTLER(auroraVeilBattlerId);
            UPDATE_COURTCHANGED_BATTLER(tailwindBattlerId);
            UPDATE_COURTCHANGED_BATTLER(luckyChantBattlerId);
            UPDATE_COURTCHANGED_BATTLER(smokescreenBattler);

#undef UPDATE_COURTCHANGED_BATTLER

            break;
        }
        case VARIOUS_GHASTLY_ECHO:
            if (!(gStatuses4[gActiveBattler] & STATUS4_GHASTLY_ECHO) && !IsSoundproof(gActiveBattler)) {
                gStatuses4[gActiveBattler] |= STATUS4_GHASTLY_ECHO;
                gVolatileStructs[gActiveBattler].ghastlyEchoTimer = 2;
                BattleScriptCall(BattleScript_AnnounceGhastlyEcho);
                return;
            }
            break;
        case VARIOUS_JUMP_IF_STATUS_4: {
            u32 status = READ_32_INC;
            ptr = READ_PTR_INC;
            if (gStatuses4[gActiveBattler] & status) gBattlescriptCurrInstr = ptr;
            return;
        }
        case VARIOUS_RESTORE_TURN_BATTLERS:
            if (gBattleStruct->switchInAbilitiesCounter < gBattlersCount) {
                gBattlerAttacker = gBattlerByTurnOrder[gBattleStruct->switchInAbilitiesCounter];
            } else if (gBattleStruct->switchInItemsCounter < gBattlersCount) {
                gBattlerAttacker = gBattlerByTurnOrder[gBattleStruct->switchInItemsCounter];
            } else if (gCurrentTurnActionNumber < gBattlersCount) {
                gBattlerAttacker = GetTurnBattler();
            }
            break;
        case VARIOUS_TRY_FLING:
            ptr = READ_PTR_INC;
            if (CanFling(gActiveBattler)) {
                switch (gBattleMons[gActiveBattler].item) {
                    case ITEM_FLAME_ORB:
                        gBattleScripting.moveEffect = MOVE_EFFECT_BURN;
                        break;
                    case ITEM_RAZOR_FANG:
                    case ITEM_KINGS_ROCK:
                        gBattleScripting.moveEffect = MOVE_EFFECT_FLINCH;
                        break;
                    case ITEM_TOXIC_ORB:
                        gBattleScripting.moveEffect = MOVE_EFFECT_TOXIC;
                        break;
                    case ITEM_FROST_ORB:
                        gBattleScripting.moveEffect = MOVE_EFFECT_FROSTBITE;
                        break;
                    case ITEM_LIGHT_BALL:
                        gBattleScripting.moveEffect = MOVE_EFFECT_PARALYSIS;
                        break;
                    case ITEM_BLACK_SLUDGE:
                    case ITEM_POISON_BARB:
                        gBattleScripting.moveEffect = MOVE_EFFECT_POISON;
                        break;
                }
                gTurnStructs[gActiveBattler].flungItem = gBattleMons[gActiveBattler].item;
                RemoveItem(gActiveBattler);
            } else {
                gTurnStructs[gActiveBattler].flungItem = 0;
                gBattlescriptCurrInstr = ptr;
            }
            return;
        case VARIOUS_TRY_REVIVAL_BLESSING: {
            ptr = READ_PTR_INC;

            if (GetFirstFaintedPartyIndex(gActiveBattler) == PARTY_SIZE) {
                gBattlescriptCurrInstr = ptr;
                return;
            }

            // Battler selected! Revive and go to next instruction.
            if (gSelectedMonPartyId < PARTY_SIZE) {
                struct Pokemon* party = GetBattlerParty(gActiveBattler);
                u16 hp = GetMonData(&party[gSelectedMonPartyId], MON_DATA_MAX_HP) / 2;
                BtlController_EmitSetMonData(0, REQUEST_HP_BATTLE, gBitTable[gSelectedMonPartyId], sizeof(hp), &hp);
                MarkBattlerForControllerExec(gBattlerAttacker);
                PREPARE_SPECIES_BUFFER(gBattleTextBuff1, GetMonData(&party[gSelectedMonPartyId], MON_DATA_SPECIES));

                // If an on-field battler is revived, it needs to be sent out again.
                if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE && gBattlerPartyIndexes[BATTLE_PARTNER(gBattlerAttacker)] == gSelectedMonPartyId) {
                    gBattleScripting.battler = BATTLE_PARTNER(gBattlerAttacker);
                    gBattleCommunication[MULTIUSE_STATE] = TRUE;
                }

                gSelectedMonPartyId = PARTY_SIZE;
            } else {
                gBattlescriptCurrInstr = ptr;
            }
        }
            return;
        case VARIOUS_WRITE_STACK_BATTLER:
            SetActiveStackBattler(gActiveBattler, READ_8_INC);
            return;
        case VARIOUS_RESTORE_STACK_STATE:
            ReadActiveScriptInitialStackState();
            break;
        case VARIOUS_DISABLE_RANDOM:
            do {
                gVolatileStructs[gActiveBattler].disabledMove = Random() % 4;
            } while (gBattleMons[gActiveBattler].moves[gVolatileStructs[gActiveBattler].disabledMove] != MOVE_NONE);
            gVolatileStructs[gActiveBattler].disableTimer = gVolatileStructs[gActiveBattler].disableTimerStartValue = 4;
            PREPARE_MOVE_BUFFER(gBattleTextBuff1, gBattleMons[gActiveBattler].moves[gVolatileStructs[gActiveBattler].disabledMove])
            break;
        case VARIOUS_GOTO_IF_STAT_UP:
            ptr = READ_PTR_INC;
            REQUIRE_NOT(IsUnaware(gBattlerAttacker))
            REQUIRE_NOT(gBattleMons[gActiveBattler].status1 & STATUS1_BLEED)
            for (i = STAT_ATK; i < NUM_BATTLE_STATS; i++) {
                FILTER(gBattleMons[gActiveBattler].statStages[i] > DEFAULT_STAT_STAGE)
                gBattlescriptCurrInstr = ptr;
                break;
            }
            return;
        case VARIOUS_TRY_UPPER_HAND:
            ptr = READ_PTR_INC;
            if (gCurrentTurnActionNumber >= GetBattlerTurnOrderNum(gActiveBattler)) {
                gBattlescriptCurrInstr = ptr;
                return;
            }
            {
                s8 chosenMovePriority = GetChosenMovePriority(gActiveBattler, gBattleStruct->moveTarget[gActiveBattler]);
                if (chosenMovePriority > 0) {
                    return;
                }
            }
            gBattlescriptCurrInstr = ptr;
            return;
        case VARIOUS_REQUIRE_CAN_DO_EFFECT: {
            u16 ability;
            u16 effect = READ_16_INC;
            u16 affectsUser = effect & MOVE_EFFECT_AFFECTS_USER;
            effect &= ~MOVE_EFFECT_AFFECTS_USER;
            switch (effect) {
                default:
                    return;

                case MOVE_EFFECT_SLEEP:
                    if (CanSleep(gActiveBattler))
                        return;
                    else if (JumpIfStandardStatusBlocking(gActiveBattler, affectsUser))
                        return;
                    else if (affectsUser && gBattleMons[gActiveBattler].status1 & STATUS1_SLEEP)
                        gBattlescriptCurrInstr = BattleScript_RestIsAlreadyAsleep;
                    else if (UproarWakeUpCheck(gActiveBattler))
                        gBattlescriptCurrInstr = BattleScript_CantMakeAsleep;
                    else if (BATTLER_HAS_ABILITY(gActiveBattler, ABILITY_INSOMNIA)) {
                        SetActiveAbilityPopupOverride(ABILITY_INSOMNIA);
                        gBattlescriptCurrInstr = BattleScript_LeafGuardProtects;
                    } else if (BATTLER_HAS_ABILITY(gActiveBattler, ABILITY_VITAL_SPIRIT)) {
                        SetActiveAbilityPopupOverride(ABILITY_VITAL_SPIRIT);
                        gBattlescriptCurrInstr = BattleScript_LeafGuardProtects;
                    } else if (GetAbilityState(gActiveBattler, ABILITY_RUDE_AWAKENING)) {
                        SetActiveAbilityPopupOverride(ABILITY_RUDE_AWAKENING);
                        gBattlescriptCurrInstr = BattleScript_LeafGuardProtects;
                    } else if (IsBattlerTerrainAffected(gActiveBattler, STATUS_FIELD_ELECTRIC_TERRAIN))
                        gBattlescriptCurrInstr = BattleScript_ElectricTerrainPrevents;
                    else if ((ability = IsAbilityOnSide(gActiveBattler, ABILITY_SWEET_VEIL))) {
                        if (gActiveBattler != ability - 1) gBattleScripting.battlerPopupOverwrite = BATTLE_PARTNER(gActiveBattler);
                        SetActiveAbilityPopupOverride(ABILITY_SWEET_VEIL);
                        gBattlescriptCurrInstr = BattleScript_LeafGuardProtects;
                    }
                    return;
                case MOVE_EFFECT_BLEED:
                    if (CanBleed(gActiveBattler))
                        return;
                    else if (IS_BATTLER_OF_TYPE(gActiveBattler, TYPE_ROCK))
                        gBattlescriptCurrInstr = BattleScript_NotAffected;
                    else if (IS_BATTLER_OF_TYPE(gActiveBattler, TYPE_GHOST))
                        gBattlescriptCurrInstr = BattleScript_NotAffected;
                    else
                        JumpIfStandardStatusBlocking(gActiveBattler, affectsUser);
                    return;
                case MOVE_EFFECT_FROSTBITE:
                    if (CanGetFrostbite(gActiveBattler))
                        return;
                    else if (IS_BATTLER_OF_TYPE(gActiveBattler, TYPE_ICE))
                        gBattlescriptCurrInstr = BattleScript_NotAffected;
                    else if (JumpIfStandardStatusBlocking(gActiveBattler, affectsUser))
                        return;
                    else if (BATTLER_HAS_ABILITY(gActiveBattler, ABILITY_MAGMA_ARMOR)) {
                        SetActiveAbilityPopupOverride(ABILITY_MAGMA_ARMOR);
                        gBattlescriptCurrInstr = BattleScript_LeafGuardProtects;
                    }
                    return;
                case MOVE_EFFECT_TOXIC:
                case MOVE_EFFECT_POISON:
                    if (CanBePoisoned(gBattlerAttacker, gActiveBattler))
                        return;
                    else if (!CanPoisonType(gBattlerAttacker, gActiveBattler))
                        gBattlescriptCurrInstr = BattleScript_NotAffected;
                    else if (JumpIfStandardStatusBlocking(gActiveBattler, affectsUser))
                        return;
                    else if (BATTLER_HAS_ABILITY(gActiveBattler, ABILITY_IMMUNITY)) {
                        SetActiveAbilityPopupOverride(ABILITY_IMMUNITY);
                        gBattlescriptCurrInstr = BattleScript_LeafGuardProtects;
                    }
                    return;
                case (MOVE_EFFECT_PARALYSIS | MOVE_EFFECT_IGNORE_TYPE_IMMUNITIES):
                    if (CanBeParalyzedIgnoreType(gBattlerAttacker, gActiveBattler))
                        return;
                    else if (JumpIfStandardStatusBlocking(gActiveBattler, affectsUser))
                        return;
                    else if (BATTLER_HAS_ABILITY(gActiveBattler, ABILITY_LIMBER)) {
                        SetActiveAbilityPopupOverride(ABILITY_LIMBER);
                        gBattlescriptCurrInstr = BattleScript_LeafGuardProtects;
                    } else if (BATTLER_HAS_ABILITY(gActiveBattler, ABILITY_JUGGERNAUT)) {
                        SetActiveAbilityPopupOverride(ABILITY_JUGGERNAUT);
                        gBattlescriptCurrInstr = BattleScript_LeafGuardProtects;
                    }
                    return;
                case MOVE_EFFECT_PARALYSIS:
                    if (CanBeParalyzedIgnoreType(gBattlerAttacker, gActiveBattler))
                        return;
                    else if (!CanParalyzeType(gBattlerAttacker, gActiveBattler))
                        gBattlescriptCurrInstr = BattleScript_NotAffected;
                    else if (JumpIfStandardStatusBlocking(gActiveBattler, affectsUser))
                        return;
                    else if (BATTLER_HAS_ABILITY(gActiveBattler, ABILITY_LIMBER)) {
                        SetActiveAbilityPopupOverride(ABILITY_LIMBER);
                        gBattlescriptCurrInstr = BattleScript_LeafGuardProtects;
                    } else if (BATTLER_HAS_ABILITY(gActiveBattler, ABILITY_JUGGERNAUT)) {
                        SetActiveAbilityPopupOverride(ABILITY_JUGGERNAUT);
                        gBattlescriptCurrInstr = BattleScript_LeafGuardProtects;
                    }
                    return;
                case MOVE_EFFECT_BURN:
                    if (CanBeBurned(gActiveBattler))
                        return;
                    else if (IS_BATTLER_OF_TYPE(gActiveBattler, TYPE_FIRE))
                        gBattlescriptCurrInstr = BattleScript_NotAffected;
                    else if (JumpIfStandardStatusBlocking(gActiveBattler, affectsUser))
                        return;
                    else if (BATTLER_HAS_ABILITY(gActiveBattler, ABILITY_WATER_VEIL)) {
                        SetActiveAbilityPopupOverride(ABILITY_WATER_VEIL);
                        gBattlescriptCurrInstr = BattleScript_LeafGuardProtects;
                    } else if (BATTLER_HAS_ABILITY(gActiveBattler, ABILITY_WATER_BUBBLE)) {
                        SetActiveAbilityPopupOverride(ABILITY_WATER_BUBBLE);
                        gBattlescriptCurrInstr = BattleScript_LeafGuardProtects;
                    } else if (BATTLER_HAS_ABILITY(gActiveBattler, ABILITY_PURIFYING_WATERS)) {
                        SetActiveAbilityPopupOverride(ABILITY_PURIFYING_WATERS);
                        gBattlescriptCurrInstr = BattleScript_LeafGuardProtects;
                    } else if (BATTLER_HAS_ABILITY(gActiveBattler, ABILITY_THERMAL_EXCHANGE)) {
                        SetActiveAbilityPopupOverride(ABILITY_THERMAL_EXCHANGE);
                        gBattlescriptCurrInstr = BattleScript_LeafGuardProtects;
                    }
                    return;
            }
        }
        case VARIOUS_INCREASE_CRIT: {
            int increase = READ_8_INC;
            ptr = READ_PTR_INC;
            increase = min(3 - gVolatileStructs[gActiveBattler].critBoost, increase);
            gBattleCommunication[MULTISTRING_CHOOSER] = increase;
            if (!increase && ptr)
                gBattlescriptCurrInstr = ptr;
            else
                gVolatileStructs[gActiveBattler].critBoost += increase;
        }
            return;
        case VARIOUS_DO_FOG_STAT_DROPS: {
            int bits = 0;
            ptr = READ_PTR_INC;

            if (!IsBattlerWeatherAffected(gActiveBattler, WEATHER_FOG_ANY) ||
                ((IS_BATTLER_OF_TYPE(gActiveBattler, TYPE_GHOST) || IS_BATTLER_OF_TYPE(gActiveBattler, TYPE_PSYCHIC)) &&
                 !gVolatileStructs[gActiveBattler].trickOrTreat)) {
                gBattlescriptCurrInstr = ptr;
                return;
            }

            for (i = STAT_ATK; i < NUM_STATS; i++) {
                if (gBattleMons[gActiveBattler].statStages[i] > DEFAULT_STAT_STAGE) {
                    bits |= (1 << i);
                    // Do stat drops directly to avoid weird ability/item interactions
                    gBattleMons[gActiveBattler].statStages[i]--;
                }
            }

            if (bits) {
                PlayStatChangeAnimation(gActiveBattler, bits, STAT_CHANGE_NEGATIVE, TRUE);
            } else {
                gBattlescriptCurrInstr = ptr;
            }

            return;
        }
        case VARIOUS_SET_STATUS_4: {
            int status4 = READ_32_INC;
            gStatuses4[gActiveBattler] |= status4;
        }
            return;
        case VARIOUS_SET_WEATHER: {
            int weather = READ_8_INC;
            ptr = READ_PTR_INC;
            if (!TryChangeBattleWeather(gActiveBattler, weather, FALSE)) {
                gBattlescriptCurrInstr = ptr;
                SetActiveMultistringChooser(B_MSG_WEATHER_FAILED);
            } else {
                SetActiveMultistringChooser(GetWeatherChangeMultistringChooser(weather));
            }
        }
            return;
        case VARIOUS_TRY_RECURRING_NIGHTMARE:
            if (GetSingleUseAbilityCounter(gActiveBattler, ABILITY_RECURRING_NIGHTMARE) == 1) {
                gBattleMoveDamage = gBattleMons[gActiveBattler].maxHP / 4;
                if (!gBattleMoveDamage) gBattleMoveDamage = 1;
                SetSingleUseAbilityCounter(gActiveBattler, ABILITY_RECURRING_NIGHTMARE, 2);
                BtlController_EmitSetMonData(0, REQUEST_HP_BATTLE, gBitTable[gBattleStruct->battlerPartyIndexes[gActiveBattler]], 2, &gBattleMoveDamage);
                MarkBattlerForControllerExec(gActiveBattler);
            }
            break;
        case VARIOUS_SET_RANDOM:
            gBattleCommunication[MULTIUSE_STATE] = Random() % READ_8_INC;
            return;
        case VARIOUS_SKY_DROP: {
            int clear = READ_8_INC;
            ptr = READ_PTR_INC;

            if (clear) {
                if (gVolatileStructs[gActiveBattler].skyDropped) {
                    gVolatileStructs[gActiveBattler].skyDropped = FALSE;
                    gVolatileStructs[gActiveBattler].shouldClearSkyDrop = FALSE;
                    gStatuses3[gActiveBattler] &= ~STATUS3_ON_AIR;
                } else {
                    gBattlescriptCurrInstr = ptr;
                }
            } else {
                if (!gVolatileStructs[gActiveBattler].skyDropped && gActiveBattler != gBattlerAttacker) {
                    gVolatileStructs[gActiveBattler].skyDropped = TRUE;
                    gVolatileStructs[gActiveBattler].skyDroppedBy = gBattlerAttacker;
                    gStatuses3[gActiveBattler] |= STATUS3_ON_AIR;
                    CancelMultiTurnMoves(gActiveBattler);
                } else {
                    gBattlescriptCurrInstr = ptr;
                }
            }
        }
            return;
        case VARIOUS_SET_CLEAR_SKIES:
            ptr = READ_PTR_INC;
            if (gFieldTimers.clearSkiesTimer) {
                gBattlescriptCurrInstr = ptr;
            } else {
                gFieldTimers.clearSkiesTimer = 5;
                gFieldTimers.started.clearSkiesTimer = TRUE;
            }
            return;
        case VARIOUS_SHOWTIME: {
            int mode = READ_8_INC;
            ptr = READ_PTR_INC;

            switch (mode) {
                case 0:
                    if (gFieldTimers.trickRoomTimer || gFieldTimers.wonderRoomTimer || gFieldTimers.inverseRoomTimer)
                        gFieldTimers.trickRoomTimer = gFieldTimers.wonderRoomTimer = gFieldTimers.inverseRoomTimer = 0;
                    else
                        gBattlescriptCurrInstr = ptr;
                    break;

                case 1:
                    if (gFieldTimers.magicRoomTimer)
                        gBattlescriptCurrInstr = ptr;
                    else {
                        gFieldTimers.magicRoomTimer = MAGIC_ROOM_DURATION;
                        gFieldTimers.started.magicRoom = TRUE;
                    }
                    break;
            }
            return;
        }
        case VARIOUS_TREPIDATION:
            ptr = READ_PTR_INC;
            if (gVolatileStructs[gActiveBattler].trepidation)
                gBattlescriptCurrInstr = ptr;
            else
                gVolatileStructs[gActiveBattler].trepidation = 3;
            break;
        case VARIOUS_DO_HAZARD_DAMAGE:
            i = READ_8_INC;
            if (!IsBattlerAlive(gActiveBattler)) break;
            gStackBattler1 = gActiveBattler;
            switch (i) {
                case HAZARD_MODE_SPIKES:
                    BattleScriptCall(BattleScript_ResolveRocks);

                    if (gSideStatuses[GetBattlerSide(gActiveBattler)] & SIDE_STATUS_SPIKES && !IsMagicGuardProtected(gActiveBattler) &&
                        IsBattlerAffectedByHazards(gActiveBattler, FALSE) && IsBattlerGrounded(gActiveBattler)) {
                        u8 spikesDmg = (5 - gSideTimers[GetBattlerSide(gActiveBattler)].spikesAmount) * 2;
                        gBattleMoveDamage = gBattleMons[gActiveBattler].maxHP / (spikesDmg);
                        if (gBattleMoveDamage == 0) gBattleMoveDamage = 1;

                        SetDmgHazardsBattlescript(gActiveBattler, B_MSG_PKMNHURTBYSPIKES);
                    }
                    break;
                case HAZARD_MODE_ROCKS:
                    BattleScriptCall(BattleScript_ResolvePoisonSpikes);

                    if (gSideStatuses[GetBattlerSide(gActiveBattler)] & SIDE_STATUS_STEALTH_ROCK &&
                        IsBattlerAffectedByHazards(gActiveBattler, gSideTimers[GetBattlerSide(gActiveBattler)].stealthRockType == TYPE_ROCK) &&
                        !IsMagicGuardProtected(gActiveBattler)) {
                        gSideStatuses[GetBattlerSide(gActiveBattler)] |= SIDE_STATUS_STEALTH_ROCK_DAMAGED;
                        gBattleMoveDamage = GetStealthHazardDamage(gSideTimers[GetBattlerSide(gActiveBattler)].stealthRockType, gActiveBattler);

                        if (gBattleMoveDamage != 0)
                            SetDmgHazardsBattlescript(
                                gActiveBattler,
                                gSideTimers[GetBattlerSide(gActiveBattler)].stealthRockType == TYPE_GRASS ? B_MSG_CREEPINGTHORNSDMG : B_MSG_STEALTHROCKDMG);
                    }
                    break;
                case HAZARD_MODE_POISON_SPIKES:
                    BattleScriptCall(BattleScript_ResolveWebs);

                    if (gSideStatuses[GetBattlerSide(gActiveBattler)] & SIDE_STATUS_TOXIC_SPIKES && IsBattlerGrounded(gActiveBattler)) {
                        if (IS_BATTLER_OF_TYPE(gActiveBattler, TYPE_POISON))  // Absorb the toxic spikes.
                        {
                            gSideStatuses[GetBattlerSide(gActiveBattler)] &= ~(SIDE_STATUS_TOXIC_SPIKES);
                            gSideTimers[GetBattlerSide(gActiveBattler)].toxicSpikesAmount = 0;
                            BattleScriptCall(BattleScript_ToxicSpikesAbsorbed);
                        } else if (IsBattlerAffectedByHazards(gActiveBattler, FALSE) && CanBePoisoned(BATTLE_OPPOSITE(gActiveBattler), gActiveBattler)) {
                            if (gSideTimers[GetBattlerSide(gActiveBattler)].toxicSpikesAmount >= 2)
                                gBattleMons[gActiveBattler].status1 |= STATUS1_TOXIC_POISON;
                            else
                                gBattleMons[gActiveBattler].status1 |= STATUS1_POISON;

                            BtlController_EmitSetMonData(0, REQUEST_STATUS_BATTLE, 0, 4, &gBattleMons[gActiveBattler].status1);
                            MarkBattlerForControllerExec(gActiveBattler);
                            BattleScriptCall(BattleScript_ToxicSpikesPoisoned);
                        }
                    }
                    break;
                case HAZARD_MODE_WEBS:
                    BattleScriptCall(BattleScript_ResolveFireTrap);

                    if (gSideStatuses[GetBattlerSide(gActiveBattler)] & SIDE_STATUS_STICKY_WEB && IsBattlerAffectedByHazards(gActiveBattler, FALSE) &&
                        IsBattlerGrounded(gActiveBattler)) {
                        gSideStatuses[GetBattlerSide(gActiveBattler)] |= SIDE_STATUS_STICKY_WEB_DAMAGED;
                        SetStatChanger(STAT_SPEED, -1);
                        BattleScriptCall(BattleScript_StickyWebOnSwitchIn);
                    }
                    break;
                case HAZARD_MODE_FIRE_TRAP:
                    BattleScriptCall(BattleScript_ResolveCaltrops);

                    if (gSideTimers[GetBattlerSide(gActiveBattler)].hotCoals && IsBattlerAffectedByHazards(gActiveBattler, FALSE) &&
                        IsBattlerGrounded(gActiveBattler)) {
                        gSideTimers[GetBattlerSide(gActiveBattler)].hotCoals = FALSE;
                        if (CanBeBurned(gActiveBattler)) {
                            gBattleMons[gActiveBattler].status1 |= STATUS1_BURN;
                            BtlController_EmitSetMonData(0, REQUEST_STATUS_BATTLE, 0, 4, &gBattleMons[gActiveBattler].status1);
                            MarkBattlerForControllerExec(gActiveBattler);
                            BattleScriptCall(BattleScript_HotCoalsActivates);
                        }
                        BattleScriptCall(BattleScript_HotCoalsFade);
                    }
                    break;
                case HAZARD_MODE_CALTROPS:
                    if (gSideTimers[GetBattlerSide(gActiveBattler)].caltrops && IsBattlerAffectedByHazards(gActiveBattler, FALSE) &&
                        IsBattlerGrounded(gActiveBattler)) {
                        gSideTimers[GetBattlerSide(gActiveBattler)].caltrops = FALSE;
                        if (CanBleed(gActiveBattler)) {
                            gBattleMons[gActiveBattler].status1 |= STATUS1_BLEED;
                            BtlController_EmitSetMonData(0, REQUEST_STATUS_BATTLE, 0, 4, &gBattleMons[gActiveBattler].status1);
                            MarkBattlerForControllerExec(gActiveBattler);
                            BattleScriptCall(BattleScript_CaltropsBleed);
                        }
                        BattleScriptCall(BattleScript_CaltropsFade);
                    }
                    break;
            }
            break;
        case VARIOUS_TRY_DESTROY_ITEM:
            ptr = READ_PTR_INC;
            if (!gBattleMons[gActiveBattler].item || !CanBattlerGetOrLoseItem(gActiveBattler, gBattleMons[gActiveBattler].item) ||
                IsStickyHold(gActiveBattler) || DoesSubstituteBlockMove(gBattlerAttacker, gActiveBattler, gCurrentMove))
                gBattlescriptCurrInstr = ptr;
            else {
                RemoveItem(gActiveBattler);
            }
            break;
        case VARIOUS_SET_CALTROPS:
            ptr = READ_PTR_INC;
            if (gSideTimers[GetBattlerSide(gActiveBattler)].caltrops)
                gBattlescriptCurrInstr = ptr;
            else
                gSideTimers[GetBattlerSide(gActiveBattler)].caltrops = TRUE;
            break;
        case VARIOUS_SWAP_WITH:
            for (i = 0; i < gBattlersCount; i++) {
                if (i == gActiveBattler) continue;
                if (i == gBattlerAttacker) continue;

                u8* target = &gBattleStruct->moveTarget[i];

                if (*target == gBattlerAttacker)
                    *target = gActiveBattler;
                else if (*target == gActiveBattler)
                    *target = gBattlerAttacker;
            }
            {
                int temp;
                if ((gSideStatuses[GET_BATTLER_SIDE(gActiveBattler)] ^ gSideStatuses[GET_BATTLER_SIDE(gBattlerAttacker)]) & SIDE_STATUS_FUTUREATTACK) {
                    gSideStatuses[GET_BATTLER_SIDE(gActiveBattler)] ^= SIDE_STATUS_FUTUREATTACK;
                    gSideStatuses[GET_BATTLER_SIDE(gActiveBattler)] ^= SIDE_STATUS_FUTUREATTACK;
                }
                SWAP(gWishFutureKnock.futureSightMove[gActiveBattler], gWishFutureKnock.futureSightMove[gBattlerAttacker], temp)
                SWAP(gWishFutureKnock.futureSightPower[gActiveBattler], gWishFutureKnock.futureSightPower[gBattlerAttacker], temp)
                SWAP(gWishFutureKnock.futureSightAttacker[gActiveBattler], gWishFutureKnock.futureSightAttacker[gBattlerAttacker], temp)
                SWAP(gWishFutureKnock.futureSightCounter[gActiveBattler], gWishFutureKnock.futureSightCounter[gBattlerAttacker], temp)
            }
            break;
        case VARIOUS_SWAP_STAT: {
            int stat = READ_8_INC - 1;
            int temp;
            REQUIRE(IsBattlerAlive(gActiveBattler))
            REQUIRE(IsBattlerAlive(gBattlerAttacker))
            SWAP(gBattleMons[gActiveBattler].stats[stat], gBattleMons[gBattlerAttacker].stats[stat], temp)
        } break;
        case VARIOUS_SET_QUICK_GUARD:
            ptr = READ_PTR_INC;
            if (gSideTimers[GetBattlerSide(gActiveBattler)].quickGuardTimer)
                gBattlescriptCurrInstr = ptr;
            else {
                gSideTimers[GetBattlerSide(gActiveBattler)].quickGuardTimer = 3;
                gSideTimers[GetBattlerSide(gActiveBattler)].started.quickGuard = TRUE;
            }
            break;
        case VARIOUS_RUDE_AWAKENING:
            ptr = READ_PTR_INC;
            REQUIRE(gBattleMons[gActiveBattler].status1 & STATUS1_SLEEP)
            REQUIRE(BattlerHasAbility(gActiveBattler, ABILITY_RUDE_AWAKENING, FALSE))
            SetAbilityState(gActiveBattler, ABILITY_RUDE_AWAKENING, TRUE);
            gBattleScripting.abilityPopupOverwrite = ABILITY_RUDE_AWAKENING;
            gBattlerAbility = gActiveBattler;
            BattleScriptPush(ptr);
            gBattlescriptCurrInstr = BattleScript_DoRudeAwakening;
            break;
        case VARIOUS_SHELL_TRAP_CHOICE:
            ptr = READ_PTR_INC;
            REQUIRE_NOT(gTurnStructs[gActiveBattler].multiHitsUsed)
            if (gRoundStructs[gBattlerAttacker].physicalDmg &&
                GetBattlerSide(gBattlerAttacker) != GetBattlerSide(gRoundStructs[gBattlerAttacker].physicalBattlerId) &&
                IsBattlerAlive(gRoundStructs[gBattlerAttacker].physicalBattlerId)) {
                gBattlerTarget = gRoundStructs[gBattlerAttacker].physicalBattlerId;
            } else {
                gBattlescriptCurrInstr = ptr;
            }
            break;
        case VARIOUS_JUMP_IF_ABILITY_STATE: {
            int type = READ_8_INC;
            int ability = READ_16_INC;
            int state = READ_32_INC;
            ptr = READ_PTR_INC;
            REQUIRE(BattlerHasAbility(gActiveBattler, ability, TRUE))
            switch (type) {
                case CMP_COMMON_BITS:
                    REQUIRE(state & GetAbilityState(gActiveBattler, ability))
                    gBattlescriptCurrInstr = ptr;
                    break;

                case CMP_NO_COMMON_BITS:
                    REQUIRE_NOT(state & GetAbilityState(gActiveBattler, ability))
                    gBattlescriptCurrInstr = ptr;
                    break;

                case CMP_EQUAL:
                    REQUIRE(state == GetAbilityState(gActiveBattler, ability))
                    gBattlescriptCurrInstr = ptr;
                    break;

                case CMP_NOT_EQUAL:
                    REQUIRE(state != GetAbilityState(gActiveBattler, ability))
                    gBattlescriptCurrInstr = ptr;
                    break;

                case CMP_GREATER_THAN:
                    REQUIRE(state > GetAbilityState(gActiveBattler, ability))
                    gBattlescriptCurrInstr = ptr;
                    break;

                case CMP_LESS_THAN:
                    REQUIRE(state < GetAbilityState(gActiveBattler, ability))
                    gBattlescriptCurrInstr = ptr;
                    break;
            }
            break;
        }
        case VARIOUS_SET_ABILITY_STATE: {
            int ability = READ_16_INC;
            int state = READ_32_INC;
            ptr = READ_PTR_INC;
            if (!BattlerHasAbility(gActiveBattler, ability, TRUE) || GetAbilityState(gActiveBattler, ability) == state) {
                gBattlescriptCurrInstr = ptr;
            } else {
                SetAbilityState(gActiveBattler, ability, state);
            }
            break;
        }
        case VARIOUS_DO_INTIMIDATE: {
            u16 ability = gBattleScripting.abilityPopupOverwrite;
            u8 numAbility = 0;
            int immunityAbility = IsBattlerImmuneToLowerStatsFromIntimidateClone(gActiveBattler);

            gBattlerTarget = gActiveBattler;

            REQUIRE(IsBattlerAlive(gActiveBattler))

            if (immunityAbility) {
                gBattlerAbility = gBattlerTarget;
                gBattleScripting.abilityPopupOverwrite = immunityAbility;
                BattleScriptCall(BattleScript_IntimidatePrevented);
                break;
            }

            // Monotype Stuff
            if (getMonotypeChampType() == TYPE_DRAGON && GetBattlerSide(gBattlerAttacker) != B_SIDE_PLAYER && ability == ABILITY_NONE)
                ability = ABILITY_FEARMONGER;

            for (i = 0; i < NUM_INTIMIDATE_CLONES; i++) {
                if (gIntimidateCloneData[i].ability == ability) break;
            }

            REQUIRE(i < NUM_INTIMIDATE_CLONES)

            numAbility = i;

            for (i = gIntimidateCloneData[numAbility].numStatsLowered - 1; i >= 0; i--) {
                int moveEffect;
                int statToLower = TranslateStatId(gIntimidateCloneData[numAbility].statsLowered[i], gBattlerTarget);
                if (gIntimidateCloneData[numAbility].statChange == 2) {
                    if (BattlerHasAbility(gBattlerTarget, ABILITY_GUARD_DOG, TRUE))
                        moveEffect = MOVE_EFFECT_ATK_PLUS_2;
                    else
                        moveEffect = MOVE_EFFECT_ATK_MINUS_2;
                } else {
                    if (BattlerHasAbility(gBattlerTarget, ABILITY_GUARD_DOG, TRUE))
                        moveEffect = MOVE_EFFECT_ATK_PLUS_1;
                    else
                        moveEffect = MOVE_EFFECT_ATK_MINUS_1;
                }
                moveEffect += statToLower - 1;
                gBattleScripting.moveEffect = moveEffect;
                SetMoveEffect(TRUE, FALSE);

                if (BattlerHasAbility(gBattlerTarget, ABILITY_GUARD_DOG, TRUE)) {
                    gBattleScripting.abilityPopupOverwrite = ABILITY_GUARD_DOG;
                    gBattlerAbility = gBattlerTarget;
                    BattleScriptCall(BattleScript_AbilityPopUp);
                }
            }
        } break;
        case VARIOUS_HP_FRACTION_TO_DAMAGE:
            int fraction = READ_8_INC;
            gBattleMoveDamage = gBattleMons[gActiveBattler].maxHP / fraction;
            if (gBattleMoveDamage > gBattleMons[gActiveBattler].hp) gBattleMoveDamage = gBattleMons[gActiveBattler].hp;
            if (!gBattleMoveDamage) gBattleMoveDamage = 1;
            break;
        case VARIOUS_JUMP_IF_CONSUMABLE_BLOCKED:
            ptr = READ_PTR_INC;
            int ability = IsUnnerveAbilityOnOpposingSide(gActiveBattler);
            REQUIRE(ability)
            SetActiveAbilityPopupOverride(ability);
            gBattleScripting.battlerPopupOverwrite = IsAbilityOnOpposingSide(gActiveBattler, ability) - 1;
            gBattlescriptCurrInstr = ptr;
            break;
    }  // End of switch (gBattlescriptCurrInstr[2])
}

u8 GetFirstFaintedPartyIndex(u8 battler) {
    u32 i;
    u32 start = 0;
    u32 end = PARTY_SIZE;
    struct Pokemon* party = GetBattlerParty(battler);

    // Check whether partner is separate trainer.
    if ((GetBattlerSide(battler) == B_SIDE_PLAYER && gBattleTypeFlags & BATTLE_TYPE_INGAME_PARTNER) ||
        (GetBattlerSide(battler) == B_SIDE_OPPONENT && gBattleTypeFlags & BATTLE_TYPE_TWO_OPPONENTS)) {
        if (GetBattlerPosition(battler) == B_POSITION_OPPONENT_LEFT || GetBattlerPosition(battler) == B_POSITION_PLAYER_LEFT) {
            end = PARTY_SIZE / 2;
        } else {
            start = PARTY_SIZE / 2;
        }
    }

    // Loop through to find fainted battler.
    for (i = start; i < end; ++i) {
        u32 species = GetMonData(&party[i], MON_DATA_SPECIES2);
        if (species != SPECIES_NONE && species != SPECIES_EGG && GetMonData(&party[i], MON_DATA_HP) == 0) {
            return i;
        }
    }

    // Returns PARTY_SIZE if none found.
    return PARTY_SIZE;
}

static void Cmd_setprotectlike(void) {
    bool32 fail = TRUE;
    bool32 notLastTurn = TRUE;

    if (gCurrentTurnActionNumber == (gBattlersCount - 1)) notLastTurn = FALSE;

    if (ProtectSucceeds(gBattlerAttacker) && notLastTurn) {
        if (!gBattleMoves[gCurrentMove].argument)  // Protects one mon only.
        {
            if (gBattleMoves[gCurrentMove].effect == EFFECT_ENDURE) {
                gRoundStructs[gBattlerAttacker].endured = TRUE;
                SetActiveMultistringChooser(B_MSG_BRACED_ITSELF);
            } else if (gCurrentMove == MOVE_PROTECT) {
                gRoundStructs[gBattlerAttacker].isProtected = TRUE;
                SetActiveMultistringChooser(B_MSG_PROTECTED_ITSELF);
            } else if (gCurrentMove == MOVE_DETECT) {
                gRoundStructs[gBattlerAttacker].detected = TRUE;
                SetActiveMultistringChooser(B_MSG_PROTECTED_ITSELF);
            } else if (gCurrentMove == MOVE_SPIKY_SHIELD) {
                gRoundStructs[gBattlerAttacker].spikyShielded = TRUE;
                SetActiveMultistringChooser(B_MSG_PROTECTED_ITSELF);
            } else if (gCurrentMove == MOVE_KINGS_SHIELD) {
                gRoundStructs[gBattlerAttacker].kingsShielded = TRUE;
                SetActiveMultistringChooser(B_MSG_PROTECTED_ITSELF);
            } else if (gCurrentMove == MOVE_BANEFUL_BUNKER) {
                gRoundStructs[gBattlerAttacker].banefulBunkered = TRUE;
                SetActiveMultistringChooser(B_MSG_PROTECTED_ITSELF);
            } else if (gCurrentMove == MOVE_OBSTRUCT) {
                gRoundStructs[gBattlerAttacker].obstructed = TRUE;
                SetActiveMultistringChooser(B_MSG_PROTECTED_ITSELF);
            } else if (gCurrentMove == MOVE_SILK_TRAP) {
                gRoundStructs[gBattlerAttacker].silkTrapped = TRUE;
                SetActiveMultistringChooser(B_MSG_PROTECTED_ITSELF);
            } else if (gCurrentMove == MOVE_BURNING_BULWARK) {
                gRoundStructs[gBattlerAttacker].burningBulwark = TRUE;
                SetActiveMultistringChooser(B_MSG_PROTECTED_ITSELF);
            } else if (gCurrentMove == MOVE_MERCULIGHT) {
                gRoundStructs[gBattlerAttacker].merculight = TRUE;
                SetActiveMultistringChooser(B_MSG_PROTECTED_ITSELF);
            } else if (gCurrentMove == MOVE_MIND_READER) {
                gRoundStructs[gBattlerAttacker].mindReader = TRUE;
                SetActiveMultistringChooser(B_MSG_PROTECTED_ITSELF);
            } else if (gCurrentMove == MOVE_TANGLING_HUSK) {
                gRoundStructs[gBattlerAttacker].tanglingHusked = TRUE;
                SetActiveMultistringChooser(B_MSG_PROTECTED_ITSELF);
            }

            gVolatileStructs[gBattlerAttacker].protectUses++;
            fail = FALSE;
        } else  // Protects the whole side.
        {
            u8 side = GetBattlerSide(gBattlerAttacker);
            if (gCurrentMove == MOVE_WIDE_GUARD && !(gSideStatuses[side] & SIDE_STATUS_WIDE_GUARD)) {
                gSideStatuses[side] |= SIDE_STATUS_WIDE_GUARD;
                SetActiveMultistringChooser(B_MSG_PROTECTED_TEAM);
                gVolatileStructs[gBattlerAttacker].protectUses++;
                fail = FALSE;
            } else if (gCurrentMove == MOVE_CRAFTY_SHIELD && !(gSideStatuses[side] & SIDE_STATUS_CRAFTY_SHIELD)) {
                gSideStatuses[side] |= SIDE_STATUS_CRAFTY_SHIELD;
                SetActiveMultistringChooser(B_MSG_PROTECTED_TEAM);
                gVolatileStructs[gBattlerAttacker].protectUses++;
                fail = FALSE;
            } else if (gCurrentMove == MOVE_MAT_BLOCK && !(gSideStatuses[side] & SIDE_STATUS_MAT_BLOCK)) {
                gSideStatuses[side] |= SIDE_STATUS_MAT_BLOCK;
                SetActiveMultistringChooser(B_MSG_PROTECTED_TEAM);
                fail = FALSE;
            }
        }
    }

    if (fail) {
        gVolatileStructs[gBattlerAttacker].protectUses = 0;
        SetActiveMultistringChooser(B_MSG_PROTECT_FAILED);
        gMoveResultFlags |= MOVE_RESULT_MISSED;
    }

    gBattlescriptCurrInstr++;
}

static void Cmd_faintifabilitynotdamp(void) {
    if (gBattleControllerExecFlags) return;

    if ((gBattlerTarget = IsAbilityOnField(ABILITY_DAMP))) {
        gBattleScripting.abilityPopupOverwrite = ABILITY_DAMP;
        gBattlescriptCurrInstr = BattleScript_DampStopsExplosion;
        return;
    }

    gActiveBattler = gBattlerAttacker;
    gBattleMoveDamage = gBattleMons[gActiveBattler].hp;
    BtlController_EmitHealthBarUpdate(0, INSTANT_HP_BAR_DROP);
    MarkBattlerForControllerExec(gActiveBattler);
    gBattlescriptCurrInstr++;

    for (gBattlerTarget = 0; gBattlerTarget < gBattlersCount; gBattlerTarget++) {
        if (gBattlerTarget == gBattlerAttacker) continue;
        if (IsBattlerAlive(gBattlerTarget)) break;
    }
}

static void Cmd_setatkhptozero(void) {
    if (gBattleControllerExecFlags) return;

    gActiveBattler = gBattlerAttacker;
    gBattleMons[gActiveBattler].hp = 0;
    BtlController_EmitSetMonData(0, REQUEST_HP_BATTLE, 0, 2, &gBattleMons[gActiveBattler].hp);
    MarkBattlerForControllerExec(gActiveBattler);

    gBattlescriptCurrInstr++;
}

static void Cmd_jumpifnexttargetvalid(void) {
    const u8* jumpPtr = T1_READ_PTR(gBattlescriptCurrInstr + 1);

    for (gBattlerTarget++; gBattlerTarget < gBattlersCount; gBattlerTarget++) {
        if (gBattlerTarget == gBattlerAttacker && !(GetBattlerBattleMoveTargetFlags(gCurrentMove, gBattlerAttacker) & MOVE_TARGET_USER)) continue;
        if (GetAbilityState(gBattlerTarget, ABILITY_COMMANDER) >= COMMANDER_ACTIVE) continue;
        if (IsBattlerAlive(gBattlerTarget)) break;
    }

    if (gBattlerTarget >= gBattlersCount)
        gBattlescriptCurrInstr += 5;
    else
        gBattlescriptCurrInstr = jumpPtr;
}

static void Cmd_tryhealhalfhealth(void) {
    const u8* failPtr = T1_READ_PTR(gBattlescriptCurrInstr + 1);

    if (gBattlescriptCurrInstr[5] == BS_ATTACKER) gBattlerTarget = gBattlerAttacker;

    if (!CanBattlerHeal(gBattlerTarget)) {
        gBattleMoveDamage = 0;
        gBattlescriptCurrInstr += 6;
        return;
    }

    gBattleMoveDamage = gBattleMons[gBattlerTarget].maxHP / 2;
    if (gBattleMoveDamage == 0) gBattleMoveDamage = 1;
    gBattleMoveDamage *= -1;

    if (gBattleMons[gBattlerTarget].hp == gBattleMons[gBattlerTarget].maxHP)
        gBattlescriptCurrInstr = failPtr;
    else
        gBattlescriptCurrInstr += 6;
}

static void Cmd_trymirrormove(void) {
    s32 validMovesCount;
    s32 i;
    u16 move;
    u16 movesArray[4] = {0};

    for (validMovesCount = 0, i = 0; i < gBattlersCount; i++) {
        if (i != gBattlerAttacker) {
            move = gBattleStruct->lastTakenMoveFrom[gBattlerAttacker][i];
            if (move != 0 && move != 0xFFFF) {
                movesArray[validMovesCount] = move;
                validMovesCount++;
            }
        }
    }

    move = gBattleStruct->lastTakenMove[gBattlerAttacker];
    if (move != 0 && move != 0xFFFF) {
        gHitMarker &= ~(HITMARKER_ATTACKSTRING_PRINTED);
        gCurrentMove = move;
        gBattlerTarget = GetMoveTarget(gCurrentMove, 0);
        gBattlescriptCurrInstr = gBattleScriptsForMoveEffects[gBattleMoves[gCurrentMove].effect];
    } else if (validMovesCount) {
        gHitMarker &= ~(HITMARKER_ATTACKSTRING_PRINTED);
        i = Random() % validMovesCount;
        gCurrentMove = movesArray[i];
        gBattlerTarget = GetMoveTarget(gCurrentMove, 0);
        gBattlescriptCurrInstr = gBattleScriptsForMoveEffects[gBattleMoves[gCurrentMove].effect];
    } else {
        gBattlescriptCurrInstr++;
    }
}

static void Cmd_setrain(void) {
    if (!TryChangeBattleWeather(gBattlerAttacker, ENUM_WEATHER_RAIN, FALSE)) {
        gMoveResultFlags |= MOVE_RESULT_MISSED;
        SetActiveMultistringChooser(B_MSG_WEATHER_FAILED);
    } else {
        SetActiveMultistringChooser(B_MSG_STARTED_RAIN);
    }
    gBattlescriptCurrInstr++;
}

static void Cmd_setreflect(void) {
    if (gSideStatuses[GET_BATTLER_SIDE(gBattlerAttacker)] & SIDE_STATUS_REFLECT) {
        gMoveResultFlags |= MOVE_RESULT_MISSED;
        SetActiveMultistringChooser(B_MSG_SIDE_STATUS_FAILED);
    } else {
        int side = GET_BATTLER_SIDE(gBattlerAttacker);
        gSideStatuses[side] |= SIDE_STATUS_REFLECT;
        gSideTimers[side].started.reflect = TRUE;
        if (GetBattlerHoldEffect(gBattlerAttacker, TRUE) == HOLD_EFFECT_LIGHT_CLAY)
            gSideTimers[side].reflectTimer = SCREEN_DURATION_EXTENDED;
        else
            gSideTimers[side].reflectTimer = SCREEN_DURATION;
        gSideTimers[side].reflectBattlerId = gBattlerAttacker;

        if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE && CountAliveMonsInBattle(BATTLE_ALIVE_ATK_SIDE) == 2)
            SetActiveMultistringChooser(B_MSG_SET_REFLECT_DOUBLE);
        else
            SetActiveMultistringChooser(B_MSG_SET_REFLECT_SINGLE);
    }
    gBattlescriptCurrInstr++;
}

static void Cmd_setseeded(void) {
    if (gMoveResultFlags & MOVE_RESULT_NO_EFFECT || gStatuses3[gBattlerTarget] & STATUS3_LEECHSEED) {
        gMoveResultFlags |= MOVE_RESULT_MISSED;
        SetActiveMultistringChooser(B_MSG_LEECH_SEED_MISS);
    } else if (IS_BATTLER_OF_TYPE(gBattlerTarget, TYPE_GRASS)) {
        gMoveResultFlags |= MOVE_RESULT_MISSED;
        SetActiveMultistringChooser(B_MSG_LEECH_SEED_FAIL);
    } else {
        gStatuses3[gBattlerTarget] |= gBattlerAttacker;
        gStatuses3[gBattlerTarget] |= STATUS3_LEECHSEED;
        SetActiveMultistringChooser(B_MSG_LEECH_SEED_SET);
    }

    gBattlescriptCurrInstr++;
}

static void Cmd_manipulatedamage(void) {
    switch (gBattlescriptCurrInstr[1]) {
        case DMG_CHANGE_SIGN:
            gBattleMoveDamage *= -1;
            break;
        case DMG_RECOIL_FROM_MISS:
            gBattleMoveDamage = gBattleMons[gBattlerAttacker].maxHP / 2;

            if (BattlerHasAbility(gBattlerAttacker, ABILITY_LIMBER, FALSE)) gBattleMoveDamage = gBattleMoveDamage * 0.5;
            break;
        case DMG_DOUBLED:
            gBattleMoveDamage *= 2;
            break;
        case DMG_1_8_TARGET_HP:
            gBattleMoveDamage = gBattleMons[gBattlerTarget].maxHP / 8;
            if (gBattleMoveDamage == 0) gBattleMoveDamage = 1;
            break;
        case DMG_1_4_TARGET_HP:
            gBattleMoveDamage = gBattleMons[gBattlerTarget].maxHP / 4;
            if (gBattleMoveDamage == 0) gBattleMoveDamage = 1;
            break;
        case DMG_FULL_ATTACKER_HP:
            gBattleMoveDamage = gBattleMons[gBattlerAttacker].maxHP;
            break;
        case DMG_CURR_ATTACKER_HP:
            gBattleMoveDamage = gBattleMons[gBattlerAttacker].hp;
            break;
        case DMG_BIG_ROOT:
            gBattleMoveDamage = GetDrainedBigRootHp(gBattlerAttacker, -gBattleMoveDamage);
            break;
        case DMG_1_2_ATTACKER_HP:
            gBattleMoveDamage = gBattleMons[gBattlerAttacker].maxHP / 2;
            break;
        case DMG_RECOIL_FROM_IMMUNE:
            gBattleMoveDamage = gBattleMons[gBattlerTarget].maxHP / 2;

            if (BattlerHasAbility(gBattlerAttacker, ABILITY_LIMBER, FALSE)) gBattleMoveDamage = gBattleMoveDamage * 0.5;
            break;
        case DMG_TO_HP_FROM_ABILITY:
            gBattleMoveDamage = GetDrainedBigRootHp(gBattlerAttacker, gBattleMoveDamage);
            break;
        case DMG_TO_HP_FROM_MOVE:
            if (!CanBattlerHeal(gBattlerAttacker)) {
                gBattleMoveDamage = 0;
                break;
            }
            if (gBattleMoves[gCurrentMove].argument != 0)
                gBattleMoveDamage = (gHpDealt * gBattleMoves[gCurrentMove].argument / 100);
            else
                gBattleMoveDamage = (gHpDealt / 2);

            gBattleMoveDamage = GetDrainedBigRootHp(gBattlerAttacker, -gBattleMoveDamage);
            break;
    }

    gBattlescriptCurrInstr += 2;
}

static void Cmd_trysetrest(void) {
    const u8* failJump = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    gActiveBattler = gBattlerTarget = gBattlerAttacker;
    gBattleMoveDamage = gBattleMons[gBattlerTarget].maxHP * (-1);

    if (gBattleMons[gBattlerTarget].hp == gBattleMons[gBattlerTarget].maxHP) {
        gBattlescriptCurrInstr = failJump;
    } else if (IsBattlerTerrainAffected(gBattlerTarget, STATUS_FIELD_ELECTRIC_TERRAIN)) {
        gBattlescriptCurrInstr = BattleScript_ElectricTerrainPrevents;
    } else if (IsBattlerTerrainAffected(gBattlerTarget, STATUS_FIELD_MISTY_TERRAIN)) {
        gBattlescriptCurrInstr = BattleScript_MistyTerrainPrevents;
    } else {
        if (gBattleMons[gBattlerTarget].status1 & ((u8)(~STATUS1_SLEEP)))
            SetActiveMultistringChooser(B_MSG_REST_STATUSED);
        else
            SetActiveMultistringChooser(B_MSG_REST);

        gBattleMons[gBattlerTarget].status1 = STATUS1_SLEEP_TURN(3);
        BtlController_EmitSetMonData(0, REQUEST_STATUS_BATTLE, 0, 4, &gBattleMons[gActiveBattler].status1);
        MarkBattlerForControllerExec(gActiveBattler);
        gBattlescriptCurrInstr += 5;
    }
}

static void Cmd_jumpifnotfirstturn(void) {
    const u8* failJump = T1_READ_PTR(gBattlescriptCurrInstr + 1);

    if (gVolatileStructs[gBattlerAttacker].isFirstTurn || gProcessingExtraAttacks)
        gBattlescriptCurrInstr += 5;
    else
        gBattlescriptCurrInstr = failJump;
}

static void Cmd_setmiracleeye(void) {
    if (!(gStatuses3[gBattlerTarget] & STATUS3_MIRACLE_EYED)) {
        gStatuses3[gBattlerTarget] |= STATUS3_MIRACLE_EYED;
        gBattlescriptCurrInstr += 5;
    } else {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    }
}

bool8 UproarWakeUpCheck(u8 battlerId) {
    s32 i;

    if (IsSoundproof(battlerId)) return FALSE;

    for (i = 0; i < gBattlersCount; i++) {
        if (!(gBattleMons[i].status2 & STATUS2_UPROAR)) continue;

        gBattleScripting.battler = i;

        if (gBattlerTarget == 0xFF)
            gBattlerTarget = i;
        else if (gBattlerTarget == i)
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_CANT_SLEEP_UPROAR;
        else
            gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_UPROAR_KEPT_AWAKE;

        return TRUE;
    }

    return FALSE;
}

static void Cmd_jumpifcantmakeasleep(void) {
    const u8* jumpPtr = READ_PTR_INC;
    int ability;

    if (UproarWakeUpCheck(gBattlerTarget)) {
        gBattlescriptCurrInstr = jumpPtr;
    } else if ((ability = BattlerHasAbility(gBattlerTarget, ABILITY_INSOMNIA, TRUE)) ||
               (ability = BattlerHasAbility(gBattlerTarget, ABILITY_VITAL_SPIRIT, TRUE))) {
        gBattleScripting.abilityPopupOverwrite = ability;
        SetActiveMultistringChooser(B_MSG_STAYED_AWAKE_USING);
        gBattlescriptCurrInstr = jumpPtr;
    }
}

static void Cmd_stockpile(void) {
    switch (gBattlescriptCurrInstr[1]) {
        case 0:
            if (gVolatileStructs[gBattlerAttacker].stockpileCounter >= 3) {
                gMoveResultFlags |= MOVE_RESULT_MISSED;
                SetActiveMultistringChooser(B_MSG_CANT_STOCKPILE);
            } else {
                gVolatileStructs[gBattlerAttacker].stockpileCounter++;
                gVolatileStructs[gBattlerAttacker].stockpileBeforeDef = gBattleMons[gBattlerAttacker].statStages[STAT_DEF];
                gVolatileStructs[gBattlerAttacker].stockpileBeforeSpDef = gBattleMons[gBattlerAttacker].statStages[STAT_SPDEF];
                PREPARE_BYTE_NUMBER_BUFFER(gBattleTextBuff1, 1, gVolatileStructs[gBattlerAttacker].stockpileCounter);
                SetActiveMultistringChooser(B_MSG_STOCKPILED);
            }
            break;
        case 1:  // Save def/sp def stats.
            if (!(gMoveResultFlags & MOVE_RESULT_NO_EFFECT)) {
                gVolatileStructs[gBattlerAttacker].stockpileDef +=
                    gBattleMons[gBattlerAttacker].statStages[STAT_DEF] - gVolatileStructs[gBattlerAttacker].stockpileBeforeDef;
                gVolatileStructs[gBattlerAttacker].stockpileSpDef +=
                    gBattleMons[gBattlerAttacker].statStages[STAT_SPDEF] - gVolatileStructs[gBattlerAttacker].stockpileBeforeSpDef;
            }
            break;
    }

    gBattlescriptCurrInstr += 2;
}

static int TryUseStockpile(int battler) {
    if (!gVolatileStructs[battler].stockpileCounter) return FALSE;

    gVolatileStructs[battler].stockpileCounter--;
    if (gVolatileStructs[battler].stockpileDef) {
        gVolatileStructs[battler].stockpileDef--;
        ChangeStatBuffs(battler, -1, STAT_DEF, MOVE_EFFECT_AFFECTS_USER | STAT_BUFF_DONT_SET_BUFFERS, NULL);
    }
    if (gVolatileStructs[battler].stockpileSpDef) {
        gVolatileStructs[battler].stockpileSpDef--;
        ChangeStatBuffs(battler, -1, STAT_SPDEF, MOVE_EFFECT_AFFECTS_USER | STAT_BUFF_DONT_SET_BUFFERS, NULL);
    }

    return TRUE;
}

static void Cmd_stockpiletobasedamage(void) {
    const u8* jumpPtr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    if (gVolatileStructs[gBattlerAttacker].stockpileCounter == 0) {
        gBattlescriptCurrInstr = jumpPtr;
    } else {
        if (gBattleCommunication[MISS_TYPE] != B_MSG_PROTECTED) gBattleScripting.animTurn = gVolatileStructs[gBattlerAttacker].stockpileCounter;

        gVolatileStructs[gBattlerAttacker].stockpileCounter = 0;
        // Restore stat changes from stockpile.
        ChangeStatBuffs(
            gBattlerAttacker, -gVolatileStructs[gBattlerAttacker].stockpileDef, STAT_DEF, MOVE_EFFECT_AFFECTS_USER | STAT_BUFF_DONT_SET_BUFFERS, NULL);
        ChangeStatBuffs(
            gBattlerAttacker, -gVolatileStructs[gBattlerAttacker].stockpileSpDef, STAT_SPDEF, MOVE_EFFECT_AFFECTS_USER | STAT_BUFF_DONT_SET_BUFFERS, NULL);
        gBattlescriptCurrInstr += 5;
    }
}

static void Cmd_stockpiletohpheal(void) {
    const u8* jumpPtr = T1_READ_PTR(gBattlescriptCurrInstr + 1);

    if (gVolatileStructs[gBattlerAttacker].stockpileCounter == 0) {
        gBattlescriptCurrInstr = jumpPtr;
        SetActiveMultistringChooser(B_MSG_SWALLOW_FAILED);
    } else {
        if (gBattleMons[gBattlerAttacker].status1 & STATUS1_BLEED) {
            gBattleMoveDamage = 0;
            gBattleScripting.animTurn = gVolatileStructs[gBattlerAttacker].stockpileCounter;
            gVolatileStructs[gBattlerAttacker].stockpileCounter = 0;
            gBattlescriptCurrInstr += 5;
            gBattlerTarget = gBattlerAttacker;
        } else if (gBattleMons[gBattlerAttacker].maxHP == gBattleMons[gBattlerAttacker].hp) {
            gVolatileStructs[gBattlerAttacker].stockpileCounter = 0;
            gBattlescriptCurrInstr = jumpPtr;
            gBattlerTarget = gBattlerAttacker;
            SetActiveMultistringChooser(B_MSG_SWALLOW_FULL_HP);
        } else {
            gBattleMoveDamage = gBattleMons[gBattlerAttacker].maxHP / (1 << (3 - gVolatileStructs[gBattlerAttacker].stockpileCounter));

            if (gBattleMoveDamage == 0) gBattleMoveDamage = 1;
            gBattleMoveDamage *= -1;

            gBattleScripting.animTurn = gVolatileStructs[gBattlerAttacker].stockpileCounter;
            gVolatileStructs[gBattlerAttacker].stockpileCounter = 0;
            gBattlescriptCurrInstr += 5;
            gBattlerTarget = gBattlerAttacker;
        }

        // Restore stat changes from stockpile.
        ChangeStatBuffs(
            gBattlerAttacker, -gVolatileStructs[gBattlerAttacker].stockpileDef, STAT_DEF, MOVE_EFFECT_AFFECTS_USER | STAT_BUFF_DONT_SET_BUFFERS, NULL);
        ChangeStatBuffs(
            gBattlerAttacker, -gVolatileStructs[gBattlerAttacker].stockpileDef, STAT_SPDEF, MOVE_EFFECT_AFFECTS_USER | STAT_BUFF_DONT_SET_BUFFERS, NULL);
    }
}

static void Cmd_checkcondition(void) {
    u8 targetSide = GetBattlerSide(gBattlerTarget);
    u16 condition = READ_FIRST_16_INC;
    const u8* jumpPtr = READ_PTR_INC;
    bool8 appliedEffect = FALSE;

    if (targetSide == GetBattlerSide(gBattlerAttacker)) {
        gBattlerTarget = BATTLE_OPPOSITE(gBattlerAttacker);
        if (!IsBattlerAlive(gBattlerTarget)) gBattlerTarget = BATTLE_PARTNER(gBattlerTarget);
        if (!IsBattlerAlive(gBattlerTarget)) gBattlerTarget = BATTLE_PARTNER(gBattlerTarget);
        targetSide = GetBattlerSide(gBattlerTarget);
    }

    switch (condition) {
        case CONDITION_SPIKES:
            if (gSideTimers[targetSide].spikesAmount < 3) {
                gSideTimers[targetSide].spikesAmount++;
                gSideStatuses[targetSide] |= SIDE_STATUS_SPIKES;
                appliedEffect = TRUE;
            }
            break;
        case CONDITION_TOXIC_SPIKES:
            if (gSideTimers[targetSide].toxicSpikesAmount < 2) {
                gSideTimers[targetSide].toxicSpikesAmount++;
                gSideStatuses[targetSide] |= SIDE_STATUS_TOXIC_SPIKES;
                appliedEffect = TRUE;
            }
            break;
        case CONDITION_STEALTH_ROCK:
            if (!(gSideStatuses[targetSide] & SIDE_STATUS_STEALTH_ROCK)) {
                gSideStatuses[targetSide] |= SIDE_STATUS_STEALTH_ROCK;
                gSideTimers[targetSide].stealthRockType = TYPE_ROCK;
                appliedEffect = TRUE;
            }
            break;
    }

    if (!appliedEffect) {
        gBattlescriptCurrInstr = jumpPtr;
    }
}

static u16 ReverseStatChangeMoveEffect(u16 moveEffect) {
    switch (moveEffect) {
        // +1
        case MOVE_EFFECT_ATK_PLUS_1:
            return MOVE_EFFECT_ATK_MINUS_1;
        case MOVE_EFFECT_DEF_PLUS_1:
            return MOVE_EFFECT_DEF_MINUS_1;
        case MOVE_EFFECT_SPD_PLUS_1:
            return MOVE_EFFECT_SPD_MINUS_1;
        case MOVE_EFFECT_SP_ATK_PLUS_1:
            return MOVE_EFFECT_SP_ATK_MINUS_1;
        case MOVE_EFFECT_SP_DEF_PLUS_1:
            return MOVE_EFFECT_SP_DEF_MINUS_1;
        case MOVE_EFFECT_ACC_PLUS_1:
            return MOVE_EFFECT_ACC_MINUS_1;
        case MOVE_EFFECT_EVS_PLUS_1:
            return MOVE_EFFECT_EVS_MINUS_1;
        // -1
        case MOVE_EFFECT_ATK_MINUS_1:
            return MOVE_EFFECT_ATK_PLUS_1;
        case MOVE_EFFECT_DEF_MINUS_1:
            return MOVE_EFFECT_DEF_PLUS_1;
        case MOVE_EFFECT_SPD_MINUS_1:
            return MOVE_EFFECT_SPD_PLUS_1;
        case MOVE_EFFECT_SP_ATK_MINUS_1:
            return MOVE_EFFECT_SP_ATK_PLUS_1;
        case MOVE_EFFECT_SP_DEF_MINUS_1:
            return MOVE_EFFECT_SP_DEF_PLUS_1;
        case MOVE_EFFECT_ACC_MINUS_1:
            return MOVE_EFFECT_ACC_PLUS_1;
        case MOVE_EFFECT_EVS_MINUS_1:
        // +2
        case MOVE_EFFECT_ATK_PLUS_2:
            return MOVE_EFFECT_ATK_MINUS_2;
        case MOVE_EFFECT_DEF_PLUS_2:
            return MOVE_EFFECT_DEF_MINUS_2;
        case MOVE_EFFECT_SPD_PLUS_2:
            return MOVE_EFFECT_SPD_MINUS_2;
        case MOVE_EFFECT_SP_ATK_PLUS_2:
            return MOVE_EFFECT_SP_ATK_MINUS_2;
        case MOVE_EFFECT_SP_DEF_PLUS_2:
            return MOVE_EFFECT_SP_DEF_MINUS_2;
        case MOVE_EFFECT_ACC_PLUS_2:
            return MOVE_EFFECT_ACC_MINUS_2;
        case MOVE_EFFECT_EVS_PLUS_2:
            return MOVE_EFFECT_EVS_MINUS_2;
        // -2
        case MOVE_EFFECT_ATK_MINUS_2:
            return MOVE_EFFECT_ATK_PLUS_2;
        case MOVE_EFFECT_DEF_MINUS_2:
            return MOVE_EFFECT_DEF_PLUS_2;
        case MOVE_EFFECT_SPD_MINUS_2:
            return MOVE_EFFECT_SPD_PLUS_2;
        case MOVE_EFFECT_SP_ATK_MINUS_2:
            return MOVE_EFFECT_SP_ATK_PLUS_2;
        case MOVE_EFFECT_SP_DEF_MINUS_2:
            return MOVE_EFFECT_SP_DEF_PLUS_2;
        case MOVE_EFFECT_ACC_MINUS_2:
            return MOVE_EFFECT_ACC_PLUS_2;
        case MOVE_EFFECT_EVS_MINUS_2:
            return MOVE_EFFECT_EVS_PLUS_2;
    }

    return moveEffect;
}

void SetStatChanger(u8 statId, s8 change) { SET_STATCHANGER_WITH_SIGN(statId, change); }

s8 ChangeStatBuffsImplicit(s8 statValue, u32 statId, u32 flags, const u8* BS_ptr) {
    return ChangeStatBuffs((flags & MOVE_EFFECT_AFFECTS_USER) ? gBattlerAttacker : gBattlerTarget, statValue, statId, flags, BS_ptr);
}

s8 ChangeStatBuffs(u8 battler, s8 statValue, u32 statId, u32 flags, const u8* BS_ptr) {
    bool32 certain = FALSE;
    bool32 notProtectAffected = FALSE;
    u32 index;
    bool32 affectsUser = (flags & MOVE_EFFECT_AFFECTS_USER);
    bool8 dontSetBuffers = flags & STAT_BUFF_DONT_SET_BUFFERS;
    int updateMoveEffect;
    int ability;

    SetStatChanger(statId, statValue);

    flags &= ~STAT_BUFF_DONT_SET_BUFFERS;
    flags &= ~MOVE_EFFECT_IGNORE_TYPE_IMMUNITIES;

    if (BS_ptr == NULL) {
        flags &= ~STAT_BUFF_ALLOW_PTR;
    }

    gActiveBattler = battler;

    gTurnStructs[battler].changedStatsBattlerId = gBattlerAttacker;

    flags &= ~(MOVE_EFFECT_AFFECTS_USER);

    if (flags & MOVE_EFFECT_CERTAIN) certain = TRUE;
    flags &= ~(MOVE_EFFECT_CERTAIN);

    if (flags & STAT_BUFF_NOT_PROTECT_AFFECTED) notProtectAffected++;
    flags &= ~(STAT_BUFF_NOT_PROTECT_AFFECTED);

    updateMoveEffect = flags & STAT_BUFF_UPDATE_MOVE_EFFECT;
    flags &= ~(STAT_BUFF_UPDATE_MOVE_EFFECT);

    if (BATTLER_HAS_ABILITY(battler, ABILITY_CONTRARY)) {
        statValue *= -1;
        if (updateMoveEffect) {
            gBattleScripting.moveEffect = ReverseStatChangeMoveEffect(gBattleScripting.moveEffect);
        }
    }

    if (dontSetBuffers) flags = 0;

    if (BattlerHasAbility(battler, ABILITY_SIMPLE, FALSE)) statValue *= 2;

    if (!affectsUser && BattlerHasAbility(gBattlerAttacker, ABILITY_SUBDUE, FALSE) && statValue <= -1) statValue *= 2;

    if (statValue <= -1)  // Stat decrease.
    {
        if (gSideTimers[GET_BATTLER_SIDE(battler)].mistTimer && !certain && gCurrentMove != MOVE_CURSE &&
            !(!affectsUser && Infiltrates(gBattlerAttacker, gCurrentMove, INFILTRATE_SCREENS))) {
            if (flags == STAT_BUFF_ALLOW_PTR) {
                if (gTurnStructs[battler].statLowered) {
                    gBattlescriptCurrInstr = BS_ptr;
                } else {
                    gBattleScripting.battler = battler;
                    BattleScriptPush(BS_ptr);
                    gBattlescriptCurrInstr = BattleScript_MistProtected;
                    gTurnStructs[battler].statLowered = TRUE;
                }
            } else if (updateMoveEffect && !gTurnStructs[battler].statLowered) {
                gBattleScripting.battler = battler;
                gTurnStructs[battler].statLowered = TRUE;
                BattleScriptCall(BattleScript_MistProtected);
            }
        } else if (gCurrentMove != MOVE_CURSE && notProtectAffected != TRUE && JumpIfMoveAffectedByProtect(0)) {
            if (flags == STAT_BUFF_ALLOW_PTR) {
                gBattlescriptCurrInstr = BattleScript_ButItFailed;
            }
            return 0;
        } else if ((ability = BATTLER_HAS_ABILITY(battler, ABILITY_CLEAR_BODY)) || (ability = BATTLER_HAS_ABILITY(battler, ABILITY_FULL_METAL_BODY)) ||
                   ((ability = BattlerHasAbility(battler, ABILITY_LUCKY_HALO, FALSE)) && affectsUser)) {
            if (flags == STAT_BUFF_ALLOW_PTR) {
                if (gTurnStructs[battler].statLowered) {
                    gBattlescriptCurrInstr = BS_ptr;
                } else {
                    gBattleScripting.abilityPopupOverwrite = ability;
                    gBattleScripting.battler = battler;
                    gBattlerAbility = battler;
                    BattleScriptPush(BS_ptr);
                    gBattlescriptCurrInstr = BattleScript_AbilityNoStatLoss;
                }
            } else if (updateMoveEffect && !gTurnStructs[battler].statLowered) {
                gBattleScripting.abilityPopupOverwrite = ability;
                gBattleScripting.battler = battler;
                gBattlerAbility = battler;
                gTurnStructs[battler].statLowered = TRUE;
                BattleScriptCall(BattleScript_AbilityNoStatLoss);
            }
            return 0;
        } else if ((index = IsFlowerVeilProtected(battler)) && !certain) {
            if (flags == STAT_BUFF_ALLOW_PTR) {
                if (gTurnStructs[battler].statLowered) {
                    gBattlescriptCurrInstr = BS_ptr;
                } else {
                    gBattleScripting.abilityPopupOverwrite = ABILITY_FLOWER_VEIL;
                    gBattleScripting.battler = battler;
                    gBattlerAbility = index - 1;
                    BattleScriptPush(BS_ptr);
                    gBattlescriptCurrInstr = BattleScript_FlowerVeilProtectsRet;
                    gTurnStructs[battler].statLowered = TRUE;
                }
            } else if (updateMoveEffect && !gTurnStructs[battler].statLowered) {
                gBattleScripting.abilityPopupOverwrite = ability;
                gBattleScripting.battler = battler;
                gBattlerAbility = battler;
                gTurnStructs[battler].statLowered = TRUE;
                BattleScriptCall(BattleScript_AbilityNoStatLoss);
            }
            return 0;
        } else if ((index = IsDesertCloakProtected(battler)) && !certain) {
            if (flags == STAT_BUFF_ALLOW_PTR) {
                if (gTurnStructs[battler].statLowered) {
                    gBattlescriptCurrInstr = BS_ptr;
                } else {
                    gBattleScripting.battler = battler;
                    gBattlerAbility = index - 1;
                    gBattleScripting.abilityPopupOverwrite = ABILITY_DESERT_CLOAK;
                    BattleScriptPush(BS_ptr);
                    gBattlescriptCurrInstr = BattleScript_DesertCloakProtectsRet;
                    gTurnStructs[battler].statLowered = TRUE;
                }
            } else if (updateMoveEffect && !gTurnStructs[battler].statLowered) {
                gBattleScripting.battler = battler;
                gBattlerAbility = index - 1;
                gBattleScripting.abilityPopupOverwrite = ABILITY_DESERT_CLOAK;
                gTurnStructs[battler].statLowered = TRUE;
                BattleScriptCall(BattleScript_DesertCloakProtectsRet);
            }
            return 0;
        } else if (!affectsUser &&
                   ((BATTLER_HAS_ABILITY(battler, ABILITY_KEEN_EYE) && statId == STAT_ACC && (gBattleScripting.abilityPopupOverwrite = ABILITY_KEEN_EYE)) ||
                    (BATTLER_HAS_ABILITY(battler, ABILITY_MINDS_EYE) && statId == STAT_ACC && (gBattleScripting.abilityPopupOverwrite = ABILITY_MINDS_EYE)) ||
                    (BATTLER_HAS_ABILITY(battler, ABILITY_HYPER_CUTTER) && (statId == STAT_ATK || statId == STAT_SPATK) &&
                     (gBattleScripting.abilityPopupOverwrite = ABILITY_HYPER_CUTTER)))) {
            if (flags == STAT_BUFF_ALLOW_PTR) {
                gBattleScripting.battler = battler;
                gBattlerAbility = battler;
                BattleScriptPush(BS_ptr);
                gBattlescriptCurrInstr = BattleScript_AbilityNoSpecificStatLoss;
            } else if (updateMoveEffect && !gTurnStructs[battler].statLowered) {
                gBattleScripting.battler = battler;
                gBattlerAbility = battler;
                BattleScriptCall(BattleScript_AbilityNoSpecificStatLoss);
            }
            return 0;
        } else if (BATTLER_HAS_ABILITY(battler, ABILITY_MIRROR_ARMOR) && !affectsUser && gBattlerAttacker != gBattlerTarget && battler == gBattlerTarget) {
            if (flags == STAT_BUFF_ALLOW_PTR) {
                gBattleScripting.abilityPopupOverwrite = ABILITY_MIRROR_ARMOR;
                SET_STATCHANGER_WITH_SIGN(statId, statValue);
                gBattleScripting.battler = battler;
                gBattlerAbility = battler;
                BattleScriptPush(BS_ptr);
                gBattlescriptCurrInstr = BattleScript_MirrorArmorReflect;
            } else if (updateMoveEffect && !gTurnStructs[battler].statLowered) {
                gBattleScripting.abilityPopupOverwrite = ABILITY_MIRROR_ARMOR;
                SET_STATCHANGER_WITH_SIGN(statId, statValue);
                gBattleScripting.battler = battler;
                gBattlerAbility = battler;
                BattleScriptCall(BattleScript_MirrorArmorReflect);
            }
            return 0;
        } else if (!certain && !affectsUser && GetBattlerHoldEffect(battler, TRUE) == HOLD_EFFECT_CLEAR_AMULET) {
            if (flags == STAT_BUFF_ALLOW_PTR) {
                if (gTurnStructs[battler].statLowered) {
                    gBattlescriptCurrInstr = BS_ptr;
                } else {
                    gBattleScripting.battler = battler;
                    BattleScriptPush(BS_ptr);
                    gBattlescriptCurrInstr = BattleScript_ItemStatsProtected;
                    gTurnStructs[battler].statLowered = TRUE;
                }
            } else if (updateMoveEffect && !gTurnStructs[battler].statLowered) {
                gLastUsedItem = gBattleMons[battler].item;
                gBattleScripting.battler = battler;
                gTurnStructs[battler].statLowered = TRUE;
                BattleScriptCall(BattleScript_ItemStatsProtected);
            }
            return 0;
        } else  // try to decrease
        {
            statValue = max(statValue, -gBattleMons[battler].statStages[statId]);

            if (!dontSetBuffers) {
                gBattleTextBuff2[0] = B_BUFF_PLACEHOLDER_BEGIN;
                index = 1;
                if (statValue == -2) {
                    gBattleTextBuff2[1] = B_BUFF_STRING;
                    gBattleTextBuff2[2] = STRINGID_STATHARSHLY;
                    gBattleTextBuff2[3] = STRINGID_STATHARSHLY >> 8;
                    index = 4;
                } else if (statValue <= -3) {
                    gBattleTextBuff2[1] = B_BUFF_STRING;
                    gBattleTextBuff2[2] = STRINGID_SEVERELY & 0xFF;
                    gBattleTextBuff2[3] = STRINGID_SEVERELY >> 8;
                    index = 4;
                }
                gBattleTextBuff2[index] = B_BUFF_STRING;
                index++;
                gBattleTextBuff2[index] = STRINGID_STATFELL;
                index++;
                gBattleTextBuff2[index] = STRINGID_STATFELL >> 8;
                index++;
                gBattleTextBuff2[index] = B_BUFF_EOS;

                if (gBattleMons[battler].statStages[statId] == MIN_STAT_STAGE) {
                    gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_STAT_WONT_DECREASE;
                } else {
                    gRoundStructs[battler].statFell = TRUE;                    // Eject pack, lash out
                    gBattleCommunication[MULTISTRING_CHOOSER] = !affectsUser;  // B_MSG_ATTACKER_STAT_FELL or B_MSG_DEFENDER_STAT_FELL
                }
            }
        }
    } else  // stat increase
    {
        statValue = min(statValue, MAX_STAT_STAGE - gBattleMons[battler].statStages[statId]);

        if (!dontSetBuffers) {
            gBattleTextBuff2[0] = B_BUFF_PLACEHOLDER_BEGIN;
            index = 1;
            if (statValue == 2) {
                gBattleTextBuff2[1] = B_BUFF_STRING;
                gBattleTextBuff2[2] = STRINGID_STATSHARPLY;
                gBattleTextBuff2[3] = STRINGID_STATSHARPLY >> 8;
                index = 4;
            } else if (statValue >= 3) {
                gBattleTextBuff2[1] = B_BUFF_STRING;
                gBattleTextBuff2[2] = STRINGID_DRASTICALLY & 0xFF;
                gBattleTextBuff2[3] = STRINGID_DRASTICALLY >> 8;
                index = 4;
            }
            gBattleTextBuff2[index] = B_BUFF_STRING;
            index++;
            gBattleTextBuff2[index] = STRINGID_STATROSE;
            index++;
            gBattleTextBuff2[index] = STRINGID_STATROSE >> 8;
            index++;
            gBattleTextBuff2[index] = B_BUFF_EOS;

            if (gBattleMons[battler].statStages[statId] == MAX_STAT_STAGE) {
                gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_STAT_WONT_INCREASE;
            } else {
                gBattleCommunication[MULTISTRING_CHOOSER] = (gBattlerTarget == battler);
                gRoundStructs[battler].statRaised = TRUE;
            }
        }
    }

    gBattleMons[battler].statStages[statId] += statValue;
    if (gBattleMons[battler].statStages[statId] < MIN_STAT_STAGE) gBattleMons[battler].statStages[statId] = MIN_STAT_STAGE;
    if (gBattleMons[battler].statStages[statId] > MAX_STAT_STAGE) gBattleMons[battler].statStages[statId] = MAX_STAT_STAGE;

    if (statValue && gBattleStruct->statStageCheckState != STAT_STAGE_CHECK_IN_PROGRESS && statId < NUM_BATTLE_STATS) {
        gBattleStruct->statStageCheckState = STAT_STAGE_CHECK_NEEDED;
        gBattleStruct->statChangesToCheck[battler][statId - 1] += statValue;
    }

    if (gBattleCommunication[MULTISTRING_CHOOSER] == B_MSG_STAT_WONT_INCREASE && flags & STAT_BUFF_ALLOW_PTR) gMoveResultFlags |= MOVE_RESULT_MISSED;

    if (gBattleCommunication[MULTISTRING_CHOOSER] == B_MSG_STAT_WONT_INCREASE && !(flags & STAT_BUFF_ALLOW_PTR)) return 0;

    return statValue;
}

static void Cmd_statbuffchange(void) {
    u16 flags = READ_FIRST_16_INC;
    const u8* jumpPtr = READ_PTR_INC;
    const u8* ptrBefore = gBattlescriptCurrInstr;

    u8 result = ChangeStatBuffsImplicit(GET_STAT_BUFF_VALUE_WITH_SIGN(gBattleScripting.statChanger), gBattleScripting.statChanger.statId, flags, jumpPtr);
    SetActiveMultistringChooser(gBattleCommunication[MULTISTRING_CHOOSER]);

    if (!result && jumpPtr && gBattlescriptCurrInstr == ptrBefore) gBattlescriptCurrInstr = jumpPtr;
}

bool32 TryResetBattlerStatChanges(u8 battler, s8 comparison) {
    u32 j;
    bool32 ret = FALSE;

    gVolatileStructs[battler].stockpileDef = 0;
    gVolatileStructs[battler].stockpileSpDef = 0;
    for (j = 0; j < NUM_BATTLE_STATS; j++) {
        switch (comparison) {
            case RESET_ALL_STATS:
                if (gBattleMons[battler].statStages[j] == DEFAULT_STAT_STAGE) continue;
                break;
            case RESET_STAT_BUFFS:
                if (gBattleMons[battler].statStages[j] <= DEFAULT_STAT_STAGE) continue;
                break;
            case RESET_STAT_DROPS:
                if (gBattleMons[battler].statStages[j] >= DEFAULT_STAT_STAGE) continue;
                break;
        }

        ret = TRUE;
        gBattleMons[battler].statStages[j] = DEFAULT_STAT_STAGE;
    }

    if ((comparison == RESET_ALL_STATS || comparison == RESET_STAT_BUFFS) && gVolatileStructs[battler].critBoost) {
        ret = TRUE;
        gVolatileStructs[battler].critBoost = 0;
    }

    return ret;
}

static void Cmd_normalisebuffs(void)  // haze
{
    s32 i;

    for (i = 0; i < gBattlersCount; i++) TryResetBattlerStatChanges(i, RESET_ALL_STATS);

    gBattlescriptCurrInstr++;
}

static void Cmd_setbide(void) {
    if (!gProcessingExtraAttacks) {
        gBattleMons[gBattlerAttacker].status2 |= STATUS2_MULTIPLETURNS;
        gLockedMoves[gBattlerAttacker] = gCurrentMove;
        gTakenDmg[gBattlerAttacker] = 0;
        gBattleMons[gBattlerAttacker].status2 |= STATUS2_BIDE_TURN(2);
    }

    gBattlescriptCurrInstr++;
}

static void Cmd_confuseifrepeatingattackends(void) {
    if (!(gBattleMons[gBattlerAttacker].status2 & STATUS2_LOCK_CONFUSE) && !gTurnStructs[gBattlerAttacker].dancerUsedMove)
        gBattleScripting.moveEffect = (MOVE_EFFECT_THRASH | MOVE_EFFECT_AFFECTS_USER);

    gBattlescriptCurrInstr++;
}

static void Cmd_setmultihitcounter(void) {
    if (gBattlescriptCurrInstr[1]) {
        gTurnStructs[gBattlerAttacker].multiHitCounter = gBattlescriptCurrInstr[1];
    } else {
        if (BattlerHasAbility(gBattlerAttacker, ABILITY_SKILL_LINK, FALSE) || BattlerHasAbility(gBattlerAttacker, ABILITY_KUNOICHI_BLADE, FALSE)) {
            gTurnStructs[gBattlerAttacker].multiHitCounter = 5;
        } else if (GetBattlerHoldEffect(gBattlerAttacker, TRUE) == HOLD_EFFECT_LOADED_DICE) {
            gTurnStructs[gBattlerAttacker].multiHitCounter = 4 + (Random() % 2);
        } else {
            // 2 and 3 hits: 33.3%
            // 4 and 5 hits: 16.7%
            gTurnStructs[gBattlerAttacker].multiHitCounter = Random() % 4;
            if (gTurnStructs[gBattlerAttacker].multiHitCounter > 2) {
                gTurnStructs[gBattlerAttacker].multiHitCounter = (Random() % 3);
                if (gTurnStructs[gBattlerAttacker].multiHitCounter < 2)
                    gTurnStructs[gBattlerAttacker].multiHitCounter = 2;
                else
                    gTurnStructs[gBattlerAttacker].multiHitCounter = 3;
            } else
                gTurnStructs[gBattlerAttacker].multiHitCounter += 3;
        }
    }

    gBattlescriptCurrInstr += 2;
}

static void Cmd_initmultihitstring(void) {
    PREPARE_BYTE_NUMBER_BUFFER(gBattleScripting.multihitString, 1, 0)

    gBattlescriptCurrInstr++;
}

static void Cmd_forcerandomswitch(void) {
    s32 i;
    s32 battler1PartyId = 0;
    s32 battler2PartyId = 0;

    s32 firstMonId;
    s32 lastMonId = 0;  // + 1
    s32 monsCount;
    struct Pokemon* party = NULL;
    s32 validMons = 0;
    s32 minNeeded;

    bool32 redCardForcedSwitch = FALSE;

    // Red card checks against wild pokemon. If we have reached here, the player has a mon to switch into
    // Red card swaps attacker with target to get the animation correct, so here we check attacker which is really the target. Thanks GF...
    if (gBattleScripting.switchCase == B_SWITCH_RED_CARD && !(gBattleTypeFlags & BATTLE_TYPE_TRAINER) &&
        GetBattlerSide(gBattlerAttacker) == B_SIDE_OPPONENT)  // Check opponent's red card activating
    {
        if (!WILD_DOUBLE_BATTLE) {
            // Wild mon with red card will end single battle
            gBattlescriptCurrInstr = BattleScript_RoarSuccessEndBattle;
            return;
        } else {
            // Wild double battle, wild mon red card activation on player
            if (IS_WHOLE_SIDE_ALIVE(gBattlerTarget)) {
                // Both player's battlers are alive
                redCardForcedSwitch = FALSE;
            } else {
                // Player has only one mon alive -> force red card switch before manually switching to other mon
                redCardForcedSwitch = TRUE;
            }
        }
    }

    // Swap battlers for Red Card; ths will switch out the attacker instead of the target
    if (gBattlescriptCurrInstr[5] == TRUE) {
        i = gBattlerAttacker;
        gBattlerAttacker = gBattlerTarget;
        gBattlerTarget = i;
    }

    // Swapping pokemon happens in:
    // trainer battles
    // wild double battles when an opposing pokemon uses it against one of the two alive player mons
    // wild double battle when a player pokemon uses it against its partner
    if ((gBattleTypeFlags & BATTLE_TYPE_TRAINER) ||
        (WILD_DOUBLE_BATTLE && GetBattlerSide(gBattlerAttacker) == B_SIDE_OPPONENT && GetBattlerSide(gBattlerTarget) == B_SIDE_PLAYER &&
         IS_WHOLE_SIDE_ALIVE(gBattlerTarget)) ||
        (WILD_DOUBLE_BATTLE && GetBattlerSide(gBattlerAttacker) == B_SIDE_PLAYER && GetBattlerSide(gBattlerTarget) == B_SIDE_PLAYER) || redCardForcedSwitch) {
        if (GetBattlerSide(gBattlerTarget) == B_SIDE_PLAYER)
            party = gPlayerParty;
        else
            party = gEnemyParty;

        if (BATTLE_TWO_VS_ONE_OPPONENT && GetBattlerSide(gBattlerTarget) == B_SIDE_OPPONENT) {
            firstMonId = 0;
            lastMonId = 6;
            monsCount = 6;
            minNeeded = 2;
            battler2PartyId = gBattlerPartyIndexes[gBattlerTarget];
            battler1PartyId = gBattlerPartyIndexes[gBattlerTarget ^ BIT_FLANK];
        } else if ((gBattleTypeFlags & BATTLE_TYPE_BATTLE_TOWER && gBattleTypeFlags & BATTLE_TYPE_LINK) ||
                   (gBattleTypeFlags & BATTLE_TYPE_BATTLE_TOWER && gBattleTypeFlags & BATTLE_TYPE_RECORDED_LINK) ||
                   (gBattleTypeFlags & BATTLE_TYPE_INGAME_PARTNER)) {
            if ((gBattlerTarget & BIT_FLANK) != 0) {
                firstMonId = 3;
                lastMonId = 6;
            } else {
                firstMonId = 0;
                lastMonId = 3;
            }
            monsCount = 3;
            minNeeded = 1;
            battler2PartyId = gBattlerPartyIndexes[gBattlerTarget];
            battler1PartyId = gBattlerPartyIndexes[gBattlerTarget ^ BIT_FLANK];
        } else if ((gBattleTypeFlags & BATTLE_TYPE_MULTI && gBattleTypeFlags & BATTLE_TYPE_LINK) ||
                   (gBattleTypeFlags & BATTLE_TYPE_MULTI && gBattleTypeFlags & BATTLE_TYPE_RECORDED_LINK)) {
            if (GetLinkTrainerFlankId(GetBattlerMultiplayerId(gBattlerTarget)) == 1) {
                firstMonId = 3;
                lastMonId = 6;
            } else {
                firstMonId = 0;
                lastMonId = 3;
            }
            monsCount = 3;
            minNeeded = 1;
            battler2PartyId = gBattlerPartyIndexes[gBattlerTarget];
            battler1PartyId = gBattlerPartyIndexes[gBattlerTarget ^ BIT_FLANK];
        } else if (gBattleTypeFlags & BATTLE_TYPE_TWO_OPPONENTS) {
            if (GetBattlerSide(gBattlerTarget) == B_SIDE_PLAYER) {
                firstMonId = 0;
                lastMonId = 6;
                monsCount = 6;
                minNeeded = 2;  // since there are two opponents, it has to be a double battle
            } else {
                if ((gBattlerTarget & BIT_FLANK) != 0) {
                    firstMonId = 3;
                    lastMonId = 6;
                } else {
                    firstMonId = 0;
                    lastMonId = 3;
                }
                monsCount = 3;
                minNeeded = 1;
            }
            battler2PartyId = gBattlerPartyIndexes[gBattlerTarget];
            battler1PartyId = gBattlerPartyIndexes[gBattlerTarget ^ BIT_FLANK];
        } else if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE) {
            firstMonId = 0;
            lastMonId = 6;
            monsCount = 6;
            minNeeded = 2;
            battler2PartyId = gBattlerPartyIndexes[gBattlerTarget];
            battler1PartyId = gBattlerPartyIndexes[gBattlerTarget ^ BIT_FLANK];
        } else {
            firstMonId = 0;
            lastMonId = 6;
            monsCount = 6;
            minNeeded = 1;
            battler2PartyId = gBattlerPartyIndexes[gBattlerTarget];  // there is only one pokemon out in single battles
            battler1PartyId = gBattlerPartyIndexes[gBattlerTarget];
        }

        for (i = firstMonId; i < lastMonId; i++) {
            if (GetMonData(&party[i], MON_DATA_SPECIES) != SPECIES_NONE && !GetMonData(&party[i], MON_DATA_IS_EGG) && GetMonData(&party[i], MON_DATA_HP) != 0) {
                validMons++;
            }
        }

        if (!redCardForcedSwitch && validMons <= minNeeded) {
            gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
        } else {
            *(gBattleStruct->battlerPartyIndexes + gBattlerTarget) = gBattlerPartyIndexes[gBattlerTarget];
            gBattlescriptCurrInstr = BattleScript_RoarSuccessSwitch;

            do {
                i = Random() % monsCount;
                i += firstMonId;
            } while (i == battler2PartyId || i == battler1PartyId || GetMonData(&party[i], MON_DATA_SPECIES) == SPECIES_NONE ||
                     GetMonData(&party[i], MON_DATA_IS_EGG) == TRUE || GetMonData(&party[i], MON_DATA_HP) == 0);

            *(gBattleStruct->monToSwitchIntoId + gBattlerTarget) = i;

            if (!IsMultiBattle()) SwitchPartyOrder(gBattlerTarget);

            if ((gBattleTypeFlags & BATTLE_TYPE_LINK && gBattleTypeFlags & BATTLE_TYPE_BATTLE_TOWER) ||
                (gBattleTypeFlags & BATTLE_TYPE_LINK && gBattleTypeFlags & BATTLE_TYPE_MULTI) ||
                (gBattleTypeFlags & BATTLE_TYPE_RECORDED_LINK && gBattleTypeFlags & BATTLE_TYPE_BATTLE_TOWER) ||
                (gBattleTypeFlags & BATTLE_TYPE_RECORDED_LINK && gBattleTypeFlags & BATTLE_TYPE_MULTI)) {
                SwitchPartyOrderLinkMulti(gBattlerTarget, i, 0);
                SwitchPartyOrderLinkMulti(gBattlerTarget ^ BIT_FLANK, i, 1);
            }

            if (gBattleTypeFlags & BATTLE_TYPE_INGAME_PARTNER) SwitchPartyOrderInGameMulti(gBattlerTarget, i);
        }
    } else {
        // In normal wild doubles, Roar will always fail if the user's level is less than the target's.
        if (gBattleMons[gBattlerAttacker].level >= gBattleMons[gBattlerTarget].level)
            gBattlescriptCurrInstr = BattleScript_RoarSuccessEndBattle;
        else
            gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    }
}

static void Cmd_tryconversiontypechange(void) {
    u8 i;
    u16 move;
    u8 type;
    u8 type2;
    struct BattlePokemon* mon;

    for (i = 0; i < MAX_MON_MOVES; i++) {
        move = gBattleMons[gBattlerAttacker].moves[i];
        if (!move) continue;

        type = gBattleMoves[move].type;
        if (type == TYPE_MYSTERY) continue;
        type2 = gBattleMoves[move].type2;
        if (!type2 || type2 == TYPE_MYSTERY) type2 = type;

        PREPARE_TYPE_BUFFER(gBattleTextBuff1, type);

        mon = &gBattleMons[gBattlerAttacker];
        if (mon->type1 == type && mon->type2 == type2 && mon->type3 == TYPE_MYSTERY) break;

        mon->type1 = type;
        mon->type2 = type2;
        mon->type3 = TYPE_MYSTERY;

        PREPARE_TYPE_BUFFER(gBattleTextBuff1, type);
        gBattlescriptCurrInstr += 5;
        return;
    }

    gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
}

static void Cmd_givepaydaymoney(void) {
    if (!(gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_RECORDED_LINK)) && gPaydayMoney != 0) {
        u32 bonusMoney = gPaydayMoney * gBattleStruct->moneyMultiplier;
        AddMoney(&gSaveBlock1Ptr->money, bonusMoney);

        PREPARE_HWORD_NUMBER_BUFFER(gBattleTextBuff1, 5, bonusMoney)

        BattleScriptPush(gBattlescriptCurrInstr + 1);
        gBattlescriptCurrInstr = BattleScript_PrintPayDayMoneyString;
    } else {
        gBattlescriptCurrInstr++;
    }
}

static void Cmd_setlightscreen(void) {
    if (gSideStatuses[GET_BATTLER_SIDE(gBattlerAttacker)] & SIDE_STATUS_LIGHTSCREEN) {
        gMoveResultFlags |= MOVE_RESULT_MISSED;
        SetActiveMultistringChooser(B_MSG_SIDE_STATUS_FAILED);
    } else {
        int side = GET_BATTLER_SIDE(gBattlerAttacker);
        gSideTimers[side].started.lightscreen = TRUE;
        gSideStatuses[side] |= SIDE_STATUS_LIGHTSCREEN;
        if (GetBattlerHoldEffect(gBattlerAttacker, TRUE) == HOLD_EFFECT_LIGHT_CLAY)
            gSideTimers[side].lightscreenTimer = SCREEN_DURATION_EXTENDED;
        else
            gSideTimers[side].lightscreenTimer = SCREEN_DURATION;
        gSideTimers[side].lightscreenBattlerId = gBattlerAttacker;

        if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE && CountAliveMonsInBattle(BATTLE_ALIVE_ATK_SIDE) == 2)
            SetActiveMultistringChooser(B_MSG_SET_LIGHTSCREEN_DOUBLE);
        else
            SetActiveMultistringChooser(B_MSG_SET_LIGHTSCREEN_SINGLE);
    }

    gBattlescriptCurrInstr++;
}

u16 IsBattlerImmuneToLowerStatsFromIntimidateClone(u8 battler) {
    int i;

    if (BattlerHasAbility(battler, ABILITY_GUARD_DOG, TRUE)) return FALSE;

    for (i = 0; i < TOTAL_ABILITY_COUNT; i++) {
        int ability = GetAbilityAtIndex(battler, i, TRUE);
        switch (ability) {
            case ABILITY_SCRAPPY:
            case ABILITY_BLIND_RAGE:
            case ABILITY_OBLIVIOUS:
            case ABILITY_VITAL_SPIRIT:
            case ABILITY_DISCIPLINE:
            case ABILITY_INNER_FOCUS:
            case ABILITY_UNLOCKED_POTENTIAL:
            case ABILITY_ENLIGHTENED:
            case ABILITY_WAY_OF_PRECISION:
            case ABILITY_OWN_TEMPO:
            case ABILITY_OVERWHELM:
                return ability;
        }
    }
    return FALSE;
}

#define BATTLEMACROS_SIZE 9
static void Cmd_battlemacros(void) {
    u8 type = T1_READ_8(gBattlescriptCurrInstr + 1);              //+1
    u16 num = T1_READ_16(gBattlescriptCurrInstr + 2);             //+2
    const u8* jumpPtr = T1_READ_PTR(gBattlescriptCurrInstr + 4);  //+4
    bool8 tryjump = FALSE;

    switch (type) {
        case MACROS_PRINT_MGBA_MESSAGE:
#ifdef DEBUG_BUILD
            if (FlagGet(FLAG_SYS_MGBA_PRINT)) {
                MgbaOpen();
                MgbaPrintf(MGBA_LOG_WARN, "Debug Stuff");
                MgbaClose();
            }
#endif
            break;
        case MACROS_FORCE_FALSE_SWIPE_EFFECT:
            gBattleScripting.forceFalseSwipeEffect = TRUE;
            break;
        case MACROS_RESET_MULTIHIT_HITS:
            gTurnStructs[gBattlerAttacker].multiHitCounter = 0;
            break;
            break;
        case MACROS_TRY_TO_ACTIVATE_INTIMIDATE_CLONE_TARGET_1:
            break;
            break;
        case MACROS_TRY_TO_ACTIVATE_INTIMIDATE_CLONE_TARGET_2:
            break;
        case MACROS_SAVE_ABILITY_TO_VARIABLE:
            VarSet(VAR_SAVED_ABILITY, gBattleScripting.abilityPopupOverwrite);
            break;
        case MACROS_OVERWRITE_NEXT_STRING: {
            if (VarGet(VAR_TEMP_BATTLE_STRING_OVERWRITE_1) == 0)
                VarSet(VAR_TEMP_BATTLE_STRING_OVERWRITE_1, num);
            else if (VarGet(VAR_TEMP_BATTLE_STRING_OVERWRITE_2) == 0)
                VarSet(VAR_TEMP_BATTLE_STRING_OVERWRITE_2, num);
            else if (VarGet(VAR_TEMP_BATTLE_STRING_OVERWRITE_3) == 0)
                VarSet(VAR_TEMP_BATTLE_STRING_OVERWRITE_3, num);
            else if (VarGet(VAR_TEMP_BATTLE_STRING_OVERWRITE_4) == 0)
                VarSet(VAR_TEMP_BATTLE_STRING_OVERWRITE_4, num);
        } break;
        case MACROS_CLEAN_OVERWRITEN_STRINGS: {
            u16 newValue = 0;
            VarSet(VAR_TEMP_BATTLE_STRING_OVERWRITE_1, newValue);
            VarSet(VAR_TEMP_BATTLE_STRING_OVERWRITE_2, newValue);
            VarSet(VAR_TEMP_BATTLE_STRING_OVERWRITE_3, newValue);
            VarSet(VAR_TEMP_BATTLE_STRING_OVERWRITE_4, newValue);
        } break;
    }

    if (tryjump) {
        gBattlescriptCurrInstr = jumpPtr;
    } else {
        gBattlescriptCurrInstr += 8;
    }
}

static void Cmd_jumpifabilityonside(void)  // King's wrath + intimidate
{
    u32 battlerId = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);
    bool32 hasAbility = FALSE;
    u32 ability = T2_READ_16(gBattlescriptCurrInstr + 2);
    u32 abilityBattlerId = 0;
    int opposite = T1_READ_8(gBattlescriptCurrInstr + 8);

    if (opposite) battlerId = BATTLE_OPPOSITE(battlerId);

    abilityBattlerId = IsAbilityOnSide(battlerId, ability);
    if (abilityBattlerId) {
        abilityBattlerId--;
        hasAbility = TRUE;
    }

    if (hasAbility) {
        gBattleScripting.abilityPopupOverwrite = ability;
        gBattlescriptCurrInstr = T2_READ_PTR(gBattlescriptCurrInstr + 4);
        gBattlerAbility = abilityBattlerId;
    } else {
        gBattlescriptCurrInstr += 9;
    }
}

static void Cmd_setsandstorm(void) {
    if (!TryChangeBattleWeather(gBattlerAttacker, ENUM_WEATHER_SANDSTORM, FALSE)) {
        gMoveResultFlags |= MOVE_RESULT_MISSED;
        SetActiveMultistringChooser(B_MSG_WEATHER_FAILED);
    } else {
        SetActiveMultistringChooser(B_MSG_STARTED_SANDSTORM);
    }
    gBattlescriptCurrInstr++;
}

static void Cmd_weatherdamage(void) {
    gBattleMoveDamage = 0;
    if (IsBattlerAlive(gBattlerAttacker) && WEATHER_HAS_EFFECT  // Sandstorm damage
        && !(IsMagicGuardProtected(gBattlerAttacker))) {
        if (gBattleWeather & WEATHER_SANDSTORM_ANY) {
            if (!IS_BATTLER_OF_TYPE(gBattlerAttacker, TYPE_ROCK) && !IS_BATTLER_OF_TYPE(gBattlerAttacker, TYPE_GROUND) &&
                !IS_BATTLER_OF_TYPE(gBattlerAttacker, TYPE_STEEL) && !IsAbilityOnSide(gBattlerAttacker, ABILITY_DESERT_CLOAK) &&
                !BATTLER_HAS_ABILITY(gBattlerAttacker, ABILITY_SAND_VEIL) && !BATTLER_HAS_ABILITY(gBattlerAttacker, ABILITY_SAND_FORCE) &&
                !BATTLER_HAS_ABILITY(gBattlerAttacker, ABILITY_SAND_RUSH) && !BATTLER_HAS_ABILITY(gBattlerAttacker, ABILITY_OVERCOAT) &&
                !(gStatuses3[gBattlerAttacker] & (STATUS3_UNDERGROUND | STATUS3_UNDERWATER)) &&
                GetBattlerHoldEffect(gBattlerAttacker, TRUE) != HOLD_EFFECT_SAFETY_GOGGLES) {
                gBattleMoveDamage = gBattleMons[gBattlerAttacker].maxHP / 16;
                if (gBattleMoveDamage == 0) gBattleMoveDamage = 1;
            }
        }
        if (gBattleWeather & WEATHER_HAIL_ANY)  // Hail damage
        {
            if (!IS_BATTLER_OF_TYPE(gBattlerAttacker, TYPE_ICE) && !BATTLER_HAS_ABILITY(gBattlerAttacker, ABILITY_SNOW_CLOAK) &&
                !BATTLER_HAS_ABILITY(gBattlerAttacker, ABILITY_OVERCOAT) && !BATTLER_HAS_ABILITY(gBattlerAttacker, ABILITY_AURORA_BOREALIS) &&
                !BATTLER_HAS_ABILITY(gBattlerAttacker, ABILITY_ICE_BODY) && !BATTLER_HAS_ABILITY(gBattlerAttacker, ABILITY_SLUSH_RUSH) &&
                !BATTLER_HAS_ABILITY(gBattlerAttacker, ABILITY_NORTH_WIND) && !(gStatuses3[gBattlerAttacker] & (STATUS3_UNDERGROUND | STATUS3_UNDERWATER)) &&
                GetBattlerHoldEffect(gBattlerAttacker, TRUE) != HOLD_EFFECT_SAFETY_GOGGLES) {
                gBattleMoveDamage = gBattleMons[gBattlerAttacker].maxHP / 16;
                if (gBattleMoveDamage == 0) gBattleMoveDamage = 1;
            }
        }
    }

    gBattlescriptCurrInstr++;
}

static void Cmd_tryinfatuating(void) {
    if (CanInfatuate(gBattlerAttacker, gBattlerTarget)) {
        gBattleMons[gBattlerTarget].status2 |= STATUS2_INFATUATED_WITH(gBattlerAttacker);
        gBattlescriptCurrInstr += 5;
    } else if (BattlerHasAbility(gBattlerTarget, ABILITY_OBLIVIOUS, TRUE)) {
        SetActiveAbilityPopupOverride(ABILITY_OBLIVIOUS);
        gBattlescriptCurrInstr = BattleScript_NotAffectedAbilityPopUp;
    } else {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    }
}

static void Cmd_updatestatusicon(void) {
    if (gBattleControllerExecFlags) return;

    if (gBattlescriptCurrInstr[1] != BS_ATTACKER_WITH_PARTNER) {
        gActiveBattler = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);
        BtlController_EmitStatusIconUpdate(0, gBattleMons[gActiveBattler].status1, gBattleMons[gActiveBattler].status2);
        MarkBattlerForControllerExec(gActiveBattler);
        gBattlescriptCurrInstr += 2;
    } else {
        gActiveBattler = gBattlerAttacker;
        if (!(gAbsentBattlerFlags & gBitTable[gActiveBattler])) {
            BtlController_EmitStatusIconUpdate(0, gBattleMons[gActiveBattler].status1, gBattleMons[gActiveBattler].status2);
            MarkBattlerForControllerExec(gActiveBattler);
        }
        if ((gBattleTypeFlags & BATTLE_TYPE_DOUBLE)) {
            gActiveBattler = GetBattlerAtPosition(GetBattlerPosition(gBattlerAttacker) ^ BIT_FLANK);
            if (!(gAbsentBattlerFlags & gBitTable[gActiveBattler])) {
                BtlController_EmitStatusIconUpdate(0, gBattleMons[gActiveBattler].status1, gBattleMons[gActiveBattler].status2);
                MarkBattlerForControllerExec(gActiveBattler);
            }
        }
        gBattlescriptCurrInstr += 2;
    }
}

static void Cmd_setmist(void) {
    if (gSideTimers[GET_BATTLER_SIDE(gBattlerAttacker)].mistTimer) {
        gMoveResultFlags |= MOVE_RESULT_FAILED;
        SetActiveMultistringChooser(B_MSG_MIST_FAILED);
    } else {
        int side = GET_BATTLER_SIDE(gBattlerAttacker);
        gSideTimers[side].started.mist = TRUE;
        gSideTimers[side].mistTimer = SCREEN_DURATION;
        gSideTimers[side].mistBattlerId = gBattlerAttacker;
        gSideStatuses[side] |= SIDE_STATUS_MIST;
        SetActiveMultistringChooser(B_MSG_SET_MIST);
    }
    gBattlescriptCurrInstr++;
}

static void Cmd_setfocusenergy(void) {
    if (gBattleMons[gBattlerAttacker].status2 & STATUS2_FOCUS_ENERGY) {
        gMoveResultFlags |= MOVE_RESULT_FAILED;
        SetActiveMultistringChooser(B_MSG_FOCUS_ENERGY_FAILED);
    } else {
        gBattleMons[gBattlerAttacker].status2 |= STATUS2_FOCUS_ENERGY;
        SetActiveMultistringChooser(B_MSG_GETTING_PUMPED);
    }
    gBattlescriptCurrInstr++;
}

static void Cmd_transformdataexecution(void) {
    gChosenMove = 0xFFFF;
    gBattlescriptCurrInstr++;
    if (gBattleMons[gBattlerTarget].status2 & STATUS2_TRANSFORMED || gBattleStruct->illusion[gBattlerTarget].on ||
        gStatuses3[gBattlerTarget] & STATUS3_SEMI_INVULNERABLE) {
        gMoveResultFlags |= MOVE_RESULT_FAILED;
        SetActiveMultistringChooser(B_MSG_TRANSFORM_FAILED);
    } else {
        s32 i;
        struct BattlePokemon *battleMonAttacker, *battleMonTarget;

        // TODO: THIS NEEDS TO BE FIXED

        gBattleMons[gBattlerAttacker].status2 |= STATUS2_TRANSFORMED;
        gVolatileStructs[gBattlerAttacker].disabledMove = 0;
        gVolatileStructs[gBattlerAttacker].disableTimer = 0;
        gVolatileStructs[gBattlerAttacker].transformedMonPersonality = gBattleMons[gBattlerTarget].personality;
        gVolatileStructs[gBattlerAttacker].mimickedMoves = 0;
        gVolatileStructs[gBattlerAttacker].usedMoves = 0;

        PREPARE_SPECIES_BUFFER(gBattleTextBuff1, gBattleMons[gBattlerTarget].species)

        battleMonAttacker = &gBattleMons[gBattlerAttacker];
        battleMonTarget = &gBattleMons[gBattlerTarget];

        battleMonAttacker->species = battleMonTarget->species;
        battleMonAttacker->attack = battleMonTarget->attack;
        battleMonAttacker->defense = battleMonTarget->defense;
        battleMonAttacker->speed = battleMonTarget->speed;
        battleMonAttacker->spAttack = battleMonTarget->spAttack;
        battleMonAttacker->spDefense = battleMonTarget->spDefense;
        battleMonAttacker->abilityNum = battleMonTarget->abilityNum;
        battleMonAttacker->type1 = battleMonTarget->type1;
        battleMonAttacker->type2 = battleMonTarget->type2;
        battleMonAttacker->type3 = battleMonTarget->type3;
        battleMonAttacker->speedDown = battleMonTarget->speedDown;
        ARRAY_COPY(battleMonAttacker->abilities, battleMonTarget->abilities)
        ARRAY_COPY(battleMonAttacker->moves, battleMonTarget->moves)
        ARRAY_COPY(battleMonAttacker->statStages, battleMonTarget->statStages)

        for (i = 0; i < MAX_MON_MOVES; i++) {
            if (gBattleMoves[gBattleMons[gBattlerAttacker].moves[i]].pp < 5)
                gBattleMons[gBattlerAttacker].pp[i] = gBattleMoves[battleMonAttacker->moves[i]].pp;
            else
                gBattleMons[gBattlerAttacker].pp[i] = 5;
        }

        gActiveBattler = gBattlerAttacker;
        BtlController_EmitResetActionMoveSelection(0, RESET_MOVE_SELECTION);
        MarkBattlerForControllerExec(gActiveBattler);
        SetActiveMultistringChooser(B_MSG_TRANSFORMED);
    }
}

static void Cmd_setsubstitute(void) {
    u32 hp = gBattleMons[gBattlerAttacker].maxHP / 4;
    u32 damage = gBattleMons[gBattlerAttacker].maxHP / ((gBattleMoves[gCurrentMove].effect == EFFECT_SHED_TAIL) ? 2 : 4);
    damage = max(damage, 1);
    hp = max(hp, 1);

    if (gBattleMons[gBattlerAttacker].hp <= damage) {
        gBattleMoveDamage = 0;
        SetActiveMultistringChooser(B_MSG_SUBSTITUTE_FAILED);
    } else {
        gBattleMoveDamage = damage;  // one bit value will only work for pokemon which max hp can go to 1020(which is more than possible in games)

        gBattleMons[gBattlerAttacker].status2 |= STATUS2_SUBSTITUTE;
        gBattleMons[gBattlerAttacker].status2 &= ~(STATUS2_WRAPPED);
        gVolatileStructs[gBattlerAttacker].substituteHP = hp;
        SetActiveMultistringChooser(B_MSG_SET_SUBSTITUTE);
        gHitMarker |= HITMARKER_IGNORE_SUBSTITUTE;
    }

    gBattlescriptCurrInstr++;
}

static void Cmd_mimicattackcopy(void) {
    if (gBattleMoves[gLastMoves[gBattlerTarget]].mimicBanned || (gBattleMons[gBattlerAttacker].status2 & STATUS2_TRANSFORMED) ||
        gLastMoves[gBattlerTarget] == 0xFFFF) {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    } else {
        int i;

        for (i = 0; i < MAX_MON_MOVES; i++) {
            if (gBattleMons[gBattlerAttacker].moves[i] == gLastMoves[gBattlerTarget]) break;
        }

        if (i == MAX_MON_MOVES && gCurrMovePos < 4) {
            gChosenMove = 0xFFFF;
            gBattleMons[gBattlerAttacker].moves[gCurrMovePos] = gLastMoves[gBattlerTarget];
            if (gBattleMoves[gLastMoves[gBattlerTarget]].pp < 5)
                gBattleMons[gBattlerAttacker].pp[gCurrMovePos] = gBattleMoves[gLastMoves[gBattlerTarget]].pp;
            else
                gBattleMons[gBattlerAttacker].pp[gCurrMovePos] = 5;

            PREPARE_MOVE_BUFFER(gBattleTextBuff1, gLastMoves[gBattlerTarget])

            gVolatileStructs[gBattlerAttacker].mimickedMoves |= gBitTable[gCurrMovePos];
            gBattlescriptCurrInstr += 5;
        } else {
            gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
        }
    }
}

static void Cmd_metronome(void) {
    int move;
    int allowed;
    do {
        move = (Random() % (MOVES_COUNT - 1)) + 1;
        switch (gBattleMoves[move].effect) {
            case EFFECT_PROTECT:
            case EFFECT_PLACEHOLDER:
            case EFFECT_DO_NOTHING:
            case EFFECT_FOLLOW_ME:
            case EFFECT_TRICK:
                allowed = FALSE;
                break;

            default:
                allowed = TRUE;
        }
    } while (allowed && !gBattleMoves[move].metronomeBanned);

    gQueuedExtraAttackData[++gQueuedAttackCount] = (struct ExtraAttackActionStruct){
        .attacker = gBattlerAttacker,
        .move = move,
        .target = GetMoveTarget(move, 0),
        .movePos = MAX_MON_MOVES,
        .prankster = BattlerHasAbility(gBattlerAttacker, ABILITY_PRANKSTER, FALSE),
    };
    gBattlescriptCurrInstr++;
    return;
}

static int AdjustFixedDamageForParentalBond(int damage) {
    if (gTurnStructs[gBattlerAttacker].parentalBondOn) {
        return ApplyModifier(
            damage,
            GetParentalBondMultiplier(gTurnStructs[gBattlerAttacker].parentalBondTrigger,
                                      gTurnStructs[gBattlerAttacker].parentalBondInitialCount - gTurnStructs[gBattlerAttacker].parentalBondOn));
    }

    return damage;
}

static void Cmd_calculatesetdamage(void) {
    s32 baseDamage = 1;
    s32 randDamage;

    // Calculate Base Damage
    switch (gBattleMoves[gCurrentMove].effect) {
        case EFFECT_SKY_DROP:
        case EFFECT_LEVEL_DAMAGE:
            // Damage is the level of the Pokemon using the move
            baseDamage = gBattleMons[gBattlerAttacker].level;
            break;
        case EFFECT_DRAGON_RAGE:
            // Damage is always 40
            baseDamage = 40;
            break;
        case EFFECT_SONICBOOM:
            // Damage is always 20
            baseDamage = 20;
            break;
        case EFFECT_PSYWAVE:
            // Inflicts a random amount of damage, varying between 1 damage and 1.5× the user's level.
            randDamage = (Random() % 101);
            baseDamage = gBattleMons[gBattlerAttacker].level * (randDamage + 50) / 100;
            break;
        case EFFECT_SUPER_FANG_HAZE:
        case EFFECT_SUPER_FANG:
            // Inflicts damage equal to half of the target's current HP.
            baseDamage = gBattleMons[gBattlerTarget].hp / 2;
            break;
    }

    gBattleMoveDamage = AdjustFixedDamageForParentalBond(baseDamage);

    // Failsafe
    if (!gBattleMoveDamage) gBattleMoveDamage = 1;

    gBattlescriptCurrInstr++;
}

static void Cmd_trytoapplymoveeffect(void) {
    // Set move effect
    bool8 appliedEffect = FALSE;
    int secondaryEffectChance = GetMoveEffectChance(gBattlerAttacker, gCurrentMove, 0, gBattleMoves[gCurrentMove].secondaryEffectChance);
    u8 rand = (Random() % 100);

    switch (gBattleMoves[gCurrentMove].effect) {
        case EFFECT_ATTRACT_HIT:
            if (rand <= secondaryEffectChance) {
                if (TARGET_TURN_DAMAGED && CanInfatuate(gBattlerAttacker, gBattlerTarget)) {
                    gBattleMons[gBattlerTarget].status2 |= STATUS2_INFATUATED_WITH(gBattlerAttacker);
                    appliedEffect = TRUE;
                }
            }
            break;
        case EFFECT_CURSE_HIT:
            if (rand <= secondaryEffectChance) {
                if (IsMyceliumMightActive(gBattlerAttacker) && !(gBattleMons[gBattlerTarget].status2 & STATUS2_CURSED)) {
                    gBattleMons[gBattlerTarget].status2 |= STATUS2_CURSED;
                    appliedEffect = TRUE;
                } else if (!(gMoveResultFlags & MOVE_RESULT_NO_EFFECT) && IsBattlerAlive(gBattlerTarget) && TARGET_TURN_DAMAGED &&
                           !IS_BATTLER_OF_TYPE(gBattlerTarget, TYPE_GHOST) && !(gBattleMons[gBattlerTarget].status2 & STATUS2_CURSED)) {
                    gBattleMons[gBattlerTarget].status2 |= STATUS2_CURSED;
                    appliedEffect = TRUE;
                }
            }
            break;
        case EFFECT_CREEPING_THORNS_HIT:
        case EFFECT_STEALTH_ROCK_HIT:
            if (rand <= secondaryEffectChance) {
                if (!(gMoveResultFlags & MOVE_RESULT_NO_EFFECT) && TARGET_TURN_DAMAGED &&
                    !(gSideStatuses[GetBattlerSide(gBattlerTarget)] & SIDE_STATUS_STEALTH_ROCK)) {
                    gSideStatuses[GetBattlerSide(gBattlerTarget)] |= SIDE_STATUS_STEALTH_ROCK;
                    gSideTimers[GetBattlerSide(gBattlerTarget)].stealthRockType =
                        gBattleMoves[gCurrentMove].effect == EFFECT_STEALTH_ROCK_HIT ? TYPE_ROCK : TYPE_GRASS;
                    appliedEffect = TRUE;
                }
            }
            break;
        case EFFECT_SPIKE_HIT:
            if (rand <= secondaryEffectChance) {
                if (!(gMoveResultFlags & MOVE_RESULT_NO_EFFECT) && TARGET_TURN_DAMAGED && gSideTimers[GetBattlerSide(gBattlerTarget)].spikesAmount < 3) {
                    gSideStatuses[GetBattlerSide(gBattlerTarget)] |= SIDE_STATUS_SPIKES;
                    gSideTimers[GetBattlerSide(gBattlerTarget)].spikesAmount++;
                    appliedEffect = TRUE;
                }
            }
            break;
        case EFFECT_LEECH_SEED_HIT:
            if (rand <= secondaryEffectChance) {
                if (IsMyceliumMightActive(gBattlerAttacker) && !(gStatuses3[gBattlerTarget] & STATUS3_LEECHSEED)) {
                    gStatuses3[gBattlerTarget] |= gBattlerAttacker;
                    gStatuses3[gBattlerTarget] |= STATUS3_LEECHSEED;
                    appliedEffect = TRUE;
                } else if (!(gMoveResultFlags & MOVE_RESULT_NO_EFFECT) && gBattleMons[gBattlerTarget].hp != 0 &&
                           !IS_BATTLER_OF_TYPE(gBattlerTarget, TYPE_GRASS) && TARGET_TURN_DAMAGED && !(gStatuses3[gBattlerTarget] & STATUS3_LEECHSEED)) {
                    gStatuses3[gBattlerTarget] |= gBattlerAttacker;
                    gStatuses3[gBattlerTarget] |= STATUS3_LEECHSEED;
                    appliedEffect = TRUE;
                }
            }
            break;
        case EFFECT_STICKY_WEB_HIT:
            if (rand <= secondaryEffectChance) {
                if (!(gMoveResultFlags & MOVE_RESULT_NO_EFFECT) && TARGET_TURN_DAMAGED &&
                    !(gSideStatuses[GetBattlerSide(gBattlerTarget)] & SIDE_STATUS_STICKY_WEB)) {
                    gStatuses3[gBattlerTarget] |= gBattlerAttacker;
                    gSideStatuses[GetBattlerSide(gBattlerTarget)] |= SIDE_STATUS_STICKY_WEB;
                    appliedEffect = TRUE;
                }
            }
            break;
    }

    if (appliedEffect) {
        BattleScriptCall(T1_READ_PTR(gBattlescriptCurrInstr + 1));
    } else {
        BattleScriptCall(BattleScript_MoveEnd);
    }
}

static void Cmd_counterdamagecalculator(void) {
    u8 sideAttacker = GetBattlerSide(gBattlerAttacker);
    u8 sideTarget = GetBattlerSide(gRoundStructs[gBattlerAttacker].physicalBattlerId);

    if (gRoundStructs[gBattlerAttacker].physicalDmg && sideAttacker != sideTarget && gBattleMons[gRoundStructs[gBattlerAttacker].physicalBattlerId].hp) {
        gBattleMoveDamage = gRoundStructs[gBattlerAttacker].physicalDmg * 2;

        if (IsAffectedByFollowMe(gBattlerAttacker, sideTarget, gCurrentMove))
            gBattlerTarget = gSideTimers[sideTarget].followmeTarget;
        else
            gBattlerTarget = gRoundStructs[gBattlerAttacker].physicalBattlerId;

        gBattlescriptCurrInstr += 5;
    } else {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    }

    gBattleMoveDamage = AdjustFixedDamageForParentalBond(gBattleMoveDamage);
    if (!gBattleMoveDamage) gBattleMoveDamage = 1;
}

static void Cmd_mirrorcoatdamagecalculator(void)  // a copy of atkA1 with the physical -> special field changes
{
    u8 sideAttacker = GetBattlerSide(gBattlerAttacker);
    u8 sideTarget = GetBattlerSide(gRoundStructs[gBattlerAttacker].specialBattlerId);

    if (gRoundStructs[gBattlerAttacker].specialDmg && sideAttacker != sideTarget && gBattleMons[gRoundStructs[gBattlerAttacker].specialBattlerId].hp) {
        gBattleMoveDamage = gRoundStructs[gBattlerAttacker].specialDmg * 2;

        if (IsAffectedByFollowMe(gBattlerAttacker, sideTarget, gCurrentMove))
            gBattlerTarget = gSideTimers[sideTarget].followmeTarget;
        else
            gBattlerTarget = gRoundStructs[gBattlerAttacker].specialBattlerId;

        gBattlescriptCurrInstr += 5;
    } else {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    }

    gBattleMoveDamage = AdjustFixedDamageForParentalBond(gBattleMoveDamage);
    if (!gBattleMoveDamage) gBattleMoveDamage = 1;
}

static void Cmd_disablelastusedattack(void) {
    if (DisableLastUsedMove(gBattlerTarget)) {
        gBattlescriptCurrInstr += 5;
    } else {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    }
}

static bool8 DisableLastUsedMove(u8 battler) {
    s32 i;

    for (i = 0; i < MAX_MON_MOVES; i++) {
        if (gBattleMons[battler].moves[i] == gLastMoves[battler]) break;
    }
    if (gVolatileStructs[battler].disabledMove == 0 && i != MAX_MON_MOVES && gBattleMons[battler].pp[i] != 0) {
        PREPARE_MOVE_BUFFER(gBattleTextBuff1, gBattleMons[battler].moves[i])

        gVolatileStructs[battler].disabledMove = gBattleMons[battler].moves[i];
        if (B_DISABLE_TURNS == GEN_3)
            gVolatileStructs[battler].disableTimer = (Random() & 3) + 2;
        else if (B_DISABLE_TURNS == GEN_4)
            gVolatileStructs[battler].disableTimer = (Random() & 3) + 4;
        else
            gVolatileStructs[battler].disableTimer = 4;

        gVolatileStructs[battler].disableTimerStartValue = gVolatileStructs[battler].disableTimer;  // used to save the random amount of turns?

        return TRUE;
    }

    return FALSE;
}

int SetEncore(int target) {
    int i;
    for (i = 0; i < MAX_MON_MOVES; i++) {
        if (gBattleMons[target].moves[i] == gLastMoves[target]) break;
    }

    if (gLastMoves[target] == MOVE_STRUGGLE || gLastMoves[target] == MOVE_ENCORE || gLastMoves[target] == MOVE_MIRROR_MOVE) {
        i = MAX_MON_MOVES;
    }

    if (gVolatileStructs[target].encoredMove == 0 && i < MAX_MON_MOVES && gBattleMons[target].pp[i] != 0) {
        gVolatileStructs[target].encoredMove = gBattleMons[target].moves[i];
        gVolatileStructs[target].encoredMovePos = i;
        gVolatileStructs[target].encoreTimer = 3;
        gVolatileStructs[target].encoreTimerStartValue = gVolatileStructs[target].encoreTimer;
        return TRUE;
    }

    return FALSE;
}

static void Cmd_trysetencore(void) {
    const u8* ptr = READ_FIRST_PTR_INC;
    if (!SetEncore(gBattlerTarget)) gBattlescriptCurrInstr = ptr;
}

static void Cmd_painsplitdmgcalc(void) {
    if (!(DoesSubstituteBlockMove(gBattlerAttacker, gBattlerTarget, gCurrentMove))) {
        s32 hpDiff = (gBattleMons[gBattlerAttacker].hp + gBattleMons[gBattlerTarget].hp) / 2;
        s32 painSplitHp = gBattleMoveDamage = gBattleMons[gBattlerTarget].hp - hpDiff;
        u8* storeLoc = (void*)(&gBattleScripting.painSplitHp);

        storeLoc[0] = (painSplitHp);
        storeLoc[1] = (painSplitHp & 0x0000FF00) >> 8;
        storeLoc[2] = (painSplitHp & 0x00FF0000) >> 16;
        storeLoc[3] = (painSplitHp & 0xFF000000) >> 24;

        gBattleMoveDamage = gBattleMons[gBattlerAttacker].hp - hpDiff;
        gTurnStructs[gBattlerTarget].dmg = 0xFFFF;

        gBattlescriptCurrInstr += 5;
    } else {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    }
}

static void Cmd_settypetorandomresistance(void)  // conversion 2
{
    if (gLastLandedMoves[gBattlerAttacker] == 0 || gLastLandedMoves[gBattlerAttacker] == 0xFFFF) {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    } else if (IsTwoTurnsMove(gLastLandedMoves[gBattlerAttacker]) && gBattleMons[gLastHitBy[gBattlerAttacker]].status2 & STATUS2_MULTIPLETURNS) {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    } else {
        u32 i, resistTypes = 0;
        u32 hitByType = gLastHitByType[gBattlerAttacker];

        for (i = 0; i < NUMBER_OF_MON_TYPES; i++)  // Find all types that resist.
        {
            switch (GetTypeModifier(hitByType, i, gBattlerAttacker, gBattlerTarget)) {
                case UQ_4_12(0):
                case UQ_4_12(0.5):
                    resistTypes |= gBitTable[i];
                    break;
            }
        }

        while (resistTypes != 0) {
            i = Random() % NUMBER_OF_MON_TYPES;
            if (resistTypes & gBitTable[i]) {
                if (IS_BATTLER_OF_TYPE(gBattlerAttacker, i)) {
                    resistTypes &= ~(gBitTable[i]);  // Type resists, but the user is already of this type.
                } else {
                    SET_BATTLER_TYPE(gBattlerAttacker, i);
                    PREPARE_TYPE_BUFFER(gBattleTextBuff1, i);
                    gBattlescriptCurrInstr += 5;
                    return;
                }
            }
        }

        PREPARE_TYPE_BUFFER(gBattleTextBuff1, i);
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    }
}

static void Cmd_setalwayshitflag(void) {
    gStatuses3[gBattlerTarget] &= ~(STATUS3_ALWAYS_HITS);
    gStatuses3[gBattlerTarget] |= STATUS3_ALWAYS_HITS_TURN(2);
    gVolatileStructs[gBattlerTarget].battlerWithSureHit = gBattlerAttacker;
    gBattlescriptCurrInstr++;
}

static void Cmd_copymovepermanently(void)  // sketch
{
    gChosenMove = 0xFFFF;

    if (!(gBattleMons[gBattlerAttacker].status2 & STATUS2_TRANSFORMED) && gLastPrintedMoves[gBattlerTarget] != MOVE_STRUGGLE &&
        gLastPrintedMoves[gBattlerTarget] != 0 && gLastPrintedMoves[gBattlerTarget] != 0xFFFF && gLastPrintedMoves[gBattlerTarget] != MOVE_SKETCH) {
        s32 i;

        for (i = 0; i < MAX_MON_MOVES; i++) {
            if (gBattleMons[gBattlerAttacker].moves[i] == MOVE_SKETCH) continue;
            if (gBattleMons[gBattlerAttacker].moves[i] == gLastPrintedMoves[gBattlerTarget]) break;
        }

        if (i != MAX_MON_MOVES || gCurrMovePos >= 4) {
            gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
        } else  // sketch worked
        {
            struct MovePpInfo movePpData;

            gBattleMons[gBattlerAttacker].moves[gCurrMovePos] = gLastPrintedMoves[gBattlerTarget];
            gBattleMons[gBattlerAttacker].pp[gCurrMovePos] = gBattleMoves[gLastPrintedMoves[gBattlerTarget]].pp;
            gActiveBattler = gBattlerAttacker;

            for (i = 0; i < MAX_MON_MOVES; i++) {
                movePpData.moves[i] = gBattleMons[gBattlerAttacker].moves[i];
                movePpData.pp[i] = gBattleMons[gBattlerAttacker].pp[i];
            }
            movePpData.ppBonuses = gBattleMons[gBattlerAttacker].ppBonuses;

            BtlController_EmitSetMonData(0, REQUEST_MOVES_PP_BATTLE, 0, sizeof(struct MovePpInfo), &movePpData);
            MarkBattlerForControllerExec(gActiveBattler);

            PREPARE_MOVE_BUFFER(gBattleTextBuff1, gLastPrintedMoves[gBattlerTarget])

            gBattlescriptCurrInstr += 5;
        }
    } else {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    }
}

static bool8 IsTwoTurnsMove(u16 move) {
    if (gBattleMoves[move].effect == EFFECT_SKULL_BASH || gBattleMoves[move].effect == EFFECT_TWO_TURNS_ATTACK ||
        gBattleMoves[move].effect == EFFECT_SOLARBEAM || gBattleMoves[move].effect == EFFECT_SEMI_INVULNERABLE || gBattleMoves[move].effect == EFFECT_BIDE)
        return TRUE;
    else
        return FALSE;
}

// unused
static u8 AttacksThisTurn(u8 battlerId, u16 move)  // Note: returns 1 if it's a charging turn, otherwise 2
{
    // first argument is unused
    if (gBattleMoves[move].effect == EFFECT_SOLARBEAM && (IsBattlerWeatherAffected(battlerId, WEATHER_SUN_ANY) || HasChloroplast(gBattlerAttacker))) return 2;

    if (gBattleMoves[move].effect == EFFECT_ELECTRO_SHOT && IsBattlerWeatherAffected(battlerId, WEATHER_RAIN_ANY)) return 2;

    if (gBattleMoves[move].effect == EFFECT_SKULL_BASH || gBattleMoves[move].effect == EFFECT_TWO_TURNS_ATTACK ||
        gBattleMoves[move].effect == EFFECT_SOLARBEAM || gBattleMoves[move].effect == EFFECT_SEMI_INVULNERABLE || gBattleMoves[move].effect == EFFECT_BIDE) {
        if ((gHitMarker & HITMARKER_CHARGING)) return 1;
    }
    return 2;
}

static void Cmd_trychoosesleeptalkmove(void) {
    u32 i, unusableMovesBits = 0, movePosition;

    for (i = 0; i < MAX_MON_MOVES; i++) {
        if (gBattleMoves[gBattleMons[gBattlerAttacker].moves[i]].sleepTalkBanned || IsTwoTurnsMove(gBattleMons[gBattlerAttacker].moves[i])) {
            unusableMovesBits |= 1 << i;
        } else {
            switch (gBattleMoves[gBattleMons[gBattlerAttacker].moves[i]].effect) {
                case EFFECT_PLACEHOLDER:
                case EFFECT_DO_NOTHING:
                case EFFECT_HIT_SWITCH_TARGET:
                case EFFECT_ROAR:
                    unusableMovesBits |= 1 << i;
                    break;
            }
        }
    }

    unusableMovesBits = CheckMoveLimitations(gBattlerAttacker, unusableMovesBits, ~(MOVE_LIMITATION_PP));
    if (unusableMovesBits == 0xF)  // all 4 moves cannot be chosen
    {
        gBattlescriptCurrInstr += 5;
    } else  // at least one move can be chosen
    {
        int reroll = TRUE;
        do {
            movePosition = Random() & (MAX_MON_MOVES - 1);
            if (reroll && gBattleMons[gBattlerAttacker].moves[movePosition] == MOVE_REST) {
                reroll = FALSE;
                movePosition = Random() & (MAX_MON_MOVES - 1);
            }
        } while ((gBitTable[movePosition] & unusableMovesBits));

        gTurnStructs[gBattlerAttacker].sleepTalk = TRUE;
        gQueuedExtraAttackData[++gQueuedAttackCount] = (struct ExtraAttackActionStruct){
            .attacker = gBattlerAttacker,
            .target = GetMoveTarget(gCalledMove, 0),
            .move = gBattleMons[gBattlerAttacker].moves[movePosition],
            .movePos = movePosition,
        };

        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    }
}

static void Cmd_setdestinybond(void) {
    gBattleMons[gBattlerAttacker].status2 |= STATUS2_DESTINY_BOND;
    gBattlescriptCurrInstr++;
}

static void TrySetDestinyBondToHappen(void) {
    u8 sideAttacker = GetBattlerSide(gBattlerAttacker);
    u8 sideTarget = GetBattlerSide(gBattlerTarget);
    if (gBattleMons[gBattlerTarget].status2 & STATUS2_DESTINY_BOND && sideAttacker != sideTarget && !(gHitMarker & HITMARKER_GRUDGE)) {
        gHitMarker |= HITMARKER_DESTINYBOND;
    }
}

static void Cmd_trysetdestinybondtohappen(void) {
    TrySetDestinyBondToHappen();
    gBattlescriptCurrInstr++;
}

static void Cmd_settailwind(void) {
    u8 side = GetBattlerSide(gBattlerAttacker);

    if (!(gSideStatuses[side] & SIDE_STATUS_TAILWIND)) {
        gSideTimers[side].started.tailwind = TRUE;
        gSideStatuses[side] |= SIDE_STATUS_TAILWIND;
        gSideTimers[side].tailwindBattlerId = gBattlerAttacker;
        gSideTimers[side].tailwindTimer = TAILWIND_DURATION;
        gBattlescriptCurrInstr += 5;
    } else {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    }
}

static void Cmd_tryspiteppreduce(void) {
    if (gLastMoves[gBattlerTarget] != 0 && gLastMoves[gBattlerTarget] != 0xFFFF) {
        s32 i;

        for (i = 0; i < MAX_MON_MOVES; i++) {
            if (gLastMoves[gBattlerTarget] == gBattleMons[gBattlerTarget].moves[i]) break;
        }

#if B_CAN_SPITE_FAIL <= GEN_3
        if (i != MAX_MON_MOVES && gBattleMons[gBattlerTarget].pp[i] > 1)
#else
        if (i != MAX_MON_MOVES && gBattleMons[gBattlerTarget].pp[i] != 0)
#endif
        {
#if B_PP_REDUCED_BY_SPITE <= GEN_3
            s32 ppToDeduct = (Random() & 3) + 2;
#else
            s32 ppToDeduct = 4;
#endif

            if (gBattleMons[gBattlerTarget].pp[i] < ppToDeduct) ppToDeduct = gBattleMons[gBattlerTarget].pp[i];

            PREPARE_MOVE_BUFFER(gBattleTextBuff1, gLastMoves[gBattlerTarget])

            ConvertIntToDecimalStringN(gBattleTextBuff2, ppToDeduct, STR_CONV_MODE_LEFT_ALIGN, 1);

            PREPARE_BYTE_NUMBER_BUFFER(gBattleTextBuff2, 1, ppToDeduct)

            gBattleMons[gBattlerTarget].pp[i] -= ppToDeduct;
            gActiveBattler = gBattlerTarget;

            if (!(gVolatileStructs[gActiveBattler].mimickedMoves & gBitTable[i]) && !(gBattleMons[gActiveBattler].status2 & STATUS2_TRANSFORMED)) {
                BtlController_EmitSetMonData(0, REQUEST_PPMOVE1_BATTLE + i, 0, 1, &gBattleMons[gActiveBattler].pp[i]);
                MarkBattlerForControllerExec(gActiveBattler);
            }

            gBattlescriptCurrInstr += 5;

            if (gBattleMons[gBattlerTarget].pp[i] == 0) CancelMultiTurnMoves(gBattlerTarget);
        } else {
            gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
        }
    } else {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    }
}

static void Cmd_healpartystatus(void) {
    u32 zero = 0;
    u8 toHeal = 0;

    if (gCurrentMove == MOVE_HEAL_BELL) {
        struct Pokemon* party;
        s32 i;

        SetActiveMultistringChooser(B_MSG_BELL);

        if (GetBattlerSide(gBattlerAttacker) == B_SIDE_PLAYER)
            party = gPlayerParty;
        else
            party = gEnemyParty;

        if (!IsSoundproof(gBattlerAttacker)) {
            gBattleMons[gBattlerAttacker].status1 = 0;
            gBattleMons[gBattlerAttacker].status2 &= ~(STATUS2_NIGHTMARE);
        } else {
            gBattleCommunication[MULTISTRING_CHOOSER] |= B_MSG_BELL_SOUNDPROOF_ATTACKER;
        }

        gActiveBattler = gBattleScripting.battler = GetBattlerAtPosition(GetBattlerPosition(gBattlerAttacker) ^ BIT_FLANK);

        if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE && !(gAbsentBattlerFlags & gBitTable[gActiveBattler])) {
            if (!IsSoundproof(gActiveBattler)) {
                gBattleMons[gActiveBattler].status1 = 0;
                gBattleMons[gActiveBattler].status2 &= ~(STATUS2_NIGHTMARE);
            } else {
                gBattleCommunication[MULTISTRING_CHOOSER] |= B_MSG_BELL_SOUNDPROOF_PARTNER;
            }
        }

        // Because the above MULTISTRING_CHOOSER are ORd, if both are set then it will be B_MSG_BELL_BOTH_SOUNDPROOF

        for (i = 0; i < PARTY_SIZE; i++) {
            u16 species = GetMonData(&party[i], MON_DATA_SPECIES2);
            u8 abilityNum = GetMonData(&party[i], MON_DATA_ABILITY_NUM);

            if (species != SPECIES_NONE && species != SPECIES_EGG) {
                u16 healMon;

                if (gBattlerPartyIndexes[gBattlerAttacker] == i || gBattlerPartyIndexes[BATTLE_PARTNER(gBattlerAttacker)] == i) {
                    healMon = !IsSoundproof(i);
                } else {
                    u16 ability = GetAbilityBySpecies(species, abilityNum);
                    healMon = ability != ABILITY_SOUNDPROOF && ability != ABILITY_NOISE_CANCEL && ability != ABILITY_PARROTING &&
                              !MonHasInnate(&party[i], ABILITY_SOUNDPROOF, FALSE) && !MonHasInnate(&party[i], ABILITY_NOISE_CANCEL, FALSE) &&
                              !MonHasInnate(&party[i], ABILITY_PARROTING, FALSE);
                }

                if (healMon) toHeal |= (1 << i);
            }
        }
    } else  // Aromatherapy
    {
        SetActiveMultistringChooser(B_MSG_SOOTHING_AROMA);
        toHeal = 0x3F;

        gBattleMons[gBattlerAttacker].status1 = 0;
        gBattleMons[gBattlerAttacker].status2 &= ~(STATUS2_NIGHTMARE);

        gActiveBattler = GetBattlerAtPosition(GetBattlerPosition(gBattlerAttacker) ^ BIT_FLANK);
        if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE && !(gAbsentBattlerFlags & gBitTable[gActiveBattler])) {
            gBattleMons[gActiveBattler].status1 = 0;
            gBattleMons[gActiveBattler].status2 &= ~(STATUS2_NIGHTMARE);
        }
    }

    if (toHeal) {
        gActiveBattler = gBattlerAttacker;
        BtlController_EmitSetMonData(0, REQUEST_STATUS_BATTLE, toHeal, 4, &zero);
        MarkBattlerForControllerExec(gActiveBattler);
    }

    gBattlescriptCurrInstr++;
}

static void Cmd_cursetarget(void) {
    if (gBattleMons[gBattlerTarget].status2 & STATUS2_CURSED) {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    } else {
        gBattleMons[gBattlerTarget].status2 |= STATUS2_CURSED;
        gBattleMoveDamage = gBattleMons[gBattlerAttacker].maxHP / 2;
        if (gBattleMoveDamage == 0) gBattleMoveDamage = 1;

        gBattlescriptCurrInstr += 5;
    }
}

static void Cmd_trysetspikes(void) {
    u8 targetSide = GetBattlerSide(gBattlerAttacker) ^ BIT_SIDE;

    if (gSideTimers[targetSide].spikesAmount == 3) {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    } else {
        gSideStatuses[targetSide] |= SIDE_STATUS_SPIKES;
        gSideTimers[targetSide].spikesAmount++;
        gBattlescriptCurrInstr += 5;
    }
}

static void Cmd_setforesight(void) {
    gBattleMons[gBattlerTarget].status2 |= STATUS2_FORESIGHT;
    gBattlescriptCurrInstr++;
}

static void Cmd_trysetperishsong(void) {
    s32 i;
    s32 affectedCount = 0;

    for (i = 0; i < gBattlersCount; i++) {
        if (gStatuses3[i] & STATUS3_PERISH_SONG || IsSoundproof(i) || BlocksPrankster(gCurrentMove, gBattlerAttacker, i, TRUE)) continue;

        affectedCount++;
        gStatuses3[i] |= STATUS3_PERISH_SONG;
        gVolatileStructs[i].perishSongTimer = 3;
        gVolatileStructs[i].perishSongTimerStartValue = 3;
    }

    if (!affectedCount)
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    else
        gBattlescriptCurrInstr += 5;
}

static void Cmd_handlerollout(void) {
    if (gMoveResultFlags & MOVE_RESULT_NO_EFFECT) {
        gVolatileStructs[gBattlerAttacker].rolloutCounter = 0;
        gBattlescriptCurrInstr = BattleScript_MoveMissedPause;
    } else {
        if (gBattleMons[gBattlerAttacker].status2 & STATUS2_DEFENSE_CURL && !gVolatileStructs[gBattlerAttacker].rolloutCounter)
            gVolatileStructs[gBattlerAttacker].rolloutCounter++;

        if (!gProcessingExtraAttacks && gTurnStructs[gBattlerAttacker].parentalBondOn == gTurnStructs[gBattlerAttacker].parentalBondInitialCount &&
            gVolatileStructs[gBattlerAttacker].rolloutCounter < 3)  // First hit only.
        {
            gVolatileStructs[gBattlerAttacker].rolloutCounter++;
        }

        gBattlescriptCurrInstr++;
    }
}

static void Cmd_jumpifconfusedandstatmaxed(void) {
    int stat = READ_FIRST_8_INC;
    u8* ptr = READ_PTR_INC;
    if (!CanBeConfused(gBattlerTarget) && !CompareStat(gBattlerTarget, stat, MAX_STAT_STAGE, CMP_LESS_THAN))
        gBattlescriptCurrInstr = ptr;  // Fails if we're confused AND stat cannot be raised
}

static void Cmd_handlefurycutter(void) {
    if (gMoveResultFlags & MOVE_RESULT_NO_EFFECT) {
        gVolatileStructs[gBattlerAttacker].furyCutterCounter = 0;
        gBattlescriptCurrInstr = BattleScript_MoveMissedPause;
    } else {
        if (gVolatileStructs[gBattlerAttacker].furyCutterCounter != 5 &&
            (gTurnStructs[gBattlerAttacker].parentalBondInitialCount > 0 &&
             gTurnStructs[gBattlerAttacker].parentalBondOn < gTurnStructs[gBattlerAttacker].parentalBondInitialCount))  // Don't increment counter on first hit
            gVolatileStructs[gBattlerAttacker].furyCutterCounter++;

        gBattlescriptCurrInstr++;
    }
}

static void Cmd_setembargo(void) {
    if (gStatuses3[gBattlerTarget] & STATUS3_EMBARGO) {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    } else {
        gStatuses3[gBattlerTarget] |= STATUS3_EMBARGO;
        gVolatileStructs[gBattlerTarget].embargoTimer = 5;
        gBattlescriptCurrInstr += 5;
    }
}

static void Cmd_presentdamagecalculation(void) {
    u32 rand = Random() & 0xFF;

    /* Don't reroll present effect/power for the second hit of Parental Bond.
     * Not sure if this is the correct behaviour, but bulbapedia states
     * that if present heals the foe, it doesn't strike twice, and if it deals
     * damage, the second strike will always deal damage too. This is a simple way
     * to replicate that effect.
     */
    if (gTurnStructs[gBattlerAttacker].parentalBondOn != 1) {
        if (rand < 102) {
            gBattleStruct->presentBasePower = 40;
        } else if (rand < 178) {
            gBattleStruct->presentBasePower = 80;
        } else if (rand < 204) {
            gBattleStruct->presentBasePower = 120;
        } else {
            gBattleMoveDamage = gBattleMons[gBattlerTarget].maxHP / 4;
            if (gBattleMoveDamage == 0) gBattleMoveDamage = 1;
            gBattleMoveDamage *= -1;
            gBattleStruct->presentBasePower = 0;
        }
    }

    if (gBattleStruct->presentBasePower) {
        gBattlescriptCurrInstr = BattleScript_HitFromCritCalc;
    } else if (gBattleMons[gBattlerTarget].maxHP == gBattleMons[gBattlerTarget].hp) {
        gBattlescriptCurrInstr = BattleScript_AlreadyAtFullHp;
    } else {
        gMoveResultFlags &= ~(MOVE_RESULT_DOESNT_AFFECT_FOE);
        gBattlescriptCurrInstr = BattleScript_PresentHealTarget;
    }
}

static void Cmd_setsafeguard(void) {
    if (gSideStatuses[GET_BATTLER_SIDE(gBattlerAttacker)] & SIDE_STATUS_SAFEGUARD) {
        gMoveResultFlags |= MOVE_RESULT_MISSED;
        SetActiveMultistringChooser(B_MSG_SIDE_STATUS_FAILED);
    } else {
        int side = GET_BATTLER_SIDE(gBattlerAttacker);
        gSideTimers[side].started.safeguard = TRUE;
        gSideStatuses[side] |= SIDE_STATUS_SAFEGUARD;
        gSideTimers[side].safeguardTimer = 5;
        gSideTimers[side].safeguardBattlerId = gBattlerAttacker;
        SetActiveMultistringChooser(B_MSG_SET_SAFEGUARD);
    }

    gBattlescriptCurrInstr++;
}

static void Cmd_magnitudedamagecalculation(void) {
    u8 maxRoll = 100;
    u32 magnitude;

    if (gProcessingExtraAttacks) {
        maxRoll = gQueuedExtraAttackData[0].movePower;
        if (!maxRoll) maxRoll = 100;
    }

    magnitude = Random() % maxRoll;

    if (magnitude < 5) {
        gBattleStruct->magnitudeBasePower = 10;
        magnitude = 4;
    } else if (magnitude < 15) {
        gBattleStruct->magnitudeBasePower = 30;
        magnitude = 5;
    } else if (magnitude < 35) {
        gBattleStruct->magnitudeBasePower = 50;
        magnitude = 6;
    } else if (magnitude < 65) {
        gBattleStruct->magnitudeBasePower = 70;
        magnitude = 7;
    } else if (magnitude < 85) {
        gBattleStruct->magnitudeBasePower = 90;
        magnitude = 8;
    } else if (magnitude < 95) {
        gBattleStruct->magnitudeBasePower = 110;
        magnitude = 9;
    } else {
        gBattleStruct->magnitudeBasePower = 150;
        magnitude = 10;
    }

    PREPARE_BYTE_NUMBER_BUFFER(gBattleTextBuff1, 2, magnitude);
    for (gBattlerTarget = 0; gBattlerTarget < gBattlersCount; gBattlerTarget++) {
        if (gBattlerTarget == gBattlerAttacker) continue;
        if (!(gAbsentBattlerFlags & gBitTable[gBattlerTarget]))  // A valid target was found.
            break;
    }

    gBattlescriptCurrInstr++;
}

static void Cmd_jumpifnopursuitswitchdmg(void) {
    if (gTurnStructs[gBattlerAttacker].multiHitCounter == 1) {
        if (GetBattlerSide(gBattlerAttacker) == B_SIDE_PLAYER)
            gBattlerTarget = GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT);
        else
            gBattlerTarget = GetBattlerAtPosition(B_POSITION_PLAYER_LEFT);
    } else {
        if (GetBattlerSide(gBattlerAttacker) == B_SIDE_PLAYER)
            gBattlerTarget = GetBattlerAtPosition(B_POSITION_OPPONENT_RIGHT);
        else
            gBattlerTarget = GetBattlerAtPosition(B_POSITION_PLAYER_RIGHT);
    }

    if (gChosenActionByBattler[gBattlerTarget] == B_ACTION_USE_MOVE && gBattlerAttacker == gBattleStruct->moveTarget[gBattlerTarget] &&
        !(gBattleMons[gBattlerTarget].status1 & (STATUS1_SLEEP | STATUS1_FREEZE)) && gBattleMons[gBattlerAttacker].hp &&
        !(GetAbilityState(gBattlerTarget, ABILITY_TRUANT) && !IS_MOVE_STATUS(gChosenMoveByBattler[gBattlerTarget])) &&
        gChosenMoveByBattler[gBattlerTarget] == MOVE_PURSUIT) {
        s32 i;

        for (i = 0; i < gBattlersCount; i++) {
            if (gBattlerByTurnOrder[i] == gBattlerTarget) gActionsByTurnOrder[i] = B_ACTION_TRY_FINISH;
        }

        gCurrentMove = MOVE_PURSUIT;
        gCurrMovePos = gChosenMovePos = gBattleStruct->chosenMovePositions[gBattlerTarget];
        gBattlescriptCurrInstr += 5;
        gBattleScripting.animTurn = 1;
        gHitMarker &= ~(HITMARKER_ATTACKSTRING_PRINTED);
    } else {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    }
}

static void Cmd_setsunny(void) {
    if (!TryChangeBattleWeather(gBattlerAttacker, ENUM_WEATHER_SUN, FALSE)) {
        gMoveResultFlags |= MOVE_RESULT_MISSED;
        SetActiveMultistringChooser(B_MSG_WEATHER_FAILED);
    } else {
        SetActiveMultistringChooser(B_MSG_STARTED_SUNLIGHT);
    }

    gBattlescriptCurrInstr++;
}

static void Cmd_maxattackhalvehp(void)  // belly drum
{
    u32 halfHp = gBattleMons[gBattlerAttacker].maxHP / 2;

    if (!(gBattleMons[gBattlerAttacker].maxHP / 2)) halfHp = 1;

    // Belly Drum fails if the user's current HP is less than half its maximum, or if the user's Attack is already at +6 (even if the user has Contrary).
    if (gBattleMons[gBattlerAttacker].hp > halfHp && ChangeStatBuffs(gBattlerAttacker, 12, STAT_ATK, MOVE_EFFECT_AFFECTS_USER, 0)) {
        gBattleMoveDamage = gBattleMons[gBattlerAttacker].maxHP / 2;
        if (gBattleMoveDamage == 0) gBattleMoveDamage = 1;

        gBattlescriptCurrInstr += 5;
    } else {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    }
}

static void Cmd_copyfoestats(void)  // psych up
{
    s32 i;

    for (i = 0; i < NUM_BATTLE_STATS; i++) {
        gBattleMons[gBattlerAttacker].statStages[i] = gBattleMons[gBattlerTarget].statStages[i];
    }

    gBattlescriptCurrInstr += 5;  // Has an unused jump ptr(possibly for a failed attempt) parameter.
}

static void Cmd_rapidspinfree(void) {
    u8 atkSide = GetBattlerSide(gBattlerAttacker);
    gBattlescriptCurrInstr++;

    if (gBattleMons[gBattlerAttacker].status2 & STATUS2_WRAPPED) {
        gBattleScripting.battler = gBattlerTarget;
        gBattleMons[gBattlerAttacker].status2 &= ~(STATUS2_WRAPPED);
        gBattlerTarget = *(gBattleStruct->wrappedBy + gBattlerAttacker);
        PREPARE_MOVE_BUFFER(gBattleTextBuff1, gBattleStruct->wrappedMove[gBattlerAttacker]);
        BattleScriptCall(BattleScript_WrapFree);
    }

    if (gStatuses3[gBattlerAttacker] & STATUS3_LEECHSEED) {
        gStatuses3[gBattlerAttacker] &= ~(STATUS3_LEECHSEED);
        gStatuses3[gBattlerAttacker] &= ~(STATUS3_LEECHSEED_BATTLER);
        BattleScriptCall(BattleScript_LeechSeedFree);
    }

    if (gSideTimers[atkSide].caltrops) {
        gSideTimers[atkSide].caltrops = FALSE;
        BattleScriptCall(BattleScript_CaltropsFree);
    }

    if (gSideTimers[atkSide].hotCoals) {
        gSideTimers[atkSide].hotCoals = FALSE;
        BattleScriptCall(BattleScript_HotCoalsFree);
    }

    if (gSideStatuses[atkSide] & SIDE_STATUS_STICKY_WEB) {
        gSideStatuses[atkSide] &= ~(SIDE_STATUS_STICKY_WEB);
        gSideTimers[atkSide].stickyWebTimer = 0;
        BattleScriptCall(BattleScript_StickyWebFree);
    }

    if (gSideStatuses[atkSide] & SIDE_STATUS_TOXIC_SPIKES) {
        gSideStatuses[atkSide] &= ~(SIDE_STATUS_TOXIC_SPIKES);
        gSideTimers[atkSide].toxicSpikesAmount = 0;
        BattleScriptCall(BattleScript_ToxicSpikesFree);
    }

    if (gSideStatuses[atkSide] & SIDE_STATUS_SPIKES) {
        gSideStatuses[atkSide] &= ~(SIDE_STATUS_SPIKES);
        gSideTimers[atkSide].spikesAmount = 0;
        BattleScriptCall(BattleScript_SpikesFree);
    }

    if (gSideStatuses[atkSide] & SIDE_STATUS_STEALTH_ROCK) {
        switch (gSideTimers[atkSide].stealthRockType) {
            case TYPE_ROCK:
                gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_STEALTH_ROCK_FREE;
                break;
            case TYPE_GRASS:
                gBattleCommunication[MULTISTRING_CHOOSER] = B_MSG_CREEPING_THORNS_FREE;
                break;
        }
        gSideStatuses[atkSide] &= ~(SIDE_STATUS_STEALTH_ROCK);
        gSideTimers[atkSide].stealthRockType = 0;
        BattleScriptCall(BattleScript_StealthRockFree);
    }
}

static void Cmd_setdefensecurlbit(void) {
    gBattleMons[gBattlerAttacker].status2 |= STATUS2_DEFENSE_CURL;
    gBattlescriptCurrInstr++;
}

static void Cmd_recoverbasedonsunlight(void) {
    gBattlerTarget = gBattlerAttacker;
    if (gBattleMons[gBattlerAttacker].status1 & STATUS1_BLEED) {
        gBattleMoveDamage = 0;
        gBattlescriptCurrInstr += 5;
    } else if (gBattleMons[gBattlerAttacker].hp != gBattleMons[gBattlerAttacker].maxHP) {
        if (gCurrentMove == MOVE_SHORE_UP) {
            if (WEATHER_HAS_EFFECT && gBattleWeather & WEATHER_SANDSTORM_ANY)
                gBattleMoveDamage = 2 * gBattleMons[gBattlerAttacker].maxHP / 3;
            else
                gBattleMoveDamage = gBattleMons[gBattlerAttacker].maxHP / 2;
        } else if (gCurrentMove == MOVE_MOONLIGHT && BATTLER_HAS_ABILITY(gBattlerAttacker, ABILITY_MOON_SPIRIT)) {
            gBattleMoveDamage = gBattleMons[gBattlerAttacker].maxHP * 3 / 4;
        } else if (IsBattlerWeatherAffected(gBattlerAttacker, WEATHER_SUN_ANY) || HasChloroplast(gBattlerAttacker)) {
            gBattleMoveDamage = gBattleMons[gBattlerAttacker].maxHP * 2 / 3;
        } else if (IsBattlerWeatherAffected(gBattlerAttacker, WEATHER_RAIN_ANY))
            gBattleMoveDamage = gBattleMons[gBattlerAttacker].maxHP / 4;
        else  // not sunny weather
            gBattleMoveDamage = gBattleMons[gBattlerAttacker].maxHP / 2;

        if (gBattleMoveDamage == 0) gBattleMoveDamage = 1;
        gBattleMoveDamage *= -1;

        gBattlescriptCurrInstr += 5;
    } else {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    }
}

static void Cmd_setstickyweb(void) {
    u8 targetSide = GetBattlerSide(gBattlerTarget);
    if (gSideStatuses[targetSide] & SIDE_STATUS_STICKY_WEB) {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    } else {
        gSideStatuses[targetSide] |= SIDE_STATUS_STICKY_WEB;
        gSideTimers[targetSide].stickyWebTimer = 0;
        gBattleStruct->stickyWebUser = gBattlerAttacker;  // For Mirror Armor
        gBattlescriptCurrInstr += 5;
    }
}

static void Cmd_selectfirstvalidtarget(void) {
    for (gBattlerTarget = 0; gBattlerTarget < gBattlersCount; gBattlerTarget++) {
        if (gBattlerTarget == gBattlerAttacker && !(GetBattlerBattleMoveTargetFlags(gCurrentMove, gBattlerAttacker) & MOVE_TARGET_USER)) continue;
        if (GetAbilityState(gBattlerTarget, ABILITY_COMMANDER) >= COMMANDER_ACTIVE) continue;
        if (IsBattlerAlive(gBattlerTarget)) break;
    }
    gBattlescriptCurrInstr++;
}

static void Cmd_trysetfutureattack(void) {
    if (gWishFutureKnock.futureSightCounter[gBattlerTarget] != 0) {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    } else {
        gSideStatuses[GET_BATTLER_SIDE(gBattlerTarget)] |= SIDE_STATUS_FUTUREATTACK;
        gWishFutureKnock.futureSightMove[gBattlerTarget] = gCurrentMove;
        gWishFutureKnock.futureSightPower[gBattlerTarget] = gBattleMoves[gCurrentMove].power;
        gWishFutureKnock.futureSightAttacker[gBattlerTarget] = gBattlerAttacker;
        gWishFutureKnock.futureSightCounter[gBattlerTarget] = 3;

        if (gCurrentMove == MOVE_DOOM_DESIRE)
            SetActiveMultistringChooser(B_MSG_DOOM_DESIRE);
        else
            SetActiveMultistringChooser(B_MSG_FUTURE_SIGHT);

        gBattlescriptCurrInstr += 5;
    }
}

static void Cmd_trydobeatup(void) {
    struct Pokemon* party;

    if (GetBattlerSide(gBattlerAttacker) == B_SIDE_PLAYER)
        party = gPlayerParty;
    else
        party = gEnemyParty;

    if (gBattleMons[gBattlerTarget].hp == 0) {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    } else {
        u8 beforeLoop = gBattleCommunication[0];
        for (; gBattleCommunication[0] < PARTY_SIZE; gBattleCommunication[0]++) {
            if (GetMonData(&party[gBattleCommunication[0]], MON_DATA_HP) && GetMonData(&party[gBattleCommunication[0]], MON_DATA_SPECIES2) &&
                GetMonData(&party[gBattleCommunication[0]], MON_DATA_SPECIES2) != SPECIES_EGG && !GetMonData(&party[gBattleCommunication[0]], MON_DATA_STATUS))
                break;
        }
        if (gBattleCommunication[0] < PARTY_SIZE) {
            PREPARE_MON_NICK_WITH_PREFIX_BUFFER(gBattleTextBuff1, gBattlerAttacker, gBattleCommunication[0])

            gBattlescriptCurrInstr += 9;

            gBattleMoveDamage = gBaseStats[GetMonData(&party[gBattleCommunication[0]], MON_DATA_SPECIES)].baseAttack;
            gBattleMoveDamage *= gBattleMoves[gCurrentMove].power;
            gBattleMoveDamage *= (GetMonData(&party[gBattleCommunication[0]], MON_DATA_LEVEL) * 2 / 5 + 2);
            gBattleMoveDamage /= gBaseStats[gBattleMons[gBattlerTarget].species].baseDefense;
            gBattleMoveDamage = (gBattleMoveDamage / 50) + 2;
            if (gRoundStructs[gBattlerAttacker].helpingHand) gBattleMoveDamage = gBattleMoveDamage * 15 / 10;

            gBattleCommunication[0]++;
        } else if (beforeLoop != 0)
            gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
        else
            gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 5);
    }
}

static void Cmd_setsemiinvulnerablebit(void) {
    switch (gCurrentMove) {
        case MOVE_SKY_DROP:
        case MOVE_SEISMIC_TOSS:
        case MOVE_FLY:
        case MOVE_BOUNCE:
            gStatuses3[gBattlerAttacker] |= STATUS3_ON_AIR;
            break;
        case MOVE_DIG:
            gStatuses3[gBattlerAttacker] |= STATUS3_UNDERGROUND;
            break;
        case MOVE_TOXIC_PLUNGE:
        case MOVE_DIVE:
            gStatuses3[gBattlerAttacker] |= STATUS3_UNDERWATER;
            break;
        case MOVE_PHANTOM_FORCE:
        case MOVE_SHADOW_FORCE:
        case MOVE_CHEAP_SHOT:
        case MOVE_READY_OR_NOT:
            gStatuses3[gBattlerAttacker] |= STATUS3_PHANTOM_FORCE;
            break;
    }

    gBattlescriptCurrInstr++;
}

static void Cmd_clearsemiinvulnerablebit(void) {
    gStatuses3[gBattlerAttacker] &= ~STATUS3_SEMI_INVULNERABLE;
    gBattlescriptCurrInstr++;
}

static void Cmd_setminimize(void) {
    if (gHitMarker & HITMARKER_OBEYS) gStatuses3[gBattlerAttacker] |= STATUS3_MINIMIZED;

    gBattlescriptCurrInstr++;
}

static void Cmd_sethail(void) {
    if (!TryChangeBattleWeather(gBattlerAttacker, ENUM_WEATHER_HAIL, FALSE)) {
        gMoveResultFlags |= MOVE_RESULT_MISSED;
        SetActiveMultistringChooser(B_MSG_WEATHER_FAILED);
    } else {
        SetActiveMultistringChooser(B_MSG_STARTED_HAIL);
    }

    gBattlescriptCurrInstr++;
}

static void Cmd_jumpifattackandspecialattackcannotfall(void)  // memento
{
#if B_MEMENTO_FAIL == GEN_3
    if (gBattleMons[gBattlerTarget].statStages[STAT_ATK] == MIN_STAT_STAGE && gBattleMons[gBattlerTarget].statStages[STAT_SPATK] == MIN_STAT_STAGE &&
        gBattleCommunication[MISS_TYPE] != B_MSG_PROTECTED)
#else
    if (gBattleCommunication[MISS_TYPE] == B_MSG_PROTECTED || gStatuses3[gBattlerTarget] & STATUS3_SEMI_INVULNERABLE ||
        IsBattlerProtected(gBattlerTarget, gCurrentMove) || DoesSubstituteBlockMove(gBattlerAttacker, gBattlerTarget, gCurrentMove))
#endif
    {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    } else {
        gActiveBattler = gBattlerAttacker;
        gBattleMoveDamage = gBattleMons[gActiveBattler].hp;
        BtlController_EmitHealthBarUpdate(0, INSTANT_HP_BAR_DROP);
        MarkBattlerForControllerExec(gActiveBattler);
        gBattlescriptCurrInstr += 5;
    }
}

static void Cmd_setforcedtarget(void)  // follow me
{
    gSideTimers[GetBattlerSide(gBattlerAttacker)].followmeTimer = 1;
    gSideTimers[GetBattlerSide(gBattlerAttacker)].followmeTarget = gBattlerAttacker;
    gSideTimers[GetBattlerSide(gBattlerAttacker)].followmePowder = TestMoveFlags(gCurrentMove, FLAG_POWDER);
    gBattlescriptCurrInstr++;
}

static void Cmd_setcharge(void) {
    gStatuses3[gBattlerAttacker] |= STATUS3_CHARGED_UP;
    gBattlescriptCurrInstr++;
}

static void Cmd_callterrainattack(void)  // nature power
{
    gHitMarker &= ~(HITMARKER_ATTACKSTRING_PRINTED);
    gCurrentMove = GetNaturePowerMove();
    gBattlerTarget = GetMoveTarget(gCurrentMove, 0);
    BattleScriptPush(gBattleScriptsForMoveEffects[gBattleMoves[gCurrentMove].effect]);
    gBattlescriptCurrInstr++;
}

u16 GetNaturePowerMove(void) {
    if (GetCurrentTerrain() == STATUS_FIELD_MISTY_TERRAIN)
        return MOVE_MOONBLAST;
    else if (GetCurrentTerrain() == STATUS_FIELD_ELECTRIC_TERRAIN)
        return MOVE_THUNDERBOLT;
    else if (GetCurrentTerrain() == STATUS_FIELD_GRASSY_TERRAIN)
        return MOVE_ENERGY_BALL;
    else if (GetCurrentTerrain() == STATUS_FIELD_PSYCHIC_TERRAIN)
        return MOVE_PSYCHIC;
    else if (sNaturePowerMoves[gBattleTerrain] == MOVE_NONE)
        return MOVE_TRI_ATTACK;
    return sNaturePowerMoves[gBattleTerrain];
}

static void Cmd_cureifburnedparalysedorpoisoned(void)  // refresh
{
    if (gBattleMons[gBattlerAttacker].status1 &
        (STATUS1_POISON | STATUS1_BURN | STATUS1_PARALYSIS | STATUS1_TOXIC_POISON | STATUS1_FROSTBITE | STATUS1_BLEED)) {
        gBattleMons[gBattlerAttacker].status1 = 0;
        gBattlescriptCurrInstr += 5;
        gActiveBattler = gBattlerAttacker;
        BtlController_EmitSetMonData(0, REQUEST_STATUS_BATTLE, 0, 4, &gBattleMons[gActiveBattler].status1);
        MarkBattlerForControllerExec(gActiveBattler);
    } else {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    }
}

static void Cmd_settorment(void) {
    if (gBattleMons[gBattlerTarget].status2 & STATUS2_TORMENT) {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    } else {
        gBattleMons[gBattlerTarget].status2 |= STATUS2_TORMENT;
        gBattlescriptCurrInstr += 5;
    }
}

static void Cmd_jumpifnodamage(void) {
    if (gRoundStructs[gBattlerAttacker].physicalDmg || gRoundStructs[gBattlerAttacker].specialDmg)
        gBattlescriptCurrInstr += 5;
    else
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
}

static void Cmd_settaunt(void) {
    if (BattlerHasAbility(gBattlerTarget, ABILITY_OBLIVIOUS, TRUE)) {
        gBattleScripting.abilityPopupOverwrite = ABILITY_OBLIVIOUS;
        gBattlescriptCurrInstr = BattleScript_NotAffectedAbilityPopUp;
    } else if (gVolatileStructs[gBattlerTarget].tauntTimer == 0) {
#if B_TAUNT_TURNS >= GEN_5
        u8 turns = 4;
        if (GetBattlerTurnOrderNum(gBattlerTarget) > GetBattlerTurnOrderNum(gBattlerAttacker))
            turns--;  // If the target hasn't yet moved this turn, Taunt lasts for only three turns (source: Bulbapedia)
#elif B_TAUNT_TURNS == GEN_4
        u8 turns = (Random() & 2) + 3;
#else
        u8 turns = 2;
#endif

        gVolatileStructs[gBattlerTarget].tauntTimer = gVolatileStructs[gBattlerTarget].tauntTimer2 = turns;
        gBattlescriptCurrInstr += 5;
    } else {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    }
}

static void Cmd_trysethelpinghand(void) {
    gBattlerTarget = GetBattlerAtPosition(GetBattlerPosition(gBattlerAttacker) ^ BIT_FLANK);

    if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE && !(gAbsentBattlerFlags & gBitTable[gBattlerTarget]) && !gRoundStructs[gBattlerAttacker].helpingHand &&
        !gRoundStructs[gBattlerTarget].helpingHand) {
        gRoundStructs[gBattlerTarget].helpingHand = TRUE;
        gBattlescriptCurrInstr += 5;
    } else {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    }
}

static void Cmd_tryswapitems(void)  // trick
{
    // opponent can't swap items with player in regular battles
    if (gBattleTypeFlags & BATTLE_TYPE_TRAINER_HILL ||
        (GetBattlerSide(gBattlerAttacker) == B_SIDE_OPPONENT &&
         !(gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_EREADER_TRAINER | BATTLE_TYPE_FRONTIER | BATTLE_TYPE_SECRET_BASE | BATTLE_TYPE_RECORDED_LINK
#if B_TRAINERS_KNOCK_OFF_ITEMS
                               | BATTLE_TYPE_TRAINER
#endif
                               )))) {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    } else {
        u8 sideAttacker = GetBattlerSide(gBattlerAttacker);
        u8 sideTarget = GetBattlerSide(gBattlerTarget);

        // can't swap if two pokemon have the same item
        // or if either of them is an enigma berry or a mail
        if (gBattleMons[gBattlerAttacker].item == gBattleMons[gBattlerTarget].item ||
            !CanBattlerGetOrLoseItem(gBattlerAttacker, gBattleMons[gBattlerAttacker].item) ||
            !CanBattlerGetOrLoseItem(gBattlerAttacker, gBattleMons[gBattlerTarget].item) ||
            !CanBattlerGetOrLoseItem(gBattlerTarget, gBattleMons[gBattlerTarget].item) ||
            !CanBattlerGetOrLoseItem(gBattlerTarget, gBattleMons[gBattlerAttacker].item)) {
            gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
        }
        // check if ability prevents swapping
        else if ((gBattleScripting.abilityPopupOverwrite = IsStickyHold(gBattlerTarget))) {
            gBattlescriptCurrInstr = BattleScript_MoveEnd;
            BattleScriptCall(BattleScript_StickyHoldActivates);
        }
        // took a while, but all checks passed and items can be safely swapped
        else {
            gLastUsedItem = UpdateBattlerItem(gBattlerAttacker, gBattleMons[gBattlerTarget].item);
            gBattleStruct->changedItems[gBattlerAttacker] = UpdateBattlerItem(gBattlerTarget, gLastUsedItem);

            gBattlescriptCurrInstr += 5;

            PREPARE_ITEM_BUFFER(gBattleTextBuff1, gBattleStruct->changedItems[gBattlerAttacker])
            PREPARE_ITEM_BUFFER(gBattleTextBuff2, gLastUsedItem)

            if (!(sideAttacker == sideTarget && IsPartnerMonFromSameTrainer(gBattlerAttacker))) {
                // if targeting your own side and you aren't in a multi battle, don't save items as stolen
                if (GetBattlerSide(gBattlerAttacker) == B_SIDE_PLAYER) TrySaveExchangedItem(gBattlerAttacker, gLastUsedItem);
                if (GetBattlerSide(gBattlerTarget) == B_SIDE_PLAYER) TrySaveExchangedItem(gBattlerTarget, gBattleStruct->changedItems[gBattlerAttacker]);
            }

            if (gLastUsedItem != 0 && gBattleStruct->changedItems[gBattlerAttacker] != 0)
                SetActiveMultistringChooser(B_MSG_ITEM_SWAP_BOTH);  // attacker's item -> <- target's item
            else if (gLastUsedItem == 0 && gBattleStruct->changedItems[gBattlerAttacker] != 0)
                SetActiveMultistringChooser(B_MSG_ITEM_SWAP_TAKEN);  // nothing -> <- target's item
            else
                SetActiveMultistringChooser(B_MSG_ITEM_SWAP_GIVEN);  // attacker's item -> <- nothing
        }
    }
}

static void Cmd_trycopyability(void)  // role play
{
    u16 defAbility = GetBattlerAbility(gBattlerTarget);

    if (HasAbilityIgnoringSuppression(gBattlerAttacker, defAbility) || defAbility == ABILITY_NONE ||
        IsRolePlayBannedAbilityAtk(GetBattlerAbility(gBattlerAttacker)) || IsRolePlayBannedAbility(defAbility) ||
        DoesBattlerHaveAbilityShield(gBattlerAttacker)) {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    } else {
        UpdateAbilityStateIndicesForNewAbility(gBattlerAttacker, defAbility);
        ReplaceAbility(gBattlerAttacker, defAbility);
        SetActiveAbilityPopupOverride(defAbility);
        gBattlescriptCurrInstr += 5;
    }
}

static void Cmd_trywish(void) {
    switch (gBattlescriptCurrInstr[1]) {
        case 0:  // use wish
            if (gWishFutureKnock.wishCounter[gBattlerAttacker] == 0) {
                gWishFutureKnock.wishCounter[gBattlerAttacker] = 2;
                gWishFutureKnock.wishPartyId[gBattlerAttacker] = gBattlerPartyIndexes[gBattlerAttacker];
                gBattlescriptCurrInstr += 6;
            } else {
                gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 2);
            }
            break;
        case 1:  // heal effect
            if (gBattleMons[gBattlerTarget].status1 & STATUS1_BLEED) {
                gBattleMoveDamage = 0;
                gBattlescriptCurrInstr += 6;
                return;
            }

#if B_WISH_HP_SOURCE >= GEN_5
            if (GetBattlerSide(gBattlerTarget) == B_SIDE_PLAYER)
                gBattleMoveDamage = max(1, GetMonData(&gPlayerParty[gWishFutureKnock.wishPartyId[gBattlerTarget]], MON_DATA_MAX_HP) / 2);
            else
                gBattleMoveDamage = max(1, GetMonData(&gEnemyParty[gWishFutureKnock.wishPartyId[gBattlerTarget]], MON_DATA_MAX_HP) / 2);
#else
            gBattleMoveDamage = max(1, gBattleMons[gBattlerTarget].maxHP / 2);
#endif

            gBattleMoveDamage *= -1;
            if (gBattleMons[gBattlerTarget].hp == gBattleMons[gBattlerTarget].maxHP)
                gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 2);
            else
                gBattlescriptCurrInstr += 6;

            break;
    }
}

static void Cmd_settoxicspikes(void) {
    u8 targetSide = GetBattlerSide(gBattlerTarget);
    if (gSideTimers[targetSide].toxicSpikesAmount >= 2) {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    } else {
        gSideTimers[targetSide].toxicSpikesAmount++;
        gSideStatuses[targetSide] |= SIDE_STATUS_TOXIC_SPIKES;
        gBattlescriptCurrInstr += 5;
    }
}

static void Cmd_setgastroacid(void) {
    if (gStatuses3[gBattlerTarget] & STATUS3_GASTRO_ACID) {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    } else {
        gStatuses3[gBattlerTarget] |= STATUS3_GASTRO_ACID;
        gBattlescriptCurrInstr += 5;
    }
}

static void Cmd_setyawn(void) {
    if (gStatuses3[gBattlerTarget] & STATUS3_YAWN || gBattleMons[gBattlerTarget].status1 & STATUS1_ANY) {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    } else if (IsBattlerTerrainAffected(gBattlerTarget, STATUS_FIELD_ELECTRIC_TERRAIN)) {
        // When Yawn is used while Electric Terrain is set and drowsiness is set from Yawn being used against target in the previous turn:
        // "But it failed" will display first.
        gBattlescriptCurrInstr = BattleScript_ElectricTerrainPrevents;
    } else if (IsBattlerTerrainAffected(gBattlerTarget, STATUS_FIELD_MISTY_TERRAIN)) {
        // When Yawn is used while Misty Terrain is set and drowsiness is set from Yawn being used against target in the previous turn:
        // "But it failed" will display first.
        gBattlescriptCurrInstr = BattleScript_MistyTerrainPrevents;
    } else {
        gStatuses3[gBattlerTarget] |= STATUS3_YAWN_TURN(2);
        gBattlescriptCurrInstr += 5;
    }
}

static void Cmd_setdamagetohealthdifference(void) {
    if (gBattleMons[gBattlerTarget].hp <= gBattleMons[gBattlerAttacker].hp) {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    } else {
        gBattleMoveDamage = gBattleMons[gBattlerTarget].hp - gBattleMons[gBattlerAttacker].hp;
        gBattlescriptCurrInstr += 5;
    }
}

static void HandleRoomMove(u32 statusFlag, u8* timer, u8 stringId, u8 duration) {
    if (gFieldStatuses & statusFlag) {
        gFieldStatuses &= ~(statusFlag);
        *timer = 0;
        SetActiveMultistringChooser(stringId + 1);
    } else {
        gFieldStatuses |= statusFlag;
        *timer = duration;
        SetActiveMultistringChooser(stringId);
    }
}

static void Cmd_setroom(void) {
    const u8* ptr = READ_FIRST_PTR_INC;
    switch (gBattleMoves[gCurrentMove].effect) {
        case EFFECT_TRICK_ROOM:
            // Permanent
            if ((gFieldStatuses & STATUS_FIELD_TRICK_ROOM) && gFieldTimers.trickRoomTimer > 10) {
                gBattlescriptCurrInstr = ptr;
            } else {
                HandleRoomMove(STATUS_FIELD_TRICK_ROOM, &gFieldTimers.trickRoomTimer, B_MSG_TRICKROOMSTARTS, TRICK_ROOM_DURATION);
                gFieldTimers.started.trickRoom = TRUE;
            }
            break;
        case EFFECT_WONDER_ROOM:
            // Permanent
            if ((gFieldStatuses & STATUS_FIELD_WONDER_ROOM) && gFieldTimers.wonderRoomTimer > 10) {
                gBattlescriptCurrInstr = ptr;
            } else {
                HandleRoomMove(STATUS_FIELD_WONDER_ROOM, &gFieldTimers.wonderRoomTimer, B_MSG_WONDERROOMSTARTS, WONDER_ROOM_DURATION);
                gFieldTimers.started.wonderRoom = TRUE;
            }
            break;
        case EFFECT_MAGIC_ROOM:
            // Permanent
            if ((gFieldStatuses & STATUS_FIELD_MAGIC_ROOM) && gFieldTimers.magicRoomTimer > 10) {
                gBattlescriptCurrInstr = ptr;
            } else {
                HandleRoomMove(STATUS_FIELD_MAGIC_ROOM, &gFieldTimers.magicRoomTimer, B_MSG_MAGICROOMSTARTS, MAGIC_ROOM_DURATION);
                gFieldTimers.started.magicRoom = TRUE;
            }
            break;
        case EFFECT_INVERSE_ROOM:
            // Permanent
            if ((gFieldStatuses & STATUS_FIELD_INVERSE_ROOM) && gFieldTimers.inverseRoomTimer > 10) {
                gBattlescriptCurrInstr = ptr;
            } else {
                HandleRoomMove(STATUS_FIELD_INVERSE_ROOM, &gFieldTimers.inverseRoomTimer, B_MSG_INVERSEROOMSTARTS, INVERSE_ROOM_DURATION);
                gFieldTimers.started.inverseRoom = TRUE;
            }
            break;
        default:
            SetActiveMultistringChooser(B_MSG_ROOMEMPTYSTRING);
            break;
    }
}

static void Cmd_tryswapabilities(void)  // skill swap
{
    if (IsRolePlayBannedAbility(GetBattlerAbility(gBattlerAttacker)) || IsRolePlayBannedAbility(GetBattlerAbility(gBattlerTarget)) ||
        HasAbilityIgnoringSuppression(gBattlerAttacker, GetBattlerAbility(gBattlerTarget)) ||
        HasAbilityIgnoringSuppression(gBattlerTarget, GetBattlerAbility(gBattlerAttacker)) || DoesBattlerHaveAbilityShield(gBattlerAttacker) ||
        DoesBattlerHaveAbilityShield(gBattlerTarget)) {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
        return;
    }

    if (gMoveResultFlags & MOVE_RESULT_NO_EFFECT) {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    } else {
        u16 abilityAtk = GetBattlerAbility(gBattlerAttacker);

        UpdateAbilityStateIndicesForNewAbility(gBattlerAttacker, GetBattlerAbility(gBattlerTarget));
        UpdateAbilityStateIndicesForNewAbility(gBattlerTarget, abilityAtk);

        ReplaceAbility(gBattlerAttacker, GetBattlerAbility(gBattlerTarget));
        ReplaceAbility(gBattlerTarget, abilityAtk);

        gBattlescriptCurrInstr += 5;
    }
}

static void Cmd_tryimprison(void) {
    if ((gStatuses3[gBattlerAttacker] & STATUS3_IMPRISONED_OTHERS)) {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    } else {
        u8 battlerId, sideAttacker;

        sideAttacker = GetBattlerSide(gBattlerAttacker);
        for (battlerId = 0; battlerId < gBattlersCount; battlerId++) {
            if (sideAttacker != GetBattlerSide(battlerId)) {
                s32 attackerMoveId;
                for (attackerMoveId = 0; attackerMoveId < MAX_MON_MOVES; attackerMoveId++) {
                    s32 i;
                    for (i = 0; i < MAX_MON_MOVES; i++) {
                        if (gBattleMons[gBattlerAttacker].moves[attackerMoveId] == gBattleMons[battlerId].moves[i] &&
                            gBattleMons[gBattlerAttacker].moves[attackerMoveId] != MOVE_NONE)
                            break;
                    }
                    if (i != MAX_MON_MOVES) break;
                }
                if (attackerMoveId != MAX_MON_MOVES) {
                    gStatuses3[gBattlerAttacker] |= STATUS3_IMPRISONED_OTHERS;
                    gBattlescriptCurrInstr += 5;
                    break;
                }
            }
        }
        if (battlerId == gBattlersCount)  // In Generation 3 games, Imprison fails if the user doesn't share any moves with any of the foes.
            gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    }
}

static void Cmd_setstealthrock(void) {
    u8 targetSide = GetBattlerSide(gBattlerTarget);
    if (gSideStatuses[targetSide] & SIDE_STATUS_STEALTH_ROCK) {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    } else {
        gSideStatuses[targetSide] |= SIDE_STATUS_STEALTH_ROCK;
        gSideTimers[targetSide].stealthRockType = gBattleMoves[gCurrentMove].type;
        SetActiveMultistringChooser(gBattleMoves[gCurrentMove].type == TYPE_GRASS ? B_MSG_CREEPING_THORNS_SET : B_MSG_STEALTH_ROCK_SET);
        gBattlescriptCurrInstr += 5;
    }
}

static void Cmd_setuserstatus3(void) {
    u32 flags = T1_READ_32(gBattlescriptCurrInstr + 1);

    if (gStatuses3[gBattlerAttacker] & flags) {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 5);
    } else {
        gStatuses3[gBattlerAttacker] |= flags;
        if (flags & STATUS3_MAGNET_RISE) gVolatileStructs[gBattlerAttacker].magnetRiseTimer = 5;
        if (flags & STATUS3_LASER_FOCUS) gVolatileStructs[gBattlerAttacker].laserFocusTimer = 2;
        gBattlescriptCurrInstr += 9;
    }
}

static void Cmd_assistattackselect(void) {
    s32 chooseableMovesNo = 0;
    struct Pokemon* party;
    s32 monId, moveId;
    u16* movesArray = gBattleStruct->assistPossibleMoves;

    if (GET_BATTLER_SIDE(gBattlerAttacker) != B_SIDE_PLAYER)
        party = gEnemyParty;
    else
        party = gPlayerParty;

    for (monId = 0; monId < PARTY_SIZE; monId++) {
        if (monId == gBattlerPartyIndexes[gBattlerAttacker]) continue;
        if (GetMonData(&party[monId], MON_DATA_SPECIES2) == SPECIES_NONE) continue;
        if (GetMonData(&party[monId], MON_DATA_SPECIES2) == SPECIES_EGG) continue;

        for (moveId = 0; moveId < MAX_MON_MOVES; moveId++) {
            u16 move = GetMonData(&party[monId], MON_DATA_MOVE1 + moveId);

            switch (gBattleMoves[move].effect) {
                case EFFECT_PLACEHOLDER:
                case EFFECT_PROTECT:
                case EFFECT_DO_NOTHING:
                case EFFECT_HIT_SWITCH_TARGET:
                case EFFECT_ROAR:
                case EFFECT_FOLLOW_ME:
                case EFFECT_TRICK:
                    continue;
            }

            if (gBattleMoves[move].copycatBanned) continue;

            if (IsTwoTurnsMove(move)) continue;

            movesArray[chooseableMovesNo] = move;
            chooseableMovesNo++;
        }
    }
    if (chooseableMovesNo) {
        gHitMarker &= ~(HITMARKER_ATTACKSTRING_PRINTED);
        gCalledMove = movesArray[((Random() & 0xFF) * chooseableMovesNo) >> 8];
        gBattlerTarget = GetMoveTarget(gCalledMove, 0);
        gBattlescriptCurrInstr += 5;
    } else {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    }
}

static void Cmd_trysetmagiccoat(void) {
    gBattlerTarget = gBattlerAttacker;
    if (gCurrentTurnActionNumber == gBattlersCount - 1)  // moves last turn
    {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    } else {
        gRoundStructs[gBattlerAttacker].bounceMove = TRUE;
        gBattlescriptCurrInstr += 5;
    }
}

static void Cmd_trysetsnatch(void)  // snatch
{
    if (gCurrentTurnActionNumber == gBattlersCount - 1)  // moves last turn
    {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    } else {
        gRoundStructs[gBattlerAttacker].stealMove = TRUE;
        gBattlescriptCurrInstr += 5;
    }
}

static void Cmd_trygetintimidatetarget(void) {
    u8 side;

    gBattleScripting.battler = gBattleStruct->intimidateBattler;
    side = GetBattlerSide(gBattleScripting.battler);

    PREPARE_ABILITY_BUFFER(gBattleTextBuff1, gBattleMons[gBattleScripting.battler].abilities[0])

    for (; gBattlerTarget < gBattlersCount; gBattlerTarget++) {
        if (GetBattlerSide(gBattlerTarget) == side) continue;
        if (!(gAbsentBattlerFlags & gBitTable[gBattlerTarget])) break;
    }

    if (gBattlerTarget >= gBattlersCount)
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    else
        gBattlescriptCurrInstr += 5;
}

void DoRegenerator() {
    if (!IsBattlerAlive(gActiveBattler)) return;
    gBattleMoveDamage = gBattleMons[gActiveBattler].maxHP / 3;
    gBattleMoveDamage += gBattleMons[gActiveBattler].hp;
    if (gBattleMoveDamage > gBattleMons[gActiveBattler].maxHP) gBattleMoveDamage = gBattleMons[gActiveBattler].maxHP;
    BtlController_EmitSetMonData(0, REQUEST_HP_BATTLE, gBitTable[*(gBattleStruct->battlerPartyIndexes + gActiveBattler)], 2, &gBattleMoveDamage);
    MarkBattlerForControllerExec(gActiveBattler);
}

static void Cmd_switchoutabilities(void) {
    gActiveBattler = GetBattlerForBattleScript(READ_FIRST_8_INC);
    gRoundStructs[gActiveBattler].protectedThisTurn = FALSE;

    ON_ABILITY(
        gActiveBattler, FALSE, gAbilities[ability].onExit, if (gAbilities[ability].onExit(ability, gActiveBattler)) {
            gBattlerAbility = gActiveBattler;
            BattleScriptCall(BattleScript_AbilityPopUp);
        })

    ReadActiveScriptInitialStackState();
}

static void Cmd_jumpifhasnohp(void) {
    gActiveBattler = GetBattlerForBattleScript(gBattlescriptCurrInstr[1]);

    if (gBattleMons[gActiveBattler].hp == 0)
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 2);
    else
        gBattlescriptCurrInstr += 6;
}

static void Cmd_getsecretpowereffect(void) {
    gBattleScripting.moveEffect = GetSecretPowerMoveEffect();
    gBattlescriptCurrInstr++;
}

u16 GetSecretPowerMoveEffect(void) {
    u16 moveEffect;
    u32 fieldTerrain = gFieldStatuses & STATUS_FIELD_TERRAIN_ANY;
    if (fieldTerrain) {
        switch (fieldTerrain) {
            case STATUS_FIELD_MISTY_TERRAIN:
                moveEffect = MOVE_EFFECT_SP_ATK_MINUS_1;
                break;
            case STATUS_FIELD_GRASSY_TERRAIN:
                moveEffect = MOVE_EFFECT_SLEEP;
                break;
            case STATUS_FIELD_ELECTRIC_TERRAIN:
                moveEffect = MOVE_EFFECT_PARALYSIS;
                break;
            case STATUS_FIELD_PSYCHIC_TERRAIN:
                moveEffect = MOVE_EFFECT_SPD_MINUS_1;
                break;
            default:
                moveEffect = MOVE_EFFECT_PARALYSIS;
                break;
        }
    } else {
        switch (gBattleTerrain) {
            case BATTLE_TERRAIN_GRASS:
#if B_SECRET_POWER_EFFECT >= GEN_4
                moveEffect = MOVE_EFFECT_SLEEP;
#else
                moveEffect = MOVE_EFFECT_POISON;
#endif
                break;
            case BATTLE_TERRAIN_LONG_GRASS:
                moveEffect = MOVE_EFFECT_SLEEP;
                break;
            case BATTLE_TERRAIN_SAND:
                moveEffect = MOVE_EFFECT_ACC_MINUS_1;
                break;
            case BATTLE_TERRAIN_UNDERWATER:
#if B_SECRET_POWER_EFFECT >= GEN_6
                moveEffect = MOVE_EFFECT_ATK_MINUS_1;
#else
                moveEffect = MOVE_EFFECT_DEF_MINUS_1;
#endif
                break;
            case BATTLE_TERRAIN_WATER:
                moveEffect = MOVE_EFFECT_ATK_MINUS_1;
                break;
            case BATTLE_TERRAIN_POND:
#if B_SECRET_POWER_EFFECT >= GEN_4
                moveEffect = MOVE_EFFECT_ATK_MINUS_1;
#else
                moveEffect = MOVE_EFFECT_SPD_MINUS_1;
#endif
                break;
            case BATTLE_TERRAIN_MOUNTAIN:
#if B_SECRET_POWER_EFFECT >= GEN_5
                moveEffect = MOVE_EFFECT_ACC_MINUS_1;
#elif B_SECRET_POWER_EFFECT == GEN_4
                moveEffect = MOVE_EFFECT_FLINCH;
#else
                moveEffect = MOVE_EFFECT_CONFUSION;
#endif
                break;
            case BATTLE_TERRAIN_CAVE:
            case BATTLE_TERRAIN_BURIAL_GROUND:
            case BATTLE_TERRAIN_SPACE:
                moveEffect = MOVE_EFFECT_FLINCH;
                break;
            case BATTLE_TERRAIN_SOARING:
            case BATTLE_TERRAIN_SKY_PILLAR:
            case BATTLE_TERRAIN_MARSH:
            case BATTLE_TERRAIN_SWAMP:
                moveEffect = MOVE_EFFECT_SPD_MINUS_1;
                break;
            case BATTLE_TERRAIN_PUDDLE:
#if B_SECRET_POWER_EFFECT >= GEN_5
                moveEffect = MOVE_EFFECT_SPD_MINUS_1;
#else
                moveEffect = MOVE_EFFECT_ACC_MINUS_1;
#endif
                break;
            case BATTLE_TERRAIN_SNOW:
            case BATTLE_TERRAIN_ICE:
#if B_USE_FROSTBITE == TRUE
                moveEffect = MOVE_EFFECT_FROSTBITE;
#else
                moveEffect = MOVE_EFFECT_FREEZE;
#endif
                break;
            case BATTLE_TERRAIN_VOLCANO:
                moveEffect = MOVE_EFFECT_BURN;
                break;
            case BATTLE_TERRAIN_ULTRA_SPACE:
                moveEffect = MOVE_EFFECT_DEF_MINUS_1;
                break;
            default:
                moveEffect = MOVE_EFFECT_PARALYSIS;
                break;
        }
    }
    return moveEffect;
}

static void Cmd_pickup(void) {
    s32 i;
    u16 species, heldItem;
    u16 ability;
    u8 lvlDivBy10 = 0;

    if (InBattlePike()) {
    } else if (InBattlePyramid()) {
        for (i = 0; i < PARTY_SIZE; i++) {
            species = GetMonData(&gPlayerParty[i], MON_DATA_SPECIES2);
            heldItem = GetMonData(&gPlayerParty[i], MON_DATA_HELD_ITEM);

            if (GetMonData(&gPlayerParty[i], MON_DATA_ABILITY_NUM))
                ability = gBaseStats[species].abilities[1];
            else
                ability = gBaseStats[species].abilities[0];

            if (ability == ABILITY_PICKUP && species != 0 && species != SPECIES_EGG && heldItem == ITEM_NONE && (Random() % 10) == 0) {
                heldItem = GetBattlePyramidPickupItemId();
                SetMonData(&gPlayerParty[i], MON_DATA_HELD_ITEM, &heldItem);
            }
#if (defined ITEM_HONEY)
            else if (ability == ABILITY_HONEY_GATHER && species != 0 && species != SPECIES_EGG && heldItem == ITEM_NONE) {
                if ((lvlDivBy10 + 1) * 5 > Random() % 100) {
                    heldItem = ITEM_HONEY;
                    SetMonData(&gPlayerParty[i], MON_DATA_HELD_ITEM, &heldItem);
                }
            }
#endif
        }
    } else {
        for (i = 0; i < PARTY_SIZE; i++) {
            species = GetMonData(&gPlayerParty[i], MON_DATA_SPECIES2);
            heldItem = GetMonData(&gPlayerParty[i], MON_DATA_HELD_ITEM);
            lvlDivBy10 = (GetMonData(&gPlayerParty[i], MON_DATA_LEVEL) - 1) / 10;  // Moving this here makes it easier to add in abilities like Honey Gather
            if (lvlDivBy10 > 9) lvlDivBy10 = 9;

            if (GetMonData(&gPlayerParty[i], MON_DATA_ABILITY_NUM))
                ability = gBaseStats[species].abilities[1];
            else
                ability = gBaseStats[species].abilities[0];

            if (ability == ABILITY_PICKUP && species != 0 && species != SPECIES_EGG && heldItem == ITEM_NONE && (Random() % 10) == 0) {
                s32 j;
                s32 rand = Random() % 100;

                for (j = 0; j < (int)ARRAY_COUNT(sPickupProbabilities); j++) {
                    if (sPickupProbabilities[j] > rand) {
                        SetMonData(&gPlayerParty[i], MON_DATA_HELD_ITEM, &sPickupItems[lvlDivBy10 + j]);
                        break;
                    } else if (rand == 99 || rand == 98) {
                        SetMonData(&gPlayerParty[i], MON_DATA_HELD_ITEM, &sRarePickupItems[lvlDivBy10 + (99 - rand)]);
                        break;
                    }
                }
            }
#if (defined ITEM_HONEY)
            else if (ability == ABILITY_HONEY_GATHER && species != 0 && species != SPECIES_EGG && heldItem == ITEM_NONE) {
                if ((lvlDivBy10 + 1) * 5 > Random() % 100) {
                    heldItem = ITEM_HONEY;
                    SetMonData(&gPlayerParty[i], MON_DATA_HELD_ITEM, &heldItem);
                }
            }
#endif
        }
    }

    gBattlescriptCurrInstr++;
}

static void Cmd_docastformchangeanimation(void) {
    gActiveBattler = gBattleScripting.battler;

    if (gBattleMons[gActiveBattler].status2 & STATUS2_SUBSTITUTE) *(&gBattleStruct->formToChangeInto) |= 0x80;

    BtlController_EmitBattleAnimation(0, B_ANIM_CASTFORM_CHANGE, gBattleStruct->formToChangeInto);
    MarkBattlerForControllerExec(gActiveBattler);

    gBattlescriptCurrInstr++;
}

static void Cmd_trycastformdatachange(void) { gBattlescriptCurrInstr++; }

static void Cmd_settypebasedhalvers(void)  // water and mud sport
{
    bool8 worked = FALSE;

    if (gBattleMoves[gCurrentMove].effect == EFFECT_MUD_SPORT) {
        if (!(gFieldStatuses & STATUS_FIELD_MUDSPORT)) {
            gFieldStatuses |= STATUS_FIELD_MUDSPORT;
            gFieldTimers.started.mudSport = TRUE;
            gFieldTimers.mudSportTimer = SPORT_DURATION;
            SetActiveMultistringChooser(B_MSG_WEAKEN_ELECTRIC);
            worked = TRUE;
        }
    } else  // water sport
    {
        if (!(gFieldStatuses & STATUS_FIELD_WATERSPORT)) {
            gFieldStatuses |= STATUS_FIELD_WATERSPORT;
            gFieldTimers.started.waterSport = TRUE;
            gFieldTimers.waterSportTimer = SPORT_DURATION;
            SetActiveMultistringChooser(B_MSG_WEAKEN_FIRE);
            worked = TRUE;
        }
    }

    if (worked)
        gBattlescriptCurrInstr += 5;
    else
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
}

int Infiltrates(int battler, int move, InfiltrateType type) {
    ON_ABILITY(battler, FALSE, gAbilities[ability].onInfiltrate, if (gAbilities[ability].onInfiltrate(battler, move) & type) return TRUE)

    return FALSE;
}

bool32 DoesSubstituteBlockMove(u8 battlerAtk, u8 battlerDef, u32 move) {
    if (!(gBattleMons[battlerDef].status2 & STATUS2_SUBSTITUTE)) return FALSE;
    if (gBattleMoves[move].flags & FLAG_SOUND && B_SOUND_SUBSTITUTE >= GEN_6) return FALSE;
    if (gBattleMoves[move].flags & FLAG_HIT_IN_SUBSTITUTE) return FALSE;

    if (Infiltrates(battlerAtk, move, INFILTRATE_SUBSTITUTE)) return FALSE;

    return TRUE;
}

s8 RemainingNoDamageHits(u8 battler) {
    s8 counts = 0;

    ON_ABILITY(battler, TRUE, gAbilities[ability].noDamageHits, counts += gAbilities[ability].noDamageHits - GetSingleUseAbilityCounter(battler, ability))

    return counts;
}

u16 GetNoDamageAbility(u8 battler) {
    ON_ABILITY(
        battler, TRUE, gAbilities[ability].noDamageHits, if (gAbilities[ability].noDamageHits > GetSingleUseAbilityCounter(battler, ability)) return ability)

    return ABILITY_NONE;
}

bool32 DoesDisguiseBlockMove(u8 battlerAtk, u8 battlerDef, u32 move) {
    ON_ABILITY(battlerDef, TRUE, gAbilities[ability].onDisguise, FILTER(gAbilities[ability].onDisguise(battlerDef, TRUE));
               FILTER_NOT(gBattleMons[battlerDef].status2 & STATUS2_TRANSFORMED);
               FILTER_NOT(IS_MOVE_STATUS(move));
               FILTER_NOT(gHitMarker & HITMARKER_IGNORE_DISGUISE && move != MOVE_SUCKER_PUNCH);
               return TRUE;)

    return FALSE;
}

static void Cmd_jumpifsubstituteblocks(void) {
    if (DoesSubstituteBlockMove(gBattlerAttacker, gBattlerTarget, gCurrentMove))
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    else
        gBattlescriptCurrInstr += 5;
}

static void Cmd_tryrecycleitem(void) {
    u16* usedHeldItem;

    gActiveBattler = gBattlerAttacker;
    usedHeldItem = &gBattleStruct->usedHeldItems[gBattlerPartyIndexes[gActiveBattler]][GetBattlerSide(gActiveBattler)];

    if (*usedHeldItem != 0 && gBattleMons[gActiveBattler].item == 0) {
        gLastUsedItem = *usedHeldItem;
        UpdateBattlerItem(gActiveBattler, *usedHeldItem);
        *usedHeldItem = 0;

        gBattlescriptCurrInstr += 5;
    } else {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    }
}

bool32 CanCamouflage(u8 battlerId) {
    if (IS_BATTLER_OF_TYPE(battlerId, sTerrainToType[gBattleTerrain])) return FALSE;
    return TRUE;
}

static void Cmd_settypetoterrain(void) {
    u8 terrainType;
    switch (gFieldStatuses & STATUS_FIELD_TERRAIN_ANY) {
        case STATUS_FIELD_ELECTRIC_TERRAIN:
            terrainType = TYPE_ELECTRIC;
            break;
        case STATUS_FIELD_GRASSY_TERRAIN:
            terrainType = TYPE_GRASS;
            break;
        case STATUS_FIELD_MISTY_TERRAIN:
            terrainType = TYPE_FAIRY;
            break;
        case STATUS_FIELD_PSYCHIC_TERRAIN:
            terrainType = TYPE_PSYCHIC;
            break;
        default:
            terrainType = sTerrainToType[gBattleTerrain];
            break;
    }

    if (!IS_BATTLER_OF_TYPE(gBattlerAttacker, terrainType)) {
        SET_BATTLER_TYPE(gBattlerAttacker, terrainType);
        PREPARE_TYPE_BUFFER(gBattleTextBuff1, terrainType);

        gBattlescriptCurrInstr += 5;
    } else {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    }
}

static void Cmd_unused_pursuitrelated(void) {
    // gActiveBattler = GetBattlerAtPosition(GetBattlerPosition(gBattlerAttacker) ^ BIT_FLANK);

    // if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE
    //     && !(gAbsentBattlerFlags & gBitTable[gActiveBattler])
    //     && gChosenActionByBattler[gActiveBattler] == B_ACTION_USE_MOVE
    //     && gChosenMoveByBattler[gActiveBattler] == MOVE_PURSUIT)
    // {
    //     gActionsByTurnOrder[gActiveBattler] = 11;
    //     gCurrentMove = MOVE_PURSUIT;
    //     gBattlescriptCurrInstr += 5;
    //     gBattleScripting.animTurn = 1;
    //     gBattleScripting.savedBattler = gBattlerAttacker;
    //     gBattlerAttacker = gActiveBattler;
    // }
    // else
    // {
    //     gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    // }
}

static void Cmd_snatchsetbattlers(void) {
    gEffectBattler = gBattlerAttacker;

    if (gBattlerAttacker == gBattlerTarget)
        gBattlerAttacker = gBattlerTarget = gBattleScripting.battler;
    else
        gBattlerTarget = gBattleScripting.battler;

    gBattleScripting.battler = gEffectBattler;
    gBattlescriptCurrInstr++;
}

static void Cmd_removelightscreenreflect(void)  // brick break
{
    u8 side;
    bool32 failed;

#if B_BRICK_BREAK >= GEN_4
    // From Gen 4 onwards, Brick Break can remove screens on the user's side if used on an ally
    side = GetBattlerSide(gBattlerTarget);
#else
    side = GetBattlerSide(gBattlerAttacker) ^ BIT_SIDE;
#endif

#if B_BRICK_BREAK >= GEN_5
    failed = (gMoveResultFlags & MOVE_RESULT_NO_EFFECT);
#else
    failed = FALSE;
#endif

    if (!failed && gSideStatuses[side] & (SIDE_STATUS_REFLECT | SIDE_STATUS_LIGHTSCREEN | SIDE_STATUS_AURORA_VEIL | SIDE_STATUS_SMOKESCREEN)) {
        gSideStatuses[side] &= ~(SIDE_STATUS_REFLECT);
        gSideStatuses[side] &= ~(SIDE_STATUS_LIGHTSCREEN);
        gSideStatuses[side] &= ~(SIDE_STATUS_AURORA_VEIL);
        gSideStatuses[side] &= ~(SIDE_STATUS_SMOKESCREEN);
        gSideTimers[side].reflectTimer = 0;
        gSideTimers[side].lightscreenTimer = 0;
        gSideTimers[side].auroraVeilTimer = 0;
        gSideTimers[side].smokescreenTimer = 0;
        gBattleScripting.animTurn = 1;
        gBattleScripting.animTargetsHit = 1;
    } else {
        gBattleScripting.animTurn = 0;
        gBattleScripting.animTargetsHit = 0;
    }

    gBattlescriptCurrInstr++;
}

u8 GetCatchingBattler(void) {
    if (IsBattlerAlive(GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT)))
        return GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT);
    else
        return GetBattlerAtPosition(B_POSITION_OPPONENT_RIGHT);
}

static void Cmd_handleballthrow(void) {
    u8 ballMultiplier = 10;
    s8 ballAddition = 0;

    if (gBattleControllerExecFlags) return;

    gActiveBattler = gBattlerAttacker;
    gBattlerTarget = GetCatchingBattler();

    if (gBattleTypeFlags & BATTLE_TYPE_TRAINER) {
        BtlController_EmitBallThrowAnim(0, BALL_TRAINER_BLOCK);
        MarkBattlerForControllerExec(gActiveBattler);
        gBattlescriptCurrInstr = BattleScript_TrainerBallBlock;
        AddBagItem(gLastUsedItem, 1);
    } else if (gBattleTypeFlags & BATTLE_TYPE_WALLY_TUTORIAL) {
        BtlController_EmitBallThrowAnim(0, BALL_3_SHAKES_SUCCESS);
        MarkBattlerForControllerExec(gActiveBattler);
        gBattlescriptCurrInstr = BattleScript_WallyBallThrow;
    } else if (FlagGet(FLAG_TOTEM_BATTLE)) {
        BtlController_EmitBallThrowAnim(0, BALL_TRAINER_BLOCK);
        MarkBattlerForControllerExec(gActiveBattler);
        gBattlescriptCurrInstr = BattleScript_LegendaryPokemonBallBlock;
        AddBagItem(gLastUsedItem, 1);
    } else {
        u32 odds, i;
        u8 catchRate;

        gLastThrownBall = gLastUsedItem;
        if (gBattleTypeFlags & BATTLE_TYPE_SAFARI)
            catchRate = gBattleStruct->safariCatchFactor * 1275 / 100;
        else
            catchRate = gBaseStats[gBattleMons[gBattlerTarget].species].catchRate;

#ifdef POKEMON_EXPANSION
        if (gBaseStats[gBattleMons[gBattlerTarget].species].flags & F_ULTRA_BEAST) {
            if (gLastUsedItem == ITEM_BEAST_BALL)
                ballMultiplier = 50;
            else
                ballMultiplier = 1;
        } else {
#endif

            switch (gLastUsedItem) {
                case ITEM_ULTRA_BALL:
                    ballMultiplier = 20;
                    break;
                case ITEM_GREAT_BALL:
                case ITEM_SAFARI_BALL:
#ifdef ITEM_EXPANSION
                case ITEM_SPORT_BALL:
#endif
                    ballMultiplier = 15;
                    break;
                case ITEM_NET_BALL:
                    if (IS_BATTLER_OF_TYPE(gBattlerTarget, TYPE_WATER) || IS_BATTLER_OF_TYPE(gBattlerTarget, TYPE_BUG))
#if B_NET_BALL_MODIFIER >= GEN_7
                        ballMultiplier = 50;
#else
                    ballMultiplier = 30;
#endif
                    break;
                case ITEM_DIVE_BALL:
#if B_DIVE_BALL_MODIFIER >= GEN_4
                    if (GetCurrentMapType() == MAP_TYPE_UNDERWATER || gIsFishingEncounter || gIsSurfingEncounter) ballMultiplier = 35;
#else
                if (GetCurrentMapType() == MAP_TYPE_UNDERWATER) ballMultiplier = 35;
#endif
                    break;
                case ITEM_NEST_BALL:
#if B_NEST_BALL_MODIFIER >= GEN_6
                    //((41 - Pokémon's level) ÷ 10)× if Pokémon's level is between 1 and 29, 1× otherwise.
                    if (gBattleMons[gBattlerTarget].level < 30) ballMultiplier = 41 - gBattleMons[gBattlerTarget].level;
#elif B_NEST_BALL_MODIFIER == GEN_5
                //((41 - Pokémon's level) ÷ 10)×, minimum 1×
                if (gBattleMons[gBattlerTarget].level < 31) ballMultiplier = 41 - gBattleMons[gBattlerTarget].level;
#else
                //((40 - Pokémon's level) ÷ 10)×, minimum 1×
                if (gBattleMons[gBattlerTarget].level < 40) {
                    ballMultiplier = 40 - gBattleMons[gBattlerTarget].level;
                    if (ballMultiplier <= 9) ballMultiplier = 10;
                }
#endif
                    break;
                case ITEM_REPEAT_BALL:
                    if (GetSetPokedexFlag(SpeciesToNationalPokedexNum(gBattleMons[gBattlerTarget].species), FLAG_GET_CAUGHT))
#if B_REPEAT_BALL_MODIFIER >= GEN_7
                        ballMultiplier = 35;
#else
                    ballMultiplier = 30;
#endif
                    break;
                case ITEM_TIMER_BALL:
#if B_TIMER_BALL_MODIFIER >= GEN_5
                    ballMultiplier = (gBattleResults.battleTurnCounter * 3) + 10;
#else
                ballMultiplier = gBattleResults.battleTurnCounter + 10;
#endif
                    if (ballMultiplier > 40) ballMultiplier = 40;
                    break;
#ifdef ITEM_EXPANSION
                case ITEM_DUSK_BALL:
                    RtcCalcLocalTime();
                    if ((gLocalTime.hours >= 20 && gLocalTime.hours <= 3) || gMapHeader.cave || gMapHeader.mapType == MAP_TYPE_UNDERGROUND)
#if B_DUSK_BALL_MODIFIER >= GEN_7
                        ballMultiplier = 30;
#else
                        ballMultiplier = 35;
#endif
                    break;
                case ITEM_QUICK_BALL:
                    if (gBattleResults.battleTurnCounter == 0)
#if B_QUICK_BALL_MODIFIER >= GEN_5
                        ballMultiplier = 50;
#else
                        ballMultiplier = 40;
#endif
                    break;
                case ITEM_LEVEL_BALL:
                    if (gBattleMons[gBattlerAttacker].level >= 4 * gBattleMons[gBattlerTarget].level)
                        ballMultiplier = 80;
                    else if (gBattleMons[gBattlerAttacker].level > 2 * gBattleMons[gBattlerTarget].level)
                        ballMultiplier = 40;
                    else if (gBattleMons[gBattlerAttacker].level > gBattleMons[gBattlerTarget].level)
                        ballMultiplier = 20;
                    break;
                case ITEM_LURE_BALL:
                    if (gIsFishingEncounter)
#if B_LURE_BALL_MODIFIER >= GEN_7
                        ballMultiplier = 50;
#else
                        ballMultiplier = 30;
#endif
                    break;
                case ITEM_MOON_BALL:
                    for (i = 0; i < EVOS_PER_MON; i++) {
                        if (gEvolutionTable[gBattleMons[gBattlerTarget].species][i].method == EVO_ITEM &&
                            gEvolutionTable[gBattleMons[gBattlerTarget].species][i].param == ITEM_MOON_STONE)
                            ballMultiplier = 40;
                    }
                    break;
                case ITEM_LOVE_BALL:
                    if (gBattleMons[gBattlerTarget].species == gBattleMons[gBattlerAttacker].species) {
                        u8 gender1 = GetMonGender(&gEnemyParty[gBattlerPartyIndexes[gBattlerTarget]]);
                        u8 gender2 = GetMonGender(&gPlayerParty[gBattlerPartyIndexes[gBattlerAttacker]]);

                        if (gender1 != gender2 && gender1 != MON_GENDERLESS && gender2 != MON_GENDERLESS) ballMultiplier = 80;
                    }
                    break;
                case ITEM_FAST_BALL:
                    if (gBaseStats[gBattleMons[gBattlerTarget].species].baseSpeed >= 100) ballMultiplier = 40;
                    break;
                case ITEM_HEAVY_BALL:
                    i = GetPokedexHeightWeight(SpeciesToNationalPokedexNum(gBattleMons[gBattlerTarget].species), 1);
#if B_HEAVY_BALL_MODIFIER >= GEN_7
                    if (i < 1000)
                        ballAddition = -20;
                    else if (i < 2000)
                        ballAddition = 0;
                    else if (i < 3000)
                        ballAddition = 20;
                    else
                        ballAddition = 30;
#elif B_HEAVY_BALL_MODIFIER >= GEN_4
                    if (i < 2048)
                        ballAddition = -20;
                    else if (i < 3072)
                        ballAddition = 20;
                    else if (i < 4096)
                        ballAddition = 30;
                    else
                        ballAddition = 40;
#else
                    if (i < 1024)
                        ballAddition = -20;
                    else if (i < 2048)
                        ballAddition = 0;
                    else if (i < 3072)
                        ballAddition = 20;
                    else if (i < 4096)
                        ballAddition = 30;
                    else
                        ballAddition = 40;
#endif
                    break;
                case ITEM_DREAM_BALL:
#if B_DREAM_BALL_MODIFIER >= GEN_8
                    if (gBattleMons[gBattlerTarget].status1 & STATUS1_SLEEP || BattlerHasAbility(gBattlerTarget, ABILITY_COMATOSE, FALSE)) ballMultiplier = 40;
#else
                    ballMultiplier = 10;
#endif
                    break;
                case ITEM_BEAST_BALL:
                    ballMultiplier = 1;
                    break;
#endif
            }

#ifdef POKEMON_EXPANSION
        }
#endif

        // catchRate is unsigned, which means that it may potentially overflow if sum is applied directly.
        if (catchRate < 21 && ballAddition == -20)
            catchRate = 1;
        else
            catchRate = catchRate + ballAddition;

        odds = (catchRate * ballMultiplier / 10) * (gBattleMons[gBattlerTarget].maxHP * 3 - gBattleMons[gBattlerTarget].hp * 2) /
               (3 * gBattleMons[gBattlerTarget].maxHP);

        if (gBattleMons[gBattlerTarget].status1 & (STATUS1_SLEEP | STATUS1_FREEZE)) odds *= 2;
        if (gBattleMons[gBattlerTarget].status1 &
            (STATUS1_POISON | STATUS1_BURN | STATUS1_PARALYSIS | STATUS1_TOXIC_POISON | STATUS1_FROSTBITE | STATUS1_BLEED))
            odds = (odds * 15) / 10;

        if (gLastUsedItem != ITEM_SAFARI_BALL) {
            if (gLastUsedItem == ITEM_MASTER_BALL) {
                gBattleResults.usedMasterBall = TRUE;
            } else {
                if (gBattleResults.catchAttempts[gLastUsedItem - ITEM_ULTRA_BALL] < 0xFF) gBattleResults.catchAttempts[gLastUsedItem - ITEM_ULTRA_BALL]++;
            }
        }

        if (TRUE)  // mon caught
        {
            BtlController_EmitBallThrowAnim(0, BALL_3_SHAKES_SUCCESS);
            MarkBattlerForControllerExec(gActiveBattler);
            UndoFormChange(gBattlerPartyIndexes[gBattlerTarget], GET_BATTLER_SIDE(gBattlerTarget), FALSE);
            if (gSaveBlock2Ptr->askForNickname)
                gBattlescriptCurrInstr = BattleScript_SuccessBallThrow;
            else
                gBattlescriptCurrInstr = BattleScript_SuccessBallThrow_NoNickname;
            SetMonData(&gEnemyParty[gBattlerPartyIndexes[gBattlerTarget]], MON_DATA_POKEBALL, &gLastUsedItem);

            if (CalculatePlayerPartyCount() == PARTY_SIZE)
                gBattleCommunication[MULTISTRING_CHOOSER] = 0;
            else
                gBattleCommunication[MULTISTRING_CHOOSER] = 1;
            if (gLastUsedItem == ITEM_HEAL_BALL) {
                MonRestorePP(&gEnemyParty[gBattlerPartyIndexes[gBattlerTarget]]);
                HealStatusConditions(&gEnemyParty[gBattlerPartyIndexes[gBattlerTarget]], gBattlerPartyIndexes[gBattlerTarget], STATUS1_ANY, gBattlerTarget);
                gBattleMons[gBattlerTarget].hp = gBattleMons[gBattlerTarget].maxHP;
                SetMonData(&gEnemyParty[gBattlerPartyIndexes[gBattlerTarget]], MON_DATA_HP, &gBattleMons[gBattlerTarget].hp);
            } else if (gLastUsedItem == ITEM_DREAM_BALL)  // Give Pokemon their Hidden Ability when caught in a Dream Ball
            {
                u8 Ability = 2;
                SetMonData(&gEnemyParty[gBattlerPartyIndexes[gBattlerTarget]], MON_DATA_ABILITY_NUM, &Ability);
            }
            if (gLastUsedItem == ITEM_FRIEND_BALL) {
                u8 Friendship = 200;
                SetMonData(&gEnemyParty[gBattlerPartyIndexes[gBattlerTarget]], MON_DATA_FRIENDSHIP, &Friendship);
            }
            AddBagItem(gLastUsedItem, 1);
        } else  // mon may be caught, calculate shakes
        {
            u8 shakes;
            u8 maxShakes;

            gBattleSpritesDataPtr->animationData->isCriticalCapture = 0;
            gBattleSpritesDataPtr->animationData->criticalCaptureSuccess = 0;

            if (CriticalCapture(odds)) {
                maxShakes = BALL_1_SHAKE;  // critical capture doesn't guarantee capture
                gBattleSpritesDataPtr->animationData->isCriticalCapture = 1;
            } else {
                maxShakes = BALL_3_SHAKES_SUCCESS;
            }

            if (gLastUsedItem == ITEM_MASTER_BALL) {
                shakes = maxShakes;
            } else {
                odds = Sqrt(Sqrt(16711680 / odds));
                odds = 1048560 / odds;
                for (shakes = 0; shakes < maxShakes && Random() < odds; shakes++);
            }

            BtlController_EmitBallThrowAnim(0, shakes);
            MarkBattlerForControllerExec(gActiveBattler);

            if (shakes == maxShakes)  // mon caught, copy of the code above
            {
                if (IsCriticalCapture()) gBattleSpritesDataPtr->animationData->criticalCaptureSuccess = 1;

                UndoFormChange(gBattlerPartyIndexes[gBattlerTarget], GET_BATTLER_SIDE(gBattlerTarget), FALSE);
                if (gSaveBlock2Ptr->askForNickname)
                    gBattlescriptCurrInstr = BattleScript_SuccessBallThrow;
                else
                    gBattlescriptCurrInstr = BattleScript_SuccessBallThrow_NoNickname;

                SetMonData(&gEnemyParty[gBattlerPartyIndexes[gBattlerTarget]], MON_DATA_POKEBALL, &gLastUsedItem);

                if (CalculatePlayerPartyCount() == PARTY_SIZE)
                    gBattleCommunication[MULTISTRING_CHOOSER] = 0;
                else
                    gBattleCommunication[MULTISTRING_CHOOSER] = 1;

                if (gLastUsedItem == ITEM_HEAL_BALL) {
                    MonRestorePP(&gEnemyParty[gBattlerPartyIndexes[gBattlerTarget]]);
                    HealStatusConditions(&gEnemyParty[gBattlerPartyIndexes[gBattlerTarget]], gBattlerPartyIndexes[gBattlerTarget], STATUS1_ANY, gBattlerTarget);
                    gBattleMons[gBattlerTarget].hp = gBattleMons[gBattlerTarget].maxHP;
                    SetMonData(&gEnemyParty[gBattlerPartyIndexes[gBattlerTarget]], MON_DATA_HP, &gBattleMons[gBattlerTarget].hp);
                } else if (gLastUsedItem == ITEM_DREAM_BALL)  // Give Pokemon their Hidden Ability when caught in a Dream Ball
                {
                    u8 Ability = 2;
                    SetMonData(&gEnemyParty[gBattlerPartyIndexes[gBattlerTarget]], MON_DATA_ABILITY_NUM, &Ability);
                } else if (gLastUsedItem == ITEM_FRIEND_BALL) {
                    u8 Friendship = 200;
                    SetMonData(&gEnemyParty[gBattlerPartyIndexes[gBattlerTarget]], MON_DATA_FRIENDSHIP, &Friendship);
                }
            } else  // not caught
            {
                if (!gHasFetchedBall) gLastUsedBall = gLastUsedItem;

                if (IsCriticalCapture())
                    gBattleCommunication[MULTISTRING_CHOOSER] = BALL_3_SHAKES_FAIL;
                else
                    gBattleCommunication[MULTISTRING_CHOOSER] = shakes;

                gBattlescriptCurrInstr = BattleScript_ShakeBallThrow;
            }
        }
    }
}

static void Cmd_givecaughtmon(void) {
    if (GiveMonToPlayer(&gEnemyParty[gBattlerPartyIndexes[GetCatchingBattler()]]) != MON_GIVEN_TO_PARTY) {
        if (!ShouldShowBoxWasFullMessage()) {
            SetActiveMultistringChooser(B_MSG_SENT_SOMEONES_PC);
            StringCopy(gStringVar1, GetBoxNamePtr(VarGet(VAR_PC_BOX_TO_SEND_MON)));
            GetMonData(&gEnemyParty[gBattlerPartyIndexes[GetCatchingBattler()]], MON_DATA_NICKNAME, gStringVar2);
        } else {
            StringCopy(gStringVar1, GetBoxNamePtr(VarGet(VAR_PC_BOX_TO_SEND_MON)));  // box the mon was sent to
            GetMonData(&gEnemyParty[gBattlerPartyIndexes[GetCatchingBattler()]], MON_DATA_NICKNAME, gStringVar2);
            StringCopy(gStringVar3, GetBoxNamePtr(GetPCBoxToSendMon()));  // box the mon was going to be sent to
            SetActiveMultistringChooser(B_MSG_SOMEONES_BOX_FULL);
        }

        // Change to B_MSG_SENT_LANETTES_PC or B_MSG_LANETTES_BOX_FULL
        if (FlagGet(FLAG_SYS_PC_LANETTE)) gBattleCommunication[MULTISTRING_CHOOSER]++;
    }

    gBattleResults.caughtMonSpecies = GetMonData(&gEnemyParty[gBattlerPartyIndexes[GetCatchingBattler()]], MON_DATA_SPECIES, NULL);
    GetMonData(&gEnemyParty[gBattlerPartyIndexes[GetCatchingBattler()]], MON_DATA_NICKNAME, gBattleResults.caughtMonNick);
    gBattleResults.caughtMonBall = GetMonData(&gEnemyParty[gBattlerPartyIndexes[GetCatchingBattler()]], MON_DATA_POKEBALL, NULL);

    gBattlescriptCurrInstr++;
}

static void Cmd_trysetcaughtmondexflags(void) {
    u16 species = GetMonData(&gEnemyParty[gBattlerPartyIndexes[GetCatchingBattler()]], MON_DATA_SPECIES, NULL);
    u32 personality = GetMonData(&gEnemyParty[gBattlerPartyIndexes[GetCatchingBattler()]], MON_DATA_PERSONALITY, NULL);

    if (GetSetPokedexFlag(SpeciesToNationalPokedexNum(species), FLAG_GET_CAUGHT)) {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    } else {
        HandleSetPokedexFlag(SpeciesToNationalPokedexNum(species), FLAG_SET_CAUGHT, personality);
        gBattlescriptCurrInstr += 5;
    }
}

static void Cmd_displaydexinfo(void) {
    u16 species = GetMonData(&gEnemyParty[gBattlerPartyIndexes[GetCatchingBattler()]], MON_DATA_SPECIES, NULL);
    u16 dexnum = SpeciesToNationalPokedexNum(species);
    u32 otId = gBattleMons[GetCatchingBattler()].otId;
    u32 personality = gBattleMons[GetCatchingBattler()].personality;
    u8 isShiny = GetMonData(&gEnemyParty[gBattlerPartyIndexes[GetCatchingBattler()]], MON_DATA_IS_SHINY, NULL);
    bool8 isAlpha = GetMonData(&gEnemyParty[gBattlerPartyIndexes[GetCatchingBattler()]], MON_DATA_IS_ALPHA, NULL);

    switch (gBattleCommunication[0]) {
        case 0:
            BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 0x10, RGB_BLACK);
            gBattleCommunication[0]++;
            break;
        case 1:
            if (!gPaletteFade.active) {
                FreeAllWindowBuffers();
                gBattleCommunication[TASK_ID] = DisplayCaughtMonDexPage(species, dexnum, otId, personality, isShiny, isAlpha);
                gBattleCommunication[0]++;
            }
            break;
        case 2:
            if (!gPaletteFade.active && gMain.callback2 == BattleMainCB2 && !gTasks[gBattleCommunication[TASK_ID]].isActive) {
                SetVBlankCallback(VBlankCB_Battle);
                gBattleCommunication[0]++;
            }
            break;
        case 3:
            InitBattleBgsVideo();
            LoadBattleTextboxAndBackground();
            gBattle_BG3_X = 0x100;
            gBattleCommunication[0]++;
            break;
        case 4:
            if (!IsDma3ManagerBusyWithBgCopy()) {
                BeginNormalPaletteFade(PALETTES_BG, 0, 0x10, 0, RGB_BLACK);
                ShowBg(0);
                ShowBg(3);
                gBattleCommunication[0]++;
            }
            break;
        case 5:
            if (!gPaletteFade.active) gBattlescriptCurrInstr++;
            break;
    }
}

void HandleBattleWindow(u8 xStart, u8 yStart, u8 xEnd, u8 yEnd, u8 flags) {
    s32 destY, destX, bgId;
    u16 var = 0;

    for (destY = yStart; destY <= yEnd; destY++) {
        for (destX = xStart; destX <= xEnd; destX++) {
            if (destY == yStart) {
                if (destX == xStart)
                    var = 0x1022;
                else if (destX == xEnd)
                    var = 0x1024;
                else
                    var = 0x1023;
            } else if (destY == yEnd) {
                if (destX == xStart)
                    var = 0x1028;
                else if (destX == xEnd)
                    var = 0x102A;
                else
                    var = 0x1029;
            } else {
                if (destX == xStart)
                    var = 0x1025;
                else if (destX == xEnd)
                    var = 0x1027;
                else
                    var = 0x1026;
            }

            if (flags & WINDOW_CLEAR) var = 0;

            bgId = (flags & WINDOW_x80) ? 1 : 0;
            CopyToBgTilemapBufferRect_ChangePalette(bgId, &var, destX, destY, 1, 1, 0x11);
        }
    }
}

void BattleCreateYesNoCursorAt(u8 cursorPosition) {
    u16 src[2];
    src[0] = 1;
    src[1] = 2;

    CopyToBgTilemapBufferRect_ChangePalette(0, src, BATTLE_BOX_YES_NO_Y + 1, 9 + (2 * cursorPosition), 1, 2, 0x11);
    CopyBgTilemapBufferToVram(0);
}

void BattleDestroyYesNoCursorAt(u8 cursorPosition) {
    u16 src[2];
    src[0] = 0x1016;
    src[1] = 0x1016;

    CopyToBgTilemapBufferRect_ChangePalette(0, src, BATTLE_BOX_YES_NO_Y + 1, 9 + (2 * cursorPosition), 1, 2, 0x11);
    CopyBgTilemapBufferToVram(0);
}

void BattleCreateYesNoCursorAt_Two(u8 cursorPosition) {
    u16 src[2];
    src[0] = 1;
    src[1] = 2;

    CopyToBgTilemapBufferRect_ChangePalette(0, src, BATTLE_BOX_YES_NO_Y + 1, 9 + (2 * cursorPosition), 1, 2, 0x11);
    CopyBgTilemapBufferToVram(0);
}

void BattleDestroyYesNoCursorAt_Two(u8 cursorPosition) {
    u16 src[2];
    src[0] = 0x1016;
    src[1] = 0x1016;

    CopyToBgTilemapBufferRect_ChangePalette(0, src, BATTLE_BOX_YES_NO_Y + 1, 9 + (2 * cursorPosition), 1, 2, 0x11);
    CopyBgTilemapBufferToVram(0);
}

static void Cmd_trygivecaughtmonnick(void) {
    switch (gBattleCommunication[MULTIUSE_STATE]) {
        case 0:
            if (!gSaveBlock2Ptr->askForNickname) {
                gBattleCommunication[MULTIUSE_STATE]++;
            } else {
                HandleBattleWindow(BATTLE_BOX_YES_NO_Y, 8, BATTLE_BOX_YES_NO_Y + BATTLE_BOX_YES_NO_WIDTH, 13, 0);
                BattlePutTextOnWindow(gText_BattleYesNoChoice, B_WIN_YESNO);
                gBattleCommunication[MULTIUSE_STATE]++;
                gBattleCommunication[CURSOR_POSITION] = 0;
                BattleCreateYesNoCursorAt(0);
            }
            break;
        case 1:
            if (!gSaveBlock2Ptr->askForNickname) {
                gBattleCommunication[MULTIUSE_STATE] = 4;
            }
            if (JOY_NEW(DPAD_UP) && gBattleCommunication[CURSOR_POSITION] != 0) {
                PlaySE(SE_SELECT);
                BattleDestroyYesNoCursorAt(gBattleCommunication[CURSOR_POSITION]);
                gBattleCommunication[CURSOR_POSITION] = 0;
                BattleCreateYesNoCursorAt(0);
            }
            if (JOY_NEW(DPAD_DOWN) && gBattleCommunication[CURSOR_POSITION] == 0) {
                PlaySE(SE_SELECT);
                BattleDestroyYesNoCursorAt(gBattleCommunication[CURSOR_POSITION]);
                gBattleCommunication[CURSOR_POSITION] = 1;
                BattleCreateYesNoCursorAt(1);
            }
            if (JOY_NEW(A_BUTTON)) {
                PlaySE(SE_SELECT);
                if (gBattleCommunication[CURSOR_POSITION] == 0) {
                    gBattleCommunication[MULTIUSE_STATE]++;
                    BeginFastPaletteFade(3);
                } else {
                    gBattleCommunication[MULTIUSE_STATE] = 4;
                }
            } else if (JOY_NEW(B_BUTTON)) {
                PlaySE(SE_SELECT);
                gBattleCommunication[MULTIUSE_STATE] = 4;
            }
            break;
        case 2:
            if (!gPaletteFade.active) {
                GetMonData(&gEnemyParty[gBattlerPartyIndexes[gBattlerTarget]], MON_DATA_NICKNAME, gBattleStruct->caughtMonNick);
                FreeAllWindowBuffers();

                DoNamingScreen(NAMING_SCREEN_CAUGHT_MON,
                               gBattleStruct->caughtMonNick,
                               GetMonData(&gEnemyParty[gBattlerPartyIndexes[gBattlerTarget]], MON_DATA_SPECIES),
                               GetMonGender(&gEnemyParty[gBattlerPartyIndexes[gBattlerTarget]]),
                               GetMonData(&gEnemyParty[gBattlerPartyIndexes[gBattlerTarget]], MON_DATA_PERSONALITY, NULL),
                               BattleMainCB2);

                gBattleCommunication[MULTIUSE_STATE]++;
            }
            break;
        case 3:
            if (gMain.callback2 == BattleMainCB2 && !gPaletteFade.active) {
                SetMonData(&gEnemyParty[gBattlerPartyIndexes[gBattlerTarget]], MON_DATA_NICKNAME, gBattleStruct->caughtMonNick);
                gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
            }
            break;
        case 4:
            if (CalculatePlayerPartyCount() == PARTY_SIZE)
                gBattlescriptCurrInstr += 5;
            else
                gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
            break;
    }
}

static void Cmd_subattackerhpbydmg(void) {
    gBattleMons[gBattlerAttacker].hp -= gBattleMoveDamage;
    gBattlescriptCurrInstr++;
}

static void Cmd_removeattackerstatus1(void) {
    gBattleMons[gBattlerAttacker].status1 = 0;
    gBattlescriptCurrInstr++;
}

static void Cmd_finishaction(void) { gCurrentActionFuncId = B_ACTION_FINISHED; }

static void Cmd_finishturn(void) {
    gCurrentActionFuncId = B_ACTION_FINISHED;
    gCurrentTurnActionNumber = gBattlersCount;
}

static void Cmd_trainerslideout(void) {
    gActiveBattler = GetBattlerAtPosition(gBattlescriptCurrInstr[1]);
    BtlController_EmitTrainerSlideBack(0);
    MarkBattlerForControllerExec(gActiveBattler);

    gBattlescriptCurrInstr += 2;
}

static const u16 sTelekinesisBanList[] = {
    SPECIES_DIGLETT,
    SPECIES_DUGTRIO,
#ifdef POKEMON_EXPANSION
    SPECIES_DIGLETT_ALOLAN,
    SPECIES_DUGTRIO_ALOLAN,
    SPECIES_SANDYGAST,
    SPECIES_PALOSSAND,
    SPECIES_GENGAR_MEGA,
#endif
};

bool32 IsTelekinesisBannedSpecies(u16 species) {
    u32 i;

    for (i = 0; i < ARRAY_COUNT(sTelekinesisBanList); i++) {
        if (species == sTelekinesisBanList[i]) return TRUE;
    }
    return FALSE;
}

static void Cmd_settelekinesis(void) {
    if (gStatuses3[gBattlerTarget] & (STATUS3_TELEKINESIS | STATUS3_ROOTED | STATUS3_SMACKED_DOWN) || gFieldStatuses & STATUS_FIELD_GRAVITY ||
        IsTelekinesisBannedSpecies(gBattleMons[gBattlerTarget].species)) {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    } else {
        gStatuses3[gBattlerTarget] |= STATUS3_TELEKINESIS;
        gVolatileStructs[gBattlerTarget].telekinesisTimer = 3;
        gBattlescriptCurrInstr += 5;
    }
}

static void Cmd_swapstatstages(void) {
    u8 statId = T1_READ_8(gBattlescriptCurrInstr + 1);
    s8 atkStatStage = gBattleMons[gBattlerAttacker].statStages[statId];
    s8 defStatStage = gBattleMons[gBattlerTarget].statStages[statId];

    gBattleMons[gBattlerAttacker].statStages[statId] = defStatStage;
    gBattleMons[gBattlerTarget].statStages[statId] = atkStatStage;

    gBattlescriptCurrInstr += 2;
}

static void Cmd_averagestats(void) {
    u8 statId = T1_READ_8(gBattlescriptCurrInstr + 1);
    u16 atkStat = *(u16*)((&gBattleMons[gBattlerAttacker].attack) + (statId - 1));
    u16 defStat = *(u16*)((&gBattleMons[gBattlerTarget].attack) + (statId - 1));
    u16 average = (atkStat + defStat) / 2;

    *(u16*)((&gBattleMons[gBattlerAttacker].attack) + (statId - 1)) = average;
    *(u16*)((&gBattleMons[gBattlerTarget].attack) + (statId - 1)) = average;

    gBattlescriptCurrInstr += 2;
}

static void Cmd_jumpifoppositegenders(void) {
    u32 atkGender = GetGenderFromSpeciesAndPersonality(gBattleMons[gBattlerAttacker].species, gBattleMons[gBattlerAttacker].personality);
    u32 defGender = GetGenderFromSpeciesAndPersonality(gBattleMons[gBattlerTarget].species, gBattleMons[gBattlerTarget].personality);

    if ((atkGender == MON_MALE && defGender == MON_FEMALE) || (atkGender == MON_FEMALE && defGender == MON_MALE))
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    else
        gBattlescriptCurrInstr += 5;
}

static void Cmd_trygetbaddreamstarget(void) {
    u8 badDreamsMonSide = GetBattlerSide(gBattlerAttacker);
    for (; gBattlerTarget < gBattlersCount; gBattlerTarget++) {
        if (GetBattlerSide(gBattlerTarget) == badDreamsMonSide) continue;
        if ((gBattleMons[gBattlerTarget].status1 & STATUS1_SLEEP || BattlerHasAbility(gBattlerTarget, ABILITY_COMATOSE, FALSE)) &&
            IsBattlerAlive(gBattlerTarget))
            break;
    }

    if (gBattlerTarget >= gBattlersCount)
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    else if (BattlerHasAbility(gBattlerTarget, ABILITY_SWEET_DREAMS, TRUE))
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 5);
    else if (BATTLER_HAS_ABILITY(gBattlerTarget, ABILITY_PEACEFUL_SLUMBER))
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 9);
    else {
        PREPARE_ABILITY_BUFFER(gBattleTextBuff1, gBattleScripting.abilityPopupOverwrite);
        gBattlescriptCurrInstr += 13;
    }
}

static void Cmd_tryworryseed(void) {
    if (IsWorrySeedBannedAbility(GetBattlerAbility(gBattlerTarget)) || DoesBattlerHaveAbilityShield(gBattlerTarget)) {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    } else {
        UpdateAbilityStateIndicesForNewAbility(gBattlerTarget, ABILITY_INSOMNIA);
        ReplaceAbility(gBattlerTarget, ABILITY_INSOMNIA);
        gBattlescriptCurrInstr += 5;
    }
}

static void Cmd_metalburstdamagecalculator(void) {
    u8 sideAttacker = GetBattlerSide(gBattlerAttacker);
    u8 sideTarget = 0;

    if (gRoundStructs[gBattlerAttacker].physicalDmg && sideAttacker != (sideTarget = GetBattlerSide(gRoundStructs[gBattlerAttacker].physicalBattlerId)) &&
        gBattleMons[gRoundStructs[gBattlerAttacker].physicalBattlerId].hp) {
        gBattleMoveDamage = gRoundStructs[gBattlerAttacker].physicalDmg * 150 / 100;

        if (IsAffectedByFollowMe(gBattlerAttacker, sideTarget, gCurrentMove))
            gBattlerTarget = gSideTimers[sideTarget].followmeTarget;
        else
            gBattlerTarget = gRoundStructs[gBattlerAttacker].physicalBattlerId;

        gBattlescriptCurrInstr += 5;
    } else if (gRoundStructs[gBattlerAttacker].specialDmg && sideAttacker != (sideTarget = GetBattlerSide(gRoundStructs[gBattlerAttacker].specialBattlerId)) &&
               gBattleMons[gRoundStructs[gBattlerAttacker].specialBattlerId].hp) {
        gBattleMoveDamage = gRoundStructs[gBattlerAttacker].specialDmg * 150 / 100;

        if (IsAffectedByFollowMe(gBattlerAttacker, sideTarget, gCurrentMove))
            gBattlerTarget = gSideTimers[sideTarget].followmeTarget;
        else
            gBattlerTarget = gRoundStructs[gBattlerAttacker].specialBattlerId;

        gBattlescriptCurrInstr += 5;
    } else {
        gBattlescriptCurrInstr = T1_READ_PTR(gBattlescriptCurrInstr + 1);
    }

    gBattleMoveDamage = AdjustFixedDamageForParentalBond(gBattleMoveDamage);
    if (!gBattleMoveDamage) gBattleMoveDamage = 1;
}

static bool32 CriticalCapture(u32 odds) {
#if B_CRITICAL_CAPTURE == TRUE
    u32 numCaught = GetNationalPokedexCount(FLAG_GET_CAUGHT);

    if (numCaught <= (NATIONAL_DEX_COUNT * 30) / 650)
        odds = 0;
    else if (numCaught <= (NATIONAL_DEX_COUNT * 150) / 650)
        odds /= 2;
    else if (numCaught <= (NATIONAL_DEX_COUNT * 300) / 650)
        ;  // odds = (odds * 100) / 100;
    else if (numCaught <= (NATIONAL_DEX_COUNT * 450) / 650)
        odds = (odds * 150) / 100;
    else if (numCaught <= (NATIONAL_DEX_COUNT * 600) / 650)
        odds *= 2;
    else
        odds = (odds * 250) / 100;

#ifdef ITEM_CATCHING_CHARM
    if (CheckBagHasItem(ITEM_CATCHING_CHARM, 1)) odds = (odds * (100 + B_CATCHING_CHARM_BOOST)) / 100;
#endif

    odds /= 6;
    if ((Random() % 255) < odds) return TRUE;

    return FALSE;
#else
    return FALSE;
#endif
}

bool8 IsMoveAffectedByParentalBond(u16 move, u8 battlerId) {
    if (gBattleMoves[move].split == SPLIT_STATUS) return FALSE;
    if (gBattleMoves[move].parentalBondBanned) return FALSE;
    if (gBattleMoves[move].effect == EFFECT_SOLARBEAM && (IsBattlerWeatherAffected(battlerId, WEATHER_SUN_ANY) || HasChloroplast(battlerId))) return TRUE;
    if (gBattleMoves[move].effect == EFFECT_ELECTRO_SHOT && IsBattlerWeatherAffected(battlerId, WEATHER_RAIN_ANY)) return TRUE;
    if (gBattleMoves[move].twoTurnMove && !BattlerHasAbility(battlerId, ABILITY_ACCELERATE, FALSE)) return FALSE;
    if (gBattleTypeFlags & BATTLE_TYPE_DOUBLE) {
        switch (GetBattlerBattleMoveTargetFlags(move, battlerId)) {
            case MOVE_TARGET_BOTH:
                if (IsBattlerAlive(BATTLE_OPPOSITE(battlerId)) && IsBattlerAlive(BATTLE_OPPOSITE(BATTLE_PARTNER(battlerId)))) return FALSE;
                break;
            case MOVE_TARGET_FOES_AND_ALLY:
                if (IsBattlerAlive(BATTLE_OPPOSITE(battlerId)) + IsBattlerAlive(BATTLE_OPPOSITE(BATTLE_PARTNER(battlerId))) +
                        IsBattlerAlive(BATTLE_PARTNER(battlerId)) >
                    1)  // Count mons on both sides; ignore attacker
                    return FALSE;
                break;
        }
    }
    return TRUE;
}

// #define CHECK_FOR_BAD_EGGS //Uncomment if you want to check for bad eggs after each step or after each fight (causes slowdown)

void CheckForBadEggs(void) {}

void SetBattlerAffectedFlag(int attacker, int target, int ability) {
    if (attacker == target) return;
    {
        int flag = GetAbilityState(attacker, ability);
        SetAbilityState(attacker, ability, flag | (1 << target));
    }
}

void ClearBattlerAffectedFlag(int attacker, int target, int ability) {
    int flag = GetAbilityState(attacker, ability);
    SetAbilityState(attacker, ability, flag & ~(1 << target));
}

int GetWeatherChangeMultistringChooser(int weather) {
    switch (weather) {
        case ENUM_WEATHER_FOG:
            return B_MSG_STARTED_FOG;
        case ENUM_WEATHER_HAIL:
            return B_MSG_STARTED_HAIL;
        case ENUM_WEATHER_RAIN:
            return B_MSG_STARTED_RAIN;
        case ENUM_WEATHER_RAIN_PRIMAL:
            return B_MSG_STARTED_DOWNPOUR;
        case ENUM_WEATHER_SANDSTORM:
            return B_MSG_STARTED_SANDSTORM;
        case ENUM_WEATHER_SUN:
            return B_MSG_STARTED_SUNLIGHT;
        default:
            return B_MSG_WEATHER_BECAME_NORMAL;
    }
}

int EatTargetBerry(int battler, int target) {
    if (ItemId_GetPocket(gBattleMons[target].item) != POCKET_BERRIES) return FALSE;
    if (IsStickyHold(target)) return FALSE;

    // target loses their berry
    gLastUsedItem = UpdateBattlerItem(target, ITEM_NONE);

    // attacker temporarily gains their item
    gBattleStruct->changedItems[battler] = gBattleMons[battler].item;
    gBattleMons[battler].item = gLastUsedItem;

    BattleScriptCall(BattleScript_MoveEffectBugBite);
    return TRUE;
}
