enum 
{
    PAL_TAG_LAPRAS_SURF = 0x3001,
    PAL_TAG_SHARPEDO_SURF,
};

const struct SpritePalette sSurfablePokemonPalettes[] = {
    {gSurfablePokemonPalette_Lapras,     PAL_TAG_LAPRAS_SURF},
    {gSurfablePokemonPalette_Sharpedo,  PAL_TAG_SHARPEDO_SURF},
    
//#ifdef POKEMON_EXPANSION
//    {gSurfablePokemonPalette_KyogrePrimal,    PAL_TAG_KYOGRE_PRIMAL_SURF},
//#endif
};

const struct SpritePalette sSurfablePokemonShinyPalettes[] = {
    {gSurfablePokemonShinyPalette_Lapras,     PAL_TAG_LAPRAS_SURF},
    {gSurfablePokemonShinyPalette_Sharpedo,  PAL_TAG_SHARPEDO_SURF},
//#ifdef POKEMON_EXPANSION
//    {gSurfablePokemonShinyPalette_KyogrePrimal,    PAL_TAG_KYOGRE_PRIMAL_SURF},
//#endif
};

const union AnimCmd gSurfablePokemonAnim_FaceSouth[] =
{
    ANIMCMD_FRAME(2, 16),
    ANIMCMD_FRAME(3, 16),
    ANIMCMD_JUMP(0),
};

const union AnimCmd gSurfablePokemonAnim_FaceNorth[] =
{
    ANIMCMD_FRAME(0, 16),
    ANIMCMD_FRAME(1, 16),
    ANIMCMD_JUMP(0),
};

const union AnimCmd gSurfablePokemonAnim_FaceWest[] =
{
    ANIMCMD_FRAME(4, 16),
    ANIMCMD_FRAME(5, 16),
    ANIMCMD_JUMP(0),
};

const union AnimCmd gSurfablePokemonAnim_FaceEast[] =
{
    ANIMCMD_FRAME(4, 16, .hFlip = TRUE),
    ANIMCMD_FRAME(5, 16, .hFlip = TRUE),
    ANIMCMD_JUMP(0),
};

const union AnimCmd gSurfablePokemonAnim_NoFlipFaceEast[] =
{
    ANIMCMD_FRAME(6, 16),
    ANIMCMD_FRAME(7, 16),
    ANIMCMD_JUMP(0),
};

const union AnimCmd *const gSurfablePokemonAnimTable[] =
{
    gSurfablePokemonAnim_FaceSouth,
    gSurfablePokemonAnim_FaceNorth,
    gSurfablePokemonAnim_FaceWest,
    gSurfablePokemonAnim_FaceEast,
};

const union AnimCmd *const gSurfablePokemonNoFlipAnimTable[] =
{
    gSurfablePokemonAnim_FaceSouth,
    gSurfablePokemonAnim_FaceNorth,
    gSurfablePokemonAnim_FaceWest,
    gSurfablePokemonAnim_NoFlipFaceEast,
};

const struct SpriteTemplate gSurfablePokemonOverworldSprites[] =
{
    {0xFFFF, PAL_TAG_LAPRAS_SURF,     &gObjectEventBaseOam_32x32, gSurfablePokemonAnimTable, gSurfingOverworldPicTable_Lapras,     gDummySpriteAffineAnimTable, UpdateSurfBlobFieldEffect},
    {0xFFFF, PAL_TAG_SHARPEDO_SURF,  &gObjectEventBaseOam_32x32, gSurfablePokemonAnimTable, gSurfingOverworldPicTable_Sharpedo,  gDummySpriteAffineAnimTable, UpdateSurfBlobFieldEffect},
//#ifdef POKEMON_EXPANSION
//    {0xFFFF, PAL_TAG_KYOGRE_PRIMAL_SURF,    &gObjectEventBaseOam_64x64, gSurfablePokemonAnimTable, gSurfingOverworldPicTable_KyogrePrimal,    gDummySpriteAffineAnimTable, UpdateSurfBlobFieldEffect},
//#endif
};

#define NO_OVERLAY {0, 0, NULL, NULL, NULL, NULL, NULL}
const struct SpriteTemplate gSurfablePokemonOverlaySprites[] =
{
    
    {0xFFFF, PAL_TAG_LAPRAS_SURF,     &gObjectEventBaseOam_32x32, gSurfablePokemonAnimTable, gSurfingOverlayPicTable_Lapras,     gDummySpriteAffineAnimTable, UpdateSurfMonOverlay},
    {0xFFFF, PAL_TAG_SHARPEDO_SURF,  &gObjectEventBaseOam_32x32, gSurfablePokemonAnimTable, gSurfingOverlayPicTable_Sharpedo,  gDummySpriteAffineAnimTable, UpdateSurfMonOverlay},
    
};
