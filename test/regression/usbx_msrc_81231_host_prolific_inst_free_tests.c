/* This test is designed to test the simple dpump host/device class operation.  */

#include <stdio.h>
#include "tx_api.h"
#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"

#include "ux_device_class_dummy.h"
#include "ux_device_stack.h"
#include "ux_host_class_prolific.h"

#include "ux_test_dcd_sim_slave.h"
#include "ux_test_hcd_sim_host.h"
#include "ux_test_utility_sim.h"

/* Define constants.  */
#define                             UX_DEMO_DEBUG_SIZE  (4096*8)
#define                             UX_DEMO_STACK_SIZE  1024
#define                             UX_DEMO_BUFFER_SIZE (UX_SLAVE_REQUEST_DATA_MAX_LENGTH + 1)
#define                             UX_DEMO_XMIT_BUFFER_SIZE 512
#define                             UX_DEMO_RECEPTION_BUFFER_SIZE 512
#define                             UX_DEMO_FILE_BUFFER_SIZE 512
#define                             UX_DEMO_RECEPTION_BLOCK_SIZE 64
#define                             UX_DEMO_MEMORY_SIZE     (64*1024)
#define                             UX_DEMO_FILE_SIZE       (128 * 1024)
#define                             UX_RAM_DISK_MEMORY      (256 * 1024)

/* Define local/extern function prototypes.  */
static VOID                                test_thread_entry(ULONG);
static TX_THREAD                           tx_test_thread_host_simulation;
static TX_THREAD                           tx_test_thread_slave_simulation;
static VOID                                tx_test_thread_host_simulation_entry(ULONG);
static VOID                                tx_test_thread_slave_simulation_entry(ULONG);

static VOID                                ux_test_hcd_entry_should_not_be_called(UX_TEST_ACTION *action, VOID *params);
static VOID                                ux_test_hcd_entry_disconnect(UX_TEST_ACTION *action, VOID *params);
static VOID                                ux_test_hcd_entry_set_cfg(UX_TEST_ACTION *action, VOID *params);

/* Define global data structures.  */
static UCHAR                        usbx_memory[UX_DEMO_MEMORY_SIZE + (UX_DEMO_STACK_SIZE * 4)];

static UX_DEVICE                    *device = UX_NULL;
static UINT                         device_stall_count = 3;

static UX_HOST_CLASS_PROLIFIC       *host_prolific = UX_NULL;

static ULONG                        host_device_insertion_counter = 0;
static ULONG                        host_device_removal_counter = 0;

static ULONG                        enum_counter;

static ULONG                        error_counter;
static ULONG                        error_callback_counter = 0;

static ULONG                        set_cfg_counter;

static ULONG                        rsc_mem_alloc_cnt_on_set_cfg;
static ULONG                        rsc_mem_free_on_set_cfg;
static ULONG                        rsc_sem_on_set_cfg;
static ULONG                        rsc_sem_get_on_set_cfg;
static ULONG                        rsc_mutex_on_set_cfg;

static ULONG                        rsc_enum_sem_usage;
static ULONG                        rsc_enum_sem_get_count;
static ULONG                        rsc_enum_mutex_usage;
static ULONG                        rsc_enum_mem_usage;
static ULONG                        rsc_enum_mem_alloc_count;

static ULONG                        rsc_test_sem_usage;
static ULONG                        rsc_test_sem_get_count;
static ULONG                        rsc_test_mutex_usage;
static ULONG                        rsc_test_mem_alloc_count;

static UX_DEVICE_CLASS_DUMMY                *device_dummy = UX_NULL;
static UX_DEVICE_CLASS_DUMMY_PARAMETER      device_dummy_parameter;


/* Define device framework.  */

#define _W0(w)      ( (w)       & 0xFF)
#define _W1(w)      (((w) >> 8) & 0xFF)

#define _CONFIGURATION_DESCRIPTOR(total_len, n_ifc, cfg_val)                    \
    0x09, 0x02, _W0(total_len), _W1(total_len), (n_ifc), (cfg_val),             \
    0x00, 0xc0, 0x32,

#define _INTERFACE_DESCRIPTOR(ifc_n, alt, n_ep, cls, sub, protocol)             \
    0x09, 0x04, (ifc_n), (alt), (n_ep), (cls), (sub), (protocol), 0x00,

#define _ENDPOINT_DESCRIPTOR(addr, attr, pktsize, interval)                     \
    0x07, 0x05, (addr), (attr), _W0(pktsize), _W1(pktsize), (interval),

#define _CFG_TOTAL_LEN (9+9+7+7+7)

#define             STRING_FRAMEWORK_LENGTH                 47
#define             LANGUAGE_ID_FRAMEWORK_LENGTH            2

static unsigned char device_framework_full_speed[] = {

    /* Device descriptor     18 bytes
       0xEF bDeviceClass:    Composite class code
       0x02 bDeviceSubclass: class sub code
       0x00 bDeviceProtocol: Device protocol
       idVendor & idProduct - http://www.linux-usb.org/usb.ids
    */
    /* REF: 0x067b, 0x2303  */
    0x12, 0x01, 0x10, 0x02,
    0x00, 0x00, 0x00,
    0x40,
    0x7B, 0x06,
    0x03, 0x23,
    0x00, 0x02,
    0x01, 0x02, 0x03,
    0x01,

    /* REF: CFG IFC BO BI II */
    _CONFIGURATION_DESCRIPTOR(_CFG_TOTAL_LEN, 1, 1)
    _INTERFACE_DESCRIPTOR(0, 0, 3, 0xFF, 0xFF, 0x00)
    _ENDPOINT_DESCRIPTOR(0x81, 0x03, 16, 0x0B)
    _ENDPOINT_DESCRIPTOR(0x82, 0x02, 64, 0x00)
    _ENDPOINT_DESCRIPTOR(0x03, 0x02, 64, 0x00)
};

#define             DEVICE_FRAMEWORK_LENGTH_FULL_SPEED      sizeof(device_framework_full_speed)
#define             DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED      sizeof(device_framework_full_speed)
#define             device_framework_high_speed             device_framework_full_speed

static unsigned char string_framework[] = {

    /* Manufacturer string descriptor : Index 1 - "AzureRTOS" */
    0x09, 0x04, 0x01, 9,
        'A','z','u','r','e','R','T','O','S',

    /* Product string descriptor : Index 2 - "Test device" */
    0x09, 0x04, 0x02, 14,
        'T','e','s','t',' ',' ',' ',' ','d','e','v','i','c','e',

    /* Serial Number string descriptor : Index 3 - "0001" */
    0x09, 0x04, 0x03, 0x04,
        0x30, 0x30, 0x30, 0x31
};

    /* Multiple languages are supported on the device, to add
       a language besides english, the unicode language code must
       be appended to the language_id_framework array and the length
       adjusted accordingly. */
static unsigned char language_id_framework[] = {

    /* English. */
        0x09, 0x04
};

static UX_TEST_SETUP _SetConfigure = UX_TEST_SETUP_SetConfigure;

/* Test interactions */

static UX_TEST_HCD_SIM_ACTION log_on_SetCfg[] = {
/* function, request to match,
   port action, port status,
   request action, request EP, request data, request actual length, request status,
   status, additional callback,
   no_return */
{   UX_HCD_TRANSFER_REQUEST, &_SetConfigure,
        UX_FALSE, UX_TEST_PORT_STATUS_DISC,
        UX_TEST_SETUP_MATCH_REQ, 0, UX_NULL, 0, 0,
        UX_SUCCESS, ux_test_hcd_entry_set_cfg,
        UX_TRUE}, /* Invoke callback & continue */
{   0   }
};

/* Define the ISR dispatch.  */

extern VOID    (*test_isr_dispatch)(void);


/* Prototype for test control return.  */

void  test_control_return(UINT status);


/* Define the ISR dispatch routine.  */

static void    test_isr(void)
{

    /* For further expansion of interrupt-level testing.  */
}

static UINT test_slave_change_function(ULONG change)
{
    return 0;
}

static UINT test_host_change_function(ULONG event, UX_HOST_CLASS *cls, VOID *inst)
{

UX_HOST_CLASS_PROLIFIC *prolific_inst = (UX_HOST_CLASS_PROLIFIC *) inst;
    // printf("test_host_change_function: event %lx, cls %p, inst %p\n", event, (void*)cls, (void*)inst);
    switch(event)
    {

    case UX_DEVICE_INSERTION:
        host_device_insertion_counter ++;
        host_prolific = prolific_inst;
        break;

    case UX_DEVICE_REMOVAL:
        host_device_removal_counter ++;
        if (host_prolific == prolific_inst)
            host_prolific = UX_NULL;
        break;

    case UX_DEVICE_CONNECTION:
        device = (UX_DEVICE *)inst;
        break;

    case UX_DEVICE_DISCONNECTION:
        if ((VOID *)device == inst)
            device = UX_NULL;
        break;

#if defined(UX_HOST_STANDALONE)
    case UX_STANDALONE_WAIT_BACKGROUND_TASK:
        tx_thread_relinquish();
        break;
#endif

    default:
        break;
    }
    return 0;
}

static VOID    test_dummy_instance_activate(VOID *dummy_instance)
{
    if (device_dummy == UX_NULL)
        device_dummy = (UX_DEVICE_CLASS_DUMMY *)dummy_instance;
}
static VOID    test_dummy_instance_deactivate(VOID *dummy_instance)
{
    if ((VOID*)device_dummy == dummy_instance)
        device_dummy = UX_NULL;
}
static VOID    test_dummy_control_request(UX_DEVICE_CLASS_DUMMY *dummy_instance, UX_SLAVE_TRANSFER *transfer_request)
{
UINT i;
ULONG cmd_type = transfer_request -> ux_slave_transfer_request_setup[0] |
            (transfer_request -> ux_slave_transfer_request_setup[1] << 8);
UCHAR *cmd_buf = transfer_request -> ux_slave_transfer_request_data_pointer;

    if (device_dummy == dummy_instance)
    {
        if (device_stall_count)
        {
            device_stall_count --;
            ux_device_stack_endpoint_stall(transfer_request -> ux_slave_transfer_request_endpoint);
            return;
        }
        switch(cmd_type)
        {
        case 0x01c0:
            cmd_buf[0] = 0x00;
            ux_device_stack_transfer_request(transfer_request, 1, 1);
            return;

        case 0x0140:
            return;

        case 0x2021:
            return;

        case 0x2221:
            return;

        default:
            printf("_dummy_CREQ:");
            for (i = 0; i < 8; i ++)
                printf(" %02x", transfer_request -> ux_slave_transfer_request_setup[i]);
            printf("\n");
            break;
        }
    }
}

static VOID test_ux_error_callback(UINT system_level, UINT system_context, UINT error_code)
{
    error_callback_counter ++;
}

static VOID ux_test_hcd_entry_set_cfg(UX_TEST_ACTION *action, VOID *params)
{

    set_cfg_counter ++;

    rsc_mem_free_on_set_cfg = _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available;

    rsc_mem_alloc_cnt_on_set_cfg = ux_test_utility_sim_mem_alloc_count();

    rsc_sem_on_set_cfg = ux_test_utility_sim_sem_create_count();
    rsc_enum_sem_get_count = ux_test_utility_sim_sem_get_count();
    rsc_mutex_on_set_cfg = ux_test_utility_sim_mutex_create_count();
}

/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void    usbx_msrc_81231_host_prolific_inst_free_tests_application_define(void *first_unused_memory)
#endif
{

UINT                    status;
CHAR *                  stack_pointer;
CHAR *                  memory_pointer;


    printf("Running MSRC 81231 - Host PROLIFIC inst free Test................... ");

    /* Reset testing counts. */
    ux_test_utility_sim_mem_alloc_log_enable(UX_TRUE);
    ux_test_utility_sim_mem_alloc_count_reset();
    ux_test_utility_sim_mutex_create_count_reset();
    ux_test_utility_sim_sem_create_count_reset();
    ux_test_utility_sim_sem_get_count_reset();
    /* Reset error generations */
    ux_test_utility_sim_sem_error_generation_stop();
    ux_test_utility_sim_mutex_error_generation_stop();
    ux_test_utility_sim_sem_get_error_generation_stop();

    /* Initialize the free memory pointer */
    stack_pointer = (CHAR *) usbx_memory;
    memory_pointer = stack_pointer + (UX_DEMO_STACK_SIZE * 4);

    /* Initialize USBX Memory */
    status = ux_system_initialize(memory_pointer, UX_DEMO_MEMORY_SIZE, UX_NULL,0);
    UX_TEST_ASSERT_MESSAGE(status == UX_SUCCESS, "ux_system_initialize failed 0x%x\n", status);

    /* Register the error callback. */
    _ux_utility_error_callback_register(test_ux_error_callback);

    /* The code below is required for installing the host portion of USBX */
    status =  ux_host_stack_initialize(test_host_change_function);
    UX_TEST_ASSERT_MESSAGE(status == UX_SUCCESS, "ux_host_stack_initialize failed 0x%x\n", status);

    /* Register CDC-ACM class.  */
    status =  ux_host_stack_class_register(_ux_system_host_class_prolific_name, ux_host_class_prolific_entry);
    UX_TEST_ASSERT_MESSAGE(status == UX_SUCCESS, "ux_host_stack_class_register failed 0x%x\n", status);

    /* The code below is required for installing the device portion of USBX. No call back for
       device status change in this example. */
    status =  ux_device_stack_initialize(device_framework_high_speed, DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED,
                                       device_framework_full_speed, DEVICE_FRAMEWORK_LENGTH_FULL_SPEED,
                                       string_framework, STRING_FRAMEWORK_LENGTH,
                                       language_id_framework, LANGUAGE_ID_FRAMEWORK_LENGTH,
                                       test_slave_change_function);
    UX_TEST_ASSERT_MESSAGE(status == UX_SUCCESS, "ux_device_stack_initialize failed 0x%x\n", status);

    /* Set the parameters for callback when insertion/extraction of a dummy device.  */
    _ux_utility_memory_set(&device_dummy_parameter, 0, sizeof(device_dummy_parameter));
    device_dummy_parameter.ux_device_class_dummy_parameter_callbacks.ux_device_class_dummy_instance_activate   = test_dummy_instance_activate;
    device_dummy_parameter.ux_device_class_dummy_parameter_callbacks.ux_device_class_dummy_instance_deactivate = test_dummy_instance_deactivate;
    device_dummy_parameter.ux_device_class_dummy_parameter_callbacks.ux_device_class_dummy_control_request =     test_dummy_control_request;
    /* Initialize the device cdc class. This class owns both interfaces starting with 0. */
    status  = ux_device_stack_class_register(_ux_device_class_dummy_name,
                                             _ux_device_class_dummy_entry,
                                             1, 0, &device_dummy_parameter);
    UX_TEST_ASSERT_MESSAGE(status == UX_SUCCESS, "ux_device_stack_class_register failed 0x%x\n", status);

    /* Initialize the simulated device controller.  */
    status =  _ux_test_dcd_sim_slave_initialize();
    UX_TEST_ASSERT_MESSAGE(status == UX_SUCCESS, "_ux_test_dcd_sim_slave_initialize failed 0x%x\n", status);

    /* Register all the USB host controllers available in this system */
    status =  ux_host_stack_hcd_register(_ux_system_host_hcd_simulator_name, _ux_test_hcd_sim_host_initialize,0,0);
    UX_TEST_ASSERT_MESSAGE(status == UX_SUCCESS, "ux_host_stack_hcd_register failed 0x%x\n", status);

    /* Create the main host simulation thread.  */
    status =  tx_thread_create(&tx_test_thread_host_simulation, "tx test host simulation", tx_test_thread_host_simulation_entry, 0,
            stack_pointer, UX_DEMO_STACK_SIZE,
            20, 20, 1, TX_AUTO_START);
    UX_TEST_ASSERT_MESSAGE(status == TX_SUCCESS, "tx_thread_create failed 0x%x\n", status);

    /* Create the main slave simulation  thread.  */
    stack_pointer += UX_DEMO_STACK_SIZE;
    status =  tx_thread_create(&tx_test_thread_slave_simulation, "tx test slave simulation", tx_test_thread_slave_simulation_entry, 0,
            stack_pointer, UX_DEMO_STACK_SIZE,
            20, 20, 1, TX_AUTO_START);
    UX_TEST_ASSERT_MESSAGE(status == TX_SUCCESS, "tx_thread_create failed 0x%x\n", status);

}

static UINT _test_check_host_connection_error(VOID)
{
    if (device_dummy && host_prolific)
        return(UX_SUCCESS);
    if (error_callback_counter >= 3)
        return(UX_SUCCESS);
    return(UX_ERROR);
}

static UINT _test_check_host_connection_success(VOID)
{
    if (device_dummy && host_prolific)
        return(UX_SUCCESS);
    return(UX_ERROR);
}

static UINT _test_check_host_disconnection_success(VOID)
{
    if (device_dummy == UX_NULL && host_prolific == UX_NULL)
        return(UX_SUCCESS);
    return(UX_ERROR);
}

void  tx_test_thread_host_simulation_entry(ULONG arg)
{

UINT                                                status;
ULONG                                               test_n;
ULONG                                               mem_free;
ULONG                                               loop;
ULONG                                               parameter_u32[64/4];
USHORT                                              *parameter_u16 = (USHORT*)parameter_u32;
UCHAR                                               *parameter_u8 = (UCHAR*)parameter_u32;
UX_INTERFACE                                        *interface;


    stepinfo("\n");
    stepinfo(">>>>>>>>>>>>>>>> Test connect\n");
    ux_test_dcd_sim_slave_connect(UX_FULL_SPEED_DEVICE);
    ux_test_hcd_sim_host_connect(UX_FULL_SPEED_DEVICE);
    status = ux_test_sleep_break_on_success(10000, _test_check_host_connection_error);
    UX_TEST_ASSERT(status == UX_SUCCESS);
    UX_TEST_ASSERT_MESSAGE((host_device_insertion_counter == 0 && host_device_removal_counter == 0),
        "Expect no INS(%ld)/RM(%ld) detection\n", host_device_insertion_counter, host_device_removal_counter);
    UX_TEST_ASSERT_MESSAGE(device, "Expect device\n");
    UX_TEST_ASSERT_MESSAGE(device -> ux_device_class_instance == UX_NULL, "Expect no device class instance\n");
    interface = device -> ux_device_current_configuration -> ux_configuration_first_interface;
    UX_TEST_ASSERT_MESSAGE(interface, "Expect interface\n");
    while(interface)
    {
        UX_TEST_ASSERT_MESSAGE(interface -> ux_interface_class_instance == UX_NULL, "Expect no interface class instance\n");
        interface = interface -> ux_interface_next_interface;
    }

    /* Finally disconnect the device. */
    ux_device_stack_disconnect();

    /* And deinitialize the class.  */
    status  = ux_device_stack_class_unregister(_ux_device_class_dummy_name, _ux_device_class_dummy_entry);

    /* Deinitialize the device side of usbx.  */
    _ux_device_stack_uninitialize();

    /* And finally the usbx system resources.  */
    _ux_system_uninitialize();

    /* Successful test.  */
    printf("SUCCESS!\n");
    test_control_return(0);

}

void  tx_test_thread_slave_simulation_entry(ULONG arg)
{

    while(1)
    {
#if defined(UX_DEVICE_STANDALONE)
        ux_system_tasks_run();
        tx_thread_relinquish();
#else
        /* Sleep so ThreadX on Win32 will delete this thread. */
        tx_thread_sleep(10);
#endif
    }
}
