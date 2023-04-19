// SPDX-License-Identifier: BSD-2-Clause
#include <common.h>
#include <qcom_ab.h>
#include <command.h>
#include <env.h>
#include <part.h>

static int do_ab_qcom_select(struct cmd_tbl *cmdtp, int flag, int argc,
			     char *const argv[])
{
	int ret;
	struct blk_desc *dev_desc;
	char slot[2] = { 0 }; /* one letter, see BOOT_SLOT_NAME. */

	if (argc != 4)
		return CMD_RET_USAGE;

	ret = blk_get_device_by_str(argv[2], argv[3], &dev_desc);
	if (ret < 0)
		return CMD_RET_FAILURE;

	ret = qcom_get_next_slot(dev_desc, slot);
	if (ret)
		return CMD_RET_FAILURE;
	env_set(argv[1], slot);
	printf("QCOM: Booting slot: %s\n", slot);
	return 0;
}

U_BOOT_CMD(ab_qcom_select, 4, 0, do_ab_qcom_select,
	   "Set the slot name in 'slot_var_name' for the selected one and\n"
	   "update the GPT metadata for this boot.\n",
	   "<slot_var_name> <interface> <dev>\n"
	   "    - Search all partitions on device type 'interface' instance\n"
	   "      'dev' and identify the next slot to use for the slotted\n"
	   "      boot. This function updates the metadata used to identify\n"
	   "      the next slot. Repeated calls to this function may return\n"
	   "      different results.\n"
);
