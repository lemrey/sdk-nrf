/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief nRF Thread Border Router's main function
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/net/mdns_responder.h>

#include "mdns_publisher.h"

#define RECORDS_NUM CONFIG_NRF_TBR_MDNS_PUBLISHER_RECORDS
#define SET_REC_STR(dst, src) memcpy(dst, src, MIN(strlen(src) + 1, sizeof(dst)))

static struct k_mem_slab dns_sd_rec_content_slab;

static struct dns_sd_rec dns_sd_rec_pool[RECORDS_NUM];
static struct mdns_publisher_rec dns_sd_rec_content[RECORDS_NUM];

void mdns_publisher_init()
{
	k_mem_slab_init(&dns_sd_rec_content_slab, dns_sd_rec_content,
			sizeof(struct mdns_publisher_rec), RECORDS_NUM);

	for (int i = 0; i < RECORDS_NUM; ++i) {
		dns_sd_rec_pool[i].instance = dns_sd_rec_content[i].instance;
		dns_sd_rec_pool[i].service = dns_sd_rec_content[i].service;
		dns_sd_rec_pool[i].proto = dns_sd_rec_content[i].proto;
		dns_sd_rec_pool[i].domain = dns_sd_rec_content[i].domain;
		dns_sd_rec_pool[i].text = dns_sd_rec_content[i].txt;
		dns_sd_rec_pool[i].port = &dns_sd_rec_content[i].port;
		dns_sd_rec_pool[i].text_size = 1; /* mDNS responder requires it */

		dns_sd_rec_content[i].sd_record_ptr = &dns_sd_rec_pool[i];
	}
}

void mdns_publisher_record_update(struct mdns_publisher_rec *record,
				  const char *instance, const char *service,
				  const char *proto, const char *domain,
				  const char *txt, size_t txt_size, uint16_t port)
{
	if (record) {
		SET_REC_STR(record->instance, instance);
		SET_REC_STR(record->service, service);
		SET_REC_STR(record->proto, proto);
		SET_REC_STR(record->domain, domain);

		memcpy(record->txt, txt, txt_size);
		record->sd_record_ptr->text_size = txt_size;

		record->port = port;
	}
}

int mdns_publisher_record_alloc(struct mdns_publisher_rec **record)
{
	return k_mem_slab_alloc(&dns_sd_rec_content_slab, (void **)record, K_NO_WAIT);
}

void mdns_publisher_record_free(struct mdns_publisher_rec **record)
{
	struct dns_sd_rec *sd_rec_ptr;

	if (*record == NULL) {
		return;
	}
	/* keep the mapping to mDNS reponder's record */
	sd_rec_ptr = (*record)->sd_record_ptr;

	memset(*record, 0, sizeof(**record));
	(*record)->sd_record_ptr = sd_rec_ptr;

	k_mem_slab_free(&dns_sd_rec_content_slab, (void **)record);
}
