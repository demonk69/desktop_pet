#include "pet_compiled_assets.h"

#include <stddef.h>
#include <string.h>

/* Palette-indexed source keeps the compiled resources small and readable. */
static const char *const patterns[PET_ASSET_IDLE_1 + 1U] = {
    [PET_ASSET_BOOT_0] =
        "......"
        ".BBBB."
        "BDBBDB"
        "BBBBBB"
        ".BBBB."
        "......",
    [PET_ASSET_BOOT_1] =
        "..FF.."
        ".FFFF."
        "FDFFDF"
        "FFFFFF"
        ".FDDF."
        "......",
    [PET_ASSET_IDLE] =
        "..FF.."
        ".FFFF."
        "FDFFDF"
        "FFFFFF"
        ".FFFF."
        "..DD..",
    [PET_ASSET_BLINK_HALF] =
        "..FF.."
        ".FFFF."
        "FDDDDF"
        "FFFFFF"
        ".FFFF."
        "..DD..",
    [PET_ASSET_BLINK_CLOSED] =
        "..FF.."
        ".FFFF."
        "FFFFFF"
        "FDDDDF"
        ".FFFF."
        "..DD..",
    [PET_ASSET_LOOK_LEFT] =
        "..FF.."
        ".FFFF."
        "DFFDFF"
        "FFFFFF"
        ".FFFF."
        "..DD..",
    [PET_ASSET_LOOK_RIGHT] =
        "..FF.."
        ".FFFF."
        "FFDFFD"
        "FFFFFF"
        ".FFFF."
        "..DD..",
    [PET_ASSET_HAPPY_0] =
        "..FF.."
        ".FFFF."
        "FDFFDF"
        "AFFFFA"
        ".FDDF."
        "......",
    [PET_ASSET_HAPPY_1] =
        "..LL.."
        ".LLLL."
        "LDLLDL"
        "ALLLLA"
        ".LDDL."
        "......",
    [PET_ASSET_SLEEP_0] =
        "..SS.."
        ".SSSS."
        "SssssS"
        "SSSSSS"
        ".SssS."
        "......",
    [PET_ASSET_SLEEP_1] =
        "......"
        "..SS.."
        ".SssS."
        ".SSSS."
        "..ss.."
        "......",
    [PET_ASSET_IDLE_1] =
        "..LL.."
        ".LLLL."
        "LDLLDL"
        "LLLLLL"
        ".LLLL."
        "..DD..",
};

static uint16_t palette_color(char key)
{
    switch (key) {
    case 'F': return 0xFEABU;
    case 'L': return 0xFEC9U;
    case 'D': return 0x20E5U;
    case 'A': return 0xFB31U;
    case 'B': return 0x5D9FU;
    case 'S': return 0x5BB7U;
    case 's': return 0x4314U;
    default: return 0x10C5U;
    }
}

static pet_status_t get_bitmap(void *context, pet_asset_id_t asset_id,
                               pet_bitmap_t *bitmap)
{
    pet_compiled_asset_provider_t *provider = context;
    if (provider == NULL || bitmap == NULL || asset_id == 0U ||
        asset_id > PET_ASSET_IDLE_1 || patterns[asset_id] == NULL) {
        return PET_STATUS_INVALID_ARGUMENT;
    }
    bitmap->pixels = provider->pixels[asset_id];
    bitmap->width = PET_COMPILED_ASSET_WIDTH;
    bitmap->height = PET_COMPILED_ASSET_HEIGHT;
    bitmap->stride_pixels = PET_COMPILED_ASSET_WIDTH;
    return PET_STATUS_OK;
}

static void release_bitmap(void *context, const pet_bitmap_t *bitmap)
{
    (void)context;
    (void)bitmap;
}

pet_status_t pet_compiled_asset_provider_init(pet_compiled_asset_provider_t *provider)
{
    pet_asset_id_t asset_id;
    if (provider == NULL) {
        return PET_STATUS_INVALID_ARGUMENT;
    }
    memset(provider, 0, sizeof(*provider));
    for (asset_id = PET_ASSET_BOOT_0; asset_id <= PET_ASSET_IDLE_1; asset_id++) {
        size_t pixel;
        if (patterns[asset_id] == NULL ||
            strlen(patterns[asset_id]) != PET_COMPILED_ASSET_WIDTH *
                                             PET_COMPILED_ASSET_HEIGHT) {
            return PET_STATUS_INVALID_ARGUMENT;
        }
        for (pixel = 0U;
             pixel < PET_COMPILED_ASSET_WIDTH * PET_COMPILED_ASSET_HEIGHT;
             pixel++) {
            provider->pixels[asset_id][pixel] = palette_color(patterns[asset_id][pixel]);
        }
    }
    provider->interface.context = provider;
    provider->interface.get_bitmap = get_bitmap;
    provider->interface.release_bitmap = release_bitmap;
    return PET_STATUS_OK;
}

const pet_asset_provider_t *pet_compiled_asset_provider_interface(
    pet_compiled_asset_provider_t *provider)
{
    return provider == NULL ? NULL : &provider->interface;
}
