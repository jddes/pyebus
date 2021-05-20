#include <Python.h>
#include <Windows.h>

#include <iostream>

using namespace std;

#include <PvTypes.h>
#include <PvDevice.h>
#include <PvSystem.h>
#include <PvDeviceAdapter.h>
#include <PvGenParameterArray.h>
#include <PvDeviceSerialPort.h>

PyObject* getInterfaceCount(    PyObject* self);
PyObject* getDeviceCount(       PyObject* self, PyObject* oInterfaceNumber);
PyObject* getInterfaceDisplayID(PyObject* self, PyObject* oInterfaceNumber);
PyObject* getDeviceUniqueID(    PyObject* self, PyObject *args);
PyObject* connectToDevice(      PyObject* self, PyObject *args);

PyObject* openDeviceSerialPort(PyObject* self);
PyObject* writeSerialPort(PyObject* self, PyObject* arg);
PyObject* readSerialPort(PyObject* self, PyObject* args);

#define MOCK_SERIAL_PORT
void printPvResultError(PvResult & lResult);

static PyMethodDef pyebustest_methods[] = {
    // The first property is the name exposed to Python, fast_tanh
    // The second is the C++ function with the implementation
    // METH_O means it takes a single PyObject argument

    { "getInterfaceCount",     (PyCFunction)getInterfaceCount,     METH_NOARGS,  nullptr },
    { "getDeviceCount",        (PyCFunction)getDeviceCount,        METH_O,       nullptr },
    { "getInterfaceDisplayID", (PyCFunction)getInterfaceDisplayID, METH_O,       nullptr },
    { "getDeviceUniqueID",     (PyCFunction)getDeviceUniqueID,     METH_VARARGS, nullptr },
    { "connectToDevice",       (PyCFunction)connectToDevice,       METH_VARARGS, nullptr },

    { "openDeviceSerialPort",  (PyCFunction)openDeviceSerialPort,  METH_NOARGS,  nullptr },
    { "writeSerialPort",       (PyCFunction)writeSerialPort,       METH_O,       nullptr },
    { "readSerialPort",        (PyCFunction)readSerialPort,        METH_VARARGS, nullptr },

    // Terminate the array with an object containing nulls.
    { nullptr, nullptr, 0, nullptr }
};

static PyModuleDef pyebustest_module = {
    PyModuleDef_HEAD_INIT,
    "pyebustest",                        // Module name to use with Python import statements
    "Provides some functions, but faster",  // Module description
    0,
    pyebustest_methods                   // Structure that defines the methods of the module
};

PyMODINIT_FUNC PyInit_pyebustest() {
    printf("hello from PyInit_pyebustest()!\n");
    return PyModule_Create(&pyebustest_module);
}

///////////////////////////////////////////////
// Pv-specific stuff starts here
///////////////////////////////////////////////

PvSystem pvSystem;

PyObject* getInterfaceCount(PyObject* self) {
    return PyLong_FromSize_t(pvSystem.GetInterfaceCount());
}

PyObject* getDeviceCount(PyObject* self, PyObject* oInterfaceNumber) {
    uint32_t interface_number = PyLong_AsLong(oInterfaceNumber);
    const PvInterface *lInterface = dynamic_cast<const PvInterface *>( pvSystem.GetInterface( interface_number ) );
    if (lInterface == NULL)
        return NULL;

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

PyObject* getDeviceUniqueID(PyObject* self, PyObject *args) {
    uint32_t interface_number, device_number;
    if (!PyArg_ParseTuple(args, "ii", &interface_number, &device_number))
        return NULL;
    const PvInterface *lInterface = dynamic_cast<const PvInterface *>( pvSystem.GetInterface( interface_number ) );
    if (lInterface == NULL)
        return NULL;
    const PvDeviceInfo *lDI = dynamic_cast<const PvDeviceInfo *>( lInterface->GetDeviceInfo( device_number ) );
    if (lDI == NULL)
        return NULL;

    PvString pvs = lInterface->GetUniqueID();
    return PyUnicode_FromStringAndSize(pvs.GetAscii(), pvs.GetLength());
}

PvDevice * lDevice = NULL;

PyObject* connectToDevice(PyObject* self, PyObject *args)
{
    char * unique_id;
    if (!PyArg_ParseTuple(args, "s", &unique_id))
        return NULL;

    PvString pvs(unique_id);
    cout << "Connecting to " << pvs.GetAscii() << "." << endl;

    PvResult lResult;
    lDevice = PvDevice::CreateAndConnect( pvs, &lResult );
    lResult.SetCode(PV_NOT_INITIALIZED);
    if ( !lResult.IsOK() || lDevice == NULL )
    {
        cout << "Unable to connect to " << pvs.GetAscii() << ": " << (const char*) lResult.GetDescription() << "." << endl;
        printPvResultError(lResult);
        return NULL;
    }

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

PvDeviceAdapter *lDeviceAdapter = NULL;
PvGenParameterArray *lParams    = NULL;
PvDeviceSerialPort lPort;

#define SPEED ( "Baud9600" )
#define STOPBITS ( "One" )
#define PARITY ( "None" )
#define RX_BUFFER_SIZE ( 2<<20 )

uint8_t serial_rx_buffer[RX_BUFFER_SIZE];

// this follows "Pleora Technologies Inc\eBUS SDK\Samples\DeviceSerialPort\DeviceSerialPort.cpp"
PyObject* openDeviceSerialPort(PyObject* self)
{
    lDeviceAdapter = new PvDeviceAdapter( lDevice );
    lParams = lDevice->GetParameters();

    lParams->SetEnumValue( "BulkSelector", "Bulk0" );
    lParams->SetEnumValue( "BulkMode", "UART" );
    lParams->SetEnumValue( "BulkBaudRate", SPEED );
    lParams->SetEnumValue( "BulkNumOfStopBits", STOPBITS );
    lParams->SetEnumValue( "BulkParity", PARITY );

    // For this test to work without attached serial hardware we enable the port loop back
    lParams->SetBooleanValue( "BulkLoopback", true );

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

PyObject* writeSerialPort(PyObject* self, PyObject* arg)
{
    const uint8_t* buf;
    Py_ssize_t size;
    buf = reinterpret_cast<const uint8_t*>(PyUnicode_AsUTF8AndSize(arg, &size));
    if (buf == NULL)
        return NULL;

    uint32_t lBytesWritten = 0;
#ifndef MOCK_SERIAL_PORT
    PvResult lResult;
    lResult = lPort.Write( buf, size, lBytesWritten );
    if ( !lResult.IsOK() )
    {
        // Unable to send data over serial port!
        cout << "Error sending data over the serial port: " << lResult.GetCodeString().GetAscii() << " " <<  lResult.GetDescription().GetAscii() << endl;
        return NULL;
    }
#else
    printf("Will send %d bytes: '", (int)size);
    for (uint32_t k=0; k<size; k++)
    {
        printf("%d, ", buf[k]);
    }
    printf("'\n");
    lBytesWritten = size;
#endif // MOCK_SERIAL_PORT

    cout << "Sent " << lBytesWritten << " bytes through the serial port" << endl;
    Py_RETURN_NONE;
}

PyObject* readSerialPort(PyObject* self, PyObject* args)
{
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
#ifndef MOCK_SERIAL_PORT
    PvResult lResult;
    while ( lTotalBytesRead < lSize )
    {
        uint32_t lBytesRead = 0;
        lResult = lPort.Read( serial_rx_buffer + lTotalBytesRead, lSize - lTotalBytesRead, lBytesRead, lTimeoutMS );
        if ( lResult.GetCode() == PvResult::Code::TIMEOUT )
        {
            cout << "Serial Read Timeout" << endl;
            break;
        }

        // Increments read head
        lTotalBytesRead += lBytesRead;
    }
#else
    lTotalBytesRead = lSize;
    for (uint32_t k=0; k<lTotalBytesRead; k++)
    {
        serial_rx_buffer[k] = 10*k;
    }
#endif // MOCK_SERIAL_PORT

    cout << "Received " << lTotalBytesRead << " bytes through the serial port" << endl;
    return PyUnicode_FromStringAndSize(reinterpret_cast<const char*>(serial_rx_buffer), lTotalBytesRead);
}
