# MPC-HC host

MPC-HC host is a tool for instance synchronization.
It's may be useful when you want to use multiple audio devices with custom audio channel mapping.

### How to build

Execute Native Tools command prompt for Visual Studio and call:
```
cl.exe main.c /link /SUBSYSTEM:CONSOLE /user32.lib
```

### Usage

Start the tool and it will print it's hwnd on console.
Then start MPC-HC executables with arguments:
```
/slave [your hwnd]
Example: mpc-hc64.exe /slave 123456
where 123456 is a hwnd
```
First connected instance will be main, other instances will sync to it.
