#include <Python.h>
#include <Windows.h>

#include <PvTypes.h>
#include <PvDevice.h>
#include <PvSystem.h>

PyObject* getInterfaceCount(    PyObject* self);
PyObject* getDeviceCount(       PyObject* self, PyObject* oInterfaceNumber);
PyObject* getInterfaceDisplayID(PyObject* self, PyObject* oInterfaceNumber);
PyObject* getDeviceUniqueID(    PyObject* self, PyObject *args);

static PyMethodDef pyebustest_methods[] = {
    // The first property is the name exposed to Python, fast_tanh
    // The second is the C++ function with the implementation
    // METH_O means it takes a single PyObject argument

    { "getInterfaceCount",     (PyCFunction)getInterfaceCount,     METH_NOARGS, nullptr },
    { "getDeviceCount",        (PyCFunction)getDeviceCount,        METH_O, nullptr },
    { "getInterfaceDisplayID", (PyCFunction)getInterfaceDisplayID, METH_O, nullptr },
    { "getDeviceUniqueID",     (PyCFunction)getDeviceUniqueID,     METH_VARARGS, nullptr },

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
