#ifndef PET_FILE_ASSET_PROVIDER_H
#define PET_FILE_ASSET_PROVIDER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "animation/pet_asset_provider.h"

#define PET_FILE_ASSET_CAPACITY 32U
#define PET_FILE_ASSET_PATH_SIZE 256U

typedef struct {
    pet_asset_id_t id;
    char path[PET_FILE_ASSET_PATH_SIZE];
    uint16_t *pixels;
    size_t width;
    size_t height;
    bool loaded;
} pet_file_asset_entry_t;

typedef struct {
    char root[PET_FILE_ASSET_PATH_SIZE];
    pet_file_asset_entry_t entries[PET_FILE_ASSET_CAPACITY];
    size_t entry_count;
    pet_asset_provider_t provider;
} pet_file_asset_provider_t;

pet_status_t pet_file_asset_provider_init(pet_file_asset_provider_t *provider,
                                          const char *root, const char *manifest_name);
/* Destroy a successfully initialized provider before initializing it again. */
void pet_file_asset_provider_destroy(pet_file_asset_provider_t *provider);
const pet_asset_provider_t *pet_file_asset_provider_interface(
    pet_file_asset_provider_t *provider);

#endif
