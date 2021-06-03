#include <Python.h>
#include <Windows.h>

#include <iostream>
#include <list>
#include <math.h>
#include <stdlib.h>     /* srand, rand */

using namespace std;

#include <PvTypes.h>
#include <PvDevice.h>
#include <PvSystem.h>
#include <PvDeviceAdapter.h>
#include <PvGenParameterArray.h>
#include <PvDeviceSerialPort.h>
#include <PvStream.h>
#include <PvBuffer.h>
#include <PvImage.h>

PyObject* useMock(              PyObject* self);
PyObject* useRealDevice(        PyObject* self);
PyObject* getInterfaceCount(    PyObject* self);

PyObject* findDevices(          PyObject* self, PyObject* timeoutMS);
PyObject* getDeviceCount(       PyObject* self, PyObject* oInterfaceNumber);
PyObject* getInterfaceDisplayID(PyObject* self, PyObject* oInterfaceNumber);
PyObject* getDeviceConnectionID(    PyObject* self, PyObject *args);
PyObject* connectToDevice(      PyObject* self, PyObject *args);
PyObject* closeDevice(          PyObject* self);
PyObject* setDeviceBooleanValue(PyObject* self, PyObject *args);
PyObject* setDeviceEnumValue(   PyObject* self, PyObject *args);
PyObject* setDeviceFloatValue(  PyObject* self, PyObject *args);
PyObject* setDeviceIntegerValue(PyObject* self, PyObject *args);
PyObject* setDeviceStringValue( PyObject* self, PyObject *args);

PyObject* openDeviceSerialPort(PyObject* self);
PyObject* closeDeviceSerialPort(PyObject* self);
PyObject* writeSerialPort(PyObject* self, PyObject* arg);
PyObject* readSerialPort(PyObject* self, PyObject* args);

PyObject* openStream(PyObject* self, PyObject* args);
PyObject* closeStream(PyObject* self);
PyObject* getBufferRequirements(PyObject* self);
PyObject* addBuffer(PyObject* self, PyObject* arg);
PyObject* releaseBuffers(PyObject* self);
PyObject* startAcquisition(PyObject* self);
PyObject* stopAcquisition(PyObject* self);
PyObject* getImage(PyObject* self, PyObject* timeoutMS);
PyObject* releaseImage(PyObject* self);

PyObject* getPvImageInfo(PvImage & img);
PyObject* getMockPvImageInfo();

void printPvResultError(PvResult & lResult);

static PyMethodDef pyebus_methods[] = {
    // The first property is the name exposed to Python, fast_tanh
    // The second is the C++ function with the implementation
    // METH_O means it takes a single PyObject argument

    { "useMock",               (PyCFunction)useMock,               METH_NOARGS,  nullptr },
    { "useRealDevice",         (PyCFunction)useRealDevice,         METH_NOARGS,  nullptr },

    { "getInterfaceCount",     (PyCFunction)getInterfaceCount,     METH_NOARGS,  nullptr },
    { "findDevices",           (PyCFunction)findDevices,           METH_O,       nullptr },
    { "getDeviceCount",        (PyCFunction)getDeviceCount,        METH_O,       nullptr },
    { "getInterfaceDisplayID", (PyCFunction)getInterfaceDisplayID, METH_O,       nullptr },
    { "getDeviceConnectionID", (PyCFunction)getDeviceConnectionID, METH_VARARGS, nullptr },
    { "connectToDevice",       (PyCFunction)connectToDevice,       METH_VARARGS, nullptr },
    { "closeDevice",           (PyCFunction)closeDevice,           METH_NOARGS,  nullptr },

    { "setDeviceBooleanValue", (PyCFunction)setDeviceBooleanValue, METH_VARARGS,  nullptr },
    { "setDeviceEnumValue",    (PyCFunction)setDeviceEnumValue,    METH_VARARGS,  nullptr },
    { "setDeviceFloatValue",   (PyCFunction)setDeviceFloatValue,   METH_VARARGS,  nullptr },
    { "setDeviceIntegerValue", (PyCFunction)setDeviceIntegerValue, METH_VARARGS,  nullptr },
    { "setDeviceStringValue",  (PyCFunction)setDeviceStringValue,  METH_VARARGS,  nullptr },

    { "openDeviceSerialPort",  (PyCFunction)openDeviceSerialPort,  METH_NOARGS,  nullptr },
    { "closeDeviceSerialPort", (PyCFunction)closeDeviceSerialPort, METH_NOARGS,  nullptr },
    { "writeSerialPort",       (PyCFunction)writeSerialPort,       METH_O,       nullptr },
    { "readSerialPort",        (PyCFunction)readSerialPort,        METH_VARARGS, nullptr },

    { "openStream",            (PyCFunction)openStream,            METH_VARARGS, nullptr },
    { "closeStream",           (PyCFunction)closeStream,           METH_NOARGS,  nullptr },
    { "getBufferRequirements", (PyCFunction)getBufferRequirements, METH_NOARGS,  nullptr },
    { "addBuffer",             (PyCFunction)addBuffer,             METH_O,       nullptr },
    { "releaseBuffers",        (PyCFunction)releaseBuffers,        METH_NOARGS,  nullptr },
    { "startAcquisition",      (PyCFunction)startAcquisition,      METH_NOARGS,  nullptr },
    { "stopAcquisition",       (PyCFunction)stopAcquisition,       METH_NOARGS,  nullptr },
    { "getImage",              (PyCFunction)getImage,              METH_O,       nullptr },
    { "releaseImage",          (PyCFunction)releaseImage,          METH_NOARGS,  nullptr },


    // Terminate the array with an object containing nulls.
    { nullptr, nullptr, 0, nullptr }
};

static PyModuleDef pyebus_module = {
    PyModuleDef_HEAD_INIT,
    "pyebus",                        // Module name to use with Python import statements
    "Wrapper for the eBUS C++ SDK from Pleora Technologies",  // Module description
    0,
    pyebus_methods                   // Structure that defines the methods of the module
};

PyMODINIT_FUNC PyInit_pyebus() {
    printf("hello from PyInit_pyebus()!\n");
    return PyModule_Create(&pyebus_module);
}

///////////////////////////////////////////////
// Pv-specific stuff starts here
///////////////////////////////////////////////

PvSystem pvSystem;
PvDevice * lDevice = NULL;
PvStream *lStream = NULL;
PvGenParameterArray *lParams = NULL;

typedef std::list<Py_buffer *> pythonBufferListType;
typedef std::list<PvBuffer *> ebusBufferListType;
pythonBufferListType pythonBufferList;
ebusBufferListType ebusBufferList;
PvBuffer *lastBuffer = NULL;

PvDeviceAdapter *lDeviceAdapter = NULL;
PvDeviceSerialPort lPort;

#define SPEED ( "Baud57600" )
#define STOPBITS ( "One" )
#define PARITY ( "None" )
#define RX_BUFFER_SIZE ( 2<<20 )
uint8_t serial_rx_buffer[RX_BUFFER_SIZE];
uint32_t rx_available_for_mock = 0;


bool use_mock=false;
#define MOCK_DEVICE_GUID "mock_device_guid"
#define MOCK_IMG_WIDTH 640
#define MOCK_IMG_HEIGHT 512
#define MOCK_BYTES_PER_PIXEL 2

pythonBufferListType::iterator itPythonMock;

PyObject* useMock(PyObject* self)
{
    use_mock = true;
    Py_RETURN_NONE;
}

PyObject* useRealDevice(PyObject* self)
{
    use_mock = false;
    Py_RETURN_NONE;
}

PyObject* getInterfaceCount(PyObject* self) {
    return PyLong_FromSize_t(pvSystem.GetInterfaceCount());
}

PyObject* findDevices(PyObject* self, PyObject* timeoutMS) {
    uint32_t iIimeoutMS = PyLong_AsUnsignedLong(timeoutMS);
    pvSystem.SetDetectionTimeout(iIimeoutMS);
    pvSystem.Find();
    Py_RETURN_NONE;
}

PyObject* getDeviceCount(PyObject* self, PyObject* oInterfaceNumber) {
    uint32_t interface_number = PyLong_AsLong(oInterfaceNumber);
    printf("interface_number = %d\n", interface_number);
    const PvInterface *lInterface = dynamic_cast<const PvInterface *>( pvSystem.GetInterface( interface_number ) );
    if (lInterface == NULL)
        return NULL;
    printf("lInterface->GetDeviceCount() = %d\n", lInterface->GetDeviceCount());
    if (use_mock)
        if (interface_number == pvSystem.GetInterfaceCount()-1)
            return PyLong_FromSize_t(1);
        else
            return PyLong_FromSize_t(0);
    else
        return PyLong_FromSize_t(lInterface->GetDeviceCount());
}

PyObject* getInterfaceDisplayID(PyObject* self, PyObject* oInterfaceNumber) {
    PyObject* retval = PyLong_FromLong(-1L);
    uint32_t interface_number = PyLong_AsLong(oInterfaceNumber);
    const PvInterface *lInterface = dynamic_cast<const PvInterface *>( pvSystem.GetInterface( interface_number ) );
    if (lInterface == NULL)
        return NULL;

    PvString pvs = lInterface->GetDisplayID();
    return PyUnicode_FromStringAndSize(pvs.GetAscii(), pvs.GetLength());
}

PyObject* getDeviceConnectionID(PyObject* self, PyObject *args) {
    uint32_t interface_number, device_number;
    if (!PyArg_ParseTuple(args, "ii", &interface_number, &device_number))
        return NULL;
    const PvInterface *lInterface = dynamic_cast<const PvInterface *>( pvSystem.GetInterface( interface_number ) );
    if (lInterface == NULL)
        return NULL;
    if (use_mock)
    {
        return PyUnicode_FromString(MOCK_DEVICE_GUID);
    } else {
        const PvDeviceInfo *lDI = dynamic_cast<const PvDeviceInfo *>( lInterface->GetDeviceInfo( device_number ) );
        if (lDI == NULL)
            return NULL;

        PvString pvs = lDI->GetConnectionID();
        return PyUnicode_FromStringAndSize(pvs.GetAscii(), pvs.GetLength());
    }
}

PyObject* connectToDevice(PyObject* self, PyObject *args)
{
    char * unique_id;
    if (!PyArg_ParseTuple(args, "s", &unique_id))
        return NULL;
    PvString pvs(unique_id);

    PvResult lResult;
    if (!use_mock)
    {
        lDevice = PvDevice::CreateAndConnect( pvs, &lResult );
        lParams = lDevice->GetParameters();
    } else {
        if (pvs == MOCK_DEVICE_GUID)
            lResult.SetCode(0);
        else
            lResult.SetCode(0x0019);
    }
    if ( !lResult.IsOK() )
    {
        cout << "Unable to connect to " << pvs.GetAscii() << ": " << (const char*) lResult.GetDescription() << "." << endl;
        printPvResultError(lResult);
        return NULL;
    }

    Py_RETURN_NONE;
}

PyObject* closeDevice(PyObject* self)
{
    if (use_mock)
        Py_RETURN_NONE;
    if ( lDevice == NULL )
    {
        return NULL;
    }

    cout << "Disconnecting device" << endl;
    lDevice->Disconnect();
    PvDevice::Free( lDevice );
    Py_RETURN_NONE;
}

void printPvResultError(PvResult & lResult)
{
    cout << "-----------------------------------------------" << endl;
    cout << "PvResult error: code " << lResult.GetCode() << ", " << lResult.GetCodeString() << endl;
    cout << "Description: " << lResult.GetDescription() << endl;
    cout << "Internal code: " << lResult.GetInternalCode() << ", OS code: " << lResult.GetOSCode() << endl;
    cout << "As string: " << ": " << lResult << "." << endl;
    cout << "-----------------------------------------------" << endl;
}


    // lParams->SetEnumValue( "Height", "512" );
    // lParams->SetEnumValue( "PixelFormat", "Mono12" );
    // lParams->SetEnumValue( "TestPattern", "Off" );
    // lParams->SetEnumValue( "PixelBusDataValidEnabled", "1" );


PyObject* setDeviceBooleanValue(PyObject* self, PyObject *args)
{
    char * setting;
    int value;
    if (!PyArg_ParseTuple(args, "sp", &setting, &value))
        return NULL;

    lParams->SetBooleanValue( setting, (bool) value );
    Py_RETURN_NONE;
}

PyObject* setDeviceEnumValue(PyObject* self, PyObject *args)
{
    char * setting;
    char * value;
    if (!PyArg_ParseTuple(args, "ss", &setting, &value))
        return NULL;

    lParams->SetEnumValue( setting, value );
    Py_RETURN_NONE;
}

PyObject* setDeviceFloatValue(PyObject* self, PyObject *args)
{
    char * setting;
    double value;
    if (!PyArg_ParseTuple(args, "sd", &setting, &value))
        return NULL;

    lParams->SetFloatValue( setting, value );
    Py_RETURN_NONE;
}

PyObject* setDeviceIntegerValue(PyObject* self, PyObject *args)
{
    char * setting;
    int64_t value;
    if (!PyArg_ParseTuple(args, "sl", &setting, &value))
        return NULL;

    lParams->SetIntegerValue( setting, value );
    Py_RETURN_NONE;
}

PyObject* setDeviceStringValue(PyObject* self, PyObject *args)
{
    char * setting;
    char * value;
    if (!PyArg_ParseTuple(args, "ss", &setting, &value))
        return NULL;

    lParams->SetEnumValue( setting, value );
    Py_RETURN_NONE;
}

// this follows "Pleora Technologies Inc\eBUS SDK\Samples\DeviceSerialPort\DeviceSerialPort.cpp"
PyObject* openDeviceSerialPort(PyObject* self)
{
    if (use_mock)
    {
        cout << "Mock serial port opened" << endl;
        Py_RETURN_NONE;
    }

    lDeviceAdapter = new PvDeviceAdapter( lDevice );

    lParams->SetEnumValue( "BulkSelector", "Bulk0" );
    lParams->SetEnumValue( "BulkMode", "UART" );
    lParams->SetEnumValue( "BulkBaudRate", SPEED );
    lParams->SetEnumValue( "BulkNumOfStopBits", STOPBITS );
    lParams->SetEnumValue( "BulkParity", PARITY );

    // For this test to work without attached serial hardware we enable the port loop back
    // lParams->SetBooleanValue( "BulkLoopback", true );

    // Open serial port
    PvResult lResult;
    lResult = lPort.Open( lDeviceAdapter, PvDeviceSerialBulk0 );
    if ( !lResult.IsOK() )
    {
        cout << "Unable to open serial port on device: " << endl;
        printPvResultError(lResult);
        return NULL;
    }
    cout << "Serial port opened" << endl;

    // Make sure the PvDeviceSerialPort receive queue is big enough
    lPort.SetRxBufferSize( RX_BUFFER_SIZE );

    Py_RETURN_NONE;
}

PyObject* closeDeviceSerialPort(PyObject* self)
{
    if (use_mock)
        Py_RETURN_NONE;
    // Close serial port
    lPort.Close();
    cout << "Serial port closed" << endl;

    // Delete device adapter (before freeing PvDevice!)
    delete lDeviceAdapter;
    lDeviceAdapter = NULL;

    Py_RETURN_NONE;
}

PyObject* writeSerialPort(PyObject* self, PyObject* arg)
{
    const uint8_t* buf;
    Py_ssize_t size;
    buf = reinterpret_cast<const uint8_t*>(PyUnicode_AsUTF8AndSize(arg, &size));
    if (buf == NULL)
        return NULL;

    uint32_t lBytesWritten = 0;
    if (!use_mock)
    {
        PvResult lResult;
        Py_BEGIN_ALLOW_THREADS
        lResult = lPort.Write( buf, size, lBytesWritten );
        Py_END_ALLOW_THREADS
        //printf("Sent %d/%d bytes: ", size, lBytesWritten);
        //for (uint32_t k = 0; k < size; k++)
        //{
        //    
        //    printf("0x%0x, ", buf[k]);
        //}
        //printf("\n");
        if ( !lResult.IsOK() )
        {
            // Unable to send data over serial port!
            cout << "Error sending data over the serial port: " << lResult.GetCodeString().GetAscii() << " " <<  lResult.GetDescription().GetAscii() << endl;
            return NULL;
        }
    } else {
        // Mock implementation: just a loopback that also prints
        //printf("Mock: Will send %d bytes.'", (int)size);
        for (uint32_t k=0; k<size; k++)
        {
            serial_rx_buffer[k+rx_available_for_mock] = buf[k];
        }
        lBytesWritten = size;
        rx_available_for_mock += lBytesWritten;
    }

    cout << "Sent " << lBytesWritten << " bytes through the serial port" << endl;
    Py_RETURN_NONE;
}

PyObject* readSerialPort(PyObject* self, PyObject* args)
{
    PyObject* retval;
    int32_t lSize;
    int32_t lTimeoutMS;
    if (!PyArg_ParseTuple(args, "ii", &lSize, &lTimeoutMS))
        return NULL;

    if (lSize > RX_BUFFER_SIZE)
    {
        printf("Error: attempting to read %d bytes, but read buffer is only %d bytes long\n", lSize, RX_BUFFER_SIZE);
        return NULL;
    }

    uint32_t lTotalBytesRead = 0;
    if (!use_mock)
    {
        PvResult lResult;
        while ( lTotalBytesRead < lSize )
        {
            uint32_t lBytesRead = 0;
            Py_BEGIN_ALLOW_THREADS
            lResult = lPort.Read( serial_rx_buffer + lTotalBytesRead, lSize - lTotalBytesRead, lBytesRead, lTimeoutMS );
            Py_END_ALLOW_THREADS
            if ( lResult.GetCode() == PvResult::Code::TIMEOUT )
            {
                //cout << "Serial Read Timeout" << endl;
                break; // this actually happens frequently in normal operation: this is how we implement polling of the data
            }

            // Increments read head
            lTotalBytesRead += lBytesRead;
        }
        retval = PyUnicode_FromStringAndSize(reinterpret_cast<const char*>(serial_rx_buffer), lTotalBytesRead);
    } else {
        // Mock implementation
        if (lSize <= rx_available_for_mock)
        {
            lTotalBytesRead = lSize;
        }
        else {
            lTotalBytesRead = rx_available_for_mock;
        }
        
        retval = PyUnicode_FromStringAndSize(reinterpret_cast<const char*>(serial_rx_buffer), lTotalBytesRead);
        rx_available_for_mock -= lTotalBytesRead;
    }

    // cout << "Received " << lTotalBytesRead << " bytes through the serial port" << endl;
    return retval;
}

PyObject* openStream(PyObject* self, PyObject *args)
{
    char * unique_id;
    if (!PyArg_ParseTuple(args, "s", &unique_id))
        return NULL;

    PvString pvs(unique_id);

    PvResult lResult;
    if (!use_mock)
    {
        lStream = PvStream::CreateAndOpen( pvs, &lResult );
        if ( lStream == NULL )
        {
            printPvResultError(lResult);
            return NULL;
        }
    } else {
        if (pvs != MOCK_DEVICE_GUID)
            return NULL;
    }
    Py_RETURN_NONE;
}

PyObject* closeStream(PyObject* self)
{
    if (use_mock)
        Py_RETURN_NONE;
    if ( lStream == NULL )
        return NULL;

    lStream->Close();
    PvStream::Free( lStream );
    Py_RETURN_NONE;
}

///////////////////////////
PyObject* getBufferRequirements(PyObject* self)
{
    // Reading payload size and count from device
    uint32_t lSize, lBufferCount;
    if (!use_mock)
    {
        lSize        = lDevice->GetPayloadSize();
        lBufferCount = lStream->GetQueuedBufferMaximum();
    } else {
        lSize = MOCK_IMG_WIDTH * MOCK_IMG_HEIGHT * MOCK_BYTES_PER_PIXEL;
        lBufferCount = 16;
    }
    return Py_BuildValue("II", lSize, lBufferCount);
}

// I don't really see a way around handling both a list of Py_buffer and PvBuffer.
// The actual large chunk of memory is not duplicated this way, only their descriptors, so it's not too bad.
PyObject* addBuffer(PyObject* self, PyObject* arg)
{
    Py_buffer * pythonBuffer = new Py_buffer;
    PvBuffer * ebusBuffer    = new PvBuffer;
    int result = PyObject_GetBuffer(arg, pythonBuffer, PyBUF_CONTIG);
    pythonBufferList.push_back( pythonBuffer );
    ebusBufferList.push_back( ebusBuffer );

    ebusBuffer->Attach( pythonBuffer->buf, static_cast<uint32_t>(pythonBuffer->len) );
    if (!use_mock)
        lStream->QueueBuffer( ebusBuffer );
    else
        itPythonMock = pythonBufferList.begin(); // needed for faking images
    Py_RETURN_NONE;
}

PyObject* releaseBuffers(PyObject* self)
{
    ebusBufferListType::iterator itEbus = ebusBufferList.begin();
    while ( itEbus != ebusBufferList.end() )
    {
        delete *itEbus ;
        itEbus++;
    }

    pythonBufferListType::iterator itPython = pythonBufferList.begin();
    while ( itPython != pythonBufferList.end() )
    {
        PyBuffer_Release(*itPython);
        delete *itPython;
        itPython++;
    }

    Py_RETURN_NONE;
}

PyObject* startAcquisition(PyObject* self)
{
    if (use_mock)
        Py_RETURN_NONE;
    // Tell the device to start sending images.
    PvGenParameterArray *lDeviceParams = lDevice->GetParameters();
    PvGenCommand *lStart = dynamic_cast<PvGenCommand *>( lDeviceParams->Get( "AcquisitionStart" ) );

    cout << "Enabling streaming and sending AcquisitionStart command." << endl;
    lDevice->StreamEnable();
    lStart->Execute();

    Py_RETURN_NONE;
}

PyObject* stopAcquisition(PyObject* self)
{
    if (use_mock)
        Py_RETURN_NONE;
    // Tell the device to stop sending images.
    PvGenParameterArray *lDeviceParams = lDevice->GetParameters();
    PvGenCommand *lStop = dynamic_cast<PvGenCommand *>( lDeviceParams->Get( "AcquisitionStop" ) );
    cout << "Sending AcquisitionStop command to the device" << endl;
    lStop->Execute();

    // Disable streaming on the device
    cout << "Disable streaming on the controller." << endl;
    lDevice->StreamDisable();

    // Abort all buffers from the stream and dequeue
    cout << "Aborting buffers still in stream" << endl;
    lStream->AbortQueuedBuffers();
    while ( lStream->GetQueuedBufferCount() > 0 )
    {
        PvBuffer *lBuffer = NULL;
        PvResult lOperationResult;

        lStream->RetrieveBuffer( &lBuffer, &lOperationResult );
    }
    Py_RETURN_NONE;
}

// Retrieve next buffer
// The Python code must call releaseImage() after each call to getImage, once the processing is done so that the buffer is made available to the camera driver again
PyObject* getImage(PyObject* self, PyObject* timeoutMS)
{
    uint32_t ltimeoutMS = PyLong_AsLong(timeoutMS);
    PvResult lResult, lOperationResult;

    PyObject *view, *info;
    if (!use_mock)
    {
        Py_BEGIN_ALLOW_THREADS
        lResult = lStream->RetrieveBuffer( &lastBuffer, &lOperationResult, ltimeoutMS );
        if ( !(lResult.IsOK() && lOperationResult.IsOK()) )
        {
            return NULL;
        }

        PvPayloadType lType;
        lType = lastBuffer->GetPayloadType();
        if ( lType == !PvPayloadTypeImage )
        {
            cout << " (buffer does not contain image)";
            return NULL;
        }
        Py_END_ALLOW_THREADS
        // Get image specific buffer interface.
        PvImage *lImage = lastBuffer->GetImage();
        
        if (lImage == NULL)
        {
            cout << "lImage is null" << endl;
            return NULL;
        }
        view = PyMemoryView_FromMemory(
                    reinterpret_cast<char *>(lImage->GetDataPointer()),
                    lImage->GetRequiredSize(),
                    PyBUF_READ);
        info = getPvImageInfo(*lImage);
    } else {
        // Mock implementation: generate a random image + gaussian beam at a fixed position
        Py_buffer * pythonBuffer = *itPythonMock;
        uint16_t * raw_buffer = reinterpret_cast<uint16_t *>(pythonBuffer->buf);
        // not 100% sure that this is totally kosher, since we are writing to a python buffer,
        // but I think that since we change just the contents, and not the size, etc, it's probably fine.
        // also, this is just for our mock, so it's all good
        Py_BEGIN_ALLOW_THREADS
        for (uint32_t y=0; y<MOCK_IMG_HEIGHT; y++)
        {
            for (uint32_t x=0; x<MOCK_IMG_WIDTH; x++)
            {
                // (rand()% 1000) takes 8 ms for a 640x512 image
                // exp() takes 10 ms for a 640x512 image
                // pow()+pow() takes 10 ms for a 640x512 image
                // total is 28 ms for the whole thing...
                raw_buffer[y*MOCK_IMG_WIDTH + x] = (rand() % 20000)
                    + static_cast<uint16_t>(30000.*exp(
                            -(pow((x-100.), 2.0) + pow((y-50.), 2.0))
                                       /2./(10.*10.) )
                                           );
            }
        }
        Py_END_ALLOW_THREADS

        // change to the next buffer in the list (wrapping around when needed)
        if (++itPythonMock == pythonBufferList.end())
        {
            itPythonMock = pythonBufferList.begin();
        }

        view = PyMemoryView_FromMemory(
                    reinterpret_cast<char *>(pythonBuffer->buf),
                    pythonBuffer->len,
                    PyBUF_READ);
        info = getMockPvImageInfo();
    }
    return Py_BuildValue("(NN)", view, info);
}

// this must be called by the Python code once the processing is done so that the buffer is made available to the camera driver again
PyObject* releaseImage(PyObject* self)
{
    if (use_mock)
        Py_RETURN_NONE;
    lStream->QueueBuffer( lastBuffer );
    Py_RETURN_NONE;
}

// Converts all the metadata stored in PvImage to a tuple of Python objects (all ints or bools)
PyObject* getPvImageInfo(PvImage & img)
{
    PvPixelType pixelType = img.GetPixelType();
    return Py_BuildValue("(IIIIIIIIHHIIIIIIIIIII)",
                img.GetWidth(),
                img.GetHeight(),
                img.GetBitsPerPixel(),
                img.GetRequiredSize(),
                img.GetImageSize(),
                img.GetEffectiveImageSize(),
                img.GetOffsetX(),
                img.GetOffsetY(),
                img.GetPaddingX(),
                img.GetPaddingY(),
                PvImage::GetPixelSize(pixelType),
                PvImage::GetBitsPerComponent(pixelType),
                PvImage::IsPixelColor(pixelType),
                PvImage::IsPixelHighRes(pixelType),
                img.IsPartialLineMissing(),
                img.IsFullLineMissing(),
                img.IsEOFByLineCount(),
                img.IsInterlacedEven(),
                img.IsInterlacedOdd(),
                img.IsImageDropped(),
                img.IsDataOverrun()
        );
}

// return sensible values for our mock images
PyObject* getMockPvImageInfo()
{
    return Py_BuildValue("(IIIIIIIIHHIIIIIIIIIII)",
                MOCK_IMG_WIDTH,
                MOCK_IMG_HEIGHT,
                MOCK_BYTES_PER_PIXEL*8,
                MOCK_IMG_WIDTH*MOCK_IMG_HEIGHT,
                MOCK_IMG_WIDTH*MOCK_IMG_HEIGHT,
                MOCK_IMG_WIDTH*MOCK_IMG_HEIGHT,
                0,
                0,
                0,
                0,
                MOCK_BYTES_PER_PIXEL*8,
                MOCK_BYTES_PER_PIXEL*8,
                true,
                true,
                false,
                false,
                false,
                false,
                false,
                false,
                false
        );
}
