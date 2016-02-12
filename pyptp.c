#include <stdlib.h>
#include <libusb-1.0/libusb.h>
#include <python2.7/Python.h>
#include "ptp.h"
#include "ptp-pima.h"
#include "ptp-sony.h"
#include "timer.h"
#include "usb.h"

static PyObject * pyptp_open(PyObject *self, PyObject *args);
static PyObject * pyptp_close(PyObject *self, PyObject *args);
static PyObject * pyptp_start(PyObject *self, PyObject *args);
static PyObject * pyptp_stop(PyObject *self, PyObject *args);
static PyObject * pyptp_setparams(PyObject *self, PyObject *args, PyObject *kwargs);

static PyMethodDef PyPTPMethods[] = {
    { "open", pyptp_open, METH_VARARGS, "Open a Sony PTP Camera connection." },
    { "close", pyptp_close, METH_VARARGS, "Close a Sony PTP Camera connection." },
    { "start", pyptp_start, METH_VARARGS, "Start shooting using a Sony PTP Camera." },
    { "stop", pyptp_stop, METH_VARARGS, "Stop shooting using a Sony PTP Camera." },
    { "setparams", (PyCFunction)pyptp_setparams, METH_VARARGS | METH_KEYWORDS, "Set a Sony PTP Camera's parameters." },
    { NULL, NULL, 0, NULL }
};

PyMODINIT_FUNC
initpyptp(void)
{
    (void) Py_InitModule("pyptp", PyPTPMethods);
}

static PyObject * pyptp_open(PyObject *self, PyObject *args)
{
	/*
	int ret;
	usb_context *ctx;
	
	ret = usb_init(&ctx);
	
	if (ret != USB_OK)
	{
		PyErr_SetString(PyExc_RuntimeError, "Error initializing libusb");
		return NULL;
	}
	
	usb_exit(ctx);
	*/
	
    PyErr_SetString(PyExc_TypeError, "Not yet implemented");
    return NULL;
}

static PyObject * pyptp_close	(PyObject *self, PyObject *args)
{
    PyErr_SetString(PyExc_TypeError, "Not yet implemented");
    return NULL;
}

static PyObject * pyptp_start(PyObject *self, PyObject *args)
{
    PyErr_SetString(PyExc_TypeError, "Not yet implemented");
    return NULL;
}

static PyObject * pyptp_stop(PyObject *self, PyObject *args)
{
    PyErr_SetString(PyExc_TypeError, "Not yet implemented");
    return NULL;
}

static PyObject * pyptp_setparams(PyObject *self, PyObject *args, PyObject *kwargs)
{
    PyErr_SetString(PyExc_TypeError, "Not yet implemented");
    return NULL;
}
