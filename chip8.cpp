#include <cstdint>
#include <fstream>
#include <chrono>
#include <random>
#include <cstring>

class Chip8
{
  public:
    uint8_t registers[16]{};
    uint8_t memory[4096]{};
    uint16_t index{};
    uint16_t stack[16]{};
    uint8_t sp{};
    uint16_t pc{};
    uint8_t delayTimer{};
    uint8_t soundTimer{};
    uint8_t keypad[16]{};
    uint32_t video[64*32]{};
    uint16_t opcode;

  const uint16_t START_ADDRESS = 0x200;
  const uint8_t VF = 0xF;
  const uint8_t VIDEO_HEIGHT = 32u;
  const uint8_t VIDEO_WIDTH = 64u;

  static const uint8_t FONTSET_SIZE = 80;
  const uint8_t FONTSET_START_ADDRESS = 0x50; // Starting location of the FONTSET. anywhere in first 512 bytes should be ok 0x50 seems to be popular
  std::default_random_engine randGen;
  std::uniform_int_distribution<uint8_t> randByte;

  uint8_t fontset[FONTSET_SIZE] = 
  {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
  };

  Chip8::Chip8() : randGen(std::chrono::system_clock::now().time_since_epoch().count())
  {
    pc = START_ADDRESS;
    // copy the fontset into memory starting at 0x50
    for (uint8_t i = 0; i < FONTSET_SIZE; ++i)
    {
      memory[FONTSET_START_ADDRESS + i] = fontset[i];
    }
    // initialize the random number generator
    randByte = std::uniform_int_distribution<uint8_t>(0, 255U);
  }

  void Chip8::LoadROM(char const* filename)
  {
    // open file as a binary stream and move the file pointer to the end so we call use tellg to get the size of the file
      std::ifstream file(filename, std::ios::binary | std::ios::ate);

      if (file.is_open())
      {
        std::streampos size = file.tellg();
        char *buffer = new char[size];

        // go back to the beginning of the file and read its contents into the allocated buffer
        file.seekg(0, std::ios::beg);
        file.read(buffer, size);
        file.close();


        //Copy the buffer into main memory, starting at 0x200 which is the offset for chip8 programs to be read in.
        for (uint32_t i = 0; i < size; i++)
        {
          memory[START_ADDRESS + i] = buffer[i];
        }

        delete[] buffer;
      }
  }

  // BEGIN INSTRUCTIONS

  /*
    00E0 - CLS
    Clear the display.
  */
  void Chip8::OP_00E0()
  {
    // Clear the video buffer
    memset(video, 0, sizeof(video));
  }

  /*
  00EE - RET
  Return from a subroutine.
  The interpreter sets the program counter to the address at the top of the stack, then subtracts 1 from the stack pointer.
  */
  void Chip8::OP_00EE()
  {
    // Subtract sp first since top of stack holds address of instruction that is one past the one who called Subrouine.
    --sp;
    pc = stack[sp];
  }

  /*
  1nnn - JP addr
  Jump to location nnn.

  The interpreter sets the program counter to nnn.
  */
  void Chip8::OP_1nnn()
  {
    uint16_t address = opcode & 0x0FFFu;
    pc = address;
  }
  
  /*
  2nnn - CALL addr
  Call subroutine at nnn.

  The interpreter increments the stack pointer, then puts the current PC on the top of the stack. The PC is then set to nnn
  */
  void Chip8::OP2nnn()
  {
    uint16_t address = opcode & 0x0FFFu;
    stack[sp] = pc;
    ++sp;
    pc = address;
  }

  /*
  Skip next instruction if Vx = kk.

  The interpreter compares register Vx to kk, and if they are equal, increments the program counter by 2.
  */
  void Chip8::OP_3xkk()
  {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t byte = opcode & 0x00FFu;

    if (registers[Vx] == byte)
    {
      pc += 2;
    }
  }

  /*
  Skip next instruction if Vx != kk.

  The interpreter compares register Vx to kk, and if they are not equal, increments the program counter by 2.

  */
  void Chip8::OP_4xkk()
  {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t byte = opcode & 0x00FFu;

    if (registers[Vx] == byte)
    {
      pc += 2;
    }
  }

  /*

  5xy0 - SE Vx, Vy
  Skip next instruction if Vx = Vy.

  The interpreter compares register Vx to register Vy, and if they are equal, increments the program counter by 2.

  */
  void Chip8::OP_5xy0()
  {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    if (registers[Vx] == registers[Vy])
    {
      pc += 2;
    }
  }
  
  /*
  6xkk - LD Vx, byte
  Set Vx = kk.

  The interpreter puts the value kk into register Vx.*/
  void Chip8::OP_6xkk()
  {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t byte = (opcode & 0x00FFu);

    registers[Vx] = byte;
  }

  /*
  7xkk - ADD Vx, byte
  Set Vx = Vx + kk.

  Adds the value kk to the value of register Vx, then stores the result in Vx.
  */
  void Chip8::OP_7xkk()
  {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t byte = (opcode & 0x00FFu);

    registers[Vx] += byte;
  }

  /*
  8xy0 - LD Vx, Vy
  Set Vx = Vy.
  Stores the value of register Vy in register Vx.
  */
 void Chip8::OP_8xy0()
 {
  uint8_t Vx = (opcode & 0x0F00u) >> 8u;
  uint8_t Vy = (opcode & 0x00F0u) >> 4u;

  registers[Vx] = registers[Vy];
 }

 /*
  8xy1 - OR Vx, Vy
  Set Vx = Vx OR Vy.

  Performs a bitwise OR on the values of Vx and Vy, then stores the result in Vx. 
  A bitwise OR compares the corrseponding bits from two values, and if either bit is 1, 
  then the same bit in the result is also 1. Otherwise, it is 0.
 */
void Chip8::OP_8xy1()
{
  uint8_t Vx = (opcode & 0x0F00u) >> 8u;
  uint8_t Vy = (opcode & 0x00F0u) >> 4u;

  registers[Vx] |= registers[Vy];
}

/*
8xy2 - AND Vx, Vy
Set Vx = Vx AND Vy.

Performs a bitwise AND on the values of Vx and Vy, then stores the result in Vx. 
A bitwise AND compares the corrseponding bits from two values, and if both bits are 1, 
then the same bit in the result is also 1. Otherwise, it is 0.
*/
void Chip8::OP_8xy2()
{
  uint8_t Vx = (opcode & 0x0F00u) >> 8u;
  uint8_t Vy = (opcode & 0x00F0u) >> 4u;

  registers[Vx] &= registers[Vy];
}

/*
8xy3 - XOR Vx, Vy
Set Vx = Vx XOR Vy.

Performs a bitwise exclusive OR on the values of Vx and Vy, then stores the result in Vx. 
An exclusive OR compares the corrseponding bits from two values, and if the bits are not both the same, 
then the corresponding bit in the result is set to 1. Otherwise, it is 0.
*/
void Chip8::OP_8xy3()
{
  uint8_t Vx = (opcode & 0x0F00u) >> 8u;
  uint8_t Vy = (opcode & 0x00F0u) >> 4u;

  registers[Vx] ^= registers[Vy];
}

/*
8xy4 - ADD Vx, Vy
Set Vx = Vx + Vy, set VF = carry.

The values of Vx and Vy are added together. If the result is greater than 8 bits (i.e., > 255,) VF is set to 1, otherwise 0. 
Only the lowest 8 bits of the result are kept, and stored in Vx.
*/
void Chip8::OP_8xy4()
{
  uint8_t Vx = (opcode & 0x0F00u) >> 8u;
  uint8_t Vy = (opcode & 0x00F0u) >> 4u;

  uint16_t res = registers[Vx] + registers[Vy];

  if (res > 255u)
  {
    registers[0xF] = 1;
  } 
  else 
  {
    registers[0xF] = 0;
  }
  registers[Vx] = res & 0xFFu;
}
/*
8xy5 - SUB Vx, Vy
Set Vx = Vx - Vy, set VF = NOT borrow.

If Vx > Vy, then VF is set to 1, otherwise 0. Then Vy is subtracted from Vx, and the results stored in Vx.
*/
void Chip8::OP_8xy5()
{
  uint8_t Vx = (opcode & 0x0F00u) >> 8u;
  uint8_t Vy = (opcode & 0x00F0u) >> 4u;

  if (registers[Vx] > registers[Vy])
  {
    registers[0xF] = 1;
  }
  else
  {
    registers[0xF] = 0;
  }

  registers[Vx] -= registers[Vy];
}

/*
8xy6 - SHR Vx {, Vy}
Set Vx = Vx SHR 1.

If the least-significant bit of Vx is 1, then VF is set to 1, otherwise 0. Then Vx is divided by 2.
*/
void Chip8::OP_8xy6()
{
  uint8_t Vx = (opcode & 0x0F00u) >> 8u;

  if (registers[Vx] & 0x1u)
  {
    registers[VF] = 1;
  }
  else
  {
    registers[VF] = 0;
  }
  registers[Vx] >>= 1u;
}

/*
8xy7 - SUBN Vx, Vy
Set Vx = Vy - Vx, set VF = NOT borrow.

If Vy > Vx, then VF is set to 1, otherwise 0. Then Vx is subtracted from Vy, and the results stored in Vx.
*/
void Chip8::OP_8xy7()
{
  uint8_t Vx = (opcode & 0x0F00u) >> 8u;
  uint8_t Vy = (opcode & 0x00F0u) >> 4u;

  if (registers[Vx] < registers[Vy])
  {
    registers[VF] = 1;
  }
  else 
  {
    registers[VF] = 0;
  }

  registers[Vx] = registers[Vy] - registers[Vx];
}

/*
8xyE - SHL Vx {, Vy}
Set Vx = Vx SHL 1.

If the most-significant bit of Vx is 1, then VF is set to 1, otherwise to 0. Then Vx is multiplied by 2.
*/
void Chip8::OP_8xyE()
{
  uint8_t Vx = (opcode & 0x0F00u) >> 8u;
  // TODO CHECK IF THIS IS CORRECT
  registers[VF] = (registers[Vx] & 0xFFu) >> 8u;

  // Multiple register Vx by 2
  registers[Vx] <<= 0x1u;
}

/*
9xy0 - SNE Vx, Vy
Skip next instruction if Vx != Vy.

The values of Vx and Vy are compared, and if they are not equal, the program counter is increased by 2.
*/
void Chip8::OP_9xy0()
{
  uint8_t Vx = (opcode & 0x0F00u) >> 8u;
  uint8_t Vy = (opcode & 0x00F0u) >> 4u;

  if (registers[Vx] != registers[Vy])
  {
    pc += 2;
  }
}

/*
Annn - LD I, addr
Set I = nnn.

The value of register I is set to nnn.

*/
void Chip8::OP_Annn()
{
  uint16_t nnn = opcode & 0x0FFFu;
  index = nnn;
}

/*
Bnnn - JP V0, addr
Jump to location nnn + V0.

The program counter is set to nnn plus the value of V0.
*/
void Chip8::OP_Bnnn()
{
  uint16_t addr = opcode & 0x0FFFu;
  pc = addr + registers[0];
}

/*
Cxkk - RND Vx, byte
Set Vx = random byte AND kk.

The interpreter generates a random number from 0 to 255, which is then ANDed with the value kk. 
The results are stored in Vx. See instruction 8xy2 for more information on AND.
*/
void Chip8::OP_Cxkk()
{
  uint8_t Vx = (opcode & 0x0F00u) >> 8u;
  uint8_t kk = (opcode & 0x00FFu);

  registers[Vx] = randByte(randGen) & kk;
}

/*
Dxyn - DRW Vx, Vy, nibble
Display n-byte sprite starting at memory location I at (Vx, Vy), set VF = collision.

The interpreter reads n bytes from memory, starting at the address stored in I. 
These bytes are then displayed as sprites on screen at coordinates (Vx, Vy). 
Sprites are XORed onto the existing screen. If this causes any pixels to be erased, VF is set to 1, otherwise it is set to 0. 
If the sprite is positioned so part of it is outside the coordinates of the display, it wraps around to the opposite side of the screen. 
See instruction 8xy3 for more information on XOR, and section 2.4, Display, for more information on the Chip-8 screen and sprites.

*/
void Chip8::OP_Dxyn()
{
  uint8_t Vx = (opcode & 0x0F00) >> 8u;
  uint8_t Vy = (opcode & 0x00F0) >> 4u;
  uint8_t n = (opcode & 0x000F);

  uint8_t xPos = registers[Vx] % VIDEO_WIDTH;
  uint8_t yPos = registers[Vy] % VIDEO_HEIGHT;

  registers[VF] = 0;
  for (uint8_t row = 0; row < n; ++row)
  {
    uint8_t byte = memory[index+row];
    for (uint8_t col = 0; col < 8; ++col){
      uint8_t pixel = byte & (0x80u >> col); // Get the bit in this row and column;
      uint32_t *screenPixel = &video[(yPos + row) * VIDEO_WIDTH + (xPos + col)]; // Get the screens current pixel state

      if (pixel)
      {
        if (*screenPixel == 0xFFFFFFFF)
        {
          registers[VF] = 1;
        }
      }
      *screenPixel ^= 0xFFFFFFFF;
    }
  }
}

/*
Ex9E - SKP Vx
Skip next instruction if key with the value of Vx is pressed.

Checks the keyboard, and if the key corresponding to the value of Vx is currently in the down position, PC is increased by 2.
*/
void Chip8::OP_Ex9E()
{
  uint8_t Vx = (opcode & 0x0F00u) >> 8u;

  uint8_t key = registers[Vx];

  if(keypad[key])
  {
    pc += 2;
  }
}

};
