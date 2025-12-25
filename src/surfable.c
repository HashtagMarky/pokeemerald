#include "global.h"
#include "event_data.h"
#include "event_object_movement.h"
#include "field_effect.h"
#include "field_effect_helpers.h"
#include "field_player_avatar.h"
#include "main.h"
#include "party_menu.h"
#include "sprite.h"
#include "surfable.h"
#include "constants/event_object_movement.h"
#include "constants/event_objects.h"
#include "constants/field_effects.h"
#include "constants/moves.h"
#include "constants/species.h"

extern const struct OamData gObjectEventBaseOam_32x32;
extern const struct OamData gObjectEventBaseOam_64x64;
extern const struct SpriteTemplate *const gFieldEffectObjectTemplatePointers[];

extern void SynchroniseSurfAnim(struct ObjectEvent *playerObj, struct Sprite *sprite);
extern void SynchroniseSurfPosition(struct ObjectEvent *playerObj, struct Sprite *sprite);

static void CreateOverlaySprite(void);
static void UpdateSurfMonOverlay(struct Sprite *sprite);
extern void UpdateBobbingEffect(struct ObjectEvent *playerObj, struct Sprite *playerSprite, struct Sprite *sprite);


struct RideablePokemon
{
    u16 species;
    u8 trainerPose;
};

#include "data/object_events/surfable/surfable_pokemon.h"
#include "data/object_events/surfable/surfable_pokemon_graphics.h"
#include "data/object_events/surfable/surfable_pokemon_pic_tables.h"
#include "data/object_events/surfable/surfable_pokemon_templates.h"

static EWRAM_DATA u16 sCurrentSurfMon = {0};



static u16 GetSurfMonSpecies(void)
{
    u8 surfMonChoice = VarGet(VAR_SURF_MON_SLOT);

    switch (surfMonChoice)
    {
    case SURF_MON_LAPRAS: // If surfMonChoice is 0
        return SPECIES_LAPRAS;
    case SURF_MON_SHARPEDO: // If surfMonChoice is 1
        return SPECIES_SHARPEDO;
    default:
        // This is the correct way to default to Lapras:
        return SPECIES_LAPRAS; // Return the actual species ID of Lapras
    }
}

static u16 GetSurfablePokemonSprite(void)
{
    u8 i;
    u16 mon = GetSurfMonSpecies();

    for (i = 0; i < ARRAY_COUNT(gSurfablePokemon); i++)
    {
        if (mon == gSurfablePokemon[i].species)
            return i;
    }
    return 0xFFFF;
}

static void LoadSurfOverworldPalette(void)
{
    // sCurrentSurfMon is already correctly determined elsewhere (0 for Lapras, 1 for Sharpedo)
    // and tells us which sprite/palette entry to use.

    // Check if the FIRST Pokémon in the party (gPlayerParty[0]) is shiny.
    // This assumes gPlayerParty[0] always exists and holds a valid Pokémon data structure.
    if (IsMonShiny(&gPlayerParty[0]) == TRUE)
    {
        // If the first Pokémon is shiny, load the shiny palette for the current surf mon
        LoadSpritePalette(&sSurfablePokemonShinyPalettes[sCurrentSurfMon]);
    }
    else
    {
        // Otherwise, load the normal palette for the current surf mon
        LoadSpritePalette(&sSurfablePokemonPalettes[sCurrentSurfMon]);
    }
}

u32 CreateSurfablePokemonSprite(void)
{
    u8 spriteId;
    struct Sprite *sprite;

    SetSpritePosToOffsetMapCoords((s16 *)&gFieldEffectArguments[0], (s16 *)&gFieldEffectArguments[1], 8, 8);

    sCurrentSurfMon = GetSurfablePokemonSprite();
    if (sCurrentSurfMon != 0xFFFF)
    {
        LoadSurfOverworldPalette();
        spriteId = CreateSpriteAtEnd(&gSurfablePokemonOverworldSprites[sCurrentSurfMon], gFieldEffectArguments[0], gFieldEffectArguments[1], 0x96);
        if (gSurfablePokemonOverlaySprites[sCurrentSurfMon].tileTag == 0xFFFF)
        {
            CreateOverlaySprite();
        }
    }
    else
    { // Create surf blob
        LoadObjectEventPalette(FLDEFFOBJ_SURF_BLOB);
        spriteId = CreateSpriteAtEnd(gFieldEffectObjectTemplatePointers[FLDEFFOBJ_SURF_BLOB], gFieldEffectArguments[0], gFieldEffectArguments[1], 0x96);
    }

    if (spriteId != MAX_SPRITES)
    {
        sprite = &gSprites[spriteId];
        sprite->coordOffsetEnabled = TRUE;
        sprite->data[2] = gFieldEffectArguments[2];
        sprite->data[3] = -1;
        sprite->data[6] = -1;
        sprite->data[7] = -1;
    }
    FieldEffectActiveListRemove(FLDEFF_SURF_BLOB);
    return spriteId;
}

static void CreateOverlaySprite(void)
{
    u8 overlaySprite;
    u8 subpriority;
    struct Sprite *sprite;

    subpriority = gSprites[gPlayerAvatar.spriteId].subpriority - 1;
    overlaySprite = CreateSpriteAtEnd(&gSurfablePokemonOverlaySprites[sCurrentSurfMon], gFieldEffectArguments[0], gFieldEffectArguments[1], subpriority);

    if (overlaySprite != MAX_SPRITES)
    {
        sprite = &gSprites[overlaySprite];
        sprite->coordOffsetEnabled = TRUE;
        sprite->data[2] = gFieldEffectArguments[2];
        sprite->data[3] = -1;
        sprite->data[6] = -1;
        sprite->data[7] = -1;
        sprite->oam.priority = 2;
    }
    SetSurfBlob_BobState(overlaySprite, BOB_PLAYER_AND_MON);
}

static void UpdateSurfMonOverlay(struct Sprite *sprite)
{
    struct ObjectEvent *playerObj;
    struct Sprite *linkedSprite;
    u8 subpriority;
	
    playerObj = &gObjectEvents[gPlayerAvatar.objectEventId];
    linkedSprite = &gSprites[playerObj->spriteId];

    SynchroniseSurfAnim(playerObj, sprite);
    SynchroniseSurfPosition(playerObj, sprite);

    if (VarGet(VAR_FREEZESURFBLOB) == 0)
	{
        UpdateBobbingEffect(playerObj, linkedSprite, sprite);
    }

    // Reset the subpriority for the overlay sprite so it shows on top of the player
    // We need this here so the subprio is correct after a screen transition (e.g. after exiting a battle)
    subpriority = gSprites[gPlayerAvatar.spriteId].subpriority - 1;
    sprite->subpriority = subpriority;

if (linkedSprite->animNum < MOVEMENT_ACTION_DELAY_16)
    if (linkedSprite->animNum < MOVEMENT_ACTION_DELAY_16)
    {
        sprite->x = linkedSprite->x;
        sprite->y = linkedSprite->y + 8;
        sprite->y2 = linkedSprite->y2;
    }
    if (!(gPlayerAvatar.flags & PLAYER_AVATAR_FLAG_SURFING))
        DestroySprite(sprite);
}

