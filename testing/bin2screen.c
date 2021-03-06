#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define BUF_LEN 32
#define VRAM_SIZE 0x2000u

uint8_t bit(uint8_t b)
{
	return (uint8_t)(1u << b);
}

bool check_bit(uint8_t byte, uint8_t b)
{
	return byte & bit(b);
}

void n2c(uint8_t n)
{
	switch (n) {
		case 0:
			printf(" ");
			break;
		case 1:
			printf("░");
			break;
		case 2:
			printf("▒");
			break;
		case 3:
			printf("█");
			break;
	}
}

int main(int argc, char **argv)
{
	if (argc == 1) {
		printf("Usage: %s bin_file\n", argv[0]);
		exit(1);
	}

	uint8_t buf[BUF_LEN];
	FILE *f = fopen(argv[1], "rbe");
	int vram_pos = 0;
	printf("VRAM Bank 0:\n");
	while (fread(buf, BUF_LEN, 1, f)) {
		for (uint8_t i = 0; i < BUF_LEN; i += 2) {
			uint8_t lo = buf[i];
			uint8_t hi = buf[i + 1];
			for (uint8_t xoff = 0; xoff < 8; xoff++) {
				n2c((uint8_t)(check_bit(hi, 7 - xoff) << 1u) | check_bit(lo, 7 - xoff));
			}
			printf("\n");
			vram_pos += 2;
			if (vram_pos >= VRAM_SIZE) {
				printf("VRAM Bank 1:\n");
				vram_pos = 0;
			}
		}
	}
	fclose(f);
	return 0;
}
