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
#include <chromeos_ab.h>

typedef union _chromeos_boot_attributes {
	struct {
		u64 priority:4;
		u64 remaining_tries:4;
		u64 successful:1;
		u64 reserved:7;
	} fields;
	unsigned short raw;
} __packed chromeos_attributes;

static inline int _part_disabled(const gpt_entry *e)
{
	chromeos_attributes a =
		{ .raw = e->attributes.fields.type_guid_specific };

	return !a.fields.successful &&
		!a.fields.remaining_tries &&
		!a.fields.priority;
}

static inline int _part_bootable(const gpt_entry *e)
{
	chromeos_attributes a =
		{ .raw = e->attributes.fields.type_guid_specific };

	return a.fields.successful || a.fields.remaining_tries;
}

static inline int _part_compare(const gpt_entry *e, const gpt_entry *f)
{
	chromeos_attributes a =
		{ .raw = e->attributes.fields.type_guid_specific };
	chromeos_attributes b =
		{ .raw = f->attributes.fields.type_guid_specific };

	return b.fields.priority - a.fields.priority;
}

static inline void _part_set_unbootable(gpt_entry *e)
{
	chromeos_attributes a =
		{ .raw = e->attributes.fields.type_guid_specific };

	a.fields.successful = 0;
	a.fields.remaining_tries = 0;
	a.fields.priority = 0;
	e->attributes.fields.type_guid_specific = a.raw;
}

static inline void _part_tries_decrement(gpt_entry *e)
{
	chromeos_attributes a =
		{ .raw = e->attributes.fields.type_guid_specific };

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

int chromeos_get_next_slot(struct blk_desc *dev_desc, char *part_name,
			   char *part_uuid)
{
	ALLOC_CACHE_ALIGN_BUFFER_PAD(gpt_header, gpt_h, 1, dev_desc->blksz);
	gpt_entry *gpt_e = NULL;
	gpt_entry *kernel_entries[NUM_SLOTS], *kernel_next = NULL;
	int i, n, len;
	char name[PARTNAME_SZ + 1];
	bool update = false;
	efi_guid_t ptype = PARTITION_CHROMEOS_KERNEL;
	int ret;

	ret = read_gpt_table(dev_desc, gpt_h, &gpt_e);
	if (ret < 0) {
		log_err("CHROMEOS: No GPT found.\n");
		return -EINVAL;
	}

	for (n = 0, i = 0; i < gpt_h->num_partition_entries; ++i) {
		if (memcmp(gpt_e[i].partition_type_guid.b,
			   &ptype, sizeof (efi_guid_t)))
			continue;
		if (n >= NUM_SLOTS) {
			log_err("CHROMEOS: More slots that supported "
				"(maximum: %d).\n", NUM_SLOTS);
			ret = -EINVAL;
			goto out;
		}
		kernel_entries[n++] = &gpt_e[i];
		len = ucs2_to_ascii(gpt_e[i].partition_name, PARTNAME_SZ,
				    name, PARTNAME_SZ);
		name[len] = '\0';
		log_debug("CHROMEOS: Found kernel partition slot: \"%s\".\n",
			  name);
	}
	for (i = 0; i < n; ++i) {
		gpt_entry *entry = kernel_entries[i];

		if (_part_disabled(entry))
			continue;
		if (!_part_bootable(entry)) {
			len = ucs2_to_ascii(gpt_e[i].partition_name, PARTNAME_SZ,
					    name, PARTNAME_SZ);
			name[len] = '\0';
			log_debug("CHROMEOS: Set partition \"%s\" unbootable.\n",
				  name);
			_part_set_unbootable(entry);
			update = true;
			continue;
		}
		if (!kernel_next) {
			kernel_next = entry;
			continue;
		}
		if (kernel_next && _part_compare(kernel_next, entry) > 0)
			kernel_next = entry;
	}
	if (!kernel_next) {
		log_err("CHROMEOS: No bootable kernel partition was found.\n");
		goto out;
	}
	_part_tries_decrement(kernel_next);
	update = true;
	len = ucs2_to_ascii(kernel_next->partition_name, PARTNAME_SZ,
			    part_name, PARTNAME_SZ);
	part_name[len] = '\0';
	uuid_bin_to_str(kernel_next->unique_partition_guid.b, part_uuid,
			UUID_STR_FORMAT_GUID);
	log_debug("CHROMEOS: Next slot is \"%s\" (%s).\n", part_name, part_uuid);
out:
	if (update) {
		log_debug("CHROMEOS: Update GPT.\n");
		write_gpt_table(dev_desc, gpt_h, gpt_e);
	}

	free(gpt_e);
	return 0;
}
