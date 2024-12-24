/*
 * Copyright (c) 2022, sakumisu
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "usbh_hub.h"
#include <driver/gpio.h>

#define DEV_FORMAT "/dev/hub%d"

#define DEBOUNCE_TIMEOUT       400
#define DEBOUNCE_TIME_STEP     25
#define DELAY_TIME_AFTER_RESET 20

#define EXTHUB_FIRST_INDEX 2

static uint32_t g_devinuse = 0;

USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t g_hub_buf[32];

usb_slist_t hub_event_head = USB_SLIST_OBJECT_INIT(hub_event_head);
usb_slist_t hub_class_head = USB_SLIST_OBJECT_INIT(hub_class_head);

usb_osal_sem_t hub_event_wait;
usb_osal_thread_t hub_thread;

usb_osal_mutex_t hub_event_mutex = NULL;
static beken_queue_t hub_event_queue = NULL;
#define HUB_EVENT_QITEM_COUNT 10
typedef void (*usbh_event_queue_cb_t)();
typedef struct {
    usbh_event_queue_cb_t callback;
    void *arg;
} hub_event_queue_t;
static int usbh_hub_event_send_queue(void *callback, void *arg);

USB_NOCACHE_RAM_SECTION struct usbh_hub roothub;
struct usbh_hubport roothub_parent_port;

USB_NOCACHE_RAM_SECTION struct usbh_hub exthub[CONFIG_USBHOST_MAX_EXTHUBS];

extern int usbh_hport_activate_ep0(struct usbh_hubport *hport);
extern int usbh_hport_deactivate_ep0(struct usbh_hubport *hport);
extern int usbh_enumerate(struct usbh_hubport *hport);

static const char *speed_table[] = { "error-speed", "low-speed", "full-speed", "high-speed", "wireless-speed", "super-speed", "superplus-speed" };

static int usbh_hub_devno_alloc(void)
{
    int devno;

    for (devno = EXTHUB_FIRST_INDEX; devno < 32; devno++) {
        uint32_t bitno = 1 << devno;
        if ((g_devinuse & bitno) == 0) {
            g_devinuse |= bitno;
            return devno;
        }
    }

    return -EMFILE;
}

static void usbh_hub_devno_free(uint8_t devno)
{
    if (devno >= EXTHUB_FIRST_INDEX && devno < 32) {
        g_devinuse &= ~(1 << devno);
    }
}

static int _usbh_hub_get_hub_descriptor(struct usbh_hub *hub, uint8_t *buffer)
{
    struct usb_setup_packet *setup;
    int ret;

    setup = &hub->parent->setup;

    setup->bmRequestType = USB_REQUEST_DIR_IN | USB_REQUEST_CLASS | USB_REQUEST_RECIPIENT_DEVICE;
    setup->bRequest = USB_REQUEST_GET_DESCRIPTOR;
    setup->wValue = HUB_DESCRIPTOR_TYPE_HUB << 8;
    setup->wIndex = 0;
    setup->wLength = USB_SIZEOF_HUB_DESC;

    ret = usbh_control_transfer(hub->parent->ep0, setup, g_hub_buf);
    if (ret < 0) {
        return ret;
    }
    memcpy(buffer, g_hub_buf, USB_SIZEOF_HUB_DESC);
    return ret;
}

static int _usbh_hub_get_status(struct usbh_hub *hub, uint8_t *buffer)
{
    struct usb_setup_packet *setup;
    int ret;

    setup = &hub->parent->setup;

    setup->bmRequestType = USB_REQUEST_DIR_IN | USB_REQUEST_CLASS | USB_REQUEST_RECIPIENT_DEVICE;
    setup->bRequest = HUB_REQUEST_GET_STATUS;
    setup->wValue = 0;
    setup->wIndex = 0;
    setup->wLength = 2;

    ret = usbh_control_transfer(hub->parent->ep0, setup, g_hub_buf);
    if (ret < 0) {
        return ret;
    }
    memcpy(buffer, g_hub_buf, 2);
    return ret;
}

static int _usbh_hub_get_portstatus(struct usbh_hub *hub, uint8_t port, struct hub_port_status *port_status)
{
    struct usb_setup_packet *setup;
    int ret;

    setup = &hub->parent->setup;

    setup->bmRequestType = USB_REQUEST_DIR_IN | USB_REQUEST_CLASS | USB_REQUEST_RECIPIENT_OTHER;
    setup->bRequest = HUB_REQUEST_GET_STATUS;
    setup->wValue = 0;
    setup->wIndex = port;
    setup->wLength = 4;

    ret = usbh_control_transfer(hub->parent->ep0, setup, g_hub_buf);
    if (ret < 0) {
        return ret;
    }
    memcpy(port_status, g_hub_buf, 4);
    return ret;
}

static int _usbh_hub_set_feature(struct usbh_hub *hub, uint8_t port, uint8_t feature)
{
    struct usb_setup_packet *setup;

    setup = &hub->parent->setup;

    setup->bmRequestType = USB_REQUEST_DIR_OUT | USB_REQUEST_CLASS | USB_REQUEST_RECIPIENT_OTHER;
    setup->bRequest = HUB_REQUEST_SET_FEATURE;
    setup->wValue = feature;
    setup->wIndex = port;
    setup->wLength = 0;

    return usbh_control_transfer(hub->parent->ep0, setup, NULL);
}

static int _usbh_hub_clear_feature(struct usbh_hub *hub, uint8_t port, uint8_t feature)
{
    struct usb_setup_packet *setup;

    setup = &hub->parent->setup;

    setup->bmRequestType = USB_REQUEST_DIR_OUT | USB_REQUEST_CLASS | USB_REQUEST_RECIPIENT_OTHER;
    setup->bRequest = HUB_REQUEST_CLEAR_FEATURE;
    setup->wValue = feature;
    setup->wIndex = port;
    setup->wLength = 0;

    return usbh_control_transfer(hub->parent->ep0, setup, NULL);
}

static int parse_hub_descriptor(struct usb_hub_descriptor *desc, uint16_t length)
{
    if (desc->bLength != USB_SIZEOF_HUB_DESC) {
        USB_LOG_ERR("invalid device bLength 0x%02x\r\n", desc->bLength);
        return -1;
    } else if (desc->bDescriptorType != HUB_DESCRIPTOR_TYPE_HUB) {
        USB_LOG_ERR("unexpected descriptor 0x%02x\r\n", desc->bDescriptorType);
        return -2;
    } else {
        USB_LOG_RAW("Hub Descriptor:\r\n");
        USB_LOG_RAW("bLength: 0x%02x             \r\n", desc->bLength);
        USB_LOG_RAW("bDescriptorType: 0x%02x     \r\n", desc->bDescriptorType);
        USB_LOG_RAW("bNbrPorts: 0x%02x           \r\n", desc->bNbrPorts);
        USB_LOG_RAW("wHubCharacteristics: 0x%04x \r\n", desc->wHubCharacteristics);
        USB_LOG_RAW("bPwrOn2PwrGood: 0x%02x      \r\n", desc->bPwrOn2PwrGood);
        USB_LOG_RAW("bHubContrCurrent: 0x%02x    \r\n", desc->bHubContrCurrent);
        USB_LOG_RAW("DeviceRemovable: 0x%02x     \r\n", desc->DeviceRemovable);
        USB_LOG_RAW("PortPwrCtrlMask: 0x%02x     \r\n", desc->PortPwrCtrlMask);
    }
    return 0;
}

static int usbh_hub_get_portstatus(struct usbh_hub *hub, uint8_t port, struct hub_port_status *port_status)
{
    struct usb_setup_packet roothub_setup;
    struct usb_setup_packet *setup;

    if (hub->is_roothub) {
        setup = &roothub_setup;
        setup->bmRequestType = USB_REQUEST_DIR_IN | USB_REQUEST_CLASS | USB_REQUEST_RECIPIENT_OTHER;
        setup->bRequest = HUB_REQUEST_GET_STATUS;
        setup->wValue = 0;
        setup->wIndex = port;
        setup->wLength = 4;
        return usbh_roothub_control(&roothub_setup, (uint8_t *)port_status);
    } else {
        return _usbh_hub_get_portstatus(hub, port, port_status);
    }
}

static int usbh_hub_set_feature(struct usbh_hub *hub, uint8_t port, uint8_t feature)
{
    struct usb_setup_packet roothub_setup;
    struct usb_setup_packet *setup;

    if (hub->is_roothub) {
        setup = &roothub_setup;
        setup->bmRequestType = USB_REQUEST_DIR_OUT | USB_REQUEST_CLASS | USB_REQUEST_RECIPIENT_OTHER;
        setup->bRequest = HUB_REQUEST_SET_FEATURE;
        setup->wValue = feature;
        setup->wIndex = port;
        setup->wLength = 0;
        return usbh_roothub_control(setup, NULL);
    } else {
        return _usbh_hub_set_feature(hub, port, feature);
    }
}

static int usbh_hub_clear_feature(struct usbh_hub *hub, uint8_t port, uint8_t feature)
{
    struct usb_setup_packet roothub_setup;
    struct usb_setup_packet *setup;

    if (hub->is_roothub) {
        setup = &roothub_setup;
        setup->bmRequestType = USB_REQUEST_DIR_OUT | USB_REQUEST_CLASS | USB_REQUEST_RECIPIENT_OTHER;
        setup->bRequest = HUB_REQUEST_CLEAR_FEATURE;
        setup->wValue = feature;
        setup->wIndex = port;
        setup->wLength = 0;
        return usbh_roothub_control(setup, NULL);
    } else {
        return _usbh_hub_clear_feature(hub, port, feature);
    }
}

static void hub_int_complete_callback(void *arg, int nbytes)
{
    struct usbh_hub *hub = (struct usbh_hub *)arg;

    if (nbytes > 0) {
        usbh_hub_event_send_queue(NULL, hub);
    }
}

static int usbh_hub_connect(struct usbh_hubport *hport, uint8_t intf)
{
    struct usb_endpoint_descriptor *ep_desc;
    struct hub_port_status port_status;
    int ret;
    int index;

    index = usbh_hub_devno_alloc();
    if (index > (CONFIG_USBHOST_MAX_EXTHUBS + EXTHUB_FIRST_INDEX - 1)) {
        USB_LOG_ERR("No memory to alloc hub class\r\n");
        usbh_hub_devno_free(index);
        return -ENOMEM;
    }

    struct usbh_hub *hub = &exthub[index - EXTHUB_FIRST_INDEX];

    memset(hub, 0, sizeof(struct usbh_hub));
    hub->hub_addr = hport->dev_addr;
    hub->parent = hport;
    hub->index = index;

    hport->config.intf[intf].priv = hub;

    ret = _usbh_hub_get_hub_descriptor(hub, (uint8_t *)&hub->hub_desc);
    if (ret < 0) {
        return ret;
    }

    parse_hub_descriptor(&hub->hub_desc, USB_SIZEOF_HUB_DESC);

    for (uint8_t port = 0; port < hub->hub_desc.bNbrPorts; port++) {
        hub->child[port].port = port + 1;
        hub->child[port].parent = hub;
    }

    ep_desc = &hport->config.intf[intf].altsetting[0].ep[0].ep_desc;
    if (ep_desc->bEndpointAddress & 0x80) {
        usbh_hport_activate_epx(&hub->intin, hport, ep_desc);
    } else {
        return -1;
    }

    for (uint8_t port = 0; port < hub->hub_desc.bNbrPorts; port++) {
        ret = usbh_hub_set_feature(hub, port + 1, HUB_PORT_FEATURE_POWER);
        if (ret < 0) {
            return ret;
        }
    }

    for (uint8_t port = 0; port < hub->hub_desc.bNbrPorts; port++) {
        ret = usbh_hub_get_portstatus(hub, port + 1, &port_status);
        USB_LOG_INFO("port %u, status:0x%02x, change:0x%02x\r\n", port + 1, port_status.wPortStatus, port_status.wPortChange);
        if (ret < 0) {
            return ret;
        }
    }

    hub->connected = true;
    snprintf(hport->config.intf[intf].devname, CONFIG_USBHOST_DEV_NAMELEN, DEV_FORMAT, hub->index);
    usbh_hub_register(hub);
    USB_LOG_INFO("Register HUB Class:%s\r\n", hport->config.intf[intf].devname);

    usbh_int_urb_fill(&hub->intin_urb, hub->intin, hub->int_buffer, 1, 0, hub_int_complete_callback, hub);
    usbh_submit_urb(&hub->intin_urb);
    return 0;
}

static int usbh_hub_disconnect(struct usbh_hubport *hport, uint8_t intf)
{
    struct usbh_hubport *child;
    int ret = 0;

    struct usbh_hub *hub = (struct usbh_hub *)hport->config.intf[intf].priv;

    if (hub) {
        usbh_hub_devno_free(hub->index);

        if (hub->intin) {
            usbh_pipe_free(hub->intin);
        }

        for (uint8_t port = 0; port < hub->hub_desc.bNbrPorts; port++) {
            child = &hub->child[port];
            usbh_hport_deactivate_ep0(child);
            for (uint8_t i = 0; i < child->config.config_desc.bNumInterfaces; i++) {
                if (child->config.intf[i].class_driver && child->config.intf[i].class_driver->disconnect) {
                    ret = CLASS_DISCONNECT(child, i);
                }
            }

            child->config.config_desc.bNumInterfaces = 0;
            child->parent = NULL;
        }

        usbh_hub_unregister(hub);
        memset(hub, 0, sizeof(struct usbh_hub));

        if (hport->config.intf[intf].devname[0] != '\0')
            USB_LOG_INFO("Unregister HUB Class:%s\r\n", hport->config.intf[intf].devname);
    }
    return ret;
}

static void usbh_roothub_register(void)
{
    USB_LOG_DBG("[+]%s\r\n", __func__);

    memset(&roothub, 0, sizeof(struct usbh_hub));
    memset(&roothub_parent_port, 0, sizeof(struct usbh_hubport));
    roothub_parent_port.port = 1;
    roothub_parent_port.dev_addr = 1;
    roothub.connected = true;
    roothub.index = 1;
    roothub.is_roothub = true;
    roothub.parent = &roothub_parent_port;
    roothub.hub_addr = roothub_parent_port.dev_addr;
    roothub.hub_desc.bNbrPorts = CONFIG_USBHOST_MAX_RHPORTS;
    usbh_hub_register(&roothub);
    USB_LOG_DBG("[-]%s\r\n", __func__);
}

static void usbh_hub_enum_fail_reset_power(void)
{
    USB_LOG_ERR("%s\r\n", __func__);
    extern void bk_usb_enum_fail_all_reg_reset();
    bk_usb_enum_fail_all_reg_reset();
}

void usbh_hub_event_lock_mutex()
{
    if(hub_event_mutex)
        usb_osal_mutex_take(&hub_event_mutex);
}

void usbh_hub_event_unlock_mutex()
{
    if(hub_event_mutex)
        usb_osal_mutex_give(&hub_event_mutex);
}

static int usbh_hub_event_send_queue(void *callback, void *arg)
{
    bk_err_t ret;
    hub_event_queue_t msg;

    msg.callback = callback;
    msg.arg = arg;

    if (hub_event_queue) {
        ret = rtos_push_to_queue(&hub_event_queue, &msg, 0);
        if (kNoErr != ret) {
            USB_LOG_INFO("%s fail ret:%d\r\n", __func__, ret);
            rtos_reset_queue(&hub_event_queue);
            return BK_FAIL;
        }
        return ret;
    }
    return BK_OK;
}

void usbh_roothub_thread_send_queue(uint8_t port, void *callback)
{
    roothub.int_buffer[0] |= (1 << port);
    usbh_hub_event_send_queue(callback, &roothub);
}

static void usbh_hub_events(struct usbh_hub *hub)
{
    struct usbh_hubport *child;
    struct hub_port_status port_status;
    uint8_t portchange_index;
    uint16_t portstatus;
    uint16_t portchange;
    uint8_t speed;
    int ret;

    if (!hub->connected) {
        return;
    }

    {
    portchange_index = hub->int_buffer[0];

    USB_LOG_DBG("Port change:0x%02x\r\n", portchange_index);
    
    if (!(portchange_index & (1 << (0 + 1)))) {
        return;
    }
    portchange_index &= ~(1 << (0 + 1));
    USB_LOG_DBG("Port %d change\r\n", 0 + 1);
    
    /* Read hub port status */
    ret = usbh_hub_get_portstatus(hub, 0 + 1, &port_status);
    if (ret < 0) {
        USB_LOG_ERR("Failed to read port %u status, errorcode: %d\r\n", 0 + 1, ret);
        return;
    }
    
    portstatus = port_status.wPortStatus;
    portchange = port_status.wPortChange;

    USB_LOG_DBG("port %u, status:0x%02x, change:0x%02x\r\n", 0 + 1, portstatus, portchange);

    if (portstatus & HUB_PORT_STATUS_CONNECTION) {
        ret = usbh_hub_set_feature(hub, 0 + 1, HUB_PORT_FEATURE_RESET);
        if (ret < 0) {
            USB_LOG_ERR("Failed to reset port %u,errorcode:%d\r\n", 0, ret);
            return;
        }

        usb_osal_msleep(DELAY_TIME_AFTER_RESET);
        /* Read hub port status */
        ret = usbh_hub_get_portstatus(hub, 0 + 1, &port_status);
        if (ret < 0) {
            USB_LOG_ERR("Failed to read port %u status, errorcode: %d\r\n", 0 + 1, ret);
            return;
        }

        portstatus = port_status.wPortStatus;
        portchange = port_status.wPortChange;
        if (!(portstatus & HUB_PORT_STATUS_RESET) && (portstatus & HUB_PORT_STATUS_ENABLE)) {
            if (portchange & HUB_PORT_STATUS_C_RESET) {
                ret = usbh_hub_clear_feature(hub, 0 + 1, HUB_PORT_FEATURE_C_RESET);
                if (ret < 0) {
                    USB_LOG_ERR("Failed to clear port %u reset change, errorcode: %d\r\n", 0, ret);
                }
            }

            if (portstatus & HUB_PORT_STATUS_HIGH_SPEED) {
                speed = USB_SPEED_HIGH;
            } else if (portstatus & HUB_PORT_STATUS_LOW_SPEED) {
                speed = USB_SPEED_LOW;
            } else {
                speed = USB_SPEED_FULL;
            }

            child = &hub->child[0];

            memset(child, 0, sizeof(struct usbh_hubport));
            child->parent = hub;
            child->connected = true;
            child->port = 0 + 1;
            child->speed = speed;

            USB_LOG_INFO("New %s device on Hub %u, Port %u connected\r\n", speed_table[speed], hub->index, 0 + 1);

            if (usbh_enumerate(child) < 0) {
                USB_LOG_ERR("Port %u enumerate fail\r\n", 0 + 1);
                usbh_hub_enum_fail_reset_power();
            }
        } else {
            USB_LOG_ERR("Failed to enable port %u\r\n", 0 + 1);
            return;
        }
    } 
    else {
        child = &hub->child[0];
        child->connected = false;
        usbh_hport_deactivate_ep0(child);
        for (uint8_t i = 0; i < child->config.config_desc.bNumInterfaces; i++) {
            if (child->config.intf[i].class_driver && child->config.intf[i].class_driver->disconnect) {
                CLASS_DISCONNECT(child, i);
            }
        }
        if (child->raw_config_desc) {
            usb_free(child->raw_config_desc);
            child->raw_config_desc = NULL;
        }
        USB_LOG_INFO("Device on Hub %u, Port %u disconnected\r\n", hub->index, 0 + 1);
        usbh_device_unmount_done_callback(child);
        child->config.config_desc.bNumInterfaces = 0;
    }

    }

}

static void usbh_hub_thread(void *argument)
{
    usb_hc_init();
    while (1) {
        if(hub_event_queue != NULL) {
            hub_event_queue_t msg;
            int ret = 0;
            ret = rtos_pop_from_queue(&hub_event_queue, &msg, BEKEN_WAIT_FOREVER);
            if(kNoErr == ret) {
                if(msg.callback != NULL) {
                    msg.callback();
                }
                usbh_hub_event_lock_mutex();
                usbh_hub_events((struct usbh_hub *)msg.arg);
                usbh_hub_event_unlock_mutex();
            }
        }
    }
}

void usbh_hub_register(struct usbh_hub *hub)
{
    USB_LOG_DBG("[+]%s\r\n", __func__);
    usb_slist_add_tail(&hub_class_head, &hub->list);
    USB_LOG_DBG("[-]%s\r\n", __func__);
}

void usbh_hub_unregister(struct usbh_hub *hub)
{
    usb_slist_remove(&hub_class_head, &hub->list);
}

const struct usbh_class_driver s_hub_driver = {
    .driver_name = "hub",
    .connect = usbh_hub_connect,
    .disconnect = usbh_hub_disconnect
};

const struct usbh_class_info s_hub_info = {
    .match_flags = USB_CLASS_MATCH_INTF_CLASS,
    .class = USB_DEVICE_CLASS_HUB,
    .subclass = 0,
    .protocol = 0,
    .vid = 0x00,
    .pid = 0x00,
    .class_driver = &s_hub_driver
};

int usbh_hub_initialize(void)
{
    USB_LOG_DBG("[+]%s\r\n", __func__);

    usbh_roothub_register();

    uint32_t os_create_fail_flag = 0;

    do {
        rtos_init_queue(&hub_event_queue, "hub_event_queue", sizeof(hub_event_queue_t), HUB_EVENT_QITEM_COUNT);
        if (hub_event_queue == NULL) {
            USB_LOG_ERR("%s create queue fail\r\n", __func__);
            os_create_fail_flag = 1;
        }

        hub_event_wait = usb_osal_sem_create(1);
        if (hub_event_wait == NULL) {
            USB_LOG_ERR("%s create sem fail\r\n", __func__);
            os_create_fail_flag = 1;
        }

        hub_event_mutex = usb_osal_mutex_create();
        if (hub_event_mutex == NULL) {
            USB_LOG_ERR("%s create mutex fail\r\n", __func__);
            os_create_fail_flag = 1;
        }

        hub_thread = usb_osal_thread_create("usbh_hub", CONFIG_USBHOST_PSC_STACKSIZE, CONFIG_USBHOST_PSC_PRIO, usbh_hub_thread, NULL);
        if (hub_thread == NULL) {
            USB_LOG_ERR("%s create thread fail\r\n", __func__);
            os_create_fail_flag = 1;
        }
    }while(0);

    if(os_create_fail_flag) {
        if(hub_event_queue) {
            rtos_deinit_queue(&hub_event_queue);
            hub_event_queue = NULL;
        }

        if (hub_event_wait) {
            usb_osal_sem_delete(&hub_event_wait);
            hub_event_wait = NULL;
        }

        if (hub_event_mutex) {
            usb_osal_mutex_delete(&hub_event_mutex);
            hub_event_mutex = NULL;
        }

        if (hub_thread) {
            usb_osal_thread_delete(&hub_thread);
            hub_thread = NULL;
        }
    }

    return 0;
}

int usbh_hub_deinitialize(void)
{
    USB_LOG_DBG("[+]%s\r\n", __func__);

    GLOBAL_INT_DECLARATION();
    GLOBAL_INT_DISABLE();

    usb_hc_deinit();

    if(hub_event_queue) {
        rtos_deinit_queue(&hub_event_queue);
        hub_event_queue = NULL;
    }

    if (hub_event_wait) {
        usb_osal_sem_delete(&hub_event_wait);
        hub_event_wait = NULL;
    }

    if (hub_event_mutex) {
        usb_osal_mutex_delete(&hub_event_mutex);
        hub_event_mutex = NULL;
    }

    if (hub_thread) {
        usb_osal_thread_delete(&hub_thread);
        hub_thread = NULL;
    }

    GLOBAL_INT_RESTORE();

    //usb_hc_deinit();
    usbh_hub_unregister(&roothub);

    USB_LOG_DBG("[-]%s\r\n", __func__);
    return 0;
}

/*
const struct usbh_class_driver hub_driver = {
    .driver_name = "hub",
    .connect = usbh_hub_connect,
    .disconnect = usbh_hub_disconnect
};

CLASS_INFO_DEFINE const struct usbh_class_info hub_info = {
    .match_flags = USB_CLASS_MATCH_INTF_CLASS,
    .class = USB_DEVICE_CLASS_HUB,
    .subclass = 0,
    .protocol = 0,
    .vid = 0x00,
    .pid = 0x00,
    .class_driver = &hub_driver
};
*/
