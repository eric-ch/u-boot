/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef __CHROMEOS_AB_H
#define __CHROMEOS_AB_H

struct blk_desc;

/* Number of slots */
#define NUM_SLOTS 2

/**
 * Select the slot where to boot from.
 *
 * ChromiumOS implements a slotted upgrade logic with at least two kernel
 * partitions being present.
 * https://chromium.googlesource.com/chromiumos/docs/+/HEAD/disk_format.md#selecting-the-kernel
 * The partition containing the Android boot image to be booted is identified by
 * the firmware using some extended GPT partition table attributes describing
 * more or less the same information as what could be found in the "misc"
 * partition, in the bootloader message, for Android.
 * This is used during OTA upgrades. The upgrade will be written to an inactive
 * slot. Upon reboot, either the system successfully comes up and updates the
 * relevant GPT entry attributes or the firmware makes the decision to switch
 * back to the original slot after a number of failed boot attempts.
 *
 * @param[in] dev_desc Place to store boot_a device description pointer
 * @param[out] slot_name Buffer PARTNAME_SZ+1 long for the partition label.
 * @param[out] slot_uuid Buffer UUID_BIN_LEN+1 long for the partition UUID.
 * Return: 0 on success, or a negative on error.
 */

int chromeos_get_next_slot(struct blk_desc *dev_desc, char *part_name,
			   char *part_uuid);

#endif /* __CHROMEOS_AB_H */
