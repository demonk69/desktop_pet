#include "pet_compiled_assets.h"

#include <stddef.h>
#include <string.h>

typedef struct {
    pet_asset_id_t asset_id;
    const char *pattern;
    const uint16_t *pixels;
    size_t width;
    size_t height;
    size_t stride_pixels;
    bool has_transparent_color;
    uint16_t transparent_color;
} compiled_asset_entry_t;

#define TEST_CHARACTER_TRANSPARENT_COLOR 0xF81FU
#define TEST_CHARACTER_FACE_0_COLOR      0xFEC0U
#define TEST_CHARACTER_FACE_1_COLOR      0xFFE0U
#define TEST_CHARACTER_HAPPY_COLOR       0xFB31U
#define TEST_CHARACTER_FEATURE_COLOR     0x20E5U

#define PX2(color)  color, color
#define PX4(color)  PX2(color), PX2(color)
#define PX8(color)  PX4(color), PX4(color)
#define PX12(color) PX8(color), PX4(color)
#define PX14(color) PX12(color), PX2(color)
#define PX16(color) PX8(color), PX8(color)
#define PX32(color) PX16(color), PX16(color)
#define PX36(color) PX32(color), PX4(color)
#define PX64(color) PX32(color), PX32(color)

#define TC_T TEST_CHARACTER_TRANSPARENT_COLOR
#define TC_D TEST_CHARACTER_FEATURE_COLOR
#define TC_TRANSPARENT_ROW PX64(TC_T)
#define TC_OUTLINE_ROW     PX14(TC_T), PX36(TC_D), PX14(TC_T)
#define TC_BODY_ROW(face) \
    PX14(TC_T), PX2(TC_D), PX32(face), PX2(TC_D), PX14(TC_T)
#define TC_EYES_ROW(face) \
    PX14(TC_T), PX2(TC_D), PX8(face), PX4(TC_D), PX8(face), PX4(TC_D), \
        PX8(face), PX2(TC_D), PX14(TC_T)
#define TC_HALF_BLINK_ROW(face) \
    PX14(TC_T), PX2(TC_D), PX8(face), PX4(TC_D), PX8(face), PX4(TC_D), \
        PX8(face), PX2(TC_D), PX14(TC_T)
#define TC_CLOSED_BLINK_ROW(face) \
    PX14(TC_T), PX2(TC_D), PX8(face), PX16(TC_D), PX8(face), PX2(TC_D), \
        PX14(TC_T)
#define TC_MOUTH_ROW(face) \
    PX14(TC_T), PX2(TC_D), PX12(face), PX8(TC_D), PX12(face), PX2(TC_D), \
        PX14(TC_T)
#define TC_HAPPY_MOUTH_ROW(face) \
    PX14(TC_T), PX2(TC_D), PX8(face), PX16(TC_D), PX8(face), PX2(TC_D), \
        PX14(TC_T)

#define TC_IDLE_ROWS(face) \
    TC_TRANSPARENT_ROW, TC_TRANSPARENT_ROW, TC_TRANSPARENT_ROW, TC_TRANSPARENT_ROW, \
    TC_TRANSPARENT_ROW, TC_TRANSPARENT_ROW, TC_TRANSPARENT_ROW, TC_TRANSPARENT_ROW, \
    TC_TRANSPARENT_ROW, TC_TRANSPARENT_ROW, TC_TRANSPARENT_ROW, TC_TRANSPARENT_ROW, \
    TC_TRANSPARENT_ROW, TC_TRANSPARENT_ROW, \
    TC_OUTLINE_ROW, TC_OUTLINE_ROW, \
    TC_BODY_ROW(face), TC_BODY_ROW(face), TC_BODY_ROW(face), TC_BODY_ROW(face), \
    TC_BODY_ROW(face), TC_BODY_ROW(face), TC_BODY_ROW(face), TC_BODY_ROW(face), \
    TC_BODY_ROW(face), TC_BODY_ROW(face), \
    TC_EYES_ROW(face), TC_EYES_ROW(face), TC_EYES_ROW(face), TC_EYES_ROW(face), \
    TC_EYES_ROW(face), TC_EYES_ROW(face), \
    TC_BODY_ROW(face), TC_BODY_ROW(face), TC_BODY_ROW(face), TC_BODY_ROW(face), \
    TC_BODY_ROW(face), TC_BODY_ROW(face), \
    TC_MOUTH_ROW(face), TC_MOUTH_ROW(face), TC_MOUTH_ROW(face), TC_MOUTH_ROW(face), \
    TC_BODY_ROW(face), TC_BODY_ROW(face), TC_BODY_ROW(face), TC_BODY_ROW(face), \
    TC_BODY_ROW(face), TC_BODY_ROW(face), \
    TC_OUTLINE_ROW, TC_OUTLINE_ROW, \
    TC_TRANSPARENT_ROW, TC_TRANSPARENT_ROW, TC_TRANSPARENT_ROW, TC_TRANSPARENT_ROW, \
    TC_TRANSPARENT_ROW, TC_TRANSPARENT_ROW, TC_TRANSPARENT_ROW, TC_TRANSPARENT_ROW, \
    TC_TRANSPARENT_ROW, TC_TRANSPARENT_ROW, TC_TRANSPARENT_ROW, TC_TRANSPARENT_ROW, \
    TC_TRANSPARENT_ROW, TC_TRANSPARENT_ROW

#define TC_BLINK_ROWS(face, eye_row) \
    TC_TRANSPARENT_ROW, TC_TRANSPARENT_ROW, TC_TRANSPARENT_ROW, TC_TRANSPARENT_ROW, \
    TC_TRANSPARENT_ROW, TC_TRANSPARENT_ROW, TC_TRANSPARENT_ROW, TC_TRANSPARENT_ROW, \
    TC_TRANSPARENT_ROW, TC_TRANSPARENT_ROW, TC_TRANSPARENT_ROW, TC_TRANSPARENT_ROW, \
    TC_TRANSPARENT_ROW, TC_TRANSPARENT_ROW, \
    TC_OUTLINE_ROW, TC_OUTLINE_ROW, \
    TC_BODY_ROW(face), TC_BODY_ROW(face), TC_BODY_ROW(face), TC_BODY_ROW(face), \
    TC_BODY_ROW(face), TC_BODY_ROW(face), TC_BODY_ROW(face), TC_BODY_ROW(face), \
    TC_BODY_ROW(face), TC_BODY_ROW(face), \
    eye_row(face), eye_row(face), eye_row(face), eye_row(face), eye_row(face), \
    eye_row(face), \
    TC_BODY_ROW(face), TC_BODY_ROW(face), TC_BODY_ROW(face), TC_BODY_ROW(face), \
    TC_BODY_ROW(face), TC_BODY_ROW(face), \
    TC_MOUTH_ROW(face), TC_MOUTH_ROW(face), TC_MOUTH_ROW(face), TC_MOUTH_ROW(face), \
    TC_BODY_ROW(face), TC_BODY_ROW(face), TC_BODY_ROW(face), TC_BODY_ROW(face), \
    TC_BODY_ROW(face), TC_BODY_ROW(face), \
    TC_OUTLINE_ROW, TC_OUTLINE_ROW, \
    TC_TRANSPARENT_ROW, TC_TRANSPARENT_ROW, TC_TRANSPARENT_ROW, TC_TRANSPARENT_ROW, \
    TC_TRANSPARENT_ROW, TC_TRANSPARENT_ROW, TC_TRANSPARENT_ROW, TC_TRANSPARENT_ROW, \
    TC_TRANSPARENT_ROW, TC_TRANSPARENT_ROW, TC_TRANSPARENT_ROW, TC_TRANSPARENT_ROW, \
    TC_TRANSPARENT_ROW, TC_TRANSPARENT_ROW

#define TC_HAPPY_ROWS(face) \
    TC_TRANSPARENT_ROW, TC_TRANSPARENT_ROW, TC_TRANSPARENT_ROW, TC_TRANSPARENT_ROW, \
    TC_TRANSPARENT_ROW, TC_TRANSPARENT_ROW, TC_TRANSPARENT_ROW, TC_TRANSPARENT_ROW, \
    TC_TRANSPARENT_ROW, TC_TRANSPARENT_ROW, TC_TRANSPARENT_ROW, TC_TRANSPARENT_ROW, \
    TC_TRANSPARENT_ROW, TC_TRANSPARENT_ROW, \
    TC_OUTLINE_ROW, TC_OUTLINE_ROW, \
    TC_BODY_ROW(face), TC_BODY_ROW(face), TC_BODY_ROW(face), TC_BODY_ROW(face), \
    TC_BODY_ROW(face), TC_BODY_ROW(face), TC_BODY_ROW(face), TC_BODY_ROW(face), \
    TC_BODY_ROW(face), TC_BODY_ROW(face), \
    TC_EYES_ROW(face), TC_EYES_ROW(face), TC_EYES_ROW(face), TC_EYES_ROW(face), \
    TC_EYES_ROW(face), TC_EYES_ROW(face), \
    TC_BODY_ROW(face), TC_BODY_ROW(face), TC_BODY_ROW(face), TC_BODY_ROW(face), \
    TC_BODY_ROW(face), TC_BODY_ROW(face), \
    TC_HAPPY_MOUTH_ROW(face), TC_HAPPY_MOUTH_ROW(face), TC_HAPPY_MOUTH_ROW(face), \
    TC_HAPPY_MOUTH_ROW(face), \
    TC_BODY_ROW(face), TC_BODY_ROW(face), TC_BODY_ROW(face), TC_BODY_ROW(face), \
    TC_BODY_ROW(face), TC_BODY_ROW(face), \
    TC_OUTLINE_ROW, TC_OUTLINE_ROW, \
    TC_TRANSPARENT_ROW, TC_TRANSPARENT_ROW, TC_TRANSPARENT_ROW, TC_TRANSPARENT_ROW, \
    TC_TRANSPARENT_ROW, TC_TRANSPARENT_ROW, TC_TRANSPARENT_ROW, TC_TRANSPARENT_ROW, \
    TC_TRANSPARENT_ROW, TC_TRANSPARENT_ROW, TC_TRANSPARENT_ROW, TC_TRANSPARENT_ROW, \
    TC_TRANSPARENT_ROW, TC_TRANSPARENT_ROW

static const uint16_t test_character_idle_0_pixels[64U * 64U] = {
    TC_IDLE_ROWS(TEST_CHARACTER_FACE_0_COLOR),
};

static const uint16_t test_character_idle_1_pixels[64U * 64U] = {
    TC_IDLE_ROWS(TEST_CHARACTER_FACE_1_COLOR),
};

static const uint16_t test_character_blink_half_pixels[64U * 64U] = {
    TC_BLINK_ROWS(TEST_CHARACTER_FACE_0_COLOR, TC_HALF_BLINK_ROW),
};

static const uint16_t test_character_blink_closed_pixels[64U * 64U] = {
    TC_BLINK_ROWS(TEST_CHARACTER_FACE_0_COLOR, TC_CLOSED_BLINK_ROW),
};

static const uint16_t test_character_happy_0_pixels[64U * 64U] = {
    TC_HAPPY_ROWS(TEST_CHARACTER_FACE_0_COLOR),
};

static const uint16_t test_character_happy_1_pixels[64U * 64U] = {
    TC_HAPPY_ROWS(TEST_CHARACTER_HAPPY_COLOR),
};

#undef TC_HAPPY_ROWS
#undef TC_BLINK_ROWS
#undef TC_IDLE_ROWS
#undef TC_HAPPY_MOUTH_ROW
#undef TC_MOUTH_ROW
#undef TC_CLOSED_BLINK_ROW
#undef TC_HALF_BLINK_ROW
#undef TC_EYES_ROW
#undef TC_BODY_ROW
#undef TC_OUTLINE_ROW
#undef TC_TRANSPARENT_ROW
#undef TC_D
#undef TC_T
#undef PX64
#undef PX36
#undef PX32
#undef PX16
#undef PX14
#undef PX12
#undef PX8
#undef PX4
#undef PX2

/* Palette-indexed source keeps the compiled resources small and readable. The
 * table is const so bitmap metadata can move to flash-backed assets without
 * changing the asset provider interface. */
static const compiled_asset_entry_t compiled_assets[] = {
    {
        PET_ASSET_BOOT_0,
        "......"
        ".BBBB."
        "BDBBDB"
        "BBBBBB"
        ".BBBB."
        "......",
        NULL,
        PET_COMPILED_ASSET_WIDTH,
        PET_COMPILED_ASSET_HEIGHT,
        PET_COMPILED_ASSET_WIDTH,
        false,
        0U,
    },
    {
        PET_ASSET_BOOT_1,
        "..FF.."
        ".FFFF."
        "FDFFDF"
        "FFFFFF"
        ".FDDF."
        "......",
        NULL,
        PET_COMPILED_ASSET_WIDTH,
        PET_COMPILED_ASSET_HEIGHT,
        PET_COMPILED_ASSET_WIDTH,
        false,
        0U,
    },
    {
        PET_ASSET_IDLE,
        "..FF.."
        ".FFFF."
        "FDFFDF"
        "FFFFFF"
        ".FFFF."
        "..DD..",
        NULL,
        PET_COMPILED_ASSET_WIDTH,
        PET_COMPILED_ASSET_HEIGHT,
        PET_COMPILED_ASSET_WIDTH,
        false,
        0U,
    },
    {
        PET_ASSET_BLINK_HALF,
        "..FF.."
        ".FFFF."
        "FDDDDF"
        "FFFFFF"
        ".FFFF."
        "..DD..",
        NULL,
        PET_COMPILED_ASSET_WIDTH,
        PET_COMPILED_ASSET_HEIGHT,
        PET_COMPILED_ASSET_WIDTH,
        false,
        0U,
    },
    {
        PET_ASSET_BLINK_CLOSED,
        "..FF.."
        ".FFFF."
        "FFFFFF"
        "FDDDDF"
        ".FFFF."
        "..DD..",
        NULL,
        PET_COMPILED_ASSET_WIDTH,
        PET_COMPILED_ASSET_HEIGHT,
        PET_COMPILED_ASSET_WIDTH,
        false,
        0U,
    },
    {
        PET_ASSET_LOOK_LEFT,
        "..FF.."
        ".FFFF."
        "DFFDFF"
        "FFFFFF"
        ".FFFF."
        "..DD..",
        NULL,
        PET_COMPILED_ASSET_WIDTH,
        PET_COMPILED_ASSET_HEIGHT,
        PET_COMPILED_ASSET_WIDTH,
        false,
        0U,
    },
    {
        PET_ASSET_LOOK_RIGHT,
        "..FF.."
        ".FFFF."
        "FFDFFD"
        "FFFFFF"
        ".FFFF."
        "..DD..",
        NULL,
        PET_COMPILED_ASSET_WIDTH,
        PET_COMPILED_ASSET_HEIGHT,
        PET_COMPILED_ASSET_WIDTH,
        false,
        0U,
    },
    {
        PET_ASSET_HAPPY_0,
        "..FF.."
        ".FFFF."
        "FDFFDF"
        "AFFFFA"
        ".FDDF."
        "......",
        NULL,
        PET_COMPILED_ASSET_WIDTH,
        PET_COMPILED_ASSET_HEIGHT,
        PET_COMPILED_ASSET_WIDTH,
        false,
        0U,
    },
    {
        PET_ASSET_HAPPY_1,
        "..LL.."
        ".LLLL."
        "LDLLDL"
        "ALLLLA"
        ".LDDL."
        "......",
        NULL,
        PET_COMPILED_ASSET_WIDTH,
        PET_COMPILED_ASSET_HEIGHT,
        PET_COMPILED_ASSET_WIDTH,
        false,
        0U,
    },
    {
        PET_ASSET_SLEEP_0,
        "..SS.."
        ".SSSS."
        "SssssS"
        "SSSSSS"
        ".SssS."
        "......",
        NULL,
        PET_COMPILED_ASSET_WIDTH,
        PET_COMPILED_ASSET_HEIGHT,
        PET_COMPILED_ASSET_WIDTH,
        false,
        0U,
    },
    {
        PET_ASSET_SLEEP_1,
        "......"
        "..SS.."
        ".SssS."
        ".SSSS."
        "..ss.."
        "......",
        NULL,
        PET_COMPILED_ASSET_WIDTH,
        PET_COMPILED_ASSET_HEIGHT,
        PET_COMPILED_ASSET_WIDTH,
        false,
        0U,
    },
    {
        PET_ASSET_IDLE_1,
        "..LL.."
        ".LLLL."
        "LDLLDL"
        "LLLLLL"
        ".LLLL."
        "..DD..",
        NULL,
        PET_COMPILED_ASSET_WIDTH,
        PET_COMPILED_ASSET_HEIGHT,
        PET_COMPILED_ASSET_WIDTH,
        false,
        0U,
    },
    {
        PET_ASSET_TEST_CHARACTER,
        NULL,
        test_character_idle_0_pixels,
        64U,
        64U,
        64U,
        true,
        TEST_CHARACTER_TRANSPARENT_COLOR,
    },
    {
        PET_ASSET_TEST_CHARACTER_IDLE_1,
        NULL,
        test_character_idle_1_pixels,
        64U,
        64U,
        64U,
        true,
        TEST_CHARACTER_TRANSPARENT_COLOR,
    },
    {
        PET_ASSET_TEST_CHARACTER_BLINK_HALF,
        NULL,
        test_character_blink_half_pixels,
        64U,
        64U,
        64U,
        true,
        TEST_CHARACTER_TRANSPARENT_COLOR,
    },
    {
        PET_ASSET_TEST_CHARACTER_BLINK_CLOSED,
        NULL,
        test_character_blink_closed_pixels,
        64U,
        64U,
        64U,
        true,
        TEST_CHARACTER_TRANSPARENT_COLOR,
    },
    {
        PET_ASSET_TEST_CHARACTER_HAPPY_0,
        NULL,
        test_character_happy_0_pixels,
        64U,
        64U,
        64U,
        true,
        TEST_CHARACTER_TRANSPARENT_COLOR,
    },
    {
        PET_ASSET_TEST_CHARACTER_HAPPY_1,
        NULL,
        test_character_happy_1_pixels,
        64U,
        64U,
        64U,
        true,
        TEST_CHARACTER_TRANSPARENT_COLOR,
    },
};

static const compiled_asset_entry_t *find_compiled_asset(pet_asset_id_t asset_id)
{
    size_t index;
    for (index = 0U; index < sizeof(compiled_assets) / sizeof(compiled_assets[0]);
         index++) {
        if (compiled_assets[index].asset_id == asset_id) {
            return &compiled_assets[index];
        }
    }
    return NULL;
}

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
    const compiled_asset_entry_t *entry = find_compiled_asset(asset_id);
    if (provider == NULL || bitmap == NULL || entry == NULL) {
        return PET_STATUS_INVALID_ARGUMENT;
    }
    if (entry->pixels != NULL) {
        bitmap->pixels = entry->pixels;
    } else if (asset_id <= PET_ASSET_IDLE_1) {
        bitmap->pixels = provider->pixels[asset_id];
    } else {
        return PET_STATUS_INVALID_ARGUMENT;
    }
    bitmap->width = entry->width;
    bitmap->height = entry->height;
    bitmap->stride_pixels = entry->stride_pixels;
    bitmap->has_transparent_color = entry->has_transparent_color;
    bitmap->transparent_color = entry->transparent_color;
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
        const compiled_asset_entry_t *entry = find_compiled_asset(asset_id);
        if (entry == NULL || entry->pattern == NULL ||
            entry->width != PET_COMPILED_ASSET_WIDTH ||
            entry->height != PET_COMPILED_ASSET_HEIGHT ||
            entry->stride_pixels != PET_COMPILED_ASSET_WIDTH ||
            strlen(entry->pattern) != PET_COMPILED_ASSET_WIDTH *
                                          PET_COMPILED_ASSET_HEIGHT) {
            return PET_STATUS_INVALID_ARGUMENT;
        }
        for (pixel = 0U;
             pixel < PET_COMPILED_ASSET_WIDTH * PET_COMPILED_ASSET_HEIGHT;
             pixel++) {
            provider->pixels[asset_id][pixel] = palette_color(entry->pattern[pixel]);
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
