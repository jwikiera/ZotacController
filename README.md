# ZotacController

This is a small C program to control a `ZOTAC RTX 4070 Ti Trinity OC White Edition` card.

The software talks to the GPU using `nvapi.dll` and allows to set a static color with a given intensity, or a rainbow effect with a given speed.

## Writeup

### Reason

My card is not supported by OpenRGB, and the only way to configure its RGB lights is a proprietary GUI app, Firestorm Utility. Besides wanting a CLI interface in order to config all of my system lights at once, the Firestorm Utility is awful. Its interface is laggy, and you can't even use values to set colors.

### Dead ends

* ~~Static analysis of the Firestorm Utility.~~ Yeah no thanks.
* First I tried adapting the following OpenRGB branch: https://gitlab.com/peterberendi/OpenRGB/-/tree/zotac_4070_ti_super_trinity_oc, but for some reason my card differs quite a bit from the other 4070's added in this branch. (I also still have not figured out how to build a standalone OpenRGB windows executable, the app complains about a missing entry point).

### Solution

* The Firestorm app talks to the GPU using an Nvidia API located at nvapi.dll.
* The OpenRGB team has made a wrapper, [NvAPISpy](https://gitlab.com/OpenRGBDevelopers/Tools/NvAPISpy), for this dll, that logs i2c reads and writes made by it.
* Using nvapi myself:
I scoured Github for examples of how to use Nvapi. First comes the init + getting a GPU handle:
```c
NvAPI_Initialize();
NvAPI_EnumPhysicalGPUs(gpuHandles, &gpuCount);
```
In my case I have only card so I can take `gpuHandles[0]`. Next, to read or write data to the card, you need to populate a `NV_I2C_INFO_V3` struct and send it using `NvAPI_I2CWrite`. How to populate it? Use `NvAPISpy`. I modified it a little bit to print all fields in the `NV_I2C_INFO_V3` struct. Sample output:
```
NvAPI_I2CWrite:  GPU handle: 00000600 Ver: 0x3002 DispMask: 0x0001 DDC: Y Dev: 0x4B RegSize: 0x00 Reg: Size: 0x02 Data: 0x28 0x00 Speed: 65535 SpeedKhz: 4 Port: 1 PortSet: Y
```
Those can be mapped quite easily to the `NV_I2C_INFO_V3` struct. Beware, `Dev`, the identifier of the device (unique to each card model), is shifted for some reason. I had to put `0x96` to get `0x4B`. To get the data values, I poked at the Firestorm app and looked at the logs. Firestorm sends data packets in chunks of two bytes. There are a dozen writes sent each time some value is updated. Each write contains a two byte data pair. `pair[0]` seems to be always the same sequence, while `pair[1]'s` contain the interesting data. Diffing logs lets you determine the important values.


I hope my code can be helpful to anyone wanting to do something similar, the program is quite short.
