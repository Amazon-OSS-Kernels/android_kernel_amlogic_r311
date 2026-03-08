#ifndef _CIMAX_USB_DEV_H_
#define _CIMAX_USB_DEV_H_

__attribute__ ((weak))
int cimax_usb_dev_add(struct device_s *device, int id);
__attribute__ ((weak))
int cimax_usb_dev_remove(struct device_s *device, int id);

#endif

