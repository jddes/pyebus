import sys
import os
import time

import numpy as np
import cv2

with os.add_dll_directory('C:\\Program Files\\Common Files\\Pleora\\eBUS SDK'):
    import pyebustest as ebus

def expand_img_info_tuple(img_info):
    """ Unpack the img_info tuple into a dictionary.
    See pyebus_main.cpp::getPvImageInfo() for the list of fields """
    keys = [
                "Width",
                "Height",
                "BitsPerPixel",
                "RequiredSize",
                "ImageSize",
                "EffectiveImageSize",
                "OffsetX",
                "OffsetY",
                "PaddingX",
                "PaddingY",
                "PixelSize",
                "BitsPerComponent",
                "IsPixelColor",
                "IsPixelHighRes",
                "IsPartialLineMissing",
                "IsFullLineMissing",
                "IsEOFByLineCount",
                "IsInterlacedEven",
                "IsInterlacedOdd",
                "IsImageDropped",
                "IsDataOverrun",]
    return {k: v for k, v in zip(keys, img_info)}

ebus.useMock()

device_unique_id = ""
print("ebus.getInterfaceCount() = ", ebus.getInterfaceCount())
ifcount = ebus.getInterfaceCount()
for if_id in range(ifcount):
    if_name = ebus.getInterfaceDisplayID(if_id)
    print("if_name = ", if_name)
    print("ebus.getDeviceCount() = ", ebus.getDeviceCount(if_id))
    for dev_id in range(ebus.getDeviceCount(if_id)):
        device_unique_id = ebus.getDeviceUniqueID(if_id, dev_id)
        print("device_unique_id = ", device_unique_id)

# device_unique_id = "random_string"
ebus.connectToDevice(device_unique_id)
ebus.openStream(device_unique_id)
(buffer_size, buffer_count) = ebus.getBufferRequirements()
# allocate "buffer_count" buffers of size "buffer_size"!
buffers = []
for k in range(buffer_count):
    buffers.append(bytearray(buffer_size))
    ebus.addBuffer(buffers[-1])


ebus.startAcquisition()


##################### Start image processing loop #####################
timeoutMS = 1000
kImage = 0
img_buffer = "test"

while 1:
    t1 = time.perf_counter()
    kImage += 1
    (img_buffer, img_info) = ebus.getImage(timeoutMS)
    # print(img_info)
    info = expand_img_info_tuple(img_info)
    # print(info)
    t2 = time.perf_counter()
    # TODO: processing goes here
    # hint: map the image into a numpy array by doing np.frombuffer(img_buffer, np.uint16)
    img_np = np.frombuffer(img_buffer, np.uint16)
    img_np = img_np.reshape(info['Height'], info['Width'])
    print("kImage=%d, t=%.3f ms, min, max: %d, %d" % (kImage, (t2-t1)*1e3, np.min(img_np), np.max(img_np)))
    cv2.imshow('Press any key to stop', img_np)
    if cv2.pollKey() != -1:
        break

    ebus.releaseImage()
##################### End   image processing loop #####################

# ebus.openDeviceSerialPort()
ebus.writeSerialPort('test string\n')
result = ebus.readSerialPort(10, 500)
print(result)
ebus.closeDeviceSerialPort();


ebus.stopAcquisition()
ebus.closeStream()
ebus.releaseBuffers()
ebus.closeDevice()
