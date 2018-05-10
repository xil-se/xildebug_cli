#include <libusb.h>
#include <stdlib.h>
#include <stdio.h>

#define XILDEBUG_VID 0x1209
#define XILDEBUG_PID 0x9450
#define XILDEBUG_HID_IFACE_NO 2

int main(int argc, char *argv[])
{
	libusb_device_handle *handle = NULL;
	uint8_t buf[64] = { 0x00 };
	libusb_device *dev = NULL;
	int xfer_len = 0;
	int r;

	r = libusb_init(NULL);
	if (r != LIBUSB_SUCCESS) {
		fprintf(stderr, "Failed to init libusb\r\n");
		exit(1);
	}

	// TODO: Allow to filter by serial
	handle = libusb_open_device_with_vid_pid(NULL, XILDEBUG_VID, XILDEBUG_PID);
	if (handle == NULL) {
		fprintf(stderr, "Failed to open vid: 0x%04X, pid: 0x%04X\r\n", XILDEBUG_VID, XILDEBUG_PID);
		libusb_exit(NULL);
		exit(1);
	}

	dev = libusb_get_device(handle);
	if (dev == NULL) {
		fprintf(stderr, "Failed to get device pointer\r\n");
		libusb_close(handle);
		libusb_exit(NULL);
		exit(1);
	}

	libusb_set_auto_detach_kernel_driver(handle, 1);

	r = libusb_claim_interface(handle, XILDEBUG_HID_IFACE_NO);
	if (r != LIBUSB_SUCCESS) {
		fprintf(stderr, "Failed to get claim the HID interface\r\n");
		libusb_close(handle);
		libusb_exit(NULL);
		exit(1);
	}

	buf[0] = 0x00; // ID_DAP_Info
	buf[1] = 0x04; // DAP_ID_FW_VER
	r = libusb_interrupt_transfer(handle, 0x03, buf, sizeof(buf), &xfer_len, 1000);
	printf("r: %d, xfer_len: %d\r\n", r, xfer_len);

	r = libusb_interrupt_transfer(handle, 0x83, buf, sizeof(buf), &xfer_len, 1000);
	printf("r: %d, xfer_len: %d\r\n", r, xfer_len);

	printf("FW ver: %s\r\n", &buf[2]);

	libusb_release_interface(handle, XILDEBUG_HID_IFACE_NO);
	libusb_close(handle);
	libusb_exit(NULL);

	return 0;
}
