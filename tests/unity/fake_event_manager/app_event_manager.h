/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Event manager header.
 */

#ifndef _APP_EVENT_MANAGER_H_
#define _APP_EVENT_MANAGER_H_

#include <zephyr/kernel.h>
#include <stdbool.h>
/**
 * @defgroup event_manager Fake Event Manager
 * @brief Fake Event Manager
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Event header.
 *
 * When defining an event structure, the event header
 * must be placed as the first field.
 */
struct app_event_header {
	/** Linked list node used to chain events. */
	sys_snode_t node;

	/** Pointer to the event type object. */
	const struct event_type *type_id;
};


/** Create an event listener object.
 *
 * @param lname   Module name.
 * @param cb_fn  Pointer to the event handler function.
 */
#define APP_EVENT_LISTENER(lname, cb_fn)

/** @brief Subscribe a listener to an event type as first module that is
 *  being notified.
 *
 * @param lname  Name of the listener.
 * @param ename  Name of the event.
 */
#define APP_EVENT_SUBSCRIBE_FIRST(lname, ename)

/** Subscribe a listener to the early notification list for an
 *  event type.
 *
 * @param lname  Name of the listener.
 * @param ename  Name of the event.
 */
#define APP_EVENT_SUBSCRIBE_EARLY(lname, ename)

/** Subscribe a listener to the normal notification list for an event
 *  type.
 *
 * @param lname  Name of the listener.
 * @param ename  Name of the event.
 */
#define APP_EVENT_SUBSCRIBE(lname, ename)

/** Subscribe a listener to an event type as final module that is
 *  being notified.
 *
 * @param lname  Name of the listener.
 * @param ename  Name of the event.
 */
#define APP_EVENT_SUBSCRIBE_FINAL(lname, ename)

/** Declare an event type.
 *
 * This macro provides declarations required for an event to be used
 * by other modules.
 *
 * @param ename  Name of the event.
 */
#define APP_EVENT_TYPE_DECLARE(ename)

/** Declare an event type with dynamic data size.
 *
 * This macro provides declarations required for an event to be used
 * by other modules.
 * Declared event will use dynamic data.
 *
 * @param ename  Name of the event.
 */
#define APP_EVENT_TYPE_DYNDATA_DECLARE(ename)

/** Define an event type.
 *
 * This macro defines an event type. In addition, it defines functions
 * specific to the event type and the event type structure.
 *
 * For every defined event, the following functions are created, where
 * <i>%event_type</i> is replaced with the given event type name @p ename
 * (for example, button_event):
 * - new_<i>%event_type</i>  - Allocates an event of a given type.
 * - is_<i>%event_type</i>   - Checks if the event header that is provided
 *                            as argument represents the given event type.
 * - cast_<i>%event_type</i> - Casts the event header that is provided
 *                            as argument to an event of the given type.
 *
 * @param ename            Name of the event.
 * @param init_log_en      Bool indicating if the event is logged
 *                         by default.
 * @param log_fn           Function to stringify an event of this type.
 * @param ev_info_struct   Data structure describing the event type.
 */
#define APP_EVENT_TYPE_DEFINE(ename, init_log_en, log_fn, ev_info_struct)

/** Verify if an event ID is valid.
 *
 * The pointer to an event type structure is used as its ID. This macro
 * validates that the provided pointer is within the range where event
 * type structures are defined.
 *
 * @param id  ID.
 */
#define APP_EVENT_ASSERT_ID(id)

/** Submit an event.
 *
 * This helper macro simplifies the event submission.
 *
 * @param event  Pointer to the event object.
 */
#define APP_EVENT_SUBMIT(event)

/**
 * @brief Register event hook after the Application Event Manager is initialized.
 *
 * The event hook called after the app_event_manager is initialized to provide some additional
 * initialization of the modules that depends on it.
 * The hook function should have a form `int hook(void)`.
 * If the initialization hook returns non-zero value the initialization process is interrupted.
 *
 * @param hook_fn Hook function.
 */
#define APP_EVENT_MANAGER_HOOK_POSTINIT_REGISTER(hook_fn)

/**
 * @brief Register hook called on event submission. The hook would be called first.
 *
 * The event hook called when the event is submitted.
 * The hook function should have a form `void hook(const struct app_event_header *aeh)`.
 * The macro makes sure that the hook provided here is called first.
 * Only one hook can be registered with this macro.
 *
 * @note
 * The registered hook may be called from many contexts.
 * To ensure that order of events in the queue matches the order of the registered callbacks calls,
 * the callbacks are called under the same spinlock as adding events to the queue.
 *
 * @param hook_fn Hook function.
 */
#define APP_EVENT_HOOK_ON_SUBMIT_REGISTER_FIRST(hook_fn)

/**
 * @brief Register event hook on submission.
 *
 * The event hook called when the event is submitted.
 * The hook function should have a form `void hook(const struct app_event_header *aeh)`.
 *
 * @note
 * The registered hook may be called from many contexts.
 * To ensure that order of events in the queue matches the order of the registered callbacks calls,
 * the callbacks are called under the same spinlock as adding events to the queue.
 *
 * @param hook_fn Hook function.
 */
#define APP_EVENT_HOOK_ON_SUBMIT_REGISTER(hook_fn)

/**
 * @brief Register event hook on submission. The hook would be called last.
 *
 * The event hook called when the event is submitted.
 * The hook function should have a form `void hook(const struct app_event_header *aeh)`.
 * The macro makes sure that the hook provided here is called last.
 * Only one hook can be registered with this macro.
 *
 * @note
 * The registered hook may be called from many contexts.
 * To ensure that order of events in the queue matches the order of the registered callbacks calls,
 * the callbacks are called under the same spinlock as adding events to the queue.
 *
 * @param hook_fn Hook function.
 */
#define APP_EVENT_HOOK_ON_SUBMIT_REGISTER_LAST(hook_fn)

/**
 * @brief Register event hook on the start of event processing. The hook would be called first.
 *
 * The hook function should have a form `void hook(const struct app_event_header *aeh)`.
 * The macro makes sure that the hook provided here is called first.
 * Only one hook can be registered with this macro.
 *
 * @param hook_fn Hook function.
 */
#define APP_EVENT_HOOK_PREPROCESS_REGISTER_FIRST(hook_fn)

/**
 * @brief Register event hook on the start of event processing.
 *
 * The event hook called when the event is being processed.
 * The hook function should have a form `void hook(const struct app_event_header *aeh)`.
 *
 * @param hook_fn Hook function.
 */
#define APP_EVENT_HOOK_PREPROCESS_REGISTER(hook_fn)

/**
 * @brief Register event hook on the start of event processing. The hook would be called last.
 *
 * The event hook called when the event is being processed.
 * The hook function should have a form `void hook(const struct app_event_header *aeh)`.
 * The macro makes sure that the hook provided here is called last.
 * Only one hook can be registered with this macro.
 *
 * @param hook_fn Hook function.
 */
#define APP_EVENT_HOOK_PREPROCESS_REGISTER_LAST(hook_fn)

/**
 * @brief Register event hook on the end of event processing. The hook would be called first.
 *
 * The event hook called after the event is processed.
 * The hook function should have a form `void hook(const struct app_event_header *aeh)`.
 * The macro makes sure that the hook provided here is called first.
 * Only one hook can be registered with this macro.
 *
 * @param hook_fn Hook function.
 */
#define APP_EVENT_HOOK_POSTPROCESS_REGISTER_FIRST(hook_fn)

/**
 * @brief Register event hook on the end of event processing.
 *
 * The event hook called after the event is processed.
 * The hook function should have a form `void hook(const struct app_event_header *aeh)`.
 *
 * @param hook_fn Hook function.
 */
#define APP_EVENT_HOOK_POSTPROCESS_REGISTER(hook_fn)

/**
 * @brief Register event hook on the end of event processing. The hook would be called last.
 *
 * The event hook called after the event is processed.
 * The hook function should have a form `void hook(const struct app_event_header *aeh)`.
 * The macro makes sure that the hook provided here is called last.
 * Only one hook can be registered with this macro.
 *
 * @param hook_fn Hook function.
 */
#define APP_EVENT_HOOK_POSTPROCESS_REGISTER_LAST(hook_fn)


/** @brief Initialize the Application Event Manager.
 *
 * @retval 0 If the operation was successful. Error values can be added by the hooks registered
 *         by @ref APP_EVENT_MANAGER_HOOK_POSTINIT_REGISTER macro.
 */
int app_event_manager_init(void);

/** @brief Allocate event.
 *
 * The behavior of this function depends on the actual implementation.
 * The default implementation of this function is same as k_malloc.
 * It is annotated as weak and can be overridden by user.
 *
 * @param size  Amount of memory requested (in bytes).
 * @retval Address of the allocated memory if successful, otherwise NULL.
 **/
void *app_event_manager_alloc(size_t size);


/** @brief Free memory occupied by the event.
 *
 * The behavior of this function depends on the actual implementation.
 * The default implementation of this function is same as k_free.
 * It is annotated as weak and can be overridden by user.
 *
 * @param addr  Pointer to previously allocated memory.
 **/
void app_event_manager_free(void *addr);


/** @brief Log event.
 *
 * This helper macro simplifies event logging.
 *
 * @param aeh  Pointer to the application event header of the event that is
 *             processed by app_event_manager.
 * @param ... `printf`- like format string and variadic list of arguments corresponding to
 *            the format string.
 */
#define APP_EVENT_MANAGER_LOG(aeh, ...)


/**
 * @brief Define flags for event type.
 *
 * @param ... Comma-separated list of flags which should be set.
 *            In case no flags should be set leave it empty.
 */
#define APP_EVENT_FLAGS_CREATE(...) __VA_ARGS__


#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _APP_EVENT_MANAGER_H_ */
