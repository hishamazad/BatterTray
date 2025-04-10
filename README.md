# 🔋 BatteryTray
**BatteryTray** is a lightweight Windows tray application that monitors your battery level and sends periodic updates to a remote server. It includes a tray icon with a right-click menu for quick actions and runtime configuration.

## Features
- Battery level monitoring
- Sends battery data to a remote server at configurable intervals
- Editable update interval and target URL via settings dialog
- Option to start automatically with Windows (Registry-based)
- Tray menu options: Send Battery Now, Settings, Start with Windows (toggle), Exit

## Sample Output
https://yourserver.com?datatype=Battery%20Level&data=75&note=MyComputer

## Demo
https://www.postb.in/1744257667496-8851193876471

## Requirements
- Windows 10/11
- Visual Studio with C++ and Win32 API support

## Building  
Open `BatteryTray.sln` in Visual Studio, build and run. You can change the default endpoint via Settings or in the code.

## Configuration
Accessible via tray menu → Settings:  
- Remote URL (target endpoint for battery data)  
- Update interval (minimum: 10 seconds)

## Resources
Ensure your `.rc` file includes:  
- Icons: `IDI_BATTERTRAY`, `IDI_BATTERTRAYLOW`, `IDI_BATTERTRAYHALF`  
- Dialog: `IDD_SETTINGS`  
- Controls: `IDC_EDIT_URL`, `IDC_EDIT_INTERVAL`

## License
MIT License