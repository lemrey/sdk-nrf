/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
/*
 * DESCRIPTION
 * This module contains test for data cache
 * on single core, using dcache debug access.
 */
#include <zephyr/ztest.h>
#include <zephyr/cache.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <hal/nrf_cache.h>

BUILD_ASSERT(IS_ENABLED(CONFIG_DCACHE), "This test requires DCACHE to be enabled");

#define TEST_PRINT_CACHE_CONTENT IS_ENABLED(CONFIG_TEST_PRINT_CACHE_CONTENT)

#define CACHEINFO_REG ((NRF_CACHEINFO_Type*)(((uintptr_t)NRF_DCACHEINFO) & ~0x10000000))
#define CACHEDATA_REG ((NRF_CACHEINFO_Type*)(((uintptr_t)NRF_DCACHEDATA) & ~0x10000000))

#define CACHE_REG NRF_DCACHE

#define CACHE_LINE_SIZE CONFIG_DCACHE_LINE_SIZE

#define _TEST_MEM_BLOCK_INIT(_node) \
	{ .data = (uint32_t *)DT_REG_ADDR(_node), .size = DT_REG_SIZE(_node) / sizeof(uint32_t) }

#define TEST_MEM_BLOCK_INIT(_id) _TEST_MEM_BLOCK_INIT(DT_NODELABEL(CONCAT(test_mem_block, _id)))

struct test_block_data {
	uint32_t *data;
	size_t size;
};

struct test_block_data test_blocks[4] = {
	TEST_MEM_BLOCK_INIT(0),
	TEST_MEM_BLOCK_INIT(1),
	TEST_MEM_BLOCK_INIT(2),
	TEST_MEM_BLOCK_INIT(3),
};

/**
 * @brief Get the value expected on given block
 *
 * @param idx Block index
 * @param n   Byte inside a block
 * @return The value expected in selected test block on selected byte position
 */
static inline uint32_t block_mem_val(size_t idx, size_t n)
{
	return (uint32_t)((idx + 0x1234) * n);
}

/**
 * @brief Get the position in cache set
 *
 * @param ptr Pointer to the memory that we wish to find in cache
 *
 * @return Set position in cache
 */
static inline uint32_t cache_get_set(const uint32_t *ptr)
{
	return (((uint32_t)ptr) / CACHE_LINE_SIZE) & (ARRAY_SIZE(CACHEINFO_REG->SET) - 1U);
}

/**
 * @brief Get the tag in cache
 *
 * @param ptr Pointer to the memory that we wish to find in cache
 *
 * @return Tag in cache for the given address
 */
static inline uint32_t cache_get_tag(const uint32_t *ptr)
{
	return (((uint32_t)ptr) / CACHE_LINE_SIZE) / ARRAY_SIZE(CACHEINFO_REG->SET);
}

/**
 * @brief Convert given set and tag to the line start address
 *
 * @param tag Cache tag
 * @param set Cache set
 *
 * @return The physical address of the RAM with given tag and set
 */
static inline uint8_t *cache_get_addr(uint32_t tag, uint32_t set)
{
	return (uint8_t *)((tag * ARRAY_SIZE(CACHEINFO_REG->SET) + set) * CACHE_LINE_SIZE);
}

/**
 * @brief Make sure test blocks are filled with the expected data.
 */
static inline void prepare_test_blocks(void)
{
	for (size_t idx = 0; idx < ARRAY_SIZE(test_blocks); ++idx) {
		for (size_t n = 0; n < test_blocks[idx].size; ++n) {
			test_blocks[idx].data[n] = block_mem_val(idx, n);
		}
	}

	cache_data_flush_and_invd_all();
}

/**
 * @brief Read selected test block index
 *
 * Reading the test block should update cache content.
 *
 * @param idx Test block index to be loaded
 */
static inline void load_test_block(size_t idx)
{
	volatile uint32_t temp;

	for (size_t n = 0; n < test_blocks[idx].size; ++n) {
		temp = test_blocks[idx].data[n];
	}
}

static inline void check_cache_tag(const uint32_t *p_data, uint8_t way)
{
	uint32_t expected_tag = cache_get_tag(p_data);
	uint32_t expected_set = cache_get_set(p_data);
	uint32_t tag = nrf_cache_tag_get(CACHEINFO_REG, expected_set, way);

	zassert_equal(cache_get_tag(p_data), tag,
		      "Expected tag 0x%x does not match 0x%x at set 0x%x way %u",
		      expected_tag, tag, expected_set, way);
}

static void print_cache_content(void)
{
#if TEST_PRINT_CACHE_CONTENT
	printk("----------------------------------------------------------\n");
	for (uint32_t set = 0; set < ARRAY_SIZE(CACHEINFO_REG->SET); ++set) {
		printk("0x%x", set * CACHE_LINE_SIZE);
		for (uint8_t way = 0; way < 2; ++way) {
			printk(" 0x%x|%s%s%s",
			       nrf_cache_tag_get(CACHEINFO_REG, set, way) * CACHE_LINE_SIZE,
			       nrf_cache_line_validity_check(CACHEINFO_REG, set, way) ? "v" : "i",
			       nrf_cache_is_data_unit_dirty_check(CACHEINFO_REG, set, way, 0) ? "d" : "c",
			       nrf_cache_is_data_unit_dirty_check(CACHEINFO_REG, set, way, 2) ? "d" : "c");
		}
		printk("\n");
		k_msleep(20);
	}
	printk("----------------------------------------------------------\n");
#endif
}

/*
 * Prepare a test.
 * This function makes sure dcache is in known state.
 */
static void *suite_setup(void)
{
	cache_data_enable();
	prepare_test_blocks();

	return NULL;
}

static void test_teardown(void *fixture)
{
	(void)fixture;
	nrf_cache_update_lock_set(CACHE_REG, false);
	cache_data_flush_all();
}

ZTEST(dcache_single, test_check_sizes)
{
	for (size_t i = 0; i < ARRAY_SIZE(test_blocks); ++i) {
		zassert_equal(8*1024, test_blocks[i].size * sizeof(uint32_t),
			      "Wrong test size of %u memory block", i);
	}
}

ZTEST(dcache_single, test_read)
{
	uint32_t set;
	uint8_t way;

	for (size_t idx = 0; idx < 2; ++idx) {
		load_test_block(idx);
	}

	nrf_cache_update_lock_set(CACHE_REG, true);

	/* Check validity */
	for (set = 0; set < ARRAY_SIZE(CACHEINFO_REG->SET); ++set) {
		for (way = 0; way < 2; ++way) {
			zassert_true(nrf_cache_line_validity_check(CACHEINFO_REG, set, way),
				     "Invalid cache line at set 0x%x, way: %u", set, way);
		}
	}

	print_cache_content();

	/* Check tags for the data that was read ealier */
	for (const uint32_t *p_data = test_blocks[0].data;
	     p_data < test_blocks[0].data + test_blocks[0].size;
	     p_data += CACHE_LINE_SIZE) {
		uint8_t mru = nrf_cache_mru_get(CACHEINFO_REG, cache_get_set(p_data));

		check_cache_tag(p_data, mru ? 0 : 1);
	}
	/* Check tags for the data that was read last */
	for (const uint32_t *p_data = test_blocks[1].data;
	     p_data < test_blocks[1].data + test_blocks[1].size;
	     p_data += CACHE_LINE_SIZE) {
		uint8_t mru = nrf_cache_mru_get(CACHEINFO_REG, cache_get_set(p_data));

		check_cache_tag(p_data, mru);
	}
}

ZTEST(dcache_single, test_invalidate_range)
{
	for (size_t idx = 0; idx < 2; ++idx) {
		load_test_block(idx);
	}

	/* Check invalidating of the region */
	uint32_t set;
	uint8_t way;
	uint8_t *inv_addr = ((uint8_t *)test_blocks[0].data) + CACHE_LINE_SIZE;
	size_t inv_size = 2 * CACHE_LINE_SIZE;

	cache_data_invd_range(inv_addr, inv_size);
	nrf_cache_update_lock_set(CACHE_REG, true);

	print_cache_content();

	for (way = 0; way < 2; ++way) {
		for (set = 0; set < ARRAY_SIZE(CACHEINFO_REG->SET); ++set) {
			uint32_t tag = nrf_cache_tag_get(CACHEINFO_REG, set, way);
			uint8_t *addr = cache_get_addr(tag, set);
			bool in_inv_range = ((addr >= inv_addr) && (addr < inv_addr + inv_size));

			if (in_inv_range) {
				zassert_false(nrf_cache_line_validity_check(CACHEINFO_REG, set, way),
					      "Cache line at set 0x%x, way: %u should be invalid!",
					      set, way);
			} else {
				zassert_true(nrf_cache_line_validity_check(CACHEINFO_REG, set, way),
					      "Cache line at set 0x%x, way: %u should be valid!",
					      set, way);
			}

		}
	}
}

ZTEST(dcache_single, test_invalidate_out_of_range)
{
	for (size_t idx = 0; idx < 2; ++idx) {
		load_test_block(idx);
	}

	/* Check invalidating of the region */
	uint32_t set;
	uint8_t way;
	uint8_t *inv_addr = ((uint8_t *)test_blocks[3].data) + CACHE_LINE_SIZE;
	size_t inv_size = 2 * CACHE_LINE_SIZE;

	cache_data_invd_range(inv_addr, inv_size);
	nrf_cache_update_lock_set(CACHE_REG, true);

	print_cache_content();

	for (way = 0; way < 2; ++way) {
		for (set = 0; set < ARRAY_SIZE(CACHEINFO_REG->SET); ++set) {
			uint32_t tag = nrf_cache_tag_get(CACHEINFO_REG, set, way);
			uint8_t *addr = cache_get_addr(tag, set);
			bool in_inv_range = ((addr >= inv_addr) && (addr < inv_addr + inv_size));

			if (in_inv_range) {
				zassert_false(nrf_cache_line_validity_check(CACHEINFO_REG, set, way),
					      "Cache line at set 0x%x, way: %u should be invalid!",
					      set, way);
			} else {
				zassert_true(nrf_cache_line_validity_check(CACHEINFO_REG, set, way),
					      "Cache line at set 0x%x, way: %u should be valid!",
					      set, way);
			}
		}
	}
}

ZTEST(dcache_single, test_dirty_flags)
{
	for (size_t idx = 0; idx < 2; ++idx) {
		load_test_block(idx);
	}

	uint32_t set;
	uint8_t way;
	uint8_t *dirty_addr = ((uint8_t *)test_blocks[0].data) + CACHE_LINE_SIZE;
	size_t dirty_size = 2 * CACHE_LINE_SIZE;

	for (uint8_t *pos = dirty_addr; pos < dirty_addr + dirty_size; ++pos) {
		*pos = ~(*pos);
	}

	nrf_cache_update_lock_set(CACHE_REG, true);

	print_cache_content();

	for (way = 0; way < 2; ++way) {
		for (set = 0; set < ARRAY_SIZE(CACHEINFO_REG->SET); ++set) {
			uint32_t tag = nrf_cache_tag_get(CACHEINFO_REG, set, way);
			uint8_t *addr = cache_get_addr(tag, set);
			bool in_dirty_range = ((addr >= dirty_addr) &&
					       (addr < dirty_addr + dirty_size));

			if (in_dirty_range) {
				zassert_true(nrf_cache_is_data_unit_dirty_check(CACHEINFO_REG,
										set, way, 0),
					     "Cache line at set 0x%x, way: %u should be dirty",
					     set, way);
				zassert_true(nrf_cache_is_data_unit_dirty_check(CACHEINFO_REG,
										set, way, 2),
					     "Cache line at set 0x%x, way: %u should be dirty",
					     set, way);
			} else {
				zassert_false(nrf_cache_is_data_unit_dirty_check(CACHEINFO_REG,
										 set, way, 0),
					      "Cache line at set 0x%x, way: %u should be clean",
					      set, way);
				zassert_false(nrf_cache_is_data_unit_dirty_check(CACHEINFO_REG,
										 set, way, 2),
					      "Cache line at set 0x%x, way: %u should be clean",
					      set, way);
			}
		}
	}
}

ZTEST(dcache_single, test_wb)
{
	for (size_t idx = 0; idx < 2; ++idx) {
		load_test_block(idx);
	}

	uint32_t set;
	uint8_t way;
	uint8_t *dirty_addr1 = ((uint8_t *)test_blocks[0].data) + CACHE_LINE_SIZE;
	size_t dirty_size1 = 2 * CACHE_LINE_SIZE;
	uint8_t *dirty_addr2 = ((uint8_t *)test_blocks[1].data) + CACHE_LINE_SIZE;
	size_t dirty_size2 = 2 * CACHE_LINE_SIZE;

	for (uint8_t *pos = dirty_addr1; pos < dirty_addr1 + dirty_size1; ++pos) {
		*pos = ~(*pos);
	}
	for (uint8_t *pos = dirty_addr2; pos < dirty_addr2 + dirty_size2; ++pos) {
		*pos = ~(*pos);
	}

	cache_data_flush_range(dirty_addr1, dirty_size1);
	nrf_cache_update_lock_set(CACHE_REG, true);

	print_cache_content();

	for (way = 0; way < 2; ++way) {
		for (set = 0; set < ARRAY_SIZE(CACHEINFO_REG->SET); ++set) {
			uint32_t tag = nrf_cache_tag_get(CACHEINFO_REG, set, way);
			uint8_t *addr = cache_get_addr(tag, set);
			bool in_dirty_range = ((addr >= dirty_addr2) &&
					       (addr < dirty_addr2 + dirty_size2));

			if (in_dirty_range) {
				zassert_true(nrf_cache_is_data_unit_dirty_check(CACHEINFO_REG,
										set, way, 0),
					     "Cache line at set 0x%x, way: %u should be dirty",
					     set, way);
				zassert_true(nrf_cache_is_data_unit_dirty_check(CACHEINFO_REG,
										set, way, 2),
					     "Cache line at set 0x%x, way: %u should be dirty",
					     set, way);
			} else {
				zassert_false(nrf_cache_is_data_unit_dirty_check(CACHEINFO_REG,
										 set, way, 0),
					      "Cache line at set 0x%x, way: %u should be clean",
					      set, way);
				zassert_false(nrf_cache_is_data_unit_dirty_check(CACHEINFO_REG,
										 set, way, 2),
					      "Cache line at set 0x%x, way: %u should be clean",
					      set, way);
			}
		}
	}
}

ZTEST(dcache_single, test_wb_out_of_range)
{
	for (size_t idx = 0; idx < 2; ++idx) {
		load_test_block(idx);
	}

	uint32_t set;
	uint8_t way;
	uint8_t *dirty_addr1 = ((uint8_t *)test_blocks[0].data) + CACHE_LINE_SIZE;
	size_t dirty_size1 = 2 * CACHE_LINE_SIZE;
	uint8_t *dirty_addr2 = ((uint8_t *)test_blocks[1].data) + CACHE_LINE_SIZE;
	size_t dirty_size2 = 2 * CACHE_LINE_SIZE;

	for (uint8_t *pos = dirty_addr1; pos < dirty_addr1 + dirty_size1; ++pos) {
		*pos = ~(*pos);
	}
	for (uint8_t *pos = dirty_addr2; pos < dirty_addr2 + dirty_size2; ++pos) {
		*pos = ~(*pos);
	}

	cache_data_flush_range(test_blocks[3].data, dirty_size1);
	nrf_cache_update_lock_set(CACHE_REG, true);

	print_cache_content();

	for (way = 0; way < 2; ++way) {
		for (set = 0; set < ARRAY_SIZE(CACHEINFO_REG->SET); ++set) {
			uint32_t tag = nrf_cache_tag_get(CACHEINFO_REG, set, way);
			uint8_t *addr = cache_get_addr(tag, set);
			bool in_dirty_range = ((addr >= dirty_addr1) &&
					       (addr < dirty_addr1 + dirty_size1))
					      ||
					      ((addr >= dirty_addr2) &&
					       (addr < dirty_addr2 + dirty_size2));

			if (in_dirty_range) {
				zassert_true(nrf_cache_is_data_unit_dirty_check(CACHEINFO_REG,
										set, way, 0),
					     "Cache line at set 0x%x, way: %u should be dirty",
					     set, way);
				zassert_true(nrf_cache_is_data_unit_dirty_check(CACHEINFO_REG,
										set, way, 2),
					     "Cache line at set 0x%x, way: %u should be dirty",
					     set, way);
			} else {
				zassert_false(nrf_cache_is_data_unit_dirty_check(CACHEINFO_REG,
										 set, way, 0),
					      "Cache line at set 0x%x, way: %u should be clean",
					      set, way);
				zassert_false(nrf_cache_is_data_unit_dirty_check(CACHEINFO_REG,
										 set, way, 2),
					      "Cache line at set 0x%x, way: %u should be clean",
					      set, way);
			}
		}
	}
}

/*
 * Test entry point
 */
ZTEST_SUITE(dcache_single, NULL, suite_setup, NULL, test_teardown, NULL);
