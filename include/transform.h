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
void ChooseMonForTransform(void);\
void CreatePlayerMountSprite(u16 gfxId);
bool32 PlayerHasMountSprite(void);
void UpdatePlayerMountSprite(void);

/*
 * Script command with optional defer flag
 */
void TransformPlayer(struct ScriptContext *ctx);
void DetransformPlayer(struct ScriptContext *ctx);

/*
 * Simple helper for items:
 *  immediately transform into a species
 */
void TransformPlayerToSpeciesSimple(u16 species);

/*
 * ============================================================================
 *  Overworld handling
 * ============================================================================
 */
void SanitizePlayerTransformOnLoad(void);
void PlayerAvatarHandleBob(void);

#endif // GUARD_TRANSFORM_H
