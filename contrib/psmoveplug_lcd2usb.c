/*
 * lcd2usb.c - test application for the lcd2usb interface
 *             http://www.harbaum.org/till/lcd2usb
 * Modified by Thomas Perl <m@thp.io> for use in psmoveplug
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <usb.h>

/* vendor and product id */
#define LCD2USB_VID  0x0403
#define LCD2USB_PID  0xc630

/* target is a bit map for CMD/DATA */
#define LCD_CTRL_0         (1<<3)
#define LCD_CTRL_1         (1<<4)
#define LCD_BOTH           (LCD_CTRL_0 | LCD_CTRL_1)

#define LCD_CMD            (1<<5)
#define LCD_DATA           (2<<5)

usb_dev_handle      *handle = NULL;

int lcd_send(int request, int value, int index) {
  if(usb_control_msg(handle, USB_TYPE_VENDOR, request, 
		      value, index, NULL, 0, 1000) < 0) {
    fprintf(stderr, "USB request failed!");
    return -1;
  }
  return 0;
}

/* to increase performance, a little buffer is being used to */
/* collect command bytes of the same type before transmitting them */
#define BUFFER_MAX_CMD 4        /* current protocol supports up to 4 bytes */
int buffer_current_type = -1;   /* nothing in buffer yet */
int buffer_current_fill = 0;    /* -"- */
unsigned char buffer[BUFFER_MAX_CMD];

/* command format:
 * 7 6 5 4 3 2 1 0
 * C C C T T R L L
 *
 * TT = target bit map
 * R = reserved for future use, set to 0
 * LL = number of bytes in transfer - 1
 */

/* flush command queue due to buffer overflow / content */
/* change or due to explicit request */
void lcd_flush(void) {
  int request, value, index;
  
  /* anything to flush? ignore request if not */
  if (buffer_current_type == -1)
    return;
  
  /* build request byte */
  request = buffer_current_type | (buffer_current_fill - 1);
  
  /* fill value and index with buffer contents. endianess should IMHO not */
  /* be a problem, since usb_control_msg() will handle this. */
  value = buffer[0] | (buffer[1] << 8);
  index = buffer[2] | (buffer[3] << 8);
  
  /* send current buffer contents */
  lcd_send(request, value, index);
  
  /* buffer is now free again */
  buffer_current_type = -1;
  buffer_current_fill = 0;
}

/* enqueue a command into the buffer */
void lcd_enqueue(int command_type, int value) {
  if ((buffer_current_type >= 0) && (buffer_current_type != command_type))
    lcd_flush();
  
  /* add new item to buffer */
  buffer_current_type = command_type;
  buffer[buffer_current_fill++] = value;
  
  /* flush buffer if it's full */
  if (buffer_current_fill == BUFFER_MAX_CMD)
    lcd_flush();
}

/* see HD44780 datasheet for a command description */
void lcd_command(const unsigned char ctrl, const unsigned char cmd) {
  lcd_enqueue(LCD_CMD | ctrl, cmd);
}

/* clear display */
void lcd_clear(void) {
  lcd_command(LCD_BOTH, 0x01);    /* clear display */
  lcd_command(LCD_BOTH, 0x03);    /* return home */
}

/* write a data string to the first display */
void lcd_write(const char *data) {
  int ctrl = LCD_CTRL_0;
  
  while(*data) 
    lcd_enqueue(LCD_DATA | ctrl, *data++);
  
  lcd_flush();
}

int main(int argc, char *argv[]) {
    struct usb_bus      *bus;
    struct usb_device   *dev;

    usb_init();

    usb_find_busses();
    usb_find_devices();

    for(bus = usb_get_busses(); bus; bus = bus->next) {
        for(dev = bus->devices; dev; dev = dev->next) {
            if((dev->descriptor.idVendor == LCD2USB_VID) && 
                    (dev->descriptor.idProduct == LCD2USB_PID)) {
                handle = usb_open(dev);
                break;
            }
        }
    }

    if(!handle) {
        fprintf(stderr, "Error: Could not find LCD2USB device\n");
        exit(-1);
    }

    lcd_clear();

    int i = 0;
    while (!feof(stdin)) {
        char line[1024];
        fgets(line, sizeof(line), stdin);
        if (feof(stdin)) break;
        line[strlen(line)-1] = '\0';
        if(strlen(line) == 0) {
            lcd_clear();
            lcd_clear();
            i = 0;
            continue;
        }
        lcd_write(line);
        i += strlen(line);
        while (i % 20 != 0) {
            lcd_write(" ");
            i++;
        }
    }

    usb_close(handle);

    return 0;
}

