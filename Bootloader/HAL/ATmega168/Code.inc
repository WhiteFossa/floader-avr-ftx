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

; Hardware-specific code for ATmega168

; UART divider
.ifdef	UseDoubleSpeedUARTMode
	.set	BaudrateDivider				= OscFreq / (8 * Baudrate) - 1
.else
	.set	BaudrateDivider				= OscFreq / (16 * Baudrate) - 1
.endif

; Disable or slow down WDT procedure
DisableOrSlowDownWDT:
							push		R16
							push		R17
							uin			R16,		SREG
							push		R16

							; Clearing "reset by WDT" flag
							in			R16,		MCUSR
							andi		R16,		~(1 << WDRF)
							out			MCUSR,		R16

							; As slow as possible (8 seconds), WDT off
							ldi			R17,		(1 << WDP3) | (1 << WDP0)

							lds			R16,		WDTCSR
							ori			R16,		(1 << WDCE) | (1 << WDE)
							sts			WDTCSR,		R16

							; Disabling timer (it may not be disabled if WDTON fuse is programmed)
							sts			WDTCSR,		R17

							pop			R16
							uout		SREG,		R16
							pop			R17
							pop			R16
							ret


; Call this to initialize hardware (before bootloader mode enter)
InitializeHardwarePreEnter:
							push		R16
							push		R17
							push		R18
							uin			R16,		SREG
							push		R16

							; All ports to input to avoid hardware damage
							clr			R16
							uout		DDRB,		R16
							uout		DDRC,		R16
							uout		DDRD,		R16

							; Listening pin pull-up enable
							sbi			ListenPort,	ListenBit

							; Letting debouncing capacitor at bootloader enter button charge
							ldi			R16,		PollPauseOuter

InitializeHardwarePreEnterPauseOuterLoop:
							ldi			R17,		PollPauseMiddle

InitializeHardwarePreEnterPauseMiddleLoop:
							ldi			R18,		PollPauseInner

InitializeHardwarePreEnterPauseInnerLoop:
							dec			R18
							brne		InitializeHardwarePreEnterPauseInnerLoop

							dec			R17
							brne		InitializeHardwarePreEnterPauseMiddleLoop

							dec			R16
							brne		InitializeHardwarePreEnterPauseOuterLoop

InitializeHardwarePreEnterExit:
							pop			R16
							uout		SREG,		R16
							pop			R18
							pop			R17
							pop			R16
							ret


; Call this to initialize hardware (after bootloader mode enter)
InitializeHardwarePostEnter:
							push		R16
							uin			R16,		SREG
							push		R16

							; Setting LED pins to output, LEDs not lit yet
							sbi			LedDDR,		LedBit0
							sbi			LedDDR,		LedBit1
							cbi			LedPort,	LedBit0
							cbi			LedPort,	LedBit1

							; UART
							ldi			R16,		low(BaudrateDivider)
							uout		UBRR0L,		R16
							ldi			R16,		high(BaudrateDivider)
							andi		R16,		0xF0 ; UBRR0H mask (to avoid writing in reserved bits)
							uout		UBRR0H,		R16

.ifdef	UseDoubleSpeedUARTMode
							ldi			R16,		1 << U2X0 ; Double speed
.else
							clr			R16
.endif
							uout		UCSR0A,		R16

							ldi			R16,		(1 << RXEN0) | (1 << TXEN0) ; Enabling TX and RX, no interrupts
							uout		UCSR0B,		R16

							ldi 		R16, 		(1 << UCSZ00) | (1 << UCSZ01) ; 8 bit data, 1 stop bit, no parity
							uout		UCSR0C,		R16

							pop			R16
							uout		SREG,		R16
							pop			R16
							ret

; Call this procedure to signalize that device is in bootloader mode, for example
; to light up the LED
SignalizeBootloaderMode:
							sbi			LedPort,	LedBit0
							sbi			LedPort,	LedBit1
							ret


; Checks do we need to enter bootloader. If yes, then R16 is 0, otherwise not 0
IsEnterBootloader:
							push		R17
							uin			R17,		SREG
							push		R17

							uin			R16,		ListenPin
							andi		R16,		1 << ListenBit

							breq		IsEnterBootloaderGoingIn
							ser			R16

IsEnterBootloaderExit:
							pop			R17
							uout		SREG,		R17
							pop			R17
							ret

IsEnterBootloaderGoingIn:
							clr			R16
							rjmp		IsEnterBootloaderExit


; Call this to wait for byte from UART. Blocks until byte is received, received byte is stored in R16
UartReadByte:
							push		R17
							uin			R17,		SREG
							push		R17

UartReadByteWait:
							wdr
							uin			R16,		UCSR0A
							sbrs		R16,		RXC0 ; Waiting for byte
							rjmp		UartReadByteWait

							; Byte came
							uin			R16,		UDR0

							pop			R17
							uout		SREG,		R17
							pop			R17
							ret


; Call this to send byte over UART. Blocks until UART is ready for transmission. Byte must be in R16.
UartSendByte:
							push		R17
							uin			R17,		SREG
							push		R17

							; Waiting for readyness
UartSendByteWait:
							wdr
							uin			R17,		UCSR0A
							sbrs		R17,		UDRE0
							rjmp		UartSendByteWait

							uout		UDR0,		R16

							pop			R17
							uout		SREG,		R17
							pop			R17
							ret


; Call this to read EEPROM data. Put address into XH:XL and data will be returned in R16.
; Doesn't check address correctness.
ReadEeprom:
							push		R17
							uin			R17,		SREG
							push		R17

							; Waiting wor write complete (if it was initiated before)
ReadEepromWaitForWriteCompletion:
							wdr
							sbic		EECR,		EEPE
							rjmp		ReadEepromWaitForWriteCompletion

							; Set up address
							uout		EEARH,		XH
							uout		EEARL,		XL

							; Reading
							sbi			EECR,		EERE
							uin			R16,		EEDR

							pop			R17
							uout		SREG,		R17
							pop			R17
							ret


; Call this to write EEPROM data. Put address into XH:XL and data into R16.
; Doesn't check address correctness
WriteEeprom:
							push		R17
							uin			R17,		SREG
							push		R17

WriteEepromWaitForWriteCompletion:
							wdr
							sbic		EECR,		EEPE
							rjmp		WriteEepromWaitForWriteCompletion

							; Set up address
							uout		EEARH,		XH
							uout		EEARL,		XL

							; Writing
							uout		EEDR,		R16
							sbi			EECR,		EEMPE
							sbi			EECR,		EEPE

							pop			R17
							uout		SREG,		R17
							pop			R17
							ret


; Converts FLASH page number (like 0-127) into word address in ZH:ZL
; Put page number into R17
FlashPageNumberIntoAddress:
							push		R16
							uin			R16,		SREG
							push		R16

							; Converting page number into physical address in ZH:ZL
							mov			ZL,				R17
							andi		ZL,				0b00000001 ; Selecting least-significant bit and moving it into most significant position of ZL
							bst			ZL,				0
							clr			ZL
							bld			ZL,				7 ; Now we have correct content of ZL

							mov			ZH,				R17
							andi		ZH,				0b01111110 ; Selecting most significant bits
							lsr			ZH

							pop			R16
							uout		SREG,			R16
							pop			R16
							ret


; Makes SPM call. Value to be written into SPMCR must be in R16
; Blocks until previous SPM completed
MakeSPM:
							push		R17
							uin			R17,			SREG
							push		R17

							; Waiting for previous write completion
MakeSPMWait:
							wdr
							uin			R17,			SPMCSR
							sbrc		R17,			SELFPRGEN
							rjmp		MakeSPMWait

							uout		SPMCSR,			R16
							spm

							pop			R17
							uout		SREG,			R17
							pop			R17
						ret
