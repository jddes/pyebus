import sys
import os
import time

import numpy as np
import cv2

with os.add_dll_directory('C:\\Program Files\\Common Files\\Pleora\\eBUS SDK'):
    import pyebus as ebus

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

device_connection_id = ""
print("ebus.getInterfaceCount() = ", ebus.getInterfaceCount())
ifcount = ebus.getInterfaceCount()
for if_id in range(ifcount):
    if_name = ebus.getInterfaceDisplayID(if_id)
    print("if_name = ", if_name)
    print("ebus.getDeviceCount() = ", ebus.getDeviceCount(if_id))
    for dev_id in range(ebus.getDeviceCount(if_id)):
        device_connection_id = ebus.getDeviceConnectionID(if_id, dev_id)
        print("device_connection_id = ", device_connection_id)

# device_connection_id = "random_string"
ebus.connectToDevice(device_connection_id)
ebus.openStream(device_connection_id)
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
    t2 = time.perf_counter()
    info = expand_img_info_tuple(img_info)
    # print(info)
    # TODO: processing goes here
    # hint: map the image into a numpy array by doing np.frombuffer(img_buffer, np.uint16)
    img_np = np.frombuffer(img_buffer, np.uint16)
    img_np = img_np.reshape(info['Height'], info['Width'])
    t3 = time.perf_counter()
    cv2.imshow('Press any key to stop', img_np)
    if cv2.pollKey() != -1:
        break
    t4 = time.perf_counter()
    print("kImage=%d, t(getImage)=%.3f ms, t(numpy)=%.3f ms, t(cv)=%.3f ms, min, max: %d, %d" %
        (kImage, (t2-t1)*1e3, (t3-t2)*1e3, (t4-t3)*1e3, np.min(img_np), np.max(img_np)))

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
