/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief nRF Thread Border Router's main function
 */

#include <stdint.h>
#include <zephyr/net/dns_sd.h>

#define MDNS_PUBLISHER_INSTANCE_LEN 64
#define MDNS_PUBLISHER_SERVICE_LEN 16
#define MDNS_PUBLISHER_DOMAIN_LEN 64
#define MDNS_PUBLISHER_PROTO_LEN 5
#define MDNS_PUBLISHER_TXT_LEN CONFIG_NRF_TBR_NRF_TBR_MDNS_PUBLISHER_MAX_TXT_LEN

struct mdns_publisher_rec {
	struct dns_sd_rec *sd_record_ptr;
	uint16_t port;
	char service[MDNS_PUBLISHER_SERVICE_LEN];
	char proto[MDNS_PUBLISHER_PROTO_LEN];
	char instance[MDNS_PUBLISHER_INSTANCE_LEN];
	char domain[MDNS_PUBLISHER_DOMAIN_LEN];
	char txt[MDNS_PUBLISHER_TXT_LEN];
};

void mdns_publisher_init();

int mdns_publisher_record_alloc(struct mdns_publisher_rec **record);

void mdns_publisher_record_update(struct mdns_publisher_rec *record,
				  const char *instance, const char *service,
				  const char *proto, const char *domain,
				  const char *txt, size_t txt_size, uint16_t port);

void mdns_publisher_record_free(struct mdns_publisher_rec **record);
