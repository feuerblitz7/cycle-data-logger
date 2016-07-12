#include <cstdio>
#include "phidget21.h"

#define MAX_FILE_LENGTH (100000)


typedef struct
{
    int serial_no;
    CPhidgetSpatialHandle handle;
    FILE* file;
    int file_count;
    size_t file_length;

    CPhidgetHandle getHandle() const {return reinterpret_cast<CPhidgetHandle>(handle);}

    void createNewFile() {
                            char filename[100+1];
                            snprintf(filename, 100, "spatial_%d_%d.log", serial_no, ++file_count);
                            if (file)
                            {
                                fclose(file);
                            }
                            file_length = 0;
                            file = fopen(filename, "wb");
                         }
} SpatialInfo;


int CCONV SpatialDataHandler(CPhidgetSpatialHandle handle, void* userptr, CPhidgetSpatial_SpatialEventDataHandle* data, int count)
{
    SpatialInfo* spatial_info = reinterpret_cast<SpatialInfo*>(userptr);
    if (handle != spatial_info->handle)
        return EPHIDGET_UNEXPECTED;
   
    int axis_count;
    CPhidgetSpatial_getAccelerationAxisCount(spatial_info->handle, &axis_count);

    int i, j;
    for (i=0; i<count; i++)
    {
        spatial_info->file_length += fprintf(spatial_info->file, "%4d.%06d", data[i]->timestamp.seconds, data[i]->timestamp.microseconds);
        for (j=0; j<axis_count; j++)
        {
            spatial_info->file_length += fprintf(spatial_info->file, ";%0.3f", data[i]->acceleration[j]);
        }
        spatial_info->file_length += fprintf(spatial_info->file, "\n");

        if (MAX_FILE_LENGTH < spatial_info->file_length)
        {
            spatial_info->createNewFile();
        }
    }
    return EPHIDGET_OK;
}

bool RegisterSpatial(SpatialInfo& spatial_info, int serial_no)
{
    const char* err;
    int result;
    spatial_info.serial_no = serial_no;
    spatial_info.handle = NULL;
    if (EPHIDGET_OK!=(result=CPhidgetSpatial_create(&spatial_info.handle)))
    {
        CPhidget_getErrorDescription(result, &err);
        fprintf(stderr, "Failed to create spatial with serial number %d: %s\n", serial_no, err);
        return false;
    }

    spatial_info.file = NULL;
    spatial_info.file_count = 0;
    spatial_info.file_length = 0;
    spatial_info.createNewFile();

    if (EPHIDGET_OK!=(result=CPhidgetSpatial_set_OnSpatialData_Handler(spatial_info.handle, SpatialDataHandler, reinterpret_cast<void*>(&spatial_info))))
    {
        CPhidget_getErrorDescription(result, &err);
        fprintf(stderr, "Failed to set data handler for spatial with serial number %d: %s\n", serial_no, err);
        return false;
    }
    if (EPHIDGET_OK!=(result=CPhidget_open(spatial_info.getHandle(), serial_no)))
    {
        CPhidget_getErrorDescription(result, &err);
        fprintf(stderr, "Failed to open spatial with serial number %d: %s\n", serial_no, err);
        return false;
    }
    if (EPHIDGET_OK!=(result=CPhidget_waitForAttachment(spatial_info.getHandle(), 1000)))
    {
        CPhidget_getErrorDescription(result, &err);
        fprintf(stderr, "Failed to wait for attachment for spatial with serial number %d: %s\n", serial_no, err);
        return false;
    }
    if (EPHIDGET_OK!=(result=CPhidgetSpatial_setDataRate(spatial_info.handle, 8)))
    {
        CPhidget_getErrorDescription(result, &err);
        fprintf(stderr, "Failed to set data rate for spatial with serial number %d: %s\n", serial_no, err);
        return false;
    }
    return true;
}

void UnregisterSpatial(const SpatialInfo& spatial_info)
{
    CPhidget_close(spatial_info.getHandle());
    CPhidget_delete(spatial_info.getHandle());

    if (spatial_info.file)
        fclose(spatial_info.file);
}

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <PhidgetSpatial serial no> [<PhidgetSpatial serial no>]...\n\n", argv[0]);
    }
   
    SpatialInfo* spatials = new SpatialInfo[argc-1];
   
    int spatial_count = 0;
    int i;
    for (i=1; i<argc; i++)
    {
        int serial_no;
        if (0 < sscanf(argv[i], "%d", &serial_no))
        {
            if (RegisterSpatial(spatials[spatial_count], serial_no))
            {
                spatial_count++;
            }
        }
    }

    if (0 < spatial_count)
    {
        fprintf(stdout, "Press [Enter] to end");
        getchar();
    }
   
    for (i=0; i<spatial_count; i++)
    {
        UnregisterSpatial(spatials[i]);
    }
    delete[] spatials;

    return 0;
}
