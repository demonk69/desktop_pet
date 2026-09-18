#include "file_asset_provider.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool join_path(char *destination, size_t size, const char *root, const char *path)
{
    int written = snprintf(destination, size, "%s/%s", root, path);
    return written > 0 && (size_t)written < size;
}

static bool read_token(FILE *file, char *token, size_t size)
{
    int character;
    size_t length = 0U;

    do {
        character = fgetc(file);
        if (character == '#') {
            do {
                character = fgetc(file);
            } while (character != '\n' && character != EOF);
        }
    } while (character != EOF && isspace((unsigned char)character));
    if (character == EOF) {
        return false;
    }
    do {
        if (length + 1U >= size) {
            return false;
        }
        token[length++] = (char)character;
        character = fgetc(file);
    } while (character != EOF && !isspace((unsigned char)character));
    token[length] = '\0';
    return true;
}

static bool read_number(FILE *file, unsigned long *number)
{
    char token[32];
    char *end;
    if (!read_token(file, token, sizeof(token))) {
        return false;
    }
    *number = strtoul(token, &end, 10);
    return end != token && *end == '\0';
}

static pet_status_t load_ppm(pet_file_asset_provider_t *provider,
                             pet_file_asset_entry_t *entry)
{
    char full_path[PET_FILE_ASSET_PATH_SIZE * 2U];
    char magic[8];
    unsigned long width;
    unsigned long height;
    unsigned long maximum;
    size_t count;
    size_t index;
    FILE *file;

    if (!join_path(full_path, sizeof(full_path), provider->root, entry->path)) {
        return PET_STATUS_INVALID_ARGUMENT;
    }
    file = fopen(full_path, "rb");
    if (file == NULL) {
        return PET_STATUS_IO_ERROR;
    }
    if (!read_token(file, magic, sizeof(magic)) || strcmp(magic, "P3") != 0 ||
        !read_number(file, &width) || !read_number(file, &height) ||
        !read_number(file, &maximum) || width == 0UL || height == 0UL ||
        width > 1024UL || height > 1024UL || maximum == 0UL || maximum > 65535UL) {
        (void)fclose(file);
        return PET_STATUS_IO_ERROR;
    }
    count = (size_t)width * (size_t)height;
    entry->pixels = malloc(count * sizeof(*entry->pixels));
    if (entry->pixels == NULL) {
        (void)fclose(file);
        return PET_STATUS_NO_MEMORY;
    }
    for (index = 0U; index < count; index++) {
        unsigned long red;
        unsigned long green;
        unsigned long blue;
        if (!read_number(file, &red) || !read_number(file, &green) ||
            !read_number(file, &blue) || red > maximum || green > maximum ||
            blue > maximum) {
            free(entry->pixels);
            entry->pixels = NULL;
            (void)fclose(file);
            return PET_STATUS_IO_ERROR;
        }
        red = red * 31UL / maximum;
        green = green * 63UL / maximum;
        blue = blue * 31UL / maximum;
        entry->pixels[index] = (uint16_t)((red << 11U) | (green << 5U) | blue);
    }
    (void)fclose(file);
    entry->width = (size_t)width;
    entry->height = (size_t)height;
    entry->loaded = true;
    return PET_STATUS_OK;
}

static pet_status_t get_bitmap(void *context, pet_asset_id_t asset_id,
                               pet_bitmap_t *bitmap)
{
    pet_file_asset_provider_t *provider = context;
    size_t index;
    if (provider == NULL || bitmap == NULL) {
        return PET_STATUS_INVALID_ARGUMENT;
    }
    for (index = 0U; index < provider->entry_count; index++) {
        pet_file_asset_entry_t *entry = &provider->entries[index];
        if (entry->id == asset_id) {
            pet_status_t status = entry->loaded ? PET_STATUS_OK : load_ppm(provider, entry);
            if (status != PET_STATUS_OK) {
                return status;
            }
            *bitmap = (pet_bitmap_t){ entry->pixels, entry->width, entry->height,
                                      entry->width };
            return PET_STATUS_OK;
        }
    }
    return PET_STATUS_NOT_SUPPORTED;
}

static void release_bitmap(void *context, const pet_bitmap_t *bitmap)
{
    (void)context;
    (void)bitmap;
    /* File assets stay cached until provider destruction. */
}

pet_status_t pet_file_asset_provider_init(pet_file_asset_provider_t *provider,
                                          const char *root, const char *manifest_name)
{
    char manifest_path[PET_FILE_ASSET_PATH_SIZE * 2U];
    char line[PET_FILE_ASSET_PATH_SIZE + 32U];
    FILE *manifest;
    int root_length;

    if (provider == NULL || root == NULL || manifest_name == NULL) {
        return PET_STATUS_INVALID_ARGUMENT;
    }
    memset(provider, 0, sizeof(*provider));
    root_length = snprintf(provider->root, sizeof(provider->root), "%s", root);
    if (root_length <= 0 || (size_t)root_length >= sizeof(provider->root) ||
        !join_path(manifest_path, sizeof(manifest_path), root, manifest_name)) {
        return PET_STATUS_INVALID_ARGUMENT;
    }
    manifest = fopen(manifest_path, "r");
    if (manifest == NULL) {
        return PET_STATUS_IO_ERROR;
    }
    while (fgets(line, sizeof(line), manifest) != NULL) {
        unsigned int id;
        char path[PET_FILE_ASSET_PATH_SIZE];
        pet_file_asset_entry_t *entry;
        if (line[0] == '#' || isspace((unsigned char)line[0])) {
            continue;
        }
        if (sscanf(line, "%u %255s", &id, path) != 2 || id == 0U ||
            provider->entry_count >= PET_FILE_ASSET_CAPACITY) {
            (void)fclose(manifest);
            pet_file_asset_provider_destroy(provider);
            return PET_STATUS_IO_ERROR;
        }
        entry = &provider->entries[provider->entry_count++];
        entry->id = (pet_asset_id_t)id;
        (void)snprintf(entry->path, sizeof(entry->path), "%s", path);
    }
    (void)fclose(manifest);
    if (provider->entry_count == 0U) {
        return PET_STATUS_IO_ERROR;
    }
    provider->provider.context = provider;
    provider->provider.get_bitmap = get_bitmap;
    provider->provider.release_bitmap = release_bitmap;
    return PET_STATUS_OK;
}

void pet_file_asset_provider_destroy(pet_file_asset_provider_t *provider)
{
    size_t index;
    if (provider == NULL) {
        return;
    }
    for (index = 0U; index < provider->entry_count; index++) {
        free(provider->entries[index].pixels);
        provider->entries[index].pixels = NULL;
        provider->entries[index].loaded = false;
    }
    provider->entry_count = 0U;
}

const pet_asset_provider_t *pet_file_asset_provider_interface(
    pet_file_asset_provider_t *provider)
{
    return provider == NULL ? NULL : &provider->provider;
}
