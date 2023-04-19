/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef __QCOM_AB_H
#define __QCOM_AB_H

struct blk_desc;

/* Android standard boot slot names are 'a', 'b', 'c', ... */
#define BOOT_SLOT_NAME(slot_num) ('a' + (slot_num))
/* Number of slots */
#define NUM_SLOTS 2

/**
 * Select the slot where to boot from.
 *
 * Qualcomm platforms firmware implement a slotted upgrade logic with at least
 * two kernel partitions being present. It looks a lot like what ChromiumOS
 * does with different offsets and different ways to synchronise the status of
 * the system between the firmware and system:
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

int qcom_get_next_slot(struct blk_desc *dev_desc, char *slot_name);

#endif /* __QCOM_AB_H */
