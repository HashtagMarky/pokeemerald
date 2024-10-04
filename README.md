# Ikigai OW Encounter Tools
```
Based off RHH's pokeemerald-expansion 1.9.2 https://github.com/rh-hideout/pokeemerald-expansion/
```
These are some of the tools I've made for my hack, aiming to provide a framework to develop your own overworld encounter feature. These rely heavily on **merrp**'s followers branch, so credit goes to them as well as the maintainers who have been recently added it to **pokeemerald-expansion**.

> **Note**: This branch tracks **pret**'s **pokeemerald**. Please cherry-pick the linked commits directly.

## Features
- [x] Create a wander in grass movement type for overworld encounters. (Found [here](https://github.com/Pawkkie/Team-Aquas-Asset-Repo/wiki/Create-Wander-in-Grass-Movement-Type))
- [x] Create a way to start a wild encounter based on an overworld mon object event, without having to specify the spcies in a script.
- [x] Create a way to specify the species of an overworld mon object event from a list of species.
- [x] Create a way to specify the species of an overworld mon object event from map wild encounter headers.

### Example - Petalburg Good Rod Encounters
![](https://github.com/HashtagMarky/pokeemerald/blob/ikigai/ow-encounters/ikiagi_ow_encounters.gif)

```
.set LOCALID_OW_MON 1

PetalburgCity_OnTransition:
    … … …
    setobjectaswildencounter LOCALID_OW_MON, ENCOUNTER_GOOD_ROD
    end

PetalburgCity_EventScript_OverworldMon::
    lock
    callnative GetOverworldMonSpecies
    bufferspeciesname STR_VAR_1, VAR_0x8004
    msgbox PetalburgCity_Text_OverworldMon, MSGBOX_DEFAULT
    closemessage
    startoverworldencounter 5
    release
    end
```

## Commits
### Start Overworld Encounters & Get Overworld Species
Start a wild battle against an overworld mon object event with a defined level.
```
.macro startoverworldencounter level=req
callnative GetOverworldMonSpecies
playmoncry VAR_0x8004, CRY_MODE_ENCOUNTER
createmon B_SIDE_OPPONENT, B_FLANK_LEFT, VAR_0x8004, \level, isShiny=VAR_0x8005
waitmoncry
dowildbattle
.endm
```

This commit adds a function that sets `VAR_0x8004` to a species based on an object event and `VAR_8005` to it's shinyness. If the object event isn't tied to a sepcies, it returns `SPECIES_NONE`, however this can be changed very easily. This function can be called in the `startoverworldencounter` which plays the relevant species cry before starting a wild encounter.

> **Note**: There is a known bug that I haven't yet squashed where adding a value an objects Sight Radius / Berry Tree ID may cause a species value to not be obtained correctly.

October 3rd 2024: [de53fa5e2d](https://github.com/HashtagMarky/pokeemerald/commit/de53fa5e2d1e46efe09e1d829137d030be81d402)

### Set Object as Wild Encounter
Sets a variable object event to an overworld mon for a wild encounter.
```
#define ENCOUNTER_FIXED         0
#define ENCOUNTER_LAND          1
#define ENCOUNTER_SURF          2
#define ENCOUNTER_ROCK_SMASH    3
#define ENCOUNTER_OLD_ROD       4
#define ENCOUNTER_GOOD_ROD      5
#define ENCOUNTER_SUPER_ROD     6
#define ENCOUNTER_TYPES         7

.macro setobjectaswildencounter localId:req, encounterType=ENCOUNTER_FIXED
.byte 0xe5
.2byte \localId
.byte \encounterType
release
.endm
```

This commits adds a few functions in order to change a variable object event into a species ready for an overworld encounter. By setting the encounter type to `ENCOUNTER_FIXED`, the species will be set using the function `ReturnFixedSpeciesEncounter`. This can be supplemented by adding to the switch statement, potentially with random chances or even by using map headers to determine encounters by map section or type. Using any other type will return a random species from the relevant encounter tables used. This also uses `SPAWN_ODDS` to define whether or not the object should be shown by setting it's flag. Objects are set to always spawn by default, but if you would like to decrease these odds, I would recommend giving the object temporary flags in order to allow them a new chance to spawn when transitioning to the map again.

> **Note**: I haven't played around with variable object events too much, and these may not play too nicely everywhere in vanilla as they are set at various points.

October 4th 2024: [28156b0b5f](https://github.com/HashtagMarky/pokeemerald/commit/28156b0b5fa6933f42c5673026d046d5e6c2e566)

## Further Work
My own implementation of dedicated overworld encounters feature is still a giant WIP, but if I make any updates on these tools or develop any others I will update this page. If you find any bugs, or even have any fixes, please feel free to submit a PR or ping me (HashtagMarky) a message on the TAH Discord.  
