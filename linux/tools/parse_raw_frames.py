import sys
import codecs

if (sys.version > '3.0'):
    version_3 = True
else:
    version_3 = False

if (version_3):
    from tkinter import filedialog
    from tkinter import *
else:
    # from Tkinter import tkFileDialog
    from Tkinter import *
    import tkFileDialog


import fnmatch
from subprocess import call
import struct

import os

import argparse

def shift(val, shift_val):

    shifted_val = val

    if (shift_val > 0):
        shifted_val = shifted_val << shift_val
    elif (shift_val < 0):
        shifted_val = shifted_val >> abs(shift_val)

    return shifted_val

# assumes frame starts with start_byte value and increments thereafter
def create_pngs(source_path, filename):

    #full_filename = os.path.join(source_path, filename)
    shift_amplitude_16 = 0;
    shift_depth_16 = 0;
    shift_amplitude_8 = 0;
    shift_depth_8 = 0;

    prefix = filename.split('.')[0]

    filename_depth_16 = prefix+"_depth_16."+"raw"
    filename_amplitude_16 = prefix+"_amplitude_16."+"raw"
    filename_depth_8 = prefix+"_depth_8."+"raw"
    filename_amplitude_8 = prefix+"_amplitude_8."+"raw"

    #raw_fh = open(os.path.join(source_path, filename), "rb")
    #bytes_read = raw_fh.read()
    raw_16_depth_fh = open(os.path.join(source_path, filename_depth_16), "wb")
    raw_16_amplitude_fh = open(os.path.join(source_path, filename_amplitude_16), "wb")
    raw_8_depth_fh = open(os.path.join(source_path, filename_depth_8), "wb")
    raw_8_amplitude_fh = open(os.path.join(source_path, filename_amplitude_8), "wb")

    with open(os.path.join(source_path, filename), "rb") as raw_fh:
        depth = raw_fh.read(2)
        amplitude = raw_fh.read(2)
        while depth:
            if (version_3):
                depth_16 = int.from_bytes(depth, byteorder='little')
                depth_16_shifted = shift(depth_16, 7)
                depth_16_shifted_bytes = (depth_16_shifted & 0xFFFF).to_bytes(2, byteorder='big')
                depth_8_shifted = shift(depth_16, -1)
                depth_8_shifted_bytes = (depth_8_shifted & 0xFF).to_bytes(1, byteorder='big')
                # depth_8_shifted = (depth_8_shifted).to_bytes(1, byteorder='big')
                raw_16_depth_fh.write(depth_16_shifted_bytes)
                raw_8_depth_fh.write(depth_8_shifted_bytes)

                amplitude_16 = int.from_bytes(amplitude, byteorder='little')
                amplitude_16_shifted = shift(amplitude_16, 4)
                amplitude_16_shifted_bytes = (amplitude_16_shifted & 0xFFFF).to_bytes(2, byteorder='big')
                amplitude_8_shifted = shift(amplitude_16, -4)
                amplitude_8_shifted_bytes = (amplitude_8_shifted & 0xFF).to_bytes(1, byteorder='big')
                raw_16_amplitude_fh.write(amplitude_16_shifted_bytes)
                raw_8_amplitude_fh.write(amplitude_8_shifted_bytes)
            else:
                depth_16 = struct.unpack("h", depth)[0]
                depth_16_shifted = shift(depth_16, 7)
                depth_16_shifted_bytes = struct.pack("H", (depth_16_shifted & 0xFFFF))
                raw_16_depth_fh.write(depth_16_shifted_bytes)

                depth_8_shifted = shift(depth_16, -1)
                depth_8_shifted_bytes = struct.pack("B", depth_8_shifted & 0xFF)
                raw_8_depth_fh.write(depth_8_shifted_bytes)

                amplitude_16 = struct.unpack("h", amplitude)[0]
                amplitude_16_shifted = shift(amplitude_16, 7)
                amplitude_16_shifted_bytes = struct.pack("H", (amplitude_16_shifted & 0xFFFF))
                raw_16_amplitude_fh.write(amplitude_16_shifted_bytes)

                amplitude_8_shifted = shift(amplitude_16, -1)
                amplitude_8_shifted_bytes = struct.pack("B", amplitude_8_shifted & 0xFF)
                raw_8_amplitude_fh.write(amplitude_8_shifted_bytes)

            depth = raw_fh.read(2)
            amplitude = raw_fh.read(2)

    #raw_fh.close()
    raw_16_depth_fh.close()
    raw_16_amplitude_fh.close()
    raw_8_depth_fh.close()
    raw_8_amplitude_fh.close()

    call(["convert", "-depth", "16", "-size", "120x720+0", "gray:"+os.path.join(source_path, filename_depth_16), prefix+"_depth_16.png"])
    call(["convert", "-depth", "16", "-size", "120x720+0", "gray:"+os.path.join(source_path, filename_amplitude_16), prefix+"_amplitude_16.png"])
    call(["convert", "-depth", "8", "-size", "120x720+0", "gray:"+os.path.join(source_path, filename_depth_8), prefix+"_depth_8.png"])
    call(["convert", "-depth", "8", "-size", "120x720+0", "gray:"+os.path.join(source_path, filename_amplitude_8), prefix+"_amplitude_8.png"])

root = Tk()
if (len(sys.argv) < 2):
    if (version_3):
        source_path = filedialog.askdirectory(initialdir = os.getcwd(),title = "Select directory")
    else:
        source_path = tkFileDialog.askdirectory()
else:
    source_path = str(sys.argv[1])
    print source_path

sorted_raw_file_list = []
sorted_raw_file_list += [each for each in os.listdir(source_path) if fnmatch.fnmatchcase(each, 'image??????.raw')]
sorted_raw_file_list.sort()

first_file = sorted_raw_file_list[0]
first_file_number = first_file.split('image')
first_file_number = int(first_file_number[1].rsplit('.')[0])

expected_file_number = first_file_number

test_passed = True

for index, filename in enumerate(sorted_raw_file_list):

    # verify incrementing frame logging
    file_number = filename.split('image')
    file_number = int(file_number[1].rsplit('.')[0])
    if (file_number != expected_file_number):
        print("test failed: expected file number {0}, observed {1}".format(expected_file_number, file_number))
        test_passed = False
        break

    try:
        create_pngs(source_path, filename)
    except:
        print("test failed: exception at index {0} file_number {1}".format(index, file_number))
        test_passed = False
        break

    print("index {0} file_number {1}".format(index, file_number))
    expected_file_number = expected_file_number + 1

if (test_passed == True):
    print("test passed")
