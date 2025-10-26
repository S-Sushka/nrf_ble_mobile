#include "nvs_part.h"

static struct nvs_fs nvs_file_system;

void __debug__print_uuid(bt_uuid_128 *uuid) 
{
    for (int i = 0; i < 16; i++)
        printk("%x", uuid->val[i]);
    printk("\n");
}

int nvs_begin() 
{
    int err= 0;
    struct flash_pages_info info;

    nvs_file_system.flash_device = FIXED_PARTITION_DEVICE(STORAGE_PARTITION);
    nvs_file_system.offset = FIXED_PARTITION_OFFSET(STORAGE_PARTITION);

    err = flash_get_page_info_by_offs(nvs_file_system.flash_device, nvs_file_system.offset, &info);
    if (err < 0) 
    {
        printk(" --- NVS ERR --- :  Get page info Failed: %d\n", err);
        return err;
    }

    nvs_file_system.sector_size = info.size;
    nvs_file_system.sector_count = 4;

    err = nvs_mount(&nvs_file_system);
    if (err < 0) 
    {
        printk(" --- NVS ERR --- :  Mount Failed: %d\n", err);
        return err;
    }

    printk(" --- NVS --- :  Successful Initialized!\n");
    return 0;
}



uint8_t get_uuids_count() 
{
    int err = 0;
    uint8_t uuids_count_buffer;

    err = nvs_read(&nvs_file_system, 0, &uuids_count_buffer, 1);
    if (err == -ENOENT)
    {
        uuids_count_buffer = 0;
        nvs_write(&nvs_file_system, 0, &uuids_count_buffer, 1);
    }
    else if (err < 0)
    {
        printk(" --- NVS ERR --- :  Count of UUIDs Failed: %d\n", err);
        return 0;
    }

    return uuids_count_buffer;
}


int check_id(bt_uuid_128 *uuid) 
{
    struct bt_uuid_128 uuid_buffer;
    uint8_t uuids_count_buffer = get_uuids_count();

    for (int i = 0; i < uuids_count_buffer; i++) 
    {
        get_uuid(i, &uuid_buffer);

        if (bt_uuid_cmp(&uuid_buffer.uuid, &uuid->uuid) == 0)
            return -EEXIST;
    }

    return 0;
}

int add_uuid(bt_uuid_128 *uuid) 
{
    int err = 0;
    uint8_t uuids_count_buffer = get_uuids_count();

    err = check_id(uuid); 
    if (err < 0)
    {
        if (err == -EEXIST)
            printk(" --- NVS ERR --- :  UUID already exsist\n");
        else 
            printk(" --- NVS ERR --- :  Check UUID Failed: %d\n", err);

        return err;
    }

    uuids_count_buffer++;
    err = nvs_write(&nvs_file_system, uuids_count_buffer, uuid, sizeof(bt_uuid_128));
    if (err < 0)
    {
        printk(" --- NVS ERR --- :  Add UUID Failed: %d\n", err);
        return err;
    }

    err = nvs_write(&nvs_file_system, 0, &uuids_count_buffer, 1);
    if (err < 0)
    {
        printk(" --- NVS ERR --- :  Increase UUID count Failed: %d\n", err);
        return err;
    }    

    return 0;
}

int get_uuid(uint8_t index, bt_uuid_128 *uuid) 
{
    int err = 0;
    uint8_t uuids_count_buffer = get_uuids_count();

    if (uuids_count_buffer == 0 || index >= uuids_count_buffer)
    {
        printk(" --- NVS ERR --- :  No such UUIDs\n");
        return -EINVAL;
    }
    
    err = nvs_read(&nvs_file_system, index+1, uuid, sizeof(bt_uuid_128));
    if (err < 0)
    {
        printk(" --- NVS ERR --- :  Get of UUID Failed: %d\n", err);
        return err;
    }
    
    return 0;
}

int delete_last_uuid() 
{
    uint8_t uuids_count_buffer = get_uuids_count();

    if (uuids_count_buffer == 0)
    {
        printk(" --- NVS ERR --- :  Cannot delete UUID\n");
        return -ENOENT;
    }

    uuids_count_buffer--;
    nvs_write(&nvs_file_system, 0, &uuids_count_buffer, 1);

    return 0;
}