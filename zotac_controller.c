#include <stdio.h>
#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include "nvapi/nvapi.h"
#include "nvapi/nvapi_lite_salstart.h"
#include "nvapi/nvapi_lite_common.h"


#pragma comment(lib, "nvapi/x86/nvapi.lib")

int main(int argc, char **argv) {
    NvU8 brightness = 0;
    if (argc < 5) {
        printf("Usage: %s <brightness> <hex_color> <preset> <preset_speed>\n", argv[0]);
        printf("Example: %s 50 #FF5733 -1 0 or %s 50 FF5733 -1 0\n", argv[0], argv[0]);
        printf("Currently available presets:\n0: Rainbow\n");
        printf("Set preset to -1 when setting a static color\n");
        return;
    }
    brightness = atoi(argv[1]);
    if (brightness < 0x0 || brightness > 0x64) {
        printf("Brightness parameter must be between 0 and 100\n");
        return;
    }

    char *hex_color = argv[2];

    if (hex_color[0] == '#') {
        hex_color++;
    }

    if (strlen(hex_color) != 6) {
        printf("Error: Hex color must be 6 characters long (e.g., FF5733)\n");
        return 1;
    }

    for (int i = 0; i < 6; i++) {
        if (!((hex_color[i] >= '0' && hex_color[i] <= '9') ||
              (hex_color[i] >= 'A' && hex_color[i] <= 'F') ||
              (hex_color[i] >= 'a' && hex_color[i] <= 'f'))) {
            printf("Error: Invalid hex character '%c'\n", hex_color[i]);
            return 1;
        }
    }

    unsigned int hex_value = (unsigned int)strtol(hex_color, NULL, 16);

    int preset = atoi(argv[3]);
    if (preset < -1 || preset > 0) {
        printf("Preset not available.\n");
        return;
    }

    int speed = atoi(argv[4]);
    if (speed < 0x0 || speed > 0x64) {
        printf("Speed parameter must be between 0 and 100.\n");
        return;
    }
    
    int r = (hex_value >> 16) & 0xFF;
    int g = (hex_value >> 8) & 0xFF;
    int b = hex_value & 0xFF;

    NvAPI_Status status;
    NvPhysicalGpuHandle gpuHandles[NVAPI_MAX_PHYSICAL_GPUS];
    NvU32 gpuCount = 0;
    NV_I2C_INFO_V3 i2cInfo;
    NvU8 regAddress;
    NvU8 writeData[2];
    NvU8 readData[2];
    int i;
    
    for (i = 0; i < NVAPI_MAX_PHYSICAL_GPUS; i++) {
        gpuHandles[i] = 0;
    }
    
    status = NvAPI_Initialize();
    if (status != NVAPI_OK) {
        printf("NvAPI_Initialize failed!\n");
        return -1;
    } else {
        printf("Initted nvapi\n");
    }
    
    status = NvAPI_EnumPhysicalGPUs(gpuHandles, &gpuCount);
    if (status != NVAPI_OK || gpuCount == 0) {
        printf("NvAPI_EnumPhysicalGPUs failed!\n");
        return -1;
    }
    printf("Found %d GPUS\n", gpuCount);
    printf("Gpu[0] handle: %d\n", (int)gpuHandles[0]);
    
    memset(&i2cInfo, 0, sizeof(i2cInfo));
    
    i2cInfo.version = (3 << 16) | sizeof(NV_I2C_INFO_V3);
    
    i2cInfo.displayMask = 1;
    
    /* I2C device address (will be shifted to the right by 1) */
    i2cInfo.i2cDevAddress = 0x96;
    
    regAddress = 0x00;
    i2cInfo.pbI2cRegAddress = &regAddress;
    i2cInfo.regAddrSize = 0;
    
    i2cInfo.cbSize = 2;
    
    i2cInfo.i2cSpeed = 65535;
    i2cInfo.i2cSpeedKhz = NVAPI_I2C_SPEED_100KHZ;
    
    i2cInfo.bIsDDCPort = 1;
    i2cInfo.portId = 1;
    i2cInfo.bIsPortIdSet = 1;

    int numPairs = 0;

    if (preset == -1) {

        NvU32 idleColorPairs[][2] = {
            {0x20, 0x00},
            {0x21, 0x01},
            {0x22, 0x01},
            {0x23, r},
            {0x24, g},
            {0x25, b},
            {0x26, 0xA7},
            {0x27, 0x00},
            {0x28, 0xFF},
            {0x29, brightness},
            {0x2A, 0x14},
            {0x2B, 0x00},
            {0x2E, 0x01},
            {0x2C, 0x08},
            {0x2D, 0x00},
            {0x2F, 0x01},
            {0x17, 0x01},
        };

        numPairs = sizeof(idleColorPairs) / sizeof(idleColorPairs[0]);

        printf("Writing idle color data pairs\n");
        for (i = 0; i < numPairs; i++) {

            writeData[0] = idleColorPairs[i][0];
            writeData[1] = idleColorPairs[i][1];
            i2cInfo.pbData = writeData;
            status = NvAPI_I2CWrite(gpuHandles[0], &i2cInfo);
            printf("data[0]: %d, data[1]: %d, status: %d, status == NVAPI_OK: %d\n", i2cInfo.pbData[0], i2cInfo.pbData[1], status, status == NVAPI_OK);
        }

        NvU32 activeColorPairs[][2] = {
            {0x20, 0x01},
            {0x21, 0x01},
            {0x22, 0x01},
            {0x23, r},
            {0x24, g},
            {0x25, b},
            {0x26, 0xFF},
            {0x27, 0x00},
            {0x28, 0x00},
            {0x29, brightness},
            {0x2A, 0x14},
            {0x2B, 0x01},
            {0x2E, 0x01},
            {0x2C, 0x08},
            {0x2D, 0x01},
            {0x2F, 0x01},
            {0x17, 0x01},
        };

        numPairs = sizeof(activeColorPairs) / sizeof(activeColorPairs[0]);

        printf("Writing idle color data pairs\n");
        for (i = 0; i < numPairs; i++) {

            writeData[0] = activeColorPairs[i][0];
            writeData[1] = activeColorPairs[i][1];
            i2cInfo.pbData = writeData;
            status = NvAPI_I2CWrite(gpuHandles[0], &i2cInfo);
            printf("data[0]: %d, data[1]: %d, status: %d, status == NVAPI_OK: %d\n", i2cInfo.pbData[0], i2cInfo.pbData[1], status, status == NVAPI_OK);
        }
    } else {
        if (preset == 0) {
            NvU32 idleRainbowPairs[][2] = {
                {0x20, 0x00},
                {0x21, 0x01},
                {0x22, 0x09},
                {0x23, 0x00},
                {0x24, 0xFF},
                {0x25, 0x11},
                {0x26, 0xA7},
                {0x27, 0x00},
                {0x28, 0xFF},
                {0x29, 0x0A},
                {0x2A, speed},
                {0x2B, 0x00},
                {0x2E, 0x01},
                {0x2C, 0x08},
                {0x2D, 0x00},
                {0x2F, 0x01},
                {0x17, 0x01},
            };

            numPairs = sizeof(idleRainbowPairs) / sizeof(idleRainbowPairs[0]);

            printf("Writing idle color data pairs\n");
            for (i = 0; i < numPairs; i++) {

                writeData[0] = idleRainbowPairs[i][0];
                writeData[1] = idleRainbowPairs[i][1];
                i2cInfo.pbData = writeData;
                status = NvAPI_I2CWrite(gpuHandles[0], &i2cInfo);
                printf("data[0]: %d, data[1]: %d, status: %d, status == NVAPI_OK: %d\n", i2cInfo.pbData[0], i2cInfo.pbData[1], status, status == NVAPI_OK);
            }

            NvU32 activeRainbowPairs[][2] = {
                {0x20, 0x01},
                {0x21, 0x01},
                {0x22, 0x09},
                {0x23, 0x00},
                {0x24, 0xFF},
                {0x25, 0x11},
                {0x26, 0xFF},
                {0x27, 0x00},
                {0x28, 0x00},
                {0x29, 0x0A},
                {0x2A, speed},
                {0x2B, 0x01},
                {0x2E, 0x01},
                {0x2C, 0x08},
                {0x2D, 0x01},
                {0x2F, 0x01},
                {0x17, 0x01},
            };

            numPairs = sizeof(activeRainbowPairs) / sizeof(activeRainbowPairs[0]);

            printf("Writing idle color data pairs\n");
            for (i = 0; i < numPairs; i++) {

                writeData[0] = activeRainbowPairs[i][0];
                writeData[1] = activeRainbowPairs[i][1];
                i2cInfo.pbData = writeData;
                status = NvAPI_I2CWrite(gpuHandles[0], &i2cInfo);
                printf("data[0]: %d, data[1]: %d, status: %d, status == NVAPI_OK: %d\n", i2cInfo.pbData[0], i2cInfo.pbData[1], status, status == NVAPI_OK);
            }
        }
    }

    

    /* Read back the data to verify */
    /*printf("\nAttempting to read back data...\n");
    readData[0] = 0;
    readData[1] = 0;
    i2cInfo.pbData = readData;
    i2cInfo.cbSize = 2;
    
    status = NvAPI_I2CRead(gpuHandles[0], &i2cInfo);
    if (status == NVAPI_OK) {
        printf("I2C read successful!\n");
        printf("Read Data: 0x%02X 0x%02X\n", readData[0], readData[1]);
    } else {
        printf("I2C read failed with status: 0x%08X\n", status);
    }*/
    
    NvAPI_Unload();
    printf("\nNVAPI unloaded.\n");
    
    return 0;
}
