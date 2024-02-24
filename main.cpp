//
// Created by andreas on 09.02.24.
//

#include "chip8/chip8.h"
int main()
{
	constexpr size_t memory_in_bytes{4096};
	constexpr size_t number_of_registers{16};
	constexpr size_t width_in_pixels{64};
	constexpr size_t height_in_pixels{32};
	constexpr size_t number_of_stack_levels{16};
	constexpr size_t number_of_keys{16};
	auto chip = Chip8<memory_in_bytes, number_of_registers, width_in_pixels, height_in_pixels, number_of_stack_levels, number_of_keys>();

}
