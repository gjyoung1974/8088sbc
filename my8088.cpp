//Compile command line
//g++ my8088.cpp spi8088.cpp -o my8088 -lwiringPi -pthread -lncurses

#include <iostream>
#include <unistd.h>
#include <fstream>
#include <thread>
#include <stdio.h>
#include <ncurses.h>
#include "spi8088.h"

using namespace std;

char v_mem[2000];                   //Video memory buffer
char Cursor_Position[2];            //Array to store cursor position 
int Key_Buffer[0x100];              //Buffer to hold keystrokes
char Key_Head, Key_Tail;            //Pointers for keyboard buffer
bool Stop_Flag = false;             //Stop flag used to end program
string drive_A = "floppy.img";      //Floppy A:      
string drive_C = "hdd.img";         //Hard Drive C:

void up_date_screen();

void Get_Disk_Parameters_A();

void Get_Disk_Parameters_C();

void Int13();

void Insert_Key(); //Interrupt_9
void keyboard();                              //Raspberry PI keyboard handler

int main() {
    Start_SPI();                              //My function to start SPI interface and Initialize MCP23S17

    std::ifstream MemoryFile;                 //New if-stream
    MemoryFile.open("bios.bin");              //Open Rom.bin
    MemoryFile.seekg(0, ios::end);           //Find the end of the file
    int FileSize = MemoryFile.tellg();        //Get the size of the file
    MemoryFile.seekg(0, MemoryFile.beg);     //Start reading at the beginning
    char Rom[FileSize];                       //New char array the size of the rom file
    MemoryFile.read(Rom, FileSize);           //Read the file
    MemoryFile.close();                       //Close the file

    //Jump code to be written to 0xFFFFF, =JMP FAR 0xF000:0X0100
    char FFFF0[] = {static_cast<char>(0XEA), 0X00, 0X01, 0X00, static_cast<char>(0XF0), 'E', 'M', ' ', '0', '4', '/',
                    '1', '0', '/', '2', '0'};

    Hold(true);                                        //the 8088 must be held proir to all read writes. Mem/IO
    Write_Memory_Array(0xFFFF0, FFFF0, sizeof(FFFF0)); //JumpCode
    Write_Memory_Array(0xF0000, Rom, sizeof(Rom));     //The Rom file
    Write_Memory_Byte(0xF00FF, 0xFF);                  //Make sure STOP byte is not zero 0x00 = Stop
    Write_Memory_Byte(0xF0000, 0xFF);                  //Make sure int13 command port is 0xFF
    Hold(false);                                       //Stop hold
    Reset();                                           //Reset the 80888
    //Start threads
    thread screen_loop(up_date_screen);                //Start screen
    thread keyboard_loop(keyboard);                    //Start Keyboard

    while (Stop_Flag != true)                          //Start access to memory/IO
    {
        char stop_byte;                                       //Byte to store Stop Command
        char Int13_Command;                                   //Byte to store Int13 Command
        unsigned short int Timer_Tick;                        //System timer tick
        while (Stop_Flag != true)                             //While Stop Command is not 0x00 loop
        {
            Hold(true);                                        //Hold the 8088 bus
            Read_Video_Memory(0xB8000, v_mem);                 //Reads 0XB8000
            Read_Memory_Array(0x00450, Cursor_Position, 2);    //Reads cursor position
            stop_byte = Read_Memory_Byte(0xF00FF);             //Reads Stop Byte
            Hold(false);                                       //Release 8088 bus
            if (stop_byte == 0X00)                              //Check for stop command
            {
                Stop_Flag = true;                               //If stop = 0x00 then stop threads
            }
            usleep(500);                                       //Give the 8088 time to run
            for (int i = 0; i < 30; ++i)                          //Repeat 'i' times
            {
                usleep(500);                                   //Give the 8088 time to run
                if (Key_Head != Key_Tail)                        //Check for keystroke
                {
                    Insert_Key();                                //If there is a key in the Raspberry PI key buffer insert key to 8088
                }
                usleep(500);                                   //Give the 8088 time to run
                Timer_Tick++;                                    //Increase system timer tick
                Hold(true);                                     //Hold the 8088 bus
                Write_Memory_Word(0X046C, Timer_Tick);          //Write system Timer tick
                Int13_Command = Read_Memory_Byte(0xF0000);      //Check for Int13 command
                Hold(false);                                    //Release 8088 bus
                if (Int13_Command != 0XFF)                       //Check for Int13
                {
                    Int13();                                     //Raspberry PI Int13 handler
                }
                usleep(500);                                    //Give the 8088 time to run
            }
        }
    }
    screen_loop.join();                                //Wait for threads to join
    keyboard_loop.join();

    endwin();                                          //ncurses end
    system("clear");                                   //Clear screen
    return 0;
}

char scan_codes[] = {               //Scan Codes Table 
//     ,   ^A,   ^B,   ^C,    ^D,   ^E,   ^F,   ^G,     ^H,  TAB,  ENT,   ^K,    ^L,   ^M,   ^N,   ^O,
        0X00, 0X1E, 0X30, 0X2E, 0X20, 0X12, 0X21, 0X22, 0X23, 0X0F, 0X1C, 0X25, 0X26, 0X32, 0X31, 0X18, //0x000
//   ^P,   ^Q,   ^R,   ^S,    ^T,   ^U,   ^V,   ^W,     ^X,   ^Y,   ^Z,  ESC,      ,     ,     ,     ,   
        0X19, 0X10, 0X13, 0X1F, 0X14, 0X16, 0X2F, 0X11, 0X2D, 0X15, 0X2C, 0X01, 0X00, 0X00, 0X00, 0X00, //0x010
//  SPC,    !,    ",    #,     $,    %,    &,    ',      (,    ),    *,    +,     ,,    -,    .,    /,  
        0X39, 0X02, 0X28, 0X04, 0X05, 0X06, 0X08, 0X27, 0X0A, 0X0B, 0X09, 0X0D, 0X33, 0X0C, 0X34, 0X35, //0x020
//    0,    1,    2,    3,     4,    5,    6,    7,      8,    9,    :,    ;,     <,    =,    >,    ?,  
        0X0B, 0X02, 0X03, 0X04, 0X05, 0X06, 0X07, 0X08, 0X09, 0X3B, 0X27, 0X27, 0X33, 0X0D, 0X34, 0X35, //0x030
//    @,    A,    B,    C,     D,    E,    F,    G,      H,    I,    J,    K,     L,    M,    N,    O,
        0X03, 0X1E, 0X30, 0X2E, 0X20, 0X12, 0X21, 0X22, 0X23, 0X17, 0X24, 0X25, 0X26, 0X32, 0X31, 0X18, //0x040
//    P,    Q,    R,    S,     T,    U,    V,    W,      X,    Y,    Z,    [,     \,    ],    ^,    _,
        0X19, 0X10, 0X13, 0X1F, 0X14, 0X16, 0X2F, 0X11, 0X2D, 0X15, 0X2C, 0X1A, 0X2B, 0X1B, 0X07, 0X0C, //0x050
//    `,    a,    b,    c,     d,    e,    f,    g,      h,    i,    j,    k,     l,    m,    n,    o,
        0X29, 0X1E, 0X30, 0X2E, 0X20, 0X12, 0X21, 0X22, 0X23, 0X17, 0X24, 0X25, 0X26, 0X32, 0X31, 0X18, //0x060
//    p,    q,    r,    s,     t,    u,    v,    w,      x,    y,    z,    {,     |,    },    ~,   BS,
        0X19, 0X10, 0X13, 0X1F, 0X14, 0X16, 0X2F, 0X11, 0X2D, 0X15, 0X2C, 0X1A, 0X2B, 0X1B, 0X29, 0X0E, //0x070

        0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, //0x080
        0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, //0x080
        0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, //0x0A0
        0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, //0x0B0

        0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, //0x0C0
        0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, //0x0D0
        0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, //0x0E0
        0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, //0x0F0
//     ,     ,   DN,   UP,    LF,   RT,  HOM,   BS,       ,   F1,   F2,   F3,    F4,   F5,   F6,   F7,
        0X00, 0X00, 0X50, 0X48, 0X4B, 0X4D, 0X47, 0X0E, 0X00, 0X3B, 0X3C, 0X3D, 0X3E, 0X3F, 0X40, 0X41, //0x100
//   F8,   F9,  F10,     ,      ,     ,     ,     ,       ,     ,     ,     ,      ,     ,     ,     ,
        0X42, 0X43, 0X44, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, //0x110
        0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, //0x120
        0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, //0x130
//     ,     ,     ,     ,      ,     ,     ,     ,       ,     ,  DEL,     ,      ,     ,     ,     ,
        0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X53, 0X00, 0X00, 0X00, 0X00, 0X00, //0x140
//     ,     , PGDN, PGUP,      ,     ,     ,     ,       ,     ,     ,     ,      ,     ,     ,     ,  
        0X00, 0X00, 0X51, 0X49, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, //0x150
//     ,     ,     ,     ,      ,     ,     ,     ,    END,     ,     ,     ,      ,     ,     ,     ,    
        0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X4F, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, //0x160
        0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, //0x170

        0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, //0x180
        0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, //0x190
        0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, //0x1A0
        0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, //0x1B0

        0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, //0x1C0
        0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, //0x1D0
        0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, //0x1E0
        0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, //0x1F0
//     ,     ,     ,     ,      ,     ,     , ^DEL,
        0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, static_cast<char>(0X93), 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00,
        0X00, //0X200
};

char character_codes[] = {       //Character Code Table
//     ,   ^A,   ^B,   ^C,    ^D,   ^E,   ^F,   ^G,     ^H,  TAB,  ENT,   ^K,    ^L,   ^M,   ^N,   ^O,
        0X00, 0X01, 0X02, 0X03, 0X04, 0X05, 0X06, 0X07, 0X08, 0X09, 0X0D, 0X0B, 0X0C, 0X0D, 0X0E, 0X0F, //0x000
//   ^P,   ^Q,   ^R,   ^S,    ^T,   ^U,   ^V,   ^W,     ^X,   ^Y,   ^Z,  ESC,      ,     ,     ,     ,
        0X10, 0X11, 0X12, 0X13, 0X14, 0X15, 0X16, 0X17, 0X18, 0X19, 0X1A, 0X1B, 0X00, 0X00, 0X00, 0X00, //0x010
//  SPC,    !,    ",    #,     $,    %,    &,    ',      (,    ),    *,    +,     ,,    -,    .,    /,  
        0X20, 0X21, 0X22, 0X23, 0X24, 0X25, 0X26, 0X27, 0X28, 0X29, 0X2A, 0X2B, 0X2C, 0X2D, 0X2E, 0X2F, //0x020
//    0,    1,    2,    3,     4,    5,    6,    7,      8,    9,    :,    ;,     <,    =,    >,    ?,     
        0X30, 0X31, 0X32, 0X33, 0X34, 0X35, 0X36, 0X37, 0X38, 0X39, 0X3A, 0X3B, 0X3C, 0X3D, 0X3E, 0X3F, //0x030
//    @,    A,    B,    C,     D,    E,    F,    G,      H,    I,    J,    K,     L,    M,    N,    O,   
        0X40, 0X41, 0X42, 0X43, 0X44, 0X45, 0X46, 0X47, 0X48, 0X49, 0X4A, 0X4B, 0X4C, 0X4D, 0X4E, 0X4F, //0x040
//    P,    Q,    R,    S,     T,    U,    V,    W,      X,    Y,    Z,    [,     \,    ],    ^,    _,
        0X50, 0X51, 0X52, 0X53, 0X54, 0X55, 0X56, 0X57, 0X58, 0X59, 0X5A, 0X5B, 0X5C, 0X5D, 0X5E, 0X5F, //0x050
//    `,    a,    b,    c,     d,    e,    f,    g,      h,    i,    j,    k,     l,    m,    n,    o,
        0X60, 0X61, 0X62, 0X63, 0X64, 0X65, 0X66, 0X67, 0X68, 0X69, 0X6A, 0X6B, 0X6C, 0X6D, 0X6E, 0X6F, //0x060
//    p,    q,    r,    s,     t,    u,    v,    w,      x,    y,    z,    {,     |,    },    ~,   BS,
        0X70, 0X71, 0X72, 0X73, 0X74, 0X75, 0X76, 0X77, 0X78, 0X79, 0X7A, 0X7B, 0X7C, 0X7D, 0X7E, 0X08, //0x070

        static_cast<char>(0X80), static_cast<char>(0X81), static_cast<char>(0X82), static_cast<char>(0X83),
        static_cast<char>(0X84), static_cast<char>(0X85), static_cast<char>(0X86), static_cast<char>(0X87),
        static_cast<char>(0X88), static_cast<char>(0X89), static_cast<char>(0X8A), static_cast<char>(0X8B),
        static_cast<char>(0X8C), static_cast<char>(0X8D), static_cast<char>(0X8E), static_cast<char>(0X8F), //0x080

        static_cast<char>(0X90), static_cast<char>(0X91), static_cast<char>(0X92), static_cast<char>(0X93),
        static_cast<char>(0X94), static_cast<char>(0X95), static_cast<char>(0X96), static_cast<char>(0X97),
        static_cast<char>(0X98), static_cast<char>(0X99), static_cast<char>(0X9A), static_cast<char>(0X9B),
        static_cast<char>(0X9C), static_cast<char>(0X9D), static_cast<char>(0X9E), static_cast<char>(0X9F), //0x090

        static_cast<char>(0XA0), static_cast<char>(0XA1), static_cast<char>(0XA2), static_cast<char>(0XA3),
        static_cast<char>(0XA4), static_cast<char>(0XA5), static_cast<char>(0XA6), static_cast<char>(0XA7),
        static_cast<char>(0XA8), static_cast<char>(0XA9), static_cast<char>(0XAA), static_cast<char>(0XAB),
        static_cast<char>(0XAC), static_cast<char>(0XAD), static_cast<char>(0XAE), static_cast<char>(0XAF), //0x0A0

        static_cast<char>(0XB0), static_cast<char>(0XB1), static_cast<char>(0XB2), static_cast<char>(0XB3),
        static_cast<char>(0XB4), static_cast<char>(0XB5), static_cast<char>(0XB6), static_cast<char>(0XB7),
        static_cast<char>(0XB8), static_cast<char>(0XB9), static_cast<char>(0XBA), static_cast<char>(0XBB),
        static_cast<char>(0XBC), static_cast<char>(0XBD), static_cast<char>(0XBE), static_cast<char>(0XBF), //0x0B0

        static_cast<char>(0XC0), static_cast<char>(0XC1), static_cast<char>(0XC2), static_cast<char>(0XC3),
        static_cast<char>(0XC4), static_cast<char>(0XC5), static_cast<char>(0XC6), static_cast<char>(0XC7),
        static_cast<char>(0XC8), static_cast<char>(0XC9), static_cast<char>(0XCA), static_cast<char>(0XCB),
        static_cast<char>(0XCC), static_cast<char>(0XCD), static_cast<char>(0XCE), static_cast<char>(0XCF), //0x0C0

        static_cast<char>(0XD0), static_cast<char>(0XD1), static_cast<char>(0XD2), static_cast<char>(0XD3),
        static_cast<char>(0XD4), static_cast<char>(0XD5), static_cast<char>(0XD6), static_cast<char>(0XD7),
        static_cast<char>(0XD8), static_cast<char>(0XD9), static_cast<char>(0XDA), static_cast<char>(0XDB),
        static_cast<char>(0XDC), static_cast<char>(0XDD), static_cast<char>(0XDE), static_cast<char>(0XDF), //0x0D0

        static_cast<char>(0XE0), static_cast<char>(0XE1), static_cast<char>(0XE2), static_cast<char>(0XE3),
        static_cast<char>(0XE4), static_cast<char>(0XE5), static_cast<char>(0XE6), static_cast<char>(0XE7),
        static_cast<char>(0XE8), static_cast<char>(0XE9), static_cast<char>(0XEA), static_cast<char>(0XEB),
        static_cast<char>(0XEC), static_cast<char>(0XED), static_cast<char>(0XEE), static_cast<char>(0XEF), //0x0E0

        static_cast<char>(0XF0), static_cast<char>(0XF1), static_cast<char>(0XF2), static_cast<char>(0XF3),
        static_cast<char>(0XF4), static_cast<char>(0XF5), static_cast<char>(0XF6), static_cast<char>(0XF7),
        static_cast<char>(0XF8), static_cast<char>(0XF9), static_cast<char>(0XFA), static_cast<char>(0XFB),
        static_cast<char>(0XFC), static_cast<char>(0XFD), static_cast<char>(0XFE), static_cast<char>(0XFF), //0x0F0
//     ,     ,   DN,   UP,    LF,   RT,  HOM,   BS,       ,   F1,   F2,   F3,    F4,   F5,   F6,   F7,
        0X00, 0X00, static_cast<char>(0XE0), static_cast<char>(0XE0), static_cast<char>(0XE0), static_cast<char>(0XE0),
        static_cast<char>(0XE0), 0X08, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, //0x100

//   F8,   F9,  F10,     ,      ,     ,     ,     ,       ,     ,     ,     ,      ,     ,     ,     ,
        0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, //0x110
        0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, //0x120
        0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, //0x130
//     ,     ,     ,     ,      ,     ,     ,     ,       ,     ,  DEL,     ,      ,     ,     ,     ,   
        0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, static_cast<char>(0XE0), 0X00, 0X00, 0X00, 0X00,
        0X00, //0x140
//     ,     , PGDN, PGUP,      ,     ,     ,     ,       ,     ,     ,     ,      ,     ,     ,     ,  
        0X00, 0X00, static_cast<char>(0XE0), static_cast<char>(0XE0), 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00,
        0X00, 0X00, 0X00, 0X00, //0x150
//     ,     ,     ,     ,      ,     ,     ,     ,    END,     ,     ,     ,      ,     ,     ,     ,    
        0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, static_cast<char>(0XE0), 0X00, 0X00, 0X00, 0X00, 0X00, 0X00,
        0X00, //0x160
        0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, //0x170

        0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, //0x180
        0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, //0x190
        0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, //0x1A0
        0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, //0x1B0

        0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, //0x1C0
        0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, //0x1D0
        0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, //0x1E0
        0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, //0x1F0
//     ,     ,     ,     ,      ,     ,     , ^DEL,
        0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, static_cast<char>(0XE0), 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00,
        0X00, //0X200
};

void up_date_screen() {
    system("clear");
    int rows = 25;
    int cols = 80;
    WINDOW *win = newwin(rows, cols, 0, 0);
    refresh();
    while (Stop_Flag != true) {
        char char1[1] = {0x00};
        int v_out = 0;
        for (int i = 0; i < rows; ++i) {
            for (int j = 0; j < cols; ++j) {
                char1[0] = v_mem[v_out];

                if (char1[0] < 0x20) {
                    char1[0] = 0x20;
                }
                if (char1[0] > 0x7E) {
                    char1[0] = 0x20;
                }
                wmove(win, i, j);
                wprintw(win, char1);
                v_out++;
            }
        }
        wmove(win, Cursor_Position[1], Cursor_Position[0]);
        wrefresh(win);
        usleep(500);
    }
}

void Get_Disk_Parameters_A() {
    std::fstream MemoryFile;                  //This opens the drive img
    MemoryFile.open(drive_A);                 //File to open
    MemoryFile.seekg(0, MemoryFile.beg);     //Moves back to the begining
    char Floppy[0x200];                       //Char array to hold the data only reading first sector
    MemoryFile.read(Floppy, sizeof(Floppy));  //Read the file into the array
    MemoryFile.close();                       //Close the file

    char Media_Descriptor[0x10] = {0x04, 0, 0, 0, 0, 0, 0, 0, static_cast<char>(0xF8), 0x02, 0, 0, 0, 0x01, 0, 0};
    //Start at port 11
    Write_Memory_Byte(0xF000B, Media_Descriptor[Floppy[0x15] & 0x0F]);    //Media Descriptor - BL

    Write_Memory_Byte(0xF000C, Floppy[0x1A]);                             //Low byte heads per cylinder - DH -1
    Write_Memory_Byte(0xF000D, Floppy[0x1B]);                             //High byte heads per cylinder

    Write_Memory_Byte(0xF000E, Floppy[0x0B]);                             //Low bytes per sector - uS
    Write_Memory_Byte(0xF000F, Floppy[0x0C]);                             //High bytes per sector

    int Head_Per_Cylinder = (Floppy[0x1B] << 8) + Floppy[0x1A];
    int Sector_Per_Track = (Floppy[0x19] << 8) + Floppy[0x18];
    int Number_Of_Sectors = (Floppy[0x14] << 8) + Floppy[0x13];

    int Number_Of_Cylinders = Number_Of_Sectors / Sector_Per_Track / Head_Per_Cylinder;
    Write_Memory_Byte(0xF0011, Number_Of_Cylinders);                      //CH

    Number_Of_Cylinders = (Number_Of_Cylinders >> 2) & 0xC0;
    Sector_Per_Track = Sector_Per_Track & 0X3F;                    //CL
    Write_Memory_Byte(0xF0010, Number_Of_Cylinders + Sector_Per_Track);
    Write_Memory_Byte(0xF0012, 0x00);             //Always zero for floppy
    Write_Memory_Byte(0xF0013, 0x00);             //Always zero for floppy
    Write_Memory_Byte(0xF0014, 0x00);             //Always zero for floppy
    Write_Memory_Byte(0xF0015, 0x00);             //Always zero for floppy
    Write_Memory_Byte(0xF0016, 0X01);             //Drive type
}

void Get_Disk_Parameters_C() {
    std::fstream MemoryFile;                  //This opens the drive img
    MemoryFile.open(drive_C);                 //File to open
    MemoryFile.seekg(0, MemoryFile.beg);     //Moves back to the beginning
    char drive[0x200];                       //Char array to hold the data only reading first sector
    MemoryFile.read(drive, sizeof(drive));  //Read MBR
    int Hidden_Sectors;
    //Locates boot partition
    if (drive[0x1BE] == 0x80) {
        Hidden_Sectors = drive[0x1C6] + (drive[0x1C7] << 8) + (drive[0x1C8] << 16) + (drive[0x1C9] << 24);
    } else if (drive[0x1CE] == 0x80) {
        Hidden_Sectors = drive[0x1D6] + (drive[0x1D7] << 8) + (drive[0x1D8] << 16) + (drive[0x1D9] << 24);
    } else if (drive[0x1DE] == 0x80) {
        Hidden_Sectors = drive[0x1E6] + (drive[0x1E7] << 8) + (drive[0x1E8] << 16) + (drive[0x1E9] << 24);
    } else if (drive[0x1EE] == 0x80) {
        Hidden_Sectors = drive[0x1F6] + (drive[0x1F7] << 8) + (drive[0x1F8] << 16) + (drive[0x1F9] << 24);
    }
        //No boot partion MBR
    else {
        MemoryFile.close();
        Stop_Flag = true;
        usleep(20000);
        printf("No boot partition in MBR C: \n");
    }
    //Read volume boot record
    MemoryFile.seekg(0x200 * Hidden_Sectors, MemoryFile.beg);
    MemoryFile.read(drive, sizeof(drive));
    MemoryFile.close();                       //Close the file

    //char Media_Descriptor[0x10] = {0x04, 0, 0, 0, 0, 0, 0, 0, 0x00, 0x02, 0, 0, 0, 0x01, 0, 0};
    //Start at port 11
    //Write_Memory_Byte(0xF000B, Media_Descriptor[drive[0x15] & 0x0F]);    //Media Descriptor - BL

    Write_Memory_Byte(0xF000C, drive[0x1A]);                             //Low byte heads per cylinder - DH -1
    Write_Memory_Byte(0xF000D, drive[0x1B]);                             //High byte heads per cylinder

    Write_Memory_Byte(0xF000E, drive[0x0B]);                             //Low bytes per sector - uS
    Write_Memory_Byte(0xF000F, drive[0x0C]);                             //High bytes per sector

    int Head_Per_Cylinder = (drive[0x1B] << 8) + drive[0x1A];
    int Sector_Per_Track = (drive[0x19] << 8) + drive[0x18];
    int Number_Of_Sectors = (drive[0x14] << 8) + drive[0x13];

    int Number_Of_Cylinders = Number_Of_Sectors / Sector_Per_Track / Head_Per_Cylinder;
    Write_Memory_Byte(0xF0011, Number_Of_Cylinders);                      //CH

    Number_Of_Cylinders = (Number_Of_Cylinders >> 2) & 0xC0;
    Sector_Per_Track = Sector_Per_Track & 0X3F;                    //CL
    Write_Memory_Byte(0xF0010, Number_Of_Cylinders + Sector_Per_Track);

    int Small_Sectors = (drive[0x14] << 8) + drive[0x13];
    if (Small_Sectors == 0x0000) {
        Write_Memory_Byte(0xF0012, drive[0x20]);   //Big sector
        Write_Memory_Byte(0xF0013, drive[0x21]);
        Write_Memory_Byte(0xF0014, drive[0x22]);
        Write_Memory_Byte(0xF0015, drive[0x23]);
    } else {
        Write_Memory_Byte(0xF0012, drive[0x13]);   //Small sector
        Write_Memory_Byte(0xF0013, drive[0x14]);
        Write_Memory_Byte(0xF0014, 0x00);
        Write_Memory_Byte(0xF0015, 0x00);
    }
    Write_Memory_Byte(0xF0016, 0X03);
}

void Int13() {
    Hold(true);
    char Int13_Command = Read_Memory_Byte(0xF0000);
    char Drive = Read_Memory_Byte(0xF0006);

    if (Int13_Command != 0xFF) {
        if (Drive == 0x00) { Get_Disk_Parameters_A(); }   //File to open
        if (Drive == 0x80) { Get_Disk_Parameters_C(); }
        char int13_data[0X20];
        Read_Memory_Array(0xF0000, int13_data, 0X20);

        if (Int13_Command == 0x00) {
            //BIOS DOES ALL THE WORK
            //RESET DISK SYSTEM
            //NOTHING TO RESET
        }
        if (Int13_Command == 0x01) {
            //BIOS DOES ALL THE WORK
            //GET STATUS OF LAST OPERATION
        }
        if (Int13_Command == 0x02) //read
        {
            int Cylinder = (int13_data[3] << 8) + int13_data[2];
            int Sector = int13_data[4];
            int Head = int13_data[5];
            int Bytes_Per_Sector = (int13_data[0x0F] << 8) + int13_data[0x0E];
            int Sector_Per_Track = (int13_data[0x10] & 0X3F);
            int Head_Per_Cylinder = (int13_data[0x0D] << 8) + int13_data[0x0C];
            int Number_Of_Sectors = int13_data[1];
            int LBA = (Cylinder * Head_Per_Cylinder + Head) * Sector_Per_Track + (Sector - 1);
            int Buffer_Address = (int13_data[10] << 12) + (int13_data[9] << 4) + (int13_data[8] << 8) + int13_data[7];

            std::fstream MemoryFile;                                    //This opens the drive img
            if (Drive == 0x00) { MemoryFile.open(drive_A); }   //File to open
            if (Drive == 0x80) { MemoryFile.open(drive_C); }
            MemoryFile.seekg(LBA * Bytes_Per_Sector, MemoryFile.beg);  //Set position
            char drive[Number_Of_Sectors * Bytes_Per_Sector];          //Char array to hold the data
            MemoryFile.read(drive, sizeof(drive));                    //Read the file into the array
            MemoryFile.close();                                         //Close the file

            Write_Memory_Array(Buffer_Address, drive, sizeof(drive));
        }
        if (Int13_Command == 0x03) {
            int Cylinder = (int13_data[3] << 8) + int13_data[2];
            int Sector = int13_data[4];
            int Head = int13_data[5];
            int Bytes_Per_Sector = (int13_data[0x0F] << 8) + int13_data[0x0E];
            int Sector_Per_Track = (int13_data[0x10] & 0X3F);
            int Head_Per_Cylinder = (int13_data[0x0D] << 8) + int13_data[0x0C];
            int Number_Of_Sectors = int13_data[1];
            int LBA = (Cylinder * Head_Per_Cylinder + Head) * Sector_Per_Track + (Sector - 1);
            int Buffer_Address = (int13_data[10] << 12) + (int13_data[9] << 4) + (int13_data[8] << 8) + int13_data[7];
            char drive[Number_Of_Sectors * Bytes_Per_Sector];    //Char array to hold the data

            Read_Memory_Array(Buffer_Address, drive, sizeof(drive));

            std::fstream MemoryFile;                                    //This opens the drive img
            if (Drive == 0x00) { MemoryFile.open(drive_A); }   //File to open
            if (Drive == 0x80) { MemoryFile.open(drive_C); }
            MemoryFile.seekp(LBA * Bytes_Per_Sector, MemoryFile.beg);  //Sets position to write to
            MemoryFile.write(drive, sizeof(drive));                   //Writes only new data
            MemoryFile.close();                                         //Close img
        }
        if (Int13_Command == 0x08)//PARAMETERS
        {
            if (int13_data[6] == 0x00) { Get_Disk_Parameters_A(); }
            if (int13_data[6] == 0x80) { Get_Disk_Parameters_C(); }
        }
        if (Int13_Command == 0x15)//GET DISK TYPE
        {
            if (int13_data[6] == 0x00) { Get_Disk_Parameters_A(); }
            if (int13_data[6] == 0x80) { Get_Disk_Parameters_C(); }
        }
        Write_Memory_Byte(0xF0000, 0xFF);
    }
    Hold(false);
}

void Insert_Key() //Interrupt_9
{
    Hold(true);                                                    //Hold the 8088 bus
    char Key_Buffer_Tail = Read_Memory_Byte(
            0x041C);               //Read the position of the keyboard buffer tail pointer
    Write_Memory_Byte(0x400 + Key_Buffer_Tail,
                      character_codes[Key_Buffer[Key_Head]]); //Write Character code at the keyboard buffer tail pointer
    Write_Memory_Byte(0x401 + Key_Buffer_Tail,
                      scan_codes[Key_Buffer[Key_Head]]);      //Write scan code at the keyboard buffer tail pointer
    Key_Buffer_Tail = Key_Buffer_Tail + 2;                         //Add 2 to the keyboard buffer tail pointer
    if (Key_Buffer_Tail >= Read_Memory_Byte(
            0x0482))                //Check to see if the keyboard buffer tail pointer is at the end of the buffer
    { Key_Buffer_Tail = Read_Memory_Byte(0x0480); }
    Write_Memory_Byte(0x041C, Key_Buffer_Tail);                    //Write the new keyboard buffer tail pointer
    Key_Head++;                                                    //Advance the Raspberry PI key buffer +1
    Hold(false);                                                   //Release 8088 bus
}

void keyboard()                              //Raspberry PI keyboard handler
{
    initscr();                                //Initialize Ncurses
    noecho();                                 //No echo
    keypad(stdscr, TRUE);                     //Special keys enabled
    nodelay(stdscr, TRUE);                    //No delay
    raw();                                    //Raw key data
    int Key_Pressed = 0;                      //int to store key pressed
    while (Stop_Flag != true)                 //While Stop Command is not 0x00 loop
    {
        Key_Pressed = getch();                 //Read Key pressed
        if (Key_Pressed != ERR)                 //If not error
        {
            Key_Buffer[Key_Tail] = Key_Pressed; //Store key in Keyboard buffer
            Key_Tail++;                         //Advance the buffer tail pointer
        }
    }
}
