# Blinker hello example  

put the componets into your projects.

make the project list like this  
```
Blinker_Hello
  |__components
  |  |__blinker
  |__main
```

```
idf.py menuconfig
```

enter Blinker Config  

* set BLINKER AUTH KEY  

* set Provisioning Type  

save and exit blinker config  

* Set serial port under Serial Flasher Options.  

enter components config  

* enable mDNS  

* disable Newlib  

save and exit

## Build and Flash  

Build the project and flash it to the board, then run monitor tool to view serial output:

```
idf.py -p PORT flash monitor
```  
