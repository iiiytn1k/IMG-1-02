from numba import njit
import numpy as np
import cv2

bayer8x8 = 1/64 * (np.array([
            [ 0, 48, 12, 60,  3, 51, 15, 63],
            [32, 16, 44, 28, 35, 19, 47, 31],
            [ 8, 56,  4, 52, 11, 59,  7, 55],
            [40, 24, 36, 20, 43, 27, 39, 23],
            [ 2, 50, 14, 62,  1, 49, 13, 61],
            [34, 18, 46, 30, 33, 17, 45, 29],
            [10, 58,  6, 54,  9, 57,  5, 53],
            [42, 26, 38, 22, 41, 25, 37, 21]
        ]))


# Find closest color value in 4-colors palette range (0,85,170,255)
@njit(cache=True)
def find_closest_color(value):
    closest = np.round(3 * value/255.0) * (255/3)
    return closest


@njit(cache=True)
def value_clip(value):
    if value > 255:
        value = 255
    elif value < 0:
        value = 0
    return value


@njit(cache=True)
def dithering_gray(image_matrix, method):
    h = image_matrix.shape[0]
    w = image_matrix.shape[1]

    if method == 'Floyd-Steinberg':
        for y in range(0, h):
            for x in range(0, w):
                old_pixel = image_matrix[y, x]
                new_pixel = find_closest_color(old_pixel)
                image_matrix[y, x] = new_pixel
                quant_error = old_pixel-new_pixel

                if y < h-1 and x < w-1:
                    image_matrix[y, x+1] = value_clip(image_matrix[y, x+1] + quant_error * 7 / 16.0)
                    image_matrix[y+1, x-1] = value_clip(image_matrix[y+1, x-1] + quant_error * 3 / 16.0)
                    image_matrix[y+1, x] = value_clip(image_matrix[y+1, x] + quant_error * 5 / 16.0)
                    image_matrix[y+1, x+1] = value_clip(image_matrix[y+1, x+1] + quant_error * 1 / 16.0)

    elif method == 'Atkinson':
        for y in range(0, h):
            for x in range(0, w):
                old_pixel = image_matrix[y, x]
                new_pixel = find_closest_color(old_pixel)
                image_matrix[y, x] = new_pixel
                quant_error = old_pixel-new_pixel

                if y < h-2 and x < w-2:
                    image_matrix[y, x+1] = value_clip(image_matrix[y, x+1] + quant_error * 1 / 8.0)
                    image_matrix[y, x+2] = value_clip(image_matrix[y, x+2] + quant_error * 1 / 8.0)
                    image_matrix[y+1, x-1] = value_clip(image_matrix[y+1, x-1] + quant_error * 1 / 8.0)
                    image_matrix[y+1, x] = value_clip(image_matrix[y+1, x] + quant_error * 1 / 8.0)
                    image_matrix[y+1, x+1] = value_clip(image_matrix[y+1, x+1] + quant_error * 1 / 8.0)
                    image_matrix[y+2, x] = value_clip(image_matrix[y+2, x] + quant_error * 1 / 8.0)

    elif method == 'Burkes':
        for y in range(0, h):
            for x in range(0, w):
                old_pixel = image_matrix[y, x]
                new_pixel = find_closest_color(old_pixel)
                image_matrix[y, x] = new_pixel
                quant_error = old_pixel-new_pixel

                if y < h-1 and x < w-2:
                    image_matrix[y, x+1] = value_clip(image_matrix[y, x+1] + quant_error * 8 / 32.0)
                    image_matrix[y, x+2] = value_clip(image_matrix[y, x+2] + quant_error * 4 / 32.0)
                    image_matrix[y+1, x-2] = value_clip(image_matrix[y+1, x-2] + quant_error * 2 / 32.0)
                    image_matrix[y+1, x-1] = value_clip(image_matrix[y+1, x-1] + quant_error * 4 / 32.0)
                    image_matrix[y+1, x] = value_clip(image_matrix[y+1, x] + quant_error * 8 / 32.0)
                    image_matrix[y+1, x+1] = value_clip(image_matrix[y+1, x+1] + quant_error * 4 / 32.0)
                    image_matrix[y+1, x+2] = value_clip(image_matrix[y+1, x+2] + quant_error * 2 / 32.0)

    elif method == 'Sierra':
        for y in range(0, h):
            for x in range(0, w):
                old_pixel = image_matrix[y, x]
                new_pixel = find_closest_color(old_pixel)
                image_matrix[y, x] = new_pixel
                quant_error = old_pixel-new_pixel

                if y < h-2 and x < w-2:
                    image_matrix[y, x+1] = value_clip(image_matrix[y, x+1] + quant_error * 5 / 32.0)
                    image_matrix[y, x+2] = value_clip(image_matrix[y, x+2] + quant_error * 3 / 32.0)
                    image_matrix[y+1, x-2] = value_clip(image_matrix[y+1, x-2] + quant_error * 2 / 32.0)
                    image_matrix[y+1, x-1] = value_clip(image_matrix[y+1, x-1] + quant_error * 4 / 32.0)
                    image_matrix[y+1, x] = value_clip(image_matrix[y+1, x] + quant_error * 5 / 32.0)
                    image_matrix[y+1, x+1] = value_clip(image_matrix[y+1, x+1] + quant_error * 4 / 32.0)
                    image_matrix[y+1, x+2] = value_clip(image_matrix[y+1, x+2] + quant_error * 2 / 32.0)
                    image_matrix[y+2, x-1] = value_clip(image_matrix[y+2, x-1] + quant_error * 2 / 32.0)
                    image_matrix[y+2, x] = value_clip(image_matrix[y+2, x] + quant_error * 3 / 32.0)
                    image_matrix[y+2, x+1] = value_clip(image_matrix[y+2, x+1] + quant_error * 2 / 32.0)

    elif method == 'Two-Row Sierra':
        for y in range(0, h):
            for x in range(0, w):
                old_pixel = image_matrix[y, x]
                new_pixel = find_closest_color(old_pixel)
                image_matrix[y, x] = new_pixel
                quant_error = old_pixel-new_pixel

                if y < h-1 and x < w-2:
                    image_matrix[y, x+1] = value_clip(image_matrix[y, x+1] + quant_error * 4 / 16.0)
                    image_matrix[y, x+2] = value_clip(image_matrix[y, x+2] + quant_error * 3 / 16.0)
                    image_matrix[y+1, x-2] = value_clip(image_matrix[y+1, x-2] + quant_error * 1 / 16.0)
                    image_matrix[y+1, x-1] = value_clip(image_matrix[y+1, x-1] + quant_error * 2 / 16.0)
                    image_matrix[y+1, x] = value_clip(image_matrix[y+1, x] + quant_error * 3 / 16.0)
                    image_matrix[y+1, x+1] = value_clip(image_matrix[y+1, x+1] + quant_error * 2 / 16.0)
                    image_matrix[y+1, x+2] = value_clip(image_matrix[y+1, x+2] + quant_error * 1 / 16.0)

    elif method == 'Sierra-lite':
        for y in range(0, h):
            for x in range(0, w):
                old_pixel = image_matrix[y, x]
                new_pixel = find_closest_color(old_pixel)
                image_matrix[y, x] = new_pixel
                quant_error = old_pixel-new_pixel

                if y < h-1 and x < w-1:
                    image_matrix[y, x+1] = value_clip(image_matrix[y, x+1] + quant_error * 2 / 4.0)
                    image_matrix[y+1, x-1] = value_clip(image_matrix[y+1, x-1] + quant_error * 1 / 4.0)
                    image_matrix[y+1, x] = value_clip(image_matrix[y+1, x] + quant_error * 1 / 4.0)

    elif method == '8x8 Bayer':
        for y in range(0, h):
            for x in range(0, w):
                old_pixel = image_matrix[y, x]
                old_pixel = value_clip(old_pixel * 0.84 + (old_pixel * bayer8x8[y % 8][x % 8]))
                new_pixel = find_closest_color(old_pixel)
                image_matrix[y, x] = new_pixel

    elif method == 'None':
        for y in range(0, h):
            for x in range(0, w):
                old_pixel = image_matrix[y, x]
                new_pixel = find_closest_color(old_pixel)
                image_matrix[y, x] = new_pixel

    return image_matrix


@njit(cache=True)
def img_to_raw_frame(input_img, output_frame):
    input_img = input_img >> 6  # Reduce bitdepth to 2-bit
    for y in range(input_img.shape[0]):
        for x in range(input_img.shape[1]):
            for i in range(2):
                output_frame[y+input_img.shape[0] * i][x] = (input_img[y][x] >> i) % 2


@njit(cache=True)
def raw_frame_to_byte_array(raw_frame, output_byte_array):
    index = 0
    for y in range(0, raw_frame.shape[0]):
        for x in range(0,  raw_frame.shape[1], 8):
            byte = 0
            for offset in range(0, 8):
                byte += raw_frame[y][x+offset] * (128 >> offset)
            output_byte_array[index] = ~byte
            index += 1


def brightness_contrast_correction(input_img, brightness, contrast):
    if brightness != 0:
        if brightness > 0:
            shadow = brightness
            highlight = 255
        else:
            shadow = 0
            highlight = 255+brightness
        alpha_b = (highlight-shadow) / 255
        buf = cv2.addWeighted(input_img, alpha_b, input_img, 0, shadow)
    else:
        buf = input_img

    if contrast != 0:
        f = 131 * (contrast+127) / (127 * (131-contrast))
        gamma_c = 127 * (1-f)
        buf = cv2.addWeighted(buf, f, buf, 0, gamma_c)
    return buf


def gamma_correction(input_img, gamma):
    invGamma = 1.0 / gamma
    lookup_table = np.array([((i/255) ** invGamma) * 255 for i in range(256)], dtype=np.uint8)
    return cv2.LUT(input_img, lookup_table)
