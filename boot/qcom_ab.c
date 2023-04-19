// SPDX-License-Identifier: BSD-2-Clause
#include <common.h>
#include <blk.h>
#include <log.h>
#include <linux/ctype.h>
#include <linux/types.h>
#include <linux/err.h>
#include <memalign.h>
#include <part.h>
#include <part_efi.h>
#include <qcom_ab.h>

typedef union _qcom_boot_attributes {
	struct {
		u64 priority:2;
		u64 active:1;
		u64 remaining_tries:3;
		u64 successful:1;
		u64 unbootable:1;
		u64 reserved:8;
	} fields;
	unsigned short raw;
} __packed qcom_attributes;

static inline int _part_disabled(const gpt_entry *e)
{
	qcom_attributes a = { .raw = e->attributes.fields.type_guid_specific };

	return a.fields.unbootable;
}

static inline int _part_bootable(const gpt_entry *e)
{
	qcom_attributes a = { .raw = e->attributes.fields.type_guid_specific };

	return !a.fields.unbootable && a.fields.remaining_tries;
}

static inline int _part_compare(const gpt_entry *e, const gpt_entry *f)
{
	qcom_attributes a = { .raw = e->attributes.fields.type_guid_specific };
	qcom_attributes b = { .raw = f->attributes.fields.type_guid_specific };

	return b.fields.priority - a.fields.priority;
}

static inline void _part_set_unbootable(gpt_entry *e)
{
	qcom_attributes a = { .raw = e->attributes.fields.type_guid_specific };

	a.fields.unbootable = 1;
	a.fields.successful = 0;
	a.fields.remaining_tries = 0;
	a.fields.priority = 0;
	e->attributes.fields.type_guid_specific = a.raw;
}

static inline void _part_tries_decrement(gpt_entry *e)
{
	qcom_attributes a = { .raw = e->attributes.fields.type_guid_specific };

	if (a.fields.remaining_tries)
		a.fields.remaining_tries--;
	e->attributes.fields.type_guid_specific = a.raw;
}

static int ucs2_to_ascii(const efi_char16_t *in, unsigned int isize,
			 char *out, unsigned int osize)
{
	unsigned int i, o;

	for (i = 0, o = 0; i < isize && o < osize && in[i]; ++i, ++o) {
		u8 c = in[i] & 0x7f;

		out[o] = in[i] <= 0x7f && isprint(c) ? c : '.';
	}
	return o;
}

static inline int efiname_to_name(const efi_char16_t *efi_name, char *name)
{
	int len;

	len = ucs2_to_ascii(efi_name, PARTNAME_SZ, name, PARTNAME_SZ);
	name[len] = '\0';
	return len;
}

int qcom_get_next_slot(struct blk_desc *dev_desc, char *slot_name)
{
	ALLOC_CACHE_ALIGN_BUFFER_PAD(gpt_header, gpt_h, 1, dev_desc->blksz);
	gpt_entry *gpt_e = NULL;
	gpt_entry *boot_entries[NUM_SLOTS] = { 0 }, *boot_next = NULL;
	char name[PARTNAME_SZ + 1];
	bool update = false;
	efi_guid_t ptype = PARTITION_QCOM_BOOTIMG;
	int ret, i;

	ret = read_gpt_table(dev_desc, gpt_h, &gpt_e);
	if (ret < 0) {
		log_err("QCOM: No GPT found.\n");
		return -EINVAL;
	}

	for (i = 0; i < gpt_h->num_partition_entries; ++i) {
		if (memcmp(gpt_e[i].partition_type_guid.b, &ptype,
			   sizeof (efi_guid_t)))
			continue;

		efiname_to_name(gpt_e[i].partition_name, name);

		if (strncmp(name, "boot_", 5) ||
		    (name[5] < 'a') || (name[5] >= 'a' + NUM_SLOTS)) {
			log_err("QCOM: Qualcomm boot image partition with"
				"unexpected name \"%s\".\n", name);
			continue;
		}
		boot_entries[name[5] - 'a'] = &gpt_e[i];
		log_debug("QCOM: Found boot partition \"%s\".\n", name);
	}

	for (i = 0; i < NUM_SLOTS; ++i) {
		gpt_entry *entry = boot_entries[i];

		if (!entry || _part_disabled(entry))
			continue;
		if (!_part_bootable(entry)) {
			efiname_to_name(entry->partition_name, name);
			log_debug("QCOM: Marking %s unbootable.\n", name);
			_part_set_unbootable(entry);
			update = true;
			continue;
		}
		if (!boot_next) {
			boot_next = entry;
			continue;
		}
		if (_part_compare(boot_next, entry) > 0)
			boot_next = entry;
	}
	if (!boot_next) {
		log_err("QCOM: No bootable kernel partition was found.\n");
		goto out;
	}
	_part_tries_decrement(boot_next);
	update = true;
	efiname_to_name(boot_next->partition_name, name);
	slot_name[0] = name[5];
	slot_name[1] = '\0';
	log_debug("QCOM: Next slot is \"%s\".\n", slot_name);
out:
	if (update) {
		log_debug("QCOM: Update GPT.\n");
		write_gpt_table(dev_desc, gpt_h, gpt_e);
	}

	free(gpt_e);
	return 0;
}
