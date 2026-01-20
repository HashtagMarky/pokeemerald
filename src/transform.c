#include "global.h"
#include "constants/flags.h"
#include "constants/species.h"
#include "constants/event_objects.h"
#include "event_data.h"
#include "field_player_avatar.h"
#include "fieldmap.h"
#include "overworld.h"
#include "script.h"
#include "sprite.h"
#include "transform.h"
#include "event_object_movement.h"
#include "follower_npc.h"
#include "overworld.h"
#include "gpu_regs.h"
#include "trig.h"
#include "task.h"
#include "sound.h"
#include "constants/songs.h"


enum
{
    RIDE_SPRITE_DIR_DOWN, // Now 0
    RIDE_SPRITE_DIR_UP,   // Now 1
    RIDE_SPRITE_DIR_WEST, // Now 2
    RIDE_SPRITE_DIR_EAST, // Now 3
    RIDE_SPRITE_DIR_COUNT,
};

enum
{
    RIDER_SHOW_INFRONT,
    RIDER_SHOW_BEHIND,
};

struct RideSpriteInfo
{
    s8 playerX;
    s8 playerY;
    u8 playerRendersInFront;
};

struct RideMonInfo
{
    u16 riderGfxId[GENDER_COUNT];
    struct RideSpriteInfo spriteInfo[RIDE_SPRITE_DIR_COUNT];
};

static const struct RideMonInfo sRideMonInfo[NUM_SPECIES] = {
    [SPECIES_TAUROS] = {
        .riderGfxId = { [MALE] = OBJ_EVENT_GFX_ELIO_RIDING, [FEMALE] = OBJ_EVENT_GFX_SELENE_RIDING },
        .spriteInfo = {
            [RIDE_SPRITE_DIR_DOWN] = { .playerX=0,  .playerY=-10, .playerRendersInFront=RIDER_SHOW_INFRONT },
            [RIDE_SPRITE_DIR_UP]   = { .playerX=0,  .playerY=-7,  .playerRendersInFront=RIDER_SHOW_INFRONT },
            [RIDE_SPRITE_DIR_WEST] = { .playerX=1, .playerY=-6,  .playerRendersInFront=RIDER_SHOW_INFRONT },
            [RIDE_SPRITE_DIR_EAST] = { .playerX=-1,  .playerY=-6,  .playerRendersInFront=RIDER_SHOW_INFRONT },
        },
    },
    [SPECIES_STOUTLAND] = {
        .riderGfxId = { [MALE] = OBJ_EVENT_GFX_ELIO_RIDING, [FEMALE] = OBJ_EVENT_GFX_SELENE_RIDING },
        .spriteInfo = {
            [RIDE_SPRITE_DIR_DOWN] = { .playerX=0,  .playerY=-8,  .playerRendersInFront=RIDER_SHOW_INFRONT },
            [RIDE_SPRITE_DIR_UP]   = { .playerX=0,  .playerY=-7,  .playerRendersInFront=RIDER_SHOW_INFRONT },
            [RIDE_SPRITE_DIR_WEST] = { .playerX=3, .playerY=-6,  .playerRendersInFront=RIDER_SHOW_INFRONT },
            [RIDE_SPRITE_DIR_EAST] = { .playerX=-3,  .playerY=-6,  .playerRendersInFront=RIDER_SHOW_INFRONT },
        }
    },
    [SPECIES_MUDSDALE] = {
        .riderGfxId = { [MALE] = OBJ_EVENT_GFX_ELIO_RIDING, [FEMALE] = OBJ_EVENT_GFX_SELENE_RIDING },
        .spriteInfo = {
            [RIDE_SPRITE_DIR_DOWN] = { .playerX=0,  .playerY=-8,  .playerRendersInFront=RIDER_SHOW_BEHIND },
            [RIDE_SPRITE_DIR_UP]   = { .playerX=0,  .playerY=-8,  .playerRendersInFront=RIDER_SHOW_INFRONT },
            [RIDE_SPRITE_DIR_WEST] = { .playerX=3, .playerY=-7,  .playerRendersInFront=RIDER_SHOW_INFRONT },
            [RIDE_SPRITE_DIR_EAST] = { .playerX=-3,  .playerY=-7,  .playerRendersInFront=RIDER_SHOW_INFRONT },
        }
    },
    [SPECIES_MACHAMP] = {
        .riderGfxId = { [MALE] = OBJ_EVENT_GFX_ELIO_RIDING, [FEMALE] = OBJ_EVENT_GFX_SELENE_RIDING },
        .spriteInfo = {
            [RIDE_SPRITE_DIR_DOWN] = { .playerX=0,  .playerY=-6,  .playerRendersInFront=RIDER_SHOW_BEHIND },
            [RIDE_SPRITE_DIR_UP]   = { .playerX=0,  .playerY=-6,  .playerRendersInFront=RIDER_SHOW_INFRONT },
            [RIDE_SPRITE_DIR_WEST] = { .playerX=4, .playerY=-6,  .playerRendersInFront=RIDER_SHOW_BEHIND },
            [RIDE_SPRITE_DIR_EAST] = { .playerX=-4,  .playerY=-6,  .playerRendersInFront=RIDER_SHOW_BEHIND },
        }
    },
};

/*
 * ============================================================================
 *  CONFIG
 * ============================================================================
 */

#define MOSAIC_MAX 8
#define MOSAIC_FRAMES_PER_STEP 1

#define tMosaic      data[0]
#define tCounter     data[1]
#define tState       data[2]
#define tSpecies     data[3]

#define STEP_FRAME_DURATION 8

#define RIDER_CENTER_X_OFFSET   0   // (32 - 16) / 2
#define RIDER_CENTER_Y_OFFSET  -10 // sits slightly above center


/*
 * ============================================================================
 *  STATE
 * ============================================================================
 */
#define gPlayerTransformSpecies (gSaveBlock2Ptr->pokemonAvatarSpecies)
EWRAM_DATA struct PlayerAvatarBobState gPlayerAvatarBobState = {0};
EWRAM_DATA s16 sPlayerMountSpriteId;

/*
 * ============================================================================
 *  INTERNAL HELPERS
 * ============================================================================
 */

static void ResetPlayerAvatar(void);
static void SetPlayerTransformFlags(void);
static void ClearPlayerTransformFlags(void);

static void DestroyPlayerMountSprite(void);
static void UpdatePlayerMountSpritePosition(struct Sprite *sprite);

static u16 GetTransformGraphicsIdFromSpecies(void)
{
    u16 gfxId = gPlayerTransformSpecies + OBJ_EVENT_MON;

    if (FlagGet(FLAG_SHINY_RIDE))
        gfxId += OBJ_EVENT_MON_SHINY;

    return gfxId;
}

/*
 * ============================================================================
 *  PUBLIC API (used by graphics hook)
 * ============================================================================
 */

struct TransformTaskData
{
    u8 mosaic;      // current mosaic size
    u8 frameCounter; // sub-frame counter
    u8 state;       // 0=grow mosaic, 1=transform, 2=shrink mosaic
    u16 species;    // species to transform into
};

// 1. Update the helper to set the hardware register AND the sprite bit
static void SetPlayerMosaic(u8 size)
{
    // 1. Set the Hardware Register (Size of blocks)
    // Bits 8-11: OBJ Horizontal, Bits 12-15: OBJ Vertical
    u16 objMosaic = (size & 0xF) | ((size & 0xF) << 4);
    SetGpuReg(REG_OFFSET_MOSAIC, (objMosaic << 8));

    // 2. Enable the Mosaic bit on the current player sprite
    if (gSprites[gPlayerAvatar.spriteId].inUse)
        gSprites[gPlayerAvatar.spriteId].oam.mosaic = TRUE;
}

static void ResetPlayerMosaic(void)
{
    struct Sprite *sprite = &gSprites[gPlayerAvatar.spriteId];
    if (sprite->inUse)
        sprite->oam.mosaic = FALSE;
    
    // Clear OBJ bits in the hardware register
    SetGpuReg(REG_OFFSET_MOSAIC, GetGpuReg(REG_OFFSET_MOSAIC) & 0x00FF);
}

static void Task_TransformMosaic(u8 taskId)
{
    struct Task *task = &gTasks[taskId];
    u8 frames = task->tCounter;
    u8 stretch;
    struct Sprite *playerSprite = NULL;

    if (gPlayerAvatar.spriteId < MAX_SPRITES &&
        gSprites[gPlayerAvatar.spriteId].inUse)
    {
        playerSprite = &gSprites[gPlayerAvatar.spriteId];
    }

    /* End condition */
    if (frames >= 16)
    {
        if (playerSprite)
            playerSprite->oam.mosaic = FALSE;

        /* Clear OBJ mosaic bits only */
        SetGpuReg(REG_OFFSET_MOSAIC, GetGpuReg(REG_OFFSET_MOSAIC) & 0x00FF);

        DestroyTask(taskId);
        return;
    }

    /* Compute mosaic stretch */
    if (frames < 8)
        stretch = frames >> 1;
    else
        stretch = (16 - frames) >> 1;

    if (playerSprite)
        playerSprite->oam.mosaic = TRUE;

    /* Apply OBJ mosaic safely */
    {
        u16 mosaic = GetGpuReg(REG_OFFSET_MOSAIC) & 0x00FF;
        mosaic |= (stretch << 12) | (stretch << 8);
        SetGpuReg(REG_OFFSET_MOSAIC, mosaic);
    }

    /* Midpoint: ACTUAL transform */
    if (frames == 8)
    {
        PlaySE(SE_M_TELEPORT);

        /* Prevent movement for ONE frame only */
        gPlayerAvatar.preventStep = TRUE;

        gPlayerTransformSpecies = task->tSpecies;

        DestroyPlayerMountSprite();

        if (task->tSpecies == SPECIES_NONE)
        {
            ClearPlayerTransformFlags();
            ResetPlayerAvatar();

            if (!FlagGet(FLAG_DISABLE_FOLLOWERS))
                UpdateFollowingPokemon();
        }
        else
        {
            SetPlayerTransformFlags();
            ResetPlayerAvatar();
            CreatePlayerMountSprite(task->tSpecies);
        }
    }


    task->tCounter++;
}

static u8 GetRideSpriteDir(void)
{
    u8 direction = GetPlayerFacingDirection();
    switch (direction)
    {
        case DIR_NORTH: return RIDE_SPRITE_DIR_UP;
        case DIR_SOUTH: return RIDE_SPRITE_DIR_DOWN;
        case DIR_WEST:  return RIDE_SPRITE_DIR_WEST;
        case DIR_EAST:  return RIDE_SPRITE_DIR_EAST;
    }
    return RIDE_SPRITE_DIR_DOWN;
}

bool32 IsPlayerTransformed(void)
{
    return FlagGet(FLAG_PLAYER_IS_POKEMON) && gPlayerTransformSpecies != SPECIES_NONE;
}

u16 GetPlayerTransformGraphicsId(void)
{
    if (gPlayerTransformSpecies == SPECIES_NONE)
        return GetPlayerAvatarGraphicsIdByStateId(PLAYER_AVATAR_STATE_NORMAL);

    u16 gfxId = gPlayerTransformSpecies + OBJ_EVENT_MON;
    if (FlagGet(FLAG_SHINY_RIDE))
        gfxId += OBJ_EVENT_MON_SHINY;

    return gfxId;
}

void SanitizePlayerTransformOnLoad(void)
{
    if (!FlagGet(FLAG_PLAYER_IS_POKEMON))
    {
        gPlayerTransformSpecies = SPECIES_NONE;
        sPlayerMountSpriteId = -1; // Only reset if not transformed
    }
}

/*
 * ============================================================================
 *  AVATAR RESET
 * ============================================================================
 */

static void ResetPlayerAvatar(void)
{
    u16 x, y;
    u8 direction;
    struct ObjectEvent *playerObj;

    GetCameraFocusCoords(&x, &y);
    direction = GetPlayerFacingDirection();

    playerObj = &gObjectEvents[gPlayerAvatar.objectEventId];
    RemoveObjectEvent(playerObj);

    ClearPlayerAvatarInfo();
    InitPlayerAvatar(x, y, direction, gSaveBlock2Ptr->playerGender);
}

/*
 * ============================================================================
 *  FLAG MANAGEMENT
 * ============================================================================
 */

static void SetPlayerTransformFlags(void)
{
    FlagSet(FLAG_PLAYER_IS_POKEMON);
    FlagSet(FLAG_DISABLE_FOLLOWERS);
    FlagSet(FLAG_DEFER_TRANSFORM);
}

static void ClearPlayerTransformFlags(void)
{
    FlagClear(FLAG_PLAYER_IS_POKEMON);
    FlagClear(FLAG_DISABLE_FOLLOWERS);
    FlagClear(FLAG_DEFER_TRANSFORM);
    gPlayerTransformSpecies = SPECIES_NONE;

    /* Cleanup mount sprite if present */
    DestroyPlayerMountSprite();
}
/*
 * ============================================================================
 *  TRANSFORM ENTRY POINTS
 * ============================================================================
 */

/*
 * Script command:
 *  - VAR_TRANSFORM_MON holds species
 *  - SPECIES_NONE = detransform
 */
void ChooseMonForTransform(void)
{
    u16 species = VarGet(VAR_TRANSFORM_MON);
    u8 taskId;

    struct ObjectEvent *follower = GetFollowerObject();
    if (follower)
        RemoveObjectEvent(follower);

    taskId = CreateTask(Task_TransformMosaic, 0);
    gTasks[taskId].tCounter = 0; // This is our frame tracker now
    gTasks[taskId].tSpecies = (species >= NUM_SPECIES) ? SPECIES_NONE : species;
}

/*
 * Script command version with explicit defer flag
 */
void TransformPlayer(struct ScriptContext *ctx)
{
    bool32 defer = ScriptReadByte(ctx);
    u16 species = VarGet(VAR_TRANSFORM_MON);

    if (species == SPECIES_NONE || species >= NUM_SPECIES)
        return;

    gPlayerTransformSpecies = species;

    if (defer)
    {
        FlagSet(FLAG_PLAYER_IS_POKEMON);
        FlagSet(FLAG_DEFER_TRANSFORM);
        FlagSet(FLAG_DISABLE_FOLLOWERS);
    }
    else
    {
        SetPlayerTransformFlags();
        ResetPlayerAvatar();
    }
}

void TransformPlayerToSpeciesSimple(u16 species)
{
    u8 taskId;

    struct ObjectEvent *follower = GetFollowerObject();
    if (follower)
        RemoveObjectEvent(follower);

    taskId = CreateTask(Task_TransformMosaic, 0);
    gTasks[taskId].tCounter = 0;
    gTasks[taskId].tSpecies = species;
}
void DetransformPlayer(struct ScriptContext *ctx)
{
    bool32 defer = ScriptReadByte(ctx);

    if (defer)
    {
        ClearPlayerTransformFlags();
    }
    else
    {
        ClearPlayerTransformFlags();
        ResetPlayerAvatar();

        // Respect the menu toggle
        if (!FlagGet(FLAG_DISABLE_FOLLOWERS))
            UpdateFollowingPokemon();

        /* Ensure any mount sprite is removed */
        DestroyPlayerMountSprite();
    }
}

void CreatePlayerMountSprite(u16 species)
{
    s16 spriteId;
    struct Sprite *playerSpr;
    struct Sprite *mountSpr;
    const struct RideMonInfo *info;
    u8 gender = gSaveBlock2Ptr->playerGender;

    DestroyPlayerMountSprite();

    info = &sRideMonInfo[species];
    if (info->riderGfxId[gender] == 0)
        return;

    spriteId = CreateObjectGraphicsSpriteWithTag(
        info->riderGfxId[gender],
        UpdatePlayerMountSpritePosition, // The callback
        0, 0,
        0,
        TAG_NONE
    );

    if (spriteId == MAX_SPRITES)
        return;

    sPlayerMountSpriteId = spriteId;

    playerSpr = &gSprites[gPlayerAvatar.spriteId];
    mountSpr  = &gSprites[spriteId];

    mountSpr->coordOffsetEnabled = TRUE;
    mountSpr->oam.priority = playerSpr->oam.priority;
    mountSpr->images = GetObjectEventGraphicsInfo(info->riderGfxId[gender])->images;
    
    // Set initial visibility
    mountSpr->invisible = playerSpr->invisible;
    
    UpdatePlayerMountSpritePosition(mountSpr);
}

static void UpdateRiderGraphics(void)
{
    struct Sprite *mountSpr;
    u8 dir;

    if (sPlayerMountSpriteId < 0)
        return;

    mountSpr = &gSprites[sPlayerMountSpriteId];
    dir = GetRideSpriteDir(); // Now returns 0, 1, 2, or 3
    
    // We no longer need actualAnim logic. 
    // We just pass the direction (0-3) directly to the animation system.
    if (mountSpr->animNum != dir)
    {
        StartSpriteAnim(mountSpr, dir);
    }

    // REMOVED: All hFlip and matrixNum logic.
    // The 4th frame in your PNG handles the facing direction naturally.

    mountSpr->animPaused = TRUE;
    mountSpr->animCmdIndex = 0;
    mountSpr->animDelayCounter = 0;
}

static void DestroyPlayerMountSprite(void)
{
    if (sPlayerMountSpriteId >= 0)
    {
        if (gSprites[sPlayerMountSpriteId].inUse)
            DestroySprite(&gSprites[sPlayerMountSpriteId]);
        sPlayerMountSpriteId = -1;
    }
}

static void UpdatePlayerMountSpritePosition(struct Sprite *mountSpr)
{
    struct Sprite *playerSpr;
    const struct RideMonInfo *info;
    const struct RideSpriteInfo *dirInfo;
    u8 dir;

    if (!IsPlayerTransformed() || sPlayerMountSpriteId < 0)
    {
        DestroyPlayerMountSprite();
        return;
    }

    playerSpr = &gSprites[gPlayerAvatar.spriteId];
    if (!playerSpr->inUse) return;

    mountSpr->invisible = playerSpr->invisible;
    dir = GetRideSpriteDir();
    
    info = &sRideMonInfo[gPlayerTransformSpecies];
    dirInfo = &info->spriteInfo[dir];

    mountSpr->x = playerSpr->x;
    mountSpr->y = playerSpr->y;

    // Direct offset application. 
    // West will use the negative value, East will use the positive value from your table.
    mountSpr->x2 = playerSpr->x2 + dirInfo->playerX;
    mountSpr->y2 = playerSpr->y2 + dirInfo->playerY;

    mountSpr->subpriority = (dirInfo->playerRendersInFront == RIDER_SHOW_INFRONT) 
                            ? playerSpr->subpriority - 1 
                            : playerSpr->subpriority + 1;
}




/*
 * ============================================================================
 *  BOB ANIMATION
 * ============================================================================
 */

void PlayerAvatarHandleBob(void)
{
    struct Sprite *sprite;

    if (!IsPlayerTransformed() || gObjectEvents[gPlayerAvatar.objectEventId].graphicsId < OBJ_EVENT_MON)
    {
        if (gPlayerAvatar.spriteId < MAX_SPRITES)
            gSprites[gPlayerAvatar.spriteId].y2 = 0;

        gPlayerAvatarBobState.frameCounter = 0;
        gPlayerAvatarBobState.spriteOffset = 0;
        return;
    }

    sprite = &gSprites[gPlayerAvatar.spriteId];

    if (gPlayerAvatarBobState.frameCounter == 0)
        gPlayerAvatarBobState.spriteOffset = Q_4_12(1.0);

    if (gPlayerAvatarBobState.frameCounter == STEP_FRAME_DURATION)
        gPlayerAvatarBobState.spriteOffset -= Q_4_12(0.5);

    if (gPlayerAvatarBobState.frameCounter == STEP_FRAME_DURATION * 2)
    {
        gPlayerAvatarBobState.spriteOffset += Q_4_12(0.5);
        gPlayerAvatarBobState.frameCounter = 0;
        return;
    }

    sprite->y2 = Q_4_12_TO_INT(gPlayerAvatarBobState.spriteOffset);
    gPlayerAvatarBobState.frameCounter++;

    // Only update graphics (anim frames) here. 
    // The position is handled automatically by the sprite's callback.
    UpdateRiderGraphics();
}

bool32 PlayerHasMountSprite(void)
{
    return sPlayerMountSpriteId >= 0
        && sPlayerMountSpriteId < MAX_SPRITES
        && gSprites[sPlayerMountSpriteId].inUse;
}

void UpdatePlayerMountSprite(void)
{
    struct Sprite *player = &gSprites[gPlayerAvatar.spriteId];
    struct Sprite *mount  = &gSprites[sPlayerMountSpriteId];

    // Position sync
    mount->x = player->x;
    mount->y = player->y;

    // Direction / anim sync
    if (mount->animNum != player->animNum)
        StartSpriteAnim(mount, player->animNum);

    // Advance frames (THIS fixes your frozen animation)
    AnimateSprite(mount);
}

void OnResetSpriteData(void)
{
    sPlayerMountSpriteId = -1;
}