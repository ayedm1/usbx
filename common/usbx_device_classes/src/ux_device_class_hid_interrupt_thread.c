/***************************************************************************
 * Copyright (c) 2024 Microsoft Corporation
 *
 * This program and the accompanying materials are made available under the
 * terms of the MIT License which is available at
 * https://opensource.org/licenses/MIT.
 *
 * SPDX-License-Identifier: MIT
 **************************************************************************/

/**************************************************************************/
/**************************************************************************/
/**                                                                       */
/** USBX Component                                                        */
/**                                                                       */
/**   Device HID Class                                                    */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/

#define UX_SOURCE_CODE


/* Include necessary system files.  */

#include "ux_api.h"
#include "ux_device_class_hid.h"
#include "ux_device_stack.h"


#if !defined(UX_DEVICE_STANDALONE)
/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    _ux_device_class_hid_interrupt_thread               PORTABLE C      */
/*                                                           6.3.0        */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function is the thread of the hid interrupt (IN) endpoint      */
/*                                                                        */
/*    It's for RTOS mode.                                                 */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    hid_class                                 Address of hid class      */
/*                                                container               */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    None                                                                */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*    _ux_utility_event_flags_get           Get event flags               */
/*    _ux_device_class_hid_event_get        Get HID event                 */
/*    _ux_device_stack_transfer_request     Request transfer              */
/*    _ux_utility_memory_copy               Copy memory                   */
/*    _ux_device_thread_suspend             Suspend thread                */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    ThreadX                                                             */
/*                                                                        */
/*  RELEASE HISTORY                                                       */
/*                                                                        */
/*    DATE              NAME                      DESCRIPTION             */
/*                                                                        */
/*  05-19-2020     Chaoqiong Xiao           Initial Version 6.0           */
/*  09-30-2020     Chaoqiong Xiao           Modified comment(s),          */
/*                                            verified memset and memcpy  */
/*                                            cases, used UX prefix to    */
/*                                            refer to TX symbols instead */
/*                                            of using them directly,     */
/*                                            resulting in version 6.1    */
/*  10-15-2021     Chaoqiong Xiao           Modified comment(s),          */
/*                                            improved idle generation,   */
/*                                            resulting in version 6.1.9  */
/*  01-31-2022     Chaoqiong Xiao           Modified comment(s),          */
/*                                            off for standalone compile, */
/*                                            resulting in version 6.1.10 */
/*  10-31-2023     Chaoqiong Xiao           Modified comment(s),          */
/*                                            added zero copy support,    */
/*                                            resulting in version 6.3.0  */
/*                                                                        */
/**************************************************************************/
VOID  _ux_device_class_hid_interrupt_thread(ULONG hid_class)
{

UX_SLAVE_CLASS              *class_ptr;
UX_SLAVE_CLASS_HID          *hid;
UX_SLAVE_DEVICE             *device;
UX_SLAVE_TRANSFER           *transfer_request_in;
UX_DEVICE_CLASS_HID_EVENT   *hid_event;
UINT                        status;
UCHAR                       *buffer;
ULONG                       actual_flags;


    /* Cast properly the hid instance.  */
    UX_THREAD_EXTENSION_PTR_GET(class_ptr, UX_SLAVE_CLASS, hid_class)

    /* Get the hid instance from this class container.  */
    hid =  (UX_SLAVE_CLASS_HID *) class_ptr -> ux_slave_class_instance;

    /* Get the pointer to the device.  */
    device =  &_ux_system_slave -> ux_system_slave_device;

    /* This thread runs forever but can be suspended or resumed.  */
    while(1)
    {

        /* All HID events are on the interrupt endpoint IN, from the host.  */
        transfer_request_in =  &hid -> ux_device_class_hid_interrupt_endpoint -> ux_slave_endpoint_transfer_request;

        /* As long as the device is in the CONFIGURED state.  */
        while (device -> ux_slave_device_state == UX_DEVICE_CONFIGURED)
        {

            /* Wait until we have a event sent by the application
               or a change in the idle state to send last or empty report.  */
            status =  _ux_utility_event_flags_get(&hid -> ux_device_class_hid_event_flags_group,
                                                    UX_DEVICE_CLASS_HID_EVENTS_MASK, UX_OR_CLEAR, &actual_flags,
                                                    hid -> ux_device_class_hid_event_wait_timeout);

            /* If there is no event, check if we have timeout defined.  */
            if (status == UX_NO_EVENTS)
            {

                /* There is no event exists on timeout, insert last.  */

                /* Check if no request been ready.  */
                if (transfer_request_in -> ux_slave_transfer_request_requested_length == 0)
                {

                    /* Assume the request use whole interrupt transfer payload.  */
                    transfer_request_in -> ux_slave_transfer_request_requested_length =
                            transfer_request_in -> ux_slave_transfer_request_transfer_length;

#if (UX_DEVICE_ENDPOINT_BUFFER_OWNER == 1) && defined(UX_DEVICE_CLASS_HID_ZERO_COPY)

                    /* Restore endpoint buffer (never touched, filled with zeros).  */
                    transfer_request_in -> ux_slave_transfer_request_data_pointer =
                                    UX_DEVICE_CLASS_HID_INTERRUPTIN_BUFFER(hid);
#else

                    /* Set the data to zeros.  */
                    _ux_utility_memory_set(
                        transfer_request_in -> ux_slave_transfer_request_data_pointer, 0,
                        transfer_request_in -> ux_slave_transfer_request_requested_length); /* Use case of memset is verified. */
#endif
                }

                /* Send the request to the device controller.  */
                status =  _ux_device_stack_transfer_request(transfer_request_in,
                                transfer_request_in -> ux_slave_transfer_request_requested_length,
                                transfer_request_in -> ux_slave_transfer_request_requested_length);

                /* Check error code. We don't want to invoke the error callback
                   if the device was disconnected, since that's expected.  */
                if (status != UX_SUCCESS && status != UX_TRANSFER_BUS_RESET)

                    /* Error trap. */
                    _ux_system_error_handler(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_CLASS, status);

                /* Next: check events.  */
                continue;
            }

            /* Check the completion code. */
            if (status != UX_SUCCESS)
            {

                /* Error trap. */
                _ux_system_error_handler(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_CLASS, status);

                /* Do not proceed.  */
                return;
            }


            /* Check if we have an event to report.  */
            while (_ux_device_class_hid_event_check(hid, &hid_event) == UX_SUCCESS)
            {

#if (UX_DEVICE_ENDPOINT_BUFFER_OWNER == 1) && defined(UX_DEVICE_CLASS_HID_ZERO_COPY)

                /* Directly use the event buffer for transfer.  */
                buffer = hid_event -> ux_device_class_hid_event_buffer;
                transfer_request_in -> ux_slave_transfer_request_data_pointer = buffer;
#else
                /* Prepare the event data payload from the hid event structure.  Get a pointer to the buffer area.  */
                buffer =  transfer_request_in -> ux_slave_transfer_request_data_pointer;

                /* Copy the event buffer into the target buffer.  */
                _ux_utility_memory_copy(buffer, UX_DEVICE_CLASS_HID_EVENT_BUFFER(hid_event), hid_event -> ux_device_class_hid_event_length); /* Use case of memcpy is verified. */
#endif

                /* Send the request to the device controller.  */
                status =  _ux_device_stack_transfer_request(transfer_request_in, hid_event -> ux_device_class_hid_event_length,
                                                            hid_event -> ux_device_class_hid_event_length);

                /* The queue tail is handled and should be freed.  */
                _ux_device_class_hid_event_free(hid);

                /* Check error code. We don't want to invoke the error callback
                   if the device was disconnected, since that's expected.  */
                if (status != UX_SUCCESS && status != UX_TRANSFER_BUS_RESET)

                    /* Error trap. */
                    _ux_system_error_handler(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_CLASS, status);
            }
        }

        /* We need to suspend ourselves. We will be resumed by the device enumeration module.  */
        _ux_device_thread_suspend(&class_ptr -> ux_slave_class_thread);
    }
}
#endif
