PlayerHouse_MapScripts::
	.byte 0


PlayerRoomDoor::
# 4 "data/maps/PlayerHouse/scripts.pory"
	playse SE_DOOR
# 5 "data/maps/PlayerHouse/scripts.pory"
	warp MAP_PLAYER_ROOM, 0
	end


MomsRoomDoor::
# 9 "data/maps/PlayerHouse/scripts.pory"
	playse SE_DOOR
# 10 "data/maps/PlayerHouse/scripts.pory"
	warp MAP_MOMS_ROOM, 0
	end

