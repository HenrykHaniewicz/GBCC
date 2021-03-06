#include "gbcc.h"
#include "gbcc_debug.h"
#include "gbcc_memory.h"
#include "gbcc_save.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>

#define MAX_NAME_LEN 4096

static void strip_ext(char *fname);

void gbcc_save(struct gbc *gbc)
{
	if (gbc->cart.ram_size == 0) {
		return;
	}
	if (!gbc->initialised) {
		gbcc_log(GBCC_LOG_INFO, "GBC not initialised, not saving.\n");
		return;
	}
	char fname[MAX_NAME_LEN];
	char tmp[MAX_NAME_LEN];
	FILE *sav;
	if (snprintf(tmp, MAX_NAME_LEN, "%s", gbc->cart.filename) >= MAX_NAME_LEN) {
		gbcc_log(GBCC_LOG_ERROR, "Filename %s too long\n", tmp);
	}
	strip_ext(tmp);
	if (snprintf(fname, MAX_NAME_LEN, "%s.sav", tmp) >= MAX_NAME_LEN) {
		gbcc_log(GBCC_LOG_ERROR, "Filename %s too long\n", fname);
	}
	gbcc_log(GBCC_LOG_INFO, "Saving %s...\n", fname);
	sav = fopen(fname, "wbe");
	if (sav == NULL) {
		gbcc_log(GBCC_LOG_ERROR, "Can't open save file %s\n", fname);
		exit(EXIT_FAILURE);
	}
	fwrite(gbc->cart.ram, 1, gbc->cart.ram_size, sav);
	fclose(sav);
	gbcc_log(GBCC_LOG_INFO, "Saved.\n");
}

void gbcc_load(struct gbc *gbc)
{
	char fname[MAX_NAME_LEN];
	char tmp[MAX_NAME_LEN];
	FILE *sav;
	if (snprintf(tmp, MAX_NAME_LEN, "%s", gbc->cart.filename) >= MAX_NAME_LEN) {
		gbcc_log(GBCC_LOG_ERROR, "Filename %s too long\n", tmp);
	}
	strip_ext(tmp);
	if (snprintf(fname, MAX_NAME_LEN, "%s.sav", tmp) >= MAX_NAME_LEN) {
		gbcc_log(GBCC_LOG_ERROR, "Filename %s too long\n", fname);
	}
	sav = fopen(fname, "rbe");
	if (sav == NULL) {
		return;
	}
	gbcc_log(GBCC_LOG_INFO, "Loading %s...\n", fname);
	fread(gbc->cart.ram, 1, gbc->cart.ram_size, sav);
	fclose(sav);
}

void gbcc_save_state(struct gbc *gbc)
{
	char fname[MAX_NAME_LEN];
	char tmp[MAX_NAME_LEN];
	FILE *sav;
	if (snprintf(tmp, MAX_NAME_LEN, "%s", gbc->cart.filename) >= MAX_NAME_LEN) {
		gbcc_log(GBCC_LOG_ERROR, "Filename %s too long\n", tmp);
	}
	strip_ext(tmp);
	if (snprintf(fname, MAX_NAME_LEN, "%s.s%d", tmp, gbc->save_state) >= MAX_NAME_LEN) {
		gbcc_log(GBCC_LOG_ERROR, "Filename %s too long\n", fname);
	}
	sav = fopen(fname, "wbe");
	if (!sav) {
		gbcc_log(GBCC_LOG_ERROR, "Error opening %s: %s\n", fname, strerror(errno));
		return;
	}
	gbc->save_state = 0;
	gbc->load_state = 0;
	fwrite(gbc, sizeof(struct gbc), 1, sav);
	if (gbc->cart.ram_size > 0) {
		fwrite(gbc->cart.ram, 1, gbc->cart.ram_size, sav);
	}
	fclose(sav);
}

void gbcc_load_state(struct gbc *gbc)
{
	uint8_t *rom = gbc->cart.rom;
	uint8_t *ram = gbc->cart.ram;
	uint8_t wram_bank;
	uint8_t vram_bank;
	const char *name = gbc->cart.filename;

	char fname[MAX_NAME_LEN];
	char tmp[MAX_NAME_LEN];
	FILE *sav;
	if (snprintf(tmp, MAX_NAME_LEN, "%s", gbc->cart.filename) >= MAX_NAME_LEN) {
		gbcc_log(GBCC_LOG_ERROR, "Filename %s too long\n", tmp);
	}
	strip_ext(tmp);
	if (snprintf(fname, MAX_NAME_LEN, "%s.s%d", tmp, gbc->load_state) >= MAX_NAME_LEN) {
		gbcc_log(GBCC_LOG_ERROR, "Filename %s too long\n", fname);
	}
	sav = fopen(fname, "rbe");
	if (!sav) {
		gbcc_log(GBCC_LOG_ERROR, "Error opening %s: %s\n", fname, strerror(errno));
		gbc->save_state = 0;
		gbc->load_state = 0;
		return;
	}
	fread(gbc, sizeof(struct gbc), 1, sav);
	/* FIXME: Thread-unsafe, screen could try to read from here while the
	 * pointer is still invalid */
	gbc->memory.gbc_screen = gbc->memory.screen_buffer_0;
	gbc->memory.sdl_screen = gbc->memory.screen_buffer_1;

	gbc->cart.rom = rom;
	gbc->cart.ram = ram;
	gbc->cart.filename = name;
	
	if (gbc->cart.ram_size > 0) {
		fread(gbc->cart.ram, 1, gbc->cart.ram_size, sav);
	}

	switch (gbc->mode) {
		case DMG:
			wram_bank = 1;
			vram_bank = 0;
			break;
		case GBC:
			wram_bank = gbcc_memory_read(gbc, SVBK, true);
			vram_bank = gbcc_memory_read(gbc, VBK, true);
			break;
	}

	gbc->memory.romx = gbc->cart.rom + (gbc->memory.romx - gbc->memory.rom0);
	gbc->memory.rom0 = gbc->cart.rom;
	gbc->memory.vram = gbc->memory.vram_bank[vram_bank];
	gbc->memory.sram = gbc->cart.ram;
	gbc->memory.wram0 = gbc->memory.wram_bank[0];
	gbc->memory.wramx = gbc->memory.wram_bank[wram_bank];
	gbc->memory.echo = gbc->memory.wram0;
	
	gbc->keys.a = false;
	gbc->keys.b = false;
	gbc->keys.start = false;
	gbc->keys.select = false;
	gbc->keys.dpad.up = false;
	gbc->keys.dpad.down = false;
	gbc->keys.dpad.left = false;
	gbc->keys.dpad.right = false;
	gbc->keys.turbo = false;
	fclose(sav);
}

void strip_ext(char *fname)
{
	char *end = fname + strlen(fname);
	while (end > fname && *end != '.') {
		--end;
	}

	if (end > fname) {
		*end = '\0';
	}
}
