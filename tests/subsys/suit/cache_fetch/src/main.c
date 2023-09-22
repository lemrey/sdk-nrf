/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <sink.h>
#include <memptr_sink.h>
#include <cache_streamer.h>
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

ZTEST_SUITE(cache_streamer_tests, NULL, NULL, test_suite_before, test_suite_after, NULL);

ZTEST(cache_streamer_tests, test_cache_streamer_ok)
{
	struct stream_sink memptr_sink;
	memptr_storage_handle handle = NULL;
	char ok_uri[] = "http://databucket.com";
	size_t ok_uri_len = sizeof("http://databucket.com");
	uint8_t *payload_ptr;
	size_t payload_size = 0;

	int ret = get_memptr_storage(&handle);
	zassert_equal(ret, 0, "get_memptr_storage failed - error %i", ret);

	ret = get_memptr_sink(&memptr_sink, handle);
	zassert_equal(ret, 0, "get_memptr_sink failed - error %i", ret);

	ret = cache_streamer(ok_uri, ok_uri_len, &memptr_sink);
	zassert_equal(ret, 0, "cache_streamer failed - error %i", ret);

	ret = get_memptr_ptr(handle, &payload_ptr, &payload_size);
	zassert_equal(ret, 0, "memptr_storage.get failed - error %i", ret);

	ret = release_memptr_storage(handle);
	zassert_equal(ret, 0, "memptr_storage.release failed - error %i", ret);
}

ZTEST(cache_streamer_tests, test_cache_streamer_nok)
{
	struct stream_sink memptr_sink;
	memptr_storage_handle handle = NULL;
	char ok_uri[] = "http://databucket.com";
	size_t ok_uri_len = sizeof("http://databucket.com");
	char nok_uri[] = "http://data123.com";
	size_t nok_uri_len = sizeof("http://data123.com");

	int ret = get_memptr_storage(&handle);
	zassert_equal(ret, 0, "get_memptr_storage failed - error %i", ret);

	ret = get_memptr_sink(&memptr_sink, handle);
	zassert_equal(ret, 0, "get_memptr_sink failed - error %i", ret);

	ret = cache_streamer(NULL, ok_uri_len, &memptr_sink);
	zassert_equal(ret, INVALID_ARGUMENT, "cache_streamer should have failed - uri == NULL");

	ret = cache_streamer(ok_uri, 0, &memptr_sink);
	zassert_equal(ret, INVALID_ARGUMENT, "cache_streamer should have failed - uri_len == 0");

	ret = cache_streamer(ok_uri, ok_uri_len, NULL);
	zassert_equal(ret, INVALID_ARGUMENT, "cache_streamer should have failed - sink == NULL");

	ret = cache_streamer(nok_uri, nok_uri_len, &memptr_sink);
	zassert_equal(ret, SOURCE_NOT_FOUND, "cache_streamer failed - error %i", ret);

	ret = release_memptr_storage(handle);
	zassert_equal(ret, 0, "memptr_storage.release failed - error %i", ret);
}
