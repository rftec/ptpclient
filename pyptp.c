#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include <libusb-1.0/libusb.h>
#include <python2.7/Python.h>
#include <python2.7/structmember.h>
#include <stdio.h>
#include "ptp.h"
#include "ptp-pima.h"
#include "ptp-sony.h"
#include "timer.h"
#include "usb.h"

#define MAX_IMAGE_PATH						255
#define POLL_TIMEOUT_SEC					1
#define TRANSFER_LOCK_NORMAL_TIMEOUT_SEC	5
#define TRANSFER_LOCK_THREAD_TIMEOUT_SEC	1

typedef enum {
	TSS_INVALID = 0,
	TSS_SEM_VALID,
	TSS_THREAD_VALID
} transfer_sync_state;

typedef struct {
	PyObject_HEAD
	usb_context *usbctx;
	usb_device_handle *usbdev;
	ptp_device *ptpdev;
	transfer_sync_state transfer_state;
	pthread_t thread_transfer;
	pthread_mutex_t mutex_transfer;
	sem_t sem_stop;
	sem_t sem_event;
	char *image_dir;
	unsigned int image_index;
	PyObject *callback;
} Camera;


static void Camera_dealloc(Camera *self);
static PyObject * Camera_new(PyTypeObject *type, PyObject *args, PyObject *kwds);
static int Camera_init_semaphores(Camera *self);
static void Camera_free_semaphores(Camera *self);
static int Camera_start_transfer(Camera *self);
static void Camera_stop_transfer(Camera *self);
static int Camera_set_image_dir(Camera *self, const char *dir);
static void Camera_clear_image_dir(Camera *self);
static int Camera_init(Camera *self, PyObject *args, PyObject *kwds);
static int Camera_lock_transfer(Camera *self);
static void Camera_unlock_transfer(Camera *self);
static PyObject * Camera_handshake(Camera *self, PyObject *args);
static PyObject * Camera_start(Camera *self, PyObject *args);
static PyObject * Camera_stop(Camera *self, PyObject *args);
static PyObject * Camera_setparams(Camera *self, PyObject *args, PyObject *kwds);
static PyObject * Camera_getbattery(Camera *self, PyObject *args);

static PyMethodDef Camera_methods[] = {
	{ "handshake", (PyCFunction)Camera_handshake, METH_NOARGS, "Camera handshake" },
	{ "start", (PyCFunction)Camera_start, METH_NOARGS, "Start shooting pictures" },
	{ "stop", (PyCFunction)Camera_stop, METH_NOARGS, "Stop shooting pictures" },
	{ "setparams", (PyCFunction)Camera_setparams, METH_VARARGS | METH_KEYWORDS, "Set camera parameters" },
	{ "getbattery", (PyCFunction)Camera_getbattery, METH_NOARGS, "Get battery level" },
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

static int pthread_mutex_timedlock_sec(pthread_mutex_t *mutex, unsigned int sec)
{
	struct timespec ts;
	
	clock_gettime(CLOCK_REALTIME, &ts);
	
	ts.tv_sec += sec;
	
	return pthread_mutex_timedlock(mutex, &ts);
}

static void pyptp_event_callback(ptp_device *dev, const ptp_params *params, void *ctx)
{
	Camera *self = (Camera *)ctx;
	
	if (params->code == PTP_EC_SONY_ObjectAdded)
	{
		sem_post(&self->sem_event);
	}
}

static void pyptp_call_callback(Camera *self, const char *path)
{
	PyObject *args, *res;
	PyGILState_STATE gstate;
	
	if (self->callback == NULL)
	{
		return;
	}
	
	gstate = PyGILState_Ensure();
	
	res = Py_BuildValue("(s)", path);
	
	if (res)
	{
		args = res;
		res = PyObject_CallObject(self->callback, args);
		
		Py_DECREF(args);
	}
	
	if (!res)
	{
		PyErr_Print();
		PyErr_Clear();
	}
	else
	{
		Py_DECREF(res);
	}
	
	PyGILState_Release(gstate);
}

static int pyptp_transfer_image(Camera *self, uint32_t handle)
{
	int retval, image_size;
	void *image_data;
	
	// Get the object's info
	retval = ptp_pima_get_object_info(self->ptpdev, handle, NULL);
	
	if (retval != PTP_OK)
	{
		return retval;
	}
	
	// Get the object's data
	retval = ptp_pima_get_object(self->ptpdev, handle, &image_data);
	
	if (retval >= 0)
	{
		char image_path[MAX_IMAGE_PATH + 1];
		
		image_size = retval;
		
		retval = snprintf(image_path, MAX_IMAGE_PATH, "%s/image-%u.jpg", self->image_dir, self->image_index);
		
		if (retval < 0 || retval > MAX_IMAGE_PATH)
		{
			// Could not create filename, drop image
		}
		else
		{
			FILE *f;
			
			image_path[MAX_IMAGE_PATH] = '\0';
			
			f = fopen(image_path, "wb");
			
			if (f)
			{
				fwrite(image_data, 1, image_size, f);
				fclose(f);
				
				pyptp_call_callback(self, image_path);
			}
		}
		
		free(image_data);
		
		self->image_index++;
		
		retval = PTP_OK;
	}
	
	return retval;
}

static void *pyptp_transfer_thread(void *ctx)
{
	int ret, ready, pending, locked;
	Camera *self = (Camera *)ctx;
	struct timespec ts;
	
	while (1)
	{
		// Reset the ready/pending count
		ready = 0;
		pending = 0;
		locked = 0;
		
		clock_gettime(CLOCK_REALTIME, &ts);
		
		ts.tv_sec += POLL_TIMEOUT_SEC;
		
		// Wait for an event at most POLL_TIMEOUT_SEC seconds
		ret = sem_timedwait(&self->sem_event, &ts);
		
		if (ret != 0)
		{
			// Did not get an event
			if (errno == ETIMEDOUT) // Poll if timed out
			{
				ret = pthread_mutex_timedlock_sec(&self->mutex_transfer, TRANSFER_LOCK_THREAD_TIMEOUT_SEC);
				
				if (ret != 0)
				{
					continue;
				}
				
				ret = ptp_sony_get_pending_objects(self->ptpdev);
				
				if (ret < 0)
				{
					// Failed to get pending objects, try again
					pthread_mutex_unlock(&self->mutex_transfer);
					continue;
				}
				
				pending = ret & 0x7FFF;
				ready = (ret & 0x8000) ? 1 : 0;
				
				if (!ready)
				{
					// Image not ready, try again
					pthread_mutex_unlock(&self->mutex_transfer);
					continue;
				}
				
				// Image ready, carry on to transfer
				locked = 1;
			}
			else
			{
				// Try again
				continue;
			}
		}
		
		// Check if we should stop, but only if there are no more pending images
		if (pending == 0 && !locked)
		{
			ret = sem_trywait(&self->sem_stop);
			
			if (ret == 0)
			{
				// Succeeded, stop the thread
				return NULL;
			}
		}
		
		if (!locked)
		{
			ret = pthread_mutex_timedlock_sec(&self->mutex_transfer, TRANSFER_LOCK_THREAD_TIMEOUT_SEC);
			
			if (ret != 0)
			{
				// Failed to lock, try again next time
				continue;
			}
		}
		
		// Transfer image
		ret = pyptp_transfer_image(self, 0xFFFFC001);
		
		pthread_mutex_unlock(&self->mutex_transfer);
	}
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
	Camera_clear_image_dir(self);
	Camera_stop_transfer(self);
	
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
	
	Py_XDECREF(self->callback);
	self->callback = NULL;
	
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
		self->transfer_state = TSS_INVALID;
		self->image_dir = NULL;
		self->image_index = 0;
		self->callback = NULL;
	}

	return (PyObject *)self;
}

static int Camera_init_semaphores(Camera *self)
{
	int ret;
	
	self->transfer_state = TSS_INVALID;
	
	ret = sem_init(&self->sem_stop, 0, 0);
	
	if (ret)
	{
		return -1;
	}
	
	ret = sem_init(&self->sem_event, 0, 0);
	
	if (ret)
	{
		sem_destroy(&self->sem_stop);
		return -1;
	}
	
	ret = pthread_mutex_init(&self->mutex_transfer, NULL);
	
	if (ret)
	{
		sem_destroy(&self->sem_event);
		sem_destroy(&self->sem_stop);
		return -1;
	}
	
	self->transfer_state = TSS_SEM_VALID;
	return 0;
}

static void Camera_free_semaphores(Camera *self)
{
	if (self->transfer_state < TSS_SEM_VALID)
	{
		return;
	}
	
	if (self->transfer_state > TSS_SEM_VALID)
	{
		Camera_stop_transfer(self);
	}
	
	// Destroy everything
	pthread_mutex_destroy(&self->mutex_transfer);
	sem_destroy(&self->sem_event);
	sem_destroy(&self->sem_stop);
	
	self->transfer_state = TSS_INVALID;
}

static int Camera_start_transfer(Camera *self)
{
	int ret;
	
	if (self->transfer_state < TSS_SEM_VALID)
	{
		return -1;
	}
	
	self->transfer_state = TSS_SEM_VALID;
	
	ret = pthread_create(&self->thread_transfer, NULL, pyptp_transfer_thread, self);
	
	if (ret)
	{
		return -1;
	}
	
	self->transfer_state = TSS_THREAD_VALID;
	return 0;
}

static void Camera_stop_transfer(Camera *self)
{
	if (self->transfer_state < TSS_THREAD_VALID)
	{
		return;
	}
	
	// Signal the transfer thread to stop
	sem_post(&self->sem_stop);
	
	// Make sure that the transfer thread has woken up
	sem_post(&self->sem_event);
	
	// Wait for the thread
	pthread_join(self->thread_transfer, NULL);
	
	self->transfer_state = TSS_SEM_VALID;
}

static int Camera_set_image_dir(Camera *self, const char *dir)
{
	if (dir == NULL || dir[0] == '\0')
	{
		dir = ".";
	}
	
	self->image_dir = strdup(dir);
	
	return (self->image_dir != NULL) ? 0 : -1;
}

static void Camera_clear_image_dir(Camera *self)
{
	if (self->image_dir)
	{
		free(self->image_dir);
		self->image_dir = NULL;
	}
}

static int Camera_init(Camera *self, PyObject *args, PyObject *kwds)
{
	static char *kwlist[] = { "vid", "pid", "image_dir", "callback", NULL };
	
	int vid, pid;
	int ret;
	const char *image_dir;
	usb_context *usbctx;
	usb_device_handle *usbdev;
	ptp_device *ptpdev;
	PyObject *callback;
	
	// Parse the keyword arguments
	if (!PyArg_ParseTupleAndKeywords(args, kwds, "ii|zO", kwlist, &vid, &pid, &image_dir, &callback))
	{
		return -1;
	}
	
	if (vid < 0 || vid > 0xFFFF)
	{
		PyErr_SetString(PyExc_ValueError, "Vendor ID out of range");
		return -1;
	}
	
	if (pid < 0 || pid > 0xFFFF)
	{
		PyErr_SetString(PyExc_ValueError, "Product ID out of range");
		return -1;
	}
	
	// Stop the transfer thread
	Camera_stop_transfer(self);
	
	// Close the previous device
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
	
	Camera_free_semaphores(self);
	
	// Clear the previous image directory string
	Camera_clear_image_dir(self);
	
	// Remove the previous callback
	Py_XDECREF(self->callback);
	self->callback = NULL;
	
	// Set the new callback
	if (!PyCallable_Check(callback))
	{
		PyErr_SetString(PyExc_TypeError, "Callback is not callable");
		return -1;
	}
	
	Py_XINCREF(callback);
	self->callback = callback;
	
	// Set the new image directory string
	if (Camera_set_image_dir(self, image_dir) != 0)
	{
		Py_XDECREF(callback);
		self->callback = NULL;
		
		PyErr_SetString(PyExc_RuntimeError, "Could not set target image directory");
		return -1;
	}
	
	if (Camera_init_semaphores(self) != 0)
	{
		Camera_clear_image_dir(self);
		Py_XDECREF(callback);
		self->callback = NULL;
		
		PyErr_SetString(PyExc_RuntimeError, "Could not initialize semaphores");
		return -1;
	}
	
	// Create a USB context if we don't have one already
	if (!self->usbctx)
	{
		ret = usb_init(&usbctx);

		if (ret != USB_OK)
		{
			Camera_free_semaphores(self);
			Camera_clear_image_dir(self);
			Py_XDECREF(callback);
			self->callback = NULL;
			
			PyErr_Format(PyExc_RuntimeError, "Error initializing libusb: Error %d", ret);
			return -1;
		}
	}
	
	// Open the USB device
	usbdev = usb_open_device_with_vid_pid(usbctx, vid, pid);
	
	if (usbdev == NULL)
	{
		usb_exit(usbctx);
		Camera_free_semaphores(self);
		Camera_clear_image_dir(self);
		Py_XDECREF(callback);
		self->callback = NULL;
		
		PyErr_SetString(PyExc_RuntimeError, "Could not open device: Device not found");
		return -1;
	}
	
	// Open the PTP device
	ret = ptp_device_init(&ptpdev, usbdev, pyptp_event_callback, self);
	
	if (ret != PTP_OK)
	{
		usb_close(usbdev);
		usb_exit(usbctx);
		Camera_free_semaphores(self);
		Camera_clear_image_dir(self);
		Py_XDECREF(callback);
		self->callback = NULL;
		
		PyErr_Format(PyExc_RuntimeError, "Could not initialize PTP device: PTP error %d", ret);
		return -1;
	}
	
	// Set the members before starting the transfer thread
	self->usbctx = usbctx;
	self->usbdev = usbdev;
	self->ptpdev = ptpdev;
	self->image_index = 0;
	
	// Start the transfer thread
	if (Camera_start_transfer(self) != 0)
	{
		self->usbctx = NULL;
		self->usbdev = NULL;
		self->ptpdev = NULL;
		
		ptp_device_free(ptpdev);
		usb_close(usbdev);
		usb_exit(usbctx);
		Camera_free_semaphores(self);
		Camera_clear_image_dir(self);
		Py_XDECREF(callback);
		self->callback = NULL;
		
		PyErr_SetString(PyExc_RuntimeError, "Could not start transfer thread");
		return -1;
	}
	
	return 0;
}

static int Camera_lock_transfer(Camera *self)
{
	int ret;
	
	ret = pthread_mutex_timedlock_sec(&self->mutex_transfer, TRANSFER_LOCK_NORMAL_TIMEOUT_SEC);
	
	if (ret != 0)
	{
		PyErr_Format(PyExc_RuntimeError, "Could not lock transfer mutex: pthread error %d", ret);
	}
	
	return ret;
}

static void Camera_unlock_transfer(Camera *self)
{
	pthread_mutex_unlock(&self->mutex_transfer);
}


static PyObject * Camera_handshake(Camera *self, PyObject *args)
{
	int ret;
	
	if (!self->ptpdev)
	{
		PyErr_SetString(PyExc_RuntimeError, "The camera has not been initialized.");
		return NULL;
	}
	
	if (Camera_lock_transfer(self) != 0)
	{
		return NULL;
	}
	
	ret = ptp_sony_handshake(self->ptpdev);
	
	if (ret != PTP_OK)
	{
		Camera_unlock_transfer(self);
		PyErr_Format(PyExc_RuntimeError, "Handshake failed: PTP error %d", ret);
		return NULL;
	}
	
	Camera_unlock_transfer(self);
	
	return Py_None;
}

static PyObject * Camera_start(Camera *self, PyObject *args)
{
	int ret;
	
	if (!self->ptpdev)
	{
		PyErr_SetString(PyExc_RuntimeError, "The camera has not been initialized.");
		return NULL;
	}
	
	if (Camera_lock_transfer(self) != 0)
	{
		return NULL;
	}
	
	ret = ptp_sony_set_control_device_b_u16(self->ptpdev, PTP_DPC_SONY_CTRL_AFLock, 2);
	
	if (ret != PTP_OK)
	{
		Camera_unlock_transfer(self);
		PyErr_Format(PyExc_RuntimeError, "Could not set AF lock: PTP error %d", ret);
		return NULL;
	}
	
	ret = ptp_sony_set_control_device_b_u16(self->ptpdev, PTP_DPC_SONY_CTRL_Shutter, 2);
	
	if (ret != PTP_OK)
	{
		ptp_sony_set_control_device_b_u16(self->ptpdev, PTP_DPC_SONY_CTRL_AFLock, 1);
		Camera_unlock_transfer(self);
		
		PyErr_Format(PyExc_RuntimeError, "Could not set shutter: PTP error %d", ret);
		return NULL;
	}
	
	Camera_unlock_transfer(self);
	
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject * Camera_stop(Camera *self, PyObject *args)
{
	int ret;
	
	if (!self->ptpdev)
	{
		PyErr_SetString(PyExc_RuntimeError, "The camera has not been initialized.");
		return NULL;
	}
	
	if (Camera_lock_transfer(self) != 0)
	{
		return NULL;
	}
	
	ret = ptp_sony_set_control_device_b_u16(self->ptpdev, PTP_DPC_SONY_CTRL_Shutter, 1);
	
	if (ret != PTP_OK)
	{
		// Try setting AF lock anyway
		ptp_sony_set_control_device_b_u16(self->ptpdev, PTP_DPC_SONY_CTRL_AFLock, 1);
		
		Camera_unlock_transfer(self);
		
		PyErr_Format(PyExc_RuntimeError, "Could not set shutter: PTP error %d", ret);
		return NULL;
	}
	
	ret = ptp_sony_set_control_device_b_u16(self->ptpdev, PTP_DPC_SONY_CTRL_AFLock, 1);
	
	if (ret != PTP_OK)
	{
		Camera_unlock_transfer(self);
		
		PyErr_Format(PyExc_RuntimeError, "Could not set AF lock: PTP error %d", ret);
		return NULL;
	}
	
	Camera_unlock_transfer(self);
	
	Py_INCREF(Py_None);
	return Py_None;
}

static int Camera_set_drive(Camera *self, const char *drive)
{
	int i, ret;
	
	static struct {
		const char *name;
		uint16_t value;
	} drive_modes[] = {
		{ "single", PTP_VAL_SONY_SCM_SINGLE },
		{ "low",    PTP_VAL_SONY_SCM_LOW },
		{ "medium", PTP_VAL_SONY_SCM_MID },
		{ "high",   PTP_VAL_SONY_SCM_HIGH },
		{ NULL, 0 }
	};
	
	if (drive != NULL)
	{
		for (i = 0; drive_modes[i].name != NULL; i++)
		{
			if (strcmp(drive, drive_modes[i].name) == 0)
			{
				ret = ptp_sony_set_drive_mode(self->ptpdev, drive_modes[i].value);
				
				if (ret != PTP_OK)
				{
					PyErr_Format(PyExc_RuntimeError, "Could not set drive mode: PTP error %d", ret);
					return -1;
				}
				
				return 0;
			}
		}
	}
	
	PyErr_Format(PyExc_ValueError, "Unrecognized drive mode: \"%s\"", drive);
	return -1;
}

static int Camera_set_iso(Camera *self, uint32_t iso)
{
	int ret;
	
	ret = ptp_sony_set_iso(self->ptpdev, iso);
	
	if (ret != PTP_OK)
	{
		PyErr_Format(PyExc_RuntimeError, "Could not set ISO: PTP error %d", ret);
		return -1;
	}
	
	return 0;
}

static int Camera_set_shutter_speed(Camera *self, const ptp_sony_shutter_speed *speed)
{
	int ret;
	
	if (speed != NULL)
	{
		ret = ptp_sony_set_shutter_speed(self->ptpdev, speed);
		
		if (ret != PTP_OK)
		{
			PyErr_Format(PyExc_RuntimeError, "Could not set shutter speed: PTP error %d", ret);
			return -1;
		}
		
		return 0;
	}
	
	PyErr_Format(PyExc_ValueError, "Unrecognized shutter speed");
	return -1;
}

static int Camera_set_fnumber(Camera *self, float fnumber)
{
	int ret;
	uint16_t f;
	
	if (fnumber <= 0.1)
	{
		PyErr_Format(PyExc_ValueError, "Invalid F-number");
		return -1;
	}
	
	f = (uint16_t)roundf(fnumber * 100);
	
	ret = ptp_sony_set_fnumber(self->ptpdev, f);
				
	if (ret != PTP_OK)
	{
		PyErr_Format(PyExc_RuntimeError, "Could not set F-number: PTP error %d", ret);
		return -1;
	}
	
	return 0;
}

static PyObject * Camera_setparams(Camera *self, PyObject *args, PyObject *kwds)
{
	static char *kwlist[] = { "drive", "iso", "shutter", "fnumber", NULL };
	
	const char *drive = NULL;
	unsigned int iso = 0;
	double fnum = 0.0;
	ptp_sony_shutter_speed shutter = { 0, 0 };
	
	if (!self->ptpdev)
	{
		PyErr_SetString(PyExc_RuntimeError, "The camera has not been initialized.");
		return NULL;
	}
	
	if (!PyArg_ParseTupleAndKeywords(args, kwds, "|zI(HH)d", kwlist, &drive, &iso, &shutter.num, &shutter.denom, &fnum))
	{
		return NULL;
	}
	
	if (Camera_lock_transfer(self) != 0)
	{
		return NULL;
	}
	
	if (drive != NULL && Camera_set_drive(self, drive) != 0)
	{
		Camera_unlock_transfer(self);
		return NULL;
	}
	
	if (iso != 0 && Camera_set_iso(self, (uint32_t)iso) != 0)
	{
		Camera_unlock_transfer(self);
		return NULL;
	}
	
	if (shutter.num != 0 || shutter.denom != 0)
	{
		if (Camera_set_shutter_speed(self, &shutter) != 0)
		{
			Camera_unlock_transfer(self);
			return NULL;
		}
	}
	
	if (fnum > 0.01 && Camera_set_fnumber(self, fnum) != 0)
	{
		Camera_unlock_transfer(self);
		return NULL;
	}
	
	Camera_unlock_transfer(self);
	
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject * Camera_getbattery(Camera *self, PyObject *args)
{
	int ret;
	
	if (!self->ptpdev)
	{
		PyErr_SetString(PyExc_RuntimeError, "The camera has not been initialized.");
		return NULL;
	}
	
	if (Camera_lock_transfer(self) != 0)
	{
		return NULL;
	}
	
	ret = ptp_sony_get_battery(self->ptpdev);
	
	Camera_unlock_transfer(self);
	
	if (ret < 0)
	{
		PyErr_Format(PyExc_RuntimeError, "Could not get battery level: PTP error %d", ret);
		return NULL;
	}
	
	return PyInt_FromLong(ret);
}
