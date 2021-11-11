import cv2
import numpy as np
import sys
import time
from numba import njit
import serial

UART = serial.Serial()


@njit(cache=True)
def frame_to_byte_array(raw_frame, output_byte_array):
    index = 0
    for y in range(0, raw_frame.shape[0]):
        for x in range(0,  raw_frame.shape[1], 8):
            byte = 0
            for offset in range(0, 8):
                byte += raw_frame[y][x + offset] // 255 * (128 >> offset)
            output_byte_array[index] = ~byte
            index += 1


if __name__ == "__main__":
    UART.port = 'COM4'
    UART.baudrate = '1000000'
    UART.open()
    raw_frame = np.zeros((100, 104), dtype=np.uint8)
    output_byte_array = np.zeros(int(raw_frame.shape[0] * raw_frame.shape[1] // 8), dtype=np.uint8)

    try:
        input_video = cv2.VideoCapture('BadApple60FPS.mp4')
    except Exception:
        print("No input file specified or something went wrong")
        sys.exit(1)

    frame_delay = time.time()
    success, input_img = input_video.read()
    while (success):
        input_img = cv2.resize(input_img, (100, 75), interpolation=cv2.INTER_AREA)
        input_img = cv2.cvtColor(input_img, cv2.COLOR_BGR2GRAY)
        input_img = cv2.copyMakeBorder(input_img, 12, 13, 0, 4, cv2.BORDER_CONSTANT, value=[0, 0, 0])
        ret, input_img = cv2.threshold(input_img, 127, 255, cv2.THRESH_BINARY)

        frame_to_byte_array(input_img, output_byte_array)
        UART.write(bytearray(output_byte_array))
        success, input_img = input_video.read()
        delta = time.time() - frame_delay
        if delta < 0.015:
            time.sleep(0.015 - delta)
        frame_delay = time.time()
