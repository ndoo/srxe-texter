
#include "_avr_includes.h"
#include "_srxe_includes.h"

#define TITLE "SRXE Texter"

#define PERIODIC_INTERVAL 3000	// update rate for stats
#define KEYSCAN_RATE 10 		// milliseconds between keyboard scans
#define RF_CHANNEL 16 			// 1 .. 16
#define INPUT_LINES 3

static bool _redraw_needed = true;
static unsigned long _update_timer;
static unsigned long _keyscan_timer;

static char transmit_buffer[RF_TX_BUFFER_SIZE] = "";
static uint8_t transmit_length = 0;

void updateStatusBar() {

	lcdFontSet(FONT2);
	lcdColorSet(LCD_WHITE, LCD_BLACK);

	// Static content
	if (_redraw_needed) {
		lcdRectangle(0, 0, LCD_WIDTH, lcdFontHeightGet() + 3, true);
		lcdPositionSet(1, 2);
		lcdPutString(TITLE);
	}

	// Dynamic content
	uint16_t voltage = powerBatteryLevel();
	lcdPositionSet(LCD_WIDTH - 1 - lcdFontWidthGet() * 5, 2);
	printDevicePrintf(PRINT_LCD, "%d.%02dV", voltage / 1000, (voltage % 1000) / 10);


	lcdPositionSet(LCD_WIDTH - 1 - lcdFontWidthGet() * 14, 2);
	printDevicePrintf(PRINT_LCD, "% 3d/% 3d", transmit_length, RF_TX_BUFFER_SIZE);

}

void updateInputBox() {
	lcdFontSet(FONT2);
	lcdColorSet(LCD_BLACK, LCD_WHITE);

	uint8_t rows, cols, start_y, x, y;
	cols = LCD_WIDTH / lcdFontWidthGet();
	rows = transmit_length / cols + 1;

	start_y = LCD_HEIGHT - lcdFontHeightGet() * rows;

	for (uint8_t i = 0; i < rows * cols; i++) {
		y = start_y + i / cols * lcdFontHeightGet();
		x = i % cols * lcdFontWidthGet();
		if (i < transmit_length) {
			lcdPositionSet(x, y);
			lcdPutChar(transmit_buffer[i]);
		} else {
			lcdRectangle(x, y, lcdFontWidthGet(), lcdFontHeightGet(), LCD_ERASE);
		}
	}

}

void updateDisplay() {
	if (clockMillis() >= _update_timer) {
		_update_timer = clockMillis() + PERIODIC_INTERVAL;
		updateStatusBar();
		if (_redraw_needed) _redraw_needed = false;
	}
}

void handlePowerButton() {
	if (powerButtonPressed()) {

		// Turn off
		if (rfInited()) rfTerm();
		uint8_t lcdContrast = lcdContrastGet();
		lcdSleep();
		powerSleep();
		_redraw_needed = true;

		// Woken up
		lcdColorSet(LCD_BLACK, LCD_WHITE);
		lcdClearScreen();
		lcdWake();
		lcdContrastSet(lcdContrast);
		rfInit(RF_CHANNEL);

		_update_timer = clockMillis();
		_keyscan_timer = _update_timer;
		updateDisplay();

	}
}

void handleKeys() {
	if (clockMillis() >= _keyscan_timer) {
		_keyscan_timer = clockMillis() + KEYSCAN_RATE;
		// action linked to some keypress
		uint8_t key = kbdGetKey();
		switch (key) {
			case KEY_NOP:
				return;
			case KEY_UP:
				lcdContrastIncrease();
				break;
			case KEY_DOWN:
				lcdContrastDecrease();
				break;
			case KEY_LEFT:
				lcdContrastReset();
				break;
			case KEY_RIGHT:
			case KEY_ENTER:
				memset(transmit_buffer, 0, sizeof(transmit_buffer));
				transmit_length = 0;
				break;
			case KEY_DEL:
				if (transmit_length) {
					transmit_buffer[--transmit_length] = 0;
				}
				break;
			default:
				if (key < 0x20 || key > 0x7e) break;
				if (transmit_length < sizeof(transmit_buffer) - 1) {
					transmit_buffer[transmit_length++] = key;
				}
				break;
		}
		updateInputBox();
	}
}

void loop() {
	handlePowerButton();
	updateDisplay();
	handleKeys();
}

int main() {
	clockInit();
	powerInit();
	//rfInit(RF_CHANNEL);
	//randomInit(); // (must be after RF)
	kbdInit();
	lcdInit();

	_update_timer = clockMillis();
	_keyscan_timer = _update_timer;

	while (true) loop();

	return 0;
}
