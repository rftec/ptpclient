#include <stdlib.h>
#include <libusb-1.0/libusb.h>
#include <python2.7/Python.h>
#include <python2.7/structmember.h>
#include "ptp.h"
#include "ptp-pima.h"
#include "ptp-sony.h"
#include "timer.h"
#include "usb.h"

typedef struct {
	PyObject_HEAD
	usb_context *usbctx;
	usb_device_handle *usbdev;
	ptp_device *ptpdev;
} Camera;


static void Camera_dealloc(Camera *self);
static PyObject * Camera_new(PyTypeObject *type, PyObject *args, PyObject *kwds);
static int Camera_init(Camera *self, PyObject *args, PyObject *kwds);
static PyObject * Camera_start(Camera *self, PyObject *args);
static PyObject * Camera_stop(Camera *self, PyObject *args);
static PyObject * Camera_setparams(Camera *self, PyObject *args, PyObject *kwds);

static PyMethodDef Camera_methods[] = {
	{ "start", (PyCFunction)Camera_start, METH_VARARGS, "Start shooting pictures" },
	{ "stop", (PyCFunction)Camera_stop, METH_VARARGS, "Stop shooting pictures" },
	{ "setparams", (PyCFunction)Camera_setparams, METH_VARARGS | METH_KEYWORDS, "Set camera parameters" },
	{ NULL }
};

static PyMemberDef Camera_members[] = {
	{ NULL }
};

static PyTypeObject pyptp_CameraType = {
	PyObject_HEAD_INIT(NULL)
	0,                         /*ob_size*/
	"pyptp.Camera",            /*tp_name*/
	sizeof(Camera),            /*tp_basicsize*/
	0,                         /*tp_itemsize*/
	(destructor)Camera_dealloc,/*tp_dealloc*/
	0,                         /*tp_print*/
	0,                         /*tp_getattr*/
	0,                         /*tp_setattr*/
	0,                         /*tp_compare*/
	0,                         /*tp_repr*/
	0,                         /*tp_as_number*/
	0,                         /*tp_as_sequence*/
	0,                         /*tp_as_mapping*/
	0,                         /*tp_hash */
	0,                         /*tp_call*/
	0,                         /*tp_str*/
	0,                         /*tp_getattro*/
	0,                         /*tp_setattro*/
	0,                         /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT,        /*tp_flags*/
	"PTP Camera Object",       /* tp_doc */
	0,		                   /* tp_traverse */
	0,		                   /* tp_clear */
	0,		                   /* tp_richcompare */
	0,		                   /* tp_weaklistoffset */
	0,		                   /* tp_iter */
	0,		                   /* tp_iternext */
	Camera_methods,            /* tp_methods */
	Camera_members,            /* tp_members */
	0,                         /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	(initproc)Camera_init,     /* tp_init */
	0,                         /* tp_alloc */
	Camera_new,                /* tp_new */
};

static PyMethodDef pyptp_Methods[] = {
	{ NULL }
};

static void pyptp_event_callback(ptp_device *dev, const ptp_params *params, void *ctx)
{
}

PyMODINIT_FUNC
initpyptp(void)
{
	PyObject* m;
	
	if (PyType_Ready(&pyptp_CameraType) < 0)
	{
		return;
	}

	m = Py_InitModule3("pyptp", pyptp_Methods, "Python PTP Camera module");

	Py_INCREF(&pyptp_CameraType);
	PyModule_AddObject(m, "Camera", (PyObject *)&pyptp_CameraType);
}

static void Camera_dealloc(Camera *self)
{
	if (self->ptpdev)
	{
		ptp_device_free(self->ptpdev);
		self->ptpdev = NULL;
	}
	
	if (self->usbdev)
	{
		usb_close(self->usbdev);
		self->usbdev = NULL;
	}
	
	if (self->usbctx)
	{
		usb_exit(self->usbctx);
		self->usbctx = NULL;
	}
	
	self->ob_type->tp_free((PyObject *)self);
}

static PyObject * Camera_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	Camera *self;
	
	self = (Camera *)type->tp_alloc(type, 0);
	
	if (self != NULL)
	{
		self->usbctx = NULL;
		self->usbdev = NULL;
		self->ptpdev = NULL;
	}

	return (PyObject *)self;
}

static int Camera_init(Camera *self, PyObject *args, PyObject *kwds)
{
	static char *kwlist[] = { "vid", "pid", NULL };
	
	int vid, pid;
	int ret;
	usb_context *usbctx;
	usb_device_handle *usbdev;
	ptp_device *ptpdev;
	
	if (!PyArg_ParseTupleAndKeywords(args, kwds, "ii", kwlist, &vid, &pid))
	{
		return -1;
	}
	
	if (vid < 0 || vid > 0xFFFF)
	{
		PyErr_SetString(PyExc_RuntimeError, "Vendor ID out of range");
		return -1;
	}
	
	if (pid < 0 || pid > 0xFFFF)
	{
		PyErr_SetString(PyExc_RuntimeError, "Product ID out of range");
		return -1;
	}
	
	if (self->ptpdev)
	{
		ptp_device_free(self->ptpdev);
		self->ptpdev = NULL;
	}
	
	if (self->usbdev)
	{
		usb_close(self->usbdev);
		self->usbdev = NULL;
	}
	
	if (!self->usbctx)
	{
		ret = usb_init(&usbctx);

		if (ret != USB_OK)
		{
			PyErr_SetString(PyExc_RuntimeError, "Error initializing libusb");
			return -1;
		}
	}
	
	usbdev = usb_open_device_with_vid_pid(usbctx, vid, pid);
	
	if (usbdev == NULL)
	{
		usb_exit(usbctx);
		
		PyErr_SetString(PyExc_RuntimeError, "Could not open device");
		return -1;
	}
	
	ret = ptp_device_init(&ptpdev, usbdev, pyptp_event_callback, NULL);
	
	if (ret != PTP_OK)
	{
		usb_close(usbdev);
		usb_exit(usbctx);
		
		PyErr_SetString(PyExc_RuntimeError, "Could not initialize PTP device");
		return -1;
	}
	
	self->usbctx = usbctx;
	self->usbdev = usbdev;
	self->ptpdev = ptpdev;
	
	return 0;
}

static PyObject * Camera_start(Camera *self, PyObject *args)
{
    PyErr_SetString(PyExc_TypeError, "Not yet implemented");
	return NULL;
}

static PyObject * Camera_stop(Camera *self, PyObject *args)
{
    PyErr_SetString(PyExc_TypeError, "Not yet implemented");
	return NULL;
}

static PyObject * Camera_setparams(Camera *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_TypeError, "Not yet implemented");
	return NULL;
}
