#include <wiringPiSPI.h>
#include "spi8088.h"

//Buffer use in read write operations
unsigned char buffer[3];

//Sets up the MCP23S17 chips
void Start_SPI() {
    //CE0-0 mHz-10
    wiringPiSPISetup(0, 10000000);
    //Enable address pins mcp23s17
    buffer[0] = MCP23S17_WRITE_10;
    buffer[1] = IOCON_0A;
    buffer[2] = 0b00001000;
    wiringPiSPIDataRW(0, buffer, 3);
    //Port set up
    //8 bit Data
    buffer[0] = MCP23S17_WRITE_00;
    buffer[1] = IODIRA;
    buffer[2] = ALL_IN;
    wiringPiSPIDataRW(0, buffer, 3);
    //ADDRESS 0-7
    buffer[0] = MCP23S17_WRITE_00;
    buffer[1] = IODIRB;
    buffer[2] = ALL_IN;
    wiringPiSPIDataRW(0, buffer, 3);
    //ADDRESS 8-15
    buffer[0] = MCP23S17_WRITE_01;
    buffer[1] = IODIRA;
    buffer[2] = ALL_IN;
    wiringPiSPIDataRW(0, buffer, 3);
    //ADDRESS 16-19
    buffer[0] = MCP23S17_WRITE_01;
    buffer[1] = IODIRB;
    buffer[2] = ALL_IN;
    wiringPiSPIDataRW(0, buffer, 3);
    //Interrrupt (this is not being used)
    buffer[0] = MCP23S17_WRITE_10;
    buffer[1] = IODIRA;
    buffer[2] = ALL_OUT;
    wiringPiSPIDataRW(0, buffer, 3);
    //Control
    buffer[0] = MCP23S17_WRITE_10;
    buffer[1] = IODIRB;
    buffer[2] = 0b00111111;
    wiringPiSPIDataRW(0, buffer, 3);
}

//Reset 8088
void Reset() {
    //Write 0 to 8284 reset pin
    buffer[0] = MCP23S17_WRITE_10;
    buffer[1] = GPIOB;
    buffer[2] = 0b00000000;
    //MCP23S17 read-write command
    wiringPiSPIDataRW(0, buffer, 3);
    //Write 1 to 8284 reset pin
    buffer[0] = MCP23S17_WRITE_10;
    buffer[1] = GPIOB;
    buffer[2] = 0b10000000;
    //MCP23S17 read-write command
    wiringPiSPIDataRW(0, buffer, 3);
}

//Hold the 8088 bus, true=hold, false=dont hold
void Hold(bool val) {
    if (val == true) {
        //Writes 1 to the hold pin 0bx1xxxxxx, keeps reset pin high
        buffer[0] = MCP23S17_WRITE_10;
        buffer[1] = GPIOB;
        buffer[2] = 0b11000111;
        wiringPiSPIDataRW(0, buffer, 3);
        //Reads from the Control port checking for HOLDA pin to go high
        buffer[0] = MCP23S17_READ_10;
        buffer[1] = GPIOB;
        0;
        wiringPiSPIDataRW(0, buffer, 3);
        buffer[2] = buffer[2] & 0b00100000;
        //waits for HOLDA
        while (buffer[2] != 0b00100000) {
            buffer[0] = MCP23S17_READ_10;
            buffer[1] = GPIOB;
            0;
            wiringPiSPIDataRW(0, buffer, 3);
            buffer[2] = buffer[2] & 0b00100000;
        }

        //Sets up Control port enables RD, WR, IO/M pins
        buffer[0] = MCP23S17_WRITE_10;
        buffer[1] = IODIRB;
        buffer[2] = 0b00111000;
        wiringPiSPIDataRW(0, buffer, 3);
        //8 bit Data out (this kind of doesn't mater becasue read and write operations change this port as needed)
        buffer[0] = MCP23S17_WRITE_00;
        buffer[1] = IODIRA;
        buffer[2] = ALL_OUT;
        wiringPiSPIDataRW(0, buffer, 3);
        //ADDRESS 0-7 port enabled as output
        buffer[0] = MCP23S17_WRITE_00;
        buffer[1] = IODIRB;
        buffer[2] = ALL_OUT;
        wiringPiSPIDataRW(0, buffer, 3);
        //ADDRESS 8-15 port enabled as output
        buffer[0] = MCP23S17_WRITE_01;
        buffer[1] = IODIRA;
        buffer[2] = ALL_OUT;
        wiringPiSPIDataRW(0, buffer, 3);
        //ADDRESS 16-19 port enabled as output
        buffer[0] = MCP23S17_WRITE_01;
        buffer[1] = IODIRB;
        buffer[2] = ALL_OUT;
        wiringPiSPIDataRW(0, buffer, 3);

    } else if (val == false) {
        //8 bit Data in this prevents inference on the 8088 data bus
        buffer[0] = MCP23S17_WRITE_00;
        buffer[1] = IODIRA;
        buffer[2] = ALL_IN;
        wiringPiSPIDataRW(0, buffer, 3);
        //ADDRESS 0-7 port in this prevents inference on the 8088 address bus
        buffer[0] = MCP23S17_WRITE_00;
        buffer[1] = IODIRB;
        buffer[2] = ALL_IN;
        wiringPiSPIDataRW(0, buffer, 3);
        //ADDRESS 8-15 port in this prevents inference on the 8088 address bus
        buffer[0] = MCP23S17_WRITE_01;
        buffer[1] = IODIRA;
        buffer[2] = ALL_IN;
        wiringPiSPIDataRW(0, buffer, 3);
        //ADDRESS 16-19 port in this prevents inference on the 8088 address bus
        buffer[0] = MCP23S17_WRITE_01;
        buffer[1] = IODIRB;
        buffer[2] = ALL_IN;
        wiringPiSPIDataRW(0, buffer, 3);
        //Sets up Control port (sets RD, WR, IO/M to inputs)
        buffer[0] = MCP23S17_WRITE_10;
        buffer[1] = IODIRB;
        buffer[2] = 0b00111111;
        wiringPiSPIDataRW(0, buffer, 3);
        //
        buffer[0] = MCP23S17_WRITE_10;
        buffer[1] = GPIOB;
        buffer[2] = 0b10000111; // or 0b10000000???
        wiringPiSPIDataRW(0, buffer, 3);
    }
}

//Writes a block of data to the 8088 (this does not automatically hold the processor
void Write_Memory_Array(unsigned long long int Address, char code_for_8088[], int Length) {
    //8 bit Data out put
    buffer[0] = MCP23S17_WRITE_00;
    buffer[1] = IODIRA;
    buffer[2] = ALL_OUT;
    wiringPiSPIDataRW(0, buffer, 3);
    for (int i = 0; i < Length; i++) {
        buffer[0] = MCP23S17_WRITE_00;
        buffer[1] = GPIOA;
        buffer[2] = code_for_8088[i];
        wiringPiSPIDataRW(0, buffer, 3);
        buffer[0] = MCP23S17_WRITE_00;
        buffer[1] = GPIOB;
        buffer[2] = Address;
        wiringPiSPIDataRW(0, buffer, 3);
        buffer[0] = MCP23S17_WRITE_01;
        buffer[1] = GPIOA;
        buffer[2] = Address >> 8;
        wiringPiSPIDataRW(0, buffer, 3);
        buffer[0] = MCP23S17_WRITE_01;
        buffer[1] = GPIOB;
        buffer[2] = Address >> 16;
        wiringPiSPIDataRW(0, buffer, 3);
        buffer[0] = MCP23S17_WRITE_10;
        buffer[1] = GPIOB;
        buffer[2] = 0b11000001;
        wiringPiSPIDataRW(0, buffer, 3);
        buffer[0] = MCP23S17_WRITE_10;
        buffer[1] = GPIOB;
        buffer[2] = 0b11000111;
        wiringPiSPIDataRW(0, buffer, 3);
        Address++;
    }
}

void Read_Memory_Array(unsigned long long int Address, char *char_Array, int Length) {
    //8 bit Data
    buffer[0] = MCP23S17_WRITE_00;
    buffer[1] = IODIRA;
    buffer[2] = ALL_IN;
    wiringPiSPIDataRW(0, buffer, 3);
    for (int i = 0; i < Length; ++i) {
        buffer[0] = MCP23S17_WRITE_00;
        buffer[1] = GPIOB;
        buffer[2] = Address;
        wiringPiSPIDataRW(0, buffer, 3);
        buffer[0] = MCP23S17_WRITE_01;
        buffer[1] = GPIOA;
        buffer[2] = Address >> 8;
        wiringPiSPIDataRW(0, buffer, 3);
        buffer[0] = MCP23S17_WRITE_01;
        buffer[1] = GPIOB;
        buffer[2] = Address >> 16;
        wiringPiSPIDataRW(0, buffer, 3);
        buffer[0] = MCP23S17_WRITE_10;
        buffer[1] = GPIOB;
        buffer[2] = 0b11000010;
        wiringPiSPIDataRW(0, buffer, 3);

        buffer[0] = MCP23S17_READ_00;
        buffer[1] = GPIOA;
        0;
        wiringPiSPIDataRW(0, buffer, 3);
        char_Array[i] = buffer[2];

        buffer[0] = MCP23S17_WRITE_10;
        buffer[1] = GPIOB;
        buffer[2] = 0b11000111;
        wiringPiSPIDataRW(0, buffer, 3);
        Address++;
    }
}

char Read_Memory_Byte(unsigned long long int Address) {
    //8 bit Data
    buffer[0] = MCP23S17_WRITE_00;
    buffer[1] = IODIRA;
    buffer[2] = ALL_IN;
    wiringPiSPIDataRW(0, buffer, 3);

    buffer[0] = MCP23S17_WRITE_00;
    buffer[1] = GPIOB;
    buffer[2] = Address;
    wiringPiSPIDataRW(0, buffer, 3);
    buffer[0] = MCP23S17_WRITE_01;
    buffer[1] = GPIOA;
    buffer[2] = Address >> 8;
    wiringPiSPIDataRW(0, buffer, 3);
    buffer[0] = MCP23S17_WRITE_01;
    buffer[1] = GPIOB;
    buffer[2] = Address >> 16;
    wiringPiSPIDataRW(0, buffer, 3);
    buffer[0] = MCP23S17_WRITE_10;
    buffer[1] = GPIOB;
    buffer[2] = 0b11000010;
    wiringPiSPIDataRW(0, buffer, 3);

    buffer[0] = MCP23S17_READ_00;
    buffer[1] = GPIOA;
    0;
    wiringPiSPIDataRW(0, buffer, 3);
    char char_byte = buffer[2];

    buffer[0] = MCP23S17_WRITE_10;
    buffer[1] = GPIOB;
    buffer[2] = 0b11000111;
    wiringPiSPIDataRW(0, buffer, 3);
    return char_byte;
}

void Write_Memory_Byte(unsigned long long int Address, char byte_for_8088) {
    //Set 8 bit Data port out
    buffer[0] = MCP23S17_WRITE_00;
    buffer[1] = IODIRA;
    buffer[2] = ALL_OUT;
    wiringPiSPIDataRW(0, buffer, 3);

    buffer[0] = MCP23S17_WRITE_00;
    buffer[1] = GPIOA;
    buffer[2] = byte_for_8088;
    wiringPiSPIDataRW(0, buffer, 3);
    buffer[0] = MCP23S17_WRITE_00;
    buffer[1] = GPIOB;
    buffer[2] = Address;
    wiringPiSPIDataRW(0, buffer, 3);
    buffer[0] = MCP23S17_WRITE_01;
    buffer[1] = GPIOA;
    buffer[2] = Address >> 8;
    wiringPiSPIDataRW(0, buffer, 3);
    buffer[0] = MCP23S17_WRITE_01;
    buffer[1] = GPIOB;
    buffer[2] = Address >> 16;
    wiringPiSPIDataRW(0, buffer, 3);
    buffer[0] = MCP23S17_WRITE_10;
    buffer[1] = GPIOB;
    buffer[2] = 0b11000001;
    wiringPiSPIDataRW(0, buffer, 3);
    buffer[0] = MCP23S17_WRITE_10;
    buffer[1] = GPIOB;
    buffer[2] = 0b11000111;
    wiringPiSPIDataRW(0, buffer, 3);
}

void Write_Memory_Word(unsigned long long int Address, unsigned short int word_for_8088) {
    //Set 8 bit Data port out
    buffer[0] = MCP23S17_WRITE_00;
    buffer[1] = IODIRA;
    buffer[2] = ALL_OUT;
    wiringPiSPIDataRW(0, buffer, 3);
    buffer[0] = MCP23S17_WRITE_00;
    buffer[1] = GPIOB;
    buffer[2] = Address;
    wiringPiSPIDataRW(0, buffer, 3);
    buffer[0] = MCP23S17_WRITE_01;
    buffer[1] = GPIOA;
    buffer[2] = Address >> 8;
    wiringPiSPIDataRW(0, buffer, 3);
    buffer[0] = MCP23S17_WRITE_01;
    buffer[1] = GPIOB;
    buffer[2] = Address >> 16;
    wiringPiSPIDataRW(0, buffer, 3);
    buffer[0] = MCP23S17_WRITE_00;
    buffer[1] = GPIOA;
    buffer[2] = word_for_8088;
    wiringPiSPIDataRW(0, buffer, 3);
    buffer[0] = MCP23S17_WRITE_10;
    buffer[1] = GPIOB;
    buffer[2] = 0b11000001;
    wiringPiSPIDataRW(0, buffer, 3);
    buffer[0] = MCP23S17_WRITE_10;
    buffer[1] = GPIOB;
    buffer[2] = 0b11000111;
    wiringPiSPIDataRW(0, buffer, 3);
    Address++;

    buffer[0] = MCP23S17_WRITE_00;
    buffer[1] = GPIOB;
    buffer[2] = Address;
    wiringPiSPIDataRW(0, buffer, 3);
    buffer[0] = MCP23S17_WRITE_01;
    buffer[1] = GPIOA;
    buffer[2] = Address >> 8;
    wiringPiSPIDataRW(0, buffer, 3);
    buffer[0] = MCP23S17_WRITE_01;
    buffer[1] = GPIOB;
    buffer[2] = Address >> 16;
    wiringPiSPIDataRW(0, buffer, 3);
    buffer[0] = MCP23S17_WRITE_00;
    buffer[1] = GPIOA;
    buffer[2] = word_for_8088 >> 8;
    wiringPiSPIDataRW(0, buffer, 3);
    buffer[0] = MCP23S17_WRITE_10;
    buffer[1] = GPIOB;
    buffer[2] = 0b11000001;
    wiringPiSPIDataRW(0, buffer, 3);
    buffer[0] = MCP23S17_WRITE_10;
    buffer[1] = GPIOB;
    buffer[2] = 0b11000111;
    wiringPiSPIDataRW(0, buffer, 3);
}

//This is a function that reads only the char code in video memory and skips the color code.
//It also trys to speed up the rate of reading
void Read_Video_Memory(unsigned long long int Address, char *char_Array) {
    //8 bit Data Port
    buffer[0] = MCP23S17_WRITE_00;
    buffer[1] = IODIRA;
    buffer[2] = ALL_IN;
    wiringPiSPIDataRW(0, buffer, 3);

    //Address Segment, lines 16-19
    buffer[0] = MCP23S17_WRITE_01;
    buffer[1] = GPIOB;
    buffer[2] = Address >> 16;
    wiringPiSPIDataRW(0, buffer, 3);
    //Address Segment, lines 8-15
    char Address8_15 = Address >> 8;
    buffer[0] = MCP23S17_WRITE_01;
    buffer[1] = GPIOA;
    buffer[2] = Address8_15;
    wiringPiSPIDataRW(0, buffer, 3);
    char Address8_15_Check = Address >> 8;

    //reads 2000 bytes
    for (int i = 0; i < 0x7D0; ++i) {
        buffer[0] = MCP23S17_WRITE_00;
        buffer[1] = GPIOB;
        buffer[2] = Address;
        wiringPiSPIDataRW(0, buffer, 3);

        Address8_15 = Address >> 8;

        //this skips rewriting address lines 8-15 if there is no change.
        if (Address8_15 != Address8_15_Check) {
            buffer[0] = MCP23S17_WRITE_01;
            buffer[1] = GPIOA;
            buffer[2] = Address8_15;
            wiringPiSPIDataRW(0, buffer, 3);
            Address8_15_Check = Address >> 8;
        }
        buffer[0] = MCP23S17_WRITE_10;
        buffer[1] = GPIOB;
        buffer[2] = 0b11000010;
        wiringPiSPIDataRW(0, buffer, 3);

        buffer[0] = MCP23S17_READ_00;
        buffer[1] = GPIOA;
        0;
        wiringPiSPIDataRW(0, buffer, 3);
        char_Array[i] = buffer[2];

        buffer[0] = MCP23S17_WRITE_10;
        buffer[1] = GPIOB;
        buffer[2] = 0b11000111;
        wiringPiSPIDataRW(0, buffer, 3);
        Address++;
        Address++;
    }
}
