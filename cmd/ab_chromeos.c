// SPDX-License-Identifier: BSD-2-Clause
#include <common.h>
#include <chromeos_ab.h>
#include <command.h>
#include <env.h>
#include <part.h>

static int do_ab_chromeos_select(struct cmd_tbl *cmdtp, int flag, int argc,
				 char *const argv[])
{
	int ret;
	struct blk_desc *dev_desc;
	char slot_name[PARTNAME_SZ + 1];
	char slot_uuid[UUID_BIN_LEN + 1];

	if (argc != 5)
		return CMD_RET_USAGE;

	ret = blk_get_device_by_str(argv[2], argv[3], &dev_desc);
	if (ret < 0)
		return CMD_RET_FAILURE;

	ret = chromeos_get_next_slot(dev_desc, slot_name, slot_uuid);
	if (ret)
		return CMD_RET_FAILURE;
	env_set(argv[1], slot_name);
	env_set(argv[2], slot_uuid);
	printf("CHROMEOS: Booting slot: %s (%s)\n", slot_name, slot_uuid);
	return 0;
}

U_BOOT_CMD(ab_chromeos_select, 5, 0, do_ab_chromeos_select,
	   "Set the partition name and uuid for the slot selected and\n"
	   "updated for this boot.\n",
	   "<slot_var_name> <slot_var_uuid> <interface> <dev>\n"
	   "    - Search all partitions on device type 'interface' instance\n"
	   "      'dev' and identify the next one to use for the slotted\n"
	   "      boot. This function updates the metadata used to identify\n"
	   "      the next slot. Repeated calls to this function may return\n"
	   "      different results.\n"
);
