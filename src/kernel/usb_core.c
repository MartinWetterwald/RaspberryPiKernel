#include "usb_hcdi.h"
#include "memory.h"
#include "semaphore.h"
#include "arm.h"
#include "bcm2835/uart.h"
#include "../libc/string.h"

#define USB_MAX_DEV 32
static struct usb_device usb_devs [ USB_MAX_DEV ];

static struct usb_device * usb_root;



struct usb_device * usb_alloc_device ( struct usb_device * parent )
{
    uint32_t irqmask = irq_disable ( );
    for ( int i = 0 ; i < USB_MAX_DEV ; ++i )
    {
        if ( ! usb_devs [ i ].used )
        {
            struct usb_device * dev = & usb_devs [ i ];
            memset ( dev, 0, sizeof ( struct usb_device ) );
            dev -> used = 1;
            dev -> parent = parent;

            irq_restore ( irqmask );
            return dev;
        }
    }

    irq_restore ( irqmask );
    return 0;
}

void usb_free_device ( struct usb_device * dev )
{
    if ( dev -> conf_desc )
    {
        uint32_t irqmask = irq_disable ( );
        memory_deallocate ( dev -> conf_desc );
        irq_restore ( irqmask );
    }
    dev -> used = 0;
}

static uint8_t usb_alloc_addr ( struct usb_device * dev )
{
    return dev - usb_devs + 1;
}

int usb_dev_is_root ( struct usb_device * dev )
{
    return dev -> parent == 0;
}

struct usb_request * usb_alloc_request ( int data_size )
{
    uint32_t irqmask = irq_disable ( );
    struct usb_request * req =
        memory_allocate ( sizeof ( struct usb_request ) + data_size );
    irq_restore ( irqmask );

    if ( ! req )
    {
        return 0;
    }

    req -> dev = 0;
    req -> setup_req = ( const struct usb_setup_req ) { 0 };
    req -> data = req + 1;
    req -> size = data_size;

    req -> status = USB_REQ_STATUS_UNPROCESSED;

    return req;
}

void usb_free_request ( struct usb_request * req )
{
    uint32_t irqmask = irq_disable ( );
    memory_deallocate ( req );
    irq_restore ( irqmask );
}

int usb_submit_request ( struct usb_request * req )
{
    hcd_submit_request ( req );

    return 0;
}

void usb_request_done ( struct usb_request * req )
{
    ( req -> callback ) ( req );
}

static void usb_ctrl_req_callback ( struct usb_request * req )
{
    signal ( req -> priv );
}

int usb_ctrl_req ( struct usb_device * dev,
        uint8_t recipient, uint8_t type, uint8_t dir,
        uint8_t bRequest, uint16_t wValue, uint16_t wIndex,
        void * data, uint16_t wLength )
{
    struct usb_request * req = usb_alloc_request ( wLength );

    if ( ! req )
    {
        return -1;
    }

    sem_t sem = sem_create ( 0 );
    if ( sem < 0 )
    {
        usb_free_request ( req );
        return -1;
    }

    req -> setup_req.bmRequestType.recipient = recipient;
    req -> setup_req.bmRequestType.type = type;
    req -> setup_req.bmRequestType.dir = dir;

    req -> setup_req.bRequest = bRequest;
    req -> setup_req.wValue = wValue;
    req -> setup_req.wIndex.raw = wIndex;
    req -> setup_req.wLength = wLength;

    req -> data = data;
    req -> size = wLength;

    req -> dev = dev;

    req -> callback = usb_ctrl_req_callback;
    req -> priv = ( void * ) ( long ) sem;

    usb_submit_request ( req );
    wait ( sem );

    int status = req -> status;

    sem_destroy ( sem );
    usb_free_request ( req );

    return status;
}

static int usb_read_device_desc ( struct usb_device * dev, uint16_t maxsize )
{
    return usb_ctrl_req ( dev,
            REQ_RECIPIENT_DEV, REQ_TYPE_STD, REQ_DIR_IN,
            REQ_GET_DESC, DESC_DEV << 8, 0,
            & ( dev -> dev_desc ), maxsize );
}

static int usb_set_device_addr ( struct usb_device * dev, uint8_t addr )
{
    int status = usb_ctrl_req ( dev,
            REQ_RECIPIENT_DEV, REQ_TYPE_STD, REQ_DIR_OUT,
            REQ_SET_ADDR, addr, 0, 0, 0 );

    if ( status == USB_REQ_STATUS_SUCCESS )
    {
        dev -> addr = addr;
    }

    return status;
}

static int usb_get_conf_desc ( struct usb_device * dev, uint8_t idx,
        void * buf, uint16_t size )
{
    return usb_ctrl_req ( dev,
            REQ_RECIPIENT_DEV, REQ_TYPE_STD, REQ_DIR_IN,
            REQ_GET_DESC, DESC_CONF << 8 | idx, 0,
            buf, size );
}

static int usb_read_conf_desc ( struct usb_device * dev, uint8_t idx )
{
    struct usb_conf_desc conf;
    int status;

    struct usb_desc_hdr * hdr;
    struct usb_intf_desc * intf;
    struct usb_endp_desc * endp;

    int intf_idx;
    int endp_idx;

    /* First, fetch only the configuration desc without intf & endp.
     * That way we'll know the wTotalLength to allocate */
    status = usb_get_conf_desc ( dev, idx, &conf, sizeof ( conf ) );
    if ( status != USB_REQ_STATUS_SUCCESS )
    {
        return -1;
    }

    // Allocate the configuration descriptor
    dev -> conf_desc = memory_allocate ( conf.wTotalLength );
    if ( ! dev -> conf_desc )
    {
        printu ( "Error when allocating memory for configuration desc" );
        return -1;
    }

    // Get the whole configuration desc with all intfs & endps
    status = usb_get_conf_desc ( dev, idx, dev -> conf_desc, conf.wTotalLength );
    if ( status != USB_REQ_STATUS_SUCCESS )
    {
        printu ( "Error when getting whole configuration desc" );
    }

    // Set the interface and endpoint pointers
    intf_idx = -1;
    endp_idx = -1;
    for ( uint32_t i = 0 ;
            i < dev -> conf_desc -> wTotalLength ; i += hdr -> bLength )
    {
        hdr = ( struct usb_desc_hdr * ) ( ( uint8_t * ) ( dev -> conf_desc ) + i );

        if ( hdr -> bLength < sizeof ( struct usb_desc_hdr ) )
        {
            printu ( "Invalid bLength in configuration descriptor header" );
            return -1;
        }

        switch ( hdr -> bDescriptorType )
        {
            case DESC_INTF:
                intf = ( struct usb_intf_desc * ) hdr;

                // TODO: Handle alternate settings
                if ( intf -> bAlternateSetting != 0 )
                {
                    printu ( "Skipping alternate settings intf..." );
                    break;
                }

                if ( ++intf_idx >= USB_MAX_INTF )
                {
                    printu ( "Too many interfaces" );
                    return -1;
                }
                if ( intf_idx >= dev -> conf_desc -> bNumInterfaces )
                {
                    printu ( "bNumInterfaces mismatch" );
                    return -1;
                }

                dev -> intf_desc [ intf_idx ] = intf;
                endp_idx = -1;
                break;

            case DESC_ENDP:
                if ( intf_idx < 0 )
                {
                    printu ( "Endpoint belonging to no Interface" );
                    return -1;
                }

                // TODO: Handle alternate settings
                if ( intf -> bAlternateSetting != 0 )
                {
                    printu ( "Skipping endp of alternate settings intf..." );
                    break;
                }

                endp = ( struct usb_endp_desc * ) hdr;

                if ( ++endp_idx >= USB_MAX_ENDP )
                {
                    printu ( "Too many endpoints" );
                    return -1;
                }
                if ( endp_idx >= intf -> bNumEndpoints )
                {
                    printu ( "bNumEnpoints mismatch" );
                    return -1;
                }

                dev -> endp_desc [ intf_idx ] [ endp_idx ] = endp;
                break;

            default:
                break;
        }
    }

    return USB_REQ_STATUS_SUCCESS;
}

int usb_set_configuration ( struct usb_device * dev, uint8_t conf )
{
    return usb_ctrl_req ( dev,
            REQ_RECIPIENT_DEV, REQ_TYPE_STD, REQ_DIR_OUT,
            REQ_SET_CONF, conf, 0, 0, 0 );
}

int usb_attach_device ( struct usb_device * dev )
{
    int status;

    // USB 2.0 Section 5.5.3
    /* In order to determine the maximum packet size for the Default Control
     * Pipe, the USB System Software reads the device descriptor. The host will
     * read the first eight bytes of the device descriptor. The device always
     * responds with at least these initial bytes in a single packet. After the
     * host reads the initial part of the device descriptor, it is guaranteed
     * to have read this default pipe's wMaxPacketSize field (byte 7 of the
     * device descriptor). It will then allow the correct size for all
     * subsequent transactions. */
    dev -> dev_desc.bMaxPacketSize0 = USB_LS_CTRL_DATALEN;
    status = usb_read_device_desc ( dev, USB_LS_CTRL_DATALEN );
    if ( status != USB_REQ_STATUS_SUCCESS )
    {
        printu ( "Error on initial device descriptor reading" );
        return -1;
    }

    // Set device address
    status = usb_set_device_addr ( dev, usb_alloc_addr ( dev ) );
    if ( status != USB_REQ_STATUS_SUCCESS )
    {
        printu ( "Error when setting device address" );
        return -1;
    }

    // Re-read the device descriptor only if bMaxPacketSize0 has increased
    if ( dev -> dev_desc.bMaxPacketSize0 > USB_LS_CTRL_DATALEN )
    {
        status = usb_read_device_desc ( dev, sizeof ( struct usb_dev_desc ) );
        if ( status != USB_REQ_STATUS_SUCCESS )
        {
            printu ( "Error on full device descriptor reading" );
            return -1;
        }
    }

    // Read the first configuration descriptor
    status = usb_read_conf_desc ( dev, 0 );
    if ( status != USB_REQ_STATUS_SUCCESS )
    {
        printu ( "Error when reading configuration descriptor" );
        return -1;
    }

    // Activate the first configuration
    status = usb_set_configuration ( dev,
            dev -> conf_desc -> bConfigurationValue );
    if ( status != USB_REQ_STATUS_SUCCESS )
    {
        printu ( "Error when activating the first configuration" );
        return -1;
    }

    return 0;
}

void usb_init ( )
{
    for ( int i = 0 ; i < USB_MAX_DEV ; ++i )
    {
        usb_devs [ i ].used = 0;
    }

    // Request our Host Controller to start up
    if ( hcd_start ( ) != 0 )
    {
        printu ( "USB Core failed to start the HCD" );
        return;
    }

    // Create the root hub
    if ( ! ( usb_root = usb_alloc_device ( 0 ) ) )
    {
        printu ( "USB Core failed to allocate the root hub" );
        return;
    }

    // Attach the root hub
    if ( usb_attach_device ( usb_root ) != 0 )
    {
        printu ( "USB Core failed to attach the root hub" );
        usb_free_device ( usb_root );
        usb_root = 0;
        return;
    }

    printu ( "USB Core Initialization complete" );
}
