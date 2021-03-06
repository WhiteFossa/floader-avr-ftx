;
;	                    Fossa's AVR bootloader
; Copyright (C) 2020 White Fossa aka Artyom Vetrov <whitefossa@protonmail.com>
;
; This program is free software: you can redistribute it and/or modify
; it under the terms of the GNU Affero General Public License as published by
; the Free Software Foundation, either version 3 of the License, or
; (at your option) any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU Affero General Public License for more details.
;
; You should have received a copy of the GNU Affero General Public License
; along with this program.  If not, see <https://www.gnu.org/licenses/>.
;

; Hardware-specific defines for ATmega168

.include "HAL/ATmega168/m168def.inc"

.set	BootloaderStartAddress		=	0x1E00			; Bootloader starts here

; Light up this LEDs on bootloader enter
.set	LedPort						= PORTC
.set	LedDDR						= DDRC
.set	LedBit0						= 1 ; LED 0
.set	LedBit1						= 4 ; LED 1

; Pause before polling bootloader enter button
.set	PollPauseOuter				= 0x04
.set	PollPauseMiddle				= 0xFF
.set	PollPauseInner				= 0xFF

; Listen this port for bootloader button press (zero level)
.set	ListenPort					= PORTB
.set	ListenDDR					= DDRB
.set	ListenPin					= PINB
.set	ListenBit					= 0

; Clock
.set	OscFreq						= 4096000

; UART
.set	Baudrate					= 38400
.set	UseDoubleSpeedUARTMode		= 1 ; Uncomment this line to use double speed mode (use it when you need fast UART
; and have slow system clock)

; FLASH
.set	FlashPagesTotal				= 128
.set	FlashPagesWriteable			= 120
.set	FlashPageAddressMultiplier	= 7 ; Multiply page address to this value to get page first byte address
.set	FlashPageSize				= 128
.set	FlashPageSizeInWords		= 64

