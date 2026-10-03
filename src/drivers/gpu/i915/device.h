/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * Device start and stop.
 *
 * PCI attach only records the device.  The hardware is brought up later by a
 * start worker, once the kernel reports that regular threads and timed waits
 * work; the two events may arrive in either order.
 */

#ifndef DRIVERS_GPU_I915_DEVICE_H
#define DRIVERS_GPU_I915_DEVICE_H

struct i915_device;

void drv_i915_device_start_ops_install(void);
void drv_i915_device_schedule_start(struct i915_device *device);
int drv_i915_device_start(struct i915_device *device);
int drv_i915_device_stop(struct i915_device *device);
void drv_i915_device_attach_settled(struct i915_device *device);

#endif
