Overview
========
The usart dma ring buffer example shows how to use usart driver with DMA driver used:

In this example, a ring buffer was implemented to receive the characters from PC side, 
to implemente the ring buffer, a descriptor was created and chain itself as the next descriptor,
then the DMA transfer will start a continuous transfer.

While data in the ring buffer reach 8 characters, routine will send them out using DMA mode.


Toolchain supported
===================
- IAR embedded Workbench  8.50.6
- MCUXpresso  11.3.0

Hardware requirements
=====================
- Mini USB cable
- OM15076-3 Carrier Board
- JN518x Module plugged on the Carrier Board
- Personal Computer

Board settings
==============
No special settings are required.

Prepare the Demo
================
1.  Connect a mini USB cable between the PC host and the USB port (J15) on the board.
2.  Open a serial terminal with the following settings:
    - 115200 baud rate
    - 8 data bits
    - No parity
    - One stop bit
    - No flow control
3.  Download the program to the target board.
4.  Reset the SoC and run the project.

Running the demo
================
The following lines are printed to the serial terminal when the demo program is executed.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
USART DMA ring buffer example
Board will send back received data
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Customization options
=====================

