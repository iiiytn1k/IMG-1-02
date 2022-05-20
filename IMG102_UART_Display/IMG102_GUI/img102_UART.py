import cv2
import numpy as np
import serial
import time
from mss import mss
from image_utils import *

UART = serial.Serial()
DEBUG_FLAG = False


class UART_send_data_thread():
    def __init__(self, capture_x, capture_y, capture_size,
                 manual_flag, hist_eq_flag, clahe_flag,
                 brightness, contrast, gamma, clip_limit, dithering_method):
        self.startflag = False
        self.capture_x = capture_x
        self.capture_y = capture_y
        self.capture_size = capture_size
        self.manual_flag = manual_flag
        self.hist_eq_flag = hist_eq_flag
        self.clahe_flag = clahe_flag
        self.brightness = brightness
        self.contrast = contrast
        self.gamma = gamma
        self.clip_limit = clip_limit
        self.dithering_method = dithering_method

    def main_loop(self):
        sct = mss()

        raw_frame = np.zeros((100 * 2, 104), dtype=np.uint8)
        output_byte_array = np.zeros(int(raw_frame.shape[0] * (raw_frame.shape[1] + 8) // 8), dtype=np.uint8)
        t = time.time()
        while True:
            while not self.startflag:
                pass
            while self.startflag:
                capture_settings = {'top': self.capture_y, 'left': self.capture_x, 'width': self.capture_size, 'height': self.capture_size}
                
                # Capture image, resize and convert to grayscale:
                input_img = np.array(sct.grab(capture_settings))
                input_img = cv2.resize(input_img, (100, 100), interpolation=cv2.INTER_AREA)
                input_img = cv2.cvtColor(input_img, cv2.COLOR_BGR2GRAY)

                # Apply brightness, contrast and gamma correction. Manually or automatically.
                if self.manual_flag:
                    input_img = brightness_contrast_correction(input_img, self.brightness, self.contrast)
                    if self.gamma != 1.0:
                        input_img = gamma_correction(input_img, self.gamma)
                else:
                    if self.hist_eq_flag:
                        input_img = cv2.equalizeHist(input_img)
                    else:
                        clahe = cv2.createCLAHE(clipLimit=self.clip_limit, tileGridSize=(1, 1))
                        input_img = clahe.apply(input_img)

                # Convert to 4-colors palette w/wo dithering:
                input_img = dithering_gray(input_img, self.dithering_method)

                # Change canvas size to 104x100
                input_img = cv2.copyMakeBorder(input_img, 0, 0, 0, 4, cv2.BORDER_CONSTANT, value=[0, 0, 0])

                # Convert to 2 monochrome images and stack into one vertical frame:
                img_to_raw_frame(input_img, raw_frame)

                # Convert frame to byte array:
                raw_frame_to_byte_array(raw_frame, output_byte_array)
                
                if DEBUG_FLAG:
                    debug_img = cv2.resize(input_img, (500, 500), interpolation = cv2.INTER_NEAREST)
                    cv2.imshow("Debug", debug_img)
                    cv2.waitKey(1)

                # Send byte array via UART:
                UART.write(bytearray(output_byte_array))
                delta = time.time() - t
                if delta < 0.018: time.sleep(0.018 - delta)
                t = time.time()
