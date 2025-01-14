#ifndef GUARD_UI_INFORMATION_MENU_H
#define GUARD_UI_INFORMATION_MENU_H

#include "main.h"

void Task_OpenInformationMenuFromStartMenu(u8 taskId);
void InformationMenu_Init(MainCallback callback);
void InformationMenu_Init_From_Battle(MainCallback callback);

#endif
