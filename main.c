// Define the CD-ROM Hardware Registers
#define CDREG0 (*(volatile unsigned char*)0x1F801800) // Index/Status
#define CDREG1 (*(volatile unsigned char*)0x1F801801) // Command / Response FIFO
#define CDREG2 (*(volatile unsigned char*)0x1F801802) // Parameter FIFO / Data
#define CDREG3 (*(volatile unsigned char*)0x1F801803) // Request / Interrupt Flag

// Wait for CD-ROM controller to not be busy
void cd_wait_ready() {
    // Bit 7 of Index/Status register: 1 = Busy
    while (CDREG0 & 0x80); 
}

// Send a secret command to the CD-ROM controller
void cd_send_secret_cmd(unsigned char cmd, const char* params, int param_len) {
    cd_wait_ready();
    
    // Switch to Index 0 to write parameters and commands
    CDREG0 = 0; 
    
    // 1. Write the string parameters to the Parameter FIFO
    for (int i = 0; i < param_len; i++) {
        CDREG2 = params[i];
    }
    
    // 2. Trigger the command
    CDREG1 = cmd;
    
    // 3. Wait for the hardware interrupt response (usually INT5)
    CDREG0 = 1; // Switch to Index 1 to access Interrupt Flags
    while ((CDREG3 & 0x07) == 0); // Bottom 3 bits will populate when INT is fired
    
    unsigned char int_flag = CDREG3 & 0x07; // Save the specific interrupt fired
    
    // 4. Drain the Response FIFO so the controller doesn't jam
    CDREG0 = 1; 
    // Bit 5: 1 = Response FIFO is NOT empty (0 = Empty)
    while (CDREG0 & 0x20) { 
        volatile unsigned char dummy = CDREG1; // Read and discard the response byte
    }
    
    // 5. Acknowledge (clear) the interrupt
    CDREG0 = 1; 
    CDREG3 = int_flag; // Writing the interrupt value back clears it
}

// The main unlock routine
// Pass 1 for PAL (Europe) consoles, 0 for NTSC-U (American) consoles
void unlock_cdrom_drive(int is_pal) {
    // Sequence 1: Init
    cd_send_secret_cmd(0x50, 0, 0);
    
    // Sequence 2: "Licensed by" (11 bytes)
    cd_send_secret_cmd(0x51, "Licensed by", 11);
    
    // Sequence 3: "Sony" (4 bytes)
    cd_send_secret_cmd(0x52, "Sony", 4);
    
    // Sequence 4: "Computer" (8 bytes)
    cd_send_secret_cmd(0x53, "Computer", 8);
    
    // Sequence 5: "Entertainment" (13 bytes)
    cd_send_secret_cmd(0x54, "Entertainment", 13);
    
    // Sequence 6: Region string
    if (is_pal) {
        cd_send_secret_cmd(0x55, "(Europe)", 8);
    } else {
        cd_send_secret_cmd(0x55, "of America", 10);
    }
    
    // Sequence 7: Finalize
    cd_send_secret_cmd(0x56, 0, 0);
}
