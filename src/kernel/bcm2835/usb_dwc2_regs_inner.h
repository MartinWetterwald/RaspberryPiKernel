#ifndef _H_BCM2835_DWC2_REGS_INNER
#define _H_BCM2835_DWC2_REGS_INNER

// 0x00014 and 18 Interrupt Status & Mask Registers
union gint
{
    uint32_t raw;

    // H = Host only    D = Device only     None = Host & Device
    struct
    {
        uint32_t curmod         : 1; //   Current Mode of Operation (RO)
        uint32_t modemis        : 1; //   Mode Mismatch Interrupt
        uint32_t otgint         : 1; //   OTG Interrupt (RO)
        uint32_t sof            : 1; //   Start of Frame
        uint32_t rxflvl         : 1; //   RX FIFO Non-Empty (RO)
        uint32_t nptxfemp       : 1; //   Non-periodic TX FIFO Empty (RO)
        uint32_t ginnakeff      : 1; // D Global IN Non-periodic NAK Effective (RO)
        uint32_t goutnakeff     : 1; // D Global OUT Non-periodic NAK Effective (RO)
        uint32_t ulpickint      : 1; //   ULPI Carkit Interrupt
        uint32_t i2cint         : 1; //   I²C-compatible Serial Bus Interrupt
        uint32_t earlysusp      : 1; // D Early Suspend
        uint32_t usbsusp        : 1; // D USB Suspend
        uint32_t usbrst         : 1; // D USB Reset
        uint32_t enumdone       : 1; // D Enumeration Done
        uint32_t isooutdrop     : 1; // D Isochronous OUT Packet Dropped Interrupt
        uint32_t eopf           : 1; // D End of Periodic Frame Interrupt
        uint32_t reserved1      : 1;
        uint32_t epmis          : 1; // D Endpoint Mismatch Interrupt
        uint32_t iepint         : 1; // D IN Endpoints Interrupt (RO)
        uint32_t oepint         : 1; // D OUT Endpoints Interrupt (RO)
        uint32_t incompisoin    : 1; // D Incomplete Isochronous IN Transfer
        uint32_t incomplp       : 1; // H Incomplete Periodic Transfer
        uint32_t fetsusp        : 1; // D Data Fetch Suspended
        uint32_t resetdet       : 1; // D Reset Detected
        uint32_t prtint         : 1; // H Host Port Interrupt (RO)
        uint32_t hchint         : 1; // H Host Channels Interrupt (RO)
        uint32_t ptxfemp        : 1; // H Periodic TX FIFO Empty (RO)
        uint32_t reserved2      : 1;
        uint32_t conidstschng   : 1; //   Connector ID Status Change
        uint32_t disconnint     : 1; //   Disconnect Detected Interrupt
        uint32_t sessreqint     : 1; //   Session Request / New Session Detected
        uint32_t wkupint        : 1; //   Resume / Remote Wakeup Detected
    };
};

// 0x00048 User Hardware Config2 Register
union ghwcfg2
{
    uint32_t raw;
    struct
    {
        uint32_t otgmode        : 3; // Mode of Operation
        uint32_t otgarch        : 2; // Architecture
        uint32_t singpnt        : 1; // Point to Point
        uint32_t hsphytype      : 2; // High-Speed PHY Interface Type
        uint32_t fsphytype      : 2; // Full-Spped PHY Interface Type
        uint32_t numdeveps      : 4; // Number of Device Endpoints
        uint32_t numhstchnl     : 4; // Number of Host Channels
        uint32_t periosupport   : 1; // Periodic OUT Channels Supported in Host Mode
        uint32_t dynfifosizing  : 1; // Dynamic FIFO Sizing Enabled
        uint32_t reserved1      : 2;
        uint32_t nptxqdepth     : 2; // Non-periodic Request Queue Depth
        uint32_t ptxqdepth      : 2; // Host Mode Periodic Queue Depth
        uint32_t tknqdepth      : 4; // Device Mode IN Token Sequence Learning Queue Depth
        uint32_t reserved2      : 2;
    };
};

// Host Channel-Specific Interrupts (hcint and hcintmsk)
union hcint
{
    uint32_t raw;
    struct
    {
        uint32_t xfercompl      : 1;
        uint32_t chhltd         : 1;
        uint32_t ahberr         : 1;
        uint32_t stall          : 1;
        uint32_t nak            : 1;
        uint32_t ack            : 1;
        uint32_t nyet           : 1;
        uint32_t xacterr        : 1;
        uint32_t bblerr         : 1;
        uint32_t frmovrun       : 1;
        uint32_t datatglerr     : 1;
        uint32_t reserved       : 21;
    };
};

#endif