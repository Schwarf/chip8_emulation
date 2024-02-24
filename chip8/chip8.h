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

		if(skip_instruction) {
			program_counter += 4;
			skip_instruction = false;
		}
		else if(advance_program_counter)
		{
			program_counter +=2;
		}
		else
		{
			advance_program_counter = true;
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
	}

	void jumpToAddress()
	{
		program_counter = current_opcode & 0x0FFF;
		advance_program_counter = false;
	}

	void jumptToSubroutine()
	{
		stack[stack_pointer] = program_counter;
		++stack_pointer;
		program_counter = current_opcode & 0x0FFF;
		advance_program_counter = false;
	}

	// 0x3XNN: Skips the next instruction if VX equals NN
	void skipNextInstructionIfRegisterDoesMatch()
	{
		const int register_index = (current_opcode & 0x0F00) >> 8;
		const int value = (current_opcode & 0x00FF);
		if (value == registers[register_index]) {
			skip_instruction = true;
		}
	}

	// 0x4XNN: Skips the next instruction if VX NOT equals NN
	void skipNextInstructionIfRegisterDoesNotMatch()
	{
		const int register_index = (current_opcode & 0x0F00) >> 8;
		const int value = (current_opcode & 0x00FF);
		if (value != registers[register_index]) {
			skip_instruction = true;
		}
	}

	void skipNextInstructionIfRegisterMatch()
	{
		const int register_index1 = (current_opcode & 0x0F00) >> 8;
		const int register_index2 = (current_opcode & 0x00F0) >> 4;
		if (registers[register_index1] == registers[register_index2]) {
			skip_instruction = true;
		}
	}

	void setValueInRegister()
	{
		const int register_index = (current_opcode & 0x0F00) >> 8;
		const int value_to_set = (current_opcode & 0x00FF);
		registers[register_index] = value_to_set;
	}

	void addsValueToRegister()
	{
		const int register_index = (current_opcode & 0x0F00) >> 8;
		const int value_to_add = (current_opcode & 0x00FF);
		registers[register_index] += value_to_add;
	}

	void twoRegisterOperations()
	{
		const int register_index1 = (current_opcode & 0x0F00) >> 8;
		const int register_index2 = (current_opcode & 0x00F0) >> 4;
		const int operation_index = (current_opcode & 0x000F);
		if(twoRegisterOperationsMap.find(operation_index) == twoRegisterOperationsMap.end())
		{
			std::cerr << "Invalid operations_index for two register operations: "<< operation_index << std::endl;
			return;
		}
		twoRegisterOperationsMap[operation_index](register_index1, register_index2);
	}

	void setIndexRegister()
	{
		index_register = current_opcode & 0x0FFF;
	}

	void jumpToAddressPlusRegisterZero()
	{
		program_counter = (current_opcode & 0x0FFF) + registers[0];
		advance_program_counter = false;
	}

	void setRegisterToRandom()
	{
		const int register_index =  (current_opcode & 0x0F00) >> 8;
		registers[register_index] = (rand() % 0xFF)	& (current_opcode & 0x00FF);
	}



	void initializeTwoRegisterOperations()
	{
		twoRegisterOperationsMap[0] = [this](const int register_index1, const int register_index2)
		{ this->oneEqualTwo(register_index1, register_index2); };
		twoRegisterOperationsMap[1] = [this](const int register_index1, const int register_index2)
		{ this->oneEqualOneOrTwo(register_index1, register_index2); };
		twoRegisterOperationsMap[2] = [this](const int register_index1, const int register_index2)
		{ this->oneEqualOneAndTwo(register_index1, register_index2); };
		twoRegisterOperationsMap[3] = [this](const int register_index1, const int register_index2)
		{ this->oneEqualOneXorTwo(register_index1, register_index2); };
		twoRegisterOperationsMap[4] = [this](const int register_index1, const int register_index2)
		{ this->oneEqualOnePlusTwo(register_index1, register_index2); };
		twoRegisterOperationsMap[5] = [this](const int register_index1, const int register_index2)
		{ this->oneEqualOneMinusTwo(register_index1, register_index2); };
		twoRegisterOperationsMap[6] = [this](const int register_index1, const int register_index2)
		{ this->oneEqualRightShiftOne(register_index1, register_index2); };
		twoRegisterOperationsMap[7] = [this](const int register_index1, const int register_index2)
		{ this->oneEqualTwoMinusOne(register_index1, register_index2); };
		twoRegisterOperationsMap[14] = [this](const int register_index1, const int register_index2)
		{ this->oneEqualLeftShiftOne(register_index1, register_index2); };

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
		{ this->setValueInRegister(); };
		// 0x7XNN: Adds NN to VX.
		opcodeMap[0x7000] = [this]()
		{ this->addsValueToRegister(); };
		// 0x8XYZ: Two register operations
		opcodeMap[0x8000] = [this]()
		{ this->twoRegisterOperations(); };
		opcodeMap[0x9000] = [this]()
		{ this->skipNextInstructionIfRegisterDoesNotMatch(); };
		opcodeMap[0xA000] = [this]()
		{ this->setIndexRegister(); };
		opcodeMap[0xB000] = [this]()
		{ this->jumpToAddressPlusRegisterZero(); };
		opcodeMap[0xC000] = [this]()
		{ this->setRegisterToRandom(); };

		opcodeMap[0xD000] = [this]()
		{ this->drawASprite(); };

	}

	void drawASprite()
	{
		const int register_index1 = (current_opcode & 0x0F00) >> 8;
		const int register_index2 = (current_opcode & 0x0F0) >> 4;
		auto x = registers[register_index1];
		auto y = registers[register_index2];
		unsigned short height = current_opcode & 0x000F;
		registers[number_of_registers-1] = 0;
		for(int y_line{}; y_line< height; ++y_line)
		{
			unsigned short pixel = memory[index_register + y_line];
			for(int x_line{}; x_line< height; ++x_line) {
				if((pixel & (0x80 >> x_line)) != 0)
				{
					if(graphics[(x + x_line + ((y + y_line) * 64))] == 1)
					{
						registers[number_of_registers-1] = 1;
					}
					graphics[x + x_line + ((y + y_line) * 64)] ^= 1;
				}
			}
		}
		draw_flag = true;
	}

	inline void oneEqualTwo(const int register_index1, const int register_index2)
	{
		registers[register_index1] = registers[register_index2];
	}

	inline void oneEqualOneOrTwo(const int register_index1, const int register_index2)
	{
		registers[register_index1] = registers[register_index2] | registers[register_index1];
	}

	inline void oneEqualOneAndTwo(const int register_index1, const int register_index2)
	{
		registers[register_index1] = registers[register_index2] & registers[register_index1];
	}

	void oneEqualOneXorTwo(const int register_index1, const int register_index2)
	{
		registers[register_index1] = registers[register_index2] ^ registers[register_index1];
	}

	void oneEqualOnePlusTwo(const int register_index1, const int register_index2)
	{
		// max value is 0xFF. if value stored in index2 is greater than 0xFF minus value stored in index1 we have a carry
		registers[number_of_registers - 1] = 0;
		if (registers[register_index2] > 0xFF - registers[register_index1]) {
			registers[number_of_registers - 1] = 1;// set carry
		}
		registers[register_index1] += registers[register_index2];
	}

	void oneEqualOneMinusTwo(const int register_index1, const int register_index2)
	{
		// if value in register_index2 is larger than value stored at register_index1 then we have a borrow
		registers[number_of_registers - 1] = 1;
		if (registers[register_index2] > registers[register_index1]) {
			registers[number_of_registers - 1] = 0;// set borrow
		}
		registers[register_index1] -= registers[register_index2];
	}

	void oneEqualRightShiftOne(const int register_index1, const int register_index2)
	{
		// Shifts value at index1 right by one. The last register is set to the value of the least significant bit of the register with index1 before the shift
		//  We store least significant bit of
		registers[number_of_registers-1] = registers[register_index1] & 0x1;
		registers[register_index1] >>=1;
	}

	void oneEqualLeftShiftOne(const int register_index1, const int register_index2)
	{
		// Shifts value at index1 right by one. The last register is set to the value of the most significant bit of the register with index1 before the shift
		//  We store most significant bit of
		registers[number_of_registers-1] = registers[register_index1] >> 7;
		registers[register_index1] <<=1;
	}

	void oneEqualTwoMinusOne(const int register_index1, const int register_index2)
	{
		registers[number_of_registers - 1] = 1;
		if (registers[register_index1] > registers[register_index2]) {
			registers[number_of_registers - 1] = 0;// set borrow
		}
		registers[register_index1] = registers[register_index2] - registers[register_index1];
	}

	void skipNextInstructionIfRegisterValueAreNotEqual()
	{
		const int register_index1 = (current_opcode & 0x0F00) >> 8;
		const int register_index2 = (current_opcode & 0x00F0) >> 4;

		if(registers[register_index1] != registers[register_index2])
			skip_instruction = true;
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
	std::unordered_map<int, std::function<void(const int register1, const int register2)>> twoRegisterOperationsMap;

	Bit16 current_opcode{};
	Bit16 index_register{};
	Bit16 program_counter = 0x200;
	Bit8 stack_pointer{};
	Bit8 delayed_timer{};
	Bit8 sound_timer{};
	bool advance_program_counter{true};
	bool skip_instruction{false};
	bool draw_flag{false};

};


#endif //CHIP8_H
