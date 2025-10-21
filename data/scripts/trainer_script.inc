Std_RegisteredInMatchCall::
	buffertrainerclassname 0, VAR_0x8000
	buffertrainername 1, VAR_0x8000
	closemessage
	delay 30
	playfanfare MUS_REGISTER_MATCH_CALL
	msgbox gText_RegisteredTrainerinPokeNav, MSGBOX_DEFAULT
	waitfanfare
	closemessage
	delay 30
	return

EventScript_TryGetTrainerScript::
	goto_if_set FLAG_RAN_FROM_TRAINER, EventScript_TryGetTrainerScript_Cont
	special ShouldTryGetTrainerScript
	compare VAR_RESULT, 1
	goto_if_eq EventScript_GotoTrainerScript
EventScript_TryGetTrainerScript_Cont:
	releaseall
	end

EventScript_GotoTrainerScript::
	gotobeatenscript
	releaseall
	end

EventScript_ObjectApproachPlayer::
	lock
	playse SE_PIN
	special DoTrainerApproach
	waitstate
	gotonative LoadTrainerObjectScript
	end
