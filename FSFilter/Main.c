#include "FsFilter.h"

//////////////////////////////////////////////////////////////////////////
// Function prototypes

VOID FsFilterUnload(
    __in PDRIVER_OBJECT DriverObject
    );

//////////////////////////////////////////////////////////////////////////
// Global data

PDRIVER_OBJECT   g_fsFilterDriverObject = NULL;

FAST_IO_DISPATCH g_fastIoDispatch =
{
    sizeof(FAST_IO_DISPATCH),
    FsFilterFastIoCheckIfPossible,
    FsFilterFastIoRead,
    FsFilterFastIoWrite,
    FsFilterFastIoQueryBasicInfo,
    FsFilterFastIoQueryStandardInfo,
    FsFilterFastIoLock,
    FsFilterFastIoUnlockSingle,
    FsFilterFastIoUnlockAll,
    FsFilterFastIoUnlockAllByKey,
    FsFilterFastIoDeviceControl,
    NULL,
    NULL,
    FsFilterFastIoDetachDevice,
    FsFilterFastIoQueryNetworkOpenInfo,
    NULL,
    FsFilterFastIoMdlRead,
    FsFilterFastIoMdlReadComplete,
    FsFilterFastIoPrepareMdlWrite,
    FsFilterFastIoMdlWriteComplete,
    FsFilterFastIoReadCompressed,
    FsFilterFastIoWriteCompressed,
    FsFilterFastIoMdlReadCompleteCompressed,
    FsFilterFastIoMdlWriteCompleteCompressed,
    FsFilterFastIoQueryOpen,
    NULL,
    NULL,
    NULL,
};

//////////////////////////////////////////////////////////////////////////
// DriverEntry - Entry point of the driver

NTSTATUS DriverEntry(
    __inout PDRIVER_OBJECT  DriverObject,
    __in    PUNICODE_STRING RegistryPath
    )
{    
    UNREFERENCED_PARAMETER(RegistryPath);
    NTSTATUS status = STATUS_SUCCESS;
    ULONG    i      = 0;


    g_fsFilterDriverObject = DriverObject;

    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; ++i) 
    {
        DriverObject->MajorFunction[i] = FsFilterDispatchPassThrough;
    }

    DriverObject->MajorFunction[IRP_MJ_CREATE] = FsFilterDispatchCreate;

    DriverObject->FastIoDispatch = &g_fastIoDispatch;

    status = IoRegisterFsRegistrationChange(DriverObject,
        FsFilterNotificationCallback); 
    if (!NT_SUCCESS(status)) 
    {
        return status;
    }

    DriverObject->DriverUnload = FsFilterUnload;

    return STATUS_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////
// Unload routine

VOID FsFilterUnload(
    __in PDRIVER_OBJECT DriverObject
    )
{
    ULONG           numDevices = 0;
    ULONG           i          = 0;    
    LARGE_INTEGER   interval;
    PDEVICE_OBJECT  devList[DEVOBJ_LIST_SIZE];

    interval.QuadPart = (5 * DELAY_ONE_SECOND); //delay 5 seconds

    //
    //  Unregistered callback routine for file system changes.
    //

    IoUnregisterFsRegistrationChange(DriverObject, FsFilterNotificationCallback);

    //
    //  This is the loop that will go through all of the devices we are attached
    //  to and detach from them.
    //

    for (;;)
    {
        IoEnumerateDeviceObjectList(
            DriverObject,
            devList,
            sizeof(devList),
            &numDevices);

        if (0 == numDevices)
        {
            break;
        }

        numDevices = min(numDevices, RTL_NUMBER_OF(devList));

        for (i = 0; i < numDevices; ++i) 
        {
            FsFilterDetachFromDevice(devList[i]);
            ObDereferenceObject(devList[i]);
        }
        
        KeDelayExecutionThread(KernelMode, FALSE, &interval);
    }
}

//////////////////////////////////////////////////////////////////////////
// Misc

BOOLEAN FsFilterIsMyDeviceObject(
    __in PDEVICE_OBJECT DeviceObject
    )
{
    return DeviceObject->DriverObject == g_fsFilterDriverObject;
}
