#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#ifdef CONFIG_MESON_REMOTE_LEARN
#include <linux/amlogic/iomap.h>
#include <linux/amlogic/scpi_protocol.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#endif
#include "remote_meson.h"
#include "sysfs.h"

#define AML_REMOTE_NAME "amremote"


#ifdef CONFIG_MESON_REMOTE_LEARN
static u64 remote_learn_dma_mask = DMA_BIT_MASK(32);


static int remote_learn_get_raw(struct remote_chip *chip, int *userpt)
{
	int data = 0;
	int cl_size;
	unsigned char *cl_data;

	chip->cl_mem = kzalloc(CL_MEM_SIZE, GFP_KERNEL);
	if (!chip->cl_mem) {
		pr_err("kzalloc error\n");
		return -EINVAL;
	}
	chip->dev->dma_mask = &remote_learn_dma_mask;
	chip->cl_mem_phy = (void*)dma_map_single(chip->dev,
		chip->cl_mem, CL_MEM_SIZE, DMA_FROM_DEVICE);
	if (dma_mapping_error(chip->dev, (dma_addr_t)chip->cl_mem_phy)) {
		pr_info("dma_map error\n");
		goto err_map;
	}

	/*if fail return -EINVAL*/
	data = scpi_get_ir_data(1, chip->cl_mem_phy, CL_MEM_SIZE, &cl_size);
	dma_unmap_single( chip->dev, (dma_addr_t)chip->cl_mem_phy,
		CL_MEM_SIZE, DMA_FROM_DEVICE);

	if (data == -EINVAL) {
		goto err_scpi;
	}

	cl_data = (unsigned char *)chip->cl_mem;
	if (copy_to_user(userpt, cl_data, cl_size)) {
        return -EINVAL;
    }
	kfree(chip->cl_mem);
	return cl_size;
err_scpi:
err_map:
	kfree(chip->cl_mem);
	return -EINVAL;
}


static int remote_learn(struct remote_chip *chip, int* user)
{
    int startlearn = 0, ret = 0;

    get_user(startlearn, (int*)(void __user *)user);
    if(startlearn) {
/*
        todo: disable ir remote
*/
        pr_info("#### start ir learn\n");
/*    	disable_irq(chip->irqno);*/
        aml_write_cbus(0x2d, 0x200);
        aml_write_cbus(0x2e,0);
        ret = scpi_set_ir_data(0, LEARN_TIMEOUT);
    } else {
/*
        todo: enable ir remote
*/
        pr_info("#### stop ir learn\n");
        ret = scpi_set_ir_data(1, 0);
/*
		msleep(50);
		enable_irq(chip->irqno);
*/
    }
    return ret;
}
#endif


static int remote_open(struct inode *inode, struct file *file)
{
	struct remote_chip *chip;

	chip = container_of(inode->i_cdev, struct remote_chip, chrdev);
	file->private_data = chip;
	disable_irq(chip->irqno);
	return 0;
}
static long remote_ioctl(struct file *file, unsigned int cmd,
				unsigned long arg)
{
	struct remote_chip *chip = (struct remote_chip *)file->private_data;
	struct ir_sw_decode_para sw_data;
	struct ir_map_tab_list *ir_map;
	struct ir_map_tab_list *ptable;
	void __user *parg = (void __user *)arg;
	unsigned long flags;
	u32 value;
	int retval = 0;

	if (!parg) {
		dev_err(chip->dev, "%s invalid user space pointer\n", __func__);
		return -EINVAL;
	}

	mutex_lock(&chip->file_lock);
	switch (cmd) {
	case REMOTE_IOC_GET_DATA_VERSION:
		if (copy_to_user(parg, SHARE_DATA_VERSION,
						sizeof(SHARE_DATA_VERSION))) {
			retval = -EFAULT;
			goto err;
		}
	break;

	case REMOTE_IOC_SET_KEY_NUMBER:
		if (copy_from_user(&value, parg, sizeof(u32))) {
			chip->key_num.update_flag = false;
			retval = -EFAULT;
			goto err;
		}
		chip->key_num.update_flag = true;
		chip->key_num.value = value;
		break;

	case REMOTE_IOC_SET_KEY_MAPPING_TAB:
		if (chip->key_num.update_flag) {
			ir_map = kzalloc(sizeof(struct ir_map_tab_list) +
				chip->key_num.value * sizeof(union _codemap),
			    GFP_KERNEL);
		    if (!ir_map) {
				dev_err(chip->dev, "%s ir map table alloc err\n",
						__func__);
				retval = -ENOMEM;
				goto err;
			}
			if (copy_from_user(&ir_map->tab, parg,
				sizeof(struct ir_map_tab) +
				chip->key_num.value * sizeof(union _codemap))) {
				ir_tab_free(ir_map);
				retval = -EFAULT;
				goto err;
			}

			/* Check data whether valid or not*/
			if (chip->key_num.value != ir_map->tab.map_size) {
				ir_tab_free(ir_map);
				retval = -EFAULT;
				goto err;
			}
			/*scancode sort*/
			ir_scancode_sort(&ir_map->tab);

			/*overwrite the old map table or insert new map table*/
			spin_lock_irqsave(&chip->slock, flags);
			ptable = seek_map_tab(chip, ir_map->tab.custom_code);
			if (ptable) {
				if (ptable == chip->cur_tab)
					chip->cur_tab = ir_map;
				list_del(&ptable->list);
				ir_tab_free(ptable);
			}
			list_add_tail(&ir_map->list, &chip->map_tab_head);
			spin_unlock_irqrestore(&chip->slock, flags);
			chip->key_num.update_flag = false;
		}
		break;

	case REMOTE_IOC_SET_SW_DECODE_PARA:
		if (copy_from_user(&sw_data, parg,
				sizeof(struct ir_sw_decode_para))) {
			retval = -EFAULT;
			goto err;
		}
		chip->r_dev->max_frame_time = sw_data.max_frame_time;
		break;

	#ifdef CONFIG_MESON_REMOTE_LEARN
		case START_LEARN:
			if (chip->ir_learn)
				retval = remote_learn(chip, parg);
			else
				retval = -ENOTTY;

			break;
		case GET_RAW:
			if (chip->ir_learn)
				retval = remote_learn_get_raw(chip, parg);
			else
				retval = -ENOTTY;

			break;
	#endif
		default:
			retval = -ENOTTY;
			goto err;
		}
	err:
		mutex_unlock(&chip->file_lock);
		return retval;
}
static int remote_release(struct inode *inode, struct file *file)
{
	struct remote_chip *chip = (struct remote_chip *)file->private_data;

	enable_irq(chip->irqno);
	file->private_data = NULL;
	return 0;
}

static const struct file_operations remote_fops = {
	.owner = THIS_MODULE,
	.open = remote_open,
#ifdef CONFIG_COMPAT
	.compat_ioctl = remote_ioctl,
#endif
	.unlocked_ioctl = remote_ioctl,
	.release = remote_release,
};

int ir_cdev_init(struct remote_chip *chip)
{
	int ret = 0;

	chip->dev_name  = AML_REMOTE_NAME;
	ret = alloc_chrdev_region(&chip->chr_devno,
		0, 1, AML_REMOTE_NAME);
	if (ret < 0) {
		dev_err(chip->dev, "failed to allocate major number\n");
		ret = -ENODEV;
		goto err_end;
	}
	cdev_init(&chip->chrdev, &remote_fops);
	chip->chrdev.owner = THIS_MODULE;
	ret = cdev_add(&chip->chrdev, chip->chr_devno, 1);
	if (ret < 0) {
		dev_err(chip->dev, "failed to cdev_add\n");
		goto err_cdev_add;
	}

	ret = ir_sys_device_attribute_init(chip);
	if (ret < 0) {
		dev_err(chip->dev, "failed to ir_sys create %d\n", ret);
		goto err_ir_sys;
	}
	return 0;

err_ir_sys:
	dev_err(chip->dev, "err_ir_sys\n");
	cdev_del(&chip->chrdev);
err_cdev_add:
	dev_err(chip->dev, "err_cdev_add\n");
	unregister_chrdev_region(chip->chr_devno, 1);
err_end:
	return ret;
}
EXPORT_SYMBOL(ir_cdev_init);

void ir_cdev_free(struct remote_chip *chip)
{
	ir_sys_device_attribute_sys(chip);
	cdev_del(&chip->chrdev);
	unregister_chrdev_region(chip->chr_devno, 1);
}
EXPORT_SYMBOL(ir_cdev_free);

