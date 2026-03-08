#ifndef GUARD_DEBUG_H
#define GUARD_DEBUG_H

void CB2_StartSoundCheckMenu(void);

// Debug options
#define TX_DEBUG_SYSTEM_ENABLE         TRUE               // Enables a overworld debug menu for changing flags, variables, giving pokemon and more, accessed by holding R and pressing START while in the overworld by default.
#define TX_DEBUG_SYSTEM_PUBLIC         TRUE               // Enables the limited public-safe debug rules instead of the full developer debug menu.
#define TX_DEBUG_SYSTEM_HELD_KEYS      (B_BUTTON)         // The keys required to be held to open the debug menu.
#define TX_DEBUG_SYSTEM_TRIGGER_EVENT  pressedStartButton // The event that opens the menu when holding the key(s) defined in DEBUG_SYSTEM_HELD_KEYS.
#define TX_DEBUG_SYSTEM_IN_MENU        FALSE              // Replaces the overworld debug menu button combination with a start menu entry (above Pokédex).

#ifdef DEBUG_BUILD
#define TX_DEBUG_BUILD_ENABLED TRUE
#else
#define TX_DEBUG_BUILD_ENABLED FALSE
#endif

void Debug_ShowMainMenu(void);

#if TX_DEBUG_BUILD_ENABLED == TRUE && TX_DEBUG_SYSTEM_ENABLE == TRUE
bool8 Debug_IsSystemEnabled(void);
bool8 Debug_IsPublicModeEnabled(void);
bool8 Debug_IsMenuAccessibleForCurrentSave(void);
#else
#define Debug_IsSystemEnabled() FALSE
#define Debug_IsPublicModeEnabled() FALSE
#define Debug_IsMenuAccessibleForCurrentSave() FALSE
#endif

extern EWRAM_DATA bool8 gIsDebugBattle;
extern EWRAM_DATA u32 gDebugAIFlags;

#endif // GUARD_DEBUG_H
