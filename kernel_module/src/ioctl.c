//////////////////////////////////////////////////////////////////////
//                      North Carolina State University
//
//
//
//                             Copyright 2018
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
//     Core of Kernel Module for Processor Container
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
#include <linux/kthread.h>

// Mutex for performing any updates on pid_list
static DEFINE_MUTEX(pid_list_lock);

// Mutex for performing any updates on oid_list
static DEFINE_MUTEX(oid_list_lock);

extern void free_all_ds(void);

// Node that stores PID:CID mapping
struct pid_node {
        int cid;
        int pid;
        int valid;
        struct pid_node *next;
};

// Node that stores OID data
struct oid_node {
        int cid;
        __u64 oid;
        void *address;
        struct mutex *lock;
        struct oid_node *next;
};

// Actual list that stores the PID nodes
struct pid_node *pid_list = NULL;

// Actual list that stores the OID nodes
struct oid_node *oid_list = NULL;

void add_pid_node(int pid, int cid){

        mutex_lock(&pid_list_lock);
        // printk("Adding PID: %d to CID: %d\n", pid, cid);
        if(pid_list == NULL) {
                // First PID ever
                pid_list = (struct pid_node *)kmalloc(sizeof(struct pid_node), GFP_KERNEL);
                pid_list->pid = pid;
                pid_list->cid = cid;
                pid_list->next = NULL;
                pid_list->valid = 1;
        } else {
                // Later PIDs, find the tail
                struct pid_node *prev_pid_node = NULL;
                struct pid_node *temp_pid_node = pid_list;
                struct pid_node *new_pid_node;

                while (temp_pid_node != NULL) {
                        prev_pid_node = temp_pid_node;
                        temp_pid_node = temp_pid_node->next;
                }

                // Initialize new PID node and add at tail
                new_pid_node = (struct pid_node *)kmalloc(sizeof(struct pid_node), GFP_KERNEL);
                new_pid_node->pid = pid;
                new_pid_node->cid = cid;
                new_pid_node->next = NULL;
                new_pid_node->valid = 1;
                prev_pid_node->next = new_pid_node;
        }
        mutex_unlock(&pid_list_lock);
        return;
}

void remove_pid_node(int pid){

        struct pid_node *curr_pid;
        struct pid_node *prev_pid = NULL;

        mutex_lock(&pid_list_lock);
        // printk("Deleting PID: %d\n", pid);
        curr_pid = pid_list;

        while (curr_pid != NULL) {
                if(curr_pid->pid == pid) {
                        // PID reference found, soft delete
                        curr_pid->valid = 0;
                        break;
                }
                prev_pid = curr_pid;
                curr_pid = curr_pid->next;
        }
        mutex_unlock(&pid_list_lock);
        return;
}

int get_cid_for_pid(int pid){
        struct pid_node *curr_pid;
        struct pid_node *prev_pid = NULL;
        int cid;

        // If PID is not present or was deleted
        cid = -1;

        curr_pid = pid_list;
        while (curr_pid != NULL) {
                if(curr_pid->pid == pid) {
                        // PID reference found, soft delete
                        cid = curr_pid->cid;
                        break;
                }
                prev_pid = curr_pid;
                curr_pid = curr_pid->next;
        }
        // printk("PID: %d belongs to CID: %d\n", pid, cid);
        return cid;
}

struct oid_node* lookup_oid_from_cid(__u64 oid, int cid){

        struct oid_node *oid_ptr = NULL;
        struct oid_node *curr_oid;
        struct oid_node *prev_oid = NULL;

        // printk("Searching OID %llu in CID %d by PID: %d\n", oid, cid, current->pid);

        curr_oid = oid_list;
        while (curr_oid != NULL) {
                if(curr_oid->oid == oid && curr_oid->cid == cid) {
                        // OID reference found
                        oid_ptr = curr_oid;
                        break;
                }
                prev_oid = curr_oid;
                curr_oid = curr_oid->next;
        }
        return oid_ptr;
}

struct oid_node* add_oid_node(__u64 oid, int cid){

        struct oid_node *oid_ptr;
        mutex_lock(&oid_list_lock);

        // Re-check if OID is not there, if not the PID has taken the lock and
        // also the responsibility to create the OID node
        oid_ptr = lookup_oid_from_cid(oid, cid);

        if (oid_ptr == NULL) {
                // Create new OID node, and no one else can now create it since lock is taken
                // printk("Adding OID %llu in CID %d by PID: %d\n", oid, cid, current->pid);
                if(oid_list == NULL) {
                        // First OID ever
                        oid_list = (struct oid_node *)kmalloc(sizeof(struct oid_node), GFP_KERNEL);
                        oid_list->oid = oid;
                        oid_list->cid = cid;
                        oid_list->next = NULL;
                        oid_list->address = NULL;
                        oid_list->lock = (struct mutex *)kmalloc(sizeof(struct mutex), GFP_KERNEL);
                        mutex_init(oid_list->lock);
                        oid_ptr = oid_list;
                } else {
                        // Later OIDs, find the tail
                        struct oid_node *prev_oid_node = NULL;
                        struct oid_node *temp_oid_node = oid_list;
                        struct oid_node *new_oid_node;

                        while (temp_oid_node != NULL) {
                                prev_oid_node = temp_oid_node;
                                temp_oid_node = temp_oid_node->next;
                        }

                        // Initialize new OID node and add at tail
                        new_oid_node = (struct oid_node *)kmalloc(sizeof(struct oid_node), GFP_KERNEL);
                        new_oid_node->oid = oid;
                        new_oid_node->cid = cid;
                        new_oid_node->next = NULL;
                        new_oid_node->address = NULL;
                        new_oid_node->lock = (struct mutex *)kmalloc(sizeof(struct mutex), GFP_KERNEL);
                        mutex_init(new_oid_node->lock);
                        prev_oid_node->next = new_oid_node;
                        oid_ptr = new_oid_node;
                }
        } else {
                // printk("Skip adding OID %llu in CID %d by PID: %d\n", oid, cid, current->pid);
        }
        mutex_unlock(&oid_list_lock);
        return oid_ptr;
}

struct oid_node* get_oid_ptr_from_cid(__u64 oid, int cid){

        struct oid_node *oid_ptr;

        // Lookup OID in CID, will get null if OID:CID doesn't exist
        oid_ptr = lookup_oid_from_cid(oid, cid);

        if (oid_ptr == NULL) {
                // If OID reference not found, create OID node and add to list
                oid_ptr = add_oid_node(oid, cid);
        }
        return oid_ptr;
}

void update_lock_oid_in_cid(__u64 oid, int cid, int op){

        struct oid_node *oid_ptr;
        // Get refernce to the oid
        oid_ptr = get_oid_ptr_from_cid(oid, cid);
        // printk("Updating lock for OID: %llu from CID: %d by PID: %d OP: %d\n", oid, cid, current->pid, op);

        if(op == 1) {
                // Lock the oid
                mutex_lock(oid_ptr->lock);
                // printk("Locked OID: %llu from CID: %d by PID: %d\n", oid, cid, current->pid);
        } else if (op == 0) {
                // Unlock the oid
                mutex_unlock(oid_ptr->lock);
                // printk("Unlocked OID: %llu from CID: %d by PID: %d\n", oid, cid, current->pid);
        }
        return;
}

void free_all_ds() {

        // printk("Start freeing everything\n");
        // Iterate over the list and kfree everything
        struct oid_node *prev_oid_node;
        struct oid_node *temp_oid_node = oid_list;

        while (temp_oid_node != NULL) {
                prev_oid_node = temp_oid_node;
                temp_oid_node = temp_oid_node->next;
                kfree(prev_oid_node->address);
                kfree(prev_oid_node);
        }

        // Iterate over the list and kfree everything
        struct pid_node *prev_pid_node;
        struct pid_node *temp_pid_node = pid_list;

        while (temp_pid_node != NULL) {
                prev_pid_node = temp_pid_node;
                temp_pid_node = temp_pid_node->next;
                kfree(prev_pid_node);
        }
        // printk("Done freeing everything\n");
}

int memory_container_mmap(struct file *filp, struct vm_area_struct *vma)
{
        void *kmalloc_ptr;
        unsigned long requested_size;
        __u64 vtp;
        int cid;
        struct oid_node *oid_ptr;

        // Get the CID for PID
        cid = get_cid_for_pid(current->pid);

        // Get OID reference for given CID
        oid_ptr = get_oid_ptr_from_cid((__u64)vma->vm_pgoff, cid);

        // Calculate requested page size
        requested_size = vma->vm_end - vma->vm_start;

        if (oid_ptr->address == NULL) {

                kmalloc_ptr = NULL;
                kmalloc_ptr = kmalloc(requested_size, GFP_KERNEL);
                oid_ptr->address = kmalloc_ptr;
                vtp = virt_to_phys(kmalloc_ptr);

                printk("Assigning new mem for OID: %ld from PID: %d\n", vma->vm_pgoff, current->pid);
                printk("OID addr %pS\n", oid_ptr->address);
                printk("VTP: %llu\n", vtp);
                printk("PAGE_SIZE: %lu\n", PAGE_SIZE);
                printk("Requested size: %lu\n", requested_size);
                printk("Start: %lu\n", vma->vm_start);
                printk("End: %lu\n", vma->vm_end);
                printk("PAGE_SHIFT %d\n", PAGE_SHIFT);
                printk("PAGE_SHIFT value %llu\n",vtp >> PAGE_SHIFT);

                if (remap_pfn_range(vma, vma->vm_start, vtp >> PAGE_SHIFT, vma->vm_end - vma->vm_start, vma->vm_page_prot) < 0)
                {
                        printk("remap_pfn_range failed\n");
                        return -EIO;
                }

        } else {
                vtp = virt_to_phys(oid_ptr->address);

                printk("Accessing mem for OID: %ld\n from PID: %d", vma->vm_pgoff, current->pid);
                printk("OID addr %pS\n", oid_ptr->address);
                printk("VTP: %llu\n", vtp);
                printk("PAGE_SIZE: %lu\n", PAGE_SIZE);
                printk("Requested size: %lu\n", requested_size);
                printk("Start: %lu\n", vma->vm_start);
                printk("End: %lu\n", vma->vm_end);
                printk("PAGE_SHIFT %d\n", PAGE_SHIFT);
                printk("PAGE_SHIFT value %llu\n",vtp >> PAGE_SHIFT);

                if (remap_pfn_range(vma, vma->vm_start, vtp >> PAGE_SHIFT, vma->vm_end - vma->vm_start, vma->vm_page_prot) < 0)
                {
                        printk("remap_pfn_range failed\n");
                        return -EIO;
                }
        }
        return 0;
}

int memory_container_lock(struct memory_container_cmd __user *user_cmd)
{
        int cid;

        // Get the current CID
        struct memory_container_cmd *user_cmd_kernal;

        user_cmd_kernal = kmalloc(sizeof(struct memory_container_cmd), GFP_KERNEL);
        copy_from_user(user_cmd_kernal, (void *)user_cmd, sizeof(struct memory_container_cmd));

        // Get CID for PID
        cid = get_cid_for_pid(current->pid);

        update_lock_oid_in_cid(user_cmd_kernal->oid, cid, 1); // 1 Means unlock
        return 0;
}

int memory_container_unlock(struct memory_container_cmd __user *user_cmd)
{
        int cid;

        // Get the current CID
        struct memory_container_cmd *user_cmd_kernal;

        user_cmd_kernal = kmalloc(sizeof(struct memory_container_cmd), GFP_KERNEL);
        copy_from_user(user_cmd_kernal, (void *)user_cmd, sizeof(struct memory_container_cmd));

        // Get CID for PID
        cid = get_cid_for_pid(current->pid);

        update_lock_oid_in_cid(user_cmd_kernal->oid, cid, 0); // 0 Means unlock
        return 0;
}

int memory_container_delete(struct memory_container_cmd __user *user_cmd)
{
        // Delete the PID from list
        remove_pid_node(current->pid);
        return 0;
}

int memory_container_create(struct memory_container_cmd __user *user_cmd)
{
        // Get the current CID
        struct memory_container_cmd *user_cmd_kernal;

        user_cmd_kernal = kmalloc(sizeof(struct memory_container_cmd), GFP_KERNEL);
        copy_from_user(user_cmd_kernal, (void *)user_cmd, sizeof(struct memory_container_cmd));

        // Add the PID:CID mapping node
        add_pid_node(current->pid, user_cmd_kernal->cid);

        return 0;
}

int memory_container_free(struct memory_container_cmd __user *user_cmd)
{
        int cid;
        struct memory_container_cmd *user_cmd_kernal;
        struct oid_node *oid_ptr;

        // Get the CID for PID
        cid = get_cid_for_pid(current->pid);

        // Get OID from user_cmd
        user_cmd_kernal = kmalloc(sizeof(struct memory_container_cmd), GFP_KERNEL);
        copy_from_user(user_cmd_kernal, (void *)user_cmd, sizeof(struct memory_container_cmd));

        oid_ptr = get_oid_ptr_from_cid(user_cmd_kernal->oid, cid);

        // Free the memory held by the object
        // printk("Trying to free Memory for OID: %llu in CID: %d by PID %d\n", user_cmd_kernal->oid, cid, current->pid);
        // printk("VOID pointer %pS\n", oid_ptr->address);
        oid_ptr->address = NULL;
        kfree(oid_ptr->address);
        // printk("Memory freed for OID: %llu in CID: %d by PID %d\n", user_cmd_kernal->oid, cid, current->pid);

        return 0;
}


/**
 * control function that receive the command in user space and pass arguments to
 * corresponding functions.
 */
int memory_container_ioctl(struct file *filp, unsigned int cmd,
                           unsigned long arg)
{
        switch (cmd)
        {
        case MCONTAINER_IOCTL_CREATE:
                return memory_container_create((void __user *)arg);
        case MCONTAINER_IOCTL_DELETE:
                return memory_container_delete((void __user *)arg);
        case MCONTAINER_IOCTL_LOCK:
                return memory_container_lock((void __user *)arg);
        case MCONTAINER_IOCTL_UNLOCK:
                return memory_container_unlock((void __user *)arg);
        case MCONTAINER_IOCTL_FREE:
                return memory_container_free((void __user *)arg);
        default:
                return -ENOTTY;
        }
}
