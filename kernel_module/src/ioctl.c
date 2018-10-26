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

// Mutex for performing any updates on pid_cid_list
static DEFINE_MUTEX(pid_list_lock);

// Node that stores PID:CID mapping
struct pid_node {
        int pid;
        int cid;
        int valid;
        struct pid_node *next;
};

// Node that stores OID data
struct oid_node {
        __u64 oid;
        struct mutex *lock;
};

// Node that stores the CID:OID(list) mapping
struct cid_node {
        int cid;
        int valid;
        struct cid_node *next;
};

// Actual list that stores the PID nodes
struct pid_node *pid_list = NULL;

void add_pid_node(int pid, int cid){

        mutex_lock(&pid_list_lock);
        printk("Adding PID: %d to CID: %d\n", pid, cid);
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
        printk("Deleting PID: %d\n", pid);
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
        printk("PID: %d belongs to CID: %d\n", pid, cid);
        return cid;
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

        printk("Lock OID: %d from PID: %d from CID: %d\n", user_cmd_kernal->oid, cid);
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

        printk("Unlock OID: %d from PID: %d from CID: %d\n", user_cmd_kernal->oid, cid);
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
