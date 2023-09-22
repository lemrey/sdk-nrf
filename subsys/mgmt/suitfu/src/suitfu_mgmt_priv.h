/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#ifndef SUITFU_MGMT_PRIV_H_
#define SUITFU_MGMT_PRIV_H_

#include <stdint.h>
#include <zcbor_common.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IMG_MGMT_VER_MAX_STR_LEN 32
/* The value here sets how many "characteristics" that describe image is
 * encoded into a map per each image (like bootable flags, and so on).
 * This value is only used for zcbor to predict map size and map encoding
 * and does not affect memory allocation.
 * In case when more "characteristics" are added to image map then
 * zcbor_map_end_encode may fail it this value does not get updated.
 */
#define MAX_IMG_CHARACTERISTICS	 15
#define IMG_MGMT_HASH_STR	 48
#define IMG_MGMT_HASH_LEN	 32
#define IMG_MGMT_DATA_SHA_LEN	 32 /* SHA256 */

/*
 * Command IDs for image management group.
 */
#define IMG_MGMT_ID_STATE    0
#define IMG_MGMT_ID_UPLOAD   1
#define IMG_MGMT_ID_FILE     2
#define IMG_MGMT_ID_CORELIST 3
#define IMG_MGMT_ID_CORELOAD 4
#define IMG_MGMT_ID_ERASE    5

/** Represents an individual upload request. */
struct img_mgmt_upload_req {
	uint32_t image; /* 0 by default. */
	size_t off;	/* SIZE_MAX if unspecified. */
	size_t size;	/* SIZE_MAX if unspecified. */
	struct zcbor_string img_data;
	struct zcbor_string data_sha;
	bool upgrade; /* Only allow greater version numbers. */
} __packed;

#ifdef __cplusplus
}
#endif

#endif /* __IMG_PRIV_H */
