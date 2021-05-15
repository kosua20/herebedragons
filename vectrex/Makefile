NAME = dragon
TGTNAME = HereBeDragons

TARGET = $(TGTNAME).bin
SOURCE = $(NAME).c

CC = cmoc
CFLAGS = --vectrex -O2

all: $(TARGET)

$(TARGET): $(SOURCE)
	@ echo "CC $<"
	@ $(CC) $(CFLAGS) -o $(TARGET) $<

run: $(TARGET)
	@ vecx $<

clean:
	@ echo $@
	@ rm -f $(NAME).{bin,i,lst,asm,hex}
	@ rm -f $(TGTNAME).{bin,i,lst,asm,hex}
