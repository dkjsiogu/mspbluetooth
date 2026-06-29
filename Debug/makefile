SHELL = cmd.exe

CG_TOOL_ROOT := D:/ccs/ccs/tools/compiler/ti-cgt-msp430_21.6.1.LTS
CC := "$(CG_TOOL_ROOT)/bin/cl430"

PROJECT_ROOT := ..
CCS_MSP430_ROOT := D:/ccs/ccs/ccs_base/msp430

COMMON_FLAGS := -vmspx --data_model=restricted --use_hw_mpy=F5 --define=__MSP430F5529__ \
	--diag_warning=225 --diag_wrap=off --display_error_number \
	--silicon_errata=CPU21 --silicon_errata=CPU22 --silicon_errata=CPU23 --silicon_errata=CPU40

INCLUDE_FLAGS := --include_path="$(CCS_MSP430_ROOT)/include" \
	--include_path="$(PROJECT_ROOT)" \
	--include_path="$(PROJECT_ROOT)/application" \
	--include_path="$(PROJECT_ROOT)/drivers" \
	--include_path="$(PROJECT_ROOT)/middleware" \
	--include_path="$(PROJECT_ROOT)/fatfs" \
	--include_path="$(CG_TOOL_ROOT)/include"

CFLAGS := $(COMMON_FLAGS) $(INCLUDE_FLAGS) -g --opt_level=2 --opt_for_speed=0 \
	--gen_data_subsections=on --printf_support=minimal --preproc_with_compile

LFLAGS := $(COMMON_FLAGS) -z -m"mspbluetooth.map" --heap_size=160 --stack_size=384 \
	--cinit_hold_wdt=on -i"$(CCS_MSP430_ROOT)/include" \
	-i"$(CCS_MSP430_ROOT)/lib/5xx_6xx_FRxx" -i"$(CG_TOOL_ROOT)/lib" \
	-i"$(CG_TOOL_ROOT)/include" --reread_libs --diag_wrap=off \
	--display_error_number --warn_sections --unused_section_elimination=on \
	--xml_link_info="mspbluetooth_linkInfo.xml" --rom_model

OBJS := \
	main.obj \
	board.obj \
	bluetooth_uart.obj \
	encoder.obj \
	i2s_dac.obj \
	audio_player.obj \
	wav_reader.obj \
	HAL_SDCard.obj \
	ff.obj \
	mmc.obj

all: mspbluetooth.out

mspbluetooth.out: $(OBJS) ../lnk_msp430f5529.cmd
	@echo MSP430 Linker - building target: "$@"
	$(CC) $(LFLAGS) -o "mspbluetooth.out" $(OBJS) "../lnk_msp430f5529.cmd" -llibmath.a -llibc.a
	@echo Finished building target: "$@"

main.obj: ../main.c
	@echo MSP430 Compiler - building file: "$<"
	$(CC) $(CFLAGS) --preproc_dependency="main.d_raw" "$<"

board.obj: ../drivers/board.c
	@echo MSP430 Compiler - building file: "$<"
	$(CC) $(CFLAGS) --preproc_dependency="board.d_raw" "$<"

bluetooth_uart.obj: ../drivers/bluetooth_uart.c
	@echo MSP430 Compiler - building file: "$<"
	$(CC) $(CFLAGS) --preproc_dependency="bluetooth_uart.d_raw" "$<"

encoder.obj: ../drivers/encoder.c
	@echo MSP430 Compiler - building file: "$<"
	$(CC) $(CFLAGS) --preproc_dependency="encoder.d_raw" "$<"

i2s_dac.obj: ../drivers/i2s_dac.c
	@echo MSP430 Compiler - building file: "$<"
	$(CC) $(CFLAGS) --preproc_dependency="i2s_dac.d_raw" "$<"

audio_player.obj: ../application/audio_player.c
	@echo MSP430 Compiler - building file: "$<"
	$(CC) $(CFLAGS) --preproc_dependency="audio_player.d_raw" "$<"

wav_reader.obj: ../middleware/wav_reader.c
	@echo MSP430 Compiler - building file: "$<"
	$(CC) $(CFLAGS) --preproc_dependency="wav_reader.d_raw" "$<"

HAL_SDCard.obj: ../fatfs/HAL_SDCard.c
	@echo MSP430 Compiler - building file: "$<"
	$(CC) $(CFLAGS) --preproc_dependency="HAL_SDCard.d_raw" "$<"

ff.obj: ../fatfs/ff.c
	@echo MSP430 Compiler - building file: "$<"
	$(CC) $(CFLAGS) --preproc_dependency="ff.d_raw" "$<"

mmc.obj: ../fatfs/mmc.c
	@echo MSP430 Compiler - building file: "$<"
	$(CC) $(CFLAGS) --preproc_dependency="mmc.d_raw" "$<"

clean:
	-DEL /F $(OBJS) 2>NUL
	-DEL /F *.d_raw *.map *.out *_linkInfo.xml 2>NUL

.PHONY: all clean

