//
// Created by andreas on 26.02.24.
//
#include "gtest/gtest.h"
#include "./../chip8/chip8.h"

class Chip8Test {
public:
    explicit Chip8Test() = default;

    static constexpr size_t memory_in_bytes{4096};
    static constexpr size_t number_of_registers{16};
    static constexpr size_t width_in_pixels{64};
    static constexpr size_t height_in_pixels{32};
    static constexpr size_t number_of_stack_levels{16};
    static constexpr size_t number_of_keys{16};
    static constexpr size_t memory_offset{512};
    Chip8<memory_in_bytes, number_of_registers, width_in_pixels, height_in_pixels, number_of_stack_levels, number_of_keys> chip8;
public:

    using Bit16 = Chip8<memory_in_bytes, number_of_registers, width_in_pixels, height_in_pixels, number_of_stack_levels, number_of_keys>::Bit16;
    using Bit8 = Chip8<memory_in_bytes, number_of_registers, width_in_pixels, height_in_pixels, number_of_stack_levels, number_of_keys>::Bit8;

    void test(int opcode) {
        chip8.opcodeMap[opcode]();
    }

    void load_memory(std::array<Bit8, memory_in_bytes - memory_offset> &memory) {
        chip8.load_memory(memory);
    }

    void set_graphics(const std::array<Bit8, width_in_pixels * height_in_pixels> &input) {
        chip8.graphics = input;
    }


    const Bit8 get_register_value(int register_index) const {
        return chip8.registers[register_index];
    }

    Bit16 get_current_opcode() const {
        return chip8.current_opcode;
    }

    Bit16 get_draw_flag() const {
        return chip8.get_draw_flag();
    }

    Bit16 get_program_counter() const {
        return chip8.program_counter;
    }

    const std::array<Bit8, memory_in_bytes> &get_memory() const {
        return chip8.memory;
    }

    const std::array<Bit8, width_in_pixels * height_in_pixels> &get_graphics() const {
        return chip8.graphics;
    }

    const std::array<Bit16, number_of_stack_levels> &get_stack() const {
        return chip8.stack;
    }

    const std::array<Bit8, number_of_keys> &get_keypad() const {
        return chip8.keypad;
    }

    static constexpr Bit16 start_program_counter() {
        return 0x200;
    }

    void set_register_value(int register_index, unsigned char value) {
        chip8.registers[register_index] = value;
    }

};


void set_opcode_to_memory_index(const unsigned int opcode, std::array<Chip8Test::Bit8,
        Chip8Test::memory_in_bytes - Chip8Test::memory_offset> &memory, int memory_index) {
    Chip8Test::Bit8 high_byte = (opcode >> 8) & 0xFF;
    Chip8Test::Bit8 low_byte = opcode & 0xFF;
    memory[memory_index] = high_byte;
    memory[memory_index + 1] = low_byte;
}

TEST(TestChip8, SetProgramCounterToAddress) {
    Chip8Test chip8;
    std::array<Chip8Test::Bit8, Chip8Test::memory_in_bytes - Chip8Test::memory_offset> memory{};
    constexpr unsigned int opcode = 0x1111;
    set_opcode_to_memory_index(opcode, memory, 0);
    chip8.load_memory(memory);
    EXPECT_EQ(chip8.start_program_counter(), chip8.get_program_counter());
    chip8.chip8.emulateCycle();
    const Chip8Test::Bit16 expected_program_counter{0x0111};
    EXPECT_EQ(opcode, chip8.get_current_opcode());
    EXPECT_EQ(expected_program_counter, chip8.get_program_counter());
}

TEST(TestChip8, ClearGraphics) {
    Chip8Test chip8;
    std::array<Chip8Test::Bit8, Chip8Test::memory_in_bytes - Chip8Test::memory_offset> memory{};
    constexpr unsigned int opcode = 0x00E0;
    set_opcode_to_memory_index(opcode, memory, 0);


    std::array<Chip8Test::Bit8, Chip8Test::width_in_pixels * Chip8Test::height_in_pixels> ones{};
    std::fill(ones.begin(), ones.end(), 1);
    chip8.set_graphics(ones);
    EXPECT_EQ(chip8.get_graphics(), ones);
    chip8.load_memory(memory);
    EXPECT_EQ(chip8.start_program_counter(), chip8.get_program_counter());
    chip8.chip8.emulateCycle();
    EXPECT_EQ(opcode, chip8.get_current_opcode());
    const Chip8Test::Bit16 expected_program_counter{0x0200 + 2};
    const std::array<Chip8Test::Bit8, Chip8Test::width_in_pixels * Chip8Test::height_in_pixels> zeroes{};
    EXPECT_EQ(expected_program_counter, chip8.get_program_counter());
    EXPECT_TRUE(chip8.get_draw_flag());
}


TEST(TestChip8, CallSubroutineAtAddress) {
    Chip8Test chip8;
    std::array<Chip8Test::Bit8, Chip8Test::memory_in_bytes - Chip8Test::memory_offset> memory{};
    constexpr unsigned int opcode = 0x2123;
    set_opcode_to_memory_index(opcode, memory, 0);
    chip8.load_memory(memory);

    EXPECT_EQ(chip8.start_program_counter(), chip8.get_program_counter());
    EXPECT_EQ(chip8.get_stack()[0], 0);
    chip8.chip8.emulateCycle();
    EXPECT_EQ(opcode, chip8.get_current_opcode());
    EXPECT_EQ(0x0123, chip8.get_program_counter());
    EXPECT_EQ(chip8.get_stack()[0], chip8.start_program_counter());
}

TEST(TestChip8, SkipInstructionIfRegisterValueIsEqualOpCodeValue) {
    Chip8Test chip8;
    std::array<Chip8Test::Bit8, Chip8Test::memory_in_bytes - Chip8Test::memory_offset> memory{};
    constexpr unsigned int opcode = 0x3EAB;
    constexpr int register_index{0xE};
    constexpr Chip8Test::Bit8 valid_register_value{0xAB};
    constexpr Chip8Test::Bit8 invalid_register_value{0xAC};
    chip8.set_register_value(register_index, valid_register_value);
    set_opcode_to_memory_index(opcode, memory, 0);
    chip8.load_memory(memory);
    EXPECT_EQ(chip8.start_program_counter(), chip8.get_program_counter());
    chip8.chip8.emulateCycle();
    EXPECT_EQ(opcode, chip8.get_current_opcode());
    // Skip instruction
    constexpr int skip_instruction_constant{4};
    EXPECT_EQ(chip8.get_program_counter(), chip8.start_program_counter() + skip_instruction_constant);

}

TEST(TestChip8, NotSkipInstructionIfRegisterValueIsEqualOpCodeValue) {
    Chip8Test chip8;
    std::array<Chip8Test::Bit8, Chip8Test::memory_in_bytes - Chip8Test::memory_offset> memory{};
    constexpr unsigned int opcode = 0x3EAB;
    constexpr int register_index{0xE};
    constexpr Chip8Test::Bit8 invalid_register_value{0xAC};
    chip8.set_register_value(register_index, invalid_register_value);
    set_opcode_to_memory_index(opcode, memory, 0);
    chip8.load_memory(memory);
    EXPECT_EQ(chip8.start_program_counter(), chip8.get_program_counter());
    chip8.chip8.emulateCycle();
    EXPECT_EQ(opcode, chip8.get_current_opcode());
    // do not skip instruction
    constexpr int no_skip_instruction_constant{2};
    EXPECT_EQ(chip8.get_program_counter(), chip8.start_program_counter() + no_skip_instruction_constant);

}

TEST(TestChip8, SkipInstructionIfRegisterValueIsNOTEqualOpCodeValue) {
    Chip8Test chip8;
    std::array<Chip8Test::Bit8, Chip8Test::memory_in_bytes - Chip8Test::memory_offset> memory{};
    constexpr unsigned int opcode = 0x4EAB;
    constexpr int register_index{0xE};
    constexpr Chip8Test::Bit8 valid_register_value{0xAC};
    chip8.set_register_value(register_index, valid_register_value);
    set_opcode_to_memory_index(opcode, memory, 0);
    chip8.load_memory(memory);
    EXPECT_EQ(chip8.start_program_counter(), chip8.get_program_counter());
    chip8.chip8.emulateCycle();
    EXPECT_EQ(opcode, chip8.get_current_opcode());
    // Skip instruction
    constexpr int skip_instruction_constant{4};
    EXPECT_EQ(chip8.get_program_counter(), chip8.start_program_counter() + skip_instruction_constant);

}

TEST(TestChip8, DoNotSkipInstructionIfRegisterValueIsEqualOpCodeValue) {
    Chip8Test chip8;
    std::array<Chip8Test::Bit8, Chip8Test::memory_in_bytes - Chip8Test::memory_offset> memory{};
    constexpr unsigned int opcode = 0x4EAB;
    constexpr int register_index{0xE};
    constexpr Chip8Test::Bit8 invalid_register_value{0xAB};
    chip8.set_register_value(register_index, invalid_register_value);
    set_opcode_to_memory_index(opcode, memory, 0);
    chip8.load_memory(memory);
    EXPECT_EQ(chip8.start_program_counter(), chip8.get_program_counter());
    chip8.chip8.emulateCycle();
    EXPECT_EQ(opcode, chip8.get_current_opcode());
    // do not skip instruction
    constexpr int not_skip_instruction_constant{2};
    EXPECT_EQ(chip8.get_program_counter(), chip8.start_program_counter() + not_skip_instruction_constant);

}

TEST(TestChip8, SkipInstructionIfNothRegisterValueAreEqual) {
    Chip8Test chip8;
    std::array<Chip8Test::Bit8, Chip8Test::memory_in_bytes - Chip8Test::memory_offset> memory{};
    constexpr unsigned int opcode = 0x5A11;
    constexpr int register_index1{0xA};
    constexpr int register_index2{0x1};
    constexpr Chip8Test::Bit8 register_value1{0x12};
    constexpr Chip8Test::Bit8 register_value2{register_value1};
    chip8.set_register_value(register_index1, register_value1);
    chip8.set_register_value(register_index2, register_value2);
    set_opcode_to_memory_index(opcode, memory, 0);
    chip8.load_memory(memory);
    EXPECT_EQ(chip8.start_program_counter(), chip8.get_program_counter());
    chip8.chip8.emulateCycle();
    EXPECT_EQ(opcode, chip8.get_current_opcode());
    // skip instruction
    constexpr int skip_instruction_constant{4};
    EXPECT_EQ(chip8.get_program_counter(), chip8.start_program_counter() + skip_instruction_constant);

}

TEST(TestChip8, DoNotSkipInstructionIfNothRegisterValueAreNotEqual) {
    Chip8Test chip8;
    std::array<Chip8Test::Bit8, Chip8Test::memory_in_bytes - Chip8Test::memory_offset> memory{};
    constexpr unsigned int opcode = 0x5A11;
    constexpr int register_index1{0xA};
    constexpr int register_index2{0x1};
    constexpr Chip8Test::Bit8 register_value1{0x12};
    constexpr Chip8Test::Bit8 register_value2{register_value1 + 1};
    chip8.set_register_value(register_index1, register_value1);
    chip8.set_register_value(register_index2, register_value2);
    set_opcode_to_memory_index(opcode, memory, 0);
    chip8.load_memory(memory);
    EXPECT_EQ(chip8.start_program_counter(), chip8.get_program_counter());
    chip8.chip8.emulateCycle();
    EXPECT_EQ(opcode, chip8.get_current_opcode());
    // do not skip instruction
    constexpr int not_skip_instruction_constant{2};
    EXPECT_EQ(chip8.get_program_counter(), chip8.start_program_counter() + not_skip_instruction_constant);
}

TEST(TestChip8, SetValueInRegister) {
    constexpr int not_skip_instruction_constant{2};
    for (unsigned int i = 0x6000; i <= 0x6FFF; ++i) {
        Chip8Test chip8;
        std::array<Chip8Test::Bit8, Chip8Test::memory_in_bytes - Chip8Test::memory_offset> memory{};
        const unsigned int opcode = i;
        const int register_index = static_cast<int>((i & 0x0F00)) >> 8;
        const Chip8Test::Bit8 value = (i & 0xFF);
        set_opcode_to_memory_index(opcode, memory, 0);
        chip8.load_memory(memory);
        EXPECT_EQ(chip8.start_program_counter(), chip8.get_program_counter());
        chip8.chip8.emulateCycle();
        EXPECT_EQ(opcode, chip8.get_current_opcode());
        EXPECT_EQ(chip8.get_program_counter(), chip8.start_program_counter() + not_skip_instruction_constant);
        EXPECT_EQ(chip8.get_register_value(register_index), value);
    }

}

TEST(TestChip8, AddValueToRegister) {
    constexpr int not_skip_instruction_constant{2};
    for (unsigned int i = 0x7011; i <= 0x7F11; i += 0xFF) {
        Chip8Test chip8;
        std::array<Chip8Test::Bit8, Chip8Test::memory_in_bytes - Chip8Test::memory_offset> memory{};
        const unsigned int opcode = i;
        const int register_index = static_cast<int>((i & 0x0F00)) >> 8;
        const Chip8Test::Bit8 result = (i & 0xFF);
        set_opcode_to_memory_index(opcode, memory, 0);
        chip8.load_memory(memory);
        EXPECT_EQ(chip8.start_program_counter(), chip8.get_program_counter());
        EXPECT_EQ(chip8.get_register_value(register_index), 0);
        chip8.chip8.emulateCycle();
        EXPECT_EQ(opcode, chip8.get_current_opcode());
        EXPECT_EQ(chip8.get_program_counter(), chip8.start_program_counter() + not_skip_instruction_constant);
        EXPECT_EQ(chip8.get_register_value(register_index), result);
    }

}

TEST(TestChip8, TwoRegisterOperationsAssignValueinRegister5ToRegister0) {
    constexpr int not_skip_instruction_constant{2};
    // set value 0xAA in register 0
    constexpr int opcode_register0{0x60AA};
    // set value 0x21 in register 5
    constexpr int opcode_register5{0x6521};
    std::array<Chip8Test::Bit8, Chip8Test::memory_in_bytes - Chip8Test::memory_offset> memory{};
    Chip8Test chip8;
    constexpr int register_index0{};
    constexpr int register_index5{5};
    set_opcode_to_memory_index(opcode_register0, memory, 0);
    set_opcode_to_memory_index(opcode_register5, memory, 2);
    constexpr int opcode_assign_register5_to_register0{0x8050};
    set_opcode_to_memory_index(opcode_assign_register5_to_register0, memory, 4);
    chip8.load_memory(memory);
    EXPECT_EQ(chip8.start_program_counter(), chip8.get_program_counter());
    EXPECT_EQ(chip8.get_register_value(register_index0), 0);
    EXPECT_EQ(chip8.get_register_value(register_index5), 0);
    chip8.chip8.emulateCycle();
    EXPECT_NE(chip8.get_register_value(register_index0), 0);
    EXPECT_EQ(chip8.get_register_value(register_index0), 0xAA);
    EXPECT_EQ(chip8.get_register_value(register_index5), 0);
    chip8.chip8.emulateCycle();
    EXPECT_EQ(chip8.get_register_value(register_index0), 0xAA);
    EXPECT_NE(chip8.get_register_value(register_index5), 0);
    EXPECT_EQ(chip8.get_register_value(register_index5), 0x21);
    chip8.chip8.emulateCycle();
    EXPECT_EQ(chip8.get_register_value(register_index5), 0x21);
    EXPECT_NE(chip8.get_register_value(register_index0), 0xAA);
    EXPECT_EQ(chip8.get_register_value(register_index0), 0x21);
}

TEST(TestChip8, TwoRegisterOperationsAssignORValueinRegister5ToRegister0) {
    constexpr int not_skip_instruction_constant{2};
    // set value 0xAA in register 0
    constexpr int opcode_register0{0x60AA};
    // set value 0x21 in register 5
    constexpr int opcode_register5{0x6521};
    std::array<Chip8Test::Bit8, Chip8Test::memory_in_bytes - Chip8Test::memory_offset> memory{};
    Chip8Test chip8;
    constexpr int register_index0{};
    constexpr int register_index5{5};
    set_opcode_to_memory_index(opcode_register0, memory, 0);
    set_opcode_to_memory_index(opcode_register5, memory, 2);
    constexpr int opcode_assign_register5_to_register0{0x8051};
    set_opcode_to_memory_index(opcode_assign_register5_to_register0, memory, 4);
    chip8.load_memory(memory);
    EXPECT_EQ(chip8.start_program_counter(), chip8.get_program_counter());
    EXPECT_EQ(chip8.get_register_value(register_index0), 0);
    EXPECT_EQ(chip8.get_register_value(register_index5), 0);
    chip8.chip8.emulateCycle();
    EXPECT_NE(chip8.get_register_value(register_index0), 0);
    EXPECT_EQ(chip8.get_register_value(register_index0), 0xAA);
    EXPECT_EQ(chip8.get_register_value(register_index5), 0);
    chip8.chip8.emulateCycle();
    EXPECT_EQ(chip8.get_register_value(register_index0), 0xAA);
    EXPECT_NE(chip8.get_register_value(register_index5), 0);
    EXPECT_EQ(chip8.get_register_value(register_index5), 0x21);
    chip8.chip8.emulateCycle();
    EXPECT_EQ(chip8.get_register_value(register_index5), 0x21);
    EXPECT_NE(chip8.get_register_value(register_index0), 0xAA);
    EXPECT_EQ(chip8.get_register_value(register_index0), 0xAB);
}

TEST(TestChip8, TwoRegisterOperationsAssignANDValueinRegister5ToRegister0) {
    constexpr int not_skip_instruction_constant{2};
    // set value 0xAA in register 0
    constexpr int opcode_register0{0x60AA};
    // set value 0x21 in register 5
    constexpr int opcode_register5{0x6521};
    std::array<Chip8Test::Bit8, Chip8Test::memory_in_bytes - Chip8Test::memory_offset> memory{};
    Chip8Test chip8;
    constexpr int register_index0{};
    constexpr int register_index5{5};
    set_opcode_to_memory_index(opcode_register0, memory, 0);
    set_opcode_to_memory_index(opcode_register5, memory, 2);
    constexpr int opcode_assign_register5_to_register0{0x8052};
    set_opcode_to_memory_index(opcode_assign_register5_to_register0, memory, 4);
    chip8.load_memory(memory);
    EXPECT_EQ(chip8.start_program_counter(), chip8.get_program_counter());
    EXPECT_EQ(chip8.get_register_value(register_index0), 0);
    EXPECT_EQ(chip8.get_register_value(register_index5), 0);
    chip8.chip8.emulateCycle();
    EXPECT_NE(chip8.get_register_value(register_index0), 0);
    EXPECT_EQ(chip8.get_register_value(register_index0), 0xAA);
    EXPECT_EQ(chip8.get_register_value(register_index5), 0);
    chip8.chip8.emulateCycle();
    EXPECT_EQ(chip8.get_register_value(register_index0), 0xAA);
    EXPECT_NE(chip8.get_register_value(register_index5), 0);
    EXPECT_EQ(chip8.get_register_value(register_index5), 0x21);
    chip8.chip8.emulateCycle();
    EXPECT_EQ(chip8.get_register_value(register_index5), 0x21);
    EXPECT_NE(chip8.get_register_value(register_index0), 0xAA);
    EXPECT_EQ(chip8.get_register_value(register_index0), 0x20);
}

TEST(TestChip8, TwoRegisterOperationsAssignXORValueinRegister5ToRegister0) {
    constexpr int not_skip_instruction_constant{2};
    // set value 0xAA in register 0
    constexpr int opcode_register0{0x60AA};
    // set value 0x21 in register 5
    constexpr int opcode_register5{0x6521};
    std::array<Chip8Test::Bit8, Chip8Test::memory_in_bytes - Chip8Test::memory_offset> memory{};
    Chip8Test chip8;
    constexpr int register_index0{};
    constexpr int register_index5{5};
    set_opcode_to_memory_index(opcode_register0, memory, 0);
    set_opcode_to_memory_index(opcode_register5, memory, 2);
    constexpr int opcode_assign_register5_to_register0{0x8053};
    set_opcode_to_memory_index(opcode_assign_register5_to_register0, memory, 4);
    chip8.load_memory(memory);
    EXPECT_EQ(chip8.start_program_counter(), chip8.get_program_counter());
    EXPECT_EQ(chip8.get_register_value(register_index0), 0);
    EXPECT_EQ(chip8.get_register_value(register_index5), 0);
    chip8.chip8.emulateCycle();
    EXPECT_NE(chip8.get_register_value(register_index0), 0);
    EXPECT_EQ(chip8.get_register_value(register_index0), 0xAA);
    EXPECT_EQ(chip8.get_register_value(register_index5), 0);
    chip8.chip8.emulateCycle();
    EXPECT_EQ(chip8.get_register_value(register_index0), 0xAA);
    EXPECT_NE(chip8.get_register_value(register_index5), 0);
    EXPECT_EQ(chip8.get_register_value(register_index5), 0x21);
    chip8.chip8.emulateCycle();
    EXPECT_EQ(chip8.get_register_value(register_index5), 0x21);
    EXPECT_NE(chip8.get_register_value(register_index0), 0xAA);
    EXPECT_EQ(chip8.get_register_value(register_index0), 0x8B);
}
