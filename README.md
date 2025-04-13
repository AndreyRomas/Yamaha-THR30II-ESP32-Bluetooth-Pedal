Minimalist wireless pedal based on esp32-s3 for Yamaha THR30II that switches between two presets.

![device](https://github.com/AndreyRomas/Yamaha-THR30II-ESP32-Bluetooth-Pedal/blob/dcaa0c728c3ff72375fa19b69dc0605f48609f22/images/front.jpg)

Warning: although it is highly unlikely to break anything, I cannot guarantee that this cannot damage your amp. Do it at your own risk. 

I was not able to find any other projects like that, so I decided to develop my own and push it to github.
There are multiple projects for devices that control the amp via USB cable. Or Bluetooth, but instead of doing it directly, it connects an esp-based device to a phone running the official application that is connected to the amp.

This one connects to the amp directly. It does it pretty quickly btw. It can't be used at the same time when your phone is connected to the amp. If the signal is lost, it reconnects. 

There are 2 LEDs: the left one represents connection. If it is yellow, it tries to connect to the amp. As soon as the connection is established it turns blue. The right one represents the battery charge level. Red, yellow or green depending on the current battery's voltage. There 2 buttons: the top one wakes the device from deep sleep, and the same one sends it into deep sleep mode. The bottom one switches between presets.

I'm not a huge expert in bluetooth or development for microcontrollers. But after endless experiments, I finally have the working code. 
It is developed and tested with the latest firmware currently available v1.5.0. It might work with the newer ones, but I'll do my best to update the code if something changes after an update.

The app exchanges many more packets with the amp, but I found that most of them can be excluded except for those you see in the code.

I do not expect people to reproduce the project 1:1, because I believe it is pretty easy to design it better and just use the code as a base. But just in case I attach the 3d model, schematics, and hardware list I used to build the final device. (Please first of all check that the code works with your amp.) Feel free to use it fully or partially:

- Seeed Xiao ESP32-S3
- BHC-18650-1P battery holder
- Metal button ø16mm (random one I found at a local shop)
- Same for the footswitch, but something very similar I find by googling "PBS-24-202"
- 2 ws2812b leds
- 4 x 3mm thread inserts. Outer diameter 4.2mm

![schematics](https://github.com/AndreyRomas/Yamaha-THR30II-ESP32-Bluetooth-Pedal/blob/dcaa0c728c3ff72375fa19b69dc0605f48609f22/images/schematic.jpg)

![inside](https://github.com/AndreyRomas/Yamaha-THR30II-ESP32-Bluetooth-Pedal/blob/dcaa0c728c3ff72375fa19b69dc0605f48609f22/images/inside.jpg)
