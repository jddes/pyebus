#include <Python.h>
#include <Windows.h>

PyObject* tanh_impl(PyObject* /* unused module reference */, PyObject* o) {
    double x = PyFloat_AsDouble(o);
    double tanh_x = x;
    return PyFloat_FromDouble(tanh_x);
}

static PyMethodDef pyebustest_methods[] = {
    // The first property is the name exposed to Python, fast_tanh
    // The second is the C++ function with the implementation
    // METH_O means it takes a single PyObject argument
    { "fast_tanh", (PyCFunction)tanh_impl, METH_O, nullptr },

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
    return PyModule_Create(&pyebustest_module);
}
