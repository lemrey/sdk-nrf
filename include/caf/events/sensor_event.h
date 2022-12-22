/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _SENSOR_EVENT_H_
#define _SENSOR_EVENT_H_

/**
 * @file
 * @defgroup caf_sensor_event CAF Sensor Event
 * @{
 * @brief CAF Sensor Event.
 */

#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>
#include <zephyr/drivers/sensor.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* This value has to be equal to fractional part of the sensor_value. */
#define FLOAT_TO_SENSOR_VAL_CONST 1000000

/** @brief Sensor states. */
enum sensor_state {
	/** Initial state of the sensor. The state is used only before sensor is initialized and
	 *  it should not be broadcasted using @ref sensor_state_event.
	 */
	SENSOR_STATE_DISABLED,

	/** Sensor is sleeping and no sampling is performed. */
	SENSOR_STATE_SLEEP,

	/** Sensor is sampled periodically. */
	SENSOR_STATE_ACTIVE,

	/** Sensor error occurred. The sensor is no longer sampled. */
	SENSOR_STATE_ERROR,

	/** Number of sensor states. */
	SENSOR_STATE_COUNT,

	/** Unused in code, required for inter-core compatibility. */
	APP_EM_ENFORCE_ENUM_SIZE(SENSOR_STATE)
};

/** @brief Sensor state event.
 *
 * The sensor state event is submitted when state of a sensor changes.
 *
 * The description field is a pointer to a string that is used to identify the sensor by the
 * application. The Common Application Framework does not impose any standard way of describing
 * sensors. Format and content of the sensor description is defined by the application.
 *
 * @note The sensor state event related to the given sensor must use the same description as
 *       #sensor_event related to the sensor.
 */
struct sensor_state_event {
	struct app_event_header header; /**< Event header. */

	const char *descr; /**< Description of the sensor. */
	enum sensor_state state; /**< New state of the sensor. */
};

APP_EVENT_TYPE_DECLARE(sensor_state_event);

/** @brief Sensor event.
 *
 * The sensor event is submitted when a sensor is sampled.
 *
 * The description field is a pointer to a string that is used to identify the sensor by the
 * application. The Common Application Framework does not impose any standard way of describing
 * sensors. Format and content of the sensor description is defined by the application.
 *
 * The dyndata contains sensor readouts represented as array of floating-point values. Content of
 * the array depends only on selected sensor. For example an accelerometer may report acceleration
 * in X, Y and Z axis as three floating-point values. @ref sensor_event_get_data_cnt and @ref
 * sensor_event_get_data_ptr can be used to access the sensor data provided by a given sensor event.
 *
 * @note The sensor event related to the given sensor must use the same description as
 *       #sensor_state_event related to the sensor.
 */
struct sensor_event {
	struct app_event_header header; /**< Event header. */

	const char *descr; /**< Description of the sensor. */
	struct event_dyndata dyndata; /**< Sensor data. Provided as floating-point values. */
};

/** @brief Get size of sensor data.
 *
 * @param[in] event       Pointer to the sensor_event.
 *
 * @return Size of the sensor data, expressed as a number of struct sensor_value.
 */
static inline size_t sensor_event_get_data_cnt(const struct sensor_event *event)
{
	__ASSERT_NO_MSG((event->dyndata.size % sizeof(struct sensor_value)) == 0);

	return (event->dyndata.size / sizeof(struct sensor_value));
}

/** @brief Get pointer to the sensor data.
 *
 * @param[in] event       Pointer to the sensor_event.
 *
 * @return Pointer to the sensor data.
 */
static inline struct sensor_value *sensor_event_get_data_ptr(const struct sensor_event *event)
{
	return (struct sensor_value *)event->dyndata.data;
}

/**
 * @brief Helper function for checking if one sensor_value is greater than the other.
 *
 * @param sensor_val1 First compered sensor value.
 * @param sensor_val2 Second compered sensor value.
 * @return True if sensor_val1 is greater than sensor_val2, false otherwise.
 */
static inline bool sensor_value_greater_then(struct sensor_value sensor_val1,
					     struct sensor_value sensor_val2)
{
	return ((sensor_val1.val1 > sensor_val2.val1) ||
		((sensor_val1.val1 == sensor_val2.val1) && (sensor_val1.val2 > sensor_val2.val2)));
}

/**
 * @brief Helper function for calculating absolute value of difference of two sensor_values.
 *
 * @param sensor_val1 First sensor value.
 * @param sensor_val2 Second sensor value.
 * @return Absolute value of difference between sensor_val1 and sensor_val2.
 */
static inline struct sensor_value sensor_value_abs_difference(struct sensor_value sensor_val1,
							      struct sensor_value sensor_val2)
{
	struct sensor_value result;

	result.val1 = abs(sensor_val1.val1 - sensor_val2.val1);
	result.val2 = abs(sensor_val1.val2 - sensor_val2.val2);
	if (result.val2 > FLOAT_TO_SENSOR_VAL_CONST) {
		result.val2 = result.val2 - FLOAT_TO_SENSOR_VAL_CONST;
		result.val1++;
	}
	return result;
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#ifdef __cplusplus
extern "C" {
#endif

APP_EVENT_TYPE_DYNDATA_DECLARE(sensor_event);

#ifdef __cplusplus
}
#endif

#endif /* _SENSOR_EVENT_H_ */
