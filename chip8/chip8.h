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
		initializeTwoRegisterOperations();
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

	void initialize()
	{

		program_counter = 0x200;
		current_opcode = 0;
		index_register = 0;
		stack_pointer = 0;
		// clear graphics
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
		current_opcode = memory[program_counter] << 8 | memory[program_counter + 1];

		register_index1 = (current_opcode & 0x0F00) >> 8;
		register_index2 = (current_opcode & 0x00F0) >> 4;

		opcodeMap[current_opcode & 0xF000];
		if (delayed_timer > 0)
			--delayed_timer;
		if (sound_timer > 0) {
			if (sound_timer == 1)
				printf("BEEP!\n");
			--sound_timer;
		}

		if (skip_instruction) {
			program_counter += 4;
			skip_instruction = false;
		}
		else if (advance_program_counter) {
			program_counter += 2;
		}
		else {
			advance_program_counter = true;
		}
	}


private:
	void twoRegisterOperations()
	{
		const int operation_index = (current_opcode & 0x000F);
		if (twoRegisterOperationsMap.find(operation_index) == twoRegisterOperationsMap.end()) {
			std::cerr << "Invalid operations_index for two register operations: " << operation_index << std::endl;
			return;
		}
		twoRegisterOperationsMap[operation_index]();
	}

	void skipInstructionKeyRegister()
	{
		int decision = current_opcode & 0x00FF;
		// EX9E: Skips the next instruction if the key stored in register is pressed
		if (decision == 0x009E && keypad[registers[register_index1]] != 0) {
			program_counter += 2;
		}
			// EX9E: Skips the next instruction if the key stored in register is NOT pressed
		else if (decision == 0x00A1 && keypad[registers[register_index1]] == 0) {
			program_counter += 2;
		}
		else {
			std::cerr << "Unknown instruction in 'skopInstructionKeyRegister'" << std::endl;
		}
	}

	void initializeExternalActions()
	{
		// FX07: Sets VX to the value of the delay timer
		externalActionOperations[0x0007] = [this](){
			registers[register_index1] = delayed_timer;
		};
		// FX0A: A key press is awaited, and then stored in VX
		externalActionOperations[0x000A] = [this](){
			bool isKeyPressed{false};

			for(int key{} ; key < number_of_keys; ++key)
			{
				if(keypad[key] != 0)
				{
					registers[register_index1] = key;
					isKeyPressed = true;
				}
			}

			// If we didn't received a keypress, skip this cycle and try again.
			if(!isKeyPressed)
				return;
		};
		// FX15: Sets the delay timer to to register with index given at X
		externalActionOperations[0x0015] = [this](){
			delayed_timer = registers[register_index1];
		};
		// FX18: Sets the sound timer to register with index given at X
		externalActionOperations[0x0018] = [this](){
			sound_timer = registers[register_index1];
		};
		// FX1E: Add register value given at index X to index_register
		externalActionOperations[0x001E] = [this](){
			registers[number_of_registers-1] = 0;
			if(index_register + registers[register_index1] > 0xFFF)	// Last register is set to 1 if range overflow (index_register + regsiter[X] +>0xFFF), and 0 when there isn't.
			{
				registers[number_of_registers-1] = 1;
			}

			index_register += registers[register_index1];
		};
		// FX29: Sets index_register to the location of the sprite for the character in register X. Characters 0-F (in hexadecimal) are represented by a 4x5 font
		externalActionOperations[0x0029] = [this](){
			index_register = registers[register_index1] * 0x5;
		};
		// FX33: Stores the Binary-coded decimal representation of register X at the addresses index_register, index_register+1, and index_register+2
		externalActionOperations[0x0033] = [this](){
			memory[index_register + 2] = registers[register_index1] % 10; // last digit
			memory[index_register + 1] = (registers[register_index1] / 10) % 10;
			memory[index_register]     = registers[register_index1] / 100; // first digit
		};
		// FX55: Stores value in register 0 to  register X in memory starting at address index_register
		externalActionOperations[0x055] = [this](){
			for (int i{}; i <= register_index1; ++i)
				memory[index_register + i] = registers[i];
			// On the original interpreter, when the operation is done, register_index = register_index + X + 1.
			index_register += register_index1 + 1;

		};
		// FX65: Fills register 0 to register X with values from memory starting at address I
		externalActionOperations[0x065] = [this](){
			for (int i{}; i <= register_index1; ++i)
				registers[i] = memory[index_register + i];
			// On the original interpreter, when the operation is done, register_index = register_index + X + 1.
			index_register += register_index1 + 1;
		};

	}

	void initializeTwoRegisterOperations()
	{
		twoRegisterOperationsMap[0] = [this]()
		{ registers[register_index1] = registers[register_index2]; };
		twoRegisterOperationsMap[1] = [this]()
		{ registers[register_index1] = registers[register_index2] | registers[register_index1]; };
		twoRegisterOperationsMap[2] = [this]()
		{registers[register_index1] = registers[register_index2] & registers[register_index1];};
		twoRegisterOperationsMap[3] = [this]()
		{ registers[register_index1] = registers[register_index2] ^ registers[register_index1]; };
		twoRegisterOperationsMap[4] = [this]()
		{
			// max value is 0xFF. if value stored in index2 is greater than 0xFF minus value stored in index1 we have a carry
			registers[number_of_registers - 1] = 0;
			if (registers[register_index2] > 0xFF - registers[register_index1]) {
				registers[number_of_registers - 1] = 1;// set carry
			}
			registers[register_index1] += registers[register_index2];
		};
		twoRegisterOperationsMap[5] = [this]()
		{
			// if value in register_index2 is larger than value stored at register_index1 then we have a borrow
			registers[number_of_registers - 1] = 1;
			if (registers[register_index2] > registers[register_index1]) {
				registers[number_of_registers - 1] = 0;// set borrow
			}
			registers[register_index1] -= registers[register_index2];
		};
		twoRegisterOperationsMap[6] = [this]()
		{
			// Shifts value at index1 right by one. The last register is set to the value of the least significant bit of the register with index1 before the shift
			//  We store least significant bit of
			registers[number_of_registers - 1] = registers[register_index1] & 0x1;
			registers[register_index1] >>= 1;

		};
		twoRegisterOperationsMap[7] = [this]()
		{
			// Shifts value at index1 right by one. The last register is set to the value of the most significant bit of the register with index1 before the shift
			//  We store most significant bit of
			registers[number_of_registers - 1] = registers[register_index1] >> 7;
			registers[register_index1] <<= 1;

		};
		twoRegisterOperationsMap[14] = [this]()
		{
			registers[number_of_registers - 1] = 1;
			if (registers[register_index1] > registers[register_index2]) {
				registers[number_of_registers - 1] = 0;// set borrow
			}
			registers[register_index1] = registers[register_index2] - registers[register_index1];

		};

	}

	void externalActions()
	{

	}

	void initializeOpcodeMap()
	{
		// clear screen
		opcodeMap[0x00E0] = [this]()
		{
			std::fill(graphics.begin(), graphics.end(), 0);
		};
		// return from subroutine
		opcodeMap[0x00EE] = [this]()
		{
			--stack_pointer;
			program_counter = stack[stack_pointer];
		};
		// jump to address
		opcodeMap[0x1000] = [this]()
		{
			program_counter = current_opcode & 0x0FFF;
			advance_program_counter = false;
		};
		// 0x2NNN: Calls subroutine at NNN.
		opcodeMap[0x2000] = [this]()
		{
			stack[stack_pointer] = program_counter;
			++stack_pointer;
			program_counter = current_opcode & 0x0FFF;
			advance_program_counter = false;
		};
		// 0x3XNN: Skips the next instruction if VX equals NN
		opcodeMap[0x3000] = [this]()
		{
			const int value = (current_opcode & 0x00FF);
			if (value == registers[register_index1]) {
				skip_instruction = true;
			}
		};
		// 0x4XNN: Skips the next instruction if VX NOT equals NN
		opcodeMap[0x4000] = [this]()
		{
			const int value = (current_opcode & 0x00FF);
			if (value != registers[register_index1]) {
				skip_instruction = true;
			}
		};
		// 0x5XY0: Skips the next instruction if VX equals VY.
		opcodeMap[0x5000] = [this]()
		{
			if (registers[register_index1] == registers[register_index2]) {
				skip_instruction = true;
			}

		};
		// 0x6XNN: Sets VX to NN.
		opcodeMap[0x6000] = [this]()
		{
			const int value_to_set = (current_opcode & 0x00FF);
			registers[register_index1] = value_to_set;

		};
		// 0x7XNN: Adds NN to VX.
		opcodeMap[0x7000] = [this]()
		{
			const int value_to_add = (current_opcode & 0x00FF);
			registers[register_index1] += value_to_add;

		};
		// 0x8XYZ: Two register operations
		opcodeMap[0x8000] = [this]()
		{
			this->twoRegisterOperations();
		};
		opcodeMap[0x9000] = [this]()
		{
			if (registers[register_index1] != registers[register_index2])
				skip_instruction = true;
		};
		// ANNN: Set index_register to address
		opcodeMap[0xA000] = [this]()
		{
			index_register = current_opcode & 0x0FFF;
		};
		// BNNN: Jumps to the address NNN plus register 0
		opcodeMap[0xB000] = [this]()
		{
			program_counter = (current_opcode & 0x0FFF) + registers[0];
			advance_program_counter = false;
		};
		opcodeMap[0xC000] = [this]()
		{
			registers[register_index1] = (rand() % 0xFF) & (current_opcode & 0x00FF);
		};
		opcodeMap[0xD000] = [this]()
		{ this->drawASprite(); };
		opcodeMap[0xE000] = [this]()
		{ this->skipInstructionKeyRegister(); };

		opcodeMap[0xF000] = [this]()
		{ this->externalActions(); };

	}

	void drawASprite()
	{
		auto x = registers[register_index1];
		auto y = registers[register_index2];
		unsigned short height = current_opcode & 0x000F;
		registers[number_of_registers - 1] = 0;
		for (int y_line{}; y_line < height; ++y_line) {
			unsigned short pixel = memory[index_register + y_line];
			for (int x_line{}; x_line < height; ++x_line) {
				if ((pixel & (0x80 >> x_line)) != 0) {
					if (graphics[(x + x_line + ((y + y_line) * 64))] == 1) {
						registers[number_of_registers - 1] = 1;
					}
					graphics[x + x_line + ((y + y_line) * 64)] ^= 1;
				}
			}
		}
		draw_flag = true;
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
	std::unordered_map<int, std::function<void()>> twoRegisterOperationsMap;
	std::unordered_map<int, std::function<void()>> externalActionOperations;
	Bit16 current_opcode{};
	Bit16 index_register{};
	Bit16 program_counter = 0x200;
	Bit8 stack_pointer{};
	Bit8 delayed_timer{};
	Bit8 sound_timer{};
	bool advance_program_counter{true};
	bool skip_instruction{false};
	bool draw_flag{false};
	int register_index1;
	int register_index2;

};


#endif //CHIP8_H
