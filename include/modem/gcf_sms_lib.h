/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef GCF_SMS_LIB_H_
#define GCF_SMS_LIB_H_

/**
 * @file gcf_sms_lib.h
 *
 * @defgroup gcf_sms_lib AT commands for GCF SMS support.
 *
 * @{
 *
 * @brief Public API for adding AT commands for GCF SMS support on application side.
 *
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the GCF SMS library.
 */
int gcf_sms_lib_init(void);

/** @} */

#endif /* GCF_SMS_LIB_H_ */
