//
// Created by andreas on 09.02.24.
//

#ifndef CHIP8_H
#define CHIP8_H
#include <cstddef>
#include <vector>
#include <array>
template<size_t memory_in_bytes, size_t number_of_registers, size_t width_in_pixels, size_t height_in_pixels,
	size_t number_of_stack_levels, size_t number_of_keys>
class Chip8
{

public:
	Chip8(){}
	
	void initialize()
	{
	}

private:
	std::array<unsigned char, memory_in_bytes> memory{};
	std::array<unsigned char, number_of_registers> registers{};
	std::array<unsigned char, width_in_pixels * height_in_pixels> graphics{};
	std::array<unsigned char, number_of_stack_levels> stack{};
	std::array<unsigned char, number_of_keys> keypad{};
	unsigned short current_opcode{};
	unsigned short index_register{};
	unsigned char stack_pointer{};
	unsigned short program_counter = 0x200;
	unsigned char delayed_timer{};
	unsigned char sound_timer{};



};


#endif //CHIP8_H
