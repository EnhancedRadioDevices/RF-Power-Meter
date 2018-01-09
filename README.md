Enhanced Radio Devices - RF Power Meter
=======

The ERD RF Power Meter is designed as an affordable RF Power Meter for use in cases where the precision of expensive professional equipment is not required.

The device is available to the host system as a USB serial device, which when accessed automatically prints the measured RF level.

This repository contains the source code to be compiled with AVR-GCC, and uploaded to the device via ATMEL's FLIP programmer or the dfu-programmer utility.

## Drivers
The Power Meter board is automatically recognized as a USB serial device under OSX and Linux, however, windows requires a driver to associate the device with the built in USB serial device drivers.

When first attaching the device to a windows based host, windows will attempt to find a driver and fail. You may then specify a driver. This is provided in ERD_RF_Power_Meter.inf.

Once you have installed the driver, the Power Meter will register as a COM port device under windows, and you may access it with any serial/terminal application of your preference.

### Windows Users
During the driver installation process on Windows 8 operating systems and above you may encounter *"The third-party INF does not contain digital signature information"* preventing the installation of the included driver.  The steps below outline the process for disabling driver signature enforcement on Windows 8/8.1 systems.

**NOTE:** If your system uses BitLocker Drive Encryption, you will need your encryption key to gain access to advanced settings to disable driver signature enforcement.

Windows 8 - Navigate to PC settings > General and look for the *Advanced startup* dialog.  Click the **Restart now** button to access the advanced startup settings.  Your system will now reboot.

Windows 8.1 - Navigate to PC settings > Update and Recovery > Recovery and look for the *Advanced startup* dialog.  Click the **Restart now** button to access the advanced startup settings.  Your system will now reboot.

After rebooting select Troubleshoot > Advanced options > Startup Settings, click the **Restart** button, your machine will reboot a second time.

After rebooting a second time you should be presented with the Windows Startup Settings dialog, choose *Disable driver signature enforcement* by pressing number `7` on your keyboard.  Your machine will reboot automatically.

Once rebooted, you should now be able to install the drviers as outlined above.  Once the appropriate driver is selected you will see a Windows Security dialog warning against the installation of the unsigned driver.  Click **Install this driver software anyway** to complete the driver installation.
