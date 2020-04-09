# _UART Echo Example_

_This is an example which echoes any data it receives on UART0 back to the sender._

## How to use example

### Hardware Required

1. Connect an external serial interface to an ESP8266 board. The external interface should have 3.3V outputs. You may use e.g. 3.3V compatible USB-to-serial dongle:  

  | ESP8266 Interface | #define | ESP8266 Pin | External UART Pin |  
  | --- | --- | --- | --- |  
  | Transmit Data (TxD) | ECHO_TEST_TXD | GPIO26 | RxD |  
  | Receive Data (RxD) | ECHO_TEST_RXD | GPIO25 | TxD |  
  | Ground | n/a | GND | GND |  

2. Verify if echo indeed comes from ESP8266 by disconnecting either 'TxD' or 'RxD' pin. There should be no any echo once any pin is disconnected.

* Using a hardware flow control

  This is an optional check to verify if the hardware flow control works. To set it up you need an external serial interface that has RTS and CTS signals. 

  1. Connect the extra RTS/CTS signals as below  

      | ESP8266 Interface | #define | ESP8266 Pin | External UART Pin |  
      | --- | --- | --- | --- |  
      | Request to Send (RTS) | ECHO_TEST_RTS | GPIO13 | CTS |  
      | Clear to Send (CTS) | ECHO_TEST_CTS | GPIO12 | RTS |  
  
  2. Configure UART0 driver to use the hardware flow control by setting `.flow_ctrl = UART_HW_FLOWCTRL_CTS_RTS` and adding `.rx_flow_ctrl_thresh = 122`

### Configure the project

```
make menuconfig
```

* Set serial port under Serial Flasher Options.  
* `make monitor` baud rate set to what you set in the example.


### Build and Flash

Build the project and flash it to the board, then run monitor tool to view serial output:

```
make -j4 flash monitor
```

(To exit the serial monitor, type ``Ctrl-]``.)

See the Getting Started Guide for full steps to configure and use ESP-IDF to build projects.

## Example Output  

```
I (180) boot: Loaded app from partition at offset 0x10000
I (0) system_api: Base MAC address is not set, read default base MAC address from BLK0 of EFUSE
I (0) system_api: Base MAC address is not set, read default base MAC address from BLK0 of EFUSE
0123456789
```