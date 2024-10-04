@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ Move Reminder @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

PKMN_Center_MoveReminder_EventScript::
	msgbox PKMN_Center_MoveReminder_Text_WantMeToTeachMove, MSGBOX_YESNO
	switch VAR_RESULT
	case NO, PKMN_Center_Move_Tutor_General_Exit
	goto PKMN_Center_MoveReminder_EventScriptChooseMon
	end

PKMN_Center_MoveReminder_EventScriptChooseMon::
	clearflag FLAG_MOVE_RELEARNER
	setvar VAR_PARTY_MENU_TUTOR_STATE, 0
	msgbox PKMN_Center_MoveReminder_Text_TutorWhichMon, MSGBOX_DEFAULT
	special ChooseMonForMoveRelearner
	waitstate
	compare VAR_0x8004, 255
	goto_if_eq PKMN_Center_Move_Tutor_General_Exit
	special IsSelectedMonEgg
	compare VAR_RESULT, TRUE
	goto_if_eq PKMN_Center_MoveReminder_EventScriptCantTeachEgg
	compare VAR_0x8005, 0
	goto_if_eq PKMN_Center_MoveReminder_EventScriptNoMoveToTeachMon
	goto PKMN_Center_MoveReminder_EventScriptChooseMove
	end

PKMN_Center_MoveReminder_EventScriptChooseMove::
	setflag FLAG_MOVE_RELEARNER
	msgbox PKMN_Center_MoveReminder_Text_TeachWhichMove, MSGBOX_DEFAULT
	special TeachMoveRelearnerMove
	waitstate
	compare VAR_0x8004, 0
	goto_if_eq PKMN_Center_MoveReminder_EventScriptChooseMon
	goto PKMN_Center_MoveReminder_EventScriptRememberAnotherMove
	end

PKMN_Center_MoveReminder_EventScriptNoMoveToTeachMon::
	msgbox PKMN_Center_MoveReminder_Text_DontHaveMoveToTeachPokemon, MSGBOX_DEFAULT
	goto PKMN_Center_MoveReminder_EventScriptChooseMon
	end

PKMN_Center_MoveReminder_EventScriptCantTeachEgg::
	msgbox PKMN_Center_MoveReminder_Text_CantTeachEgg, MSGBOX_DEFAULT
	goto PKMN_Center_MoveReminder_EventScriptChooseMon
	end
	
PKMN_Center_MoveReminder_EventScriptRememberAnotherMove::
	msgbox PKMN_Center_MoveReminder_Text_RememberAnotherMove, MSGBOX_YESNO
	compare VAR_RESULT, NO
	goto_if_eq PKMN_Center_Move_Tutor_General_Exit
	goto PKMN_Center_MoveReminder_EventScriptChooseMon

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ Move Deleter @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

PKMN_Center_MoveDeleter_EventScript::
	msgbox PKMN_Center_MoveDeleter_Text_ICanMakeMonForgetMove, MSGBOX_YESNO
	switch VAR_RESULT
	case YES, PKMN_Center_MoveDeleter_EventScript_ChooseMonAndMoveToForget
	case NO, PKMN_Center_Move_Tutor_General_Exit
	releaseall
	end

PKMN_Center_MoveDeleter_EventScript_ChooseMonAndMoveToForget::
	msgbox PKMN_Center_MoveDeleter_Text_WhichMonShouldForget, MSGBOX_DEFAULT
	special ChoosePartyMon
	waitstate
	compare VAR_0x8004, 255
	goto_if_eq PKMN_Center_Move_Tutor_General_Exit
	special IsSelectedMonEgg
	compare VAR_RESULT, TRUE
	goto_if_eq PKMN_Center_MoveDeleter_EventScript_EggCantForgetMoves
	special GetNumMovesSelectedMonHas
	compare VAR_RESULT, 1
	goto_if_eq PKMN_Center_MoveDeleter_EventScript_MonOnlyKnowsOneMove
	msgbox PKMN_Center_MoveDeleter_Text_WhichMoveShouldBeForgotten, MSGBOX_DEFAULT
	fadescreen FADE_TO_BLACK
	special MoveDeleterChooseMoveToForget
	fadescreen FADE_FROM_BLACK
	compare VAR_0x8005, MAX_MON_MOVES
	goto_if_eq PKMN_Center_MoveDeleter_EventScript_ChooseMonAndMoveToForget
	special BufferMoveDeleterNicknameAndMove
	msgbox PKMN_Center_MoveDeleter_Text_MonsMoveShouldBeForgotten, MSGBOX_YESNO
	switch VAR_RESULT
	case YES, PKMN_Center_MoveDeleter_EventScript_TryForgetMove
	case NO, PKMN_Center_Move_Tutor_General_Exit
	releaseall
	end

PKMN_Center_MoveDeleter_EventScript_TryForgetMove::
	special IsLastMonThatKnowsSurf
	compare VAR_RESULT, TRUE
	goto_if_eq PKMN_Center_MoveDeleter_EventScript_LastMonWithSurf
	special MoveDeleterForgetMove
	playfanfare MUS_MOVE_DELETED
	waitfanfare
	msgbox PKMN_Center_MoveDeleter_Text_MonHasForgottenMove, MSGBOX_DEFAULT
	releaseall
	end

PKMN_Center_MoveDeleter_EventScript_MonOnlyKnowsOneMove::
	special BufferMoveDeleterNicknameAndMove
	msgbox PKMN_Center_MoveDeleter_Text_MonOnlyKnowsOneMove, MSGBOX_DEFAULT
	releaseall
	end

PKMN_Center_MoveDeleter_EventScript_EggCantForgetMoves::
	msgbox PKMN_Center_MoveDeleter_Text_EggCantForgetMoves, MSGBOX_DEFAULT
	releaseall
	end

PKMN_Center_MoveDeleter_EventScript_LastMonWithSurf::
	special BufferMoveDeleterNicknameAndMove
	msgbox PKMN_Center_MoveDeleter_Text_CantForgetSurf, MSGBOX_DEFAULT
	releaseall
	end

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ Move Tutor @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

PKMN_Center_Move_Tutor_MoveTutorIntro::
	goto_if_set FLAG_TALKED_TO_MOVE_TUTOR, PKMN_Center_Move_Tutor_MoveTutorMenu
	msgbox PKMN_Center_Move_Tutor_Text_Explanation, MSGBOX_DEFAULT
	setflag FLAG_TALKED_TO_MOVE_TUTOR
	goto PKMN_Center_Move_Tutor_MoveTutorMenu

PKMN_Center_Move_Tutor_NoBadges::
	msgbox PKMN_Center_Move_Tutor_Text_NoBadgeNoMoves, MSGBOX_DEFAULT
	releaseall
	end

PKMN_Center_Move_Tutor_MoveTutorMenu::
	setvar VAR_0x8008, 0
	goto_if_unset FLAG_BADGE01_GET, PKMN_Center_Move_Tutor_NoBadges
	msgbox PKMN_Center_Move_Tutor_Text_WhichSetofMoves, MSGBOX_DEFAULT
	special ScriptMenu_CreatePKMNCenterMoveTutorMultichoice
	waitstate
	special GetPKMNCenterMoveListMultichoice
	switch VAR_RESULT
	case 0, PKMN_Center_Move_Tutor_MoveList1
	case 1, PKMN_Center_Move_Tutor_MoveList2
	case 2, PKMN_Center_Move_Tutor_MoveList3
	case 3, PKMN_Center_Move_Tutor_MoveList4
	case 4, PKMN_Center_Move_Tutor_MoveList5
	case 5, PKMN_Center_Move_Tutor_MoveList6
	case 6, PKMN_Center_Move_Tutor_MoveList7
	case 7, PKMN_Center_Move_Tutor_ExitReturn
	case MULTI_B_PRESSED, PKMN_Center_Move_Tutor_ExitReturn
	releaseall
	end
	
PKMN_Center_Move_Tutor_MoveList1::
	setvar VAR_TEMP_C, SCROLL_MULTI_PC_TUTOR_SET_1
	setvar VAR_TEMP_E, 0
	setvar VAR_TEMP_F, 20
	setvar VAR_0x8004, SCROLL_MULTI_PC_TUTOR_SET_1
	setvar VAR_0x8006, 0
	goto PKMN_Center_Move_Tutor_ChooseMove
	
PKMN_Center_Move_Tutor_MoveList2::
	setvar VAR_TEMP_C, SCROLL_MULTI_PC_TUTOR_SET_2
	setvar VAR_TEMP_E, 1
	setvar VAR_TEMP_F, 18
	setvar VAR_0x8004, SCROLL_MULTI_PC_TUTOR_SET_2
	setvar VAR_0x8006, 0
	goto PKMN_Center_Move_Tutor_ChooseMove

PKMN_Center_Move_Tutor_MoveList3::
	setvar VAR_TEMP_C, SCROLL_MULTI_PC_TUTOR_SET_3
	setvar VAR_TEMP_E, 2
	setvar VAR_TEMP_F, 18
	setvar VAR_0x8004, SCROLL_MULTI_PC_TUTOR_SET_3
	setvar VAR_0x8006, 0
	goto PKMN_Center_Move_Tutor_ChooseMove

PKMN_Center_Move_Tutor_MoveList4::
	setvar VAR_TEMP_C, SCROLL_MULTI_PC_TUTOR_SET_4
	setvar VAR_TEMP_E, 3
	setvar VAR_TEMP_F, 18
	setvar VAR_0x8004, SCROLL_MULTI_PC_TUTOR_SET_4
	setvar VAR_0x8006, 0
	goto PKMN_Center_Move_Tutor_ChooseMove

PKMN_Center_Move_Tutor_MoveList5::
	setvar VAR_TEMP_C, SCROLL_MULTI_PC_TUTOR_SET_5
	setvar VAR_TEMP_E, 4
	setvar VAR_TEMP_F, 18
	setvar VAR_0x8004, SCROLL_MULTI_PC_TUTOR_SET_5
	setvar VAR_0x8006, 0
	goto PKMN_Center_Move_Tutor_ChooseMove

PKMN_Center_Move_Tutor_MoveList6::
	setvar VAR_TEMP_C, SCROLL_MULTI_PC_TUTOR_SET_6
	setvar VAR_TEMP_E, 5
	setvar VAR_TEMP_F, 18
	setvar VAR_0x8004, SCROLL_MULTI_PC_TUTOR_SET_6
	setvar VAR_0x8006, 0
	goto PKMN_Center_Move_Tutor_ChooseMove

PKMN_Center_Move_Tutor_MoveList7::
	setvar VAR_TEMP_C, SCROLL_MULTI_PC_TUTOR_SET_7
	setvar VAR_TEMP_E, 6
	setvar VAR_TEMP_F, 15
	setvar VAR_0x8004, SCROLL_MULTI_PC_TUTOR_SET_7
	setvar VAR_0x8006, 0
	goto PKMN_Center_Move_Tutor_ChooseMove

PKMN_Center_Move_Tutor_ChooseMove::
    msgbox PKMN_Center_Move_Tutor_Text_WhichMove, MSGBOX_DEFAULT
	special ShowScrollableMultichoice
	waitstate
	copyvar VAR_TEMP_D, VAR_RESULT
	compare VAR_RESULT, VAR_TEMP_F @ VAR_TEMP_F is set to the ID of the "Exit" option
	goto_if_eq PKMN_Center_Move_Tutor_Exit
	compare VAR_RESULT MULTI_B_PRESSED
	goto_if_eq PKMN_Center_Move_Tutor_CloseDescriptionWindow
	goto PKMN_Center_Move_Tutor_ConfirmMoveSelection
	end
	
PKMN_Center_Move_Tutor_ConfirmMoveSelection::
	copyvar VAR_0x8004, VAR_TEMP_D
	copyvar VAR_0x8005, VAR_TEMP_E
	special BufferBattleFrontierTutorMoveName
	buffernumberstring 1, VAR_0x8008
	copyvar VAR_0x8004, VAR_TEMP_C
	msgbox PKMN_Center_Move_Tutor_Text_IsThisRightMove, MSGBOX_YESNO
	compare VAR_RESULT, NO
	goto_if_eq PKMN_Center_Move_Tutor_CloseDescriptionWindow
	goto PKMN_Center_Move_Tutor_TeachTutorMove
	end

PKMN_Center_Move_Tutor_TeachTutorMove::
	msgbox PKMN_Center_Move_Tutor_Text_TeachMoveToWhichMon, MSGBOX_DEFAULT
	special GetBattleFrontierTutorMoveIndex
	fadescreen FADE_TO_BLACK
	special CloseBattleFrontierTutorWindow
	special ChooseMonForMoveTutor
	waitstate
	compare VAR_RESULT, FALSE
	goto_if_eq PKMN_Center_Move_Tutor_CloseDescriptionWindow
	msgbox PKMN_Center_Move_Tutor_Text_TeachAnotherMove, MSGBOX_YESNO
    compare VAR_RESULT, NO
	goto_if_eq PKMN_Center_Move_Tutor_Exit
    goto PKMN_Center_Move_Tutor_CloseDescriptionWindow
	end

PKMN_Center_Move_Tutor_CloseDescriptionWindow::
    special CloseBattleFrontierTutorWindow
    goto PKMN_Center_Move_Tutor_MoveTutorMenu

PKMN_Center_Move_Tutor_CloseDescriptionWindowReturn::
    special CloseBattleFrontierTutorWindow
    goto PKMN_Center_Move_Tutor

PKMN_Center_Move_Tutor_ExitReturn::
    goto PKMN_Center_Move_Tutor

PKMN_Center_Move_Tutor_Exit::
	special CloseBattleFrontierTutorWindow
    msgbox PKMN_Center_Move_Tutor_Text_SeeYou, MSGBOX_DEFAULT
	releaseall
	end

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ Name Rater @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

PKMN_Center_NameRater_EventScript::
	msgbox PKMN_Center_NameRater_Text_CritiqueWhichMonNickname, MSGBOX_DEFAULT
	special ChoosePartyMon
	waitstate
	compare VAR_0x8004, 255
	goto_if_ne PKMN_Center_NameRater_EventScript_RateMonNickname
	compare VAR_0x8004, 255
	goto_if_eq PKMN_Center_Move_Tutor_General_Exit
	end

PKMN_Center_NameRater_EventScript_RateMonNickname::
	specialvar VAR_RESULT, ScriptGetPartyMonSpecies
	compare VAR_RESULT, SPECIES_EGG
	goto_if_eq PKMN_Center_NameRater_EventScript_CantRateEgg
	special BufferMonNickname
	msgbox PKMN_Center_NameRater_Text_WhatShallNewNameBe, MSGBOX_DEFAULT
	call Common_EventScript_NameReceivedPartyMon
	specialvar VAR_RESULT, TryPutNameRaterShowOnTheAir
	special BufferMonNickname
	compare VAR_RESULT, TRUE
	goto_if_eq PKMN_Center_NameRater_EventScript_NewNameDifferent
	msgbox PKMN_Center_NameRater_Text_NameNoDifferentYetSuperior, MSGBOX_DEFAULT
	releaseall
	end

PKMN_Center_NameRater_EventScript_CantRateEgg::
	msgbox PKMN_Center_NameRater_Text_ThatIsMerelyAnEgg, MSGBOX_DEFAULT
	releaseall
	end

PKMN_Center_NameRater_EventScript_NewNameDifferent::
	msgbox PKMN_Center_NameRater_Text_MonShallBeKnownAsName, MSGBOX_DEFAULT
	releaseall
	end

PKMN_Center_Move_Tutor_General_Exit::
    msgbox PKMN_Center_Move_Tutor_Text_SeeYou, MSGBOX_DEFAULT
	releaseall
	end

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ EXP Grinding @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
PKMN_Center_EventScript_Exp_Grind::

	releaseall
	end

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ Event Reset @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
PKMN_Center_EventScript_Event_Reset::
	compare VAR_EVENT_RESET_STATE, 2
	goto_if_eq PKMN_Center_EventReset_EventScript_AlreadyReset
	msgbox PKMN_Center_EventResetText_WhatsBrokenThisTime, MSGBOX_YESNO
	switch VAR_RESULT
	case NO, PKMN_Center_Move_Tutor_General_Exit
	case YES, PKMN_Center_EventReset_EventScript_ResetEvents
	case MULTI_B_PRESSED, PKMN_Center_Move_Tutor_General_Exit
	releaseall
	end

PKMN_Center_EventReset_EventScript_AlreadyReset::
	msgbox PKMN_Center_EventReset_Text_AlreadyReset, MSGBOX_DEFAULT
	releaseall
	end

PKMN_Center_EventReset_EventScript_ResetEvents::
	compare VAR_EVENT_RESET_STATE, 0
	call_if_eq PKMN_Center_EventReset_EventScript_ResetEvents1
	compare VAR_EVENT_RESET_STATE, 1
	call_if_eq PKMN_Center_EventReset_EventScript_ResetEvents2
	msgbox PKMN_Center_EventReset_Text_Reset, MSGBOX_DEFAULT
	releaseall
	end

PKMN_Center_EventReset_EventScript_ResetEvents1::
	setvar VAR_LAVARIDGE_LUCY_STATE, 1
	clearflag FLAG_HIDE_LAVARIDGE_POKECENTER_LUCY
	clearflag FLAG_ITEM_MOSSDEEP_CITY_MILOTICITE
	clearflag FLAG_RECEIVED_TOGEPI_EGG
	setvar VAR_EVENT_RESET_STATE, 1
	return

PKMN_Center_EventReset_EventScript_ResetEvents2::
	clearflag FLAG_RECEIVED_AUDINO
	setvar VAR_EVENT_RESET_STATE, 2
	return

PKMN_Center_EventScript_Mystery_Gift::
	specialvar VAR_TEMP_1, GetMysteryGiftSpecies
	compare VAR_TEMP_1, SPECIES_NONE
	goto_if_eq PKMN_Center_EventScript_Mystery_Gift_NoGiftAvailable
	bufferspeciesname 0, VAR_TEMP_1
	msgbox PKMN_Center_Text_Mystery_Gift_WouldYouLikeGift, MSGBOX_YESNO
	compare VAR_RESULT, NO
	goto_if_eq PKMN_Center_Move_Tutor_General_Exit
	special GetStaticEncounterLevel
	givemon VAR_TEMP_1, VAR_0x800A, VAR_0x8009
	compare VAR_RESULT, 0
	goto_if_eq PKMN_Center_Mystery_Gift_EventScript_ReceiveGiftParty
	compare VAR_RESULT, 1
	goto_if_eq PKMN_Center_Mystery_Gift_EventScript_ReceiveGiftPC
	goto Common_EventScript_NoMoreRoomForPokemon
PKMN_Center_Mystery_Gift_EventScript_TakeCare::
	switch VAR_TEMP_1
	case SPECIES_GRENINJA_BATTLE_BOND, PKMN_Center_EventScript_SetflagGreninja
	case SPECIES_ZYGARDE_10, PKMN_Center_EventScript_SetflagZygarde10
	case SPECIES_ZYGARDE_10_POWER_CONSTRUCT, PKMN_Center_EventScript_SetflagZygarde10PowerConstruct
	case SPECIES_MELOETTA, PKMN_Center_EventScript_SetflagMeloetta
	case SPECIES_SILVALLY, PKMN_Center_Eventscript_SetflagSilvally
	case SPECIES_TERAPAGOS, PKMN_Center_Eventscript_SetflagTerapagos
	case SPECIES_CHI_YU, PKMN_Center_Eventscript_SetflagChiYu
	case SPECIES_WO_CHIEN, PKMN_Center_Eventscript_SetflagWoChien
	case SPECIES_CHIEN_PAO, PKMN_Center_Eventscript_SetflagChienPao
	case SPECIES_TING_LU, PKMN_Center_Eventscript_SetflagTingLu
	case SPECIES_ZACIAN, PKMN_Center_Eventscript_SetflagZacian
	case SPECIES_ZAMAZENTA, PKMN_Center_Eventscript_SetflagZamazenta
	case SPECIES_ETERNATUS, PKMN_Center_Eventscript_SetflagEternatus
	case SPECIES_KELDEO, PKMN_Center_Eventscript_SetflagKeldeo
	case SPECIES_PIKACHU_PARTNER, PKMN_Center_Eventscript_SetflagPikachuPartner
	case SPECIES_EEVEE_PARTNER, PKMN_Center_Eventscript_SetflagEeveePartner
	case SPECIES_MEOWTH_PARTNER, PKMN_Center_Eventscript_SetflagMeowthPartner
	releaseall
	end

PKMN_Center_EventScript_SetflagGreninja::
	setflag FLAG_RECEIVED_ASH_GRENINJA
	releaseall
	end

PKMN_Center_EventScript_SetflagZygarde10::
	setflag FLAG_RECEIVED_ZYGARDE_10
	releaseall
	end

PKMN_Center_EventScript_SetflagChiYu::
	setflag FLAG_RECEIVED_CHI_YU
	releaseall
	end

PKMN_Center_Eventscript_SetflagChienPao::
	setflag FLAG_RECEIVED_CHIEN_PAO
	releaseall
	end

PKMN_Center_EventScript_SetflagWoChien::
	setflag FLAG_RECEIVED_WO_CHIEN
	releaseall
	end

PKMN_Center_EventScript_SetflagTingLu::
	setflag FLAG_RECEIVED_TING_LU
	releaseall
	end

PKMN_Center_EventScript_SetflagZacian::
	setflag FLAG_RECEIVED_ZACIAN
	releaseall
	end

PKMN_Center_EventScript_SetflagZamazenta::
	setflag FLAG_RECEIVED_ZAMAZENTA
	releaseall
	end

PKMN_Center_EventScript_SetflagEternatus::
	setflag FLAG_RECEIVED_ETERNATUS
	releaseall
	end

PKMN_Center_EventScript_SetflagKeldeo::
	setflag FLAG_RECEIVED_KELDEO
	releaseall
	end

PKMN_Center_EventScript_SetflagZygarde10PowerConstruct::
	setflag FLAG_RECEIVED_ZYGARDE_10_POWER_CONSTRUCT
	releaseall
	end

PKMN_Center_EventScript_SetflagEnamorus::
	setflag FLAG_RECEIVED_ENAMORUS
	releaseall
	end

PKMN_Center_EventScript_SetflagMeloetta::
	setflag FLAG_RECEIVED_MELOETTA
	releaseall
	end

PKMN_Center_Eventscript_SetflagSilvally::
	giveitem ITEM_FIRE_MEMORY
	giveitem ITEM_WATER_MEMORY
	giveitem ITEM_GRASS_MEMORY
	giveitem ITEM_ELECTRIC_MEMORY
	giveitem ITEM_PSYCHIC_MEMORY
	giveitem ITEM_ROCK_MEMORY
	giveitem ITEM_GHOST_MEMORY
	giveitem ITEM_DARK_MEMORY
	giveitem ITEM_FAIRY_MEMORY
	giveitem ITEM_STEEL_MEMORY
	giveitem ITEM_ICE_MEMORY
	giveitem ITEM_DRAGON_MEMORY
	giveitem ITEM_FIGHTING_MEMORY
	giveitem ITEM_FLYING_MEMORY
	giveitem ITEM_POISON_MEMORY
	giveitem ITEM_BUG_MEMORY
	giveitem ITEM_GROUND_MEMORY
	setflag FLAG_RECEIVED_SILVALLY
	releaseall
	end

PKMN_Center_Eventscript_SetflagPikachuPartner::
	giveitem ITEM_PIKANITE
	setflag FLAG_RECEIVED_PIKACHU_PARTNER
	releaseall
	end

PKMN_Center_Eventscript_SetflagEeveePartner::
	giveitem ITEM_EEVITE
	setflag FLAG_RECEIVED_EEVEE_PARTNER
	releaseall
	end

PKMN_Center_Eventscript_SetflagMeowthPartner::
	giveitem ITEM_MEOWTHITE
	giveitem ITEM_AMULET_COIN
	setflag FLAG_RECEIVED_MEOWTH_PARTNER
	releaseall
	end

PKMN_Center_Eventscript_SetflagTerapagosPartner::
	setflag FLAG_RECEIVED_TERAPAGOS
	releaseall
	end

PKMN_Center_EventScript_Mystery_Gift_NoGiftAvailable::
	msgbox PKMN_Center_Text_Mystery_Gift_NoGiftAvailable, MSGBOX_DEFAULT
	releaseall
	end

PKMN_Center_Mystery_Gift_EventScript_ReceiveGiftParty::
	call PKMN_Center_Mystery_Gift_EventScript_ReceivedGiftFanfare
	msgbox gText_NicknameThisPokemon, MSGBOX_YESNO
	compare VAR_RESULT, NO
	goto_if_eq PKMN_Center_Mystery_Gift_EventScript_TakeCare
	call Common_EventScript_GetGiftMonPartySlot
	call Common_EventScript_NameReceivedPartyMon
	goto PKMN_Center_Mystery_Gift_EventScript_TakeCare
	end

PKMN_Center_Mystery_Gift_EventScript_ReceiveGiftPC::
	call PKMN_Center_Mystery_Gift_EventScript_ReceivedGiftFanfare
	msgbox gText_NicknameThisPokemon, MSGBOX_YESNO
	compare VAR_RESULT, NO
	goto_if_eq PKMN_Center_Mystery_Gift_EventScript_SendGiftToPC
	call Common_EventScript_NameReceivedBoxMon
	goto PKMN_Center_Mystery_Gift_EventScript_SendGiftToPC
	end

PKMN_Center_Mystery_Gift_EventScript_SendGiftToPC::
	call Common_EventScript_TransferredToPC
	goto PKMN_Center_Mystery_Gift_EventScript_TakeCare
	end

PKMN_Center_Mystery_Gift_EventScript_ReceivedGiftFanfare::
	bufferspeciesname 1, VAR_TEMP_1
	playfanfare MUS_OBTAIN_ITEM
	message PKMN_Center_Mystery_Gift_Text_ReceivedGift
	waitmessage
	waitfanfare
	bufferspeciesname 0, VAR_TEMP_1
	return

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ Strings @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

PKMN_Center_EventReset_Text_Reset:
	.string "Events reset!$"

PKMN_Center_EventReset_Text_AlreadyReset:
	.string "You've already reset events.$"

PKMN_Center_EventResetText_WhatsBrokenThisTime:
	.string "If you've updated from an earlier\n"
	.string "version of Elite Redux, you may\l"
	.string "have encountered some buggy events.\p"
	.string "If needed, I can reset these events\n"
	.string "so they can be played again.\p"
	.string "Each event can only be reset\n"
	.string "once per save.\p"
	.string "Would you like to reset these events?$"

PKMN_Center_EventResetText_AlreadyReset:

PKMN_Center_Move_Tutor_Text_HowCanHelp:
	.string "How can I help you today?$"

PKMN_Center_Move_Tutor_Text_Explanation:
	.string "I can teach your Pokémon moves\n"
	.string "that they can't learn anywhere else.\p"
	.string "There's no charge, but you'll only\n"
	.string "have access to some moves to start\l"
	.string "with.\p"
	.string "As you earn more Gym Badges, I'll\n"
	.string "be able to teach you more moves,\l"
	.string "so please check back often!$"

PKMN_Center_Move_Tutor_Text_NoBadgeNoMoves:
	.string "I'm sorry, but there are no moves I\n"
	.string "can teach you right now.\p"
	.string "Come back when you have at least\n"
	.string "one Gym Badge.$"

PKMN_Center_Move_Tutor_Text_WhichSetofMoves:
	.string "Which set of moves would you\n"
	.string "like to choose from?$"

PKMN_Center_Move_Tutor_Text_WhichMove:
	.string "Which move should I teach to your\n"
	.string "Pokémon?$"
	
PKMN_Center_Move_Tutor_Text_IsThisRightMove:
	.string "The move {STR_VAR_1}, is it?$"

PKMN_Center_Move_Tutor_Text_TeachMoveToWhichMon:
	.string "Which Pokémon should learn\n"
	.string "{STR_VAR_1}?$"
	
PKMN_Center_Move_Tutor_Text_TeachAnotherMove:
	.string "Would you like me to teach\n"
    .string "another move?$"

PKMN_Center_Move_Tutor_Text_SeeYou:
	.string "See you later, then!$"

PKMN_Center_MoveReminder_Text_WantMeToTeachMove:
	.string "Would you like a Pokémon to remember\n"
	.string "a move it has forgotten?$"

PKMN_Center_MoveReminder_Text_TutorWhichMon:
	.string "Which Pokémon should remember a move?$"

PKMN_Center_MoveReminder_Text_TeachWhichMove:
	.string "Which move would you like it to\n"
	.string "remember?$"

PKMN_Center_MoveReminder_Text_DontHaveMoveToTeachPokemon:
	.string "I'm sorry, that Pokémon doesn't have\n"
	.string "any moves it can remember.$"

PKMN_Center_MoveReminder_Text_CantTeachEgg:
	.string "I'm sorry, an Egg can't remember any\n"
	.string "moves.$"

PKMN_Center_MoveReminder_Text_RememberAnotherMove:
	.string "Would you like a Pokémon to remember\n"
	.string "another move?$"

PKMN_Center_MoveDeleter_Text_ICanMakeMonForgetMove:
	.string "Would you like one of your Pokémon\n"
	.string "to forget a move?$"

PKMN_Center_MoveDeleter_Text_WhichMonShouldForget:
	.string "Which Pokémon should forget a move?$"

PKMN_Center_MoveDeleter_Text_WhichMoveShouldBeForgotten:
	.string "Which move should be forgotten?$"

PKMN_Center_MoveDeleter_Text_MonOnlyKnowsOneMove:
	.string "{STR_VAR_1} knows only one move,\n"
	.string "so it can't be forgotten.$"

PKMN_Center_MoveDeleter_Text_MonsMoveShouldBeForgotten:
	.string "Hm! {STR_VAR_1}'s {STR_VAR_2}?\n"
	.string "That move should be forgotten?$"

PKMN_Center_MoveDeleter_Text_MonHasForgottenMove:
	.string "It worked to perfection!\p"
	.string "{STR_VAR_1} has forgotten\n"
	.string "{STR_VAR_2} completely.$"

PKMN_Center_MoveDeleter_Text_EggCantForgetMoves:
	.string "What?\n"
	.string "No Egg should know any moves.$"

PKMN_Center_MoveDeleter_Text_CantForgetSurf:
	.string "Hm!\p"
	.string "Your {STR_VAR_1} doesn't seem willing\n"
	.string "to forget Surf.$"

PKMN_Center_NameRater_Text_CritiqueWhichMonNickname:
	.string "Which Pokémon's nickname should\n"
	.string "be changed?$"

PKMN_Center_NameRater_Text_WhatShallNewNameBe:
	.string "What would you like {STR_VAR_1}'s\n"
	.string "new nickname to be?$"

PKMN_Center_NameRater_Text_MonShallBeKnownAsName:
	.string "Done! From now on, this Pokémon\n"
	.string "shall be known as {STR_VAR_1}!$"

PKMN_Center_NameRater_Text_NameNoDifferentYetSuperior:
	.string "Okay! {STR_VAR_1}'s nickname\n"
	.string "will stay as it is.$"

PKMN_Center_NameRater_Text_ThatIsMerelyAnEgg:
	.string "Now, now.\n"
	.string "That is merely an Egg!$"

PKMN_Center_Text_Mystery_Gift_NoGiftAvailable:
	.string "I'm sorry, there are no Mystery Gifts\n"
	.string "available at this time.$"

PKMN_Center_Mystery_Gift_Text_ReceivedGift::
	.string "{PLAYER} received {STR_VAR_1}!\p"
	.string "Take good care of {STR_VAR_1}!$"

PKMN_Center_Text_Mystery_Gift_WouldYouLikeGift::
	.string "Thank you for playing\n"
	.string "Elite Redux!\p"
	.string "You have a Mystery Gift available:\n"
	.string "a special {STR_VAR_1}!\p"
	.string "Would you like to receive\n"
	.string "{STR_VAR_1}?$"
