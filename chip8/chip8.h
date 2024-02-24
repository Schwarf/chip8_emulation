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
#include <unordered_map>
#include <functional>
template<size_t memory_in_bytes, size_t number_of_registers, size_t width_in_pixels, size_t height_in_pixels,
	size_t number_of_stack_levels, size_t number_of_keys>
class Chip8
{
	using Bit16 = unsigned short;
	using Bit8 = unsigned char;

public:
	Chip8()
	{
		loadSpritesToMemory();
		initializeOpcodeMap();
	}

	std::vector<Bit8> load_program(const std::string &filename)
	{
		std::ifstream file(filename, std::ios::binary | std::ios::ate);
		if (!file) {
			std::cerr << "Failed to open file: " << filename << std::endl;
			return {};
		}
		std::streamsize size = file.tellg();
		file.seekg(0, std::ios::beg);

		std::vector<Bit8> buffer(size);
		if (file.read(reinterpret_cast<char *>(buffer.data()), size)) {
			return buffer;
		}
		else {
			std::cerr << "Error while reading file: " << filename << std::endl;
			return {};

		}
	}

	void fetch_operation_code()
	{
		current_opcode = memory[program_counter++] << 8 | memory[program_counter++];
	}

	void initialize()
	{

		program_counter = 0x200;
		current_opcode = 0;
		index_register = 0;
		stack_pointer = 0;
		clearScreen();
		std::fill(stack.begin(), stack.end(), 0);
		std::fill(registers.begin(), registers.end(), 0);
		std::fill(memory.begin(), memory.end(), 0);
		loadSpritesToMemory();
		delayed_timer = 0;
		sound_timer = 0;
	}

	void emulateCycle()
	{
		current_opcode = memory[program_counter] << 8 | memory[program_counter + 1];
		// MIssing
		if (delayed_timer > 0)
			--delayed_timer;
		if (sound_timer > 0) {
			if (sound_timer == 1)
				printf("BEEP!\n");
			--sound_timer;
		}
	}


private:
	void clearScreen()
	{
		std::fill(graphics.begin(), graphics.end(), 0);
	}

	void returnFromSubroutine()
	{
		--stack_pointer;
		program_counter = stack[stack_pointer];
		program_counter += 2;
	}

	void jumpToAddress()
	{
		program_counter = current_opcode & 0x0FFF;
	}

	void jumptToSubroutine()
	{
		stack[stack_pointer] = program_counter;
		++stack_pointer;
		program_counter = current_opcode & 0x0FFF;
	}

	// 0x3XNN: Skips the next instruction if VX equals NN
	void skipNextInstructionIfRegisterDoesMatch()
	{
		const int register_index = (current_opcode & 0x0F00) >> 8;
		const int value = (current_opcode & 0x00FF);
		program_counter += 2;
		if (value == registers[register_index]) {
			program_counter += 2;
		}
	}

	// 0x4XNN: Skips the next instruction if VX NOT equals NN
	void skipNextInstructionIfRegisterDoesNotMatch()
	{
		const int register_index = (current_opcode & 0x0F00) >> 8;
		const int value = (current_opcode & 0x00FF);
		program_counter += 2;
		if (value != registers[register_index]) {
			program_counter += 2;
		}
	}

	void skipNextInstructionIfRegisterMatch()
	{
		const int register_index1 = (current_opcode & 0x0F00) >> 8;
		const int register_index2 = (current_opcode & 0x00F0) >> 4;
		program_counter += 2;
		if (registers[register_index1] == registers[register_index2]) {
			program_counter += 2;
		}
	}

	void setValueInRegister()
	{
		const int register_index = (current_opcode & 0x0F00) >> 8;
		const int value_to_set = (current_opcode & 0x00FF);
		registers[register_index] = value_to_set;
		program_counter+=2;
	}

	void addsValueToRegister()
	{
		const int register_index = (current_opcode & 0x0F00) >> 8;
		const int value_to_add = (current_opcode & 0x00FF);
		registers[register_index] += value_to_add;
		program_counter+=2;
	}

	void twoRegisterOperations()
	{
		const int register_index = (current_opcode & 0x0F00) >> 8;
		const int value_to_add = (current_opcode & 0x00FF);
		registers[register_index] += value_to_add;
		program_counter+=2;
	}


	void initializeOpcodeMap()
	{
		opcodeMap[0x00E0] = [this]()
		{ this->clearScreen(); };
		opcodeMap[0x00EE] = [this]()
		{ this->returnFromSubroutine(); };
		opcodeMap[0x1000] = [this]()
		{ this->jumpToAddress(); };
		// 0x2NNN: Calls subroutine at NNN.
		opcodeMap[0x2000] = [this]()
		{ this->jumptToSubroutine(); };
		// 0x3XNN: Skips the next instruction if VX equals NN
		opcodeMap[0x3000] = [this]()
		{ this->skipNextInstructionIfRegisterDoesMatch(); };
		// 0x4XNN: Skips the next instruction if VX NOT equals NN
		opcodeMap[0x4000] = [this]()
		{ this->skipNextInstructionIfRegisterDoesNotMatch(); };
		// 0x5XY0: Skips the next instruction if VX equals VY.
		opcodeMap[0x5000] = [this]()
		{ this->skipNextInstructionIfRegisterMatch(); };
		// 0x6XNN: Sets VX to NN.
		opcodeMap[0x6000] = [this]()
		{this->setValueInRegister();};
		// 0x7XNN: Adds NN to VX.
		opcodeMap[0x7000] = [this]()
		{this->addsValueToRegister();};
		// 0x8XYZ: Two register operations
		opcodeMap[0x8000] = [this]()
		{this->twoRegisterOperations();};


	}

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
	std::unordered_map<int, std::function<void()>> opcodeMap;

	Bit16 current_opcode{};
	Bit16 index_register{};
	Bit16 program_counter = 0x200;
	Bit8 stack_pointer{};
	Bit8 delayed_timer{};
	Bit8 sound_timer{};

};


#endif //CHIP8_H
