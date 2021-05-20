#include <Python.h>
#include <Windows.h>

#include <PvTypes.h>
#include <PvDevice.h>
#include <PvSystem.h>

PyObject* tanh_impl(PyObject* self, PyObject* o);

//PyObject* getTestValue(         PyObject* self);
PyObject* initPvSystem(         PyObject* self);
PyObject* freePvSystem(         PyObject* self);
PyObject* getInterfaceCount(    PyObject* self);
PyObject* getDeviceCount(       PyObject* self, PyObject* oInterfaceNumber);
PyObject* getInterfaceDisplayID(PyObject* self, PyObject* oInterfaceNumber);
PyObject* getDeviceUniqueID(    PyObject* self, PyObject *args);

static PyMethodDef pyebustest_methods[] = {
    // The first property is the name exposed to Python, fast_tanh
    // The second is the C++ function with the implementation
    // METH_O means it takes a single PyObject argument
    { "fast_tanh", (PyCFunction)tanh_impl, METH_O, nullptr },

    //{ "getTestValue",          (PyCFunction)getTestValue,          METH_NOARGS, nullptr },
    { "initPvSystem",          (PyCFunction)initPvSystem,          METH_NOARGS, nullptr },
    //{ "freePvSystem",          (PyCFunction)freePvSystem,          METH_NOARGS, nullptr },
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


PyObject* tanh_impl(PyObject* self, PyObject* o) {
    double x = PyFloat_AsDouble(o);
    double tanh_x = x;
    return PyFloat_FromDouble(tanh_x);
}

///////////////////////////////////////////////
// Pv-specific stuff starts here
///////////////////////////////////////////////

PvSystem pvSystem;
PvSystem *lPvSystem = NULL;

long* some_value = NULL;

PyObject* initPvSystem(PyObject* self) {
    //lPvSystem = new PvSys
    lPvSystem = &pvSystem;
    //PvSystem pvSystem;
    Py_RETURN_NONE;
}

//PyObject* getTestValue(PyObject* self) {
//    printf("hello from getTestValue()!\n");
//    printf("%s\n", getenv("path"));
//    wchar_t buffer[10000];
//    GetCurrentDirectory(10000, buffer);
//    printf("%ls\n", buffer);
//    some_value = new long;
//    *some_value = 10;
//    return PyFloat_FromDouble(*some_value);
//}

//PyObject* freePvSystem(PyObject* self) {
//    if (lPvSystem)
//        delete lPvSystem;
//    lPvSystem = NULL;
//    Py_RETURN_NONE;
//}

PyObject* getInterfaceCount(PyObject* self) {
    if (lPvSystem == NULL)
    {
        return NULL;
    }
    return PyLong_FromSize_t(lPvSystem->GetInterfaceCount());
}

PyObject* getDeviceCount(PyObject* self, PyObject* oInterfaceNumber) {
    uint32_t interface_number = PyLong_AsLong(oInterfaceNumber);
    if (lPvSystem == NULL)
    {
        return NULL;
    }
    const PvInterface *lInterface = dynamic_cast<const PvInterface *>( lPvSystem->GetInterface( interface_number ) );
    if (lInterface == NULL)
    {
        return NULL;
    }
    return PyLong_FromSize_t(lInterface->GetDeviceCount());
}

PyObject* getInterfaceDisplayID(PyObject* self, PyObject* oInterfaceNumber) {
    PyObject* retval = PyLong_FromLong(-1L);
    uint32_t interface_number = PyLong_AsLong(oInterfaceNumber);
    const PvInterface *lInterface = dynamic_cast<const PvInterface *>( lPvSystem->GetInterface( interface_number ) );
    if (lInterface == NULL)
    {
        return NULL;
    }
    PvString pvs = lInterface->GetDisplayID();
    return PyUnicode_FromStringAndSize(pvs.GetAscii(), pvs.GetLength());
}

PyObject* getDeviceUniqueID(PyObject* self, PyObject *args) {
    uint32_t interface_number, device_number;
    if (!PyArg_ParseTuple(args, "ii", &interface_number, &device_number))
        return NULL;
    const PvInterface *lInterface = dynamic_cast<const PvInterface *>( lPvSystem->GetInterface( interface_number ) );
    if (lInterface == NULL)
    {
        return NULL;
    }
    const PvDeviceInfo *lDI = dynamic_cast<const PvDeviceInfo *>( lInterface->GetDeviceInfo( device_number ) );
    if (lDI == NULL)
    {
        return NULL;
    }
    PvString pvs = lInterface->GetUniqueID();
    return PyUnicode_FromStringAndSize(pvs.GetAscii(), pvs.GetLength());
}
