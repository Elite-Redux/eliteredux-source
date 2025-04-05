#ifndef GUARD_UI_INFORMATION_MENU_H
#define GUARD_UI_INFORMATION_MENU_H

#include "main.h"

typedef struct {
    const u8* title;
    const u8* const* pages;
    u8 numPages;
} HelpArticle;

typedef struct {
    const u8* title;
    const HelpArticle* entries;
    u8 color;
    u8 numEntries;
} HelpCategory;

void Task_OpenInformationMenuFromStartMenu(u8 taskId);
void InformationMenu_Init(MainCallback callback);
void InformationMenu_Init_From_Battle(MainCallback callback);

#endif
