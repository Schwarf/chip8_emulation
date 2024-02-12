//
// Created by andreas on 09.02.24.
//

#ifndef CHIP8_H
#define CHIP8_H
#include <cstddef>
#include <vector>
#include <array>
#include <string>
#include <fstream>
#include <iostream>
template<size_t memory_in_bytes, size_t number_of_registers, size_t width_in_pixels, size_t height_in_pixels,
	size_t number_of_stack_levels, size_t number_of_keys>
class Chip8
{
	using Bit16 = unsigned short;
	using Bit8 = unsigned char;

public:
	Chip8(){
		loadSpritesToMemory();
	}

	std::vector<Bit8> load_program(const std::string & filename)
	{
		std::ifstream file(filename, std::ios::binary | std::ios::ate);
		if (!file) {
			std::cerr << "Failed to open file: " << filename << std::endl;
			return {};
		}
		std::streamsize size = file.tellg();
		file.seekg(0, std::ios::beg);

		std::vector<Bit8> buffer(size);
		if(file.read(reinterpret_cast<char*>(buffer.data()),size))
		{
			return buffer;
		}
		else
		{
			std::cerr << "Error while reading file: " << filename << std::endl;
			return {};

		}
	}

	void initialize()
	{
		program_counter = 0x200;
		current_opcode = 0;
		index_register = 0;
		stack_pointer = 0;
		std::fill(graphics.begin(), graphics.end(), 0);
		std::fill(stack.begin(), stack.end(), 0);
		std::fill(registers.begin(), registers.end(), 0);
		std::fill(memory.begin(), memory.end(), 0);
		loadSpritesToMemory();
		delayed_timer = 0;
		sound_timer = 0;
	}

	void emulateCycle()
	{

	}

private:
	void loadSpritesToMemory()
	{
		std::array<Bit8, 80> sprites = {
			0xF0, 0x90, 0x90, 0x90, 0xF0, // "0"
			0x20, 0x60, 0x20, 0x20, 0x70, // "1"
			0xF0, 0x10, 0xF0, 0x80, 0xF0, // "2"
			0xF0, 0x10, 0xF0, 0x10, 0xF0, // "3"
			0x90, 0x90, 0xF0, 0x10, 0x10, // "4"
			0xF0, 0x80, 0xF0, 0x10, 0xF0, // "5"
			0xF0, 0x80, 0xF0, 0x90, 0xF0, // "6"
			0xF0, 0x10, 0x20, 0x40, 0x40, // "7"
			0xF0, 0x90, 0xF0, 0x90, 0xF0, // "8"
			0xF0, 0x90, 0xF0, 0x10, 0xF0, // "9"
			0xF0, 0x90, 0xF0, 0x90, 0x90, // "A"
			0xE0, 0x90, 0xE0, 0x90, 0xE0, // "B"
			0xF0, 0x80, 0x80, 0x80, 0xF0, // "C"
			0xE0, 0x90, 0x90, 0x90, 0xE0, // "D"
			0xF0, 0x80, 0xF0, 0x80, 0xF0, // "E"
			0xF0, 0x80, 0xF0, 0x80, 0x80  // "F"
		};

		std::copy(sprites.begin(), sprites.end(), memory.begin());
	}


	std::array<Bit8, memory_in_bytes> memory{};
	std::array<Bit8, number_of_registers> registers{};
	std::array<Bit8, width_in_pixels * height_in_pixels> graphics{};
	std::array<Bit8, number_of_stack_levels> stack{};
	std::array<Bit8, number_of_keys> keypad{};
	Bit16 current_opcode{};
	Bit16 index_register{};
	Bit16 program_counter = 0x200;
	Bit8 stack_pointer{};
	Bit8 delayed_timer{};
	Bit8 sound_timer{};



};


#endif //CHIP8_H
