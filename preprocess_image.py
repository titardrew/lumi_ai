import argparse
import os

import cv2
import numpy as np


def apply_padding_pow2(img, divisor: int, pad_val: int = 0):
    pad_h = int(np.ceil(img.shape[0] / divisor)) * divisor
    pad_w = int(np.ceil(img.shape[1] / divisor)) * divisor
    width = max(pad_w - img.shape[1], 0)
    height = max(pad_h - img.shape[0], 0)
    padding = (0, 0, width, height)

    img = cv2.copyMakeBorder(
        img,
        padding[1],
        padding[3],
        padding[0],
        padding[2],
        cv2.BORDER_CONSTANT,
        value=pad_val)

    return img


def apply_padding(img, target_width, target_height):
    height = img.shape[0]
    width = img.shape[1]

    pad_y = max(target_height - height, 0)
    pad_x = max(target_width - width, 0)

    pad_bottom = pad_y
    pad_top = 0
    pad_right = pad_x
    pad_left = 0

    img = cv2.copyMakeBorder(
        img,
        pad_top,
        pad_bottom,
        pad_left,
        pad_right,
        cv2.BORDER_CONSTANT,
        value=0)

    return img


def rescale_keep_ratio(img, width, height, inter = cv2.INTER_AREA):
    img_height, img_width = img.shape[:2]

    if img_width < img_height:
        ratio = height / float(img_height)
        dim = (int(img_width * ratio), height)
    else:
        ratio = width / float(img_width)
        dim = (width, int(img_height * ratio))

    resized = cv2.resize(img, dim, interpolation = inter)
    return resized


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("-i", "--input", type=str, help="Input image path.")
    args = parser.parse_args()

    INPUT_W = 416
    INPUT_H = 256

    img = cv2.imread(args.input)
    img = rescale_keep_ratio(img, INPUT_W, INPUT_H)
    img = apply_padding(img, INPUT_W, INPUT_H)
    split_path = os.path.splitext(args.input)

    cv2.imwrite(f"{split_path[0]}_prep{split_path[1]}", img)


if __name__ == "__main__":
    main()
