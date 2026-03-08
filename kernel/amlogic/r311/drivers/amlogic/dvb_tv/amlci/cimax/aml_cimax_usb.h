#ifndef _AML_CIMAX_USB_H_
#define _AML_CIMAX_USB_H_

#include <linux/platform_device.h>
#include "aml_cimax.h"

int aml_cimax_usb_init(struct platform_device *pdev, struct aml_cimax *ci);
int aml_cimax_usb_exit(struct aml_cimax *ci);

#endif
