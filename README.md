## IMG-1-02 Plasma display driver

![](./kdpv.jpg)

### IMG102_badapple_328p_test_image_(mono, 4 colors, 8 colors)_328p

Simple programs for show static images on IMG-1-02 plasma display by AVR Atmega328p microcontroller. All folders have a python script for convert images to byte array.
Usage ex.:
```python bmp2raw_img_4colors.py IMG102gray.bmp```

### IMG102_badapple_328p

A python script sends the video over UART to the AVR Atmega328p microcontroller, then that data is displayed on IMG-1-02 display. 

How to use:
1. Compile the firmware and flash the microcontroller 
2. Connect microcontroller to PC. In file badapple.py set the COM port.
3. Download badapple60fps.mp4 by link https://drive.google.com/file/d/17_bePX1w6BbXz39kd6KIoPLDp9sqObGx/view?usp=sharing
4. Run ```python badapple.py```

### IMG102_UART_Display 

A python program sends the captured screen over UART to the STM32F103C8 microcontroller, then that data is displayed on IMG-1-02 display. 

How to use:
1. Compile the firmware and flash the microcontroller.
2. Connect microcontroller to PC with any USB-UART converter.
3. Install all dependencies ```pip install requirements.txt```
4. Run ```GUI.pyw```

### Hardware

Project for Diptrace

### Demo:
1. https://youtu.be/T3mTXGTW0yQ

