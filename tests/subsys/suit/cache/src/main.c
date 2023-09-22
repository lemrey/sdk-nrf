/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <suit_cache.h>

/*
  {"http://source1.com": h'4235623423462346456234623487723572702975',
	"http://source2.com.no": h'25672519384710',
	"ftp://altsource.com": h'92384859284720'}
*/
static const uint8_t cache[] = {
	0xBF, 0x72, 0x68, 0x74, 0x74, 0x70, 0x3A, 0x2F, 0x2F, 0x73, 0x6F, 0x75, 0x72, 0x63, 0x65,
	0x31, 0x2E, 0x63, 0x6F, 0x6D, 0x54, 0x42, 0x35, 0x62, 0x34, 0x23, 0x46, 0x23, 0x46, 0x45,
	0x62, 0x34, 0x62, 0x34, 0x87, 0x72, 0x35, 0x72, 0x70, 0x29, 0x75, 0x75, 0x68, 0x74, 0x74,
	0x70, 0x3A, 0x2F, 0x2F, 0x73, 0x6F, 0x75, 0x72, 0x63, 0x65, 0x32, 0x2E, 0x63, 0x6F, 0x6D,
	0x2E, 0x6E, 0x6F, 0x47, 0x25, 0x67, 0x25, 0x19, 0x38, 0x47, 0x10, 0x73, 0x66, 0x74, 0x70,
	0x3A, 0x2F, 0x2F, 0x61, 0x6C, 0x74, 0x73, 0x6F, 0x75, 0x72, 0x63, 0x65, 0x2E, 0x63, 0x6F,
	0x6D, 0x47, 0x92, 0x38, 0x48, 0x59, 0x28, 0x47, 0x20, 0xFF};
static const size_t cache_len = sizeof(cache);

/*
	{"http://databucket.com": h'4360021135853785764409444542662512368400086117',
	"http://storagehole.com": h'053714514299994946548928821768209760220451452304',
	"#file.bin": h'12358902317049812091623890476012378490701892365192830986701923'}
*/
static const uint8_t cache2[] = {
	0xA3, 0x75, 0x68, 0x74, 0x74, 0x70, 0x3A, 0x2F, 0x2F, 0x64, 0x61, 0x74, 0x61, 0x62,
	0x75, 0x63, 0x6B, 0x65, 0x74, 0x2E, 0x63, 0x6F, 0x6D, 0x57, 0x43, 0x60, 0x02, 0x11,
	0x35, 0x85, 0x37, 0x85, 0x76, 0x44, 0x09, 0x44, 0x45, 0x42, 0x66, 0x25, 0x12, 0x36,
	0x84, 0x00, 0x08, 0x61, 0x17, 0x76, 0x68, 0x74, 0x74, 0x70, 0x3A, 0x2F, 0x2F, 0x73,
	0x74, 0x6F, 0x72, 0x61, 0x67, 0x65, 0x68, 0x6F, 0x6C, 0x65, 0x2E, 0x63, 0x6F, 0x6D,
	0x58, 0x18, 0x05, 0x37, 0x14, 0x51, 0x42, 0x99, 0x99, 0x49, 0x46, 0x54, 0x89, 0x28,
	0x82, 0x17, 0x68, 0x20, 0x97, 0x60, 0x22, 0x04, 0x51, 0x45, 0x23, 0x04, 0x69, 0x23,
	0x66, 0x69, 0x6C, 0x65, 0x2E, 0x62, 0x69, 0x6E, 0x58, 0x1F, 0x12, 0x35, 0x89, 0x02,
	0x31, 0x70, 0x49, 0x81, 0x20, 0x91, 0x62, 0x38, 0x90, 0x47, 0x60, 0x12, 0x37, 0x84,
	0x90, 0x70, 0x18, 0x92, 0x36, 0x51, 0x92, 0x83, 0x09, 0x86, 0x70, 0x19, 0x23};
static const size_t cache2_len = sizeof(cache2);

static void test_suite_before(void *f)
{
	struct suit_cache suit_caches;

	zassert_between_inclusive(2, 1, CONFIG_SUIT_CACHE_MAX_CACHES,
				  "Failed to prepare test fixture: cache istoo small");

	suit_caches.partitions[0].address = (uint8_t *)cache;
	suit_caches.partitions[0].size = (size_t)cache_len;

	suit_caches.partitions[1].address = (uint8_t *)cache2;
	suit_caches.partitions[1].size = (size_t)cache2_len;
	suit_caches.partitions_count = 2;

	int rc = suit_cache_initialize(&suit_caches);
	zassert_equal(rc, 0, "Failed to initialize cache: %i", rc);
}

static void test_suite_after(void *f)
{
	suit_cache_deinitialize();
}

ZTEST_SUITE(cache_tests, NULL, NULL, test_suite_before, test_suite_after, NULL);

ZTEST(cache_tests, test_suit_cache_search_ok)
{
	struct zcbor_string payload;
	struct zcbor_string ok_uri = {.value = "http://databucket.com",
				      .len = sizeof("http://databucket.com")};

	int ret = suit_cache_search(&ok_uri, &payload);
	zassert_equal(ret, 0, "\nGet from cache failed");
}

ZTEST(cache_tests, test_suit_cache_search_hash_ok)
{
	struct zcbor_string payload;
	struct zcbor_string ok_uri = {.value = "#file.bin", .len = sizeof("#file.bin")};

	int ret = suit_cache_search(&ok_uri, &payload);
	zassert_equal(ret, 0, "\nGet from cache failed");
}

ZTEST(cache_tests, test_suit_cache_search_nok)
{
	struct zcbor_string payload;
	struct zcbor_string nok_uri = {.value = "http://data123.com",
				       .len = sizeof("http://data123.com")};

	int ret = suit_cache_search(&nok_uri, &payload);
	zassert_not_equal(ret, 0, "\nGet from cache should have failed");
}

ZTEST(cache_tests, test_suit_cache_search_nok_uri_null)
{
	struct zcbor_string payload;

	int ret = suit_cache_search(NULL, &payload);
	zassert_not_equal(ret, 0, "\nGet from cache should have failed");
}

ZTEST(cache_tests, test_suit_cache_search_nok_payload_null)
{
	struct zcbor_string ok_uri = {.value = "http://databucket.com",
				      .len = sizeof("http://databucket.com")};

	int ret = suit_cache_search(&ok_uri, NULL);
	zassert_not_equal(ret, 0, "\nGet from cache should have failed");
}

ZTEST(cache_tests, test_suit_cache_search_key_is_substring)
{
	struct zcbor_string payload;
	struct zcbor_string ok_uri = {.value = "http://databucket.com/subdir/",
				      .len = sizeof("http://databucket.com/subdir/")};

	int ret = suit_cache_search(&ok_uri, &payload);
	zassert_not_equal(ret, 0, "\nGet from cache should have failed");
}

ZTEST(cache_tests, test_suit_cache_search_empty)
{
	struct zcbor_string payload;
	struct zcbor_string ok_uri = {.value = "http://databucket.com",
				      .len = sizeof("http://databucket.com")};

	suit_cache_deinitialize();

	int ret = suit_cache_search(&ok_uri, &payload);
	zassert_not_equal(ret, 0, "\nGet from cache should have failed");
}
