#ifndef GUARD_TRANSFORM_H
#define GUARD_TRANSFORM_H

#include "global.h"
#include "script.h"

/*
 * ============================================================================
 *  Bob animation state
 * ============================================================================
 */

struct PlayerAvatarBobState
{
    u16 frameCounter;
    u16 spriteOffset;
};

extern struct PlayerAvatarBobState gPlayerAvatarBobState;

/*
 * ============================================================================
 *  Transform state accessors
 * ============================================================================
 */

/* Returns TRUE if the player is currently a Pokémon */
bool32 IsPlayerTransformed(void);

/* Returns the overworld graphicsId for the current transformed species */
u16 GetPlayerTransformGraphicsId(void);

/* Check if a species can be ridden (has valid ride data) */
bool8 CanRideOnSpecies(u16 species);

extern u16 gPlayerTransformSpecies;
void TransformPlayerToSpeciesSimple(u16 species);
/*
 * ============================================================================
 *  Transform control (scripts / items)
 * ============================================================================
 */

/*
 * Uses VAR_TRANSFORM_MON:
 *  - SPECIES_NONE → detransform
 *  - otherwise → transform into species
 */
void ChooseMonForTransform(void);
void CreatePlayerMountSprite(u16 gfxId);
void DestroyPlayerMountSprite(void);
bool32 PlayerHasMountSprite(void);
void UpdatePlayerMountSprite(void);
void Task_UpdatePlayerTransformAnimation(u8 taskId);

/*
 * Script command with optional defer flag
 */
void TransformPlayer(struct ScriptContext *ctx);
void DetransformPlayer(struct ScriptContext *ctx);

/*
 * Simple helper for items:
 *  immediately transform into a species
 */
void TransformPlayerToSpeciesScript(u16 species, bool8 unlockControls);

/*
 * ============================================================================
 *  Overworld handling
 * ============================================================================
 */
void SanitizePlayerTransformOnLoad(void);
void PlayerAvatarHandleBob(void);

void OnResetSpriteData(void);

#endif // GUARD_TRANSFORM_H
