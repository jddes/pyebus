import os
with os.add_dll_directory('C:\\Program Files\\Common Files\\Pleora\\eBUS SDK'):
    from .pyebus import *
    # import pyebus as ebus