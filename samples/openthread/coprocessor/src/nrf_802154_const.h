/*
 * Copyright (c) 2021 Nordic Semiconductor
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef NRF_802154_CONST_FIXINCLUDE_H
#define NRF_802154_CONST_FIXINCLUDE_H

#include "autoconf.h"

#include_next "nrf_802154_const.h"

// Override ACK IFS value for timing compatibility with FPGA.
#undef  ACK_IFS
#define ACK_IFS CONFIG_NRF_802154_AIFS_TIME

#endif // NRF_802154_CONST_FIXINCLUDE_H
