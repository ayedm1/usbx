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
/**   Host Stack                                                          */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/

#define UX_SOURCE_CODE


/* Include necessary system files.  */

#include "ux_api.h"
#include "ux_host_stack.h"


/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    _ux_host_stack_interface_endpoint_get               PORTABLE C      */
/*                                                           6.1.12       */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function returns an endpoint container based on the interface  */
/*    handle and an endpoint index.                                       */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    interface                             Pointer to interface          */
/*    endpoint_index                        Index of endpoint to get      */
/*    endpoint                              Destination for endpoint      */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    Completion Status                                                   */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*    None                                                                */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    Application                                                         */
/*    USBX Components                                                     */
/*                                                                        */
/*  RELEASE HISTORY                                                       */
/*                                                                        */
/*    DATE              NAME                      DESCRIPTION             */
/*                                                                        */
/*  05-19-2020     Chaoqiong Xiao           Initial Version 6.0           */
/*  09-30-2020     Chaoqiong Xiao           Modified comment(s),          */
/*                                            resulting in version 6.1    */
/*  07-29-2022     Chaoqiong Xiao           Modified comment(s),          */
/*                                            fixed parameter/variable    */
/*                                            names conflict C++ keyword, */
/*                                            resulting in version 6.1.12 */
/*                                                                        */
/**************************************************************************/
UINT  _ux_host_stack_interface_endpoint_get(UX_INTERFACE *interface_ptr, UINT endpoint_index, UX_ENDPOINT **endpoint)
{

UINT            current_endpoint_index;
UX_ENDPOINT     *current_endpoint;

    /* If trace is enabled, insert this event into the trace buffer.  */
    UX_TRACE_IN_LINE_INSERT(UX_TRACE_HOST_STACK_INTERFACE_ENDPOINT_GET, interface_ptr, endpoint_index, 0, 0, UX_TRACE_HOST_STACK_EVENTS, 0, 0)

    /* Do a sanity check on the interface handle.  */
    if (interface_ptr -> ux_interface_handle != (ULONG) (ALIGN_TYPE) interface_ptr)
    {

        /* Error trap. */
        _ux_system_error_handler(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_ENUMERATOR, UX_INTERFACE_HANDLE_UNKNOWN);

        /* If trace is enabled, insert this event into the trace buffer.  */
        UX_TRACE_IN_LINE_INSERT(UX_TRACE_ERROR, UX_INTERFACE_HANDLE_UNKNOWN, interface_ptr, 0, 0, UX_TRACE_ERRORS, 0, 0)

        return(UX_INTERFACE_HANDLE_UNKNOWN);
    }

    /* Start with the endpoint attached to the interface.  */
    current_endpoint =  interface_ptr -> ux_interface_first_endpoint;

    /* The first endpoint has the index 0.  */
    current_endpoint_index =  0;

    /* Traverse the list of the endpoints until we found the right one.  */
    while (current_endpoint != UX_NULL)
    {

        /* Check if the endpoint index matches the current one.  */
        if (endpoint_index == current_endpoint_index)
        {

            /* Setup the return endpoint pointer.  */
            *endpoint=current_endpoint;

            /* Return success to the caller.  */
            return(UX_SUCCESS);
        }

        /* Move to the next endpoint.  */
        current_endpoint =  current_endpoint -> ux_endpoint_next_endpoint;

        /* Move to the next index.  */
        current_endpoint_index++;
    }

    /* Error trap. */
    _ux_system_error_handler(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_ENUMERATOR, UX_ENDPOINT_HANDLE_UNKNOWN);

    /* If trace is enabled, insert this event into the trace buffer.  */
    UX_TRACE_IN_LINE_INSERT(UX_TRACE_ERROR, UX_ENDPOINT_HANDLE_UNKNOWN, endpoint, 0, 0, UX_TRACE_ERRORS, 0, 0)

    /* Return an error!  */
    return(UX_ENDPOINT_HANDLE_UNKNOWN);
}


/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    _uxe_host_stack_interface_endpoint_get              PORTABLE C      */
/*                                                           6.3.0        */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function checks errors in host stack endpoint get function     */
/*    call.                                                               */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    interface_ptr                         Pointer to interface          */
/*    endpoint_index                        Index of endpoint to get      */
/*    endpoint                              Destination for endpoint      */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    None                                                                */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*    _ux_host_stack_interface_endpoint_get Endpoint get                  */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    Application                                                         */
/*                                                                        */
/*  RELEASE HISTORY                                                       */
/*                                                                        */
/*    DATE              NAME                      DESCRIPTION             */
/*                                                                        */
/*  10-31-2023     Chaoqiong Xiao           Initial Version 6.3.0         */
/*                                                                        */
/**************************************************************************/
UINT  _uxe_host_stack_interface_endpoint_get(UX_INTERFACE *interface_ptr, UINT endpoint_index, UX_ENDPOINT **endpoint)
{

    /* Sanity checks.  */
    if ((interface_ptr == UX_NULL) || (endpoint == UX_NULL))
        return(UX_INVALID_PARAMETER);

    /* Invoke endpoint get function.  */
    return(_ux_host_stack_interface_endpoint_get(interface_ptr, endpoint_index, endpoint));
}
