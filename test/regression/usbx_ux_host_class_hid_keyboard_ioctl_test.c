/* This file tests that the host correctly receives the keys the device sends to it. */

#include "usbx_test_common_hid.h"

#include "ux_host_class_hid_keyboard.h"

#define HOST_WAIT_TIME  1
#define SLAVE_WAIT_TIME (3*HOST_WAIT_TIME)

static UCHAR ux_host_class_hid_keyboard_regular_array[] =
{
   0,0,0,0,
   'a','b','c','d','e','f','g','h','i','j', 'k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z',
   '1','2','3','4','5','6','7','8','9','0',
   0x0d,0x1b,0x08,0x07,0x20,'-','=','[',']',
   '\\','#',';',0x27,'`',',','.','/',0xf0,
   0xbb,0xbc,0xbd,0xbe,0xbf,0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,
   0x00,0xf1,0x00,0xd2,0xc7,0xc9,0xd3,0xcf,0xd1,0xcd,0xcd,0xd0,0xc8,0xf2,
   '/','*','-','+',
   0x0d,'1','2','3','4','5','6','7','8','9','0','.','\\',0x00,0x00,'=',
   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};

static UCHAR ux_host_class_hid_keyboard_shift_array[] =
{
   0,0,0,0,
   'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
   '!','@','#','$','%','^','&','*','(',')',
   0x0d,0x1b,0x08,0x07,0x20,'_','+','{','}',
   '|','~',':','"','~','<','>','?',0xf0,
   0xbb,0xbc,0xbd,0xbe,0xbf,0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,
   0x00,0xf1,0x00,0xd2,0xc7,0xc9,0xd3,0xcf,0xd1,0xcd,0xcd,0xd0,0xc8,0xf2,
   '/','*','-','+',
   0x0d,'1','2','3','4','5','6','7','8','9','0','.','\\',0x00,0x00,'=',
   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};

static UCHAR ux_host_class_hid_keyboard_numlock_on_array[] =
{
   '/','*','-','+',
   0x0d,'1','2','3','4','5','6','7','8','9','0','.','\\',0x00,0x00,'=',
};

static UCHAR ux_host_class_hid_keyboard_numlock_off_array[] =
{
   '/','*','-','+',
   0x0d,0xcf,0xd0,0xd1,0xcb,'5',0xcd,0xc7,0xc8,0xc9,0xd2,0xd3,'\\',0x00,0x00,'=',
};

static UX_HOST_CLASS_HID_KEYBOARD_LAYOUT test_layout_swap_regular_shift =
{
    .ux_host_class_hid_keyboard_layout_regular_array = ux_host_class_hid_keyboard_shift_array,
    .ux_host_class_hid_keyboard_layout_shift_array = ux_host_class_hid_keyboard_regular_array,
    .ux_host_class_hid_keyboard_layout_numlock_on_array = ux_host_class_hid_keyboard_numlock_on_array,
    .ux_host_class_hid_keyboard_layout_numlock_off_array = ux_host_class_hid_keyboard_numlock_off_array,
    .ux_host_class_hid_keyboard_layout_keys_upper_range = UX_HID_KEYBOARD_KEYS_UPPER_RANGE,
    .ux_host_class_hid_keyboard_layout_letters_lower_range = UX_HID_KEYBOARD_KEY_LETTER_A,
    .ux_host_class_hid_keyboard_layout_letters_upper_range = UX_HID_KEYBOARD_KEY_LETTER_Z,
    .ux_host_class_hid_keyboard_layout_keypad_lower_range = UX_HID_KEYBOARD_KEYS_KEYPAD_LOWER_RANGE,
    .ux_host_class_hid_keyboard_layout_keypad_upper_range = UX_HID_KEYBOARD_KEYS_KEYPAD_UPPER_RANGE,
};


#define DUMMY_USBX_MEMORY_SIZE (64*1024)
static UCHAR dummy_usbx_memory[DUMMY_USBX_MEMORY_SIZE];

static UCHAR hid_report_descriptor[] = {

    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x06,                    // USAGE (Keyboard)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x05, 0x07,                    //   USAGE_PAGE (Keyboard)
    0x19, 0xe0,                    //   USAGE_MINIMUM (Keyboard LeftControl)
    0x29, 0xe7,                    //   USAGE_MAXIMUM (Keyboard Right GUI)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //   LOGICAL_MAXIMUM (1)
    0x75, 0x01,                    //   REPORT_SIZE (1)
    0x95, 0x08,                    //   REPORT_COUNT (8)
    0x81, 0x02,                    //   INPUT (Data,Var,Abs)
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x81, 0x03,                    //   INPUT (Cnst,Var,Abs)
    0x95, 0x05,                    //   REPORT_COUNT (5)
    0x75, 0x01,                    //   REPORT_SIZE (1)
    0x05, 0x08,                    //   USAGE_PAGE (LEDs)
    0x19, 0x01,                    //   USAGE_MINIMUM (Num Lock)
    0x29, 0x05,                    //   USAGE_MAXIMUM (Kana)
    0x91, 0x02,                    //   OUTPUT (Data,Var,Abs)
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x75, 0x03,                    //   REPORT_SIZE (3)
    0x91, 0x03,                    //   OUTPUT (Cnst,Var,Abs)
    0x95, 0x06,                    //   REPORT_COUNT (6)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x25, ARRAY_COUNT(ux_host_class_hid_keyboard_regular_array) + 1,  //   LOGICAL_MAXIMUM ()
    0x05, 0x07,                    //   USAGE_PAGE (Keyboard)
    0x19, 0x00,                    //   USAGE_MINIMUM (Reserved (no event indicated))
    0x29, ARRAY_COUNT(ux_host_class_hid_keyboard_regular_array) + 1,  //   USAGE_MAXIMUM ()
    0x81, 0x00,                    //   INPUT (Data,Ary,Abs)
    0xc0                           // END_COLLECTION
};
#define HID_REPORT_LENGTH sizeof(hid_report_descriptor)/sizeof(hid_report_descriptor[0])


#define DEVICE_FRAMEWORK_LENGTH_FULL_SPEED 52
static UCHAR device_framework_full_speed[DEVICE_FRAMEWORK_LENGTH_FULL_SPEED] = {

    /* Device descriptor */
        0x12, 0x01, 0x10, 0x01, 0x00, 0x00, 0x00, 0x08,
        0x81, 0x0A, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x01,

    /* Configuration descriptor */
        0x09, 0x02, 0x22, 0x00, 0x01, 0x01, 0x00, 0xc0,
        0x32,

    /* Interface descriptor */
        0x09, 0x04, 0x02, 0x00, 0x01, 0x03, 0x00, 0x00,
        0x00,

    /* HID descriptor */
        0x09, 0x21, 0x10, 0x01, 0x21, 0x01, 0x22, LSB(HID_REPORT_LENGTH),
        MSB(HID_REPORT_LENGTH),

    /* Endpoint descriptor (Interrupt) */
        0x07, 0x05, 0x82, 0x03, 0x08, 0x00, 0x08

    };


#define DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED 62
static UCHAR device_framework_high_speed[DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED] = {

    /* Device descriptor */
        0x12, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00, 0x40,
        0x0a, 0x07, 0x25, 0x40, 0x01, 0x00, 0x01, 0x02,
        0x03, 0x01,

    /* Device qualifier descriptor */
        0x0a, 0x06, 0x00, 0x02, 0x00, 0x00, 0x00, 0x40,
        0x01, 0x00,

    /* Configuration descriptor */
        0x09, 0x02, 0x22, 0x00, 0x01, 0x01, 0x00, 0xc0,
        0x32,

    /* Interface descriptor */
        0x09, 0x04, 0x02, 0x00, 0x01, 0x03, 0x00, 0x00,
        0x00,

    /* HID descriptor */
        0x09, 0x21, 0x10, 0x01, 0x21, 0x01, 0x22, LSB(HID_REPORT_LENGTH),
        MSB(HID_REPORT_LENGTH),

    /* Endpoint descriptor (Interrupt) */
        0x07, 0x05, 0x82, 0x03, 0x08, 0x00, 0x08

    };


    /* String Device Framework :
     Byte 0 and 1 : Word containing the language ID : 0x0904 for US
     Byte 2       : Byte containing the index of the descriptor
     Byte 3       : Byte containing the length of the descriptor string
    */

#define STRING_FRAMEWORK_LENGTH 40
static UCHAR string_framework[] = {

    /* Manufacturer string descriptor : Index 1 */
        0x09, 0x04, 0x01, 0x0c,
        0x45, 0x78, 0x70, 0x72,0x65, 0x73, 0x20, 0x4c,
        0x6f, 0x67, 0x69, 0x63,

    /* Product string descriptor : Index 2 */
        0x09, 0x04, 0x02, 0x0c,
        0x55, 0x53, 0x42, 0x20, 0x4b, 0x65, 0x79, 0x62,
        0x6f, 0x61, 0x72, 0x64,

    /* Serial Number string descriptor : Index 3 */
        0x09, 0x04, 0x03, 0x04,
        0x30, 0x30, 0x30, 0x31
    };


    /* Multiple languages are supported on the device, to add
       a language besides english, the unicode language code must
       be appended to the language_id_framework array and the length
       adjusted accordingly. */
#define LANGUAGE_ID_FRAMEWORK_LENGTH 2
static UCHAR language_id_framework[] = {

    /* English. */
        0x09, 0x04
    };


UINT  _ux_hcd_sim_host_entry(UX_HCD *hcd, UINT function, VOID *parameter);


static UINT ux_system_host_change_function(ULONG a, UX_HOST_CLASS *b, VOID *c)
{
    return 0;
}

static VOID error_callback(UINT system_level, UINT system_context, UINT error_code)
{

    /* @BUG_FIX_PENDING: ux_dcd_sim_slave_function.c doesn't support transfer aborts, which happen during device unregistration of a class. */
    if (error_code != UX_FUNCTION_NOT_SUPPORTED)
    {

        /* Failed test.  */
        printf("Error on line %d, system_level: %d, system_context: %d, error code: 0x%x\n", __LINE__, system_level, system_context, error_code);
        test_control_return(1);
    }
}

/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void    usbx_ux_host_class_hid_keyboard_ioctl_test_application_define(void *first_unused_memory)
#endif
{

UINT status;
CHAR *                          stack_pointer;
CHAR *                          memory_pointer;


    /* Inform user.  */
    printf("Running ux_host_class_hid_keyboard_ioctl Test....................... ");

    /* Initialize the free memory pointer */
    stack_pointer = (CHAR *) usbx_memory;
    memory_pointer = stack_pointer + (UX_DEMO_STACK_SIZE * 2);

    /* Initialize USBX. Memory */
    status = ux_system_initialize(memory_pointer, UX_DEMO_MEMORY_SIZE, UX_NULL,0);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Register the error callback. */
    _ux_utility_error_callback_register(error_callback);

    /* The code below is required for installing the host portion of USBX */
    status =  ux_host_stack_initialize(ux_system_host_change_function);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    status =  ux_host_stack_class_register(_ux_system_host_class_hid_name, ux_host_class_hid_entry);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Register the HID client(s).  */
    status =  ux_host_class_hid_client_register(_ux_system_host_class_hid_client_keyboard_name, ux_host_class_hid_keyboard_entry);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* The code below is required for installing the device portion of USBX. No call back for
       device status change in this example. */
    status =  ux_device_stack_initialize(device_framework_high_speed, DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED,
                                       device_framework_full_speed, DEVICE_FRAMEWORK_LENGTH_FULL_SPEED,
                                       string_framework, STRING_FRAMEWORK_LENGTH,
                                       language_id_framework, LANGUAGE_ID_FRAMEWORK_LENGTH,UX_NULL);
    if(status!=UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Initialize the hid class parameters.  */
    hid_parameter.ux_device_class_hid_parameter_report_address = hid_report_descriptor;
    hid_parameter.ux_device_class_hid_parameter_report_length  = HID_REPORT_LENGTH;
    hid_parameter.ux_device_class_hid_parameter_callback       = demo_thread_hid_callback;

    /* Initialize the device hid class. The class is connected with interface 2 */
    status =  ux_device_stack_class_register(_ux_system_slave_class_hid_name, ux_device_class_hid_entry,
                                                1,2, (VOID *)&hid_parameter);
    if(status!=UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Initialize the simulated device controller.  */
    status =  _ux_dcd_sim_slave_initialize();

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Register all the USB host controllers available in this system */
    status =  ux_host_stack_hcd_register(_ux_system_host_hcd_simulator_name, ux_hcd_sim_host_initialize,0,0);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Create the main host simulation thread.  */
    status =  tx_thread_create(&tx_demo_thread_host_simulation, "tx demo host simulation", tx_demo_thread_host_simulation_entry, 0,
            stack_pointer, UX_DEMO_STACK_SIZE,
            20, 20, 1, TX_AUTO_START);

    /* Check for error.  */
    if (status != TX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Create the main demo thread.  */
    status =  tx_thread_create(&tx_demo_thread_slave_simulation, "tx demo slave simulation", tx_demo_thread_slave_simulation_entry, 0,
            stack_pointer + UX_DEMO_STACK_SIZE, UX_DEMO_STACK_SIZE,
            20, 20, 1, TX_AUTO_START);

    /* Check for error.  */
    if (status != TX_SUCCESS)
    {

        printf("Running HID Keyboard Basic Functionality Test....................... ERROR #10\n");
        test_control_return(1);
    }

}

static UINT  _wait_key(UX_HOST_CLASS_HID_KEYBOARD *keyboard, ULONG *keyboard_key, ULONG *keyboard_state)
{

UINT        status;
UINT        i;


    for(i = 0; i < 200; i ++)
    {
        status = ux_host_class_hid_keyboard_key_get(keyboard, keyboard_key, keyboard_state);
        if (status == UX_SUCCESS)
        {

#if defined(UX_HOST_CLASS_HID_KEYBOARD_EVENTS_KEY_CHANGES_MODE_REPORT_LOCK_KEYS) || defined(UX_HOST_CLASS_HID_KEYBOARD_EVENTS_KEY_CHANGES_MODE_REPORT_MODIFIER_KEYS)
            if (*keyboard_state & UX_HID_KEYBOARD_STATE_FUNCTION)
                continue;
#endif

#if !defined(UX_HOST_CLASS_HID_KEYBOARD_EVENTS_KEY_CHANGES_MODE_REPORT_KEY_DOWN_ONLY)
            if (*keyboard_state & UX_HID_KEYBOARD_STATE_KEY_UP)
                continue;
#endif
            return UX_SUCCESS;
        }
        _ux_utility_delay_ms(2);
    }
    return UX_ERROR;
}

static void  tx_demo_thread_host_simulation_entry(ULONG arg)
{

UINT                            status;
UX_HOST_CLASS_HID_KEYBOARD      *keyboard;
ULONG                           keyboard_key;
ULONG                           keyboard_state;

    /* Find the HID class */
    status = demo_class_hid_get();
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Get the HID client */
    hid_client = hid -> ux_host_class_hid_client;

    /* Check if the instance of the keyboard is live */
    while (hid_client -> ux_host_class_hid_client_local_instance == UX_NULL)
        tx_thread_sleep(10);

    /* Get the keyboard instance */
    keyboard =  (UX_HOST_CLASS_HID_KEYBOARD *)hid_client -> ux_host_class_hid_client_local_instance;

    /**************************************************************************/
    /* Case: normal layout */

    /* Wait a regular key. */
    status = _wait_key(keyboard, &keyboard_key, &keyboard_state);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    if (keyboard_key != 'a')
    {
        printf("ERROR #%d: key 0x%x (%c)\n", __LINE__, status, (UCHAR)keyboard_key);
        test_control_return(1);
    }

    /**************************************************************************/
    /* Case: swap layout */

    status = ux_host_class_hid_keyboard_ioctl(keyboard, UX_HID_KEYBOARD_IOCTL_SET_LAYOUT, (VOID *)&test_layout_swap_regular_shift);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Resume slave to send a key. */
    _ux_utility_thread_resume(&tx_demo_thread_slave_simulation);

    /* Wait a regular key. */
    status = _wait_key(keyboard, &keyboard_key, &keyboard_state);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    if (keyboard_key != 'A')
    {
        printf("ERROR #%d: key 0x%x (%c)\n", __LINE__, status, (UCHAR)keyboard_key);
        test_control_return(1);
    }

    /**************************************************************************/
    /* Case: normal layout */

    status = ux_host_class_hid_keyboard_ioctl(keyboard, UX_HID_KEYBOARD_IOCTL_SET_LAYOUT, UX_NULL);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Resume slave to send a key. */
    _ux_utility_thread_resume(&tx_demo_thread_slave_simulation);

    /* Wait a regular key. */
    status = _wait_key(keyboard, &keyboard_key, &keyboard_state);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    if (keyboard_key != 'a')
    {
        printf("ERROR #%d: key 0x%x (%c)\n", __LINE__, status, (UCHAR)keyboard_key);
        test_control_return(1);
    }

    /**************************************************************************/
    /* Case: invalid IOCTL */

    status = ux_host_class_hid_keyboard_ioctl(keyboard, ~0, UX_NULL);
    if (status == UX_SUCCESS)
    {
        printf("ERROR #%d: code 0x%x\n", __LINE__, status);
        test_control_return(1);
    }

    /* Now disconnect the device.  */
    _ux_device_stack_disconnect();

    /* And deinitialize the class.  */
    status =  ux_device_stack_class_unregister(_ux_system_slave_class_hid_name, ux_device_class_hid_entry);

    /* Deinitialize the device side of usbx.  */
    _ux_device_stack_uninitialize();

    /* And finally the usbx system resources.  */
    _ux_system_uninitialize();

    /* Successful test.  */
    printf("SUCCESS!\n");
    test_control_return(0);
}

static UINT    demo_thread_hid_callback(UX_SLAVE_CLASS_HID *class, UX_SLAVE_CLASS_HID_EVENT *event)
{
    return(UX_SUCCESS);
}

static void  tx_demo_thread_slave_simulation_entry(ULONG arg)
{

UINT                            status;
UX_SLAVE_DEVICE                 *device;
UX_SLAVE_INTERFACE              *interface;
UX_SLAVE_CLASS_HID              *hid;
UX_SLAVE_CLASS_HID_EVENT        hid_event;
UCHAR                           state_modifier[2] = {0, (0x01 << 1)};

    /* Get the pointer to the device.  */
    device =  &_ux_system_slave -> ux_system_slave_device;

    /* reset the HID event structure.  */
    ux_utility_memory_set(&hid_event, 0, sizeof(UX_SLAVE_CLASS_HID_EVENT));

    /* Is the device configured ? */
    while (device->ux_slave_device_state != UX_DEVICE_CONFIGURED)

        /* Then wait.  */
        tx_thread_sleep(10);

    /* Get the interface.  We use the first interface, this is a simple device.  */
    interface = device->ux_slave_device_first_interface;

    /* Form that interface, derive the HID owner.  */
    hid = interface->ux_slave_interface_class_instance;

    /* Wait resume to continue. */
    while(1) {

        /* Send a key!  */
        hid_event.ux_device_class_hid_event_length = 8;
        hid_event.ux_device_class_hid_event_buffer[0] = 0;
        hid_event.ux_device_class_hid_event_buffer[1] = 0;
        hid_event.ux_device_class_hid_event_buffer[2] = UX_HID_KEYBOARD_KEY_LETTER_A;
        status = ux_device_class_hid_event_set(hid, &hid_event);
        if (status != UX_SUCCESS)
        {

            printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
            test_control_return(1);
        }

        /* Release it.  */
        hid_event.ux_device_class_hid_event_buffer[2] = 0;
        status = ux_device_class_hid_event_set(hid, &hid_event);
        if (status != UX_SUCCESS)
        {

            printf("Error on line %d, error code: 0x%x\n", __LINE__, status);
            test_control_return(1);
        }

        ux_utility_thread_suspend(&tx_demo_thread_slave_simulation);
    }
}
