#include "patterns.h"

static const uint8_t data[] = {
		0xFF, 0xCC, 0x88, 0x44, 0, 0x44, 0x88, 0xCC,
		0xFF, 0xCC, 0x88, 0x44, 0, 0x44, 0x88, 0xCC,
};

uint8_t fading_pattern(uint8_t c, uint8_t color, uint8_t col, uint8_t row, uint8_t target_color) {
	static uint8_t lit = 0;

	if (c == 0 && col == 0 && row == 0)
		lit = (lit + 1) & 0x07;

	// Only show for blue
	if (color == target_color) {
		return data[(col & 0x07) + lit];
	} else
		return 0;
}

uint8_t fading_pattern_b(uint8_t c, uint8_t color, uint8_t col, uint8_t row) {
	return fading_pattern(c, color, col, row, 0);
}

uint8_t fading_pattern_g(uint8_t c, uint8_t color, uint8_t col, uint8_t row) {
	return fading_pattern(c, color, col, row, 1);
}

uint8_t fading_vertical_pattern(uint8_t c, uint8_t color, uint8_t col, uint8_t row) {
	static uint8_t lit = 0;

	if (c == 0 && col == 0 && row == 0)
		lit = (lit + 1) & 0x07;

	return data[(row & 0x07) + lit];
}

uint8_t diagonal_pattern(uint8_t c, uint8_t color, uint8_t col, uint8_t row) {
	return (col - color) % 8 == row ? 0xFF : 0;
}