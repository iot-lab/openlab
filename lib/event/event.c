/*
 * This file is part of HiKoB Openlab.
 *
 * HiKoB Openlab is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation, version 3.
 *
 * HiKoB Openlab is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with HiKoB Openlab. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * Copyright (C) 2011,2012 HiKoB.
 */

/*
 * event_rtos.c
 *
 *  Created on: Jul 6, 2011
 *      Author: Clément Burin des Roziers <clement.burin-des-roziers.at.hikob.com>
 */

#include <stdint.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "event.h"
#include "event_priorities.h"
#include "event_custom.h"
#include "printf.h"
#include "debug.h"

#include "soft_timer.h"

#ifndef EVENT_QUEUE_LENGTH
#define EVENT_QUEUE_LENGTH 12
#endif

#ifndef EVENT_HALT_ON_POST_ERROR
#define EVENT_HALT_ON_POST_ERROR 1
#endif


// prototypes
static void event_task(void *param);
static void event_queue_init(struct event_queue *e_queue);

// data
static struct event_queue *event_queues = NULL;
static unsigned int num_event_queues = 0;


void event_init(void)
{
    static struct event_queue default_event_queues[2];
    /* Default previous configuration */
    default_event_queues[0].priority = event_priorities[0];
    default_event_queues[0].stack_size = 4 * configMINIMAL_STACK_SIZE;
    // DEFAULT default_event_queues[0].queue_length

    default_event_queues[1].priority = event_priorities[1];
    // DEFAULT default_event_queues[0].stack_size
    // DEFAULT default_event_queues[0].queue_length

    event_init_with_queues(default_event_queues, 2);
}


void event_init_with_queues(struct event_queue *ev_queues, int num)
{
    if (event_queues != NULL && event_queues != ev_queues) {
        log_warning("Event already init with different ev_queues %x:%x",
                event_queues, ev_queues);
        return;
    }
    event_queues = ev_queues;
    num_event_queues = num;
    int i;

    for (i = 0; i < num; i++) {
        struct event_queue *e_queue = &event_queues[i];
        e_queue->num = i;
        event_queue_init(e_queue);
    }
}


#define DEFAULT_VALUE_IF_0(variable, value) \
    variable = (variable ? variable : (value))


static void event_queue_init(struct event_queue *e_queue)
{
    if (e_queue->queue != NULL)
        return;
    e_queue->current_event.event = NULL;
    e_queue->current_event.event_arg = NULL;

    // Default values
    DEFAULT_VALUE_IF_0(e_queue->stack_size, configMINIMAL_STACK_SIZE);
    DEFAULT_VALUE_IF_0(e_queue->queue_length, EVENT_QUEUE_LENGTH);

    char name[configMAX_TASK_NAME_LEN];
    snprintf(name, configMAX_TASK_NAME_LEN, "evt%d", e_queue->num);

    e_queue->queue = xQueueCreate(e_queue->queue_length, sizeof(queue_entry_t));
    if (e_queue->queue == NULL)
    {
            log_error("Failed to create the event queue #%d!", e_queue->num);
            HALT();
    }

    xTaskCreate(event_task, (signed char *)name,
            e_queue->stack_size, (void *) e_queue->num,
            e_queue->priority, &e_queue->task);
    log_info("Priority of event task #%d: %u/%u", e_queue->num,
            e_queue->priority, configMAX_PRIORITIES - 1);

    if (e_queue->task == NULL)
    {
        log_error("Failed to create the event task #%d!", e_queue->num);
        HALT();
    }
}



event_status_t event_post(event_queue_t queue, handler_t event,
                          handler_arg_t arg)
{
    queue_entry_t entry;

    // Fill the entry
    entry.event = event;
    entry.event_arg = arg;

    // Send to Queue
    if (xQueueSendToBack(event_queues[queue].queue, &entry, 0) == pdTRUE)
    {
        return EVENT_OK;
    }
    else
    {
        log_error("Failed to post to queue #%u, current event: %x", queue,
            entry.event);
#if EVENT_HALT_ON_POST_ERROR
        HALT();
#endif
        return EVENT_FULL;
    }
}

event_status_t event_post_from_isr(event_queue_t queue, handler_t event,
                                   handler_arg_t arg)
{
    queue_entry_t entry;
    portBASE_TYPE yield = pdFALSE;

    // Fill the entry
    entry.event = event;
    entry.event_arg = arg;

    // Send to Queue
    if (xQueueSendToBackFromISR(event_queues[queue].queue, &entry, &yield) == pdTRUE)
    {
        if (yield)
        {
            // The event task should yield!
            vPortYieldFromISR();
        }
        else
        {
        }

        return EVENT_OK;
    }
    else
    {
        log_error("Failed to post to queue #%u from ISR, current event: %x",
                 queue, entry.event);
#if EVENT_HALT_ON_POST_ERROR
        HALT();
#endif
        return EVENT_FULL;
    }
}

static void event_task(void *param)
{
    uint32_t num = (uint32_t) param;
    xQueueHandle queue = event_queues[num].queue;
    queue_entry_t *entry = &event_queues[num].current_event;

    // Infinite loop
    while (1)
    {
        // Get next event
        entry->event = NULL;

        if (xQueueReceive(queue, entry, portMAX_DELAY) == pdTRUE)
        {
            // Call the event
            entry->event(entry->event_arg);
        }
        else
        {
            log_error("Failed to receive from queue #%d", num);
            HALT();
        }
    }
}

void event_debug()
{
#if RELEASE == 0
    uint32_t i;
    log_printf("Debugging Queues...\n");

    for (i = 0; i <= num_event_queues; i++)
    {
        struct event_queue *e_queue = &event_queues[i];

        log_printf("Queue #%u Current event:  %08x (%08x)", i,
                e_queue->current_event.event,
                e_queue->current_event.event_arg);

        int count = uxQueueMessagesWaiting(e_queue->queue);
        log_printf(", %u waiting:\n", count);

        queue_entry_t e;
        while ((xQueueReceive(e_queue->queue, &e, 0) == pdTRUE))
        {
            log_printf("\tevt: %08x (%08x)\n", e.event, e.event_arg);
        }
    }
#endif
}
