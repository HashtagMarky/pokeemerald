HaouliPokemonCenter_MapScripts::
# 2 "data/maps/HaouliPokemonCenter/scripts.pory"
	map_script MAP_SCRIPT_ON_TRANSITION, Route1_Pokecenter_SetHealSpot
	.byte 0


Route1_Pokecenter_SetHealSpot::
# 6 "data/maps/HaouliPokemonCenter/scripts.pory"
	setrespawn HEAL_LOCATION_ROUTE1N
	end


HaouliPokemonCenter_EventScript_Nurse::
# 11 "data/maps/HaouliPokemonCenter/scripts.pory"
	setvar VAR_0x800B, Route_1_Nurse
# 12 "data/maps/HaouliPokemonCenter/scripts.pory"
	call Common_EventScript_PkmnCenterNurse
# 13 "data/maps/HaouliPokemonCenter/scripts.pory"
	waitmessage
# 14 "data/maps/HaouliPokemonCenter/scripts.pory"
	waitbuttonpress
# 15 "data/maps/HaouliPokemonCenter/scripts.pory"
	release
	end

