import cv2
import numpy as np
import sys
from numba import njit


@njit(cache=True)
def find_closest_color(value):
    closest = np.round(7 * value/255.0) * (255/7)
    return closest


@njit(cache=True)
def value_clip(value):
    if value > 255:
        value = 255
    elif value < 0:
        value = 0
    return value


@njit(cache=True)
def dithering_gray_fs(image_matrix):
    h = image_matrix.shape[0]
    w = image_matrix.shape[1]

    for y in range(0, h):
        for x in range(0, w):
            old_pixel = image_matrix[y, x]
            new_pixel = find_closest_color(old_pixel)
            image_matrix[y, x] = new_pixel
            quant_error = old_pixel-new_pixel

            if y < 99 and x < 99:
                image_matrix[y, x+1] = value_clip(image_matrix[y, x+1] + quant_error * 7 / 16.0)
                image_matrix[y+1, x-1] = value_clip(image_matrix[y+1, x-1] + quant_error * 3 / 16.0)
                image_matrix[y+1, x] = value_clip(image_matrix[y+1, x] + quant_error * 5 / 16.0)
                image_matrix[y+1, x+1] = value_clip(image_matrix[y+1, x+1] + quant_error * 1 / 16.0)

    return image_matrix


@njit(cache=True)
def img_to_raw_frame(input_img):
    raw_frame = np.zeros((100 * 3, 104), dtype=np.uint8)
    input_img = input_img >> 5  # Reduce bitdepth to 3-bit
    for y in range(input_img.shape[0]):
        for x in range(input_img.shape[1]):
            for i in range(3):
                raw_frame[y+input_img.shape[0] * i][x] = (input_img[y][x] >> i) % 2
    return raw_frame


@njit(cache=True)
def frame_to_byte_array(raw_frame):
    output_byte_array = np.zeros(int(raw_frame.shape[0] * raw_frame.shape[1] // 8), dtype=np.uint8)
    index = 0
    for y in range(0, raw_frame.shape[0]):
        for x in range(0,  raw_frame.shape[1], 8):
            byte = 0
            for offset in range(0, 8):
                byte += raw_frame[y][x+offset] * (128 >> offset)
            output_byte_array[index] = ~byte
            index += 1
    return output_byte_array


def byte_array_to_hex(byte_array):
    with open("image_8.h", 'w') as output_file:
        output_file.write('static const uint8_t image[300][13] PROGMEM = {\n')
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
        print("No input file specified or something went wrong")
        sys.exit(1)
    input_img = cv2.resize(input_img, (100, 100), interpolation=cv2.INTER_AREA)
    input_img = cv2.cvtColor(input_img, cv2.COLOR_BGR2GRAY)
    input_img = dithering_gray_fs(input_img)
    input_img = cv2.copyMakeBorder(input_img, 0, 0, 0, 4, cv2.BORDER_CONSTANT, value=[255, 255, 255])
    raw_frame = img_to_raw_frame(input_img)
    byte_array = frame_to_byte_array(raw_frame)
    byte_array_to_hex(byte_array)
