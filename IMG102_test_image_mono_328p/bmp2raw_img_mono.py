import cv2
import numpy as np
import sys
from numba import njit


@njit(cache=True)
def frame_to_byte_array(raw_frame):
    output_byte_array = np.zeros(int(raw_frame.shape[0] * raw_frame.shape[1] // 8), dtype=np.uint8)
    index = 0
    for y in range(0, raw_frame.shape[0]):
        for x in range(0,  raw_frame.shape[1], 8):
            byte = 0
            for offset in range(0, 8):
                byte += raw_frame[y][x+offset] // 255 * (128 >> offset)
            output_byte_array[index] = ~byte
            index += 1
    return output_byte_array


def byte_array_to_hex(byte_array):
    with open("image_mono.h", 'w') as output_file:
        output_file.write('static const uint8_t image[100][13] PROGMEM = {\n')
        counter = 0
        for i in byte_array:
            output_file.write(hex(i) + ', ')
            counter += 1
            if counter == 13:
                output_file.write('\n')
                counter = 0
        output_file.write('};\n')


if __name__ == "__main__":
    try:
        input_img = cv2.imread(sys.argv[1])
    except Exception:
        print("No input file specified")
        sys.exit(1)
    input_img = cv2.resize(input_img, (100, 100), interpolation=cv2.INTER_AREA)
    input_img = cv2.cvtColor(input_img, cv2.COLOR_BGR2GRAY)
    input_img = cv2.copyMakeBorder(input_img, 0, 0, 0, 4, cv2.BORDER_CONSTANT, value=[255, 255, 255])
    ret, input_img = cv2.threshold(input_img, 127, 255, cv2.THRESH_BINARY)

    byte_array = frame_to_byte_array(input_img)
    byte_array_to_hex(byte_array)
