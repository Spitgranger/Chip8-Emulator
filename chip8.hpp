#include <chrono>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <random>

const uint16_t START_ADDRESS = 0x200;
const uint8_t VF = 0xF;
const uint8_t VIDEO_HEIGHT = 32u;
const uint8_t VIDEO_WIDTH = 64u;

static const uint8_t FONTSET_SIZE = 80;
const uint8_t FONTSET_START_ADDRESS =
    0x50; // Starting location of the FONTSET. anywhere in first 512 bytes
          // should be ok 0x50 seems to be popular

class Chip8
{
public:
    Chip8();
    void LoadROM(const char* filename);
    uint8_t keypad[16] = {0};
    uint32_t video[VIDEO_WIDTH * VIDEO_HEIGHT] = {0};

private:
    uint8_t registers[16] = {0};
    uint8_t memory[4096] = {0};
    uint16_t index = 0;
    uint16_t stack[16] = {0};
    uint8_t sp = 0;
    uint16_t pc = 0;
    uint8_t delayTimer = 0;
    uint8_t soundTimer = 0;
    uint16_t opcode = 0;
    void OP_1nnn();
    void OP_2nnn();
    void OP_3xkk();
    void OP_4xkk();
    void OP_5xy0();
    void OP_6xkk();
    void OP_7xkk();
    void OP_9xy0();
    void OP_Annn();
    void OP_Bnnn();
    void OP_Cxkk();
    void OP_Dxyn();
    void OP_00E0();
    void OP_00EE();
    void OP_8xy0();
    void OP_8xy1();
    void OP_8xy2();
    void OP_8xy3();
    void OP_8xy4();
    void OP_8xy5();
    void OP_8xy6();
    void OP_8xy7();
    void OP_8xyE();
    void OP_ExA1();
    void OP_Ex9E();
    void OP_Fx07();
    void OP_Fx0A();
    void OP_Fx15();
    void OP_Fx18();
    void OP_Fx1E();
    void OP_Fx29();
    void OP_Fx33();
    void OP_Fx55();
    void OP_Fx65();
    void OP_NULL();
    void Table0();
    void TableE();
    void TableF();
    void Table8();

    typedef void (Chip8::*Chip8Func)();

    Chip8Func table[0xF + 1];
    Chip8Func table0[0xE + 1];
    Chip8Func table8[0xE + 1];
    Chip8Func tableE[0xE + 1];
    Chip8Func tableF[0x65 + 1];

    std::default_random_engine randGen;
    std::uniform_int_distribution<uint8_t> randByte;
};