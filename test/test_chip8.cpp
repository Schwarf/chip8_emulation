//
// Created by andreas on 26.02.24.
//
#include "gtest/gtest.h"
#include "./../chip8/chip8.h"

class Chip8Test : public testing::Test {
protected:
	static constexpr size_t memory_in_bytes{4096};
	static constexpr size_t number_of_registers{16};
	static constexpr size_t width_in_pixels{64};
	static constexpr size_t height_in_pixels{32};
	static constexpr size_t number_of_stack_levels{16};
	static constexpr size_t number_of_keys{16};

	Chip8<memory_in_bytes, number_of_registers, width_in_pixels, height_in_pixels, number_of_stack_levels, number_of_keys> chip8;
public:

	using Bit16 = Chip8<memory_in_bytes, number_of_registers, width_in_pixels, height_in_pixels, number_of_stack_levels, number_of_keys>::Bit16;
	using Bit8 = Chip8<memory_in_bytes, number_of_registers, width_in_pixels, height_in_pixels, number_of_stack_levels, number_of_keys>::Bit8;
	void test(int opcode){
		chip8.opcodeMap[opcode]();
	}

	const Bit8 get_register_value(int register_index) const
	{
		return chip8.registers[register_index];
	}

	const Bit16 get_current_opcode() const
	{
		return chip8.current_opcode;
	}

	const Bit16 get_program_counter() const
	{
		return chip8.program_counter;
	}

	const std::array<Bit8, memory_in_bytes> & get_memory() const
	{
		return chip8.memory;
	}

	const std::array<Bit8, width_in_pixels*height_in_pixels> & get_graphics() const
	{
		return chip8.graphics;
	}

	const std::array<Bit8, number_of_stack_levels> & get_stack() const
	{
		return chip8.stack;
	}

	const std::array<Bit8, number_of_keys> & get_keypad() const
	{
		return chip8.keypad;
	}

};
