/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Utility functions for nRF TBR.
 *
 */

#include <zephyr/kernel.h>

#ifdef CONFIG_LITTLE_ENDIAN
#define CHECKSUM_BIG_ENDIAN 0
#else
#define CHECKSUM_BIG_ENDIAN 1
#endif

static uint16_t offset_based_swap8(const uint8_t *data)
{
	uint16_t data16 = (uint16_t)*data;

	if (((uintptr_t)(data) & 1) == CHECKSUM_BIG_ENDIAN) {
		return data16;
	} else {
		return data16 << 8;
	}
}

/**
 * @brief Function for recomputing checksum.
 *
 * @param chksum	Old checksum to be recalculated.
 * @param data		Input data.
 * @param len		Number of bytes to be recomputed.
 * @param comp		Flag indicating whether input of @ref data should use ones' complement
 *			during recomputing.
 */
static void recompute_chksum(uint32_t *chksum, const uint8_t *data, size_t len, bool comp)
{
	uint32_t *p;
	size_t pending = len;
	size_t i = 0;

	p = (uint32_t *)data;
	while (pending >= sizeof(uint32_t)) {
		*chksum += (comp ? ~(p[i]) : p[i]);
		*chksum += (*chksum < (comp ? ~(p[i]) : p[i]));

		i++;
		pending -= sizeof(uint32_t);
	}
	data = (uint8_t *)(p + i);
	if (pending >= 2) {
		*chksum += (comp ? ~(*(uint16_t *)(data)) : *(uint16_t *)(data));
		*chksum += (*chksum < (comp ? ~(*(uint16_t *)(data)) : *(uint16_t *)(data)));

		data += sizeof(uint16_t);
		pending -= sizeof(uint16_t);
	}
	if (pending == 1) {
		uint16_t tmp = offset_based_swap8(data);

		*chksum += (comp ? ~tmp : tmp);
		*chksum += (*chksum < (comp ? ~tmp : tmp));
	}
}

uint16_t recompute_chksum_ipv4(uint16_t sum_in, const uint8_t *data_old,
			       const uint8_t *data_new, size_t len)
{
	/* According to the rfc1624 the checksum can be recomputed using
	 * following equation [Eqn. 3]:
	 * new_chksum = ~(~old_checksum + ~old_value + new_value)
	 */

	/* sum = ~old_checksum */
	uint32_t sum = ~sum_in;

	if (!data_old || !data_new || len == 0) {
		return sum_in;
	}

	/* inside following function is computed:
	 * sum + ~old_value => ~old_checksum + ~old_value
	 */
	recompute_chksum(&sum, data_old, len, true);
	/* and here:
	 * sum + new_value => (~old_checksum + ~old_value) + new_value
	 */
	recompute_chksum(&sum, data_new, len, false);

	/* Fold sum into 16-bit word. */
	while (sum >> 16) {
		sum = (sum & 0xffff) + (sum >> 16);
	}

	/* Finally ~sum => ~(~old_checksum + ~old_value + new_value) */
	return ~sum;
}
