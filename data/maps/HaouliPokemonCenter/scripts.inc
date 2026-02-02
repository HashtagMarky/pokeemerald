HaouliPokemonCenter_MapScripts::
	.byte 0


HaouliPokemonCenter_EventScript_Nurse::
# 5 "data/maps/HaouliPokemonCenter/scripts.pory"
	setvar VAR_0x800B, HauoliNurse1
# 6 "data/maps/HaouliPokemonCenter/scripts.pory"
	call Common_EventScript_PkmnCenterNurse
# 7 "data/maps/HaouliPokemonCenter/scripts.pory"
	waitmessage
# 8 "data/maps/HaouliPokemonCenter/scripts.pory"
	waitbuttonpress
# 9 "data/maps/HaouliPokemonCenter/scripts.pory"
	release
	end

