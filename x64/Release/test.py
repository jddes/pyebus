import sys
import os
# print(sys.argv)
# print(os.path.dirname(sys.argv[0]))
# import ctypes
# d = ctypes.cdll.LoadLibrary('Second_DLL_test.py')
# d = ctypes.cdll.LoadLibrary('I:\\Projects\\Second_DLL_test\\x64\\Release\\Second_DLL_test.dll')
# d = ctypes.cdll.LoadLibrary('\\Second_DLL_test.py')

with os.add_dll_directory('C:\\Program Files\\Common Files\\Pleora\\eBUS SDK'):
    import pyebustest as ebus

print("ebus.getInterfaceCount() = ", ebus.getInterfaceCount())
ifcount = ebus.getInterfaceCount()
for if_id in range(ifcount):
    if_name = ebus.getInterfaceDisplayID(if_id)
    print("if_name = ", if_name)
    print("ebus.getDeviceCount() = ", ebus.getDeviceCount(if_id))

# ebus.connectToDevice("random_string")

ebus.writeSerialPort('test string\n')
result = ebus.readSerialPort(10, 500)
print(result)
