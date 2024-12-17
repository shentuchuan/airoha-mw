OBJ = $(patsubst %.c, %.o, $(SRC))

CGI_DIR = ../html/cgi

SRC = mqttd.c
SRC += mqttd_queue.c
SRC += hr_cjson.c
all: $(OBJ)
%.o:%.c
ifeq ("$(AIR_LOG)", "")
	$(CC) -c -MMD $(CFLAGS) $(MW_SYS_INC) -I$(CGI_DIR) $< -o $(OUTPUT_DIR)/$@
else
	$(CC) -c -MMD $(CFLAGS) $(MW_SYS_INC) -I$(CGI_DIR) $< -o $(OUTPUT_DIR)/$@ >> $(AIR_LOG) 2>&1
endif
