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

/*
 * ============================================================================
 *  STATE
 * ============================================================================
 */
#define gPlayerTransformSpecies (gSaveBlock2Ptr->pokemonAvatarSpecies)
EWRAM_DATA struct PlayerAvatarBobState gPlayerAvatarBobState = {0};

/*
 * ============================================================================
 *  INTERNAL HELPERS
 * ============================================================================
 */

static void ResetPlayerAvatar(void);
static void SetPlayerTransformFlags(void);
static void ClearPlayerTransformFlags(void);

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
    struct Sprite *playerSprite = &gSprites[gPlayerAvatar.spriteId];
    u8 stretch;

    u16 frames = task->tCounter;

    if (frames < 8)
        stretch = frames >> 1;
    else if (frames < 16)
        stretch = (16 - frames) >> 1;
    else 
    {
        if (playerSprite->inUse)
            playerSprite->oam.mosaic = FALSE;
        SetGpuReg(REG_OFFSET_MOSAIC, 0);
        DestroyTask(taskId);
        return;
    }

    if (playerSprite->inUse)
        playerSprite->oam.mosaic = TRUE;

    SetGpuReg(REG_OFFSET_MOSAIC, (stretch << 12) | (stretch << 8));

    if (frames == 8)
    {
        PlaySE(SE_M_TELEPORT);
        gPlayerTransformSpecies = task->tSpecies;
        
        if (task->tSpecies == SPECIES_NONE)
        {
            ClearPlayerTransformFlags(); 
            ResetPlayerAvatar(); 
            
            // CHECK THE SUPPRESSOR
            // If VAR_0x8004 is NOT 1, and menu allows it, show the follower
            if (VarGet(VAR_0x8004) != 1)
            {
                if (!FlagGet(FLAG_FOLLOWERS_MENU_TOGGLE))
                {
                    FlagClear(FLAG_DISABLE_FOLLOWERS);
                    UpdateFollowingPokemon();
                }
            }
            else
            {
                // It was suppressed (e.g. for Surfing), so keep them disabled
                FlagSet(FLAG_DISABLE_FOLLOWERS);
                VarSet(VAR_0x8004, 0); // Reset the suppressor for next time
            }
        }
        else
        {
            SetPlayerTransformFlags();
            ResetPlayerAvatar(); 
        }
        
        playerSprite = &gSprites[gPlayerAvatar.spriteId];
        if (playerSprite->inUse)
            playerSprite->oam.mosaic = TRUE;
    }

    task->tCounter++;
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
        gPlayerTransformSpecies = SPECIES_NONE;
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
    FlagClear(FLAG_DEFER_TRANSFORM);
    gPlayerTransformSpecies = SPECIES_NONE;

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
    }
}

/*
 * ============================================================================
 *  BOB ANIMATION
 * ============================================================================
 */

void PlayerAvatarHandleBob(void)
{
    struct Sprite *sprite;

    // If not transformed, or if we ARE transformed but the sprite is still a human (during transition)
    // We check if the graphicsId is less than the MON base ID to identify human sprites.
    if (!IsPlayerTransformed() || gObjectEvents[gPlayerAvatar.objectEventId].graphicsId < OBJ_EVENT_MON)
    {
        if (gPlayerAvatar.spriteId < MAX_SPRITES)
            gSprites[gPlayerAvatar.spriteId].y2 = 0;

        gPlayerAvatarBobState.frameCounter = 0;
        gPlayerAvatarBobState.spriteOffset = 0;
        return;
    }

    sprite = &gSprites[gPlayerAvatar.spriteId];

    // Standard Pokémon Bobbing Logic
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
}