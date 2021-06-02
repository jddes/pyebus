# pyebus

pyebus is a basic Python wrapper around the eBUS SDK from Pleora Technologies.  It is a work-in-progress and not fully functional yet.
This repository: https://github.com/jddes/cameraprocess contains GUI code for displaying images and doing basic processing on them.

In order to use this, copy the "pyebus" directory to your Python installation's site-packages directory.  For WinPython, this is:
WPy64-3940\python-3.9.4.amd64\lib\site-packages

# Required dependencies

-The eBUS SDK from Pleora Technologies, composed of drivers and dlls installed in "C:\\Program Files\\Common Files\\Pleora\\eBUS SDK".    This path is currently hard-coded in __init__.py, which should ideally be changed.  It might be sufficient to install their free "eBUS player" only, which might install all the required drivers and API even without the SDK
-WinPython64-3.9.4.0  
-Opencv: "pip install opencv-python"
