/******
* this cpp file defines and runs a runtime that test and allow us to know if a drive from the path provided matches the elements and criteria that the Driver for APPID would apply to 
* define a device as "HOT" or "hotswappable"
* this should suffice for testing on our env. if THIS proves to provide a positive result. we need to check deeper on MS driver function. 
* 
* 
*/
#include <iostream>
#include <windows.h>
#include <stdlib.h>


bool setConsoleColor( /*WORD*/unsigned short newColor);

bool setConsoleColorHa(const HANDLE OutputHandle, /*WORD*/unsigned short newColor);


/*WORD*/unsigned short getCurrentColor(void);



/*
* prints error message or code. 
*/
void PrintLastError();

/*
*
* this function does the testing based on a provided letter. or "Path if a path is NOT provided the application will crash."
*/
int RunHotPlugTest(const wchar_t driveLetter);


int main(int argc, char** argv)
{
    if (argv[1]) {
        int wchars_num = MultiByteToWideChar(CP_UTF8, 0, argv[1], -1, NULL, 0);
        wchar_t* wstr = new wchar_t[wchars_num];
        MultiByteToWideChar(CP_UTF8, 0, argv[1], -1, wstr, wchars_num);
        RunHotPlugTest(wstr[0]);
        delete[] wstr;

        std::cout << "Test finish!\n";
    }
    else {
        std::cout << "Parameter is Empty!\n";
    }

}


int RunHotPlugTest(const wchar_t driveLetter)
{
    wchar_t volumeName[8] = L"";
    swprintf_s(volumeName, L"\\\\.\\%c:", driveLetter);

    WCHAR szVolumeName[MAX_PATH];
    wchar_t volumePath[3] = L"";
    swprintf_s(volumePath, L"%c:", driveLetter);
    QueryDosDevice(volumePath, szVolumeName, MAX_PATH);
    wprintf(L"Drive identified by: %s\\\n", volumePath);
    wprintf(L"Testing on device: %s\n", szVolumeName);
    wprintf(L"\n");



    //Create File is a Option. BUT I would suggest to use NtOpenFile to do a 1:1 test but for simplicity we will use CreateFile.  
    //Reason begin we would like a 1:1 test and detect where the error might be for the affected case. 

    HANDLE volume = CreateFile(
        volumeName,
        SYNCHRONIZE,
        FILE_SHARE_READ | FILE_SHARE_DELETE | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING, 
        0,
        NULL
    );

    if (volume == INVALID_HANDLE_VALUE) {
        printf("we are unable to open the Device : %ls \n", volumeName);
        PrintLastError();// get last error. 
        return -1;
    }

    DWORD bytesReturned = 0;
    bool pass = false, pass2=false;
    STORAGE_HOTPLUG_INFO Info = { 0 };
    unsigned short normal = getCurrentColor();
    if (DeviceIoControl(volume, IOCTL_STORAGE_GET_HOTPLUG_INFO, 0, 0, &Info, sizeof(Info), &bytesReturned, NULL))
    {
        printf("Device Information for: %ls \n", volumeName);
        printf("Device Information contains \"%ld\" bytes \n", bytesReturned);
        printf("Device %ls has \"MediaRemovable\" : %s \n", volumeName, Info.MediaRemovable?"Yes":"no");
        Info.DeviceHotplug ? setConsoleColor(FOREGROUND_GREEN) : setConsoleColor(FOREGROUND_RED);
        printf("Device ** %ls has \"DeviceHotplug\" : %s \n", volumeName, Info.DeviceHotplug ? "Yes" : "no");
        Info.MediaHotplug ? setConsoleColor(FOREGROUND_GREEN) : setConsoleColor(FOREGROUND_RED);
        printf("Device ** %ls has \"MediaHotplug\" : %s \n", volumeName, Info.MediaHotplug ? "Yes" : "no");
        pass = Info.MediaHotplug || Info.DeviceHotplug;
        pass ? setConsoleColor(FOREGROUND_GREEN) : setConsoleColor(FOREGROUND_RED);
        printf("\tDevice first Stage result: %s \n", pass?"Sucess":"Fail");
        setConsoleColor(normal);
    }
    else {
        printf("fail to call DeviceIoControl, Fail at First Stage\n");
        PrintLastError();
    }
    setConsoleColor(normal);
    // second stage   IOCTL_STORAGE_QUERY_PROPERTY
    STORAGE_PROPERTY_QUERY spq = { StorageDeviceProperty, PropertyStandardQuery };
    STORAGE_DEVICE_DESCRIPTOR  StorDescriptor = { 0 };
    if (DeviceIoControl(volume, IOCTL_STORAGE_QUERY_PROPERTY, &spq, sizeof(STORAGE_PROPERTY_QUERY), &StorDescriptor,sizeof(STORAGE_DEVICE_DESCRIPTOR), &bytesReturned, NULL))
    {
        printf("Device Second Stage Information contains \"%ld\" bytes \n", bytesReturned);
        setConsoleColor(FOREGROUND_RED);
        switch (StorDescriptor.BusType)
        {
        case BusTypeScsi:
            printf("Device Second Stage Fail device detected as \"BusTypeScsi\", this flag remove Hot Plug Flag\n");
            pass2 = false;
            break;
        case BusTypeAta:
            printf("Device Second Stage Fail device detected as \"BusTypeScsi\", this flag remove Hot Plug Flag\n");
            pass2 = false;
            break;
        case BusTypeSata:
            printf("Device Second Stage Fail device detected as \"BusTypeScsi\", this flag remove Hot Plug Flag\n");
            pass2 = false;
            break;
        case BusTypeFibre:
            printf("Device Second Stage Fail device detected as \"BusTypeScsi\", this flag remove Hot Plug Flag\n");
            pass2 = false;
            break;
        case BusTypeRAID:
            printf("Device Second Stage Fail device detected as \"BusTypeScsi\", this flag remove Hot Plug Flag\n");
            pass2 = false;
            break;
        default:
            setConsoleColor(FOREGROUND_GREEN);
            printf("Device Pass the Second Stage match as HotPlug type as it does not match the \"blacklisted type's\"\n");
            pass2 = true;
            break;
        }
        pass2 ? setConsoleColor(FOREGROUND_GREEN) : setConsoleColor(FOREGROUND_RED);
        printf("\tDevice Second Stage result: %s \n", pass2 ? "Sucess" : "Fail");
    }
    else {
        printf("fail to call DeviceIoControl, Fail at Second test\n");
        PrintLastError();
    }
    setConsoleColor(normal);
    printf("\n\n");
    pass2&&pass ? setConsoleColor(FOREGROUND_GREEN) : setConsoleColor(FOREGROUND_RED);
    printf("\tConclusion: the Device : %ls is: %s  \n", volumeName, (pass2 && pass ? "Hotplug" : "Not Hotplug"));
    setConsoleColor(normal);

    CloseHandle(volume);
    return 0;
}

void PrintLastError() {
    DWORD dwError = GetLastError();
    wchar_t String[1000];
    if (!FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, dwError, 0, String, 1000, NULL))
    {
        printf("Format message failed with 0x%ld\n", GetLastError());//%x (print any kind of data. )
    }

    printf("Error Detected Code: %ld, Error Message: %ls \n", dwError, String);
    printf("ReadString Operation failed\n\n");
}


/**
* console color output stuff..... 
* 
**/



/*WORD*/unsigned short currColor = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;//--> most likely  
const /*WORD*/unsigned short MAXCOLOR = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY
| BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED | BACKGROUND_INTENSITY;//--> likely 0xff or 255 
const /*WORD*/unsigned short MINCOLOR = 0;  // most likely?   

bool setConsoleColor(/*WORD*/unsigned short newColor) {
    if (newColor >= MINCOLOR || newColor <= MAXCOLOR) {
        return SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), (WORD)newColor);
    }
    else {
        //if suceed returns a NONCERO therefore return a 0 
        printf("invalid Color");
        return false;
    }
}

bool setConsoleColorHa(const HANDLE OutputHandle, /*WORD*/unsigned short newColor) {
    if (newColor >= MINCOLOR || newColor <= MAXCOLOR) {
        return SetConsoleTextAttribute(OutputHandle, (WORD)newColor);
    }
    else {
        //if suceed returns a NONCERO therefore return a 0 
        printf("invalid Color");
        return false;
    }
}

/*WORD*/unsigned short getCurrentColor(void) {
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (!GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info))
        return currColor;//hmmm not sure maybe should be better to return bool? and use a pointer? dunno 
    return info.wAttributes;
}

