//////////////////////////////////////////////////////////////////////
//                      North Carolina State University
//
//
//
//                             Copyright 2016
//
////////////////////////////////////////////////////////////////////////
//
// This program is free software; you can redistribute it and/or modify it
// under the terms and conditions of the GNU General Public License,
// version 2, as published by the Free Software Foundation.
//
// This program is distributed in the hope it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
//
////////////////////////////////////////////////////////////////////////
//
//   Author:  Hung-Wei Tseng, Yu-Chia Liu
//
//   Description:
//     Core of Kernel Module for Memory Container
//
////////////////////////////////////////////////////////////////////////

#include "memory_container.h"

#include <asm/uaccess.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/poll.h>
#include <linux/mutex.h>
#include <linux/sched.h>

#define LEN (64*1024)

extern struct miscdevice memory_container_dev;

int memory_container_mmap(struct file *filp, struct vm_area_struct *vma)
{
        printk("Print from memory_container_mmap %d", LEN+2*PAGE_SIZE);
        static int *kmalloc_area = NULL;
        kmalloc_ptr=kmalloc(LEN+2*PAGE_SIZE, GFP_KERNEL);
        kmalloc_area=(int *)(((unsigned long)kmalloc_ptr + PAGE_SIZE -1) & PAGE_MASK);
        vma->flags |= VM_LOCKED;
        if (remap_pfn_range(vma, vma->vm_start, virt_to_phys((void *)kmalloc_area) >> PAGE_SHIFT, vma->vm_end - vma->vm_start, vma->vm_page_prot) < 0)
        {
                printk("remap_pfn_range failed\n");
                return -EIO;
        }
        return 0;
}

int memory_container_init(void)
{
        int ret;

        if ((ret = misc_register(&memory_container_dev)))
        {
                printk(KERN_ERR "Unable to register \"memory_container\" misc device\n");
                return ret;
        }

        printk(KERN_ERR "\"memory_container\" misc device installed\n");
        printk(KERN_ERR "\"memory_container\" version 0.1\n");
        return ret;
}


void memory_container_exit(void)
{
        misc_deregister(&memory_container_dev);
}
