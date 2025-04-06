#include "global.h"
#include "battle.h"
#include "battle_ai_main.h"
#include "battle_ai_util.h"
#include "battle_anim.h"
#include "battle_arena.h"
#include "battle_controllers.h"
#include "battle_dome.h"
#include "battle_interface.h"
#include "battle_message.h"
#include "battle_setup.h"
#include "battle_tv.h"
#include "battle_main.h"
#include "bg.h"
#include "data.h"
#include "event_data.h"
#include "item.h"
#include "item_menu.h"
#include "item_use.h"
#include "international_string_util.h"
#include "link.h"
#include "main.h"
#include "menu.h"
#include "m4a.h"
#include "overworld.h"
#include "palette.h"
#include "party_menu.h"
#include "pokeball.h"
#include "pokemon.h"
#include "pokemon_icon.h"
#include "pokemon_storage_system.h"
#include "pokemon_summary_screen.h"
#include "random.h"
#include "recorded_battle.h"
#include "reshow_battle_screen.h"
#include "sound.h"
#include "sprite.h"
#include "string_util.h"
#include "task.h"
#include "text.h"
#include "util.h"
#include "ui_battle_menu.h"
#include "ui_information_menu.h"
#include "window.h"
#include "generated/constants/abilities.h"
#include "constants/battle_anim.h"
#include "constants/battle_config.h"
#include "generated/constants/battle_move_effects.h"
#include "constants/items.h"
#include "generated/constants/moves.h"
#include "constants/party_menu.h"
#include "constants/songs.h"
#include "constants/trainers.h"
#include "constants/rgb.h"
#include "mgba_printf/mgba.h"

// Theme Stuff
// Theme 1 - Dark
static const u8 sTheme_Dark_BattleButton_1[] = INCBIN_U8("graphics/ui_menus/battle_interface/theme_1_dark/buttons/button_1.4bpp");
static const u8 sTheme_Dark_BattleButton_1_Selected[] = INCBIN_U8("graphics/ui_menus/battle_interface/theme_1_dark/buttons/button_1_selected.4bpp");
static const u8 sTheme_Dark_BattleButton_2[] = INCBIN_U8("graphics/ui_menus/battle_interface/theme_1_dark/buttons/button_2.4bpp");
static const u8 sTheme_Dark_BattleButton_2_Selected[] = INCBIN_U8("graphics/ui_menus/battle_interface/theme_1_dark/buttons/button_2_selected.4bpp");
static const u8 sTheme_Dark_BattleButton_3[] = INCBIN_U8("graphics/ui_menus/battle_interface/theme_1_dark/buttons/button_3.4bpp");
static const u8 sTheme_Dark_BattleButton_3_Selected[] = INCBIN_U8("graphics/ui_menus/battle_interface/theme_1_dark/buttons/button_3_selected.4bpp");
static const u8 sTheme_Dark_BattleButton_4[] = INCBIN_U8("graphics/ui_menus/battle_interface/theme_1_dark/buttons/button_4.4bpp");
static const u8 sTheme_Dark_BattleButton_4_Selected[] = INCBIN_U8("graphics/ui_menus/battle_interface/theme_1_dark/buttons/button_4_selected.4bpp");
static const u8 sTheme_Dark_BattleButton_Mega[] = INCBIN_U8("graphics/ui_menus/battle_interface/theme_1_dark/buttons/mega_button.4bpp");
static const u8 sTheme_Dark_BattleButton_Catch[] = INCBIN_U8("graphics/ui_menus/battle_interface/theme_1_dark/buttons/catch_button.4bpp");
static const u8 sTheme_Dark_BattleMoveSelector[] = INCBIN_U8("graphics/ui_menus/battle_interface/theme_1_dark/move_selector.4bpp");

static const u8 sTheme_Dark_Extra_Button_Fight[] = INCBIN_U8("graphics/ui_menus/battle_interface/theme_1_dark/buttons/extra/l_button_fight.4bpp");
static const u8 sTheme_Dark_Extra_Button_Forfeit[] = INCBIN_U8("graphics/ui_menus/battle_interface/theme_1_dark/buttons/extra/l_button_forfeit.4bpp");
static const u8 sTheme_Dark_Extra_Button_Info[] = INCBIN_U8("graphics/ui_menus/battle_interface/theme_1_dark/buttons/extra/l_button_info.4bpp");
static const u8 sTheme_Dark_Extra_Button_Pokemon[] = INCBIN_U8("graphics/ui_menus/battle_interface/theme_1_dark/buttons/extra/l_button_pokemon.4bpp");
static const u8 sTheme_Dark_Extra_Button_Run[] = INCBIN_U8("graphics/ui_menus/battle_interface/theme_1_dark/buttons/extra/l_button_run.4bpp");
static const u8 sTheme_Dark_Extra_Button_Debug[] = INCBIN_U8("graphics/ui_menus/battle_interface/theme_1_dark/buttons/extra/l_button_debug.4bpp");
static const u8 sTheme_Dark_Extra_Button_Wiki[] = INCBIN_U8("graphics/ui_menus/battle_interface/theme_1_dark/buttons/extra/l_button_wiki.4bpp");

static const u8 sTheme_Dark_Pokeball_Sane_Gfx[] = INCBIN_U8("graphics/ui_menus/battle_interface/theme_1_dark/pokeball_icons/pokeball_sane.4bpp");
static const u8 sTheme_Dark_Pokeball_Status_Gfx[] = INCBIN_U8("graphics/ui_menus/battle_interface/theme_1_dark/pokeball_icons/pokeball_status.4bpp");
static const u8 sTheme_Dark_Pokeball_Fainted_Gfx[] = INCBIN_U8("graphics/ui_menus/battle_interface/theme_1_dark/pokeball_icons/pokeball_fainted.4bpp");

static const u8 sTheme_Dark_Title_Dmg_Calculation[] = INCBIN_U8("graphics/ui_menus/battle_interface/theme_1_dark/move_info_icons/text_dmg_calc.4bpp");
static const u8 sTheme_Dark_Title_Move_Description[] = INCBIN_U8("graphics/ui_menus/battle_interface/theme_1_dark/move_info_icons/text_move_desc.4bpp");
static const u8 sTheme_Dark_Title_Move_Info[] = INCBIN_U8("graphics/ui_menus/battle_interface/theme_1_dark/move_info_icons/text_move_info.4bpp");
static const u8 sTheme_Dark_Title_Speed_Order[] = INCBIN_U8("graphics/ui_menus/battle_interface/theme_1_dark/move_info_icons/text_spd_order.4bpp");
static const u8 sTheme_Dark_Title_Enemy_Dmg[] = INCBIN_U8("graphics/ui_menus/battle_interface/theme_1_dark/move_info_icons/text_enemy_dmg.4bpp");

static const u8 sTheme_Dark_Title_Move_Name[] = INCBIN_U8("graphics/ui_menus/battle_interface/theme_1_dark/move_info_icons/text_move_name.4bpp");
static const u8 sTheme_Dark_Title_Move_PP[] = INCBIN_U8("graphics/ui_menus/battle_interface/theme_1_dark/move_info_icons/text_move_pp.4bpp");

static const u8 sTheme_Dark_Split_Physical[] = INCBIN_U8("graphics/ui_menus/battle_interface/theme_1_dark/split/split_physical.4bpp");
static const u8 sTheme_Dark_Split_Special[] = INCBIN_U8("graphics/ui_menus/battle_interface/theme_1_dark/split/split_special.4bpp");
static const u8 sTheme_Dark_Split_Status[] = INCBIN_U8("graphics/ui_menus/battle_interface/theme_1_dark/split/split_status.4bpp");

static const u8 sTheme_Dark_Target_0[] = INCBIN_U8("graphics/ui_menus/battle_interface/theme_1_dark/pokeball_icons/target_0.4bpp");
static const u8 sTheme_Dark_Target_1[] = INCBIN_U8("graphics/ui_menus/battle_interface/theme_1_dark/pokeball_icons/target_1.4bpp");

// Theme 2 - Light
static const u8 sTheme_Light_BattleButton_1[] = INCBIN_U8("graphics/ui_menus/battle_interface/theme_1_light/buttons/button_1.4bpp");
static const u8 sTheme_Light_BattleButton_1_Selected[] = INCBIN_U8("graphics/ui_menus/battle_interface/theme_1_light/buttons/button_1_selected.4bpp");
static const u8 sTheme_Light_BattleButton_2[] = INCBIN_U8("graphics/ui_menus/battle_interface/theme_1_light/buttons/button_2.4bpp");
static const u8 sTheme_Light_BattleButton_2_Selected[] = INCBIN_U8("graphics/ui_menus/battle_interface/theme_1_light/buttons/button_2_selected.4bpp");
static const u8 sTheme_Light_BattleButton_3[] = INCBIN_U8("graphics/ui_menus/battle_interface/theme_1_light/buttons/button_3.4bpp");
static const u8 sTheme_Light_BattleButton_3_Selected[] = INCBIN_U8("graphics/ui_menus/battle_interface/theme_1_light/buttons/button_3_selected.4bpp");
static const u8 sTheme_Light_BattleButton_4[] = INCBIN_U8("graphics/ui_menus/battle_interface/theme_1_light/buttons/button_4.4bpp");
static const u8 sTheme_Light_BattleButton_4_Selected[] = INCBIN_U8("graphics/ui_menus/battle_interface/theme_1_light/buttons/button_4_selected.4bpp");
static const u8 sTheme_Light_BattleButton_Mega[] = INCBIN_U8("graphics/ui_menus/battle_interface/theme_1_light/buttons/mega_button.4bpp");
static const u8 sTheme_Light_BattleButton_Catch[] = INCBIN_U8("graphics/ui_menus/battle_interface/theme_1_light/buttons/catch_button.4bpp");
static const u8 sTheme_Light_BattleMoveSelector[] = INCBIN_U8("graphics/ui_menus/battle_interface/theme_1_light/move_selector.4bpp");

static const u8 sTheme_Light_Extra_Button_Fight[] = INCBIN_U8("graphics/ui_menus/battle_interface/theme_1_light/buttons/extra/l_button_fight.4bpp");
static const u8 sTheme_Light_Extra_Button_Forfeit[] = INCBIN_U8("graphics/ui_menus/battle_interface/theme_1_light/buttons/extra/l_button_forfeit.4bpp");
static const u8 sTheme_Light_Extra_Button_Info[] = INCBIN_U8("graphics/ui_menus/battle_interface/theme_1_light/buttons/extra/l_button_info.4bpp");
static const u8 sTheme_Light_Extra_Button_Pokemon[] = INCBIN_U8("graphics/ui_menus/battle_interface/theme_1_light/buttons/extra/l_button_pokemon.4bpp");
static const u8 sTheme_Light_Extra_Button_Run[] = INCBIN_U8("graphics/ui_menus/battle_interface/theme_1_light/buttons/extra/l_button_run.4bpp");
static const u8 sTheme_Light_Extra_Button_Debug[] = INCBIN_U8("graphics/ui_menus/battle_interface/theme_1_light/buttons/extra/l_button_debug.4bpp");
static const u8 sTheme_Light_Extra_Button_Wiki[] = INCBIN_U8("graphics/ui_menus/battle_interface/theme_1_light/buttons/extra/l_button_wiki.4bpp");

static const u8 sTheme_Light_Pokeball_Sane_Gfx[] = INCBIN_U8("graphics/ui_menus/battle_interface/theme_1_light/pokeball_icons/pokeball_sane.4bpp");
static const u8 sTheme_Light_Pokeball_Status_Gfx[] = INCBIN_U8("graphics/ui_menus/battle_interface/theme_1_light/pokeball_icons/pokeball_status.4bpp");
static const u8 sTheme_Light_Pokeball_Fainted_Gfx[] = INCBIN_U8("graphics/ui_menus/battle_interface/theme_1_light/pokeball_icons/pokeball_fainted.4bpp");

static const u8 sTheme_Light_Title_Dmg_Calculation[] = INCBIN_U8("graphics/ui_menus/battle_interface/theme_1_light/move_info_icons/text_dmg_calc.4bpp");
static const u8 sTheme_Light_Title_Move_Description[] = INCBIN_U8("graphics/ui_menus/battle_interface/theme_1_light/move_info_icons/text_move_desc.4bpp");
static const u8 sTheme_Light_Title_Move_Info[] = INCBIN_U8("graphics/ui_menus/battle_interface/theme_1_light/move_info_icons/text_move_info.4bpp");
static const u8 sTheme_Light_Title_Speed_Order[] = INCBIN_U8("graphics/ui_menus/battle_interface/theme_1_light/move_info_icons/text_spd_order.4bpp");
static const u8 sTheme_Light_Title_Enemy_Dmg[] = INCBIN_U8("graphics/ui_menus/battle_interface/theme_1_light/move_info_icons/text_enemy_dmg.4bpp");

static const u8 sTheme_Light_Title_Move_Name[] = INCBIN_U8("graphics/ui_menus/battle_interface/theme_1_light/move_info_icons/text_move_name.4bpp");
static const u8 sTheme_Light_Title_Move_PP[] = INCBIN_U8("graphics/ui_menus/battle_interface/theme_1_light/move_info_icons/text_move_pp.4bpp");

static const u8 sTheme_Light_Split_Physical[] = INCBIN_U8("graphics/ui_menus/battle_interface/theme_1_light/split/split_physical.4bpp");
static const u8 sTheme_Light_Split_Special[] = INCBIN_U8("graphics/ui_menus/battle_interface/theme_1_light/split/split_special.4bpp");
static const u8 sTheme_Light_Split_Status[] = INCBIN_U8("graphics/ui_menus/battle_interface/theme_1_light/split/split_status.4bpp");

static const u8 sTheme_Light_Target_0[] = INCBIN_U8("graphics/ui_menus/battle_interface/theme_1_light/pokeball_icons/target_0.4bpp");
static const u8 sTheme_Light_Target_1[] = INCBIN_U8("graphics/ui_menus/battle_interface/theme_1_light/pokeball_icons/target_1.4bpp");

// Theme 3 - DPPt
static const u8 sTheme_DPPt_BattleButton_1[] = INCBIN_U8("graphics/ui_menus/battle_interface/dppt_theme/buttons/button_1.4bpp");
static const u8 sTheme_DPPt_BattleButton_1_Selected[] = INCBIN_U8("graphics/ui_menus/battle_interface/dppt_theme/buttons/button_1_selected.4bpp");
static const u8 sTheme_DPPt_BattleButton_2[] = INCBIN_U8("graphics/ui_menus/battle_interface/dppt_theme/buttons/button_2.4bpp");
static const u8 sTheme_DPPt_BattleButton_2_Selected[] = INCBIN_U8("graphics/ui_menus/battle_interface/dppt_theme/buttons/button_2_selected.4bpp");
static const u8 sTheme_DPPt_BattleButton_3[] = INCBIN_U8("graphics/ui_menus/battle_interface/dppt_theme/buttons/button_3.4bpp");
static const u8 sTheme_DPPt_BattleButton_3_Selected[] = INCBIN_U8("graphics/ui_menus/battle_interface/dppt_theme/buttons/button_3_selected.4bpp");
static const u8 sTheme_DPPt_BattleButton_4[] = INCBIN_U8("graphics/ui_menus/battle_interface/dppt_theme/buttons/button_4.4bpp");
static const u8 sTheme_DPPt_BattleButton_4_Selected[] = INCBIN_U8("graphics/ui_menus/battle_interface/dppt_theme/buttons/button_4_selected.4bpp");
static const u8 sTheme_DPPt_BattleButton_Mega[] = INCBIN_U8("graphics/ui_menus/battle_interface/dppt_theme/buttons/mega_button.4bpp");
static const u8 sTheme_DPPt_BattleButton_Catch[] = INCBIN_U8("graphics/ui_menus/battle_interface/dppt_theme/buttons/catch_button.4bpp");
static const u8 sTheme_DPPt_BattleMoveSelector[] = INCBIN_U8("graphics/ui_menus/battle_interface/dppt_theme/move_selector.4bpp");

static const u8 sTheme_DPPt_Extra_Button_Fight[] = INCBIN_U8("graphics/ui_menus/battle_interface/dppt_theme/buttons/extra/l_button_fight.4bpp");
static const u8 sTheme_DPPt_Extra_Button_Forfeit[] = INCBIN_U8("graphics/ui_menus/battle_interface/dppt_theme/buttons/extra/l_button_forfeit.4bpp");
static const u8 sTheme_DPPt_Extra_Button_Info[] = INCBIN_U8("graphics/ui_menus/battle_interface/dppt_theme/buttons/extra/l_button_info.4bpp");
static const u8 sTheme_DPPt_Extra_Button_Pokemon[] = INCBIN_U8("graphics/ui_menus/battle_interface/dppt_theme/buttons/extra/l_button_pokemon.4bpp");
static const u8 sTheme_DPPt_Extra_Button_Run[] = INCBIN_U8("graphics/ui_menus/battle_interface/dppt_theme/buttons/extra/l_button_run.4bpp");
static const u8 sTheme_DPPt_Extra_Button_Debug[] = INCBIN_U8("graphics/ui_menus/battle_interface/dppt_theme/buttons/extra/l_button_debug.4bpp");
static const u8 sTheme_DPPt_Extra_Button_Wiki[] = INCBIN_U8("graphics/ui_menus/battle_interface/dppt_theme/buttons/extra/l_button_wiki.4bpp");

static const u8 sTheme_DPPt_Pokeball_Sane_Gfx[] = INCBIN_U8("graphics/ui_menus/battle_interface/dppt_theme/pokeball_icons/pokeball_sane.4bpp");
static const u8 sTheme_DPPt_Pokeball_Status_Gfx[] = INCBIN_U8("graphics/ui_menus/battle_interface/dppt_theme/pokeball_icons/pokeball_status.4bpp");
static const u8 sTheme_DPPt_Pokeball_Fainted_Gfx[] = INCBIN_U8("graphics/ui_menus/battle_interface/dppt_theme/pokeball_icons/pokeball_fainted.4bpp");

static const u8 sTheme_DPPt_Title_Dmg_Calculation[] = INCBIN_U8("graphics/ui_menus/battle_interface/dppt_theme/move_info_icons/text_dmg_calc.4bpp");
static const u8 sTheme_DPPt_Title_Move_Description[] = INCBIN_U8("graphics/ui_menus/battle_interface/dppt_theme/move_info_icons/text_move_desc.4bpp");
static const u8 sTheme_DPPt_Title_Move_Info[] = INCBIN_U8("graphics/ui_menus/battle_interface/dppt_theme/move_info_icons/text_move_info.4bpp");
static const u8 sTheme_DPPt_Title_Speed_Order[] = INCBIN_U8("graphics/ui_menus/battle_interface/dppt_theme/move_info_icons/text_spd_order.4bpp");
static const u8 sTheme_DPPt_Title_Enemy_Dmg[] = INCBIN_U8("graphics/ui_menus/battle_interface/dppt_theme/move_info_icons/text_enemy_dmg.4bpp");

static const u8 sTheme_DPPt_Title_Move_Name[] = INCBIN_U8("graphics/ui_menus/battle_interface/dppt_theme/move_info_icons/text_move_name.4bpp");
static const u8 sTheme_DPPt_Title_Move_PP[] = INCBIN_U8("graphics/ui_menus/battle_interface/dppt_theme/move_info_icons/text_move_pp.4bpp");

static const u8 sTheme_DPPt_Split_Physical[] = INCBIN_U8("graphics/ui_menus/battle_interface/dppt_theme/split/split_physical.4bpp");
static const u8 sTheme_DPPt_Split_Special[] = INCBIN_U8("graphics/ui_menus/battle_interface/dppt_theme/split/split_special.4bpp");
static const u8 sTheme_DPPt_Split_Status[] = INCBIN_U8("graphics/ui_menus/battle_interface/dppt_theme/split/split_status.4bpp");

static const u8 sTheme_DPPt_Target_0[] = INCBIN_U8("graphics/ui_menus/battle_interface/dppt_theme/pokeball_icons/target_0.4bpp");
static const u8 sTheme_DPPt_Target_1[] = INCBIN_U8("graphics/ui_menus/battle_interface/dppt_theme/pokeball_icons/target_1.4bpp");

// Theme 4 - Classic
/*static const u8 sTheme_Classic_BattleButton_1[]          = INCBIN_U8("graphics/ui_menus/battle_interface/classic_theme/buttons/button_1.4bpp");
static const u8 sTheme_Classic_BattleButton_1_Selected[] = INCBIN_U8("graphics/ui_menus/battle_interface/classic_theme/buttons/button_1_selected.4bpp");
static const u8 sTheme_Classic_BattleButton_2[]          = INCBIN_U8("graphics/ui_menus/battle_interface/classic_theme/buttons/button_2.4bpp");
static const u8 sTheme_Classic_BattleButton_2_Selected[] = INCBIN_U8("graphics/ui_menus/battle_interface/classic_theme/buttons/button_2_selected.4bpp");
static const u8 sTheme_Classic_BattleButton_3[]          = INCBIN_U8("graphics/ui_menus/battle_interface/classic_theme/buttons/button_3.4bpp");
static const u8 sTheme_Classic_BattleButton_3_Selected[] = INCBIN_U8("graphics/ui_menus/battle_interface/classic_theme/buttons/button_3_selected.4bpp");
static const u8 sTheme_Classic_BattleButton_4[]          = INCBIN_U8("graphics/ui_menus/battle_interface/classic_theme/buttons/button_4.4bpp");
static const u8 sTheme_Classic_BattleButton_4_Selected[] = INCBIN_U8("graphics/ui_menus/battle_interface/classic_theme/buttons/button_4_selected.4bpp");
static const u8 sTheme_Classic_BattleButton_Mega[]       = INCBIN_U8("graphics/ui_menus/battle_interface/classic_theme/buttons/mega_button.4bpp");*/
static const u8 sTheme_Classic_BattleButton_Catch[] = INCBIN_U8("graphics/ui_menus/battle_interface/classic_theme/buttons/catch_button.4bpp");
static const u8 sTheme_Classic_BattleMoveSelector[] = INCBIN_U8("graphics/ui_menus/battle_interface/classic_theme/move_selector.4bpp");

static const u8 sTheme_Classic_Extra_Button_Fight[] = INCBIN_U8("graphics/ui_menus/battle_interface/classic_theme/buttons/extra/l_button_fight.4bpp");
static const u8 sTheme_Classic_Extra_Button_Forfeit[] = INCBIN_U8("graphics/ui_menus/battle_interface/classic_theme/buttons/extra/l_button_forfeit.4bpp");
static const u8 sTheme_Classic_Extra_Button_Info[] = INCBIN_U8("graphics/ui_menus/battle_interface/classic_theme/buttons/extra/l_button_info.4bpp");
static const u8 sTheme_Classic_Extra_Button_Pokemon[] = INCBIN_U8("graphics/ui_menus/battle_interface/classic_theme/buttons/extra/l_button_pokemon.4bpp");
static const u8 sTheme_Classic_Extra_Button_Run[] = INCBIN_U8("graphics/ui_menus/battle_interface/classic_theme/buttons/extra/l_button_run.4bpp");
static const u8 sTheme_Classic_Extra_Button_Debug[] = INCBIN_U8("graphics/ui_menus/battle_interface/classic_theme/buttons/extra/l_button_debug.4bpp");
static const u8 sTheme_Classic_Extra_Button_Wiki[] = INCBIN_U8("graphics/ui_menus/battle_interface/classic_theme/buttons/extra/l_button_wiki.4bpp");

static const u8 sTheme_Classic_Pokeball_Sane_Gfx[] = INCBIN_U8("graphics/ui_menus/battle_interface/classic_theme/pokeball_icons/pokeball_sane.4bpp");
static const u8 sTheme_Classic_Pokeball_Status_Gfx[] = INCBIN_U8("graphics/ui_menus/battle_interface/classic_theme/pokeball_icons/pokeball_status.4bpp");
static const u8 sTheme_Classic_Pokeball_Fainted_Gfx[] = INCBIN_U8("graphics/ui_menus/battle_interface/classic_theme/pokeball_icons/pokeball_fainted.4bpp");

static const u8 sTheme_Classic_Title_Dmg_Calculation[] = INCBIN_U8("graphics/ui_menus/battle_interface/classic_theme/move_info_icons/text_dmg_calc.4bpp");
static const u8 sTheme_Classic_Title_Move_Description[] = INCBIN_U8("graphics/ui_menus/battle_interface/classic_theme/move_info_icons/text_move_desc.4bpp");
static const u8 sTheme_Classic_Title_Move_Info[] = INCBIN_U8("graphics/ui_menus/battle_interface/classic_theme/move_info_icons/text_move_info.4bpp");
static const u8 sTheme_Classic_Title_Speed_Order[] = INCBIN_U8("graphics/ui_menus/battle_interface/classic_theme/move_info_icons/text_spd_order.4bpp");
static const u8 sTheme_Classic_Title_Enemy_Dmg[] = INCBIN_U8("graphics/ui_menus/battle_interface/classic_theme/move_info_icons/text_enemy_dmg.4bpp");

static const u8 sTheme_Classic_Title_Move_Name[] = INCBIN_U8("graphics/ui_menus/battle_interface/classic_theme/move_info_icons/text_move_name.4bpp");
static const u8 sTheme_Classic_Title_Move_PP[] = INCBIN_U8("graphics/ui_menus/battle_interface/classic_theme/move_info_icons/text_move_pp.4bpp");

static const u8 sTheme_Classic_Split_Physical[] = INCBIN_U8("graphics/ui_menus/battle_interface/classic_theme/split/split_physical.4bpp");
static const u8 sTheme_Classic_Split_Special[] = INCBIN_U8("graphics/ui_menus/battle_interface/classic_theme/split/split_special.4bpp");
static const u8 sTheme_Classic_Split_Status[] = INCBIN_U8("graphics/ui_menus/battle_interface/classic_theme/split/split_status.4bpp");

static const u8 sTheme_Classic_Target_0[] = INCBIN_U8("graphics/ui_menus/battle_interface/classic_theme/pokeball_icons/target_0.4bpp");
static const u8 sTheme_Classic_Target_1[] = INCBIN_U8("graphics/ui_menus/battle_interface/classic_theme/pokeball_icons/target_1.4bpp");

static const u8 sSplit_Physical[] = INCBIN_U8("graphics/ui_menus/battle_interface/split_physical.4bpp");
static const u8 sSplit_Special[] = INCBIN_U8("graphics/ui_menus/battle_interface/split_special.4bpp");
static const u8 sSplit_Status[] = INCBIN_U8("graphics/ui_menus/battle_interface/split_status.4bpp");

extern struct MusicPlayerInfo gMPlayInfo_BGM;
// Mon Icons
u8 BattleInterface_CreateMonIcon(u8 battler);
void BattleInterface_DestroyMonIcon(void);

// this file's functions
static void PlayerHandleGetMonData(void);
static void PlayerHandleSetMonData(void);
static void PlayerHandleSetRawMonData(void);
static void PlayerHandleLoadMonSprite(void);
static void PlayerHandleSwitchInAnim(void);
static void PlayerHandleReturnMonToBall(void);
static void PlayerHandleDrawTrainerPic(void);
static void PlayerHandleTrainerSlide(void);
static void PlayerHandleTrainerSlideBack(void);
static void PlayerHandleFaintAnimation(void);
static void PlayerHandlePaletteFade(void);
static void PlayerHandleSuccessBallThrowAnim(void);
static void PlayerHandleBallThrowAnim(void);
static void PlayerHandlePause(void);
static void PlayerHandleMoveAnimation(void);
static void PlayerHandlePrintString(void);
static void PlayerHandlePrintSelectionString(void);
static void PlayerHandleChooseAction(void);
static void PlayerHandleYesNoBox(void);
static void PlayerHandleChooseMove(void);
static void PlayerHandleChooseItem(void);
static void PlayerHandleChoosePokemon(void);
static void PlayerHandleCmd23(void);
static void PlayerHandleHealthBarUpdate(void);
static void PlayerHandleExpUpdate(void);
static void PlayerHandleStatusIconUpdate(void);
static void PlayerHandleStatusAnimation(void);
static void PlayerHandleStatusXor(void);
static void PlayerHandleDataTransfer(void);
static void PlayerHandleDMA3Transfer(void);
static void PlayerHandlePlayBGM(void);
static void PlayerHandleCmd32(void);
static void PlayerHandleTwoReturnValues(void);
static void PlayerHandleChosenMonReturnValue(void);
static void PlayerHandleOneReturnValue(void);
static void PlayerHandleOneReturnValue_Duplicate(void);
static void PlayerHandleClearUnkVar(void);
static void PlayerHandleSetUnkVar(void);
static void PlayerHandleClearUnkFlag(void);
static void PlayerHandleToggleUnkFlag(void);
static void PlayerHandleHitAnimation(void);
static void PlayerHandleCantSwitch(void);
static void PlayerHandlePlaySE(void);
static void PlayerHandlePlayFanfareOrBGM(void);
static void PlayerHandleFaintingCry(void);
static void PlayerHandleIntroSlide(void);
static void PlayerHandleIntroTrainerBallThrow(void);
static void PlayerHandleDrawPartyStatusSummary(void);
static void PlayerHandleHidePartyStatusSummary(void);
static void PlayerHandleEndBounceEffect(void);
static void PlayerHandleSpriteInvisibility(void);
static void PlayerHandleBattleAnimation(void);
static void PlayerHandleLinkStandbyMsg(void);
static void PlayerHandleResetActionMoveSelection(void);
static void PlayerHandleEndLinkBattle(void);
static void PlayerHandleBattleDebug(void);
static void PlayerHandleBattleInfoMenu(void);
static void PlayerHandleBattleInGameWikiMenu(void);
static void PlayerCmdEnd(void);
static void HandleInputChooseActionPlayer(void);

static void PlayerBufferRunCommand(void);
static void HandleInputChooseTarget(void);
static void HandleInputChooseMove(void);
static void MoveSelectionCreateCursorAt(u8 cursorPos, u8 arg1);
static void MoveSelectionDestroyCursorAt(u8 cursorPos);
static void MoveSelectionDisplayPpNumber(void);
static void MoveSelectionDisplayPpString(void);
static void MoveSelectionDisplayMoveType(void);
static void MoveSelectionDisplayMoveTypeDoubles(u8 targetId);
static void MoveSelectionDisplayMoveNames(void);
static void HandleMoveSwitching(void);
static void SwitchIn_HandleSoundAndEnd(void);
static void WaitForMonSelection(void);
static void CompleteWhenChoseItem(void);
static void Task_LaunchLvlUpAnim(u8 taskId);
static void Task_PrepareToGiveExpWithExpBar(u8 taskId);
static void DestroyExpTaskAndCompleteOnInactiveTextPrinter(u8 taskId);
static void Task_GiveExpWithExpBar(u8 taskId);
static void Task_UpdateLvlInHealthbox(u8 taskId);
static void PrintLinkStandbyMsg(void);
static u32 CopyPlayerMonData(u8 monId, u8 *dst);
static void SetPlayerMonData(u8 monId);
static void StartSendOutAnim(u8 battlerId, bool8 dontClearSubstituteBit);
static void DoSwitchOutAnimation(void);
static void PlayerDoMoveAnimation(void);
static void Task_StartSendOutAnim(u8 taskId);
static void EndDrawPartyStatusSummary(void);
static void ChangeMoveDisplayMode();
static void MoveSelectionDisplaySplitIcon(void);
static void PlayerBufferExecCompleted(void);
static u8 GetMoveTypeEffectiveness(u16 moveNum, u8 targetId, u8 userId, u16 moveType, u16 typeEffectivenessMultiplier);
static u8 GetMoveTypeEffectivenessStatus(u16 moveNum, u8 targetId, u8 userId);

static void (*const sPlayerBufferCommands[CONTROLLER_CMDS_COUNT])(void) = {
    [CONTROLLER_GETMONDATA] = PlayerHandleGetMonData,
    [CONTROLLER_GETRAWMONDATA] = PlayerHandleGetRawMonData,
    [CONTROLLER_SETMONDATA] = PlayerHandleSetMonData,
    [CONTROLLER_SETRAWMONDATA] = PlayerHandleSetRawMonData,
    [CONTROLLER_LOADMONSPRITE] = PlayerHandleLoadMonSprite,
    [CONTROLLER_SWITCHINANIM] = PlayerHandleSwitchInAnim,
    [CONTROLLER_RETURNMONTOBALL] = PlayerHandleReturnMonToBall,
    [CONTROLLER_DRAWTRAINERPIC] = PlayerHandleDrawTrainerPic,
    [CONTROLLER_TRAINERSLIDE] = PlayerHandleTrainerSlide,
    [CONTROLLER_TRAINERSLIDEBACK] = PlayerHandleTrainerSlideBack,
    [CONTROLLER_FAINTANIMATION] = PlayerHandleFaintAnimation,
    [CONTROLLER_PALETTEFADE] = PlayerHandlePaletteFade,
    [CONTROLLER_SUCCESSBALLTHROWANIM] = PlayerHandleSuccessBallThrowAnim,
    [CONTROLLER_BALLTHROWANIM] = PlayerHandleBallThrowAnim,
    [CONTROLLER_PAUSE] = PlayerHandlePause,
    [CONTROLLER_MOVEANIMATION] = PlayerHandleMoveAnimation,
    [CONTROLLER_PRINTSTRING] = PlayerHandlePrintString,
    [CONTROLLER_PRINTSTRINGPLAYERONLY] = PlayerHandlePrintSelectionString,
    [CONTROLLER_CHOOSEACTION] = PlayerHandleChooseAction,
    [CONTROLLER_YESNOBOX] = PlayerHandleYesNoBox,
    [CONTROLLER_CHOOSEMOVE] = PlayerHandleChooseMove,
    [CONTROLLER_OPENBAG] = PlayerHandleChooseItem,
    [CONTROLLER_CHOOSEPOKEMON] = PlayerHandleChoosePokemon,
    [CONTROLLER_23] = PlayerHandleCmd23,
    [CONTROLLER_HEALTHBARUPDATE] = PlayerHandleHealthBarUpdate,
    [CONTROLLER_EXPUPDATE] = PlayerHandleExpUpdate,
    [CONTROLLER_STATUSICONUPDATE] = PlayerHandleStatusIconUpdate,
    [CONTROLLER_STATUSANIMATION] = PlayerHandleStatusAnimation,
    [CONTROLLER_STATUSXOR] = PlayerHandleStatusXor,
    [CONTROLLER_DATATRANSFER] = PlayerHandleDataTransfer,
    [CONTROLLER_DMA3TRANSFER] = PlayerHandleDMA3Transfer,
    [CONTROLLER_PLAYBGM] = PlayerHandlePlayBGM,
    [CONTROLLER_32] = PlayerHandleCmd32,
    [CONTROLLER_TWORETURNVALUES] = PlayerHandleTwoReturnValues,
    [CONTROLLER_CHOSENMONRETURNVALUE] = PlayerHandleChosenMonReturnValue,
    [CONTROLLER_ONERETURNVALUE] = PlayerHandleOneReturnValue,
    [CONTROLLER_ONERETURNVALUE_DUPLICATE] = PlayerHandleOneReturnValue_Duplicate,
    [CONTROLLER_CLEARUNKVAR] = PlayerHandleClearUnkVar,
    [CONTROLLER_SETUNKVAR] = PlayerHandleSetUnkVar,
    [CONTROLLER_CLEARUNKFLAG] = PlayerHandleClearUnkFlag,
    [CONTROLLER_TOGGLEUNKFLAG] = PlayerHandleToggleUnkFlag,
    [CONTROLLER_HITANIMATION] = PlayerHandleHitAnimation,
    [CONTROLLER_CANTSWITCH] = PlayerHandleCantSwitch,
    [CONTROLLER_PLAYSE] = PlayerHandlePlaySE,
    [CONTROLLER_PLAYFANFAREORBGM] = PlayerHandlePlayFanfareOrBGM,
    [CONTROLLER_FAINTINGCRY] = PlayerHandleFaintingCry,
    [CONTROLLER_INTROSLIDE] = PlayerHandleIntroSlide,
    [CONTROLLER_INTROTRAINERBALLTHROW] = PlayerHandleIntroTrainerBallThrow,
    [CONTROLLER_DRAWPARTYSTATUSSUMMARY] = PlayerHandleDrawPartyStatusSummary,
    [CONTROLLER_HIDEPARTYSTATUSSUMMARY] = PlayerHandleHidePartyStatusSummary,
    [CONTROLLER_ENDBOUNCE] = PlayerHandleEndBounceEffect,
    [CONTROLLER_SPRITEINVISIBILITY] = PlayerHandleSpriteInvisibility,
    [CONTROLLER_BATTLEANIMATION] = PlayerHandleBattleAnimation,
    [CONTROLLER_LINKSTANDBYMSG] = PlayerHandleLinkStandbyMsg,
    [CONTROLLER_RESETACTIONMOVESELECTION] = PlayerHandleResetActionMoveSelection,
    [CONTROLLER_ENDLINKBATTLE] = PlayerHandleEndLinkBattle,
    [CONTROLLER_DEBUGMENU] = PlayerHandleBattleDebug,
    [CONTROLLER_INFO_MENU] = PlayerHandleBattleInfoMenu,
    [CONTROLLER_IN_GAME_WIKI] = PlayerHandleBattleInGameWikiMenu,
    [CONTROLLER_TERMINATOR_NOP] = PlayerCmdEnd,
};

// unknown unused data
static const u8 sUnused[] = {0x48, 0x48, 0x20, 0x5a, 0x50, 0x50, 0x50, 0x58};

void BattleControllerDummy(void) {
    // dummy function
}

enum Colors {
    FONT_BLACK,
    FONT_BLACK_2,
    FONT_WHITE,
    FONT_WHITE_2,
    FONT_RED,
    FONT_BLUE,
    FONT_GREEN,
    FONT_YELLOW,
    FONT_PURPLE,
    FONT_GRAY,
    FONT_GRAY_2,
};

static const u8 sMenuWindowFontColors[][3] = {
    [FONT_BLACK] = {TEXT_COLOR_TRANSPARENT, 2, 3},
    [FONT_BLACK_2] = {TEXT_COLOR_TRANSPARENT, 2, TEXT_COLOR_TRANSPARENT},
    [FONT_WHITE] = {TEXT_COLOR_TRANSPARENT, 10, 8},
    [FONT_WHITE_2] = {TEXT_COLOR_TRANSPARENT, 10, TEXT_COLOR_TRANSPARENT},
    [FONT_RED] = {TEXT_COLOR_TRANSPARENT, 12, TEXT_COLOR_TRANSPARENT},
    [FONT_BLUE] = {TEXT_COLOR_TRANSPARENT, 15, TEXT_COLOR_TRANSPARENT},
    [FONT_GREEN] = {TEXT_COLOR_TRANSPARENT, 14, TEXT_COLOR_TRANSPARENT},
    [FONT_YELLOW] = {TEXT_COLOR_TRANSPARENT, 13, TEXT_COLOR_TRANSPARENT},
    [FONT_PURPLE] = {TEXT_COLOR_TRANSPARENT, 11, TEXT_COLOR_TRANSPARENT},
    [FONT_GRAY] = {TEXT_COLOR_TRANSPARENT, 8, TEXT_COLOR_TRANSPARENT},
    [FONT_GRAY_2] = {TEXT_COLOR_TRANSPARENT, 4, TEXT_COLOR_TRANSPARENT},
};

enum optionsButtonMode {
    BATTLE_ACTION_FIGHT,
    BATTLE_ACTION_POKEMON,
    BATTLE_ACTION_RUN,
    BATTLE_ACTION_INFO,
    BATTLE_ACTION_DEBUG,
    BATTLE_ACTION_WIKI,
};

const u8 sText_BattleMenu_Action_Fight[] = _("Fight");
const u8 sText_BattleMenu_Action_Pokemon[] = _("Pokémon");
const u8 sText_BattleMenu_Action_Forfeit[] = _("Forfeit");
const u8 sText_BattleMenu_Action_Run[] = _("Run");
const u8 sText_BattleMenu_Action_Info[] = _("Info");
const u8 sText_BattleMenu_Action_Catch[] = _("Catch");

const u8 sText_BattleMenu_Action_Fight_Case[] = _("FIGHT");
const u8 sText_BattleMenu_Action_Pokemon_Case[] = _("{PKMN}");
const u8 sText_BattleMenu_Action_Forfeit_Case[] = _("FORFEIT");
const u8 sText_BattleMenu_Action_Run_Case[] = _("RUN");
const u8 sText_BattleMenu_Action_Info_Case[] = _("INFO");
const u8 sText_BattleMenu_Action_Catch_Case[] = _("CATCH");

const u8 sText_BattleMenu_Action_Debug[] = _("Debug");  // Unused
const u8 sText_BattleMenu_Action_Bag[] = _("Bag");      // Unused

const u8 sText_BattleMenu_Action_What_Will_X_Do_1[] = _("What will");
const u8 sText_BattleMenu_Action_What_Will_X_Do_2[] = _("{STR_VAR_1} do?");
// const u8 sText_BattleMenu_Action_What_Will_X_Do[] = _("What will\nCrabominable\ndo?");

#define BATTLE_WINDOW_WHAT_WILL_X_DO_SQUARE_SIZE 100
#define BATTLE_WINDOW_WHAT_WILL_X_DO_SQUARE_SIZE_DPPT 88
#define BATTLE_WINDOW_SQUARE_SIZE 47
#define BATTLE_WINDOW_SQUARE_SIZE_DPPT 44
#define BATTLE_WINDOW_SPACE_BETWEEN_SQUARE_AND_TEXT 13
#define BATTLE_WINDOW_SPACE_BETWEEN_SQUARE_AND_TEXT_DPPT 14
#define BATTLE_WINDOW_SPACE_BETWEEN_SQUARE_AND_TEXT_CLASSIC 4

void PrintShortcutButton(u8 windowId, u8 button, u8 x, u8 y, u8 x2, u8 y2) {
    u8 option;
    u8 theme = getBattleInterfaceTheme();
    bool8 isTrainerBattle = FALSE;

    if(button == 0)
        option = BATTLE_ACTION_WIKI;
    else
        option = button - 1;
    
    switch (option) {
        case BATTLE_ACTION_FIGHT:
            if (theme == THEME_DARK)
                BlitBitmapToWindow(windowId, sTheme_Dark_Extra_Button_Fight, (x * 8) + x2, (y * 8) + y2, 32, 8);
            else if (theme == THEME_LIGHT)
                BlitBitmapToWindow(windowId, sTheme_Light_Extra_Button_Fight, (x * 8) + x2, (y * 8) + y2, 32, 8);
            else if (theme == THEME_DPPT)
                BlitBitmapToWindow(windowId, sTheme_DPPt_Extra_Button_Fight, (x * 8) + x2, (y * 8) + y2, 32, 16);
            else if (theme == THEME_CLASSIC)
                BlitBitmapToWindow(windowId, sTheme_Classic_Extra_Button_Fight, (x * 8) + x2, (y * 8) + y2, 32, 8);
            break;
        case BATTLE_ACTION_INFO:
            if (theme == THEME_DARK)
                BlitBitmapToWindow(windowId, sTheme_Dark_Extra_Button_Info, (x * 8) + x2, (y * 8) + y2, 32, 8);
            else if (theme == THEME_LIGHT)
                BlitBitmapToWindow(windowId, sTheme_Light_Extra_Button_Info, (x * 8) + x2, (y * 8) + y2, 32, 8);
            else if (theme == THEME_DPPT)
                BlitBitmapToWindow(windowId, sTheme_DPPt_Extra_Button_Info, (x * 8) + x2, (y * 8) + y2, 32, 16);
            else if (theme == THEME_CLASSIC)
                BlitBitmapToWindow(windowId, sTheme_Classic_Extra_Button_Info, (x * 8) + x2, (y * 8) + y2, 32, 8);
            break;
        case BATTLE_ACTION_POKEMON:
            if (theme == THEME_DARK)
                BlitBitmapToWindow(windowId, sTheme_Dark_Extra_Button_Pokemon, (x * 8) + x2, (y * 8) + y2, 32, 8);
            else if (theme == THEME_LIGHT)
                BlitBitmapToWindow(windowId, sTheme_Light_Extra_Button_Pokemon, (x * 8) + x2, (y * 8) + y2, 32, 8);
            else if (theme == THEME_DPPT)
                BlitBitmapToWindow(windowId, sTheme_DPPt_Extra_Button_Pokemon, (x * 8) + x2, (y * 8) + y2, 32, 16);
            else if (theme == THEME_CLASSIC)
                BlitBitmapToWindow(windowId, sTheme_Classic_Extra_Button_Pokemon, (x * 8) + x2, (y * 8) + y2, 32, 8);
            break;
        case BATTLE_ACTION_RUN:
            isTrainerBattle = (gBattleTypeFlags & (BATTLE_TYPE_TRAINER | BATTLE_TYPE_FRONTIER | BATTLE_TYPE_EREADER_TRAINER | BATTLE_TYPE_TRAINER_HILL));
            if (isTrainerBattle) {
                if (theme == THEME_DARK)
                    BlitBitmapToWindow(windowId, sTheme_Dark_Extra_Button_Run, (x * 8) + x2, (y * 8) + y2, 32, 8);
                else if (theme == THEME_LIGHT)
                    BlitBitmapToWindow(windowId, sTheme_Light_Extra_Button_Run, (x * 8) + x2, (y * 8) + y2, 32, 8);
                else if (theme == THEME_DPPT)
                    BlitBitmapToWindow(windowId, sTheme_DPPt_Extra_Button_Run, (x * 8) + x2, (y * 8) + y2, 32, 16);
                else
                    BlitBitmapToWindow(windowId, sTheme_Classic_Extra_Button_Run, (x * 8) + x2, (y * 8) + y2, 32, 8);
            } else {
                if (theme == THEME_DARK)
                    BlitBitmapToWindow(windowId, sTheme_Dark_Extra_Button_Forfeit, (x * 8) + x2, (y * 8) + y2, 40, 8);
                else if (theme == THEME_LIGHT)
                    BlitBitmapToWindow(windowId, sTheme_Light_Extra_Button_Forfeit, (x * 8) + x2, (y * 8) + y2, 40, 8);
                else if (theme == THEME_DPPT)
                    BlitBitmapToWindow(windowId, sTheme_DPPt_Extra_Button_Forfeit, (x * 8) + x2, (y * 8) + y2, 40, 16);
                else if (theme == THEME_CLASSIC)
                    BlitBitmapToWindow(windowId, sTheme_Classic_Extra_Button_Forfeit, (x * 8) + x2, (y * 8) + y2, 40, 8);
            }
            break;
        case BATTLE_ACTION_DEBUG:
            if (B_ENABLE_DEBUG) {
                if (theme == THEME_DARK)
                    BlitBitmapToWindow(windowId, sTheme_Dark_Extra_Button_Debug, (x * 8) + x2, (y * 8) + y2, 32, 8);
                else if (theme == THEME_LIGHT)
                    BlitBitmapToWindow(windowId, sTheme_Light_Extra_Button_Debug, (x * 8) + x2, (y * 8) + y2, 32, 8);
                else if (theme == THEME_DPPT)
                    BlitBitmapToWindow(windowId, sTheme_DPPt_Extra_Button_Debug, (x * 8) + x2, (y * 8) + y2, 32, 16);
                else if (theme == THEME_CLASSIC)
                    BlitBitmapToWindow(windowId, sTheme_Classic_Extra_Button_Debug, (x * 8) + x2, (y * 8) + y2, 32, 8);
            }
            break;
        case BATTLE_ACTION_WIKI:
            if (theme == THEME_DARK)
                BlitBitmapToWindow(windowId, sTheme_Dark_Extra_Button_Wiki, (x * 8) + x2, (y * 8) + y2, 32, 8);
            else if (theme == THEME_LIGHT)
                BlitBitmapToWindow(windowId, sTheme_Light_Extra_Button_Wiki, (x * 8) + x2, (y * 8) + y2, 32, 8);
            else if (theme == THEME_DPPT)
                BlitBitmapToWindow(windowId, sTheme_DPPt_Extra_Button_Wiki, (x * 8) + x2, (y * 8) + y2, 32, 16);
            else if (theme == THEME_CLASSIC)
                BlitBitmapToWindow(windowId, sTheme_Classic_Extra_Button_Wiki, (x * 8) + x2, (y * 8) + y2, 32, 8);
            break;
    }
}

void PrintBattleWindow_ActionPromt(void) {
    u8 i, x, y, x2, y2, offset;
    u8 windowId = B_WIN_ACTION_PROMPT;
    u8 font = FONT_SMALL_NARROW;
    u8 fontColor = FONT_WHITE_2;
    bool8 isTrainerBattle = (gBattleTypeFlags & (BATTLE_TYPE_TRAINER | BATTLE_TYPE_FRONTIER | BATTLE_TYPE_EREADER_TRAINER | BATTLE_TYPE_TRAINER_HILL));
    u8 battler = gActiveBattler;
    bool8 canUsePokeball = !isTrainerBattle && CanThrowBall() == 0 && FlagGet(FLAG_SYS_DEXNAV_GET);
    u8 shortcutButton = gSaveBlock2Ptr->shortcutButton;
    bool8 hasShortcutButtonEnabled = (shortcutButton != BATTLE_ACTION_DEBUG || B_ENABLE_DEBUG);
    u8 battleTheme = getBattleInterfaceTheme();

    switch (battleTheme) {
        case THEME_DARK:
            fontColor = FONT_WHITE_2;
            break;
        case THEME_LIGHT:
            fontColor = FONT_BLACK_2;
            break;
        case THEME_DPPT:
            fontColor = FONT_BLACK_2;
            break;
        case THEME_CLASSIC:
            fontColor = FONT_BLACK_2;
            break;
    }

    // Fill the window with the fill value
    MoveIntoBattleBgWindow(windowId);
    FillWindowPixelBuffer(windowId, PIXEL_FILL(TEXT_COLOR_TRANSPARENT));

    // Pokeball Icons
    x = 19;
    y = 0;
    y2 = 0;
    x2 = 0;
    switch (battleTheme) {
        default:
            for (i = 0; i < PARTY_SIZE; i++) {
                if (GetMonData(&gPlayerParty[i], MON_DATA_SPECIES, NULL) != SPECIES_NONE) {
                    if (GetMonData(&gPlayerParty[i], MON_DATA_HP, NULL) == 0)  // Fainted
                        BlitBitmapToWindow(windowId, sTheme_Dark_Pokeball_Fainted_Gfx, (x * 8) + x2, (y * 8) + y2, 8, 8);
                    else if (GetMonData(&gPlayerParty[i], MON_DATA_STATUS, NULL) != STATUS1_NONE)  // Status
                        BlitBitmapToWindow(windowId, sTheme_Dark_Pokeball_Status_Gfx, (x * 8) + x2, (y * 8) + y2, 8, 8);
                    else  // Sane
                        BlitBitmapToWindow(windowId, sTheme_Dark_Pokeball_Sane_Gfx, (x * 8) + x2, (y * 8) + y2, 8, 8);
                }
                x++;
            }
            break;
        case THEME_CLASSIC:
            for (i = 0; i < PARTY_SIZE; i++) {
                if (GetMonData(&gPlayerParty[i], MON_DATA_SPECIES, NULL) != SPECIES_NONE) {
                    if (GetMonData(&gPlayerParty[i], MON_DATA_HP, NULL) == 0)  // Fainted
                        BlitBitmapToWindow(windowId, sTheme_Classic_Pokeball_Fainted_Gfx, (x * 8) + x2, (y * 8) + y2, 8, 8);
                    else if (GetMonData(&gPlayerParty[i], MON_DATA_STATUS, NULL) != STATUS1_NONE)  // Status
                        BlitBitmapToWindow(windowId, sTheme_Classic_Pokeball_Status_Gfx, (x * 8) + x2, (y * 8) + y2, 8, 8);
                    else  // Sane
                        BlitBitmapToWindow(windowId, sTheme_Classic_Pokeball_Sane_Gfx, (x * 8) + x2, (y * 8) + y2, 8, 8);
                }
                x++;
            }
            break;
    }

    // Catch Icon
    x = 1;
    y = 1;
    y2 = 0;
    x2 = 0;
    switch (battleTheme) {
        case THEME_DARK:
            if (hasShortcutButtonEnabled) PrintShortcutButton(windowId, shortcutButton, x, y, x2, y2);
            x = x + 9;
            if (canUsePokeball) BlitBitmapToWindow(windowId, sTheme_Dark_BattleButton_Catch, (x * 8) + x2, (y * 8) + y2, 32, 8);
            break;
        case THEME_LIGHT:
            if (hasShortcutButtonEnabled) PrintShortcutButton(windowId, shortcutButton, x, y, x2, y2);
            x = x + 9;
            if (canUsePokeball) BlitBitmapToWindow(windowId, sTheme_Light_BattleButton_Catch, (x * 8) + x2, (y * 8) + y2, 32, 8);
            break;
        case THEME_DPPT:
            x = 0;
            y = 0;
            y2 = 0;
            x2 = 0;
            if (hasShortcutButtonEnabled) PrintShortcutButton(windowId, shortcutButton, x, y, x2, y2);
            x = x + 10;
            if (canUsePokeball) BlitBitmapToWindow(windowId, sTheme_DPPt_BattleButton_Catch, (x * 8) + x2, (y * 8) + y2, 32, 16);
            break;
        case THEME_CLASSIC:
            if (hasShortcutButtonEnabled) PrintShortcutButton(windowId, shortcutButton, x, y, x2, y2);
            x = x + 7;
            if (canUsePokeball) BlitBitmapToWindow(windowId, sTheme_Classic_BattleButton_Catch, (x * 8) + x2 + 4, (y * 8) + y2, 32, 8);
            break;
    }

    // What Will X Do?
    x = 1;
    if ((!canUsePokeball && !hasShortcutButtonEnabled) || battleTheme == THEME_DPPT) {
        y = 1;
        y2 = 4;
    } else {
        y = 2;
        y2 = 0;
    }

    // First Part
    StringCopy(gStringVar1, sText_BattleMenu_Action_What_Will_X_Do_1);
    switch (battleTheme) {
        case THEME_DPPT:
            offset = GetStringCenterAlignXOffset(font, gStringVar1, BATTLE_WINDOW_WHAT_WILL_X_DO_SQUARE_SIZE_DPPT);
            AddTextPrinterParameterized4(windowId, font, (x * 8) + offset, (y * 8) + y2, 0, 0, sMenuWindowFontColors[fontColor], 0xFF, gStringVar1);
            y++;

            // Part two
            StringCopy(gStringVar1, gSpeciesNames[gBattleMons[battler].species]);
            StringExpandPlaceholders(gStringVar4, sText_BattleMenu_Action_What_Will_X_Do_2);
            offset = 4 + GetStringCenterAlignXOffset(font, gStringVar4, BATTLE_WINDOW_WHAT_WILL_X_DO_SQUARE_SIZE_DPPT);
            AddTextPrinterParameterized4(windowId, font, (x * 8) + offset, (y * 8) + y2, 0, 0, sMenuWindowFontColors[fontColor], 0xFF, gStringVar4);
            break;
        case THEME_CLASSIC:
            // This theme does not have this part
            break;
        default:
            offset = GetStringCenterAlignXOffset(font, gStringVar1, BATTLE_WINDOW_WHAT_WILL_X_DO_SQUARE_SIZE);
            AddTextPrinterParameterized4(windowId, font, (x * 8) + offset, (y * 8) + y2, 0, 0, sMenuWindowFontColors[fontColor], 0xFF, gStringVar1);
            y++;

            // Part two
            StringCopy(gStringVar1, gSpeciesNames[gBattleMons[battler].species]);
            StringExpandPlaceholders(gStringVar4, sText_BattleMenu_Action_What_Will_X_Do_2);
            offset = GetStringCenterAlignXOffset(font, gStringVar4, BATTLE_WINDOW_WHAT_WILL_X_DO_SQUARE_SIZE);
            AddTextPrinterParameterized4(windowId, font, (x * 8) + offset, (y * 8) + y2, 0, 0, sMenuWindowFontColors[fontColor], 0xFF, gStringVar4);
            break;
    }

    // Buttons
    x = 15;
    y = 1;
    y2 = 0;

    // Fight Button
    switch (battleTheme) {
        case THEME_DARK:
            if (gActionSelectionCursor[gActiveBattler] == BATTLE_ACTION_FIGHT)
                BlitBitmapToWindow(windowId, sTheme_Dark_BattleButton_1_Selected, (x * 8) + x2, (y * 8) + y2, 64, 16);
            else
                BlitBitmapToWindow(windowId, sTheme_Dark_BattleButton_1, (x * 8) + x2, (y * 8) + y2, 64, 16);
            break;
        case THEME_LIGHT:
            if (gActionSelectionCursor[gActiveBattler] == BATTLE_ACTION_FIGHT)
                BlitBitmapToWindow(windowId, sTheme_Light_BattleButton_1_Selected, (x * 8) + x2, (y * 8) + y2, 64, 16);
            else
                BlitBitmapToWindow(windowId, sTheme_Light_BattleButton_1, (x * 8) + x2, (y * 8) + y2, 64, 16);
            break;
        case THEME_DPPT:
            if (gActionSelectionCursor[gActiveBattler] == BATTLE_ACTION_FIGHT)
                BlitBitmapToWindow(windowId, sTheme_DPPt_BattleButton_1_Selected, (x * 8) + x2, (y * 8) + y2, 64, 16);
            else
                BlitBitmapToWindow(windowId, sTheme_DPPt_BattleButton_1, (x * 8) + x2, (y * 8) + y2, 64, 16);
            break;
        case THEME_CLASSIC:
            if (gActionSelectionCursor[gActiveBattler] == BATTLE_ACTION_FIGHT)
                BlitBitmapToWindow(windowId, sTheme_Classic_BattleMoveSelector, (x * 8) + x2 - 4, (y * 8) + y2 + 4, 8, 8);
            break;
    }

    // Text
    switch (battleTheme) {
        case THEME_DPPT:
            StringCopy(gStringVar1, sText_BattleMenu_Action_Fight);
            offset = 6 + GetStringCenterAlignXOffset(font, gStringVar1, BATTLE_WINDOW_SQUARE_SIZE_DPPT);
            AddTextPrinterParameterized4(windowId, font, (x * 8) + offset, (y * 8) + y2, 0, 0, sMenuWindowFontColors[fontColor], 0xFF, gStringVar1);

            x = x + 6;
            break;
        case THEME_CLASSIC:
            StringCopy(gStringVar1, sText_BattleMenu_Action_Fight_Case);
            offset = BATTLE_WINDOW_SPACE_BETWEEN_SQUARE_AND_TEXT_CLASSIC;
            AddTextPrinterParameterized4(windowId, font, (x * 8) + offset, (y * 8) + y2, 0, 0, sMenuWindowFontColors[fontColor], 0xFF, gStringVar1);
            x = x + 6;
            break;
        default:
            StringCopy(gStringVar1, sText_BattleMenu_Action_Fight);
            offset = GetStringCenterAlignXOffset(font, gStringVar1, BATTLE_WINDOW_SQUARE_SIZE);
            AddTextPrinterParameterized4(windowId, font, (x * 8) + offset, (y * 8) + y2, 0, 0, sMenuWindowFontColors[fontColor], 0xFF, gStringVar1);

            x = x + 6;
            break;
    }

    // Info
    switch (battleTheme) {
        case THEME_DARK:
            if (gActionSelectionCursor[gActiveBattler] == BATTLE_ACTION_INFO)
                BlitBitmapToWindow(windowId, sTheme_Dark_BattleButton_2_Selected, (x * 8) + x2, (y * 8) + y2, 64, 16);
            else
                BlitBitmapToWindow(windowId, sTheme_Dark_BattleButton_2, (x * 8) + x2, (y * 8) + y2, 64, 16);
            break;
        case THEME_LIGHT:
            if (gActionSelectionCursor[gActiveBattler] == BATTLE_ACTION_INFO)
                BlitBitmapToWindow(windowId, sTheme_Light_BattleButton_2_Selected, (x * 8) + x2, (y * 8) + y2, 64, 16);
            else
                BlitBitmapToWindow(windowId, sTheme_Light_BattleButton_2, (x * 8) + x2, (y * 8) + y2, 64, 16);
            break;
        case THEME_DPPT:
            if (gActionSelectionCursor[gActiveBattler] == BATTLE_ACTION_INFO)
                BlitBitmapToWindow(windowId, sTheme_DPPt_BattleButton_2_Selected, (x * 8) + x2, (y * 8) + y2, 64, 16);
            else
                BlitBitmapToWindow(windowId, sTheme_DPPt_BattleButton_2, (x * 8) + x2, (y * 8) + y2, 64, 16);
            break;
        case THEME_CLASSIC:
            if (gActionSelectionCursor[gActiveBattler] == BATTLE_ACTION_INFO)
                BlitBitmapToWindow(windowId, sTheme_Classic_BattleMoveSelector, (x * 8) + x2 - 4, (y * 8) + y2 + 4, 8, 8);
            break;
    }

    // Text
    switch (battleTheme) {
        case THEME_DPPT:
            StringCopy(gStringVar1, sText_BattleMenu_Action_Info);
            offset = BATTLE_WINDOW_SPACE_BETWEEN_SQUARE_AND_TEXT_DPPT + GetStringCenterAlignXOffset(font, gStringVar1, BATTLE_WINDOW_SQUARE_SIZE_DPPT);
            AddTextPrinterParameterized4(windowId, font, (x * 8) + offset, (y * 8) + y2, 0, 0, sMenuWindowFontColors[fontColor], 0xFF, gStringVar1);

            y = y + 2;
            x = 15;
            break;
        case THEME_CLASSIC:
            StringCopy(gStringVar1, sText_BattleMenu_Action_Info_Case);
            offset = BATTLE_WINDOW_SPACE_BETWEEN_SQUARE_AND_TEXT_CLASSIC;
            AddTextPrinterParameterized4(windowId, font, (x * 8) + offset, (y * 8) + y2, 0, 0, sMenuWindowFontColors[fontColor], 0xFF, gStringVar1);

            y = y + 2;
            x = 15;
            break;
        default:
            StringCopy(gStringVar1, sText_BattleMenu_Action_Info);
            offset = BATTLE_WINDOW_SPACE_BETWEEN_SQUARE_AND_TEXT + GetStringCenterAlignXOffset(font, gStringVar1, BATTLE_WINDOW_SQUARE_SIZE);
            AddTextPrinterParameterized4(windowId, font, (x * 8) + offset, (y * 8) + y2, 0, 0, sMenuWindowFontColors[fontColor], 0xFF, gStringVar1);

            y = y + 2;
            x = 15;
            break;
    }

    // Pokemon
    switch (battleTheme) {
        case THEME_DARK:
            if (gActionSelectionCursor[gActiveBattler] == BATTLE_ACTION_POKEMON)
                BlitBitmapToWindow(windowId, sTheme_Dark_BattleButton_3_Selected, (x * 8) + x2, (y * 8) + y2, 64, 16);
            else
                BlitBitmapToWindow(windowId, sTheme_Dark_BattleButton_3, (x * 8) + x2, (y * 8) + y2, 64, 16);
            break;
        case THEME_LIGHT:
            if (gActionSelectionCursor[gActiveBattler] == BATTLE_ACTION_POKEMON)
                BlitBitmapToWindow(windowId, sTheme_Light_BattleButton_3_Selected, (x * 8) + x2, (y * 8) + y2, 64, 16);
            else
                BlitBitmapToWindow(windowId, sTheme_Light_BattleButton_3, (x * 8) + x2, (y * 8) + y2, 64, 16);
            break;
        case THEME_DPPT:
            if (gActionSelectionCursor[gActiveBattler] == BATTLE_ACTION_POKEMON)
                BlitBitmapToWindow(windowId, sTheme_DPPt_BattleButton_3_Selected, (x * 8) + x2, (y * 8) + y2, 64, 16);
            else
                BlitBitmapToWindow(windowId, sTheme_DPPt_BattleButton_3, (x * 8) + x2, (y * 8) + y2, 64, 16);
            break;
        case THEME_CLASSIC:
            if (gActionSelectionCursor[gActiveBattler] == BATTLE_ACTION_POKEMON)
                BlitBitmapToWindow(windowId, sTheme_Classic_BattleMoveSelector, (x * 8) + x2 - 4, (y * 8) + y2 + 4, 8, 8);
            break;
    }

    // Text
    switch (battleTheme) {
        case THEME_DPPT:
            StringCopy(gStringVar1, sText_BattleMenu_Action_Pokemon);
            offset = 6 + GetStringCenterAlignXOffset(font, gStringVar1, BATTLE_WINDOW_SQUARE_SIZE_DPPT);
            AddTextPrinterParameterized4(windowId, font, (x * 8) + offset, (y * 8) + y2, 0, 0, sMenuWindowFontColors[fontColor], 0xFF, gStringVar1);

            x = x + 6;
            break;
        case THEME_CLASSIC:
            StringCopy(gStringVar1, sText_BattleMenu_Action_Pokemon_Case);
            offset = BATTLE_WINDOW_SPACE_BETWEEN_SQUARE_AND_TEXT_CLASSIC;
            AddTextPrinterParameterized4(windowId, font, (x * 8) + offset, (y * 8) + y2, 0, 0, sMenuWindowFontColors[fontColor], 0xFF, gStringVar1);

            x = x + 6;
            break;
        default:
            StringCopy(gStringVar1, sText_BattleMenu_Action_Pokemon);
            offset = GetStringCenterAlignXOffset(font, gStringVar1, BATTLE_WINDOW_SQUARE_SIZE);
            AddTextPrinterParameterized4(windowId, font, (x * 8) + offset, (y * 8) + y2, 0, 0, sMenuWindowFontColors[fontColor], 0xFF, gStringVar1);

            x = x + 6;
            break;
    }

    // Run
    switch (battleTheme) {
        case THEME_DARK:
            if (gActionSelectionCursor[gActiveBattler] == BATTLE_ACTION_RUN)
                BlitBitmapToWindow(windowId, sTheme_Dark_BattleButton_4_Selected, (x * 8) + x2, (y * 8) + y2, 64, 16);
            else
                BlitBitmapToWindow(windowId, sTheme_Dark_BattleButton_4, (x * 8) + x2, (y * 8) + y2, 64, 16);
            break;
        case THEME_LIGHT:
            if (gActionSelectionCursor[gActiveBattler] == BATTLE_ACTION_RUN)
                BlitBitmapToWindow(windowId, sTheme_Light_BattleButton_4_Selected, (x * 8) + x2, (y * 8) + y2, 64, 16);
            else
                BlitBitmapToWindow(windowId, sTheme_Light_BattleButton_4, (x * 8) + x2, (y * 8) + y2, 64, 16);
            break;
        case THEME_DPPT:
            if (gActionSelectionCursor[gActiveBattler] == BATTLE_ACTION_RUN)
                BlitBitmapToWindow(windowId, sTheme_DPPt_BattleButton_4_Selected, (x * 8) + x2, (y * 8) + y2, 64, 16);
            else
                BlitBitmapToWindow(windowId, sTheme_DPPt_BattleButton_4, (x * 8) + x2, (y * 8) + y2, 64, 16);
            break;
        case THEME_CLASSIC:
            if (gActionSelectionCursor[gActiveBattler] == BATTLE_ACTION_RUN)
                BlitBitmapToWindow(windowId, sTheme_Classic_BattleMoveSelector, (x * 8) + x2 - 4, (y * 8) + y2 + 4, 8, 8);
            break;
    }

    // Text
    switch (battleTheme) {
        case THEME_DPPT:
            if (isTrainerBattle)
                StringCopy(gStringVar1, sText_BattleMenu_Action_Forfeit);
            else
                StringCopy(gStringVar1, sText_BattleMenu_Action_Run);
            offset = BATTLE_WINDOW_SPACE_BETWEEN_SQUARE_AND_TEXT_DPPT + GetStringCenterAlignXOffset(font, gStringVar1, BATTLE_WINDOW_SQUARE_SIZE_DPPT);
            AddTextPrinterParameterized4(windowId, font, (x * 8) + offset, (y * 8) + y2, 0, 0, sMenuWindowFontColors[fontColor], 0xFF, gStringVar1);
            break;
        case THEME_CLASSIC:
            if (isTrainerBattle)
                StringCopy(gStringVar1, sText_BattleMenu_Action_Forfeit_Case);
            else
                StringCopy(gStringVar1, sText_BattleMenu_Action_Run_Case);
            offset = BATTLE_WINDOW_SPACE_BETWEEN_SQUARE_AND_TEXT_CLASSIC;
            AddTextPrinterParameterized4(windowId, font, (x * 8) + offset, (y * 8) + y2, 0, 0, sMenuWindowFontColors[fontColor], 0xFF, gStringVar1);
            break;
        default:
            if (isTrainerBattle)
                StringCopy(gStringVar1, sText_BattleMenu_Action_Forfeit);
            else
                StringCopy(gStringVar1, sText_BattleMenu_Action_Run);
            offset = BATTLE_WINDOW_SPACE_BETWEEN_SQUARE_AND_TEXT + GetStringCenterAlignXOffset(font, gStringVar1, BATTLE_WINDOW_SQUARE_SIZE);
            AddTextPrinterParameterized4(windowId, font, (x * 8) + offset, (y * 8) + y2, 0, 0, sMenuWindowFontColors[fontColor], 0xFF, gStringVar1);
            break;
    }

    PutWindowTilemap(windowId);
    CopyWindowToVram(windowId, 3);
}

enum optionsButtonModeSafari {
    BATTLE_ACTION_SAFARI_CATCH,
    BATTLE_ACTION_SAFARI_POKEBLOCK,
    BATTLE_ACTION_SAFARI_GO_NEAR,
    BATTLE_ACTION_SAFARI_RUN,
};

const u8 sText_BattleMenu_Action_Go_Near[] = _("Go Near");
const u8 sText_BattleMenu_Action_Pokeblock[] = _("Pokéblock");

const u8 sText_BattleMenu_Action_Go_Near_Case[] = _("GO NEAR");
const u8 sText_BattleMenu_Action_Pokeblock_Case[] = _("POKéBLOCK");

const u8 sText_BattleMenu_Action_You[] = _("{PLAYER}");

void PrintBattleWindow_ActionPromt_Safari(void) {
    u8 x, y, x2, y2, offset;
    u8 windowId = B_WIN_ACTION_PROMPT;
    u8 font = FONT_SMALL_NARROW;
    u8 fontColor = FONT_WHITE_2;

    switch (getBattleInterfaceTheme()) {
        case THEME_DARK:
            fontColor = FONT_WHITE_2;
            break;
        case THEME_LIGHT:
            fontColor = FONT_BLACK_2;
            break;
        case THEME_DPPT:
            fontColor = FONT_BLACK_2;
            break;
        case THEME_CLASSIC:
            fontColor = FONT_BLACK_2;
            break;
    }

    // Fill the window with the fill value
    MoveIntoBattleBgWindow(windowId);
    FillWindowPixelBuffer(windowId, PIXEL_FILL(TEXT_COLOR_TRANSPARENT));

    // What Will X Do?
    x = 1;
    y = 1;
    y2 = 4;

    // First Part
    StringCopy(gStringVar1, sText_BattleMenu_Action_What_Will_X_Do_1);
    switch (getBattleInterfaceTheme()) {
        case THEME_DPPT:
            offset = GetStringCenterAlignXOffset(font, gStringVar1, BATTLE_WINDOW_WHAT_WILL_X_DO_SQUARE_SIZE_DPPT);
            AddTextPrinterParameterized4(windowId, font, (x * 8) + offset, (y * 8) + y2, 0, 0, sMenuWindowFontColors[fontColor], 0xFF, gStringVar1);
            y++;

            // Part two
            StringCopy(gStringVar1, sText_BattleMenu_Action_You);
            StringExpandPlaceholders(gStringVar4, sText_BattleMenu_Action_What_Will_X_Do_2);
            offset = 4 + GetStringCenterAlignXOffset(font, gStringVar4, BATTLE_WINDOW_WHAT_WILL_X_DO_SQUARE_SIZE_DPPT);
            AddTextPrinterParameterized4(windowId, font, (x * 8) + offset, (y * 8) + y2, 0, 0, sMenuWindowFontColors[fontColor], 0xFF, gStringVar4);
            break;
        case THEME_CLASSIC:
            // This theme does not have this part
            break;
        default:
            offset = GetStringCenterAlignXOffset(font, gStringVar1, BATTLE_WINDOW_WHAT_WILL_X_DO_SQUARE_SIZE);
            AddTextPrinterParameterized4(windowId, font, (x * 8) + offset, (y * 8) + y2, 0, 0, sMenuWindowFontColors[fontColor], 0xFF, gStringVar1);
            y++;

            // Part two
            StringCopy(gStringVar1, sText_BattleMenu_Action_You);
            StringExpandPlaceholders(gStringVar4, sText_BattleMenu_Action_What_Will_X_Do_2);
            offset = GetStringCenterAlignXOffset(font, gStringVar4, BATTLE_WINDOW_WHAT_WILL_X_DO_SQUARE_SIZE);
            AddTextPrinterParameterized4(windowId, font, (x * 8) + offset, (y * 8) + y2, 0, 0, sMenuWindowFontColors[fontColor], 0xFF, gStringVar4);
            break;
    }

    // Buttons
    x = 15;
    y = 1;
    y2 = 0;
    // Not sure if this is correct, it was unset
    x2 = 0;

    // Fight Button
    switch (getBattleInterfaceTheme()) {
        case THEME_DARK:
            if (gActionSelectionCursor[gActiveBattler] == BATTLE_ACTION_SAFARI_CATCH)
                BlitBitmapToWindow(windowId, sTheme_Dark_BattleButton_1_Selected, (x * 8) + x2, (y * 8) + y2, 64, 16);
            else
                BlitBitmapToWindow(windowId, sTheme_Dark_BattleButton_1, (x * 8) + x2, (y * 8) + y2, 64, 16);
            break;
        case THEME_LIGHT:
            if (gActionSelectionCursor[gActiveBattler] == BATTLE_ACTION_SAFARI_CATCH)
                BlitBitmapToWindow(windowId, sTheme_Light_BattleButton_1_Selected, (x * 8) + x2, (y * 8) + y2, 64, 16);
            else
                BlitBitmapToWindow(windowId, sTheme_Light_BattleButton_1, (x * 8) + x2, (y * 8) + y2, 64, 16);
            break;
        case THEME_DPPT:
            if (gActionSelectionCursor[gActiveBattler] == BATTLE_ACTION_SAFARI_CATCH)
                BlitBitmapToWindow(windowId, sTheme_DPPt_BattleButton_1_Selected, (x * 8) + x2, (y * 8) + y2, 64, 16);
            else
                BlitBitmapToWindow(windowId, sTheme_DPPt_BattleButton_1, (x * 8) + x2, (y * 8) + y2, 64, 16);
            break;
        case THEME_CLASSIC:
            if (gActionSelectionCursor[gActiveBattler] == BATTLE_ACTION_SAFARI_CATCH)
                BlitBitmapToWindow(windowId, sTheme_Classic_BattleMoveSelector, (x * 8) + x2 - 4, (y * 8) + y2 + 4, 8, 8);
            break;
    }

    // Text
    switch (getBattleInterfaceTheme()) {
        case THEME_DPPT:
            StringCopy(gStringVar1, sText_BattleMenu_Action_Catch);
            offset = 6 + GetStringCenterAlignXOffset(font, gStringVar1, BATTLE_WINDOW_SQUARE_SIZE_DPPT);
            AddTextPrinterParameterized4(windowId, font, (x * 8) + offset, (y * 8) + y2, 0, 0, sMenuWindowFontColors[fontColor], 0xFF, gStringVar1);

            x = x + 6;
            break;
        case THEME_CLASSIC:
            StringCopy(gStringVar1, sText_BattleMenu_Action_Catch_Case);
            offset = BATTLE_WINDOW_SPACE_BETWEEN_SQUARE_AND_TEXT_CLASSIC;
            AddTextPrinterParameterized4(windowId, font, (x * 8) + offset, (y * 8) + y2, 0, 0, sMenuWindowFontColors[fontColor], 0xFF, gStringVar1);
            x = x + 6;
            break;
        default:
            StringCopy(gStringVar1, sText_BattleMenu_Action_Catch);
            offset = GetStringCenterAlignXOffset(font, gStringVar1, BATTLE_WINDOW_SQUARE_SIZE);
            AddTextPrinterParameterized4(windowId, font, (x * 8) + offset, (y * 8) + y2, 0, 0, sMenuWindowFontColors[fontColor], 0xFF, gStringVar1);

            x = x + 6;
            break;
    }

    // Info
    switch (getBattleInterfaceTheme()) {
        case THEME_DARK:
            if (gActionSelectionCursor[gActiveBattler] == BATTLE_ACTION_SAFARI_POKEBLOCK)
                BlitBitmapToWindow(windowId, sTheme_Dark_BattleButton_2_Selected, (x * 8) + x2, (y * 8) + y2, 64, 16);
            else
                BlitBitmapToWindow(windowId, sTheme_Dark_BattleButton_2, (x * 8) + x2, (y * 8) + y2, 64, 16);
            break;
        case THEME_LIGHT:
            if (gActionSelectionCursor[gActiveBattler] == BATTLE_ACTION_SAFARI_POKEBLOCK)
                BlitBitmapToWindow(windowId, sTheme_Light_BattleButton_2_Selected, (x * 8) + x2, (y * 8) + y2, 64, 16);
            else
                BlitBitmapToWindow(windowId, sTheme_Light_BattleButton_2, (x * 8) + x2, (y * 8) + y2, 64, 16);
            break;
        case THEME_DPPT:
            if (gActionSelectionCursor[gActiveBattler] == BATTLE_ACTION_SAFARI_POKEBLOCK)
                BlitBitmapToWindow(windowId, sTheme_DPPt_BattleButton_2_Selected, (x * 8) + x2, (y * 8) + y2, 64, 16);
            else
                BlitBitmapToWindow(windowId, sTheme_DPPt_BattleButton_2, (x * 8) + x2, (y * 8) + y2, 64, 16);
            break;
        case THEME_CLASSIC:
            if (gActionSelectionCursor[gActiveBattler] == BATTLE_ACTION_SAFARI_POKEBLOCK)
                BlitBitmapToWindow(windowId, sTheme_Classic_BattleMoveSelector, (x * 8) + x2 - 4, (y * 8) + y2 + 4, 8, 8);
    }

    // Text
    switch (getBattleInterfaceTheme()) {
        case THEME_DPPT:
            StringCopy(gStringVar1, sText_BattleMenu_Action_Pokeblock);
            offset = BATTLE_WINDOW_SPACE_BETWEEN_SQUARE_AND_TEXT_DPPT + GetStringCenterAlignXOffset(font, gStringVar1, BATTLE_WINDOW_SQUARE_SIZE_DPPT);
            AddTextPrinterParameterized4(windowId, font, (x * 8) + offset, (y * 8) + y2, 0, 0, sMenuWindowFontColors[fontColor], 0xFF, gStringVar1);

            y = y + 2;
            x = 15;
            break;
        case THEME_CLASSIC:
            StringCopy(gStringVar1, sText_BattleMenu_Action_Pokeblock_Case);
            offset = BATTLE_WINDOW_SPACE_BETWEEN_SQUARE_AND_TEXT_CLASSIC;
            AddTextPrinterParameterized4(windowId, font, (x * 8) + offset, (y * 8) + y2, 0, 0, sMenuWindowFontColors[fontColor], 0xFF, gStringVar1);

            y = y + 2;
            x = 15;
            break;
        default:
            StringCopy(gStringVar1, sText_BattleMenu_Action_Pokeblock);
            offset = BATTLE_WINDOW_SPACE_BETWEEN_SQUARE_AND_TEXT + GetStringCenterAlignXOffset(font, gStringVar1, BATTLE_WINDOW_SQUARE_SIZE) + 4;
            AddTextPrinterParameterized4(windowId, font, (x * 8) + offset, (y * 8) + y2, 0, 0, sMenuWindowFontColors[fontColor], 0xFF, gStringVar1);

            y = y + 2;
            x = 15;
            break;
    }

    // Pokemon
    switch (getBattleInterfaceTheme()) {
        case THEME_DARK:
            if (gActionSelectionCursor[gActiveBattler] == BATTLE_ACTION_SAFARI_GO_NEAR)
                BlitBitmapToWindow(windowId, sTheme_Dark_BattleButton_3_Selected, (x * 8) + x2, (y * 8) + y2, 64, 16);
            else
                BlitBitmapToWindow(windowId, sTheme_Dark_BattleButton_3, (x * 8) + x2, (y * 8) + y2, 64, 16);
            break;
        case THEME_LIGHT:
            if (gActionSelectionCursor[gActiveBattler] == BATTLE_ACTION_SAFARI_GO_NEAR)
                BlitBitmapToWindow(windowId, sTheme_Light_BattleButton_3_Selected, (x * 8) + x2, (y * 8) + y2, 64, 16);
            else
                BlitBitmapToWindow(windowId, sTheme_Light_BattleButton_3, (x * 8) + x2, (y * 8) + y2, 64, 16);
            break;
        case THEME_DPPT:
            if (gActionSelectionCursor[gActiveBattler] == BATTLE_ACTION_SAFARI_GO_NEAR)
                BlitBitmapToWindow(windowId, sTheme_DPPt_BattleButton_3_Selected, (x * 8) + x2, (y * 8) + y2, 64, 16);
            else
                BlitBitmapToWindow(windowId, sTheme_DPPt_BattleButton_3, (x * 8) + x2, (y * 8) + y2, 64, 16);
            break;
        case THEME_CLASSIC:
            if (gActionSelectionCursor[gActiveBattler] == BATTLE_ACTION_SAFARI_GO_NEAR)
                BlitBitmapToWindow(windowId, sTheme_Classic_BattleMoveSelector, (x * 8) + x2 - 4, (y * 8) + y2 + 4, 8, 8);
            break;
    }

    // Text
    switch (getBattleInterfaceTheme()) {
        case THEME_DPPT:
            StringCopy(gStringVar1, sText_BattleMenu_Action_Go_Near);
            offset = 6 + GetStringCenterAlignXOffset(font, gStringVar1, BATTLE_WINDOW_SQUARE_SIZE_DPPT);
            AddTextPrinterParameterized4(windowId, font, (x * 8) + offset, (y * 8) + y2, 0, 0, sMenuWindowFontColors[fontColor], 0xFF, gStringVar1);

            x = x + 6;
            break;
        case THEME_CLASSIC:
            StringCopy(gStringVar1, sText_BattleMenu_Action_Go_Near_Case);
            offset = BATTLE_WINDOW_SPACE_BETWEEN_SQUARE_AND_TEXT_CLASSIC;
            AddTextPrinterParameterized4(windowId, font, (x * 8) + offset, (y * 8) + y2, 0, 0, sMenuWindowFontColors[fontColor], 0xFF, gStringVar1);

            x = x + 6;
            break;
        default:
            StringCopy(gStringVar1, sText_BattleMenu_Action_Go_Near);
            offset = GetStringCenterAlignXOffset(font, gStringVar1, BATTLE_WINDOW_SQUARE_SIZE);
            AddTextPrinterParameterized4(windowId, font, (x * 8) + offset, (y * 8) + y2, 0, 0, sMenuWindowFontColors[fontColor], 0xFF, gStringVar1);

            x = x + 6;
            break;
    }

    // Run
    switch (getBattleInterfaceTheme()) {
        case THEME_DARK:
            if (gActionSelectionCursor[gActiveBattler] == BATTLE_ACTION_SAFARI_RUN)
                BlitBitmapToWindow(windowId, sTheme_Dark_BattleButton_4_Selected, (x * 8) + x2, (y * 8) + y2, 64, 16);
            else
                BlitBitmapToWindow(windowId, sTheme_Dark_BattleButton_4, (x * 8) + x2, (y * 8) + y2, 64, 16);
            break;
        case THEME_LIGHT:
            if (gActionSelectionCursor[gActiveBattler] == BATTLE_ACTION_SAFARI_RUN)
                BlitBitmapToWindow(windowId, sTheme_Light_BattleButton_4_Selected, (x * 8) + x2, (y * 8) + y2, 64, 16);
            else
                BlitBitmapToWindow(windowId, sTheme_Light_BattleButton_4, (x * 8) + x2, (y * 8) + y2, 64, 16);
            break;
        case THEME_DPPT:
            if (gActionSelectionCursor[gActiveBattler] == BATTLE_ACTION_SAFARI_RUN)
                BlitBitmapToWindow(windowId, sTheme_DPPt_BattleButton_4_Selected, (x * 8) + x2, (y * 8) + y2, 64, 16);
            else
                BlitBitmapToWindow(windowId, sTheme_DPPt_BattleButton_4, (x * 8) + x2, (y * 8) + y2, 64, 16);
            break;
        case THEME_CLASSIC:
            if (gActionSelectionCursor[gActiveBattler] == BATTLE_ACTION_SAFARI_RUN)
                BlitBitmapToWindow(windowId, sTheme_Classic_BattleMoveSelector, (x * 8) + x2 - 4, (y * 8) + y2 + 4, 8, 8);
            break;
    }

    // Text
    switch (getBattleInterfaceTheme()) {
        case THEME_DPPT:
            StringCopy(gStringVar1, sText_BattleMenu_Action_Run);
            offset = BATTLE_WINDOW_SPACE_BETWEEN_SQUARE_AND_TEXT_DPPT + GetStringCenterAlignXOffset(font, gStringVar1, BATTLE_WINDOW_SQUARE_SIZE_DPPT);
            AddTextPrinterParameterized4(windowId, font, (x * 8) + offset, (y * 8) + y2, 0, 0, sMenuWindowFontColors[fontColor], 0xFF, gStringVar1);
            break;
        case THEME_CLASSIC:
            StringCopy(gStringVar1, sText_BattleMenu_Action_Run_Case);
            offset = BATTLE_WINDOW_SPACE_BETWEEN_SQUARE_AND_TEXT_CLASSIC;
            AddTextPrinterParameterized4(windowId, font, (x * 8) + offset, (y * 8) + y2, 0, 0, sMenuWindowFontColors[fontColor], 0xFF, gStringVar1);
            break;
        default:
            StringCopy(gStringVar1, sText_BattleMenu_Action_Run);
            offset = BATTLE_WINDOW_SPACE_BETWEEN_SQUARE_AND_TEXT + GetStringCenterAlignXOffset(font, gStringVar1, BATTLE_WINDOW_SQUARE_SIZE);
            AddTextPrinterParameterized4(windowId, font, (x * 8) + offset, (y * 8) + y2, 0, 0, sMenuWindowFontColors[fontColor], 0xFF, gStringVar1);
            break;
    }

    PutWindowTilemap(windowId);
    CopyWindowToVram(windowId, 3);
}

enum {
    // MOVE_INFO_DESCRIPTION,
    MOVE_INFO_POWER_ACC_PRIO_TYPE,
    MOVE_INFO_POWER_ACC_PRIO_TYPE_2,
    MOVE_SPEED_CALCULATION,
    MOVE_INFO_DAMAGE_CALCULATION,
    MOVE_INFO_DAMAGE_ENEMY,
    NUM_MOVE_INFO_TYPES,
};

#define SPACE_BETWEEN_MOVE_NAME_AND_PP (10 * 8)
#define SPACE_BETWEEN_MOVE_NAME_AND_DESCRIPTION SPACE_BETWEEN_MOVE_NAME_AND_PP + (5 * 8) + 4
#define MOVE_SPLIT_X SPACE_BETWEEN_MOVE_NAME_AND_DESCRIPTION + (6 * 8) + 4
static const u8 sText_PP[] = _("{STR_VAR_1}/{STR_VAR_2}");
const u8 sText_MoveInfo_Power[] = _("Power:");
const u8 sText_MoveInfo_True_Power[] = _("True Power:");
const u8 sText_MoveInfo_Accuracy[] = _("Accuracy:");
const u8 sText_MoveInfo_Priority[] = _("Priority:");
const u8 sText_MoveInfo_Type_Null[] = _("Type:");
const u8 sText_MoveInfo_Types_Null[] = _("Types:");
const u8 sText_MoveInfo_Type[] = _("{STR_VAR_2} {STR_VAR_1}");
const u8 sText_MoveInfo_Type_Double[] = _("{STR_VAR_2} {STR_VAR_1} & {STR_VAR_3}");
const u8 sText_MoveInfo_Chance[] = _("Chance: {STR_VAR_1}");
const u8 sText_MoveInfo_Contact[] = _("Contact:");
const u8 sText_MoveInfo_Boost[] = _("Boost Type:");
const u8 sText_Effect_Based_Type[] = _("Based on:");
const u8 sText_Target_Nothing[] = _("---");
const u8 sText_DamageCalculation_Status[] = _("This is a status\nmove. It does not\ndeal direct\ndamage.");
static const u8 gStabIcon[] = _("{PLUS}");
static const u8 gNoStabIcon[] = _(" ");
static const u8 sText_Yes[] = _("Yes");
static const u8 sText_No[] = _("No");

const u8 sText_Effect_Hit_Number[] = _("Max Hits");
const u8 sText_Effect_Boost_Type_Weather[] = _("Weather");
const u8 sText_Effect_Boost_Type_Fist[] = _("Fist");
const u8 sText_Effect_Boost_Type_Kick[] = _("Kick");
const u8 sText_Effect_Boost_Type_Sound[] = _("Sound");
const u8 sText_Effect_Boost_Type_Cannon[] = _("Cannon");
const u8 sText_Effect_Boost_Type_Dance[] = _("Dance");
const u8 sText_Effect_Boost_Type_Field[] = _("Field");
const u8 sText_Effect_Boost_Type_Bone[] = _("Bone");
const u8 sText_Effect_Boost_Type_Slash[] = _("Slash");
const u8 sText_Effect_Boost_Type_Bite[] = _("Bite");
const u8 sText_Effect_Boost_Type_Arrow[] = _("Arrow");
const u8 sText_Effect_Boost_Type_Hammer[] = _("Hammer");
const u8 sText_Effect_Boost_Type_Air[] = _("Air");
const u8 sText_Effect_Boost_Type_Horn[] = _("Horn");
const u8 sText_Effect_Boost_Type_Throw[] = _("Throwing");
const u8 sText_Effect_Boost_Type_Reckless[] = _("Reckless");
const u8 sText_Effect_Critical_High[] = _("High");
const u8 sText_Effect_Critical_Always[] = _("Always");
const u8 sText_Effect_Sheer_Force_Boosted[] = _("Sheer Force");
const u8 sText_Effect_Ignores_Ability[] = _("Ign. Ability");
const u8 sText_Effect_Ignores_Stats[] = _("Ign. Stats");
/*
const u8 gText_MoveInfo_Split[]    = _("Split: {STR_VAR_1}");
const u8 gText_MoveInfo_STAB[]     = _("STAB: {STR_VAR_1}");

const u8 gText_Split_Physical[] = _("Physical");
const u8 gText_Split_Special[]  = _("Special");
const u8 gText_Split_Status[]   = _("Status");

const u8 gText_MoveInfo_Target1[] = _("Target 1: {STR_VAR_1}");
const u8 gText_MoveInfo_Target2[] = _("Target 2: {STR_VAR_1}");

const u8 gText_MoveInfo_Damage[] = _("Damage");

const u8 gText_Target_Nothing[]   = _("---");
const u8 sText_Title_Controllers_Move[]      = _("{DPAD_UPDOWN}Switch {DPAD_LEFTRIGHT}Page {A_BUTTON}Mode");*/
const u8 sText_Effect_DamageDone[] = _("Calculated Damage\nRange: {STR_VAR_1}% - {STR_VAR_2}%\nof {STR_VAR_3}\nCurrent Health.");
const u8 sText_Effect_DamageDone_Guaranteed_KO[] = _("{COLOR 14}Guaranteed {COLOR 2}to KO\n{STR_VAR_3}\nin the next hit.");
const u8 sText_Effect_DamageDone_Guaranteed_KO_White[] = _("{COLOR 14}Guaranteed {COLOR 10}to KO\n{STR_VAR_3}\nin the next hit.");
const u8 sText_Effect_DamageDone_Immune[] = _("{STR_VAR_3}\nis immune to\n{STR_VAR_1}.");
const u8 sText_Effect_Speed_Calculation_Singles[] = _("{STR_VAR_3}\n{STR_VAR_1} Spd: {STR_VAR_2}");
const u8 sText_Effect_Speed_Calculation_Priority2[] = _("{STR_VAR_3}\n{STR_VAR_1} Spd: {STR_VAR_2}{PLUS}");
const u8 sText_Effect_Speed_Calculation[] = _("{STR_VAR_1}");
const u8 sText_Effect_Speed_CalculationPriority[] = _("{PLUS} {STR_VAR_1}");

enum {
    MOVE_EFFECTIVENESS_NONE,
    MOVE_EFFECTIVENESS_HALF,
    MOVE_EFFECTIVENESS_NORMAL,
    MOVE_EFFECTIVENESS_DOUBLE,
    MOVE_EFFECTIVENESS_STATUS,
};

#define NEGATIVE_MOVE_X 2
#define MAX_DAMAGE_FACTOR 0
#define MIN_DAMAGE_FACTOR 16
#define MAX_PERCENT 100
#define MAX_PERCENT_2 10000

static bool8 HasPriorityMove(u8 battler, int target) {
    u8 i;
    for (i = 0; i < MAX_MON_MOVES; i++) {
        if (gBattleMons[battler].moves[i] == MOVE_NONE)
            continue;
        else if (GetMovePriority(battler, gBattleMons[battler].moves[i], target) > 0)
            return TRUE;
    }
    return FALSE;
}

#define MOVE_INFO_WINDOW_SIZE 80

static const u8 sTheme_Classic_Can_KO_Mark_Gfx[] = INCBIN_U8("graphics/ui_menus/battle_interface/classic_theme/check.4bpp");
static const u8 sTheme_DPPt_Can_KO_Mark_Gfx[] = INCBIN_U8("graphics/ui_menus/battle_interface/dppt_theme/check.4bpp");
static const u8 sTheme_Dark_Can_KO_Mark_Gfx[] = INCBIN_U8("graphics/ui_menus/battle_interface/theme_1_dark/check.4bpp");
static const u8 sTheme_Light_Can_KO_Mark_Gfx[] = INCBIN_U8("graphics/ui_menus/battle_interface/theme_1_light/check.4bpp");
void PrintBattleWindow_MoveSelection(void) {
    struct ChooseMoveStruct *moveInfo = (struct ChooseMoveStruct *)(&gBattleResources->bufferA[gActiveBattler][MAX_MON_MOVES]);
    u8 i, x, y, x2, y2, offset, moveType, effectiveness, extraX;
    u16 move, movePower;
    u8 windowId = B_WIN_ACTION_PROMPT;
    u8 font = FONT_SMALL_NARROW;
    u8 fontColor = FONT_WHITE_2;
    u8 moveInfoType = VarGet(VAR_BATTLE_CONTROLLER_MOVE_WINDOW);
    u8 target = gMultiUsePlayerCursor;
    u8 battleTheme = getBattleInterfaceTheme();
    int maxDamage;
    bool8 isStatusMove;
    bool8 isDoubleTypedMove = FALSE;
    u16 typeEffectivenessMultiplier;
    int ignored, immune;

    switch (battleTheme) {
        case THEME_DARK:
            fontColor = FONT_WHITE_2;
            break;
        case THEME_LIGHT:
            fontColor = FONT_BLACK_2;
            break;
        case THEME_DPPT:
            fontColor = FONT_BLACK_2;
            break;
        case THEME_CLASSIC:
            fontColor = FONT_BLACK_2;
            break;
    }

    if (target >= MAX_BATTLERS_COUNT) target = BATTLE_OPPOSITE(gActiveBattler);

    if (!IsBattlerAlive(target)) target = BATTLE_PARTNER(target);

    // Fill the window with the fill value
    MoveIntoBattleBgWindow(windowId);
    FillWindowPixelBuffer(windowId, PIXEL_FILL(TEXT_COLOR_TRANSPARENT));

    gBattle_BG1_X = 0;
    gBattle_BG1_Y = 0;

    x = 2;
    y = 0;
    y2 = 4;
    x2 = 0;

    // Move Selection
    for (i = 0; i < MAX_MON_MOVES; i++) {
        move = moveInfo->moves[i];
        if (move == MOVE_NONE) break;
        StringCopy(gStringVar1, gMoveNamesLong[move]);
        AddTextPrinterParameterized4(windowId, font, (x * 8) + x2 - NEGATIVE_MOVE_X, (y * 8) + y2, 0, 0, sMenuWindowFontColors[fontColor], 0xFF, gStringVar1);
        // PP
        ConvertIntToDecimalStringN(gStringVar1, gBattleMons[gActiveBattler].pp[i], STR_CONV_MODE_LEFT_ALIGN, 3);   // Current PP
        ConvertIntToDecimalStringN(gStringVar2, CalculatePPWithBonus(move, 255, 0), STR_CONV_MODE_LEFT_ALIGN, 3);  // Max PP
        StringExpandPlaceholders(gStringVar4, sText_PP);
        AddTextPrinterParameterized4(windowId,
                                     font,
                                     (x * 8) + SPACE_BETWEEN_MOVE_NAME_AND_PP - NEGATIVE_MOVE_X,
                                     (y * 8) + y2,
                                     0,
                                     0,
                                     sMenuWindowFontColors[fontColor],
                                     0xFF,
                                     gStringVar4);

        if (gMoveSelectionCursor[gActiveBattler] == i) {
            switch (battleTheme) {
                case THEME_DARK:
                    BlitBitmapToWindow(windowId, sTheme_Dark_BattleMoveSelector, ((x - 1) * 8) - 1, (y * 8) + y2 + 4, 8, 8);
                    break;
                case THEME_LIGHT:
                    BlitBitmapToWindow(windowId, sTheme_Light_BattleMoveSelector, ((x - 1) * 8) - 1, (y * 8) + y2 + 4, 8, 8);
                    break;
                case THEME_DPPT:
                    BlitBitmapToWindow(windowId, sTheme_DPPt_BattleMoveSelector, ((x - 1) * 8) - 1, (y * 8) + y2 + 4, 8, 8);
                    break;
                case THEME_CLASSIC:
                    BlitBitmapToWindow(windowId, sTheme_Classic_BattleMoveSelector, ((x - 1) * 8) - 1, (y * 8) + y2 + 4, 8, 8);
                    break;
            }
        }

        y++;
    }

    // Move Info
    x = 1;
    y = 0;
    x2 = 4;
    y2 = 0;
    switch (battleTheme) {
        case THEME_DARK:
            BlitBitmapToWindow(windowId, sTheme_Dark_Title_Move_Name, (x * 8) + x2, (y * 8) + y2, 24, 8);
            x = x + 10;
            BlitBitmapToWindow(windowId, sTheme_Dark_Title_Move_PP, (x * 8) + x2, (y * 8) + y2, 16, 8);
            break;
        case THEME_LIGHT:
            BlitBitmapToWindow(windowId, sTheme_Light_Title_Move_Name, (x * 8) + x2, (y * 8) + y2, 24, 8);
            x = x + 10;
            BlitBitmapToWindow(windowId, sTheme_Light_Title_Move_PP, (x * 8) + x2, (y * 8) + y2, 16, 8);
            break;
        case THEME_DPPT:
            BlitBitmapToWindow(windowId, sTheme_DPPt_Title_Move_Name, (x * 8) + x2, (y * 8) + y2, 24, 8);
            x = x + 10;
            BlitBitmapToWindow(windowId, sTheme_DPPt_Title_Move_PP, (x * 8) + x2, (y * 8) + y2, 16, 8);
            break;
        case THEME_CLASSIC:
            BlitBitmapToWindow(windowId, sTheme_Classic_Title_Move_Name, (x * 8) + x2, (y * 8) + y2, 24, 8);
            x = x + 10;
            BlitBitmapToWindow(windowId, sTheme_Classic_Title_Move_PP, (x * 8) + x2, (y * 8) + y2, 16, 8);
            break;
    }

    // Title
    x = 17;
    y = 0;
    x2 = 2;
    y2 = 0;
    switch (moveInfoType) {
        // case MOVE_INFO_DESCRIPTION:
        //     if (battleTheme == THEME_DARK) {
        //         BlitBitmapToWindow(windowId, sTheme_Dark_Title_Move_Description, (x * 8) + x2, (y * 8) + y2, 56, 8);
        //     } else if (battleTheme == THEME_LIGHT) {
        //         BlitBitmapToWindow(windowId, sTheme_Light_Title_Move_Description, (x * 8) + x2, (y * 8) + y2, 56, 8);

        //     } else if (battleTheme == THEME_DPPT) {
        //         BlitBitmapToWindow(windowId, sTheme_DPPt_Title_Move_Description, (x * 8) + x2, (y * 8) + y2, 56, 8);
        //     } else if (battleTheme == THEME_CLASSIC) {
        //         BlitBitmapToWindow(windowId, sTheme_Classic_Title_Move_Description, (x * 8) + x2, (y * 8) + y2, 56, 8);
        //     }
        //     break;
        case MOVE_INFO_POWER_ACC_PRIO_TYPE:
        case MOVE_INFO_POWER_ACC_PRIO_TYPE_2:
            if (battleTheme == THEME_DARK)
                BlitBitmapToWindow(windowId, sTheme_Dark_Title_Move_Info, (x * 8) + x2, (y * 8) + y2, 56, 8);
            else if (battleTheme == THEME_LIGHT)
                BlitBitmapToWindow(windowId, sTheme_Light_Title_Move_Info, (x * 8) + x2, (y * 8) + y2, 56, 8);
            else if (battleTheme == THEME_DPPT)
                BlitBitmapToWindow(windowId, sTheme_DPPt_Title_Move_Info, (x * 8) + x2, (y * 8) + y2, 56, 8);
            else if (battleTheme == THEME_CLASSIC)
                BlitBitmapToWindow(windowId, sTheme_Classic_Title_Move_Info, (x * 8) + x2, (y * 8) + y2, 56, 8);
            break;
        case MOVE_SPEED_CALCULATION:
            if (battleTheme == THEME_DARK)
                BlitBitmapToWindow(windowId, sTheme_Dark_Title_Speed_Order, (x * 8) + x2, (y * 8) + y2, 56, 8);
            else if (battleTheme == THEME_LIGHT)
                BlitBitmapToWindow(windowId, sTheme_Light_Title_Speed_Order, (x * 8) + x2, (y * 8) + y2, 56, 8);
            else if (battleTheme == THEME_DPPT)
                BlitBitmapToWindow(windowId, sTheme_DPPt_Title_Speed_Order, (x * 8) + x2, (y * 8) + y2, 56, 8);
            else if (battleTheme == THEME_CLASSIC)
                BlitBitmapToWindow(windowId, sTheme_Classic_Title_Speed_Order, (x * 8) + x2, (y * 8) + y2, 56, 8);
            break;
        case MOVE_INFO_DAMAGE_CALCULATION:
            if (battleTheme == THEME_DARK)
                BlitBitmapToWindow(windowId, sTheme_Dark_Title_Dmg_Calculation, (x * 8) + x2, (y * 8) + y2, 56, 8);
            else if (battleTheme == THEME_LIGHT)
                BlitBitmapToWindow(windowId, sTheme_Light_Title_Dmg_Calculation, (x * 8) + x2, (y * 8) + y2, 56, 8);
            else if (battleTheme == THEME_DPPT)
                BlitBitmapToWindow(windowId, sTheme_DPPt_Title_Dmg_Calculation, (x * 8) + x2, (y * 8) + y2, 56, 8);
            else if (battleTheme == THEME_CLASSIC)
                BlitBitmapToWindow(windowId, sTheme_Classic_Title_Dmg_Calculation, (x * 8) + x2, (y * 8) + y2, 56, 8);
            break;
        case MOVE_INFO_DAMAGE_ENEMY:
            if (battleTheme == THEME_DARK)
                BlitBitmapToWindow(windowId, sTheme_Dark_Title_Enemy_Dmg, (x * 8) + x2, (y * 8) + y2, 96, 8);
            else if (battleTheme == THEME_LIGHT)
                BlitBitmapToWindow(windowId, sTheme_Light_Title_Enemy_Dmg, (x * 8) + x2, (y * 8) + y2, 56, 8);
            else if (battleTheme == THEME_DPPT)
                BlitBitmapToWindow(windowId, sTheme_DPPt_Title_Enemy_Dmg, (x * 8) + x2, (y * 8) + y2, 56, 8);
            else if (battleTheme == THEME_CLASSIC)
                BlitBitmapToWindow(windowId, sTheme_Classic_Title_Enemy_Dmg, (x * 8) + x2, (y * 8) + y2, 56, 8);
            break;
    }

    // Target
    switch (battleTheme) {
        case THEME_DARK:
            if (IsDoubleBattle()) {
                if (target == 3) {
                    BlitBitmapToWindow(windowId, sTheme_Dark_Target_0, (x * 8) + x2 + 64, (y * 8) + y2, 8, 8);
                    BlitBitmapToWindow(windowId, sTheme_Dark_Target_1, (x * 8) + x2 + 64 + 8, (y * 8) + y2, 8, 8);
                } else if (target == 1) {
                    BlitBitmapToWindow(windowId, sTheme_Dark_Target_1, (x * 8) + x2 + 64, (y * 8) + y2, 8, 8);
                    BlitBitmapToWindow(windowId, sTheme_Dark_Target_0, (x * 8) + x2 + 64 + 8, (y * 8) + y2, 8, 8);
                }
            }
            break;
        case THEME_LIGHT:
            if (IsDoubleBattle()) {
                if (target == 3) {
                    BlitBitmapToWindow(windowId, sTheme_Light_Target_0, (x * 8) + x2 + 64, (y * 8) + y2, 8, 8);
                    BlitBitmapToWindow(windowId, sTheme_Light_Target_1, (x * 8) + x2 + 64 + 4, (y * 8) + y2, 8, 8);
                } else if (target == 1) {
                    BlitBitmapToWindow(windowId, sTheme_Light_Target_1, (x * 8) + x2 + 64, (y * 8) + y2, 8, 8);
                    BlitBitmapToWindow(windowId, sTheme_Light_Target_0, (x * 8) + x2 + 64 + 8, (y * 8) + y2, 8, 8);
                }
            }
            break;
        case THEME_DPPT:
            if (IsDoubleBattle()) {
                if (target == 3) {
                    BlitBitmapToWindow(windowId, sTheme_DPPt_Target_0, (x * 8) + x2 + 64, (y * 8) + y2, 8, 8);
                    BlitBitmapToWindow(windowId, sTheme_DPPt_Target_1, (x * 8) + x2 + 64 + 8, (y * 8) + y2, 8, 8);
                } else if (target == 1) {
                    BlitBitmapToWindow(windowId, sTheme_DPPt_Target_1, (x * 8) + x2 + 64, (y * 8) + y2, 8, 8);
                    BlitBitmapToWindow(windowId, sTheme_DPPt_Target_0, (x * 8) + x2 + 64 + 8, (y * 8) + y2, 8, 8);
                }
            }
            break;
        case THEME_CLASSIC:
            if (IsDoubleBattle()) {
                if (target == 3) {
                    BlitBitmapToWindow(windowId, sTheme_Classic_Target_0, (x * 8) + x2 + 64, (y * 8) + y2, 8, 8);
                    BlitBitmapToWindow(windowId, sTheme_Classic_Target_1, (x * 8) + x2 + 64 + 8, (y * 8) + y2, 8, 8);
                } else if (target == 1) {
                    BlitBitmapToWindow(windowId, sTheme_Classic_Target_1, (x * 8) + x2 + 64, (y * 8) + y2, 8, 8);
                    BlitBitmapToWindow(windowId, sTheme_Classic_Target_0, (x * 8) + x2 + 64 + 8, (y * 8) + y2, 8, 8);
                }
            }
            break;
    }

    // Move Description
    x = 2;
    y = 0;
    y2 = 4;
    x2 = 0;

    move = moveInfo->moves[gMoveSelectionCursor[gActiveBattler]];
    movePower = gBattleMoves[move].power;
    isStatusMove = gBattleMoves[move].split == SPLIT_STATUS;
    moveType = gBattleMoves[move].type;
    gBattlerAttacker = gActiveBattler;
    gHitMarker |= (ShouldSetMoldBreaker(gActiveBattler, move) && HITMARKER_MOLD_BREAKER);
    SetTypeBeforeUsingMove(move, gActiveBattler);
    GET_MOVE_TYPE(move, moveType);
    maxDamage = DoMoveDamageCalcBattleMenu(move, gActiveBattler, target, &moveType, FALSE, MAX_DAMAGE_FACTOR, &typeEffectivenessMultiplier);
    immune = !typeEffectivenessMultiplier || TestAbsorbingAbilitiesOnly(target, gActiveBattler, move, moveType) ||
             TestImmunityAbilitiesOnly(target, gActiveBattler, move, moveType);
    x2 = SPACE_BETWEEN_MOVE_NAME_AND_DESCRIPTION;  // Default

    if (gBattleMoves[move].type != gBattleMoves[move].type2 && gBattleMoves[move].type2 != TYPE_NORMAL && gBattleMoves[move].type2 != TYPE_NONE)
        isDoubleTypedMove = TRUE;

    switch (moveInfoType) {
        // case MOVE_INFO_DESCRIPTION:
        //     StringCopy(gStringVar4, gMoveFourLineDescriptionPointers[move - 1]);
        //     AddTextPrinterParameterized4(windowId, font, (x * 8) + x2, (y * 8) + y2, 0, 0, sMenuWindowFontColors[fontColor], 0xFF, gStringVar4);
        //     break;
        case MOVE_INFO_POWER_ACC_PRIO_TYPE:
            if (isDoubleTypedMove)
                extraX = 8;
            else
                extraX = 0;
            x2 = SPACE_BETWEEN_MOVE_NAME_AND_DESCRIPTION + 4;
            // Move Power
            if (!isStatusMove)
                ConvertIntToDecimalStringN(gStringVar1, movePower, STR_CONV_MODE_LEFT_ALIGN, 3);
            else
                StringCopy(gStringVar1, sText_Target_Nothing);

            AddTextPrinterParameterized4(
                windowId, font, (x * 8) + x2 - (extraX / 2), (y * 8) + y2, 0, 0, sMenuWindowFontColors[fontColor], 0xFF, sText_MoveInfo_Power);
            offset = MOVE_INFO_WINDOW_SIZE - GetStringWidth(font, gStringVar1, 0xFF) + extraX;
            AddTextPrinterParameterized4(windowId, font, (x * 8) + x2 + offset, (y * 8) + y2, 0, 0, sMenuWindowFontColors[fontColor], 0xFF, gStringVar1);

            // Split
            x2 = MOVE_SPLIT_X;
            y2 = y2 + 4;

            switch (gBattleMoves[move].split) {
                case SPLIT_PHYSICAL:
                    if (battleTheme == THEME_DARK)
                        BlitBitmapToWindow(windowId, sTheme_Dark_Split_Physical, (x * 8) + x2, (y * 8) + y2, 16, 8);
                    else if (battleTheme == THEME_LIGHT)
                        BlitBitmapToWindow(windowId, sTheme_Light_Split_Physical, (x * 8) + x2, (y * 8) + y2, 16, 8);
                    else if (battleTheme == THEME_DPPT)
                        BlitBitmapToWindow(windowId, sTheme_DPPt_Split_Physical, (x * 8) + x2, (y * 8) + y2, 16, 8);
                    else if (battleTheme == THEME_CLASSIC)
                        BlitBitmapToWindow(windowId, sTheme_Classic_Split_Physical, (x * 8) + x2, (y * 8) + y2, 16, 8);
                    else
                        BlitBitmapToWindow(windowId, sSplit_Physical, (x * 8) + x2, (y * 8) + y2, 16, 8);
                    break;
                case SPLIT_SPECIAL:
                    if (battleTheme == THEME_DARK)
                        BlitBitmapToWindow(windowId, sTheme_Dark_Split_Special, (x * 8) + x2, (y * 8) + y2, 16, 8);
                    else if (battleTheme == THEME_LIGHT)
                        BlitBitmapToWindow(windowId, sTheme_Light_Split_Special, (x * 8) + x2, (y * 8) + y2, 16, 8);
                    else if (battleTheme == THEME_DPPT)
                        BlitBitmapToWindow(windowId, sTheme_DPPt_Split_Special, (x * 8) + x2, (y * 8) + y2, 16, 8);
                    else if (battleTheme == THEME_CLASSIC)
                        BlitBitmapToWindow(windowId, sTheme_Classic_Split_Special, (x * 8) + x2, (y * 8) + y2, 16, 8);
                    else
                        BlitBitmapToWindow(windowId, sSplit_Special, (x * 8) + x2, (y * 8) + y2, 16, 8);
                    break;
                case SPLIT_STATUS:
                    if (battleTheme == THEME_DARK)
                        BlitBitmapToWindow(windowId, sTheme_Dark_Split_Status, (x * 8) + x2, (y * 8) + y2, 16, 8);
                    else if (battleTheme == THEME_LIGHT)
                        BlitBitmapToWindow(windowId, sTheme_Light_Split_Status, (x * 8) + x2, (y * 8) + y2, 16, 8);
                    else if (battleTheme == THEME_DPPT)
                        BlitBitmapToWindow(windowId, sTheme_DPPt_Split_Status, (x * 8) + x2, (y * 8) + y2, 16, 8);
                    else if (battleTheme == THEME_CLASSIC)
                        BlitBitmapToWindow(windowId, sTheme_Classic_Split_Status, (x * 8) + x2, (y * 8) + y2, 16, 8);
                    else
                        BlitBitmapToWindow(windowId, sSplit_Status, (x * 8) + x2, (y * 8) + y2, 16, 8);
                    break;
            }
            y2 = y2 - 4;

            y++;
            x2 = SPACE_BETWEEN_MOVE_NAME_AND_DESCRIPTION + 4;

            // Move Accuracy
            ConvertIntToDecimalStringN(gStringVar1, gBattleMoves[move].accuracy, STR_CONV_MODE_LEFT_ALIGN, 3);
            AddTextPrinterParameterized4(
                windowId, font, (x * 8) + x2 - (extraX / 2), (y * 8) + y2, 0, 0, sMenuWindowFontColors[fontColor], 0xFF, sText_MoveInfo_Accuracy);
            offset = MOVE_INFO_WINDOW_SIZE - GetStringWidth(font, gStringVar1, 0xFF) + extraX;
            AddTextPrinterParameterized4(windowId, font, (x * 8) + x2 + offset, (y * 8) + y2, 0, 0, sMenuWindowFontColors[fontColor], 0xFF, gStringVar1);
            y++;
            // Move Priority
            ConvertIntToDecimalStringN(gStringVar1, gBattleMoves[move].priority, STR_CONV_MODE_LEFT_ALIGN, 3);
            offset = MOVE_INFO_WINDOW_SIZE - GetStringWidth(font, gStringVar1, 0xFF) + extraX;
            AddTextPrinterParameterized4(
                windowId, font, (x * 8) + x2 - (extraX / 2), (y * 8) + y2, 0, 0, sMenuWindowFontColors[fontColor], 0xFF, sText_MoveInfo_Priority);
            AddTextPrinterParameterized4(windowId, font, (x * 8) + x2 + offset, (y * 8) + y2, 0, 0, sMenuWindowFontColors[fontColor], 0xFF, gStringVar1);
            y++;
            // Move Type
            AddTextPrinterParameterized4(
                windowId, font, (x * 8) + x2 - (extraX / 2), (y * 8) + y2, 0, 0, sMenuWindowFontColors[fontColor], 0xFF, sText_MoveInfo_Type_Null);
            StringCopy(gStringVar1, gTypeNames[moveType]);
            // Stab
            if (StabMultiplierInHalves(gActiveBattler, moveType, move) > 2)
                StringCopy(gStringVar2, gStabIcon);
            else
                StringCopy(gStringVar2, gNoStabIcon);

            if (immune)
                effectiveness = MOVE_EFFECTIVENESS_NONE;
            else
                effectiveness = GetMoveTypeEffectiveness(move, target, gActiveBattler, moveType, typeEffectivenessMultiplier);

            switch (battleTheme) {
                case THEME_DARK:
                    if (effectiveness == MOVE_EFFECTIVENESS_NONE)
                        fontColor = FONT_GRAY_2;
                    else if (effectiveness == MOVE_EFFECTIVENESS_DOUBLE)
                        fontColor = FONT_GREEN;
                    else if (effectiveness == MOVE_EFFECTIVENESS_HALF)
                        fontColor = FONT_RED;
                    else
                        fontColor = FONT_WHITE_2;
                    break;
                case THEME_LIGHT:
                case THEME_DPPT:
                case THEME_CLASSIC:
                    if (effectiveness == MOVE_EFFECTIVENESS_NONE)
                        fontColor = FONT_GRAY;
                    else if (effectiveness == MOVE_EFFECTIVENESS_DOUBLE)
                        fontColor = FONT_GREEN;
                    else if (effectiveness == MOVE_EFFECTIVENESS_HALF)
                        fontColor = FONT_RED;
                    else
                        fontColor = FONT_BLACK_2;
                    break;
            }

            StringExpandPlaceholders(gStringVar4, sText_MoveInfo_Type);

            offset = MOVE_INFO_WINDOW_SIZE - GetStringWidth(font, gStringVar4, 0xFF) + extraX;
            AddTextPrinterParameterized4(windowId, font, (x * 8) + x2 + offset, (y * 8) + y2, 0, 0, sMenuWindowFontColors[fontColor], 0xFF, gStringVar4);
            break;
        case MOVE_INFO_POWER_ACC_PRIO_TYPE_2:
            x2 = SPACE_BETWEEN_MOVE_NAME_AND_DESCRIPTION + 4;

            // Move Power
            movePower = CalcMoveBasePowerAfterModifiers(move, 0, gActiveBattler, target, moveType, FALSE);
            if (movePower) {
                u16 multiplier =
                    CalculateAbilityMultipliers(gActiveBattler, target, move, moveType, movePower, typeEffectivenessMultiplier, FALSE, (u16 *)&ignored);
                MulModifier(&multiplier, typeEffectivenessMultiplier);
                movePower *= StabMultiplierInHalves(gActiveBattler, moveType, move);
                movePower /= 2;
                movePower = ApplyModifier(multiplier, movePower);
            }
            if (!isStatusMove)
                ConvertIntToDecimalStringN(gStringVar1, movePower, STR_CONV_MODE_LEFT_ALIGN, 3);
            else
                StringCopy(gStringVar1, sText_Target_Nothing);
            AddTextPrinterParameterized4(windowId, font, (x * 8) + x2, (y * 8) + y2, 0, 0, sMenuWindowFontColors[fontColor], 0xFF, sText_MoveInfo_True_Power);

            offset = MOVE_INFO_WINDOW_SIZE - GetStringWidth(font, gStringVar1, 0xFF);
            AddTextPrinterParameterized4(windowId, font, (x * 8) + x2 + offset, (y * 8) + y2, 0, 0, sMenuWindowFontColors[fontColor], 0xFF, gStringVar1);
            y++;

            // Boost Type
            if ((gBattleMoves[move].flags & FLAG_IRON_FIST_BOOST))
                StringCopy(gStringVar1, sText_Effect_Boost_Type_Fist);
            else if ((gBattleMoves[move].flags & FLAG_STRIKER_BOOST))
                StringCopy(gStringVar1, sText_Effect_Boost_Type_Kick);
            else if ((gBattleMoves[move].flags & FLAG_BONE_BASED))
                StringCopy(gStringVar1, sText_Effect_Boost_Type_Bone);
            else if ((gBattleMoves[move].flags & FLAG_MEGA_LAUNCHER_BOOST))
                StringCopy(gStringVar1, sText_Effect_Boost_Type_Cannon);
            else if ((gBattleMoves[move].flags & FLAG_DANCE))
                StringCopy(gStringVar1, sText_Effect_Boost_Type_Dance);
            else if ((gBattleMoves[move].flags & FLAG_KEEN_EDGE_BOOST))
                StringCopy(gStringVar1, sText_Effect_Boost_Type_Slash);
            else if ((gBattleMoves[move].flags & FLAG_STRONG_JAW_BOOST))
                StringCopy(gStringVar1, sText_Effect_Boost_Type_Bite);
            else if (gBattleMoves[move].airBased)
                StringCopy(gStringVar1, sText_Effect_Boost_Type_Air);
            else if (gBattleMoves[move].hornBased)
                StringCopy(gStringVar1, sText_Effect_Boost_Type_Horn);
            else if (gBattleMoves[move].hammerBased)
                StringCopy(gStringVar1, sText_Effect_Boost_Type_Hammer);
            else if (gBattleMoves[move].arrowBased)
                StringCopy(gStringVar1, sText_Effect_Boost_Type_Arrow);
            else if (gBattleMoves[move].throwingBased)
                StringCopy(gStringVar1, sText_Effect_Boost_Type_Throw);
            else if ((gBattleMoves[move].flags & FLAG_RECKLESS_BOOST))
                StringCopy(gStringVar1, sText_Effect_Boost_Type_Reckless);
            else
                StringCopy(gStringVar1, sText_Target_Nothing);
            AddTextPrinterParameterized4(windowId, font, (x * 8) + x2, (y * 8) + y2, 0, 0, sMenuWindowFontColors[fontColor], 0xFF, sText_MoveInfo_Boost);
            offset = MOVE_INFO_WINDOW_SIZE - GetStringWidth(font, gStringVar1, 0xFF);
            AddTextPrinterParameterized4(windowId, font, (x * 8) + x2 + offset, (y * 8) + y2, 0, 0, sMenuWindowFontColors[fontColor], 0xFF, gStringVar1);
            y++;

            // Based
            if ((gBattleMoves[move].flags & FLAG_WEATHER_BASED))
                StringCopy(gStringVar1, sText_Effect_Boost_Type_Weather);
            else if ((gBattleMoves[move].flags & FLAG_FIELD_BASED))
                StringCopy(gStringVar1, sText_Effect_Boost_Type_Field);
            else if ((gBattleMoves[move].flags & FLAG_BONE_BASED))
                StringCopy(gStringVar1, sText_Effect_Boost_Type_Bone);
            else if ((gBattleMoves[move].flags & FLAG_SOUND))
                StringCopy(gStringVar1, sText_Effect_Boost_Type_Sound);
            else
                StringCopy(gStringVar1, sText_Target_Nothing);
            AddTextPrinterParameterized4(windowId, font, (x * 8) + x2, (y * 8) + y2, 0, 0, sMenuWindowFontColors[fontColor], 0xFF, sText_Effect_Based_Type);
            offset = MOVE_INFO_WINDOW_SIZE - GetStringWidth(font, gStringVar1, 0xFF);
            AddTextPrinterParameterized4(windowId, font, (x * 8) + x2 + offset, (y * 8) + y2, 0, 0, sMenuWindowFontColors[fontColor], 0xFF, gStringVar1);
            y++;

            // Contact
            if (gBattleMoves[move].flags & FLAG_MAKES_CONTACT)
                StringCopy(gStringVar1, sText_Yes);
            else
                StringCopy(gStringVar1, sText_No);
            AddTextPrinterParameterized4(windowId, font, (x * 8) + x2, (y * 8) + y2, 0, 0, sMenuWindowFontColors[fontColor], 0xFF, sText_MoveInfo_Contact);
            offset = MOVE_INFO_WINDOW_SIZE - GetStringWidth(font, gStringVar1, 0xFF);
            AddTextPrinterParameterized4(windowId, font, (x * 8) + x2 + offset, (y * 8) + y2, 0, 0, sMenuWindowFontColors[fontColor], 0xFF, gStringVar1);
            y++;

            // Chance
            /*ConvertIntToDecimalStringN(gStringVar1, gBattleMoves[move].secondaryEffectChance, STR_CONV_MODE_LEFT_ALIGN, 3);
            StringExpandPlaceholders(gStringVar4, sText_MoveInfo_Chance);
            AddTextPrinterParameterized4(windowId, font, (x * 8) + x2, (y * 8) + y2, 0, 0, sMenuWindowFontColors[fontColor], 0xFF, gStringVar4);
            y++;*/
            break;
        case MOVE_SPEED_CALCULATION: {
            u16 battlertoCheck, monSpeed;
            u8 sBattlerByTurnOrder[gBattlersCount];
            SortBattlersBySpeed(sBattlerByTurnOrder, FALSE);

            for (i = 0; i < gBattlersCount; i++) {
                battlertoCheck = sBattlerByTurnOrder[i];
                if (IsBattlerAlive(battlertoCheck)) {
                    StringCopy(gStringVar1, gSpeciesNames[gBattleMons[battlertoCheck].species]);
                    AddTextPrinterParameterized4(windowId, font, (x * 8) + x2, (y * 8) + y2, 0, 0, sMenuWindowFontColors[fontColor], 0xFF, gStringVar1);

                    monSpeed = GetBattlerTotalSpeedStat(battlertoCheck, TOTAL_SPEED_FULL);
                    ConvertIntToDecimalStringN(gStringVar1, monSpeed, STR_CONV_MODE_LEFT_ALIGN, 3);
                    if (HasPriorityMove(battlertoCheck, battlertoCheck == gActiveBattler ? target : BATTLE_OPPOSITE(battlertoCheck)))
                        StringExpandPlaceholders(gStringVar4, sText_Effect_Speed_CalculationPriority);
                    else
                        StringExpandPlaceholders(gStringVar4, sText_Effect_Speed_Calculation);

                    offset = MOVE_INFO_WINDOW_SIZE - GetStringWidth(font, gStringVar4, 0xFF);
                    AddTextPrinterParameterized4(
                        windowId, font, (x * 8) + x2 + offset, (y * 8) + y2, 0, 0, sMenuWindowFontColors[fontColor], 0xFF, gStringVar4);
                    y++;
                }
            }
        } break;
        case MOVE_INFO_DAMAGE_CALCULATION:
            if (!isStatusMove && move != MOVE_NONE) {
                s16 percentage;
                u16 targetCurrentHp = gBattleMons[target].hp;
                u16 minDamage = DoMoveDamageCalcBattleMenu(move, gActiveBattler, target, &moveType, FALSE, MIN_DAMAGE_FACTOR, &typeEffectivenessMultiplier);
                u16 heldItem = gBattleMons[target].item;

                if (immune) minDamage = maxDamage = 0;
                x2 = SPACE_BETWEEN_MOVE_NAME_AND_DESCRIPTION + 4;
                StringCopy(gStringVar3, gSpeciesNames[gBattleMons[target].species]);

                if (maxDamage == 0) {
                    StringCopy(gStringVar1, gMoveNamesLong[move]);
                    StringExpandPlaceholders(gStringVar4, sText_Effect_DamageDone_Immune);
                    AddTextPrinterParameterized4(windowId, font, (x * 8) + x2, (y * 8) + y2, 0, 0, sMenuWindowFontColors[fontColor], 0xFF, gStringVar4);
                } else if (targetCurrentHp > minDamage) {
                    // Min Damage Percentage
                    percentage = (minDamage * MAX_PERCENT_2) / targetCurrentHp;
                    percentage = (percentage / MAX_PERCENT);
                    if (percentage >= MAX_PERCENT) percentage = MAX_PERCENT;
                    ConvertIntToDecimalStringN(gStringVar1, percentage, STR_CONV_MODE_LEFT_ALIGN, 3);

                    // Max Damage Percentage
                    percentage = (maxDamage * MAX_PERCENT_2) / targetCurrentHp;
                    percentage = (percentage / MAX_PERCENT);
                    if (percentage > MAX_PERCENT) percentage = MAX_PERCENT;

                    // Focus Sash Check
                    if ((heldItem == ITEM_FOCUS_SASH || BattlerHasAbility(target, ABILITY_STURDY, TRUE)) && percentage == MAX_PERCENT &&
                        targetCurrentHp == gBattleMons[target].maxHP)
                        percentage = 99;

                    ConvertIntToDecimalStringN(gStringVar2, percentage, STR_CONV_MODE_LEFT_ALIGN, 3);

                    StringExpandPlaceholders(gStringVar4, sText_Effect_DamageDone);
                    AddTextPrinterParameterized4(windowId, font, (x * 8) + x2, (y * 8) + y2, 0, 0, sMenuWindowFontColors[fontColor], 0xFF, gStringVar4);
                } else {
                    if (fontColor == FONT_WHITE_2)
                        StringExpandPlaceholders(gStringVar4, sText_Effect_DamageDone_Guaranteed_KO_White);
                    else
                        StringExpandPlaceholders(gStringVar4, sText_Effect_DamageDone_Guaranteed_KO);
                    AddTextPrinterParameterized4(windowId, font, (x * 8) + x2, (y * 8) + y2, 0, 0, sMenuWindowFontColors[fontColor], 0xFF, gStringVar4);
                }
            } else {
                AddTextPrinterParameterized4(
                    windowId, font, (x * 8) + x2, (y * 8) + y2, 0, 0, sMenuWindowFontColors[fontColor], 0xFF, sText_DamageCalculation_Status);
            }
            break;
        case MOVE_INFO_DAMAGE_ENEMY:
            for (i = 0; i < MAX_MON_MOVES; i++) {
                u16 newMove = gBattleMons[target].moves[i];
                u16 priority = gBattleMoves[newMove].priority;
                u16 targetCurrentHp = gBattleMons[gActiveBattler].hp;
                u16 heldItem = gBattleMons[gActiveBattler].item;
                u16 moveDamage;

                if (newMove != MOVE_NONE) {
                    // Move Name
                    AddTextPrinterParameterized4(
                        windowId, font, (x * 8) + x2, ((y + i) * 8) + y2, 0, 0, sMenuWindowFontColors[fontColor], 0xFF, gMoveNames[newMove]);

                    // Move Priority
                    if (gBattleMoves[newMove].priority >= 0)
                        ConvertIntToDecimalStringN(gStringVar4, priority, STR_CONV_MODE_LEFT_ALIGN, 3);
                    else
                        ConvertIntToDecimalStringN(gStringVar4, 0, STR_CONV_MODE_LEFT_ALIGN, 3);

                    AddTextPrinterParameterized4(
                        windowId, font, ((x + 8) * 8) + x2, ((y + i) * 8) + y2, 0, 0, sMenuWindowFontColors[fontColor], 0xFF, gStringVar4);

                    // Can KO - Todo: Check calculation
                    if (gBattleMoves[newMove].split != SPLIT_STATUS && gBattleMoves[newMove].power > 0) {
                        u8 moveType = gBattleMoves[newMove].type;
                        if (ShouldSetMoldBreaker(target, newMove)) gHitMarker |= HITMARKER_MOLD_BREAKER;
                        SetTypeBeforeUsingMove(newMove, target);
                        GET_MOVE_TYPE(newMove, moveType);
                        moveDamage = CalculateMoveDamageAndEffectiveness(newMove, target, gActiveBattler, &moveType, &typeEffectivenessMultiplier);
                        if (!typeEffectivenessMultiplier || TestAbsorbingAbilitiesOnly(gActiveBattler, target, newMove, moveType) ||
                            TestImmunityAbilitiesOnly(gActiveBattler, target, newMove, moveType))
                            moveDamage = 0;
                        // gSwapDamageCategory = FALSE;

                        if (targetCurrentHp <= moveDamage && !(heldItem == ITEM_FOCUS_SASH && targetCurrentHp == gBattleMons[gActiveBattler].maxHP)) {
                            switch (battleTheme) {
                                case THEME_DARK:
                                    BlitBitmapToWindow(windowId, sTheme_Dark_Can_KO_Mark_Gfx, ((x + 10) * 8) + x2 + 4, ((y + i) * 8) + y2 + 4, 8, 8);
                                    break;
                                case THEME_LIGHT:
                                    BlitBitmapToWindow(windowId, sTheme_Light_Can_KO_Mark_Gfx, ((x + 9) * 8) + x2 + 4, ((y + i) * 8) + y2 + 4, 8, 8);
                                    break;
                                case THEME_DPPT:
                                    BlitBitmapToWindow(windowId, sTheme_DPPt_Can_KO_Mark_Gfx, ((x + 9) * 8) + x2 + 4, ((y + i) * 8) + y2 + 4, 8, 8);
                                    break;
                                case THEME_CLASSIC:
                                    BlitBitmapToWindow(windowId, sTheme_Classic_Can_KO_Mark_Gfx, ((x + 9) * 8) + x2 + 4, ((y + i) * 8) + y2 + 4, 8, 8);
                                    break;
                            }
                        }
                    }
                }
            }
            break;
    }

    PutWindowTilemap(windowId);
    CopyWindowToVram(windowId, 3);
}

enum { BATTLE_MENU_NONE, BATTLE_MENU_CHOSE_ACTION, BATTLE_MENU_CHOSE_MOVE, NUM_BATTLE_MENUS };

void ReshowNewBattleMenuAfterMenu(void) {
    if (VarGet(VAR_TEMP_SPECIAL_VAR) != 0xFF) gActiveBattler = VarGet(VAR_TEMP_SPECIAL_VAR);
    LoadBattleTextboxAndBackground();
    gBattle_BG0_Y = 160;
    VarSet(VAR_TEMP_SPECIAL_VAR, 0xFF);
    // SetControllerToPlayer();

    PrintBattleWindow_ActionPromt();
}

#define ENABLE_BATTLE_INPUT_GOING_BEYOND_SCREEN FALSE  // No idea what to call this constant

static void HandleInputChooseActionPlayer(void) {
    u16 itemId = gBattleResources->bufferA[gActiveBattler][2] | (gBattleResources->bufferA[gActiveBattler][3] << 8);
    u8 shortcutButton = gSaveBlock2Ptr->shortcutButton;

    DoBounceEffect(gActiveBattler, BOUNCE_HEALTHBOX, 7, 1);
    DoBounceEffect(gActiveBattler, BOUNCE_MON, 7, 1);

    if (JOY_REPEAT(DPAD_ANY) && gSaveBlock2Ptr->optionsButtonMode == OPTIONS_BUTTON_MODE_L_EQUALS_A)
        gPlayerDpadHoldFrames++;
    else
        gPlayerDpadHoldFrames = 0;

    if (JOY_NEW(A_BUTTON)) {
        switch (gActionSelectionCursor[gActiveBattler]) {
            case BATTLE_ACTION_FIGHT:
                PlaySE(SE_SELECT);
                BtlController_EmitTwoReturnValues(1, B_ACTION_USE_MOVE, 0);
                PrintBattleWindow_ActionPromt();
                PlayerBufferExecCompleted();
                break;
            case BATTLE_ACTION_DEBUG:
                if (B_ENABLE_DEBUG) {
                    PlaySE(SE_SELECT);
                    gBattle_BG1_Y = 160;
                    BtlController_EmitTwoReturnValues(1, B_ACTION_DEBUG, 0);
                    PlayerBufferExecCompleted();
                } else {
                    PlaySE(SE_BOO);
                }
                break;
            case BATTLE_ACTION_POKEMON:
                PlaySE(SE_SELECT);
                BtlController_EmitTwoReturnValues(1, B_ACTION_SWITCH, 0);
                PrintBattleWindow_ActionPromt();
                PlayerBufferExecCompleted();
                break;
            case BATTLE_ACTION_RUN:
                PlaySE(SE_SELECT);
                gBattle_BG1_Y = 160;
                BtlController_EmitTwoReturnValues(1, B_ACTION_RUN, 0);
                PrintBattleWindow_ActionPromt();
                PlayerBufferExecCompleted();
                break;
            case BATTLE_ACTION_INFO:
                PlaySE(SE_SELECT);
                FreeAllWindowBuffers();
                BeginNormalPaletteFade(0xFFFFFFFF, 0, 0, 0x10, RGB_BLACK);
                BtlController_EmitTwoReturnValues(1, B_ACTION_SHOW_BATTLE_INFO, 0);
                PrintBattleWindow_ActionPromt();
                PlayerBufferExecCompleted();
                break;
        }
    } else if (JOY_NEW(R_BUTTON)) {
        if (!(gBattleTypeFlags & (BATTLE_TYPE_TRAINER | BATTLE_TYPE_FRONTIER | BATTLE_TYPE_EREADER_TRAINER | BATTLE_TYPE_TRAINER_HILL)) &&
            CanThrowBall() == 0 && FlagGet(FLAG_SYS_DEXNAV_GET)) {
            gBattle_BG1_Y = 160;
            gLastThrownBall = gLastUsedItem = ITEM_POKE_BALL;
            BtlController_EmitTwoReturnValues(1, B_ACTION_THROW_BALL, 0);
            PlayerBufferExecCompleted();
        }
    } else if (JOY_NEW(L_BUTTON)) {
        if(shortcutButton == 0)
            shortcutButton = BATTLE_ACTION_WIKI;
        else
            shortcutButton--;
        
        switch (shortcutButton) {
            case BATTLE_ACTION_FIGHT:
                PlaySE(SE_SELECT);
                BtlController_EmitTwoReturnValues(1, B_ACTION_USE_MOVE, 0);
                PrintBattleWindow_ActionPromt();
                PlayerBufferExecCompleted();
                break;
            case BATTLE_ACTION_POKEMON:
                PlaySE(SE_SELECT);
                BtlController_EmitTwoReturnValues(1, B_ACTION_SWITCH, 0);
                PrintBattleWindow_ActionPromt();
                PlayerBufferExecCompleted();
                break;
            case BATTLE_ACTION_RUN:
                PlaySE(SE_SELECT);
                gBattle_BG1_Y = 160;
                BtlController_EmitTwoReturnValues(1, B_ACTION_RUN, 0);
                PrintBattleWindow_ActionPromt();
                PlayerBufferExecCompleted();
                break;
            case BATTLE_ACTION_INFO:
                PlaySE(SE_SELECT);
                FreeAllWindowBuffers();
                BeginNormalPaletteFade(0xFFFFFFFF, 0, 0, 0x10, RGB_BLACK);
                BtlController_EmitTwoReturnValues(1, B_ACTION_SHOW_BATTLE_INFO, 0);
                PrintBattleWindow_ActionPromt();
                PlayerBufferExecCompleted();
                break;
            case BATTLE_ACTION_DEBUG:
                if (B_ENABLE_DEBUG) {
                    PlaySE(SE_SELECT);
                    gBattle_BG1_Y = 160;
                    BtlController_EmitTwoReturnValues(1, B_ACTION_DEBUG, 0);
                    PlayerBufferExecCompleted();
                }
                break;
            case BATTLE_ACTION_WIKI:
                PlaySE(SE_SELECT);
                FreeAllWindowBuffers();
                BeginNormalPaletteFade(0xFFFFFFFF, 0, 0, 0x10, RGB_BLACK);
                BtlController_EmitTwoReturnValues(1, B_ACTION_SHOW_IN_GAME_WIKI, 0);
                PrintBattleWindow_ActionPromt();
                PlayerBufferExecCompleted();
                break;
        }
    } else if (JOY_NEW(B_BUTTON) || gPlayerDpadHoldFrames > 59) {
        if ((gBattleTypeFlags & BATTLE_TYPE_DOUBLE) && GetBattlerPosition(gActiveBattler) == B_POSITION_PLAYER_RIGHT &&
            !(gAbsentBattlerFlags & gBitTable[GetBattlerAtPosition(B_POSITION_PLAYER_LEFT)]) && !(gBattleTypeFlags & BATTLE_TYPE_MULTI)) {
            if (gBattleResources->bufferA[gActiveBattler][1] == B_ACTION_USE_ITEM) {
                // Add item to bag if it is a ball
                if (itemId <= LAST_BALL_INDEX)
                    AddBagItem(itemId, 1);
                else
                    return;
            }
            PlaySE(SE_SELECT);
            BtlController_EmitTwoReturnValues(1, B_ACTION_CANCEL_PARTNER, 0);
            PlayerBufferExecCompleted();
        }
    } else if (JOY_NEW(DPAD_RIGHT)) {
        PlaySE(SE_SELECT);
        switch (gActionSelectionCursor[gActiveBattler]) {
            case BATTLE_ACTION_FIGHT:
                gActionSelectionCursor[gActiveBattler] = BATTLE_ACTION_INFO;
                break;
            case BATTLE_ACTION_POKEMON:
                gActionSelectionCursor[gActiveBattler] = BATTLE_ACTION_RUN;
                break;
            case BATTLE_ACTION_INFO:
                if (ENABLE_BATTLE_INPUT_GOING_BEYOND_SCREEN) gActionSelectionCursor[gActiveBattler] = BATTLE_ACTION_FIGHT;
                break;
            case BATTLE_ACTION_RUN:
                if (ENABLE_BATTLE_INPUT_GOING_BEYOND_SCREEN) gActionSelectionCursor[gActiveBattler] = BATTLE_ACTION_POKEMON;
                break;
        }
        PrintBattleWindow_ActionPromt();
    } else if (JOY_NEW(DPAD_LEFT)) {
        PlaySE(SE_SELECT);
        switch (gActionSelectionCursor[gActiveBattler]) {
            case BATTLE_ACTION_FIGHT:
                if (ENABLE_BATTLE_INPUT_GOING_BEYOND_SCREEN) gActionSelectionCursor[gActiveBattler] = BATTLE_ACTION_INFO;
                break;
            case BATTLE_ACTION_POKEMON:
                if (ENABLE_BATTLE_INPUT_GOING_BEYOND_SCREEN) gActionSelectionCursor[gActiveBattler] = BATTLE_ACTION_RUN;
                break;
            case BATTLE_ACTION_INFO:
                gActionSelectionCursor[gActiveBattler] = BATTLE_ACTION_FIGHT;
                break;
            case BATTLE_ACTION_RUN:
                gActionSelectionCursor[gActiveBattler] = BATTLE_ACTION_POKEMON;
                break;
        }
        PrintBattleWindow_ActionPromt();
    } else if (JOY_NEW(DPAD_UP)) {
        PlaySE(SE_SELECT);
        switch (gActionSelectionCursor[gActiveBattler]) {
            case BATTLE_ACTION_FIGHT:
                if (ENABLE_BATTLE_INPUT_GOING_BEYOND_SCREEN) gActionSelectionCursor[gActiveBattler] = BATTLE_ACTION_POKEMON;
                break;
            case BATTLE_ACTION_POKEMON:
                gActionSelectionCursor[gActiveBattler] = BATTLE_ACTION_FIGHT;
                break;
            case BATTLE_ACTION_INFO:
                if (ENABLE_BATTLE_INPUT_GOING_BEYOND_SCREEN) gActionSelectionCursor[gActiveBattler] = BATTLE_ACTION_RUN;
                break;
            case BATTLE_ACTION_RUN:
                gActionSelectionCursor[gActiveBattler] = BATTLE_ACTION_INFO;
                break;
        }
        PrintBattleWindow_ActionPromt();
    } else if (JOY_NEW(DPAD_DOWN)) {
        PlaySE(SE_SELECT);
        switch (gActionSelectionCursor[gActiveBattler]) {
            case BATTLE_ACTION_FIGHT:
                gActionSelectionCursor[gActiveBattler] = BATTLE_ACTION_POKEMON;
                break;
            case BATTLE_ACTION_POKEMON:
                if (ENABLE_BATTLE_INPUT_GOING_BEYOND_SCREEN) gActionSelectionCursor[gActiveBattler] = BATTLE_ACTION_FIGHT;
                break;
            case BATTLE_ACTION_INFO:
                gActionSelectionCursor[gActiveBattler] = BATTLE_ACTION_RUN;
                break;
            case BATTLE_ACTION_RUN:
                if (ENABLE_BATTLE_INPUT_GOING_BEYOND_SCREEN) gActionSelectionCursor[gActiveBattler] = BATTLE_ACTION_INFO;
                break;
        }
        PrintBattleWindow_ActionPromt();
    } else if (JOY_NEW(START_BUTTON)) {
        SwapHpBarsWithHpText();
    } else if (B_ENABLE_DEBUG && gMain.newKeys & SELECT_BUTTON) {
        BtlController_EmitTwoReturnValues(1, B_ACTION_DEBUG, 0);
        PlayerBufferExecCompleted();
    }
}

void SetControllerToPlayer(void) {
    gBattlerControllerFuncs[gActiveBattler] = PlayerBufferRunCommand;
    gDoingBattleAnim = FALSE;
    gPlayerDpadHoldFrames = 0;
}

static void PlayerBufferExecCompleted(void) {
    gBattlerControllerFuncs[gActiveBattler] = PlayerBufferRunCommand;
    if (gBattleTypeFlags & BATTLE_TYPE_LINK) {
        u8 playerId = GetMultiplayerId();

        PrepareBufferDataTransferLink(2, 4, &playerId);
        gBattleResources->bufferA[gActiveBattler][0] = CONTROLLER_TERMINATOR_NOP;
    } else {
        gBattleControllerExecFlags &= ~gBitTable[gActiveBattler];
    }
}

static void PlayerBufferRunCommand(void) {
    if (gBattleControllerExecFlags & gBitTable[gActiveBattler]) {
        if (gBattleResources->bufferA[gActiveBattler][0] < ARRAY_COUNT(sPlayerBufferCommands))
            sPlayerBufferCommands[gBattleResources->bufferA[gActiveBattler][0]]();
        else
            PlayerBufferExecCompleted();
    }
}

static void CompleteOnBankSpritePosX_0(void) {
    if (gSprites[gBattlerSpriteIds[gActiveBattler]].x2 == 0) PlayerBufferExecCompleted();
}

static void UnusedEndBounceEffect(void) {
    EndBounceEffect(gActiveBattler, BOUNCE_HEALTHBOX);
    EndBounceEffect(gActiveBattler, BOUNCE_MON);
    gBattlerControllerFuncs[gActiveBattler] = HandleInputChooseTarget;
}

static void HandleInputChooseTarget(void) {
    s32 i;
    static const u8 identities[MAX_BATTLERS_COUNT] = {B_POSITION_PLAYER_LEFT, B_POSITION_PLAYER_RIGHT, B_POSITION_OPPONENT_RIGHT, B_POSITION_OPPONENT_LEFT};
    u16 move = GetMonData(&gPlayerParty[gBattlerPartyIndexes[gActiveBattler]], MON_DATA_MOVE1 + gMoveSelectionCursor[gActiveBattler]);
    u8 windowMode = VarGet(VAR_BATTLE_CONTROLLER_MOVE_WINDOW);

    DoBounceEffect(gMultiUsePlayerCursor, BOUNCE_HEALTHBOX, 15, 1);
    for (i = 0; i < gBattlersCount; i++) {
        if (i != gMultiUsePlayerCursor) EndBounceEffect(i, BOUNCE_HEALTHBOX);
    }

    if (JOY_HELD(DPAD_ANY) && gSaveBlock2Ptr->optionsButtonMode == OPTIONS_BUTTON_MODE_L_EQUALS_A)
        gPlayerDpadHoldFrames++;
    else
        gPlayerDpadHoldFrames = 0;

    if (JOY_NEW(A_BUTTON)) {
        PlaySE(SE_SELECT);
        gSprites[gBattlerSpriteIds[gMultiUsePlayerCursor]].callback = SpriteCb_HideAsMoveTarget;
        if (gBattleStruct->mega.playerSelect)
            BtlController_EmitTwoReturnValues(1, 10, gMoveSelectionCursor[gActiveBattler] | RET_MEGA_EVOLUTION | (gMultiUsePlayerCursor << 8));
        else
            BtlController_EmitTwoReturnValues(1, 10, gMoveSelectionCursor[gActiveBattler] | (gMultiUsePlayerCursor << 8));
        EndBounceEffect(gMultiUsePlayerCursor, BOUNCE_HEALTHBOX);
        // TryHideLastUsedBall();
        // TryToHideEnemyInfoWindow();
        // HideMegaTriggerSprite();
        PlayerBufferExecCompleted();
    } else if (JOY_NEW(B_BUTTON) || gPlayerDpadHoldFrames > 59) {
        PlaySE(SE_SELECT);
        PrintBattleWindow_MoveSelection();
        gSprites[gBattlerSpriteIds[gMultiUsePlayerCursor]].callback = SpriteCb_HideAsMoveTarget;
        gBattlerControllerFuncs[gActiveBattler] = HandleInputChooseMove;
        DoBounceEffect(gActiveBattler, BOUNCE_HEALTHBOX, 7, 1);
        DoBounceEffect(gActiveBattler, BOUNCE_MON, 7, 1);
        EndBounceEffect(gMultiUsePlayerCursor, BOUNCE_HEALTHBOX);
    } else if (JOY_NEW(DPAD_LEFT | DPAD_UP)) {
        PlaySE(SE_SELECT);
        gSprites[gBattlerSpriteIds[gMultiUsePlayerCursor]].callback = SpriteCb_HideAsMoveTarget;

        if (GetBattlerBattleMoveTargetFlags(move, gActiveBattler) == (MOVE_TARGET_USER | MOVE_TARGET_ALLY)) {
            gMultiUsePlayerCursor ^= BIT_FLANK;
        } else {
            do {
                u8 currSelIdentity = GetBattlerPosition(gMultiUsePlayerCursor);

                for (i = 0; i < MAX_BATTLERS_COUNT; i++) {
                    if (currSelIdentity == identities[i]) break;
                }
                do {
                    if (--i < 0) i = MAX_BATTLERS_COUNT - 1;
                    gMultiUsePlayerCursor = GetBattlerAtPosition(identities[i]);
                } while (gMultiUsePlayerCursor == gBattlersCount);

                i = 0;
                switch (GetBattlerPosition(gMultiUsePlayerCursor)) {
                    case B_POSITION_PLAYER_LEFT:
                    case B_POSITION_PLAYER_RIGHT:
                        if (gActiveBattler != gMultiUsePlayerCursor)
                            i++;
                        else if (GetBattlerBattleMoveTargetFlags(move, gActiveBattler) & MOVE_TARGET_USER_OR_SELECTED)
                            i++;
                        break;
                    case B_POSITION_OPPONENT_LEFT:
                    case B_POSITION_OPPONENT_RIGHT:
                        i++;
                        break;
                }
                MoveSelectionDisplayMoveTypeDoubles(GetBattlerPosition(gMultiUsePlayerCursor));

                if (gAbsentBattlerFlags & gBitTable[gMultiUsePlayerCursor]) i = 0;
            } while (i == 0);
        }
        gSprites[gBattlerSpriteIds[gMultiUsePlayerCursor]].callback = SpriteCb_ShowAsMoveTarget;
        PrintBattleWindow_MoveSelection();
    } else if (JOY_NEW(DPAD_RIGHT | DPAD_DOWN)) {
        PlaySE(SE_SELECT);
        gSprites[gBattlerSpriteIds[gMultiUsePlayerCursor]].callback = SpriteCb_HideAsMoveTarget;

        if (GetBattlerBattleMoveTargetFlags(move, gActiveBattler) == (MOVE_TARGET_USER | MOVE_TARGET_ALLY)) {
            gMultiUsePlayerCursor ^= BIT_FLANK;
        } else {
            do {
                u8 currSelIdentity = GetBattlerPosition(gMultiUsePlayerCursor);

                for (i = 0; i < MAX_BATTLERS_COUNT; i++) {
                    if (currSelIdentity == identities[i]) break;
                }
                do {
                    if (++i > 3) i = 0;
                    gMultiUsePlayerCursor = GetBattlerAtPosition(identities[i]);
                } while (gMultiUsePlayerCursor == gBattlersCount);

                i = 0;
                switch (GetBattlerPosition(gMultiUsePlayerCursor)) {
                    case B_POSITION_PLAYER_LEFT:
                    case B_POSITION_PLAYER_RIGHT:
                        if (gActiveBattler != gMultiUsePlayerCursor)
                            i++;
                        else if (GetBattlerBattleMoveTargetFlags(move, gActiveBattler) & MOVE_TARGET_USER_OR_SELECTED)
                            i++;
                        break;
                    case B_POSITION_OPPONENT_LEFT:
                    case B_POSITION_OPPONENT_RIGHT:
                        i++;
                        break;
                }
                MoveSelectionDisplayMoveTypeDoubles(GetBattlerPosition(gMultiUsePlayerCursor));

                if (gAbsentBattlerFlags & gBitTable[gMultiUsePlayerCursor]) i = 0;
            } while (i == 0);
        }

        gSprites[gBattlerSpriteIds[gMultiUsePlayerCursor]].callback = SpriteCb_ShowAsMoveTarget;
        PrintBattleWindow_MoveSelection();
    } else if (JOY_NEW(R_BUTTON)) {
        if (windowMode == 0)
            windowMode = NUM_MOVE_INFO_TYPES - 1;
        else
            windowMode--;
        VarSet(VAR_BATTLE_CONTROLLER_MOVE_WINDOW, windowMode);
        PrintBattleWindow_MoveSelection();
    } else if (JOY_NEW(L_BUTTON)) {
        if (windowMode == NUM_MOVE_INFO_TYPES - 1)
            windowMode = 0;
        else
            windowMode++;
        VarSet(VAR_BATTLE_CONTROLLER_MOVE_WINDOW, windowMode);
        PrintBattleWindow_MoveSelection();
    }
}

static void HideAllTargets(void) {
    s32 i;
    for (i = 0; i < MAX_BATTLERS_COUNT; i++) {
        if (IsBattlerAlive(i) && gBattleSpritesDataPtr->healthBoxesData[i].healthboxIsBouncing) {
            gSprites[gBattlerSpriteIds[i]].callback = SpriteCb_HideAsMoveTarget;
            EndBounceEffect(i, BOUNCE_HEALTHBOX);
        }
    }
}

static void HideShownTargets(void) {
    s32 i;
    for (i = 0; i < MAX_BATTLERS_COUNT; i++) {
        if (IsBattlerAlive(i) && gBattleSpritesDataPtr->healthBoxesData[i].healthboxIsBouncing && i != gActiveBattler) {
            gSprites[gBattlerSpriteIds[i]].callback = SpriteCb_HideAsMoveTarget;
            EndBounceEffect(i, BOUNCE_HEALTHBOX);
        }
    }
}

static void HandleInputShowEntireFieldTargets(void) {
    if (JOY_HELD(DPAD_ANY) && gSaveBlock2Ptr->optionsButtonMode == OPTIONS_BUTTON_MODE_L_EQUALS_A)
        gPlayerDpadHoldFrames++;
    else
        gPlayerDpadHoldFrames = 0;

    if (JOY_NEW(A_BUTTON)) {
        PlaySE(SE_SELECT);
        HideAllTargets();
        if (gBattleStruct->mega.playerSelect)
            BtlController_EmitTwoReturnValues(1, 10, gMoveSelectionCursor[gActiveBattler] | RET_MEGA_EVOLUTION | (gMultiUsePlayerCursor << 8));
        else
            BtlController_EmitTwoReturnValues(1, 10, gMoveSelectionCursor[gActiveBattler] | (gMultiUsePlayerCursor << 8));
        HideMegaTriggerSprite();
        PlayerBufferExecCompleted();
    } else if (gMain.newKeys & B_BUTTON || gPlayerDpadHoldFrames > 59) {
        PlaySE(SE_SELECT);
        HideAllTargets();
        PrintBattleWindow_MoveSelection();
        gBattlerControllerFuncs[gActiveBattler] = HandleInputChooseMove;
        DoBounceEffect(gActiveBattler, BOUNCE_HEALTHBOX, 7, 1);
        DoBounceEffect(gActiveBattler, BOUNCE_MON, 7, 1);
    }
}

static void HandleInputShowTargets(void) {
    if (JOY_HELD(DPAD_ANY) && gSaveBlock2Ptr->optionsButtonMode == OPTIONS_BUTTON_MODE_L_EQUALS_A)
        gPlayerDpadHoldFrames++;
    else
        gPlayerDpadHoldFrames = 0;

    if (JOY_NEW(A_BUTTON)) {
        PlaySE(SE_SELECT);
        HideShownTargets();
        if (gBattleStruct->mega.playerSelect)
            BtlController_EmitTwoReturnValues(1, 10, gMoveSelectionCursor[gActiveBattler] | RET_MEGA_EVOLUTION | (gMultiUsePlayerCursor << 8));
        else
            BtlController_EmitTwoReturnValues(1, 10, gMoveSelectionCursor[gActiveBattler] | (gMultiUsePlayerCursor << 8));
        // HideMegaTriggerSprite();
        // TryHideLastUsedBall();
        TryToHideEnemyInfoWindow();
        PlayerBufferExecCompleted();
    } else if (gMain.newKeys & B_BUTTON || gPlayerDpadHoldFrames > 59) {
        PlaySE(SE_SELECT);
        HideShownTargets();
        PrintBattleWindow_MoveSelection();
        gBattlerControllerFuncs[gActiveBattler] = HandleInputChooseMove;
        DoBounceEffect(gActiveBattler, BOUNCE_HEALTHBOX, 7, 1);
        DoBounceEffect(gActiveBattler, BOUNCE_MON, 7, 1);
    }
}

static void TryShowAsTarget(u32 battlerId) {
    if (IsBattlerAlive(battlerId)) {
        DoBounceEffect(battlerId, BOUNCE_HEALTHBOX, 15, 1);
        gSprites[gBattlerSpriteIds[battlerId]].callback = SpriteCb_ShowAsMoveTarget;
    }
}
static u8 getNumMoves(u8 battler) {
    u8 i, numMoves = 0;
    for (i = 0; i < MAX_MON_MOVES; i++) {
        if (gBattleMons[battler].moves[i] != MOVE_NONE) numMoves++;
    }
    return numMoves;
}

static void HandleInputChooseMove(void) {
    u16 moveTarget;
    u32 canSelectTarget = 0;
    u8 windowMode = VarGet(VAR_BATTLE_CONTROLLER_MOVE_WINDOW);
    struct ChooseMoveStruct *moveInfo = (struct ChooseMoveStruct *)(&gBattleResources->bufferA[gActiveBattler][4]);

    if (gMain.newKeys & A_BUTTON) {
        FlagClear(FLAG_SYS_MOVE_INFO);
        PlaySE(SE_SELECT);
        if (moveInfo->moves[gMoveSelectionCursor[gActiveBattler]] == MOVE_CURSE) {
            if (moveInfo->monType1 == TYPE_GHOST || moveInfo->monType2 == TYPE_GHOST || moveInfo->monType3 == TYPE_GHOST)
                moveTarget = MOVE_TARGET_SELECTED;
            else if (IsBattlerWeatherAffected(gActiveBattler, WEATHER_FOG_ANY))
                moveTarget = MOVE_TARGET_SELECTED;
            else
                moveTarget = MOVE_TARGET_USER;
        } else {
            moveTarget = GetBattlerBattleMoveTargetFlags(moveInfo->moves[gMoveSelectionCursor[gActiveBattler]], gActiveBattler);
        }

        if (moveTarget & MOVE_TARGET_USER)
            gMultiUsePlayerCursor = gActiveBattler;
        else
            gMultiUsePlayerCursor = GetBattlerAtPosition((GetBattlerPosition(gActiveBattler) & BIT_SIDE) ^ BIT_SIDE);

        if (!gBattleResources->bufferA[gActiveBattler][1])  // not a double battle
        {
            if (moveTarget & MOVE_TARGET_USER_OR_SELECTED && !gBattleResources->bufferA[gActiveBattler][2]) canSelectTarget = 1;
        } else  // double battle
        {
            if (!(moveTarget & (MOVE_TARGET_RANDOM | MOVE_TARGET_BOTH | MOVE_TARGET_DEPENDS | MOVE_TARGET_FOES_AND_ALLY | MOVE_TARGET_OPPONENTS_FIELD |
                                MOVE_TARGET_USER | MOVE_TARGET_ALLY)))
                canSelectTarget = 1;  // either selected or user
            if (moveTarget == (MOVE_TARGET_USER | MOVE_TARGET_ALLY) && IsBattlerAlive(BATTLE_PARTNER(gActiveBattler))) canSelectTarget = 1;

            if (moveInfo->currentPp[gMoveSelectionCursor[gActiveBattler]] == 0) {
                canSelectTarget = 0;
            } else if (!(moveTarget & (MOVE_TARGET_USER | MOVE_TARGET_USER_OR_SELECTED)) && CountAliveMonsInBattle(BATTLE_ALIVE_EXCEPT_ACTIVE) <= 1) {
                gMultiUsePlayerCursor = GetDefaultMoveTarget(gActiveBattler);
                canSelectTarget = 0;
            }

            // Show all available targets for multi-target moves
            if (B_SHOW_TARGETS) {
                if ((moveTarget & MOVE_TARGET_ALL_BATTLERS) == MOVE_TARGET_ALL_BATTLERS) {
                    u32 i = 0;
                    for (i = 0; i < gBattlersCount; i++) TryShowAsTarget(i);

                    canSelectTarget = 3;
                } else if (moveTarget & (MOVE_TARGET_OPPONENTS_FIELD | MOVE_TARGET_BOTH | MOVE_TARGET_FOES_AND_ALLY)) {
                    TryShowAsTarget(gMultiUsePlayerCursor);
                    TryShowAsTarget(BATTLE_PARTNER(gMultiUsePlayerCursor));
                    if (moveTarget & MOVE_TARGET_FOES_AND_ALLY) TryShowAsTarget(BATTLE_PARTNER(gActiveBattler));
                    canSelectTarget = 2;
                }
            }
        }

        switch (canSelectTarget) {
            case 0:
            default:
                if (gBattleStruct->mega.playerSelect)
                    BtlController_EmitTwoReturnValues(1, 10, gMoveSelectionCursor[gActiveBattler] | RET_MEGA_EVOLUTION | (gMultiUsePlayerCursor << 8));
                else
                    BtlController_EmitTwoReturnValues(1, 10, gMoveSelectionCursor[gActiveBattler] | (gMultiUsePlayerCursor << 8));
                HideMegaTriggerSprite();
                PlayerBufferExecCompleted();
                break;
            case 1:
                gBattlerControllerFuncs[gActiveBattler] = HandleInputChooseTarget;

                if (moveTarget & (MOVE_TARGET_USER | MOVE_TARGET_USER_OR_SELECTED))
                    gMultiUsePlayerCursor = gActiveBattler;
                else if (gAbsentBattlerFlags & gBitTable[GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT)])
                    gMultiUsePlayerCursor = GetBattlerAtPosition(B_POSITION_OPPONENT_RIGHT);
                else
                    gMultiUsePlayerCursor = GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT);

                gSprites[gBattlerSpriteIds[gMultiUsePlayerCursor]].callback = SpriteCb_ShowAsMoveTarget;
                break;
            case 2:
                gBattlerControllerFuncs[gActiveBattler] = HandleInputShowTargets;
                break;
            case 3:  // Entire field
                gBattlerControllerFuncs[gActiveBattler] = HandleInputShowEntireFieldTargets;
                break;
        }

        PrintBattleWindow_MoveSelection();
    } else if (JOY_NEW(B_BUTTON) || gPlayerDpadHoldFrames > 59) {
        PlaySE(SE_SELECT);
        gBattleStruct->mega.playerSelect = FALSE;
        BtlController_EmitTwoReturnValues(1, 10, 0xFFFF);
        HideMegaTriggerSprite();
        PlayerBufferExecCompleted();
        FlagClear(FLAG_SYS_MOVE_INFO);
    } else if (JOY_NEW(DPAD_UP)) {
        if (gMoveSelectionCursor[gActiveBattler] != 0)
            gMoveSelectionCursor[gActiveBattler]--;
        else
            gMoveSelectionCursor[gActiveBattler] = getNumMoves(gActiveBattler) - 1;
        PlaySE(SE_SELECT);
        PrintBattleWindow_MoveSelection();
    } else if (JOY_NEW(DPAD_DOWN)) {
        if (gMoveSelectionCursor[gActiveBattler] < getNumMoves(gActiveBattler) - 1)
            gMoveSelectionCursor[gActiveBattler]++;
        else
            gMoveSelectionCursor[gActiveBattler] = 0;
        PlaySE(SE_SELECT);
        PrintBattleWindow_MoveSelection();
    } else if (JOY_NEW(R_BUTTON) || JOY_NEW(DPAD_RIGHT)) {
        if (windowMode >= NUM_MOVE_INFO_TYPES - 1)
            windowMode = 0;
        else
            windowMode++;
        VarSet(VAR_BATTLE_CONTROLLER_MOVE_WINDOW, windowMode);
        PrintBattleWindow_MoveSelection();
    } else if (JOY_NEW(L_BUTTON) || JOY_NEW(DPAD_LEFT)) {
        if (windowMode == 0)
            windowMode = NUM_MOVE_INFO_TYPES - 1;
        else
            windowMode--;
        VarSet(VAR_BATTLE_CONTROLLER_MOVE_WINDOW, windowMode);
        PrintBattleWindow_MoveSelection();
    } else if (gMain.newKeys & START_BUTTON) {
        if (CanMegaEvolve(gActiveBattler)) {
            gBattleStruct->mega.playerSelect ^= 1;
            ChangeMegaTriggerSprite(gBattleStruct->mega.triggerSpriteId, gBattleStruct->mega.playerSelect);
            PlaySE(SE_SELECT);
        }
    }

    /*
    if (gMain.heldKeys & DPAD_ANY && gSaveBlock2Ptr->optionsButtonMode == OPTIONS_BUTTON_MODE_L_EQUALS_A)
        gPlayerDpadHoldFrames++;
    else
        gPlayerDpadHoldFrames = 0;

    if (gMain.newKeys & A_BUTTON)

    else if (JOY_NEW(B_BUTTON) || gPlayerDpadHoldFrames > 59)
    {
        PlaySE(SE_SELECT);
        gBattleStruct->mega.playerSelect = FALSE;
        BtlController_EmitTwoReturnValues(1, 10, 0xFFFF);
        HideMegaTriggerSprite();
        PlayerBufferExecCompleted();
        FlagClear(FLAG_SYS_MOVE_INFO);
    }
    else if (JOY_NEW(L_BUTTON) || gPlayerDpadHoldFrames > 59)
    {
        if (!FlagGet(FLAG_SYS_MOVE_INFO)) {
            MoveSelectionDestroyCursorAt(gMoveSelectionCursor[gActiveBattler]);
            MoveSelectionCreateCursorAt(0, 0);
            ChangeMoveDisplayMode();
            FlagSet(FLAG_SYS_MOVE_INFO);
        }
        else {
            MoveSelectionDestroyCursorAt(0);
            MoveSelectionDisplayMoveNames();
            MoveSelectionCreateCursorAt(gMoveSelectionCursor[gActiveBattler], 0);
            FlagClear(FLAG_SYS_MOVE_INFO);
        }
    }
    else if (JOY_NEW(DPAD_LEFT) && !FlagGet(FLAG_SYS_MOVE_INFO))
    {
        if (gMoveSelectionCursor[gActiveBattler] & 1)
        {
            MoveSelectionDestroyCursorAt(gMoveSelectionCursor[gActiveBattler]);
            gMoveSelectionCursor[gActiveBattler] ^= 1;
            PlaySE(SE_SELECT);
            MoveSelectionCreateCursorAt(gMoveSelectionCursor[gActiveBattler], 0);
            MoveSelectionDisplayPpNumber();
            MoveSelectionDisplayMoveType();
        }
    }
    else if (JOY_NEW(DPAD_RIGHT) && !FlagGet(FLAG_SYS_MOVE_INFO))
    {
        if (!(gMoveSelectionCursor[gActiveBattler] & 1)
         && (gMoveSelectionCursor[gActiveBattler] ^ 1) < gNumberOfMovesToChoose)
        {
            MoveSelectionDestroyCursorAt(gMoveSelectionCursor[gActiveBattler]);
            gMoveSelectionCursor[gActiveBattler] ^= 1;
            PlaySE(SE_SELECT);
            MoveSelectionCreateCursorAt(gMoveSelectionCursor[gActiveBattler], 0);
            MoveSelectionDisplayPpNumber();
            MoveSelectionDisplayMoveType();
        }
    }
    else if (JOY_NEW(DPAD_UP) && !FlagGet(FLAG_SYS_MOVE_INFO))
    {
        if (gMoveSelectionCursor[gActiveBattler] & 2)
        {
            MoveSelectionDestroyCursorAt(gMoveSelectionCursor[gActiveBattler]);
            gMoveSelectionCursor[gActiveBattler] ^= 2;
            PlaySE(SE_SELECT);
            MoveSelectionCreateCursorAt(gMoveSelectionCursor[gActiveBattler], 0);
            MoveSelectionDisplayPpNumber();
            MoveSelectionDisplayMoveType();
        }
    }
    else if (JOY_NEW(DPAD_DOWN) && !FlagGet(FLAG_SYS_MOVE_INFO))
    {
        if (!(gMoveSelectionCursor[gActiveBattler] & 2)
         && (gMoveSelectionCursor[gActiveBattler] ^ 2) < gNumberOfMovesToChoose)
        {
            MoveSelectionDestroyCursorAt(gMoveSelectionCursor[gActiveBattler]);
            gMoveSelectionCursor[gActiveBattler] ^= 2;
            PlaySE(SE_SELECT);
            MoveSelectionCreateCursorAt(gMoveSelectionCursor[gActiveBattler], 0);
            MoveSelectionDisplayPpNumber();
            MoveSelectionDisplayMoveType();
        }
    }
    else if (JOY_NEW(SELECT_BUTTON))
    {
        if (gNumberOfMovesToChoose > 1 && !(gBattleTypeFlags & BATTLE_TYPE_LINK))
        {
            MoveSelectionCreateCursorAt(gMoveSelectionCursor[gActiveBattler], 29);

            if (gMoveSelectionCursor[gActiveBattler] != 0)
                gMultiUsePlayerCursor = 0;
            else
                gMultiUsePlayerCursor = gMoveSelectionCursor[gActiveBattler] + 1;

            MoveSelectionCreateCursorAt(gMultiUsePlayerCursor, 27);
            BattlePutTextOnWindow(gText_BattleSwitchWhich, B_WIN_SWITCH_PROMPT);
            gBattlerControllerFuncs[gActiveBattler] = HandleMoveSwitching;
        }
    }
    else if (gMain.newKeys & START_BUTTON)
    {
        if (CanMegaEvolve(gActiveBattler))
        {
            gBattleStruct->mega.playerSelect ^= 1;
            ChangeMegaTriggerSprite(gBattleStruct->mega.triggerSpriteId, gBattleStruct->mega.playerSelect);
            PlaySE(SE_SELECT);
        }
    }*/
}

static u32 HandleMoveInputUnused(void) {
    u32 var = 0;

    if (JOY_NEW(A_BUTTON)) {
        PlaySE(SE_SELECT);
        var = 1;
    }
    if (JOY_NEW(B_BUTTON)) {
        PlaySE(SE_SELECT);
        gBattle_BG0_X = 0;
        gBattle_BG0_Y = DISPLAY_HEIGHT * 2;
        var = 0xFF;
    }
    if (JOY_NEW(DPAD_LEFT) && gMoveSelectionCursor[gActiveBattler] & 1) {
        MoveSelectionDestroyCursorAt(gMoveSelectionCursor[gActiveBattler]);
        gMoveSelectionCursor[gActiveBattler] ^= 1;
        PlaySE(SE_SELECT);
        MoveSelectionCreateCursorAt(gMoveSelectionCursor[gActiveBattler], 0);
    }
    if (JOY_NEW(DPAD_RIGHT) && !(gMoveSelectionCursor[gActiveBattler] & 1) && (gMoveSelectionCursor[gActiveBattler] ^ 1) < gNumberOfMovesToChoose) {
        MoveSelectionDestroyCursorAt(gMoveSelectionCursor[gActiveBattler]);
        gMoveSelectionCursor[gActiveBattler] ^= 1;
        PlaySE(SE_SELECT);
        MoveSelectionCreateCursorAt(gMoveSelectionCursor[gActiveBattler], 0);
    }
    if (JOY_NEW(DPAD_UP) && gMoveSelectionCursor[gActiveBattler] & 2) {
        MoveSelectionDestroyCursorAt(gMoveSelectionCursor[gActiveBattler]);
        gMoveSelectionCursor[gActiveBattler] ^= 2;
        PlaySE(SE_SELECT);
        MoveSelectionCreateCursorAt(gMoveSelectionCursor[gActiveBattler], 0);
    }
    if (JOY_NEW(DPAD_DOWN) && !(gMoveSelectionCursor[gActiveBattler] & 2) && (gMoveSelectionCursor[gActiveBattler] ^ 2) < gNumberOfMovesToChoose) {
        MoveSelectionDestroyCursorAt(gMoveSelectionCursor[gActiveBattler]);
        gMoveSelectionCursor[gActiveBattler] ^= 2;
        PlaySE(SE_SELECT);
        MoveSelectionCreateCursorAt(gMoveSelectionCursor[gActiveBattler], 0);
    }

    return var;
}

static void HandleMoveSwitching(void) {
    u8 perMovePPBonuses[MAX_MON_MOVES];
    struct ChooseMoveStruct moveStruct;
    u8 totalPPBonuses;

    if (JOY_NEW(A_BUTTON | SELECT_BUTTON)) {
        PlaySE(SE_SELECT);

        if (gMoveSelectionCursor[gActiveBattler] != gMultiUsePlayerCursor) {
            struct ChooseMoveStruct *moveInfo = (struct ChooseMoveStruct *)(&gBattleResources->bufferA[gActiveBattler][4]);
            s32 i;

            // swap moves and pp
            i = moveInfo->moves[gMoveSelectionCursor[gActiveBattler]];
            moveInfo->moves[gMoveSelectionCursor[gActiveBattler]] = moveInfo->moves[gMultiUsePlayerCursor];
            moveInfo->moves[gMultiUsePlayerCursor] = i;

            i = moveInfo->currentPp[gMoveSelectionCursor[gActiveBattler]];
            moveInfo->currentPp[gMoveSelectionCursor[gActiveBattler]] = moveInfo->currentPp[gMultiUsePlayerCursor];
            moveInfo->currentPp[gMultiUsePlayerCursor] = i;

            i = moveInfo->maxPp[gMoveSelectionCursor[gActiveBattler]];
            moveInfo->maxPp[gMoveSelectionCursor[gActiveBattler]] = moveInfo->maxPp[gMultiUsePlayerCursor];
            moveInfo->maxPp[gMultiUsePlayerCursor] = i;

            if (gVolatileStructs[gActiveBattler].mimickedMoves & gBitTable[gMoveSelectionCursor[gActiveBattler]]) {
                gVolatileStructs[gActiveBattler].mimickedMoves &= (~gBitTable[gMoveSelectionCursor[gActiveBattler]]);
                gVolatileStructs[gActiveBattler].mimickedMoves |= gBitTable[gMultiUsePlayerCursor];
            }

            MoveSelectionDisplayMoveNames();

            for (i = 0; i < MAX_MON_MOVES; i++) perMovePPBonuses[i] = (gBattleMons[gActiveBattler].ppBonuses & (3 << (i * 2))) >> (i * 2);

            totalPPBonuses = perMovePPBonuses[gMoveSelectionCursor[gActiveBattler]];
            perMovePPBonuses[gMoveSelectionCursor[gActiveBattler]] = perMovePPBonuses[gMultiUsePlayerCursor];
            perMovePPBonuses[gMultiUsePlayerCursor] = totalPPBonuses;

            totalPPBonuses = 0;
            for (i = 0; i < MAX_MON_MOVES; i++) totalPPBonuses |= perMovePPBonuses[i] << (i * 2);

            gBattleMons[gActiveBattler].ppBonuses = totalPPBonuses;

            for (i = 0; i < MAX_MON_MOVES; i++) {
                gBattleMons[gActiveBattler].moves[i] = moveInfo->moves[i];
                gBattleMons[gActiveBattler].pp[i] = moveInfo->currentPp[i];
            }

            if (!(gBattleMons[gActiveBattler].status2 & STATUS2_TRANSFORMED)) {
                for (i = 0; i < MAX_MON_MOVES; i++) {
                    moveStruct.moves[i] = GetMonData(&gPlayerParty[gBattlerPartyIndexes[gActiveBattler]], MON_DATA_MOVE1 + i);
                    moveStruct.currentPp[i] = GetMonData(&gPlayerParty[gBattlerPartyIndexes[gActiveBattler]], MON_DATA_PP1 + i);
                }

                totalPPBonuses = GetMonData(&gPlayerParty[gBattlerPartyIndexes[gActiveBattler]], MON_DATA_PP_BONUSES);
                for (i = 0; i < MAX_MON_MOVES; i++) perMovePPBonuses[i] = (totalPPBonuses & (3 << (i * 2))) >> (i * 2);

                i = moveStruct.moves[gMoveSelectionCursor[gActiveBattler]];
                moveStruct.moves[gMoveSelectionCursor[gActiveBattler]] = moveStruct.moves[gMultiUsePlayerCursor];
                moveStruct.moves[gMultiUsePlayerCursor] = i;

                i = moveStruct.currentPp[gMoveSelectionCursor[gActiveBattler]];
                moveStruct.currentPp[gMoveSelectionCursor[gActiveBattler]] = moveStruct.currentPp[gMultiUsePlayerCursor];
                moveStruct.currentPp[gMultiUsePlayerCursor] = i;

                totalPPBonuses = perMovePPBonuses[gMoveSelectionCursor[gActiveBattler]];
                perMovePPBonuses[gMoveSelectionCursor[gActiveBattler]] = perMovePPBonuses[gMultiUsePlayerCursor];
                perMovePPBonuses[gMultiUsePlayerCursor] = totalPPBonuses;

                totalPPBonuses = 0;
                for (i = 0; i < MAX_MON_MOVES; i++) totalPPBonuses |= perMovePPBonuses[i] << (i * 2);

                for (i = 0; i < MAX_MON_MOVES; i++) {
                    SetMonData(&gPlayerParty[gBattlerPartyIndexes[gActiveBattler]], MON_DATA_MOVE1 + i, &moveStruct.moves[i]);
                    SetMonData(&gPlayerParty[gBattlerPartyIndexes[gActiveBattler]], MON_DATA_PP1 + i, &moveStruct.currentPp[i]);
                }

                SetMonData(&gPlayerParty[gBattlerPartyIndexes[gActiveBattler]], MON_DATA_PP_BONUSES, &totalPPBonuses);
            }
        }

        PrintBattleWindow_MoveSelection();
        gBattlerControllerFuncs[gActiveBattler] = HandleInputChooseMove;
        gMoveSelectionCursor[gActiveBattler] = gMultiUsePlayerCursor;
        MoveSelectionCreateCursorAt(gMoveSelectionCursor[gActiveBattler], 0);
        MoveSelectionDisplayPpString();
        MoveSelectionDisplayPpNumber();
        MoveSelectionDisplayMoveType();
    } else if (JOY_NEW(B_BUTTON | SELECT_BUTTON)) {
        PlaySE(SE_SELECT);
        MoveSelectionDestroyCursorAt(gMultiUsePlayerCursor);
        MoveSelectionCreateCursorAt(gMoveSelectionCursor[gActiveBattler], 0);
        PrintBattleWindow_MoveSelection();
        gBattlerControllerFuncs[gActiveBattler] = HandleInputChooseMove;
        MoveSelectionDisplayPpString();
        MoveSelectionDisplayPpNumber();
        MoveSelectionDisplayMoveType();
    } else if (JOY_NEW(DPAD_LEFT)) {
        if (gMultiUsePlayerCursor & 1) {
            if (gMultiUsePlayerCursor == gMoveSelectionCursor[gActiveBattler])
                MoveSelectionCreateCursorAt(gMoveSelectionCursor[gActiveBattler], 29);
            else
                MoveSelectionDestroyCursorAt(gMultiUsePlayerCursor);

            gMultiUsePlayerCursor ^= 1;
            PlaySE(SE_SELECT);

            if (gMultiUsePlayerCursor == gMoveSelectionCursor[gActiveBattler])
                MoveSelectionCreateCursorAt(gMultiUsePlayerCursor, 0);
            else
                MoveSelectionCreateCursorAt(gMultiUsePlayerCursor, 27);
        }
    } else if (JOY_NEW(DPAD_RIGHT)) {
        if (!(gMultiUsePlayerCursor & 1) && (gMultiUsePlayerCursor ^ 1) < gNumberOfMovesToChoose) {
            if (gMultiUsePlayerCursor == gMoveSelectionCursor[gActiveBattler])
                MoveSelectionCreateCursorAt(gMoveSelectionCursor[gActiveBattler], 29);
            else
                MoveSelectionDestroyCursorAt(gMultiUsePlayerCursor);

            gMultiUsePlayerCursor ^= 1;
            PlaySE(SE_SELECT);

            if (gMultiUsePlayerCursor == gMoveSelectionCursor[gActiveBattler])
                MoveSelectionCreateCursorAt(gMultiUsePlayerCursor, 0);
            else
                MoveSelectionCreateCursorAt(gMultiUsePlayerCursor, 27);
        }
    } else if (JOY_NEW(DPAD_UP)) {
        if (gMultiUsePlayerCursor & 2) {
            if (gMultiUsePlayerCursor == gMoveSelectionCursor[gActiveBattler])
                MoveSelectionCreateCursorAt(gMoveSelectionCursor[gActiveBattler], 29);
            else
                MoveSelectionDestroyCursorAt(gMultiUsePlayerCursor);

            gMultiUsePlayerCursor ^= 2;
            PlaySE(SE_SELECT);

            if (gMultiUsePlayerCursor == gMoveSelectionCursor[gActiveBattler])
                MoveSelectionCreateCursorAt(gMultiUsePlayerCursor, 0);
            else
                MoveSelectionCreateCursorAt(gMultiUsePlayerCursor, 27);
        }
    } else if (JOY_NEW(DPAD_DOWN)) {
        if (!(gMultiUsePlayerCursor & 2) && (gMultiUsePlayerCursor ^ 2) < gNumberOfMovesToChoose) {
            if (gMultiUsePlayerCursor == gMoveSelectionCursor[gActiveBattler])
                MoveSelectionCreateCursorAt(gMoveSelectionCursor[gActiveBattler], 29);
            else
                MoveSelectionDestroyCursorAt(gMultiUsePlayerCursor);

            gMultiUsePlayerCursor ^= 2;
            PlaySE(SE_SELECT);

            if (gMultiUsePlayerCursor == gMoveSelectionCursor[gActiveBattler])
                MoveSelectionCreateCursorAt(gMultiUsePlayerCursor, 0);
            else
                MoveSelectionCreateCursorAt(gMultiUsePlayerCursor, 27);
        }
    }
}

static void SetLinkBattleEndCallbacks(void) {
    if (gWirelessCommType == 0) {
        if (gReceivedRemoteLinkPlayers == 0) {
            m4aSongNumStop(SE_LOW_HEALTH);
            gMain.inBattle = 0;
            gMain.callback1 = gPreBattleCallback1;
            SetMainCallback2(CB2_InitEndLinkBattle);
            if (gBattleOutcome == B_OUTCOME_WON) TryPutLinkBattleTvShowOnAir();
            FreeAllWindowBuffers();
        }
    } else {
        if (IsLinkTaskFinished()) {
            m4aSongNumStop(SE_LOW_HEALTH);
            gMain.inBattle = 0;
            gMain.callback1 = gPreBattleCallback1;
            SetMainCallback2(CB2_InitEndLinkBattle);
            if (gBattleOutcome == B_OUTCOME_WON) TryPutLinkBattleTvShowOnAir();
            FreeAllWindowBuffers();
        }
    }
}

// Despite handling link battles separately, this is only ever used by link battles
void SetBattleEndCallbacks(void) {
    if (!gPaletteFade.active) {
        if (gBattleTypeFlags & BATTLE_TYPE_LINK) {
            if (IsLinkTaskFinished()) {
                if (gWirelessCommType == 0)
                    SetCloseLinkCallback();
                else
                    SetLinkStandbyCallback();

                gBattlerControllerFuncs[gActiveBattler] = SetLinkBattleEndCallbacks;
            }
        } else {
            m4aSongNumStop(SE_LOW_HEALTH);
            gMain.inBattle = 0;
            gMain.callback1 = gPreBattleCallback1;
            SetMainCallback2(gMain.savedCallback);
        }
    }
}

static void CompleteOnBattlerSpriteCallbackDummy(void) {
    if (gSprites[gBattlerSpriteIds[gActiveBattler]].callback == SpriteCallbackDummy) PlayerBufferExecCompleted();
}

static void CompleteOnBankSpriteCallbackDummy2(void) {
    if (gSprites[gBattlerSpriteIds[gActiveBattler]].callback == SpriteCallbackDummy) PlayerBufferExecCompleted();
}

static void FreeTrainerSpriteAfterSlide(void) {
    if (gSprites[gBattlerSpriteIds[gActiveBattler]].callback == SpriteCallbackDummy) {
        BattleGfxSfxDummy3(gSaveBlock2Ptr->playerGender);
        FreeSpriteOamMatrix(&gSprites[gBattlerSpriteIds[gActiveBattler]]);
        DestroySprite(&gSprites[gBattlerSpriteIds[gActiveBattler]]);
        PlayerBufferExecCompleted();
    }
}

static void Intro_DelayAndEnd(void) {
    if (--gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].introEndDelay == (u8)-1) {
        gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].introEndDelay = 0;
        PlayerBufferExecCompleted();
    }
}

static bool32 TwoIntroMons(u32 battlerId)  // Double battle with both player pokemon active.
{
    return (IsDoubleBattle() && IsValidForBattle(&gPlayerParty[gBattlerPartyIndexes[battlerId ^ BIT_FLANK]]));
}

static void Intro_WaitForShinyAnimAndHealthbox(void) {
    bool8 healthboxAnimDone = FALSE;

    // Check if healthbox has finished sliding in
    if (TwoIntroMons(gActiveBattler) && !(gBattleTypeFlags & BATTLE_TYPE_MULTI)) {
        if (gSprites[gHealthboxSpriteIds[gActiveBattler]].callback == SpriteCallbackDummy &&
            gSprites[gHealthboxSpriteIds[gActiveBattler ^ BIT_FLANK]].callback == SpriteCallbackDummy)
            healthboxAnimDone = TRUE;
    } else {
        if (gSprites[gHealthboxSpriteIds[gActiveBattler]].callback == SpriteCallbackDummy) healthboxAnimDone = TRUE;
    }

    // If healthbox and shiny anim are done
    if (healthboxAnimDone && gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].finishedShinyMonAnim &&
        gBattleSpritesDataPtr->healthBoxesData[gActiveBattler ^ BIT_FLANK].finishedShinyMonAnim) {
        // Reset shiny anim (even if it didn't occur)
        gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].triedShinyMonAnim = FALSE;
        gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].finishedShinyMonAnim = FALSE;
        gBattleSpritesDataPtr->healthBoxesData[gActiveBattler ^ BIT_FLANK].triedShinyMonAnim = FALSE;
        gBattleSpritesDataPtr->healthBoxesData[gActiveBattler ^ BIT_FLANK].finishedShinyMonAnim = FALSE;
        FreeSpriteTilesByTag(ANIM_TAG_GOLD_STARS);
        FreeSpritePaletteByTag(ANIM_TAG_GOLD_STARS);

        HandleLowHpMusicChange(&gPlayerParty[gBattlerPartyIndexes[gActiveBattler]], gActiveBattler);

        if (TwoIntroMons(gActiveBattler)) HandleLowHpMusicChange(&gPlayerParty[gBattlerPartyIndexes[gActiveBattler ^ BIT_FLANK]], gActiveBattler ^ BIT_FLANK);

        gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].introEndDelay = 3;
        gBattlerControllerFuncs[gActiveBattler] = Intro_DelayAndEnd;
    }
}

static void Intro_TryShinyAnimShowHealthbox(void) {
    bool32 bgmRestored = FALSE;
    bool32 battlerAnimsDone = FALSE;

    // Start shiny animation if applicable for 1st pokemon
    if (!gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].triedShinyMonAnim && !gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].ballAnimActive)
        TryShinyAnimation(gActiveBattler, &gPlayerParty[gBattlerPartyIndexes[gActiveBattler]]);

    // Start shiny animation if applicable for 2nd pokemon
    if (!gBattleSpritesDataPtr->healthBoxesData[gActiveBattler ^ BIT_FLANK].triedShinyMonAnim &&
        !gBattleSpritesDataPtr->healthBoxesData[gActiveBattler ^ BIT_FLANK].ballAnimActive)
        TryShinyAnimation(gActiveBattler ^ BIT_FLANK, &gPlayerParty[gBattlerPartyIndexes[gActiveBattler ^ BIT_FLANK]]);

    // Show healthbox after ball anim
    if (!gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].ballAnimActive &&
        !gBattleSpritesDataPtr->healthBoxesData[gActiveBattler ^ BIT_FLANK].ballAnimActive) {
        if (!gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].healthboxSlideInStarted) {
            if (TwoIntroMons(gActiveBattler) && !(gBattleTypeFlags & BATTLE_TYPE_MULTI)) {
                UpdateHealthboxAttribute(
                    gHealthboxSpriteIds[gActiveBattler ^ BIT_FLANK], &gPlayerParty[gBattlerPartyIndexes[gActiveBattler ^ BIT_FLANK]], HEALTHBOX_ALL);
                StartHealthboxSlideIn(gActiveBattler ^ BIT_FLANK);
                SetHealthboxSpriteVisible(gHealthboxSpriteIds[gActiveBattler ^ BIT_FLANK]);
            }
            UpdateHealthboxAttribute(gHealthboxSpriteIds[gActiveBattler], &gPlayerParty[gBattlerPartyIndexes[gActiveBattler]], HEALTHBOX_ALL);
            StartHealthboxSlideIn(gActiveBattler);
            SetHealthboxSpriteVisible(gHealthboxSpriteIds[gActiveBattler]);
        }
        gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].healthboxSlideInStarted = TRUE;
    }

    // Restore bgm after cry has played and healthbox anim is started
    if (!gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].waitForCry && gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].healthboxSlideInStarted &&
        !gBattleSpritesDataPtr->healthBoxesData[gActiveBattler ^ BIT_FLANK].waitForCry && !IsCryPlayingOrClearCrySongs()) {
        if (!gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].bgmRestored) {
            if (gBattleTypeFlags & BATTLE_TYPE_MULTI && gBattleTypeFlags & BATTLE_TYPE_LINK)
                m4aMPlayContinue(&gMPlayInfo_BGM);
            else
                m4aMPlayVolumeControl(&gMPlayInfo_BGM, 0xFFFF, 0x100);
        }
        gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].bgmRestored = TRUE;
        bgmRestored = TRUE;
    }

    // Wait for battler anims
    if (TwoIntroMons(gActiveBattler) && !(gBattleTypeFlags & BATTLE_TYPE_MULTI)) {
        if (gSprites[gBattleControllerData[gActiveBattler]].callback == SpriteCallbackDummy &&
            gSprites[gBattlerSpriteIds[gActiveBattler]].callback == SpriteCallbackDummy &&
            gSprites[gBattleControllerData[gActiveBattler ^ BIT_FLANK]].callback == SpriteCallbackDummy &&
            gSprites[gBattlerSpriteIds[gActiveBattler ^ BIT_FLANK]].callback == SpriteCallbackDummy) {
            battlerAnimsDone = TRUE;
        }
    } else {
        if (gSprites[gBattleControllerData[gActiveBattler]].callback == SpriteCallbackDummy &&
            gSprites[gBattlerSpriteIds[gActiveBattler]].callback == SpriteCallbackDummy) {
            battlerAnimsDone = TRUE;
        }
    }

    // Clean up
    if (bgmRestored && battlerAnimsDone) {
        if (TwoIntroMons(gActiveBattler) && !(gBattleTypeFlags & BATTLE_TYPE_MULTI))
            DestroySprite(&gSprites[gBattleControllerData[gActiveBattler ^ BIT_FLANK]]);
        DestroySprite(&gSprites[gBattleControllerData[gActiveBattler]]);

        gBattleSpritesDataPtr->animationData->introAnimActive = FALSE;
        gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].bgmRestored = FALSE;
        gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].healthboxSlideInStarted = FALSE;

        gBattlerControllerFuncs[gActiveBattler] = Intro_WaitForShinyAnimAndHealthbox;
    }
}

static void SwitchIn_CleanShinyAnimShowSubstitute(void) {
    if (gSprites[gHealthboxSpriteIds[gActiveBattler]].callback == SpriteCallbackDummy &&
        gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].finishedShinyMonAnim &&
        gSprites[gBattlerSpriteIds[gActiveBattler]].callback == SpriteCallbackDummy) {
        CopyBattleSpriteInvisibility(gActiveBattler);

        // Reset shiny anim (even if it didn't occur)
        gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].triedShinyMonAnim = FALSE;
        gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].finishedShinyMonAnim = FALSE;
        FreeSpriteTilesByTag(ANIM_TAG_GOLD_STARS);
        FreeSpritePaletteByTag(ANIM_TAG_GOLD_STARS);

        // Check if Substitute should be shown
        if (gBattleSpritesDataPtr->battlerData[gActiveBattler].behindSubstitute)
            InitAndLaunchSpecialAnimation(gActiveBattler, gActiveBattler, gActiveBattler, B_ANIM_MON_TO_SUBSTITUTE);

        gBattlerControllerFuncs[gActiveBattler] = SwitchIn_HandleSoundAndEnd;
    }
}

static void SwitchIn_HandleSoundAndEnd(void) {
    if (!gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].specialAnimActive && !IsCryPlayingOrClearCrySongs()) {
        m4aMPlayVolumeControl(&gMPlayInfo_BGM, 0xFFFF, 0x100);
        HandleLowHpMusicChange(&gPlayerParty[gBattlerPartyIndexes[gActiveBattler]], gActiveBattler);
        PlayerBufferExecCompleted();
    }
}

static void SwitchIn_TryShinyAnimShowHealthbox(void) {
    // Start shiny animation if applicable
    if (!gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].triedShinyMonAnim && !gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].ballAnimActive)
        TryShinyAnimation(gActiveBattler, &gPlayerParty[gBattlerPartyIndexes[gActiveBattler]]);

    // Wait for ball anim, then show healthbox
    if (gSprites[gBattleControllerData[gActiveBattler]].callback == SpriteCallbackDummy &&
        !gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].ballAnimActive) {
        DestroySprite(&gSprites[gBattleControllerData[gActiveBattler]]);
        UpdateHealthboxAttribute(gHealthboxSpriteIds[gActiveBattler], &gPlayerParty[gBattlerPartyIndexes[gActiveBattler]], HEALTHBOX_ALL);
        StartHealthboxSlideIn(gActiveBattler);
        SetHealthboxSpriteVisible(gHealthboxSpriteIds[gActiveBattler]);
        gBattlerControllerFuncs[gActiveBattler] = SwitchIn_CleanShinyAnimShowSubstitute;
    }
}

void Task_PlayerController_RestoreBgmAfterCry(u8 taskId) {
    if (!IsCryPlayingOrClearCrySongs()) {
        m4aMPlayVolumeControl(&gMPlayInfo_BGM, 0xFFFF, 0x100);
        DestroyTask(taskId);
    }
}

static void CompleteOnHealthbarDone(void) {
    s16 hpValue = MoveBattleBar(gActiveBattler, gHealthboxSpriteIds[gActiveBattler], HEALTH_BAR, 0);

    SetHealthboxSpriteVisible(gHealthboxSpriteIds[gActiveBattler]);

    if (hpValue != -1) {
        UpdateHpTextInHealthbox(gHealthboxSpriteIds[gActiveBattler], hpValue, HP_CURRENT);
    } else {
        HandleLowHpMusicChange(&gPlayerParty[gBattlerPartyIndexes[gActiveBattler]], gActiveBattler);
        PlayerBufferExecCompleted();
    }
}

static void CompleteOnInactiveTextPrinter(void) {
    if (!IsTextPrinterActive(0)) PlayerBufferExecCompleted();
}

#define tExpTask_monId data[0]
#define tExpTask_battler data[2]
#define tExpTask_gainedExp_1 data[3]
#define tExpTask_gainedExp_2 data[4]  // Stored as two half-words containing a word.
#define tExpTask_frames data[10]

static s32 GetTaskExpValue(u8 taskId) { return (u16)(gTasks[taskId].tExpTask_gainedExp_1) | (gTasks[taskId].tExpTask_gainedExp_2 << 16); }

static void Task_GiveExpToMon(u8 taskId) {
    u32 monId = (u8)(gTasks[taskId].tExpTask_monId);
    u8 battlerId = gTasks[taskId].tExpTask_battler;
    s32 gainedExp = GetTaskExpValue(taskId);

    if (IsDoubleBattle() == TRUE || monId != gBattlerPartyIndexes[battlerId])  // Give exp without moving the expbar.
    {
        struct Pokemon *mon = &gPlayerParty[monId];
        SpeciesEnum species = GetMonData(mon, MON_DATA_SPECIES);
        u8 level = GetMonData(mon, MON_DATA_LEVEL);
        u32 currExp = GetMonData(mon, MON_DATA_EXP);
        u32 nextLvlExp = gExperienceTables[gBaseStats[species].growthRate][level + 1];

        if (currExp + gainedExp >= nextLvlExp) {
            u8 savedActiveBattler;

            SetMonData(mon, MON_DATA_EXP, &nextLvlExp);
            CalculateMonStats(mon);
            gainedExp -= nextLvlExp - currExp;
            savedActiveBattler = gActiveBattler;
            gActiveBattler = battlerId;
            BtlController_EmitTwoReturnValues(1, RET_VALUE_LEVELED_UP, gainedExp);
            gActiveBattler = savedActiveBattler;

            if (IsDoubleBattle() == TRUE && ((u16)(monId) == gBattlerPartyIndexes[battlerId] || (u16)(monId) == gBattlerPartyIndexes[battlerId ^ BIT_FLANK]))
                gTasks[taskId].func = Task_LaunchLvlUpAnim;
            else
                gTasks[taskId].func = DestroyExpTaskAndCompleteOnInactiveTextPrinter;
        } else {
            currExp += gainedExp;
            SetMonData(mon, MON_DATA_EXP, &currExp);
            gBattlerControllerFuncs[battlerId] = CompleteOnInactiveTextPrinter;
            DestroyTask(taskId);
        }
    } else {
        gTasks[taskId].func = Task_PrepareToGiveExpWithExpBar;
    }
}

static void Task_PrepareToGiveExpWithExpBar(u8 taskId) {
    u8 monIndex = gTasks[taskId].tExpTask_monId;
    s32 gainedExp = GetTaskExpValue(taskId);
    u8 battlerId = gTasks[taskId].tExpTask_battler;
    struct Pokemon *mon = &gPlayerParty[monIndex];
    u8 level = GetMonData(mon, MON_DATA_LEVEL);
    SpeciesEnum species = GetMonData(mon, MON_DATA_SPECIES);
    u32 exp = GetMonData(mon, MON_DATA_EXP);
    u32 currLvlExp = gExperienceTables[gBaseStats[species].growthRate][level];
    u32 expToNextLvl;

    exp -= currLvlExp;
    expToNextLvl = gExperienceTables[gBaseStats[species].growthRate][level + 1] - currLvlExp;
    SetBattleBarStruct(battlerId, gHealthboxSpriteIds[battlerId], expToNextLvl, exp, -gainedExp);
    PlaySE(SE_EXP);
    gTasks[taskId].func = Task_GiveExpWithExpBar;
}

static void Task_GiveExpWithExpBar(u8 taskId) {
    if (gTasks[taskId].tExpTask_frames < 13) {
        gTasks[taskId].tExpTask_frames++;
    } else {
        u8 monId = gTasks[taskId].tExpTask_monId;
        s32 gainedExp = GetTaskExpValue(taskId);
        u8 battlerId = gTasks[taskId].tExpTask_battler;
        s32 newExpPoints;

        newExpPoints = MoveBattleBar(battlerId, gHealthboxSpriteIds[battlerId], EXP_BAR, 0);
        SetHealthboxSpriteVisible(gHealthboxSpriteIds[battlerId]);
        if (newExpPoints == -1)  // The bar has been filled with given exp points.
        {
            u8 level;
            s32 currExp;
            SpeciesEnum species;
            s32 expOnNextLvl;

            m4aSongNumStop(SE_EXP);
            level = GetMonData(&gPlayerParty[monId], MON_DATA_LEVEL);
            currExp = GetMonData(&gPlayerParty[monId], MON_DATA_EXP);
            species = GetMonData(&gPlayerParty[monId], MON_DATA_SPECIES);
            expOnNextLvl = gExperienceTables[gBaseStats[species].growthRate][level + 1];

            if (currExp + gainedExp >= expOnNextLvl) {
                u8 savedActiveBattler;

                SetMonData(&gPlayerParty[monId], MON_DATA_EXP, &expOnNextLvl);
                CalculateMonStats(&gPlayerParty[monId]);
                gainedExp -= expOnNextLvl - currExp;
                savedActiveBattler = gActiveBattler;
                gActiveBattler = battlerId;
                BtlController_EmitTwoReturnValues(1, RET_VALUE_LEVELED_UP, gainedExp);
                gActiveBattler = savedActiveBattler;
                gTasks[taskId].func = Task_LaunchLvlUpAnim;
            } else {
                currExp += gainedExp;
                SetMonData(&gPlayerParty[monId], MON_DATA_EXP, &currExp);
                gBattlerControllerFuncs[battlerId] = CompleteOnInactiveTextPrinter;
                DestroyTask(taskId);
            }
        }
    }
}

static void Task_LaunchLvlUpAnim(u8 taskId) {
    u8 battlerId = gTasks[taskId].tExpTask_battler;
    u8 monIndex = gTasks[taskId].tExpTask_monId;

    if (IsDoubleBattle() == TRUE && monIndex == gBattlerPartyIndexes[battlerId ^ BIT_FLANK]) battlerId ^= BIT_FLANK;

    InitAndLaunchSpecialAnimation(battlerId, battlerId, battlerId, B_ANIM_LVL_UP);
    gTasks[taskId].func = Task_UpdateLvlInHealthbox;
}

static void Task_UpdateLvlInHealthbox(u8 taskId) {
    u8 battlerId = gTasks[taskId].tExpTask_battler;

    if (!gBattleSpritesDataPtr->healthBoxesData[battlerId].specialAnimActive) {
        u8 monIndex = gTasks[taskId].tExpTask_monId;

        GetMonData(&gPlayerParty[monIndex], MON_DATA_LEVEL);  // Unused return value.

        if (IsDoubleBattle() == TRUE && monIndex == gBattlerPartyIndexes[battlerId ^ BIT_FLANK])
            UpdateHealthboxAttribute(gHealthboxSpriteIds[battlerId ^ BIT_FLANK], &gPlayerParty[monIndex], HEALTHBOX_ALL);
        else
            UpdateHealthboxAttribute(gHealthboxSpriteIds[battlerId], &gPlayerParty[monIndex], HEALTHBOX_ALL);

        gTasks[taskId].func = DestroyExpTaskAndCompleteOnInactiveTextPrinter;
    }
}

static void DestroyExpTaskAndCompleteOnInactiveTextPrinter(u8 taskId) {
    u8 monIndex;
    u8 battlerId;

    monIndex = gTasks[taskId].tExpTask_monId;
    GetMonData(&gPlayerParty[monIndex], MON_DATA_LEVEL);  // Unused return value.
    battlerId = gTasks[taskId].tExpTask_battler;
    gBattlerControllerFuncs[battlerId] = CompleteOnInactiveTextPrinter;
    DestroyTask(taskId);
}

static void FreeMonSpriteAfterFaintAnim(void) {
    if (gSprites[gBattlerSpriteIds[gActiveBattler]].y + gSprites[gBattlerSpriteIds[gActiveBattler]].y2 > DISPLAY_HEIGHT) {
        SpeciesEnum species = GetMonData(&gPlayerParty[gBattlerPartyIndexes[gActiveBattler]], MON_DATA_SPECIES);

        BattleGfxSfxDummy2(species);
        FreeOamMatrix(gSprites[gBattlerSpriteIds[gActiveBattler]].oam.matrixNum);
        DestroySprite(&gSprites[gBattlerSpriteIds[gActiveBattler]]);
        SetHealthboxSpriteInvisible(gHealthboxSpriteIds[gActiveBattler]);
        PlayerBufferExecCompleted();
    }
}

static void FreeMonSpriteAfterSwitchOutAnim(void) {
    if (!gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].specialAnimActive) {
        FreeSpriteOamMatrix(&gSprites[gBattlerSpriteIds[gActiveBattler]]);
        DestroySprite(&gSprites[gBattlerSpriteIds[gActiveBattler]]);
        SetHealthboxSpriteInvisible(gHealthboxSpriteIds[gActiveBattler]);
        PlayerBufferExecCompleted();
    }
}

static void CompleteOnInactiveTextPrinter2(void) {
    if (!IsTextPrinterActive(0)) PlayerBufferExecCompleted();
}

static void OpenPartyMenuToChooseMon(void) {
    if (!gPaletteFade.active) {
        u8 caseId;

        gBattlerControllerFuncs[gActiveBattler] = WaitForMonSelection;
        caseId = gTasks[gBattleControllerData[gActiveBattler]].data[0];
        DestroyTask(gBattleControllerData[gActiveBattler]);
        FreeAllWindowBuffers();
        OpenPartyMenuInBattle(caseId);
    }
}

static void WaitForMonSelection(void) {
    if (gMain.callback2 == BattleMainCB2 && !gPaletteFade.active) {
        if (gPartyMenuUseExitCallback == TRUE)
            BtlController_EmitChosenMonReturnValue(1, gSelectedMonPartyId, gBattlePartyCurrentOrder);
        else
            BtlController_EmitChosenMonReturnValue(1, PARTY_SIZE, NULL);

        if ((gBattleResources->bufferA[gActiveBattler][1] & 0xF) == 1) PrintLinkStandbyMsg();

        PlayerBufferExecCompleted();
    }
}

static void OpenBagAndChooseItem(void) {
    if (!gPaletteFade.active) {
        gBattlerControllerFuncs[gActiveBattler] = CompleteWhenChoseItem;
        ReshowBattleScreenDummy();
        FreeAllWindowBuffers();
        CB2_BagMenuFromBattle();
    }
}

static void CompleteWhenChoseItem(void) {
    if (gMain.callback2 == BattleMainCB2 && !gPaletteFade.active) {
        BtlController_EmitOneReturnValue(1, gSpecialVar_ItemId);
        PlayerBufferExecCompleted();
    }
}

static void CompleteOnSpecialAnimDone(void) {
    if (!gDoingBattleAnim || !gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].specialAnimActive) PlayerBufferExecCompleted();
}

static void DoHitAnimBlinkSpriteEffect(void) {
    u8 spriteId = gBattlerSpriteIds[gActiveBattler];

    if (gSprites[spriteId].data[1] == 32) {
        gSprites[spriteId].data[1] = 0;
        gSprites[spriteId].invisible = FALSE;
        gDoingBattleAnim = FALSE;
        PlayerBufferExecCompleted();
    } else {
        if ((gSprites[spriteId].data[1] % 4) == 0) gSprites[spriteId].invisible ^= 1;
        gSprites[spriteId].data[1]++;
    }
}

static void PlayerHandleYesNoInput(void) {
    if (JOY_NEW(DPAD_UP) && gMultiUsePlayerCursor != 0) {
        PlaySE(SE_SELECT);
        BattleDestroyYesNoCursorAt(gMultiUsePlayerCursor);
        gMultiUsePlayerCursor = 0;
        BattleCreateYesNoCursorAt(0);
    }
    if (JOY_NEW(DPAD_DOWN) && gMultiUsePlayerCursor == 0) {
        PlaySE(SE_SELECT);
        BattleDestroyYesNoCursorAt(gMultiUsePlayerCursor);
        gMultiUsePlayerCursor = 1;
        BattleCreateYesNoCursorAt(1);
    }
    if (JOY_NEW(A_BUTTON)) {
        HandleBattleWindow(BATTLE_BOX_YES_NO_Y, 8, BATTLE_BOX_YES_NO_Y + BATTLE_BOX_YES_NO_WIDTH, 13, WINDOW_CLEAR);
        PlaySE(SE_SELECT);

        if (gMultiUsePlayerCursor != 0)
            BtlController_EmitTwoReturnValues(1, 0xE, 0);
        else
            BtlController_EmitTwoReturnValues(1, 0xD, 0);

        PlayerBufferExecCompleted();
    }
    if (JOY_NEW(B_BUTTON)) {
        HandleBattleWindow(BATTLE_BOX_YES_NO_Y, 8, BATTLE_BOX_YES_NO_Y + BATTLE_BOX_YES_NO_WIDTH, 13, WINDOW_CLEAR);
        PlaySE(SE_SELECT);
        PlayerBufferExecCompleted();
    }
}

static void MoveSelectionDisplayMoveNames(void) {}

static void MoveSelectionDisplayPpString(void) {}

static void MoveSelectionDisplayPpNumber(void) {}

u8 GetMoveTypeEffectiveness(u16 moveNum, u8 targetId, u8 userId, u16 moveType, u16 typeEffectivenessMultiplier) {
    int abilityNullifiesDamage = FALSE;
    u16 mod = typeEffectivenessMultiplier;
    int target = GetBattlerBattleMoveTargetFlags(moveNum, userId);

    if (gBattleMoves[moveNum].split == SPLIT_STATUS) return GetMoveTypeEffectivenessStatus(moveNum, targetId, userId);

    if (IsBattlerAlive(BATTLE_PARTNER(targetId)) && target == MOVE_TARGET_SELECTED &&
        HasRedirectionAbility(userId, BATTLE_PARTNER(targetId), moveNum, moveType))
        abilityNullifiesDamage = TRUE;

    if (!abilityNullifiesDamage) CalculateAbilityMultipliers(userId, targetId, moveNum, moveType, 100, typeEffectivenessMultiplier, FALSE, &mod);

    if (abilityNullifiesDamage)
        return MOVE_EFFECTIVENESS_NONE;
    else if (mod >= UQ_4_12(2.0))
        return MOVE_EFFECTIVENESS_DOUBLE;
    else if (mod <= UQ_4_12(0.5))
        return MOVE_EFFECTIVENESS_HALF;
    else
        return MOVE_EFFECTIVENESS_NORMAL;
}

static u8 GetMoveTypeEffectivenessStatus(u16 moveNum, u8 targetId, u8 userId) {
    bool8 moveNullified = FALSE;

    if (BattlerHasAbility(userId, ABILITY_MYCELIUM_MIGHT, TRUE) && gBattleMoves[moveNum].split == SPLIT_STATUS &&
        gBattleMoves[moveNum].target & !MOVE_TARGET_USER) {
        switch (gBattleMoves[moveNum].effect) {
            case EFFECT_SLEEP:
            case EFFECT_TOXIC:
            case EFFECT_POISON:
            case EFFECT_WILL_O_WISP:
            case EFFECT_FROSTBITE:
            case EFFECT_PARALYZE_IGNORE_TYPE:
            case EFFECT_PARALYZE:
                return gBattleMons[targetId].status1 & STATUS1_ANY ? MOVE_EFFECTIVENESS_NONE : MOVE_EFFECTIVENESS_STATUS;
            case EFFECT_CONFUSE:
                return gBattleMons[targetId].status2 & STATUS2_CONFUSION ? MOVE_EFFECTIVENESS_NONE : MOVE_EFFECTIVENESS_STATUS;
            default:
                return MOVE_EFFECTIVENESS_STATUS;
        }
    }

    if (BATTLER_HAS_ABILITY(targetId, ABILITY_GOOD_AS_GOLD) && IS_MOVE_STATUS(moveNum) && targetId != userId) moveNullified = TRUE;

    // Move Effects
    switch (gBattleMoves[moveNum].effect) {
        case EFFECT_SLEEP:
            if (!CanSleep(targetId)) moveNullified = TRUE;
            break;
        case EFFECT_TOXIC:
        case EFFECT_POISON:
            if (!CanBePoisoned(userId, targetId, moveNum)) moveNullified = TRUE;
            break;
        case EFFECT_WILL_O_WISP:
            if (!CanBeBurned(targetId)) moveNullified = TRUE;
            break;
        case EFFECT_PARALYZE_IGNORE_TYPE:
            if (!CanBeParalyzedIgnoreType(userId, targetId)) moveNullified = TRUE;
            break;
        case EFFECT_PARALYZE:
            if (!CanBeParalyzed(userId, targetId)) moveNullified = TRUE;
            break;
        case EFFECT_CONFUSE:
            if (!CanBeConfused(targetId)) moveNullified = TRUE;
            break;
    }

    // Powder moves don't work on grass types
    if (IS_BATTLER_OF_TYPE(targetId, TYPE_GRASS)) {
        if (TestMoveFlags(moveNum, FLAG_POWDER)) {
            moveNullified = TRUE;
        }
    }

    if (moveNullified)
        return MOVE_EFFECTIVENESS_NONE;
    else
        return MOVE_EFFECTIVENESS_STATUS;
}

static void MoveSelectionDisplayMoveTypeDoubles(u8 targetId) {}

static void MoveSelectionDisplayMoveType(void) {}

static void MoveSelectionCreateCursorAt(u8 cursorPosition, u8 arg1) {}

static void MoveSelectionDestroyCursorAt(u8 cursorPosition) {}

void ActionSelectionCreateCursorAt(u8 cursorPosition, u8 arg1) {}

void ActionSelectionDestroyCursorAt(u8 cursorPosition) {}

void CB2_SetUpReshowBattleScreenAfterMenu(void) { SetMainCallback2(ReshowBattleScreenAfterMenu); }

void CB2_SetUpReshowBattleScreenAfterMenu2(void) { SetMainCallback2(ReshowBattleScreenAfterMenu); }

static void CompleteOnFinishedStatusAnimation(void) {
    if (!gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].statusAnimActive) PlayerBufferExecCompleted();
}

static void CompleteOnFinishedBattleAnimation(void) {
    if (!gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].animFromTableActive) PlayerBufferExecCompleted();
}

static void PrintLinkStandbyMsg(void) {
    if (gBattleTypeFlags & BATTLE_TYPE_LINK) {
        gBattle_BG0_X = 0;
        gBattle_BG0_Y = 0;
        BattlePutTextOnWindow(gText_LinkStandby, B_WIN_MSG);
    }
}

static void PlayerHandleGetMonData(void) {
    u8 monData[sizeof(struct Pokemon) * 2 + 56];  // this allows to get full data of two pokemon, trying to get more will result in overwriting data
    u32 size = 0;
    u8 monToCheck;
    s32 i;

    if (gBattleResources->bufferA[gActiveBattler][2] == 0) {
        size += CopyPlayerMonData(gBattlerPartyIndexes[gActiveBattler], monData);
    } else {
        monToCheck = gBattleResources->bufferA[gActiveBattler][2];
        for (i = 0; i < PARTY_SIZE; i++) {
            if (monToCheck & 1) size += CopyPlayerMonData(i, monData + size);
            monToCheck >>= 1;
        }
    }
    BtlController_EmitDataTransfer(1, size, monData);
    PlayerBufferExecCompleted();
}

static u32 CopyPlayerMonData(u8 monId, u8 *dst) {
    struct BattlePokemon battleMon;
    struct MovePpInfo moveData;
    u8 nickname[20];
    u8 *src;
    s16 data16;
    u32 data32;
    s32 size = 0;

    switch (gBattleResources->bufferA[gActiveBattler][1]) {
        case REQUEST_ALL_BATTLE:
            battleMon.species = GetMonData(&gPlayerParty[monId], MON_DATA_SPECIES);
            battleMon.item = GetMonData(&gPlayerParty[monId], MON_DATA_HELD_ITEM);
            for (size = 0; size < MAX_MON_MOVES; size++) {
                battleMon.moves[size] = GetMonData(&gPlayerParty[monId], MON_DATA_MOVE1 + size);
                battleMon.pp[size] = GetMonData(&gPlayerParty[monId], MON_DATA_PP1 + size);
            }
            battleMon.ppBonuses = GetMonData(&gPlayerParty[monId], MON_DATA_PP_BONUSES);
            battleMon.friendship = GetMonData(&gPlayerParty[monId], MON_DATA_FRIENDSHIP);
            battleMon.experience = GetMonData(&gPlayerParty[monId], MON_DATA_EXP);
            battleMon.speedDown = !GetMonData(&gPlayerParty[monId], MON_DATA_SPEED_IV);
            battleMon.personality = GetMonData(&gPlayerParty[monId], MON_DATA_PERSONALITY);
            battleMon.status1 = GetMonData(&gPlayerParty[monId], MON_DATA_STATUS);
            battleMon.level = GetMonData(&gPlayerParty[monId], MON_DATA_LEVEL);
            battleMon.hp = GetMonData(&gPlayerParty[monId], MON_DATA_HP);
            battleMon.maxHP = GetMonData(&gPlayerParty[monId], MON_DATA_MAX_HP);
            battleMon.attack = GetMonData(&gPlayerParty[monId], MON_DATA_ATK);
            battleMon.defense = GetMonData(&gPlayerParty[monId], MON_DATA_DEF);
            battleMon.speed = GetMonData(&gPlayerParty[monId], MON_DATA_SPEED);
            battleMon.spAttack = GetMonData(&gPlayerParty[monId], MON_DATA_SPATK);
            battleMon.spDefense = GetMonData(&gPlayerParty[monId], MON_DATA_SPDEF);
            battleMon.abilityNum = GetMonData(&gPlayerParty[monId], MON_DATA_ABILITY_NUM);
            battleMon.otId = GetMonData(&gPlayerParty[monId], MON_DATA_OT_ID);
            battleMon.nature = GetMonData(&gPlayerParty[monId], MON_DATA_NATURE);
            battleMon.hpType = GetMonData(&gPlayerParty[monId], MON_DATA_HP_TYPE);
            battleMon.abilities[0] = RandomizeAbility(GetAbilityBySpecies(battleMon.species, battleMon.abilityNum), battleMon.species, battleMon.personality);
            battleMon.abilities[1] = GetInnateInSlot(battleMon.level, battleMon.species, 0, battleMon.personality, TRUE);
            battleMon.abilities[2] = GetInnateInSlot(battleMon.level, battleMon.species, 1, battleMon.personality, TRUE);
            battleMon.abilities[3] = GetInnateInSlot(battleMon.level, battleMon.species, 2, battleMon.personality, TRUE);
            GetMonData(&gPlayerParty[monId], MON_DATA_NICKNAME, nickname);
            StringCopy10(battleMon.nickname, nickname);
            GetMonData(&gPlayerParty[monId], MON_DATA_OT_NAME, battleMon.otName);
            src = (u8 *)&battleMon;
            for (size = 0; size < sizeof(battleMon); size++) dst[size] = src[size];
            break;
        case REQUEST_SPECIES_BATTLE:
            data16 = GetMonData(&gPlayerParty[monId], MON_DATA_SPECIES);
            dst[0] = data16;
            dst[1] = data16 >> 8;
            size = 2;
            break;
        case REQUEST_HELDITEM_BATTLE:
            data16 = GetMonData(&gPlayerParty[monId], MON_DATA_HELD_ITEM);
            dst[0] = data16;
            dst[1] = data16 >> 8;
            size = 2;
            break;
        case REQUEST_MOVES_PP_BATTLE:
            for (size = 0; size < MAX_MON_MOVES; size++) {
                moveData.moves[size] = GetMonData(&gPlayerParty[monId], MON_DATA_MOVE1 + size);
                moveData.pp[size] = GetMonData(&gPlayerParty[monId], MON_DATA_PP1 + size);
            }
            moveData.ppBonuses = GetMonData(&gPlayerParty[monId], MON_DATA_PP_BONUSES);
            src = (u8 *)(&moveData);
            for (size = 0; size < sizeof(moveData); size++) dst[size] = src[size];
            break;
        case REQUEST_MOVE1_BATTLE:
        case REQUEST_MOVE2_BATTLE:
        case REQUEST_MOVE3_BATTLE:
        case REQUEST_MOVE4_BATTLE:
            data16 = GetMonData(&gPlayerParty[monId], MON_DATA_MOVE1 + gBattleResources->bufferA[gActiveBattler][1] - REQUEST_MOVE1_BATTLE);
            dst[0] = data16;
            dst[1] = data16 >> 8;
            size = 2;
            break;
        case REQUEST_PP_DATA_BATTLE:
            for (size = 0; size < MAX_MON_MOVES; size++) dst[size] = GetMonData(&gPlayerParty[monId], MON_DATA_PP1 + size);
            dst[size] = GetMonData(&gPlayerParty[monId], MON_DATA_PP_BONUSES);
            size++;
            break;
        case REQUEST_PPMOVE1_BATTLE:
        case REQUEST_PPMOVE2_BATTLE:
        case REQUEST_PPMOVE3_BATTLE:
        case REQUEST_PPMOVE4_BATTLE:
            dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_PP1 + gBattleResources->bufferA[gActiveBattler][1] - REQUEST_PPMOVE1_BATTLE);
            size = 1;
            break;
        case REQUEST_OTID_BATTLE:
            data32 = GetMonData(&gPlayerParty[monId], MON_DATA_OT_ID);
            dst[0] = (data32 & 0x000000FF);
            dst[1] = (data32 & 0x0000FF00) >> 8;
            dst[2] = (data32 & 0x00FF0000) >> 16;
            size = 3;
            break;
        case REQUEST_EXP_BATTLE:
            data32 = GetMonData(&gPlayerParty[monId], MON_DATA_EXP);
            dst[0] = (data32 & 0x000000FF);
            dst[1] = (data32 & 0x0000FF00) >> 8;
            dst[2] = (data32 & 0x00FF0000) >> 16;
            size = 3;
            break;
        case REQUEST_HP_EV_BATTLE:
            dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_HP_EV);
            size = 1;
            break;
        case REQUEST_ATK_EV_BATTLE:
            dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_ATK_EV);
            size = 1;
            break;
        case REQUEST_DEF_EV_BATTLE:
            dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_DEF_EV);
            size = 1;
            break;
        case REQUEST_SPEED_EV_BATTLE:
            dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_SPEED_EV);
            size = 1;
            break;
        case REQUEST_SPATK_EV_BATTLE:
            dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_SPATK_EV);
            size = 1;
            break;
        case REQUEST_SPDEF_EV_BATTLE:
            dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_SPDEF_EV);
            size = 1;
            break;
        case REQUEST_FRIENDSHIP_BATTLE:
            dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_FRIENDSHIP);
            size = 1;
            break;
        case REQUEST_POKERUS_BATTLE:
            dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_POKERUS);
            size = 1;
            break;
        case REQUEST_MET_LOCATION_BATTLE:
            dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_MET_LOCATION);
            size = 1;
            break;
        case REQUEST_MET_LEVEL_BATTLE:
            dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_MET_LEVEL);
            size = 1;
            break;
        case REQUEST_MET_GAME_BATTLE:
            dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_MET_GAME);
            size = 1;
            break;
        case REQUEST_POKEBALL_BATTLE:
            dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_POKEBALL);
            size = 1;
            break;
        case REQUEST_ALL_IVS_BATTLE:
            dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_HP_IV);
            dst[1] = GetMonData(&gPlayerParty[monId], MON_DATA_ATK_IV);
            dst[2] = GetMonData(&gPlayerParty[monId], MON_DATA_DEF_IV);
            dst[3] = GetMonData(&gPlayerParty[monId], MON_DATA_SPEED_IV);
            dst[4] = GetMonData(&gPlayerParty[monId], MON_DATA_SPATK_IV);
            dst[5] = GetMonData(&gPlayerParty[monId], MON_DATA_SPDEF_IV);
            size = 6;
            break;
        case REQUEST_HP_IV_BATTLE:
            dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_HP_IV);
            size = 1;
            break;
        case REQUEST_ATK_IV_BATTLE:
            dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_ATK_IV);
            size = 1;
            break;
        case REQUEST_DEF_IV_BATTLE:
            dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_DEF_IV);
            size = 1;
            break;
        case REQUEST_SPEED_IV_BATTLE:
            dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_SPEED_IV);
            size = 1;
            break;
        case REQUEST_SPATK_IV_BATTLE:
            dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_SPATK_IV);
            size = 1;
            break;
        case REQUEST_SPDEF_IV_BATTLE:
            dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_SPDEF_IV);
            size = 1;
            break;
        case REQUEST_PERSONALITY_BATTLE:
            data32 = GetMonData(&gPlayerParty[monId], MON_DATA_PERSONALITY);
            dst[0] = (data32 & 0x000000FF);
            dst[1] = (data32 & 0x0000FF00) >> 8;
            dst[2] = (data32 & 0x00FF0000) >> 16;
            dst[3] = (data32 & 0xFF000000) >> 24;
            size = 4;
            break;
        case REQUEST_CHECKSUM_BATTLE:
            data16 = GetMonData(&gPlayerParty[monId], MON_DATA_CHECKSUM);
            dst[0] = data16;
            dst[1] = data16 >> 8;
            size = 2;
            break;
        case REQUEST_STATUS_BATTLE:
            data32 = GetMonData(&gPlayerParty[monId], MON_DATA_STATUS);
            dst[0] = (data32 & 0x000000FF);
            dst[1] = (data32 & 0x0000FF00) >> 8;
            dst[2] = (data32 & 0x00FF0000) >> 16;
            dst[3] = (data32 & 0xFF000000) >> 24;
            size = 4;
            break;
        case REQUEST_LEVEL_BATTLE:
            dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_LEVEL);
            size = 1;
            break;
        case REQUEST_HP_BATTLE:
            data16 = GetMonData(&gPlayerParty[monId], MON_DATA_HP);
            dst[0] = data16;
            dst[1] = data16 >> 8;
            size = 2;
            break;
        case REQUEST_MAX_HP_BATTLE:
            data16 = GetMonData(&gPlayerParty[monId], MON_DATA_MAX_HP);
            dst[0] = data16;
            dst[1] = data16 >> 8;
            size = 2;
            break;
        case REQUEST_ATK_BATTLE:
            data16 = GetMonData(&gPlayerParty[monId], MON_DATA_ATK);
            dst[0] = data16;
            dst[1] = data16 >> 8;
            size = 2;
            break;
        case REQUEST_DEF_BATTLE:
            data16 = GetMonData(&gPlayerParty[monId], MON_DATA_DEF);
            dst[0] = data16;
            dst[1] = data16 >> 8;
            size = 2;
            break;
        case REQUEST_SPEED_BATTLE:
            data16 = GetMonData(&gPlayerParty[monId], MON_DATA_SPEED);
            dst[0] = data16;
            dst[1] = data16 >> 8;
            size = 2;
            break;
        case REQUEST_SPATK_BATTLE:
            data16 = GetMonData(&gPlayerParty[monId], MON_DATA_SPATK);
            dst[0] = data16;
            dst[1] = data16 >> 8;
            size = 2;
            break;
        case REQUEST_SPDEF_BATTLE:
            data16 = GetMonData(&gPlayerParty[monId], MON_DATA_SPDEF);
            dst[0] = data16;
            dst[1] = data16 >> 8;
            size = 2;
            break;
        case REQUEST_COOL_BATTLE:
            dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_COOL);
            size = 1;
            break;
        case REQUEST_BEAUTY_BATTLE:
            dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_BEAUTY);
            size = 1;
            break;
        case REQUEST_CUTE_BATTLE:
            dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_CUTE);
            size = 1;
            break;
        case REQUEST_SMART_BATTLE:
            dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_SMART);
            size = 1;
            break;
        case REQUEST_TOUGH_BATTLE:
            dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_TOUGH);
            size = 1;
            break;
        case REQUEST_SHEEN_BATTLE:
            dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_SHEEN);
            size = 1;
            break;
        case REQUEST_COOL_RIBBON_BATTLE:
            dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_COOL_RIBBON);
            size = 1;
            break;
        case REQUEST_BEAUTY_RIBBON_BATTLE:
            dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_BEAUTY_RIBBON);
            size = 1;
            break;
        case REQUEST_CUTE_RIBBON_BATTLE:
            dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_CUTE_RIBBON);
            size = 1;
            break;
        case REQUEST_SMART_RIBBON_BATTLE:
            dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_SMART_RIBBON);
            size = 1;
            break;
        case REQUEST_TOUGH_RIBBON_BATTLE:
            dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_TOUGH_RIBBON);
            size = 1;
            break;
        case REQUEST_NATURE_BATTLE:
            dst[0] = GetMonData(&gPlayerParty[monId], MON_DATA_NATURE);
            size = 1;
            break;
    }

    return size;
}

void PlayerHandleGetRawMonData(void) {
    struct BattlePokemon battleMon;
    u8 *src = (u8 *)&gPlayerParty[gBattlerPartyIndexes[gActiveBattler]] + gBattleResources->bufferA[gActiveBattler][1];
    u8 *dst = (u8 *)&battleMon + gBattleResources->bufferA[gActiveBattler][1];
    u8 i;

    for (i = 0; i < gBattleResources->bufferA[gActiveBattler][2]; i++) dst[i] = src[i];

    BtlController_EmitDataTransfer(1, gBattleResources->bufferA[gActiveBattler][2], dst);
    PlayerBufferExecCompleted();
}

static void PlayerHandleSetMonData(void) {
    u8 monToCheck;
    u8 i;

    if (gBattleResources->bufferA[gActiveBattler][2] == 0) {
        SetPlayerMonData(gBattlerPartyIndexes[gActiveBattler]);
    } else {
        monToCheck = gBattleResources->bufferA[gActiveBattler][2];
        for (i = 0; i < PARTY_SIZE; i++) {
            if (monToCheck & 1) SetPlayerMonData(i);
            monToCheck >>= 1;
        }
    }
    PlayerBufferExecCompleted();
}

static void SetPlayerMonData(u8 monId) {
    struct BattlePokemon *battlePokemon = (struct BattlePokemon *)&gBattleResources->bufferA[gActiveBattler][3];
    struct MovePpInfo *moveData = (struct MovePpInfo *)&gBattleResources->bufferA[gActiveBattler][3];
    s32 i;

    switch (gBattleResources->bufferA[gActiveBattler][1]) {
        case REQUEST_ALL_BATTLE: {
            u8 iv;

            SetMonData(&gPlayerParty[monId], MON_DATA_SPECIES, &battlePokemon->species);
            SetMonData(&gPlayerParty[monId], MON_DATA_HELD_ITEM, &battlePokemon->item);
            for (i = 0; i < MAX_MON_MOVES; i++) {
                SetMonData(&gPlayerParty[monId], MON_DATA_MOVE1 + i, &battlePokemon->moves[i]);
                SetMonData(&gPlayerParty[monId], MON_DATA_PP1 + i, &battlePokemon->pp[i]);
            }
            SetMonData(&gPlayerParty[monId], MON_DATA_PP_BONUSES, &battlePokemon->ppBonuses);
            SetMonData(&gPlayerParty[monId], MON_DATA_FRIENDSHIP, &battlePokemon->friendship);
            SetMonData(&gPlayerParty[monId], MON_DATA_EXP, &battlePokemon->experience);
            SetMonData(&gPlayerParty[monId], MON_DATA_SPDEF_IV, &iv);
            SetMonData(&gPlayerParty[monId], MON_DATA_PERSONALITY, &battlePokemon->personality);
            SetMonData(&gPlayerParty[monId], MON_DATA_STATUS, &battlePokemon->status1);
            SetMonData(&gPlayerParty[monId], MON_DATA_LEVEL, &battlePokemon->level);
            SetMonData(&gPlayerParty[monId], MON_DATA_HP, &battlePokemon->hp);
            SetMonData(&gPlayerParty[monId], MON_DATA_MAX_HP, &battlePokemon->maxHP);
            SetMonData(&gPlayerParty[monId], MON_DATA_ATK, &battlePokemon->attack);
            SetMonData(&gPlayerParty[monId], MON_DATA_DEF, &battlePokemon->defense);
            SetMonData(&gPlayerParty[monId], MON_DATA_SPEED, &battlePokemon->speed);
            SetMonData(&gPlayerParty[monId], MON_DATA_SPATK, &battlePokemon->spAttack);
            SetMonData(&gPlayerParty[monId], MON_DATA_SPDEF, &battlePokemon->spDefense);
            SetMonData(&gPlayerParty[monId], MON_DATA_NATURE, &battlePokemon->nature);
            SetMonData(&gPlayerParty[monId], MON_DATA_HP_TYPE, &battlePokemon->hpType);
        } break;
        case REQUEST_SPECIES_BATTLE:
            SetMonData(&gPlayerParty[monId], MON_DATA_SPECIES, &gBattleResources->bufferA[gActiveBattler][3]);
            break;
        case REQUEST_HELDITEM_BATTLE:
            SetMonData(&gPlayerParty[monId], MON_DATA_HELD_ITEM, &gBattleResources->bufferA[gActiveBattler][3]);
            break;
        case REQUEST_MOVES_PP_BATTLE:
            for (i = 0; i < MAX_MON_MOVES; i++) {
                SetMonData(&gPlayerParty[monId], MON_DATA_MOVE1 + i, &moveData->moves[i]);
                SetMonData(&gPlayerParty[monId], MON_DATA_PP1 + i, &moveData->pp[i]);
            }
            SetMonData(&gPlayerParty[monId], MON_DATA_PP_BONUSES, &moveData->ppBonuses);
            break;
        case REQUEST_MOVE1_BATTLE:
        case REQUEST_MOVE2_BATTLE:
        case REQUEST_MOVE3_BATTLE:
        case REQUEST_MOVE4_BATTLE:
            SetMonData(&gPlayerParty[monId],
                       MON_DATA_MOVE1 + gBattleResources->bufferA[gActiveBattler][1] - REQUEST_MOVE1_BATTLE,
                       &gBattleResources->bufferA[gActiveBattler][3]);
            break;
        case REQUEST_PP_DATA_BATTLE:
            SetMonData(&gPlayerParty[monId], MON_DATA_PP1, &gBattleResources->bufferA[gActiveBattler][3]);
            SetMonData(&gPlayerParty[monId], MON_DATA_PP2, &gBattleResources->bufferA[gActiveBattler][4]);
            SetMonData(&gPlayerParty[monId], MON_DATA_PP3, &gBattleResources->bufferA[gActiveBattler][5]);
            SetMonData(&gPlayerParty[monId], MON_DATA_PP4, &gBattleResources->bufferA[gActiveBattler][6]);
            SetMonData(&gPlayerParty[monId], MON_DATA_PP_BONUSES, &gBattleResources->bufferA[gActiveBattler][7]);
            break;
        case REQUEST_PPMOVE1_BATTLE:
        case REQUEST_PPMOVE2_BATTLE:
        case REQUEST_PPMOVE3_BATTLE:
        case REQUEST_PPMOVE4_BATTLE:
            SetMonData(&gPlayerParty[monId],
                       MON_DATA_PP1 + gBattleResources->bufferA[gActiveBattler][1] - REQUEST_PPMOVE1_BATTLE,
                       &gBattleResources->bufferA[gActiveBattler][3]);
            break;
        case REQUEST_OTID_BATTLE:
            SetMonData(&gPlayerParty[monId], MON_DATA_OT_ID, &gBattleResources->bufferA[gActiveBattler][3]);
            break;
        case REQUEST_EXP_BATTLE:
            SetMonData(&gPlayerParty[monId], MON_DATA_EXP, &gBattleResources->bufferA[gActiveBattler][3]);
            break;
        case REQUEST_HP_EV_BATTLE:
            SetMonData(&gPlayerParty[monId], MON_DATA_HP_EV, &gBattleResources->bufferA[gActiveBattler][3]);
            break;
        case REQUEST_ATK_EV_BATTLE:
            SetMonData(&gPlayerParty[monId], MON_DATA_ATK_EV, &gBattleResources->bufferA[gActiveBattler][3]);
            break;
        case REQUEST_DEF_EV_BATTLE:
            SetMonData(&gPlayerParty[monId], MON_DATA_DEF_EV, &gBattleResources->bufferA[gActiveBattler][3]);
            break;
        case REQUEST_SPEED_EV_BATTLE:
            SetMonData(&gPlayerParty[monId], MON_DATA_SPEED_EV, &gBattleResources->bufferA[gActiveBattler][3]);
            break;
        case REQUEST_SPATK_EV_BATTLE:
            SetMonData(&gPlayerParty[monId], MON_DATA_SPATK_EV, &gBattleResources->bufferA[gActiveBattler][3]);
            break;
        case REQUEST_SPDEF_EV_BATTLE:
            SetMonData(&gPlayerParty[monId], MON_DATA_SPDEF_EV, &gBattleResources->bufferA[gActiveBattler][3]);
            break;
        case REQUEST_FRIENDSHIP_BATTLE:
            SetMonData(&gPlayerParty[monId], MON_DATA_FRIENDSHIP, &gBattleResources->bufferA[gActiveBattler][3]);
            break;
        case REQUEST_POKERUS_BATTLE:
            SetMonData(&gPlayerParty[monId], MON_DATA_POKERUS, &gBattleResources->bufferA[gActiveBattler][3]);
            break;
        case REQUEST_MET_LOCATION_BATTLE:
            SetMonData(&gPlayerParty[monId], MON_DATA_MET_LOCATION, &gBattleResources->bufferA[gActiveBattler][3]);
            break;
        case REQUEST_MET_LEVEL_BATTLE:
            SetMonData(&gPlayerParty[monId], MON_DATA_MET_LEVEL, &gBattleResources->bufferA[gActiveBattler][3]);
            break;
        case REQUEST_MET_GAME_BATTLE:
            SetMonData(&gPlayerParty[monId], MON_DATA_MET_GAME, &gBattleResources->bufferA[gActiveBattler][3]);
            break;
        case REQUEST_POKEBALL_BATTLE:
            SetMonData(&gPlayerParty[monId], MON_DATA_POKEBALL, &gBattleResources->bufferA[gActiveBattler][3]);
            break;
        case REQUEST_ALL_IVS_BATTLE:
            SetMonData(&gPlayerParty[monId], MON_DATA_HP_IV, &gBattleResources->bufferA[gActiveBattler][3]);
            SetMonData(&gPlayerParty[monId], MON_DATA_ATK_IV, &gBattleResources->bufferA[gActiveBattler][4]);
            SetMonData(&gPlayerParty[monId], MON_DATA_DEF_IV, &gBattleResources->bufferA[gActiveBattler][5]);
            SetMonData(&gPlayerParty[monId], MON_DATA_SPEED_IV, &gBattleResources->bufferA[gActiveBattler][6]);
            SetMonData(&gPlayerParty[monId], MON_DATA_SPATK_IV, &gBattleResources->bufferA[gActiveBattler][7]);
            SetMonData(&gPlayerParty[monId], MON_DATA_SPDEF_IV, &gBattleResources->bufferA[gActiveBattler][8]);
            break;
        case REQUEST_HP_IV_BATTLE:
            SetMonData(&gPlayerParty[monId], MON_DATA_HP_IV, &gBattleResources->bufferA[gActiveBattler][3]);
            break;
        case REQUEST_ATK_IV_BATTLE:
            SetMonData(&gPlayerParty[monId], MON_DATA_ATK_IV, &gBattleResources->bufferA[gActiveBattler][3]);
            break;
        case REQUEST_DEF_IV_BATTLE:
            SetMonData(&gPlayerParty[monId], MON_DATA_DEF_IV, &gBattleResources->bufferA[gActiveBattler][3]);
            break;
        case REQUEST_SPEED_IV_BATTLE:
            SetMonData(&gPlayerParty[monId], MON_DATA_SPEED_IV, &gBattleResources->bufferA[gActiveBattler][3]);
            break;
        case REQUEST_SPATK_IV_BATTLE:
            SetMonData(&gPlayerParty[monId], MON_DATA_SPATK_IV, &gBattleResources->bufferA[gActiveBattler][3]);
            break;
        case REQUEST_SPDEF_IV_BATTLE:
            SetMonData(&gPlayerParty[monId], MON_DATA_SPDEF_IV, &gBattleResources->bufferA[gActiveBattler][3]);
            break;
        case REQUEST_PERSONALITY_BATTLE:
            SetMonData(&gPlayerParty[monId], MON_DATA_PERSONALITY, &gBattleResources->bufferA[gActiveBattler][3]);
            break;
        case REQUEST_CHECKSUM_BATTLE:
            SetMonData(&gPlayerParty[monId], MON_DATA_CHECKSUM, &gBattleResources->bufferA[gActiveBattler][3]);
            break;
        case REQUEST_STATUS_BATTLE:
            SetMonData(&gPlayerParty[monId], MON_DATA_STATUS, &gBattleResources->bufferA[gActiveBattler][3]);
            break;
        case REQUEST_LEVEL_BATTLE:
            SetMonData(&gPlayerParty[monId], MON_DATA_LEVEL, &gBattleResources->bufferA[gActiveBattler][3]);
            break;
        case REQUEST_HP_BATTLE:
            SetMonData(&gPlayerParty[monId], MON_DATA_HP, &gBattleResources->bufferA[gActiveBattler][3]);
            break;
        case REQUEST_MAX_HP_BATTLE:
            SetMonData(&gPlayerParty[monId], MON_DATA_MAX_HP, &gBattleResources->bufferA[gActiveBattler][3]);
            break;
        case REQUEST_ATK_BATTLE:
            SetMonData(&gPlayerParty[monId], MON_DATA_ATK, &gBattleResources->bufferA[gActiveBattler][3]);
            break;
        case REQUEST_DEF_BATTLE:
            SetMonData(&gPlayerParty[monId], MON_DATA_DEF, &gBattleResources->bufferA[gActiveBattler][3]);
            break;
        case REQUEST_SPEED_BATTLE:
            SetMonData(&gPlayerParty[monId], MON_DATA_SPEED, &gBattleResources->bufferA[gActiveBattler][3]);
            break;
        case REQUEST_SPATK_BATTLE:
            SetMonData(&gPlayerParty[monId], MON_DATA_SPATK, &gBattleResources->bufferA[gActiveBattler][3]);
            break;
        case REQUEST_SPDEF_BATTLE:
            SetMonData(&gPlayerParty[monId], MON_DATA_SPDEF, &gBattleResources->bufferA[gActiveBattler][3]);
            break;
        case REQUEST_COOL_BATTLE:
            SetMonData(&gPlayerParty[monId], MON_DATA_COOL, &gBattleResources->bufferA[gActiveBattler][3]);
            break;
        case REQUEST_BEAUTY_BATTLE:
            SetMonData(&gPlayerParty[monId], MON_DATA_BEAUTY, &gBattleResources->bufferA[gActiveBattler][3]);
            break;
        case REQUEST_CUTE_BATTLE:
            SetMonData(&gPlayerParty[monId], MON_DATA_CUTE, &gBattleResources->bufferA[gActiveBattler][3]);
            break;
        case REQUEST_SMART_BATTLE:
            SetMonData(&gPlayerParty[monId], MON_DATA_SMART, &gBattleResources->bufferA[gActiveBattler][3]);
            break;
        case REQUEST_TOUGH_BATTLE:
            SetMonData(&gPlayerParty[monId], MON_DATA_TOUGH, &gBattleResources->bufferA[gActiveBattler][3]);
            break;
        case REQUEST_SHEEN_BATTLE:
            SetMonData(&gPlayerParty[monId], MON_DATA_SHEEN, &gBattleResources->bufferA[gActiveBattler][3]);
            break;
        case REQUEST_COOL_RIBBON_BATTLE:
            SetMonData(&gPlayerParty[monId], MON_DATA_COOL_RIBBON, &gBattleResources->bufferA[gActiveBattler][3]);
            break;
        case REQUEST_BEAUTY_RIBBON_BATTLE:
            SetMonData(&gPlayerParty[monId], MON_DATA_BEAUTY_RIBBON, &gBattleResources->bufferA[gActiveBattler][3]);
            break;
        case REQUEST_CUTE_RIBBON_BATTLE:
            SetMonData(&gPlayerParty[monId], MON_DATA_CUTE_RIBBON, &gBattleResources->bufferA[gActiveBattler][3]);
            break;
        case REQUEST_SMART_RIBBON_BATTLE:
            SetMonData(&gPlayerParty[monId], MON_DATA_SMART_RIBBON, &gBattleResources->bufferA[gActiveBattler][3]);
            break;
        case REQUEST_TOUGH_RIBBON_BATTLE:
            SetMonData(&gPlayerParty[monId], MON_DATA_TOUGH_RIBBON, &gBattleResources->bufferA[gActiveBattler][3]);
            break;
        case REQUEST_NATURE_BATTLE:
            SetMonData(&gPlayerParty[monId], MON_DATA_NATURE, &gBattleResources->bufferA[gActiveBattler][3]);
            break;
    }

    HandleLowHpMusicChange(&gPlayerParty[gBattlerPartyIndexes[gActiveBattler]], gActiveBattler);
}

static void PlayerHandleSetRawMonData(void) {
    u8 *dst = (u8 *)&gPlayerParty[gBattlerPartyIndexes[gActiveBattler]] + gBattleResources->bufferA[gActiveBattler][1];
    u8 i;

    for (i = 0; i < gBattleResources->bufferA[gActiveBattler][2]; i++) dst[i] = gBattleResources->bufferA[gActiveBattler][3 + i];

    PlayerBufferExecCompleted();
}

static void PlayerHandleLoadMonSprite(void) {
    BattleLoadPlayerMonSpriteGfx(&gPlayerParty[gBattlerPartyIndexes[gActiveBattler]], gActiveBattler);
    gSprites[gBattlerSpriteIds[gActiveBattler]].oam.paletteNum = gActiveBattler;
    gBattlerControllerFuncs[gActiveBattler] = CompleteOnBankSpritePosX_0;
}

static void PlayerHandleSwitchInAnim(void) {
    ClearTemporarySpeciesSpriteData(gActiveBattler, gBattleResources->bufferA[gActiveBattler][2]);
    gBattlerPartyIndexes[gActiveBattler] = gBattleResources->bufferA[gActiveBattler][1];
    BattleLoadPlayerMonSpriteGfx(&gPlayerParty[gBattlerPartyIndexes[gActiveBattler]], gActiveBattler);
    gActionSelectionCursor[gActiveBattler] = 0;
    gMoveSelectionCursor[gActiveBattler] = 0;
    StartSendOutAnim(gActiveBattler, gBattleResources->bufferA[gActiveBattler][2]);
    gBattlerControllerFuncs[gActiveBattler] = SwitchIn_TryShinyAnimShowHealthbox;
}

static void StartSendOutAnim(u8 battlerId, bool8 dontClearSubstituteBit) {
    SpeciesEnum species;

    ClearTemporarySpeciesSpriteData(battlerId, dontClearSubstituteBit);
    gBattlerPartyIndexes[battlerId] = gBattleResources->bufferA[battlerId][1];
    species = GetMonData(&gPlayerParty[gBattlerPartyIndexes[battlerId]], MON_DATA_SPECIES);
    gBattleControllerData[battlerId] = CreateInvisibleSpriteWithCallback(SpriteCB_WaitForBattlerBallReleaseAnim);
    SetMultiuseSpriteTemplateToPokemon(species, GetBattlerPosition(battlerId));

    gBattlerSpriteIds[battlerId] = CreateSprite(
        &gMultiuseSpriteTemplate, GetBattlerSpriteCoord(battlerId, 2), GetBattlerSpriteDefault_Y(battlerId), GetBattlerSpriteSubpriority(battlerId));

    gSprites[gBattleControllerData[battlerId]].data[1] = gBattlerSpriteIds[battlerId];
    gSprites[gBattleControllerData[battlerId]].data[2] = battlerId;

    gSprites[gBattlerSpriteIds[battlerId]].data[0] = battlerId;
    gSprites[gBattlerSpriteIds[battlerId]].data[2] = species;
    gSprites[gBattlerSpriteIds[battlerId]].oam.paletteNum = battlerId;

    StartSpriteAnim(&gSprites[gBattlerSpriteIds[battlerId]], gBattleMonForms[battlerId]);

    gSprites[gBattlerSpriteIds[battlerId]].invisible = TRUE;
    gSprites[gBattlerSpriteIds[battlerId]].callback = SpriteCallbackDummy;

    gSprites[gBattleControllerData[battlerId]].data[0] = DoPokeballSendOutAnimation(0, POKEBALL_PLAYER_SENDOUT);
}

static void PlayerHandleReturnMonToBall(void) {
    if (gBattleResources->bufferA[gActiveBattler][1] == 0) {
        gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].animationState = 0;
        gBattlerControllerFuncs[gActiveBattler] = DoSwitchOutAnimation;
    } else {
        FreeSpriteOamMatrix(&gSprites[gBattlerSpriteIds[gActiveBattler]]);
        DestroySprite(&gSprites[gBattlerSpriteIds[gActiveBattler]]);
        SetHealthboxSpriteInvisible(gHealthboxSpriteIds[gActiveBattler]);
        PlayerBufferExecCompleted();
    }
}

static void DoSwitchOutAnimation(void) {
    switch (gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].animationState) {
        case 0:
            if (gBattleSpritesDataPtr->battlerData[gActiveBattler].behindSubstitute)
                InitAndLaunchSpecialAnimation(gActiveBattler, gActiveBattler, gActiveBattler, B_ANIM_SUBSTITUTE_TO_MON);

            gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].animationState = 1;
            break;
        case 1:
            if (!gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].specialAnimActive) {
                gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].animationState = 0;
                InitAndLaunchSpecialAnimation(gActiveBattler, gActiveBattler, gActiveBattler, B_ANIM_SWITCH_OUT_PLAYER_MON);
                gBattlerControllerFuncs[gActiveBattler] = FreeMonSpriteAfterSwitchOutAnim;
            }
            break;
    }
}

#define sSpeedX data[0]

// In emerald it's possible to have a tag battle in the battle frontier facilities with AI
// which use the front sprite for both the player and the partner as opposed to any other battles (including the one with Steven)
// that use an animated back pic.
static void PlayerHandleDrawTrainerPic(void) {
    s16 xPos, yPos;
    u32 trainerPicId;

    if (gBattleTypeFlags & BATTLE_TYPE_LINK) {
        if ((gLinkPlayers[GetMultiplayerId()].version & 0xFF) == VERSION_FIRE_RED || (gLinkPlayers[GetMultiplayerId()].version & 0xFF) == VERSION_LEAF_GREEN) {
            trainerPicId = gLinkPlayers[GetMultiplayerId()].gender + TRAINER_BACK_PIC_RED;
        } else if ((gLinkPlayers[GetMultiplayerId()].version & 0xFF) == VERSION_RUBY || (gLinkPlayers[GetMultiplayerId()].version & 0xFF) == VERSION_SAPPHIRE) {
            trainerPicId = gLinkPlayers[GetMultiplayerId()].gender + TRAINER_BACK_PIC_RUBY_SAPPHIRE_BRENDAN;
        } else {
            trainerPicId = gLinkPlayers[GetMultiplayerId()].gender + TRAINER_BACK_PIC_BRENDAN;
        }
    } else {
        trainerPicId = gSaveBlock2Ptr->playerGender;
    }

    if (gBattleTypeFlags & BATTLE_TYPE_MULTI) {
        if ((GetBattlerPosition(gActiveBattler) & BIT_FLANK) != B_FLANK_LEFT)  // Second mon, on the right.
            xPos = 90;
        else  // First mon, on the left.
            xPos = 32;

        if (gBattleTypeFlags & BATTLE_TYPE_INGAME_PARTNER && gPartnerTrainerId != TRAINER_STEVEN_PARTNER && gPartnerTrainerId < TRAINER_CUSTOM_PARTNER) {
            xPos = 90;
            yPos = (8 - gTrainerFrontPicCoords[trainerPicId].size) * 4 + 80;
        } else {
            yPos = (8 - gTrainerBackPicCoords[trainerPicId].size) * 4 + 80;
        }

    } else {
        xPos = 80;
        yPos = (8 - gTrainerBackPicCoords[trainerPicId].size) * 4 + 80;
    }

    // Use front pic table for any tag battles unless your partner is Steven.
    if (gBattleTypeFlags & BATTLE_TYPE_INGAME_PARTNER && gPartnerTrainerId != TRAINER_STEVEN_PARTNER && gPartnerTrainerId < TRAINER_CUSTOM_PARTNER) {
        trainerPicId = PlayerGenderToFrontTrainerPicId(gSaveBlock2Ptr->playerGender);
        DecompressTrainerFrontPic(trainerPicId, gActiveBattler);
        SetMultiuseSpriteTemplateToTrainerFront(trainerPicId, GetBattlerPosition(gActiveBattler));
        gBattlerSpriteIds[gActiveBattler] = CreateSprite(&gMultiuseSpriteTemplate, xPos, yPos, GetBattlerSpriteSubpriority(gActiveBattler));

        gSprites[gBattlerSpriteIds[gActiveBattler]].oam.paletteNum = IndexOfSpritePaletteTag(gTrainerFrontPicPaletteTable[trainerPicId].tag);
        gSprites[gBattlerSpriteIds[gActiveBattler]].x2 = DISPLAY_WIDTH;
        gSprites[gBattlerSpriteIds[gActiveBattler]].y2 = 48;
        gSprites[gBattlerSpriteIds[gActiveBattler]].sSpeedX = -2;
        gSprites[gBattlerSpriteIds[gActiveBattler]].callback = SpriteCB_TrainerSpawn;
        gSprites[gBattlerSpriteIds[gActiveBattler]].oam.affineMode = ST_OAM_AFFINE_OFF;
        gSprites[gBattlerSpriteIds[gActiveBattler]].hFlip = 1;
    }
    // Use the back pic in any other scenario.
    else {
        DecompressTrainerBackPic(trainerPicId, gActiveBattler);
        SetMultiuseSpriteTemplateToTrainerBack(trainerPicId, GetBattlerPosition(gActiveBattler));
        gBattlerSpriteIds[gActiveBattler] = CreateSprite(&gMultiuseSpriteTemplate, xPos, yPos, GetBattlerSpriteSubpriority(gActiveBattler));

        gSprites[gBattlerSpriteIds[gActiveBattler]].oam.paletteNum = gActiveBattler;
        gSprites[gBattlerSpriteIds[gActiveBattler]].x2 = DISPLAY_WIDTH;
        gSprites[gBattlerSpriteIds[gActiveBattler]].sSpeedX = -2;
        gSprites[gBattlerSpriteIds[gActiveBattler]].callback = SpriteCB_TrainerSpawn;
    }

    gBattlerControllerFuncs[gActiveBattler] = CompleteOnBattlerSpriteCallbackDummy;
}

static void PlayerHandleTrainerSlide(void) {
    u32 trainerPicId;

    if (gBattleTypeFlags & BATTLE_TYPE_LINK) {
        if ((gLinkPlayers[GetMultiplayerId()].version & 0xFF) == VERSION_FIRE_RED || (gLinkPlayers[GetMultiplayerId()].version & 0xFF) == VERSION_LEAF_GREEN) {
            trainerPicId = gLinkPlayers[GetMultiplayerId()].gender + TRAINER_BACK_PIC_RED;
        } else if ((gLinkPlayers[GetMultiplayerId()].version & 0xFF) == VERSION_RUBY || (gLinkPlayers[GetMultiplayerId()].version & 0xFF) == VERSION_SAPPHIRE) {
            trainerPicId = gLinkPlayers[GetMultiplayerId()].gender + TRAINER_BACK_PIC_RUBY_SAPPHIRE_BRENDAN;
        } else {
            trainerPicId = gLinkPlayers[GetMultiplayerId()].gender + TRAINER_BACK_PIC_BRENDAN;
        }
    } else {
        trainerPicId = gSaveBlock2Ptr->playerGender + TRAINER_BACK_PIC_BRENDAN;
    }

    DecompressTrainerBackPic(trainerPicId, gActiveBattler);
    SetMultiuseSpriteTemplateToTrainerBack(trainerPicId, GetBattlerPosition(gActiveBattler));
    gBattlerSpriteIds[gActiveBattler] = CreateSprite(&gMultiuseSpriteTemplate, 80, (8 - gTrainerBackPicCoords[trainerPicId].size) * 4 + 80, 30);

    gSprites[gBattlerSpriteIds[gActiveBattler]].oam.paletteNum = gActiveBattler;
    gSprites[gBattlerSpriteIds[gActiveBattler]].x2 = -96;
    gSprites[gBattlerSpriteIds[gActiveBattler]].sSpeedX = 2;
    gSprites[gBattlerSpriteIds[gActiveBattler]].callback = SpriteCB_TrainerSlideIn;

    gBattlerControllerFuncs[gActiveBattler] = CompleteOnBankSpriteCallbackDummy2;
}

#undef sSpeedX

static void PlayerHandleTrainerSlideBack(void) {
    SetSpritePrimaryCoordsFromSecondaryCoords(&gSprites[gBattlerSpriteIds[gActiveBattler]]);
    gSprites[gBattlerSpriteIds[gActiveBattler]].data[0] = 50;
    gSprites[gBattlerSpriteIds[gActiveBattler]].data[2] = -40;
    gSprites[gBattlerSpriteIds[gActiveBattler]].data[4] = gSprites[gBattlerSpriteIds[gActiveBattler]].y;
    gSprites[gBattlerSpriteIds[gActiveBattler]].callback = StartAnimLinearTranslation;
    StoreSpriteCallbackInData6(&gSprites[gBattlerSpriteIds[gActiveBattler]], SpriteCallbackDummy);
    StartSpriteAnim(&gSprites[gBattlerSpriteIds[gActiveBattler]], 1);
    gBattlerControllerFuncs[gActiveBattler] = FreeTrainerSpriteAfterSlide;
}

#define sSpeedX data[1]
#define sSpeedY data[2]

static void PlayerHandleFaintAnimation(void) {
    if (gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].animationState == 0) {
        if (gBattleSpritesDataPtr->battlerData[gActiveBattler].behindSubstitute)
            InitAndLaunchSpecialAnimation(gActiveBattler, gActiveBattler, gActiveBattler, B_ANIM_SUBSTITUTE_TO_MON);
        gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].animationState++;
    } else {
        if (!gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].specialAnimActive) {
            gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].animationState = 0;
            HandleLowHpMusicChange(&gPlayerParty[gBattlerPartyIndexes[gActiveBattler]], gActiveBattler);
            PlaySE12WithPanning(SE_FAINT, SOUND_PAN_ATTACKER);
            gSprites[gBattlerSpriteIds[gActiveBattler]].sSpeedX = 0;
            gSprites[gBattlerSpriteIds[gActiveBattler]].sSpeedY = 5;
            gSprites[gBattlerSpriteIds[gActiveBattler]].callback = SpriteCB_FaintSlideAnim;
            gBattlerControllerFuncs[gActiveBattler] = FreeMonSpriteAfterFaintAnim;
        }
    }
}

#undef sSpeedX
#undef sSpeedY

static void PlayerHandlePaletteFade(void) {
    BeginNormalPaletteFade(PALETTES_ALL, 2, 0, 16, RGB_BLACK);
    PlayerBufferExecCompleted();
}

static void PlayerHandleSuccessBallThrowAnim(void) {
    gBattleSpritesDataPtr->animationData->ballThrowCaseId = BALL_3_SHAKES_SUCCESS;
    gDoingBattleAnim = TRUE;
    if (IsCriticalCapture())
        InitAndLaunchSpecialAnimation(gActiveBattler, gActiveBattler, gBattlerTarget, B_ANIM_CRITICAL_CAPTURE_THROW);
    else
        InitAndLaunchSpecialAnimation(gActiveBattler, gActiveBattler, gBattlerTarget, B_ANIM_BALL_THROW);

    gBattlerControllerFuncs[gActiveBattler] = CompleteOnSpecialAnimDone;
}

static void PlayerHandleBallThrowAnim(void) {
    u8 ballThrowCaseId = gBattleResources->bufferA[gActiveBattler][1];

    gBattleSpritesDataPtr->animationData->ballThrowCaseId = ballThrowCaseId;
    gDoingBattleAnim = TRUE;
    if (IsCriticalCapture())
        InitAndLaunchSpecialAnimation(gActiveBattler, gActiveBattler, gBattlerTarget, B_ANIM_CRITICAL_CAPTURE_THROW);
    else
        InitAndLaunchSpecialAnimation(gActiveBattler, gActiveBattler, gBattlerTarget, B_ANIM_BALL_THROW);

    gBattlerControllerFuncs[gActiveBattler] = CompleteOnSpecialAnimDone;
}

static void PlayerHandlePause(void) {
    u8 timer = gBattleResources->bufferA[gActiveBattler][1];

    while (timer != 0) timer--;

    PlayerBufferExecCompleted();
}

static void PlayerHandleMoveAnimation(void) {
    if (!IsBattleSEPlaying(gActiveBattler)) {
        u16 move = gBattleResources->bufferA[gActiveBattler][1] | (gBattleResources->bufferA[gActiveBattler][2] << 8);

        gAnimMoveTurn = gBattleResources->bufferA[gActiveBattler][3];
        gAnimMovePower = gBattleResources->bufferA[gActiveBattler][4] | (gBattleResources->bufferA[gActiveBattler][5] << 8);
        gAnimMoveDmg = gBattleResources->bufferA[gActiveBattler][6] | (gBattleResources->bufferA[gActiveBattler][7] << 8) |
                       (gBattleResources->bufferA[gActiveBattler][8] << 16) | (gBattleResources->bufferA[gActiveBattler][9] << 24);
        gAnimFriendship = gBattleResources->bufferA[gActiveBattler][10];
        gWeatherMoveAnim = gBattleResources->bufferA[gActiveBattler][12] | (gBattleResources->bufferA[gActiveBattler][13] << 8);
        gAnimVolatileStructPtr = (struct VolatileStruct *)&gBattleResources->bufferA[gActiveBattler][16];
        gTransformedPersonalities[gActiveBattler] = gAnimVolatileStructPtr->transformedMonPersonality;
        if (IsMoveWithoutAnimation(move, gAnimMoveTurn))  // Always returns FALSE.
        {
            PlayerBufferExecCompleted();
        } else {
            gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].animationState = 0;
            gBattlerControllerFuncs[gActiveBattler] = PlayerDoMoveAnimation;
            BattleTv_SetDataBasedOnMove(move, gWeatherMoveAnim, gAnimVolatileStructPtr);
        }
    }
}

static void PlayerDoMoveAnimation(void) {
    u16 move = gBattleResources->bufferA[gActiveBattler][1] | (gBattleResources->bufferA[gActiveBattler][2] << 8);
    u8 multihit = gBattleResources->bufferA[gActiveBattler][11];

    switch (gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].animationState) {
        case 0:
            if (gBattleSpritesDataPtr->battlerData[gActiveBattler].behindSubstitute && !gBattleSpritesDataPtr->battlerData[gActiveBattler].flag_x8) {
                gBattleSpritesDataPtr->battlerData[gActiveBattler].flag_x8 = 1;
                InitAndLaunchSpecialAnimation(gActiveBattler, gActiveBattler, gActiveBattler, B_ANIM_SUBSTITUTE_TO_MON);
            }
            gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].animationState = 1;
            break;
        case 1:
            if (!gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].specialAnimActive) {
                SetBattlerSpriteAffineMode(ST_OAM_AFFINE_OFF);
                DoMoveAnim(move);
                gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].animationState = 2;
            }
            break;
        case 2:
            gAnimScriptCallback();
            if (!gAnimScriptActive) {
                SetBattlerSpriteAffineMode(ST_OAM_AFFINE_NORMAL);
                if (gBattleSpritesDataPtr->battlerData[gActiveBattler].behindSubstitute && multihit < 2) {
                    InitAndLaunchSpecialAnimation(gActiveBattler, gActiveBattler, gActiveBattler, B_ANIM_MON_TO_SUBSTITUTE);
                    gBattleSpritesDataPtr->battlerData[gActiveBattler].flag_x8 = 0;
                }
                gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].animationState = 3;
            }
            break;
        case 3:
            if (!gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].specialAnimActive) {
                CopyAllBattleSpritesInvisibilities();
                TrySetBehindSubstituteSpriteBit(gActiveBattler,
                                                gBattleResources->bufferA[gActiveBattler][1] | (gBattleResources->bufferA[gActiveBattler][2] << 8));
                gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].animationState = 0;
                PlayerBufferExecCompleted();
            }
            break;
    }
}

static void PlayerHandlePrintString(void) {
    u16 *stringId;

    gBattle_BG0_X = 0;
    gBattle_BG0_Y = 0;
    stringId = (u16 *)(&gBattleResources->bufferA[gActiveBattler][2]);
    BufferStringBattle(*stringId);
    BattlePutTextOnWindow(gDisplayedStringBattle, B_WIN_MSG);
    gBattlerControllerFuncs[gActiveBattler] = CompleteOnInactiveTextPrinter2;
    BattleTv_SetDataBasedOnString(*stringId);
    BattleArena_DeductMindPoints(gActiveBattler, *stringId);
}

static void PlayerHandlePrintSelectionString(void) {
    if (GetBattlerSide(gActiveBattler) == B_SIDE_PLAYER)
        PlayerHandlePrintString();
    else
        PlayerBufferExecCompleted();
}

void HandleChooseActionAfterDma3_Player(void) {
    if (!IsDma3ManagerBusyWithBgCopy()) {
        gBattle_BG0_X = 0;
        gBattle_BG0_Y = DISPLAY_HEIGHT;
        gBattlerControllerFuncs[gActiveBattler] = HandleInputChooseActionPlayer;
    }
}

static void PlayerHandleChooseAction(void) {
    // Reshow Bg
    gBattle_BG1_X = 0;
    gBattle_BG1_Y = 0;
    ShowBg(1);

    gBattlerControllerFuncs[gActiveBattler] = HandleChooseActionAfterDma3_Player;
    BattleTv_ClearExplosionFaintCause();

    PrintBattleWindow_ActionPromt();
}

static void PlayerHandleYesNoBox(void) {
    if (GetBattlerSide(gActiveBattler) == B_SIDE_PLAYER) {
        // HandleBattleWindow(0x18, 8, 0x1D, 0xD, 0);
        HandleBattleWindow(BATTLE_BOX_YES_NO_Y, 8, BATTLE_BOX_YES_NO_Y + BATTLE_BOX_YES_NO_WIDTH, 13, 0);
        BattlePutTextOnWindow(gText_BattleYesNoChoice, B_WIN_YESNO);
        gMultiUsePlayerCursor = 1;
        BattleCreateYesNoCursorAt(1);
        gBattlerControllerFuncs[gActiveBattler] = PlayerHandleYesNoInput;
    } else {
        PlayerBufferExecCompleted();
    }
}

static void HandleChooseMoveAfterDma3(void) {
    if (!IsDma3ManagerBusyWithBgCopy()) {
        gBattle_BG0_X = 0;
        gBattle_BG0_Y = DISPLAY_HEIGHT * 2;
        PrintBattleWindow_MoveSelection();
        gBattlerControllerFuncs[gActiveBattler] = HandleInputChooseMove;
    }
}

// arenaMindPoints is used here as a placeholder for a timer.

static void PlayerChooseMoveInBattlePalace(void) {
    if (--*(gBattleStruct->arenaMindPoints + gActiveBattler) == 0) {
        gBattlePalaceMoveSelectionRngValue = gRngValue;
        BtlController_EmitTwoReturnValues(1, 10, ChooseMoveAndTargetInBattlePalace());
        PlayerBufferExecCompleted();
    }
}

static void PlayerHandleChooseMove(void) {
    if (gBattleTypeFlags & BATTLE_TYPE_PALACE) {
        *(gBattleStruct->arenaMindPoints + gActiveBattler) = 8;
        gBattlerControllerFuncs[gActiveBattler] = PlayerChooseMoveInBattlePalace;
    } else {
        InitMoveSelectionsVarsAndStrings();
        gBattleStruct->mega.playerSelect = FALSE;
        TryToAddMoveInfoWindow();
        if (!IsMegaTriggerSpriteActive()) gBattleStruct->mega.triggerSpriteId = 0xFF;
        if (CanMegaEvolve(gActiveBattler)) CreateMegaTriggerSprite(gActiveBattler, 0);
        gBattlerControllerFuncs[gActiveBattler] = HandleChooseMoveAfterDma3;
    }
}

void InitMoveSelectionsVarsAndStrings(void) {
    MoveSelectionDisplayMoveNames();
    gMultiUsePlayerCursor = 0xFF;
    MoveSelectionCreateCursorAt(gMoveSelectionCursor[gActiveBattler], 0);
    MoveSelectionDisplayPpString();
    MoveSelectionDisplayPpNumber();
    MoveSelectionDisplayMoveType();
}

static void PlayerHandleChooseItem(void) {
    s32 i;

    BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 0x10, RGB_BLACK);
    gBattlerControllerFuncs[gActiveBattler] = OpenBagAndChooseItem;
    gBattlerInMenuId = gActiveBattler;

    for (i = 0; i < ARRAY_COUNT(gBattlePartyCurrentOrder); i++) gBattlePartyCurrentOrder[i] = gBattleResources->bufferA[gActiveBattler][1 + i];
}

static void PlayerHandleChoosePokemon(void) {
    s32 i;

    for (i = 0; i < ARRAY_COUNT(gBattlePartyCurrentOrder); i++) gBattlePartyCurrentOrder[i] = gBattleResources->bufferA[gActiveBattler][4 + i];

    if (gBattleTypeFlags & BATTLE_TYPE_ARENA && (gBattleResources->bufferA[gActiveBattler][1] & 0xF) != PARTY_ACTION_CANT_SWITCH) {
        BtlController_EmitChosenMonReturnValue(1, gBattlerPartyIndexes[gActiveBattler] + 1, gBattlePartyCurrentOrder);
        PlayerBufferExecCompleted();
    } else {
        gBattleControllerData[gActiveBattler] = CreateTask(TaskDummy, 0xFF);
        gTasks[gBattleControllerData[gActiveBattler]].data[0] = gBattleResources->bufferA[gActiveBattler][1] & 0xF;
        *(&gBattleStruct->battlerPreventingSwitchout) = gBattleResources->bufferA[gActiveBattler][1] >> 4;
        *(&gBattleStruct->prevSelectedPartySlot) = gBattleResources->bufferA[gActiveBattler][2];
        *(&gBattleStruct->abilityPreventingSwitchout) =
            (gBattleResources->bufferA[gActiveBattler][3] & 0xFF) | (gBattleResources->bufferA[gActiveBattler][7] << 8);
        BeginNormalPaletteFade(0xFFFFFFFF, 0, 0, 0x10, RGB_BLACK);
        gBattlerControllerFuncs[gActiveBattler] = OpenPartyMenuToChooseMon;
        gBattlerInMenuId = gActiveBattler;
    }
}

static void PlayerHandleCmd23(void) {
    BattleStopLowHpSound();
    BeginNormalPaletteFade(PALETTES_ALL, 2, 0, 16, RGB_BLACK);
    PlayerBufferExecCompleted();
}

static void PlayerHandleHealthBarUpdate(void) {
    s16 hpVal;

    LoadBattleBarGfx(0);
    hpVal = gBattleResources->bufferA[gActiveBattler][2] | (gBattleResources->bufferA[gActiveBattler][3] << 8);

    // gPlayerPartyLostHP used by Battle Dome, but never read
    if (hpVal > 0) gPlayerPartyLostHP += hpVal;

    if (hpVal != INSTANT_HP_BAR_DROP) {
        u32 maxHP = GetMonData(&gPlayerParty[gBattlerPartyIndexes[gActiveBattler]], MON_DATA_MAX_HP);
        u32 curHP = GetMonData(&gPlayerParty[gBattlerPartyIndexes[gActiveBattler]], MON_DATA_HP);

        SetBattleBarStruct(gActiveBattler, gHealthboxSpriteIds[gActiveBattler], maxHP, curHP, hpVal);
    } else {
        u32 maxHP = GetMonData(&gPlayerParty[gBattlerPartyIndexes[gActiveBattler]], MON_DATA_MAX_HP);

        SetBattleBarStruct(gActiveBattler, gHealthboxSpriteIds[gActiveBattler], maxHP, 0, hpVal);
        UpdateHpTextInHealthbox(gHealthboxSpriteIds[gActiveBattler], 0, HP_CURRENT);
    }

    gBattlerControllerFuncs[gActiveBattler] = CompleteOnHealthbarDone;
}

static void PlayerHandleExpUpdate(void) {
    u8 monId = gBattleResources->bufferA[gActiveBattler][1];
    s32 taskId, expPointsToGive;

    if (GetMonData(&gPlayerParty[monId], MON_DATA_LEVEL) >= MAX_LEVEL) {
        PlayerBufferExecCompleted();
    } else {
        LoadBattleBarGfx(1);
        GetMonData(&gPlayerParty[monId], MON_DATA_SPECIES);  // Unused return value.
        expPointsToGive = T1_READ_32(&gBattleResources->bufferA[gActiveBattler][2]);
        taskId = CreateTask(Task_GiveExpToMon, 10);
        gTasks[taskId].tExpTask_monId = monId;
        gTasks[taskId].tExpTask_gainedExp_1 = expPointsToGive;
        gTasks[taskId].tExpTask_gainedExp_2 = expPointsToGive >> 16;
        gTasks[taskId].tExpTask_battler = gActiveBattler;
        gBattlerControllerFuncs[gActiveBattler] = BattleControllerDummy;
    }
}

#undef tExpTask_monId
#undef tExpTask_battler
#undef tExpTask_gainedExp_1
#undef tExpTask_gainedExp_2
#undef tExpTask_frames

static void PlayerHandleStatusIconUpdate(void) {
    if (!IsBattleSEPlaying(gActiveBattler)) {
        u8 battlerId;

        UpdateHealthboxAttribute(gHealthboxSpriteIds[gActiveBattler], &gPlayerParty[gBattlerPartyIndexes[gActiveBattler]], HEALTHBOX_STATUS_ICON);
        battlerId = gActiveBattler;
        gBattleSpritesDataPtr->healthBoxesData[battlerId].statusAnimActive = 0;
        gBattlerControllerFuncs[gActiveBattler] = CompleteOnFinishedStatusAnimation;
    }
}

static void PlayerHandleStatusAnimation(void) {
    if (!IsBattleSEPlaying(gActiveBattler)) {
        InitAndLaunchChosenStatusAnimation(gBattleResources->bufferA[gActiveBattler][1],
                                           gBattleResources->bufferA[gActiveBattler][2] | (gBattleResources->bufferA[gActiveBattler][3] << 8) |
                                               (gBattleResources->bufferA[gActiveBattler][4] << 16) | (gBattleResources->bufferA[gActiveBattler][5] << 24));
        gBattlerControllerFuncs[gActiveBattler] = CompleteOnFinishedStatusAnimation;
    }
}

static void PlayerHandleStatusXor(void) {
    u8 val = GetMonData(&gPlayerParty[gBattlerPartyIndexes[gActiveBattler]], MON_DATA_STATUS) ^ gBattleResources->bufferA[gActiveBattler][1];

    SetMonData(&gPlayerParty[gBattlerPartyIndexes[gActiveBattler]], MON_DATA_STATUS, &val);
    PlayerBufferExecCompleted();
}

static void PlayerHandleDataTransfer(void) { PlayerBufferExecCompleted(); }

static void PlayerHandleDMA3Transfer(void) {
    u32 dstArg = gBattleResources->bufferA[gActiveBattler][1] | (gBattleResources->bufferA[gActiveBattler][2] << 8) |
                 (gBattleResources->bufferA[gActiveBattler][3] << 16) | (gBattleResources->bufferA[gActiveBattler][4] << 24);
    u16 sizeArg = gBattleResources->bufferA[gActiveBattler][5] | (gBattleResources->bufferA[gActiveBattler][6] << 8);

    const u8 *src = &gBattleResources->bufferA[gActiveBattler][7];
    u8 *dst = (u8 *)(dstArg);
    u32 size = sizeArg;

    while (1) {
        if (size <= 0x1000) {
            DmaCopy16(3, src, dst, size);
            break;
        }
        DmaCopy16(3, src, dst, 0x1000);
        src += 0x1000;
        dst += 0x1000;
        size -= 0x1000;
    }
    PlayerBufferExecCompleted();
}

static void PlayerHandlePlayBGM(void) {
    PlayBGM(gBattleResources->bufferA[gActiveBattler][1] | (gBattleResources->bufferA[gActiveBattler][2] << 8));
    PlayerBufferExecCompleted();
}

static void PlayerHandleCmd32(void) { PlayerBufferExecCompleted(); }

static void PlayerHandleTwoReturnValues(void) {
    BtlController_EmitTwoReturnValues(1, 0, 0);
    PlayerBufferExecCompleted();
}

static void PlayerHandleChosenMonReturnValue(void) {
    BtlController_EmitChosenMonReturnValue(1, 0, NULL);
    PlayerBufferExecCompleted();
}

static void PlayerHandleOneReturnValue(void) {
    BtlController_EmitOneReturnValue(1, 0);
    PlayerBufferExecCompleted();
}

static void PlayerHandleOneReturnValue_Duplicate(void) {
    BtlController_EmitOneReturnValue_Duplicate(1, 0);
    PlayerBufferExecCompleted();
}

static void PlayerHandleClearUnkVar(void) {
    gUnusedControllerStruct.unk = 0;
    PlayerBufferExecCompleted();
}

static void PlayerHandleSetUnkVar(void) {
    gUnusedControllerStruct.unk = gBattleResources->bufferA[gActiveBattler][1];
    PlayerBufferExecCompleted();
}

static void PlayerHandleClearUnkFlag(void) {
    gUnusedControllerStruct.flag = 0;
    PlayerBufferExecCompleted();
}

static void PlayerHandleToggleUnkFlag(void) {
    gUnusedControllerStruct.flag ^= 1;
    PlayerBufferExecCompleted();
}

static void PlayerHandleHitAnimation(void) {
    if (gSprites[gBattlerSpriteIds[gActiveBattler]].invisible == TRUE) {
        PlayerBufferExecCompleted();
    } else {
        gDoingBattleAnim = TRUE;
        gSprites[gBattlerSpriteIds[gActiveBattler]].data[1] = 0;
        DoHitAnimHealthboxEffect(gActiveBattler);
        gBattlerControllerFuncs[gActiveBattler] = DoHitAnimBlinkSpriteEffect;
    }
}

static void PlayerHandleCantSwitch(void) { PlayerBufferExecCompleted(); }

static void PlayerHandlePlaySE(void) {
    s8 pan;

    if (GetBattlerSide(gActiveBattler) == B_SIDE_PLAYER)
        pan = SOUND_PAN_ATTACKER;
    else
        pan = SOUND_PAN_TARGET;

    PlaySE12WithPanning(gBattleResources->bufferA[gActiveBattler][1] | (gBattleResources->bufferA[gActiveBattler][2] << 8), pan);
    PlayerBufferExecCompleted();
}

static void PlayerHandlePlayFanfareOrBGM(void) {
    if (gBattleResources->bufferA[gActiveBattler][3]) {
        BattleStopLowHpSound();
        PlayBGM(gBattleResources->bufferA[gActiveBattler][1] | (gBattleResources->bufferA[gActiveBattler][2] << 8));
    } else {
        PlayFanfare(gBattleResources->bufferA[gActiveBattler][1] | (gBattleResources->bufferA[gActiveBattler][2] << 8));
    }

    PlayerBufferExecCompleted();
}

static void PlayerHandleFaintingCry(void) {
    SpeciesEnum species = GetMonData(&gPlayerParty[gBattlerPartyIndexes[gActiveBattler]], MON_DATA_SPECIES);

    PlayCry3(species, -25, 5);
    PlayerBufferExecCompleted();
}

static void PlayerHandleIntroSlide(void) {
    HandleIntroSlide(gBattleResources->bufferA[gActiveBattler][1]);
    gIntroSlideFlags |= 1;
    PlayerBufferExecCompleted();
}

// Task data for Task_StartSendOutAnim
#define tBattlerId data[0]
#define tStartTimer data[1]

#define sBattlerId data[5]

static void PlayerHandleIntroTrainerBallThrow(void) {
    u8 paletteNum;
    u8 taskId;

    SetSpritePrimaryCoordsFromSecondaryCoords(&gSprites[gBattlerSpriteIds[gActiveBattler]]);

    gSprites[gBattlerSpriteIds[gActiveBattler]].data[0] = 50;
    gSprites[gBattlerSpriteIds[gActiveBattler]].data[2] = -40;
    gSprites[gBattlerSpriteIds[gActiveBattler]].data[4] = gSprites[gBattlerSpriteIds[gActiveBattler]].y;
    gSprites[gBattlerSpriteIds[gActiveBattler]].callback = StartAnimLinearTranslation;
    gSprites[gBattlerSpriteIds[gActiveBattler]].sBattlerId = gActiveBattler;

    StoreSpriteCallbackInData6(&gSprites[gBattlerSpriteIds[gActiveBattler]], SpriteCB_FreePlayerSpriteLoadMonSprite);
    StartSpriteAnim(&gSprites[gBattlerSpriteIds[gActiveBattler]], 1);

    paletteNum = AllocSpritePalette(0xD6F8);
    LoadCompressedPalette(gTrainerBackPicPaletteTable[gSaveBlock2Ptr->playerGender].data, 0x100 + paletteNum * 16, 32);
    gSprites[gBattlerSpriteIds[gActiveBattler]].oam.paletteNum = paletteNum;

    taskId = CreateTask(Task_StartSendOutAnim, 5);
    gTasks[taskId].tBattlerId = gActiveBattler;

    if (gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].partyStatusSummaryShown)
        gTasks[gBattlerStatusSummaryTaskId[gActiveBattler]].func = Task_HidePartyStatusSummary;

    gBattleSpritesDataPtr->animationData->introAnimActive = TRUE;
    gBattlerControllerFuncs[gActiveBattler] = BattleControllerDummy;
}

void SpriteCB_FreePlayerSpriteLoadMonSprite(struct Sprite *sprite) {
    u8 battlerId = sprite->sBattlerId;

    // Free player trainer sprite
    FreeSpriteOamMatrix(sprite);
    FreeSpritePaletteByTag(GetSpritePaletteTagByPaletteNum(sprite->oam.paletteNum));
    DestroySprite(sprite);

    // Load mon sprite
    BattleLoadPlayerMonSpriteGfx(&gPlayerParty[gBattlerPartyIndexes[battlerId]], battlerId);
    StartSpriteAnim(&gSprites[gBattlerSpriteIds[battlerId]], 0);
}

#undef sBattlerId

// Send out at start of battle
static void Task_StartSendOutAnim(u8 taskId) {
    if (gTasks[taskId].tStartTimer < 31) {
        gTasks[taskId].tStartTimer++;
    } else {
        u8 savedActiveBattler = gActiveBattler;

        gActiveBattler = gTasks[taskId].tBattlerId;
        if (TwoIntroMons(gActiveBattler) && !(gBattleTypeFlags & BATTLE_TYPE_MULTI)) {
            gBattleResources->bufferA[gActiveBattler][1] = gBattlerPartyIndexes[gActiveBattler];
            StartSendOutAnim(gActiveBattler, FALSE);
            gActiveBattler ^= BIT_FLANK;
            gBattleResources->bufferA[gActiveBattler][1] = gBattlerPartyIndexes[gActiveBattler];
            BattleLoadPlayerMonSpriteGfx(&gPlayerParty[gBattlerPartyIndexes[gActiveBattler]], gActiveBattler);
            StartSendOutAnim(gActiveBattler, FALSE);
            gActiveBattler ^= BIT_FLANK;
        } else {
            gBattleResources->bufferA[gActiveBattler][1] = gBattlerPartyIndexes[gActiveBattler];
            StartSendOutAnim(gActiveBattler, FALSE);
        }
        gBattlerControllerFuncs[gActiveBattler] = Intro_TryShinyAnimShowHealthbox;
        gActiveBattler = savedActiveBattler;
        DestroyTask(taskId);
    }
}

#undef tBattlerId
#undef tStartTimer

static void PlayerHandleDrawPartyStatusSummary(void) {
    if (gBattleResources->bufferA[gActiveBattler][1] != 0 && GetBattlerSide(gActiveBattler) == B_SIDE_PLAYER) {
        PlayerBufferExecCompleted();
    } else {
        gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].partyStatusSummaryShown = 1;
        gBattlerStatusSummaryTaskId[gActiveBattler] = CreatePartyStatusSummarySprites(gActiveBattler,
                                                                                      (struct HpAndStatus *)&gBattleResources->bufferA[gActiveBattler][4],
                                                                                      gBattleResources->bufferA[gActiveBattler][1],
                                                                                      gBattleResources->bufferA[gActiveBattler][2]);
        gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].partyStatusDelayTimer = 0;

        // If intro, skip the delay after drawing
        if (gBattleResources->bufferA[gActiveBattler][2] != 0) gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].partyStatusDelayTimer = 93;

        gBattlerControllerFuncs[gActiveBattler] = EndDrawPartyStatusSummary;
    }
}

static void EndDrawPartyStatusSummary(void) {
    if (gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].partyStatusDelayTimer++ > 92) {
        gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].partyStatusDelayTimer = 0;
        PlayerBufferExecCompleted();
    }
}

static void PlayerHandleHidePartyStatusSummary(void) {
    if (gBattleSpritesDataPtr->healthBoxesData[gActiveBattler].partyStatusSummaryShown)
        gTasks[gBattlerStatusSummaryTaskId[gActiveBattler]].func = Task_HidePartyStatusSummary;
    PlayerBufferExecCompleted();
}

static void PlayerHandleEndBounceEffect(void) {
    EndBounceEffect(gActiveBattler, BOUNCE_HEALTHBOX);
    EndBounceEffect(gActiveBattler, BOUNCE_MON);
    PlayerBufferExecCompleted();
}

static void PlayerHandleSpriteInvisibility(void) {
    if (IsBattlerSpritePresent(gActiveBattler)) {
        gSprites[gBattlerSpriteIds[gActiveBattler]].invisible = gBattleResources->bufferA[gActiveBattler][1];
        CopyBattleSpriteInvisibility(gActiveBattler);
    }
    PlayerBufferExecCompleted();
}

static void PlayerHandleBattleAnimation(void) {
    if (!IsBattleSEPlaying(gActiveBattler)) {
        u8 animationId = gBattleResources->bufferA[gActiveBattler][1];
        u16 argument = gBattleResources->bufferA[gActiveBattler][2] | (gBattleResources->bufferA[gActiveBattler][3] << 8);

        if (TryHandleLaunchBattleTableAnimation(gActiveBattler, gActiveBattler, gActiveBattler, animationId, argument))
            PlayerBufferExecCompleted();
        else
            gBattlerControllerFuncs[gActiveBattler] = CompleteOnFinishedBattleAnimation;

        BattleTv_SetDataBasedOnAnimation(animationId);
    }
}

static void PlayerHandleLinkStandbyMsg(void) {
    RecordedBattle_RecordAllBattlerData(&gBattleResources->bufferA[gActiveBattler][2]);
    switch (gBattleResources->bufferA[gActiveBattler][1]) {
        case 0:
            PrintLinkStandbyMsg();
            FALLTHROUGH
        case 1:
            EndBounceEffect(gActiveBattler, BOUNCE_HEALTHBOX);
            EndBounceEffect(gActiveBattler, BOUNCE_MON);
            break;
        case 2:
            PrintLinkStandbyMsg();
            break;
    }
    PlayerBufferExecCompleted();
}

static void PlayerHandleResetActionMoveSelection(void) {
    switch (gBattleResources->bufferA[gActiveBattler][1]) {
        case RESET_ACTION_MOVE_SELECTION:
            gActionSelectionCursor[gActiveBattler] = 0;
            gMoveSelectionCursor[gActiveBattler] = 0;
            break;
        case RESET_ACTION_SELECTION:
            gActionSelectionCursor[gActiveBattler] = 0;
            break;
        case RESET_MOVE_SELECTION:
            gMoveSelectionCursor[gActiveBattler] = 0;
            break;
    }
    PlayerBufferExecCompleted();
}

static void PlayerHandleEndLinkBattle(void) {
    RecordedBattle_RecordAllBattlerData(&gBattleResources->bufferA[gActiveBattler][4]);
    gBattleOutcome = gBattleResources->bufferA[gActiveBattler][1];
    gSaveBlock2Ptr->frontier.disableRecordBattle = gBattleResources->bufferA[gActiveBattler][2];
    FadeOutMapMusic(5);
    BeginFastPaletteFade(3);
    PlayerBufferExecCompleted();
    gBattlerControllerFuncs[gActiveBattler] = SetBattleEndCallbacks;
}

static void WaitForDebug(void) {
    if (gMain.callback2 == BattleMainCB2 && !gPaletteFade.active) {
        PlayerBufferExecCompleted();
    }
}

static void PlayerHandleBattleDebug(void) {
    BeginNormalPaletteFade(-1, 0, 0, 0x10, 0);
    SetMainCallback2(CB2_BattleDebugMenu);
    gBattlerControllerFuncs[gActiveBattler] = WaitForDebug;
}

static void PlayerHandleBattleInfoMenu(void) {
    BeginNormalPaletteFade(-1, 0, 0, 0x10, 0);
    // SetMainCallback2(ReshowBattleScreenAfterMenu);
    UI_Battle_Menu_Init(ReshowBattleScreenAfterMenu);
    gBattlerControllerFuncs[gActiveBattler] = WaitForDebug;
}

static void PlayerHandleBattleInGameWikiMenu(void) {
    BeginNormalPaletteFade(-1, 0, 0, 0x10, 0);
    // SetMainCallback2(ReshowBattleScreenAfterMenu);
    InformationMenu_Init_From_Battle(ReshowBattleScreenAfterMenu);
    gBattlerControllerFuncs[gActiveBattler] = WaitForDebug;
}

static void PlayerCmdEnd(void) {}

static void ChangeMoveDisplayMode() {}

static void MoveSelectionDisplaySplitIcon(void) {}
