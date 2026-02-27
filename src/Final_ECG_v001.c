//Final_ECG_v001.c
// Final_ECG_v001.c
// AD8232 -> PIC24FJ64GA002 -> FT232 -> PC (Processing)
// Sends either "!\n" (leads off) or "0..1023\n" (ECG sample) at ~1 kHz.

#include <xc.h>
#define FCY 4000000UL        // FRC = 8 MHz -> Fcy = 4 MHz
#include <libpic30.h>
#include <stdint.h>

// -----------------------------------------------------------------------------
// CONFIG BITS  (same style as your working UART demo)
// -----------------------------------------------------------------------------
#pragma config ICS = PGx1, FWDTEN = OFF, JTAGEN = OFF
#pragma config POSCMOD = NONE, I2C1SEL = PRI, IOL1WAY = OFF, OSCIOFNC = ON
#pragma config FNOSC = FRC, FCKSM = CSDCMD   // internal FRC, no PLL

// -----------------------------------------------------------------------------
// PIN DEFINITIONS
// -----------------------------------------------------------------------------

// AD8232 OUTPUT -> AN0 / RA0 (pin 2)
#define ECG_TRIS         TRISAbits.TRISA0    // RA0 input

// AD8232 leads-off inputs -> RB4/RB5
// LO+ -> RB4 (pin 11)
#define LOP_TRIS         TRISBbits.TRISB4
#define LOP_PORT         PORTBbits.RB4

// LO- -> RB5 (pin 14)
#define LOM_TRIS         TRISBbits.TRISB5
#define LOM_PORT         PORTBbits.RB5

// Optional blink LED on RA1 (same as before)
#define LED_TRIS         TRISAbits.TRISA1
#define LED_LAT          LATAbits.LATA1

// -----------------------------------------------------------------------------
// UART1 on RB2/RB3 (RP2/RP3) @ 9600 8N1, Fcy = 4 MHz
// -----------------------------------------------------------------------------
static void uart1_init(void)
{
    // Make RB2 / RB3 digital (AN4 / AN5)
    AD1PCFGbits.PCFG4 = 1;   // RB2 digital
    AD1PCFGbits.PCFG5 = 1;   // RB3 digital

    // Directions
    TRISBbits.TRISB2 = 1;    // RB2 = input  (U1RX)
    TRISBbits.TRISB3 = 0;    // RB3 = output (U1TX)

    // --- Unlock PPS, map pins, relock ---
    __builtin_write_OSCCONL(OSCCON & 0xBF);   // clear IOLOCK

    RPINR18bits.U1RXR = 2;                    // U1RX on RP2 (RB2, pin 6)
    RPOR1bits.RP3R    = 3;                    // U1TX (func 3) on RP3 (RB3, pin 7)

    __builtin_write_OSCCONL(OSCCON | 0x40);   // set IOLOCK

    // UART config: 9600 baud, 8N1, Fcy = 4 MHz
    U1MODE = 0;              // BRGH=0, 8-bit, no parity, 1 stop
    U1BRG  = 25;             // 9600: (4e6 / (16*9600)) - 1 ? 25
    U1STA  = 0;

    U1MODEbits.UARTEN = 1;   // enable UART
    U1STAbits.UTXEN   = 1;   // enable transmitter
}

static void uart1_putc(char c)
{
    while (!U1STAbits.TRMT); // wait until transmitter idle
    U1TXREG = c;
}

static void uart1_puts(const char *s)
{
    while (*s) {
        uart1_putc(*s++);
    }
}

// Print unsigned 10-bit value as decimal ASCII (0?1023)
static void uart1_print_uint16(uint16_t v)
{
    char buf[5];
    int i = 0;

    if (v == 0) {
        uart1_putc('0');
        return;
    }

    while (v > 0 && i < 5) {
        buf[i++] = '0' + (v % 10);
        v /= 10;
    }
    while (i--) {
        uart1_putc(buf[i]);
    }
}

// -----------------------------------------------------------------------------
// ADC1 on AN0, single-sample 10-bit
// -----------------------------------------------------------------------------
static void adc1_init(void)
{
    // Make AN0 analog, others digital
    AD1PCFG = 0xFFFF;
    AD1PCFGbits.PCFG0 = 0;    // AN0 analog
    ECG_TRIS = 1;             // RA0 input

    AD1CON1 = 0;
    AD1CON1bits.FORM = 0;     // integer
    AD1CON1bits.SSRC = 0b111; // auto-convert
    AD1CON1bits.ASAM = 0;     // manual sample start

    AD1CON2 = 0;
    AD1CON2bits.VCFG = 0;     // AVdd/AVss
    AD1CON2bits.SMPI = 0;     // interrupt every sample (we poll)

    AD1CON3 = 0;
    AD1CON3bits.ADCS = 31;    // Tad ? (ADCS+1)*Tcy -> nice and slow/safe
    AD1CON3bits.SAMC = 16;    // sample time

    AD1CHS = 0;
    AD1CHSbits.CH0SA = 0;     // AN0

    AD1CON1bits.ADON = 1;     // turn on ADC
}

static uint16_t adc1_read_once(void)
{
    AD1CON1bits.SAMP = 1;     // start sampling
    __delay_us(5);            // small delay while sampling (or rely on SAMC)
    AD1CON1bits.SAMP = 0;     // start conversion

    while (!AD1CON1bits.DONE); // wait for conversion
    AD1CON1bits.DONE = 0;
    return ADC1BUF0;          // 10-bit result (0..1023)
}

// -----------------------------------------------------------------------------
// MAIN
// -----------------------------------------------------------------------------
int main(void)
{
    // Blink LED on RA1 so we know we?re alive
    AD1PCFGbits.PCFG1 = 1;    // RA1 digital
    LED_TRIS = 0;
    LED_LAT  = 0;

    // Leads-off pins as inputs
    LOP_TRIS = 1;             // RB4 input
    LOM_TRIS = 1;             // RB5 input

    uart1_init();
    adc1_init();

    while (1) {
        uint8_t lo_p = LOP_PORT;
        uint8_t lo_m = LOM_PORT;

        LED_LAT ^= 1;          // slow blink (~500 Hz / 2 = 1 kHz loop feels fast; we'll still see it)

        if (lo_p || lo_m) {
            // Leads off -> send "!\n"
            uart1_putc('!');
            uart1_putc('\n');
        } else {
            uint16_t ecg = adc1_read_once();   // 0..1023

            uart1_print_uint16(ecg);
            uart1_putc('\n');                  // Processing expects newline-terminated values
        }

        __delay_ms(1);   // ~1000 samples/sec
    }

    return 0;
}



// #include <xc.h>
// #define FCY 4000000UL        // REAL instruction clock: FRC = 8 MHz -> Fcy = 4 MHz
// #include <libpic30.h>

// #pragma config ICS = PGx1, FWDTEN = OFF, JTAGEN = OFF
// #pragma config POSCMOD = NONE, I2C1SEL = PRI, IOL1WAY = OFF, OSCIOFNC = ON
// #pragma config FNOSC = FRC, FCKSM = CSDCMD   // FRC, no PLL

// // ===== UART1 helper code =====
// void UART1_Init(void) {
//     // Make RB2 / RB3 digital (AN4 / AN5)
//     AD1PCFGbits.PCFG4 = 1;
//     AD1PCFGbits.PCFG5 = 1;

//     // Directions
//     TRISBbits.TRISB2 = 1;    // RB2 = input  (U1RX)
//     TRISBbits.TRISB3 = 0;    // RB3 = output (U1TX)

//     // --- Unlock PPS, map pins, lock PPS again ---
//     __builtin_write_OSCCONL(OSCCON & 0xBF);   // clear IOLOCK (unlock PPS)

//     RPINR18bits.U1RXR = 2;                    // U1RX on RP2 (pin 6)
//     RPOR1bits.RP3R    = 3;                    // U1TX (function 3) on RP3 (pin 7)

//     __builtin_write_OSCCONL(OSCCON | 0x40);   // set IOLOCK (lock PPS)

//     // --- UART config: 8-N-1 @ 9600 baud, Fcy = 4 MHz ---
//     U1MODE = 0;              // BRGH=0, 8-bit, no parity, 1 stop
//     U1BRG  = 25;             // 9600 baud: (4e6 / (16*9600)) - 1 ? 25
//     U1STA  = 0;
//     U1MODEbits.UARTEN = 1;   // enable UART module
//     U1STAbits.UTXEN   = 1;   // enable transmitter
// }

// void UART1_WriteChar(char c) {
//     while (!U1STAbits.TRMT); // wait until transmitter idle
//     U1TXREG = c;
// }

// void UART1_WriteString(const char *s) {
//     while (*s) {
//         UART1_WriteChar(*s++);
//     }
// }

// int main(void)
// {
//     // RA1 LED setup
//     AD1PCFGbits.PCFG1 = 1;   // RA1 digital
//     TRISAbits.TRISA1 = 0;
//     LATAbits.LATA1 = 0;

//     UART1_Init();

//     while (1)
//     {
//         LATAbits.LATA1 ^= 1;
//         UART1_WriteString("Hello from PIC24!\r\n");
//         __delay_ms(500);

//         // Echo everything currently in the RX FIFO
//         while (U1STAbits.URXDA) {      // while data available
//             char c = U1RXREG;
//             UART1_WriteChar(c);        // echo immediately
//         }


//         // Alternate: only echo if data is available
//         // if (U1STAbits.URXDA) {
//         //     char c = U1RXREG;
//         //     UART1_WriteChar(c);    // simple character echo
//         // }
//     }
// }