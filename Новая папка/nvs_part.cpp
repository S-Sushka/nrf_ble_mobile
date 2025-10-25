#include "nvs_part.h"

struct nvs_fs nvs_file_system;

int nvs_begin() 
{
    int err = -1;
    struct flash_pages_info info;

    nvs_file_system.flash_device = FIXED_PARTITION_DEVICE(STORAGE_PARTITION);
    nvs_file_system.offset = FIXED_PARTITION_OFFSET(STORAGE_PARTITION);

    err = flash_get_page_info_by_offs(nvs_file_system.flash_device, nvs_file_system.offset, &info);
    if (err) return err; 

    nvs_file_system.sector_size = info.size;
    nvs_file_system.sector_count = 4;

    err = nvs_mount(&nvs_file_system);
    if (err) return err; 

    return 0;
}



uint8_t get_uuids_count() 
{
    uint8_t uuids_count_buffer;

    if (nvs_read(&nvs_file_system, 0, &uuids_count_buffer, 1) == ENOENT)
    {
        uuids_count_buffer = 0;
        nvs_write(&nvs_file_system, 0, &uuids_count_buffer, 1);
    }

    return uuids_count_buffer;
}


int check_id(bt_uuid_128 *uuid) 
{
    bt_uuid_128 uuid_buffer;
    uint8_t uuids_count_buffer = get_uuids_count();

    for (int i = 0; i < uuids_count_buffer; i++) 
    {
        get_uuid(i, &uuid_buffer);
        if (bt_uuid_cmp(&uuid_buffer.uuid, &uuid->uuid) == 0)
            return EEXIST;
    }

    return 0;
}

int get_uuid(uint8_t index, bt_uuid_128 *uuid) 
{
    if (index >= get_uuids_count())
        return EINVAL;
    
    nvs_read(&nvs_file_system, index+1, uuid, sizeof(bt_uuid_128));
    
    return 0;
}

int add_uuid(bt_uuid_128 *uuid) 
{
    uint8_t uuids_count_buffer = get_uuids_count();

    if (check_id(uuid))
        return EEXIST;

    uuids_count_buffer++;
    nvs_write(&nvs_file_system, uuids_count_buffer, uuid, sizeof(bt_uuid_128));
    nvs_write(&nvs_file_system, 0, &uuids_count_buffer, 1);

    return 0;
}

int delete_last_uuid() 
{
    uint8_t uuids_count_buffer = get_uuids_count();

    if (uuids_count_buffer == 0)
        return ENOENT;

    uuids_count_buffer--;
    nvs_write(&nvs_file_system, 0, &uuids_count_buffer, 1);

    return 0;
}