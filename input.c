#include <stdio.h>
#include <fcntl.h>
#include <linux/input.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <sys/poll.h>
#include <sys/sysinfo.h>
#include "input.h"
#include "user_io.h"
#include "menu.h"
#include "hardware.h"

#define NUMDEV 10

typedef struct
{
	uint16_t vid, pid;
	char     led;
	char     last_l, last_r, last_u, last_d;

	char     has_map;
	uint32_t map[32];

	char     has_mmap;
	uint32_t mmap[32];

	char     has_kbdmap;
	uint8_t  kbdmap[256];
}  devInput;

static devInput input[NUMDEV] = {0};
static int first_joystick = -1;

#define LCTRL   0x0100
#define LSHIFT  0x0200
#define LALT    0x0400
#define LGUI    0x0800
#define RCTRL   0x1000
#define RSHIFT  0x2000
#define RALT    0x4000
#define RGUI    0x8000
#define MODMASK 0xFF00
#define NONE    0

static int ev2usb[] =
{
	NONE, //0   KEY_RESERVED	
	0x29, //1   KEY_ESC			
	0x1e, //2   KEY_1			
	0x1f, //3   KEY_2			
	0x20, //4   KEY_3			
	0x21, //5   KEY_4			
	0x22, //6   KEY_5			
	0x23, //7   KEY_6			
	0x24, //8   KEY_7			
	0x25, //9   KEY_8			
	0x26, //10  KEY_9			
	0x27, //11  KEY_0			
	0x2D, //12  KEY_MINUS		
	0x2E, //13  KEY_EQUAL		
	0x2A, //14  KEY_BACKSPACE	
	0x2B, //15  KEY_TAB			
	0x14, //16  KEY_Q			
	0x1a, //17  KEY_W			
	0x08, //18  KEY_E			
	0x15, //19  KEY_R			
	0x17, //20  KEY_T			
	0x1c, //21  KEY_Y			
	0x18, //22  KEY_U			
	0x0c, //23  KEY_I			
	0x12, //24  KEY_O			
	0x13, //25  KEY_P			
	0x2F, //26  KEY_LEFTBRACE	
	0x30, //27  KEY_RIGHTBRACE	
	0x28, //28  KEY_ENTER		
	LCTRL, //29  KEY_LEFTCTRL	
	0x04, //30  KEY_A			
	0x16, //31  KEY_S			
	0x07, //32  KEY_D			
	0x09, //33  KEY_F			
	0x0a, //34  KEY_G			
	0x0b, //35  KEY_H			
	0x0d, //36  KEY_J			
	0x0e, //37  KEY_K			
	0x0f, //38  KEY_L			
	0x33, //39  KEY_SEMICOLON	
	0x34, //40  KEY_APOSTROPHE	
	0x35, //41  KEY_GRAVE		
	LSHIFT, //42  KEY_LEFTSHIFT	
	0x31, //43  KEY_BACKSLASH	
	0x1d, //44  KEY_Z			
	0x1b, //45  KEY_X			
	0x06, //46  KEY_C			
	0x19, //47  KEY_V			
	0x05, //48  KEY_B			
	0x11, //49  KEY_N			
	0x10, //50  KEY_M			
	0x36, //51  KEY_COMMA		
	0x37, //52  KEY_DOT			
	0x38, //53  KEY_SLASH		
	RSHIFT, //54  KEY_RIGHTSHIFT	
	0x55, //55  KEY_KPASTERISK	
	LALT, //56  KEY_LEFTALT		
	0x2C, //57  KEY_SPACE		
	0x39, //58  KEY_CAPSLOCK	
	0x3a, //59  KEY_F1			
	0x3b, //60  KEY_F2			
	0x3c, //61  KEY_F3			
	0x3d, //62  KEY_F4			
	0x3e, //63  KEY_F5			
	0x3f, //64  KEY_F6			
	0x40, //65  KEY_F7			
	0x41, //66  KEY_F8			
	0x42, //67  KEY_F9			
	0x43, //68  KEY_F10			
	0x53, //69  KEY_NUMLOCK		
	0x47, //70  KEY_SCROLLLOCK	
	0x5F, //71  KEY_KP7			
	0x60, //72  KEY_KP8			
	0x61, //73  KEY_KP9			
	0x56, //74  KEY_KPMINUS		
	0x5C, //75  KEY_KP4			
	0x5D, //76  KEY_KP5			
	0x5E, //77  KEY_KP6			
	0x57, //78  KEY_KPPLUS		
	0x59, //79  KEY_KP1			
	0x5A, //80  KEY_KP2			
	0x5B, //81  KEY_KP3			
	0x62, //82  KEY_KP0			
	0x63, //83  KEY_KPDOT		
	NONE, //84  ???				
	NONE, //85  KEY_ZENKAKU		
	NONE, //86  KEY_102ND		
	0x44, //87  KEY_F11			
	0x45, //88  KEY_F12			
	NONE, //89  KEY_RO			
	NONE, //90  KEY_KATAKANA	
	NONE, //91  KEY_HIRAGANA	
	NONE, //92  KEY_HENKAN		
	NONE, //93  KEY_KATAKANA	
	NONE, //94  KEY_MUHENKAN	
	NONE, //95  KEY_KPJPCOMMA	
	0x28, //96  KEY_KPENTER		
	RCTRL, //97  KEY_RIGHTCTRL	
	0x54, //98  KEY_KPSLASH		
	NONE, //99  KEY_SYSRQ		
	RALT, //100 KEY_RIGHTALT	
	NONE, //101 KEY_LINEFEED	
	0x4A, //102 KEY_HOME		
	0x52, //103 KEY_UP			
	0x4B, //104 KEY_PAGEUP		
	0x50, //105 KEY_LEFT		
	0x4F, //106 KEY_RIGHT		
	0x4D, //107 KEY_END			
	0x51, //108 KEY_DOWN		
	0x4E, //109 KEY_PAGEDOWN	
	0x49, //110 KEY_INSERT		
	0x4C, //111 KEY_DELETE		
	NONE, //112 KEY_MACRO		
	NONE, //113 KEY_MUTE		
	NONE, //114 KEY_VOLUMEDOWN	
	NONE, //115 KEY_VOLUMEUP	
	NONE, //116 KEY_POWER		
	0x67, //117 KEY_KPEQUAL		
	NONE, //118 KEY_KPPLUSMINUS	
	0x48, //119 KEY_PAUSE		
	NONE, //120 KEY_SCALE		
	NONE, //121 KEY_KPCOMMA		
	NONE, //122 KEY_HANGEUL		
	NONE, //123 KEY_HANJA		
	NONE, //124 KEY_YEN			
	LGUI, //125 KEY_LEFTMETA	
	RGUI, //126 KEY_RIGHTMETA	
	0x65, //127 KEY_COMPOSE		
	NONE, //128 KEY_STOP		
	NONE, //129 KEY_AGAIN		
	NONE, //130 KEY_PROPS		
	NONE, //131 KEY_UNDO		
	NONE, //132 KEY_FRONT		
	NONE, //133 KEY_COPY		
	NONE, //134 KEY_OPEN		
	NONE, //135 KEY_PASTE		
	NONE, //136 KEY_FIND		
	NONE, //137 KEY_CUT			
	NONE, //138 KEY_HELP		
	NONE, //139 KEY_MENU		
	NONE, //140 KEY_CALC		
	NONE, //141 KEY_SETUP		
	NONE, //142 KEY_SLEEP		
	NONE, //143 KEY_WAKEUP		
	NONE, //144 KEY_FILE		
	NONE, //145 KEY_SENDFILE	
	NONE, //146 KEY_DELETEFILE	
	NONE, //147 KEY_XFER		
	NONE, //148 KEY_PROG1		
	NONE, //149 KEY_PROG2		
	NONE, //150 KEY_WWW			
	NONE, //151 KEY_MSDOS		
	NONE, //152 KEY_SCREENLOCK	
	NONE, //153 KEY_DIRECTION	
	NONE, //154 KEY_CYCLEWINDOWS
	NONE, //155 KEY_MAIL		
	NONE, //156 KEY_BOOKMARKS	
	NONE, //157 KEY_COMPUTER	
	NONE, //158 KEY_BACK		
	NONE, //159 KEY_FORWARD		
	NONE, //160 KEY_CLOSECD		
	NONE, //161 KEY_EJECTCD		
	NONE, //162 KEY_EJECTCLOSECD
	NONE, //163 KEY_NEXTSONG	
	NONE, //164 KEY_PLAYPAUSE	
	NONE, //165 KEY_PREVIOUSSONG
	NONE, //166 KEY_STOPCD		
	NONE, //167 KEY_RECORD		
	NONE, //168 KEY_REWIND		
	NONE, //169 KEY_PHONE		
	NONE, //170 KEY_ISO			
	NONE, //171 KEY_CONFIG		
	NONE, //172 KEY_HOMEPAGE	
	NONE, //173 KEY_REFRESH		
	NONE, //174 KEY_EXIT		
	NONE, //175 KEY_MOVE		
	NONE, //176 KEY_EDIT		
	NONE, //177 KEY_SCROLLUP	
	NONE, //178 KEY_SCROLLDOWN	
	NONE, //179 KEY_KPLEFTPAREN	
	NONE, //180 KEY_KPRIGHTPAREN
	NONE, //181 KEY_NEW			
	NONE, //182 KEY_REDO		
	NONE, //183 KEY_F13			
	NONE, //184 KEY_F14			
	NONE, //185 KEY_F15			
	NONE, //186 KEY_F16			
	NONE, //187 KEY_F17			
	NONE, //188 KEY_F18			
	NONE, //189 KEY_F19			
	NONE, //190 KEY_F20			
	NONE, //191 KEY_F21			
	NONE, //192 KEY_F22			
	NONE, //193 KEY_F23			
	NONE, //194 KEY_F24			
	NONE, //195 ???				
	NONE, //196 ???				
	NONE, //197 ???				
	NONE, //198 ???				
	NONE, //199 ???				
	NONE, //200 KEY_PLAYCD		
	NONE, //201 KEY_PAUSECD		
	NONE, //202 KEY_PROG3		
	NONE, //203 KEY_PROG4		
	NONE, //204 KEY_DASHBOARD	
	NONE, //205 KEY_SUSPEND		
	NONE, //206 KEY_CLOSE		
	NONE, //207 KEY_PLAY		
	NONE, //208 KEY_FASTFORWARD	
	NONE, //209 KEY_BASSBOOST	
	0x46, //210 KEY_PRINT		
	NONE, //211 KEY_HP			
	NONE, //212 KEY_CAMERA		
	NONE, //213 KEY_SOUND		
	NONE, //214 KEY_QUESTION	
	NONE, //215 KEY_EMAIL		
	NONE, //216 KEY_CHAT		
	NONE, //217 KEY_SEARCH		
	NONE, //218 KEY_CONNECT		
	NONE, //219 KEY_FINANCE		
	NONE, //220 KEY_SPORT		
	NONE, //221 KEY_SHOP		
	NONE, //222 KEY_ALTERASE	
	NONE, //223 KEY_CANCEL		
	NONE, //224 KEY_BRIGHT_DOWN	
	NONE, //225 KEY_BRIGHT_UP	
	NONE, //226 KEY_MEDIA		
	NONE, //227 KEY_SWITCHVIDEO	
	NONE, //228 KEY_DILLUMTOGGLE
	NONE, //229 KEY_DILLUMDOWN	
	NONE, //230 KEY_DILLUMUP	
	NONE, //231 KEY_SEND		
	NONE, //232 KEY_REPLY		
	NONE, //233 KEY_FORWARDMAIL	
	NONE, //234 KEY_SAVE		
	NONE, //235 KEY_DOCUMENTS	
	NONE, //236 KEY_BATTERY		
	NONE, //237 KEY_BLUETOOTH	
	NONE, //238 KEY_WLAN		
	NONE, //239 KEY_UWB			
	NONE, //240 KEY_UNKNOWN		
	NONE, //241 KEY_VIDEO_NEXT	
	NONE, //242 KEY_VIDEO_PREV	
	NONE, //243 KEY_BRIGHT_CYCLE
	NONE, //244 KEY_BRIGHT_AUTO	
	NONE, //245 KEY_DISPLAY_OFF	
	NONE, //246 KEY_WWAN		
	NONE, //247 KEY_RFKILL		
	NONE, //248 KEY_MICMUTE		
	NONE, //249 ???				
	NONE, //250 ???				
	NONE, //251 ???				
	NONE, //252 ???				
	NONE, //253 ???				
	NONE, //254 ???				
	NONE  //255 ???				
};

int mfd = -1;
int mwd = -1;

static int set_watch()
{
	mwd = -1;
	mfd = inotify_init();
	if (mfd < 0)
	{
		printf("ERR: inotify_init");
		return -1;
	}

	mwd = inotify_add_watch(mfd, "/dev/input",
		IN_MODIFY | IN_CREATE | IN_DELETE);

	if (mwd < 0)
	{
		printf("ERR: inotify_add_watch");
		return -1;
	}

	return mfd;
}

#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )

static int check_devs()
{
	int result = 0;
	int length, i = 0;
	char buffer[BUF_LEN];
	length = read(mfd, buffer, BUF_LEN);

	if (length < 0)
	{
		printf("ERR: read\n");
		return 0;
	}

	while (i<length)
	{
		struct inotify_event *event = (struct inotify_event *) &buffer[i];
		if (event->len)
		{
			if (event->mask & IN_CREATE)
			{
				result = 1;
				if (event->mask & IN_ISDIR)
				{
					printf("The directory %s was created.\n", event->name);
				}
				else
				{
					printf("The file %s was created.\n", event->name);
				}
			}
			else if (event->mask & IN_DELETE)
			{
				result = 1;
				if (event->mask & IN_ISDIR)
				{
					printf("The directory %s was deleted.\n", event->name);
				}
				else
				{
					printf("The file %s was deleted.\n", event->name);
				}
			}
			/*
			else if ( event->mask & IN_MODIFY )
			{
				result = 1;
				if ( event->mask & IN_ISDIR )
				{
					printf( "The directory %s was modified.\n", event->name );
				}
				else
				{
					printf( "The file %s was modified.\n", event->name );
				}
			}
			*/
		}
		i += EVENT_SIZE + event->len;
	}

	return result;
}

static void INThandler()
{
	printf("\nExiting...\n");

	if (mwd >= 0) inotify_rm_watch(mfd, mwd);
	if (mfd >= 0) close(mfd);

	exit(0);
}

#define test_bit(bit, array)  (array [bit / 8] & (1 << (bit % 8)))

static char has_led(int fd)
{
	unsigned char evtype_b[(EV_MAX + 7) / 8];
	if (fd<0) return 0;

	memset(&evtype_b, 0, sizeof(evtype_b));
	if (ioctl(fd, EVIOCGBIT(0, sizeof(evtype_b)), evtype_b) < 0)
	{
		printf("ERR: evdev ioctl.\n");
		return 0;
	}

	if (test_bit(EV_LED, evtype_b))
	{
		printf("has LEDs.\n");
		return 1;
	}

	return 0;
}

static char leds_state = 0;
void set_kbdled(int mask, int state)
{
	leds_state = state ? leds_state | (mask&HID_LED_MASK) : leds_state & ~(mask&HID_LED_MASK);
}

int get_kbdled(int mask)
{
	return (leds_state & (mask&HID_LED_MASK)) ? 1 : 0;
}

int toggle_kbdled(int mask)
{
	int state = !get_kbdled(mask);
	set_kbdled(mask, state);
	return state;
}

static int mapping = 0;
static int mapping_button;
static int mapping_dev;
static int mapping_type;
static int mapping_count;
static uint8_t mapping_key;

void start_map_setting(int cnt)
{
	mapping_button = 0;
	mapping = 1;
	mapping_dev = -1;
	mapping_type = cnt ? 1 : 2;
	mapping_count = cnt;
}

int get_map_button()
{
	return (mapping_type == 2) ? mapping_key : mapping_button;
}

int get_map_type()
{
	return mapping_type;
}

static char *get_map_name(int dev, int def)
{
	static char name[128];
	if (def || is_menu_core()) sprintf(name, "input_%04x_%04x.map", input[dev].vid, input[dev].pid);
	else sprintf(name, "%s_input_%04x_%04x.map", user_io_get_core_name_ex(), input[dev].vid, input[dev].pid);
	return name;
}

static char *get_kbdmap_name(int dev)
{
	static char name[128];
	sprintf(name, "kbd_%04x_%04x.map", input[dev].vid, input[dev].pid);
	return name;
}

void finish_map_setting(int dismiss)
{
	mapping = 0;
	if (mapping_dev<0) return;
	
	if (mapping_type == 2)
	{
		if (dismiss) input[mapping_dev].has_kbdmap = 0;
		else FileSaveConfig(get_kbdmap_name(mapping_dev), &input[mapping_dev].kbdmap, sizeof(input[mapping_dev].kbdmap));
	}
	else
	{
		if (dismiss) input[mapping_dev].has_map = 0;
		else FileSaveConfig(get_map_name(mapping_dev, 0), &input[mapping_dev].map, sizeof(input[mapping_dev].map));
		if(is_menu_core()) input[mapping_dev].has_mmap = 0;
	}
}

uint16_t get_map_vid()
{
	return input[mapping_dev].vid;
}

uint16_t get_map_pid()
{
	return input[mapping_dev].pid;
}

#define KEY_EMU_LEFT  (KEY_MAX+1)
#define KEY_EMU_RIGHT (KEY_MAX+2)
#define KEY_EMU_UP    (KEY_MAX+3)
#define KEY_EMU_DOWN  (KEY_MAX+4)

static void input_cb(struct input_event *ev, int dev);

static int kbd_toggle = 0;
static uint16_t joy[2] = { 0 }, joy_prev[2] = { 0 };
static uint16_t autofire[2] = { 0 };
static uint32_t af_delay[2] = { 50, 50 };

static unsigned char mouse_btn = 0;
static int mouse_emu = 0;
static int mouse_emu_x = 0;
static int mouse_emu_y = 0;

static uint32_t mouse_timer = 0;

static void joy_digital(int num, uint16_t mask, char press, int bnum)
{
	if (num < 2)
	{
		if (!user_io_osd_is_visible() && ((bnum == 17) || (bnum == 16)) && joy[num])
		{
			uint16_t amask = joy[num] & 0xFFF0;
			if(press)
			{
				if (amask)
				{
					if (autofire[num] & amask) autofire[num] &= ~amask;
					else autofire[num] |= amask;

					InfoMessage((autofire[num] & amask) ? "\n\n          Auto fire\n             ON" :
						"\n\n          Auto fire\n             OFF");
				}
				else
				{
					if (joy[num] & 1) af_delay[num] = 50;
					else if (joy[num] & 2) af_delay[num] = 100;
					else if (joy[num] & 4) af_delay[num] = 200;
					else af_delay[num] = 500;
					static char str[256];
					sprintf(str, "\n\n       Auto fire period\n            %dms", af_delay[num] * 2);
					InfoMessage(str);
				}
			}
			return;
		}

		if (bnum == 16)
		{
			if(press) kbd_toggle = !kbd_toggle;
			return;
		}

		if (!user_io_osd_is_visible() && (bnum == 17) && (mouse_emu & 1))
		{
			if (press)
			{
				mouse_emu ^= 2;
				InfoMessage((mouse_emu & 2) ? "\n\n       Mouse mode lock\n             ON" :
					"\n\n       Mouse mode lock\n             OFF");
			}
			return;
		}

		if (user_io_osd_is_visible() || (bnum == 17))
		{
			memset(joy, 0, sizeof(joy));
			struct input_event ev;
			ev.type = EV_KEY;
			ev.value = press;
			switch (mask)
			{
			case JOY_RIGHT:
				ev.code = KEY_RIGHT;
				break;

			case JOY_LEFT:
				ev.code = KEY_LEFT;
				break;

			case JOY_UP:
				ev.code = KEY_UP;
				break;

			case JOY_DOWN:
				ev.code = KEY_DOWN;
				break;

			case JOY_BTN1:
				ev.code = KEY_ENTER;
				break;

			case JOY_BTN2:
				ev.code = KEY_ESC;
				break;

			case JOY_BTN3:
				ev.code = KEY_BACKSPACE;
				break;

			default:
				ev.code = (bnum == 17) ? KEY_F12 : 0;
			}

			input_cb(&ev, 0);
		}
		else
		{
			if (press) joy[num] |= mask;
			else joy[num] &= ~mask;
			//user_io_digital_joystick(num, joy[num]);
		}
	}
}

static void joy_analog(int num, int axis, int offset)
{
	static int pos[2][2] = { 0 };

	if (num < 2)
	{
		pos[num][axis] = offset;
		user_io_analog_joystick(num, (char)(pos[num][0]), (char)(pos[num][1]));
	}
}

static void input_cb(struct input_event *ev, int dev)
{
	static int key_mapped = 0;
	static uint8_t modifiers = 0;
	static char keys[6] = { 0,0,0,0,0,0 };

	// repeat events won't be processed
	if (ev->type == EV_KEY && ev->value > 1) return;

	int map_skip = (ev->type == EV_KEY && ev->code == 57 && mapping_dev >= 0 && mapping_type==1);
	int cancel   = (ev->type == EV_KEY && ev->code == 1);
	int enter    = (ev->type == EV_KEY && ev->code == 28);

	//mouse
	switch (ev->type)
	{
	case EV_KEY:
		if (ev->code >= 272 && ev->code <= 279)
		{
			unsigned char mask = 1 << (ev->code - 272);
			mouse_btn = (ev->value) ? mouse_btn | mask : mouse_btn & ~mask;
			user_io_mouse(mouse_btn, 0, 0);
			return;
		}
		break;

	case EV_REL:
		{
			switch (ev->code)
			{
			case 0:
				//printf("Mouse PosX: %d\n", ev->value);
				user_io_mouse(mouse_btn, ev->value, 0);
				return;
			case 1:
				//printf("Mouse PosY: %d\n", ev->value);
				user_io_mouse(mouse_btn, 0, ev->value);
				return;
			}
		}
		break;
	}

	if (!input[dev].has_map)
	{
		if (!FileLoadConfig(get_map_name(dev, 0), &input[dev].map, sizeof(input[dev].map)))
		{
			if (is_menu_core() || !FileLoadConfig(get_map_name(dev, 1), &input[dev].map, sizeof(input[dev].map)))
			{
				memset(input[dev].map, 0, sizeof(input[dev].map));
			}

			//remove alt controls from default map
			input[dev].map[8]  = 0;
			input[dev].map[9]  = 0;
			input[dev].map[10] = 0;
			input[dev].map[11] = 0;
			input[dev].map[12] = 0;
			input[dev].map[13] = 0;
			input[dev].map[14] = 0;
			input[dev].map[15] = 0;
		}
		input[dev].has_map = 1;
	}

	if (!input[dev].has_mmap)
	{
		if (!FileLoadConfig(get_map_name(dev, 1), &input[dev].mmap, sizeof(input[dev].mmap)))
		{
			memset(input[dev].mmap, 0, sizeof(input[dev].mmap));
		}
		input[dev].has_mmap = 1;
	}

	//mapping
	if (mapping && (mapping_dev >=0 || ev->value) && !cancel && !enter)
	{
		if (ev->type == EV_KEY)
		{
			if (mapping_type == 2)
			{
				if (ev->value && ev->code < 256)
				{
					if(mapping_dev < 0)
					{
						mapping_dev = dev;
						mapping_key = 0;
					}

					if (!mapping_key) mapping_key = ev->code;
					else
					{
						input[dev].kbdmap[mapping_key] = ev->code;
						mapping_key = 0;
					}
				}
				return;
			}
			else if ((mapping_dev < 0) || ((ev->code >= 256) ? mapping_type : !mapping_type))
			{
				if (mapping_dev < 0)
				{
					mapping_dev = dev;
					mapping_type = (ev->code >= 256) ? 1 : 0;
				}

				if (mapping_dev == dev && mapping_button < mapping_count)
				{
					if (ev->value)
					{
						if (!mapping_button) memset(input[dev].map, 0, sizeof(input[dev].map));

						int found = 0;
						for (int i = (is_menu_core() && mapping_button >= 8) ? 8 : 0; i < mapping_button; i++) if (input[dev].map[i] == ev->code) found = 1;

						if (!found)
						{
							input[dev].map[(mapping_button == (mapping_count - 1)) ? 16 + mapping_type : mapping_button] = ev->code;
							key_mapped = 1;
						}
					}
					else
					{
						if (key_mapped) mapping_button++;
						key_mapped = 0;
					}
				}

				return;
			}
		}

		if (map_skip && mapping_button < mapping_count)
		{
			if (ev->value == 1) mapping_button++;
			return;
		}
	}
	else
	{
		key_mapped = 0;
		switch (ev->type)
		{
		case EV_KEY:
			//joystick buttons, digital directions
			if (ev->code >= 256)
			{
				if (first_joystick < 0) first_joystick = dev;

				if (mouse_emu && !user_io_osd_is_visible())
				{
					for (int i = 8; i <= 14; i++)
					{
						if (ev->code == input[dev].mmap[i])
						{
							int val = (mouse_emu == 3) ? 2 : 10;
							switch (i)
							{
							case 8:
								mouse_emu_x = ev->value ?  val : 0;
								break;
							case 9:
								mouse_emu_x = ev->value ? -val : 0;
								break;
							case 10:
								mouse_emu_y = ev->value ?  val : 0;
								break;
							case 11:
								mouse_emu_y = ev->value ? -val : 0;
								break;

							default:
								mouse_btn = ev->value ? mouse_btn | 1<<(i-12) : mouse_btn & ~(1<<(i-12));
								user_io_mouse(mouse_btn, 0, 0);
								break;
							}
							return;
						}
					}

					for (int i = 0; i <= (is_menu_core() ? 7 : 15); i++)
					{
						if (ev->code == input[dev].map[i])
						{
							joy_digital((first_joystick == dev) ? 0 : 1, 1 << i, ev->value, i);
							return;
						}
					}
				}
				else
				{
					for (int i = 0; i <= (is_menu_core() ? 7 : 15); i++)
					{
						if (ev->code == input[dev].map[i])
						{
							joy_digital((first_joystick == dev) ? 0 : 1, 1 << i, ev->value, i);
							return;
						}
					}

					for (int i = 8; i <= 11; i++)
					{
						if (ev->code == input[dev].mmap[i])
						{
							joy_digital((first_joystick == dev) ? 0 : 1, 1 << (i - 8), ev->value, i - 8);
							return;
						}
					}
				}

				if (ev->code == input[dev].map[17])
				{
					joy_digital((first_joystick == dev) ? 0 : 1, 0, ev->value, 17);
					return;
				}


				if (ev->code == input[dev].mmap[15])
				{
					mouse_emu = ev->value ? mouse_emu | 1 : mouse_emu & ~1;
					printf("mouse_emu = %d\n", mouse_emu);

					mouse_timer = 0;
					mouse_btn   = 0;
					mouse_emu_x = 0;
					mouse_emu_y = 0;
					user_io_mouse(0, 0, 0);
				}
				return;
			}
			// keyboard
			else
			{
				if(!user_io_osd_is_visible() && ((user_io_get_kbdemu() == EMU_JOY0) || (user_io_get_kbdemu() == EMU_JOY1)))
				{
					if (!kbd_toggle)
					{
						for (int i = 0; i <= 15; i++)
						{
							if (ev->code == input[dev].map[i])
							{
								joy_digital((user_io_get_kbdemu() == EMU_JOY0) ? 0 : 1, 1 << i, ev->value, i);
								return;
							}
						}
					}

					if (ev->code == input[dev].map[16])
					{
						joy_digital((user_io_get_kbdemu() == EMU_JOY0) ? 0 : 1, 0, ev->value, 16);
						return;
					}
				}
				else
				{
					kbd_toggle = 0;
				}


				if (!input[dev].has_kbdmap)
				{
					if (!FileLoadConfig(get_kbdmap_name(dev), &input[dev].kbdmap, sizeof(input[dev].kbdmap)))
					{
						memset(input[dev].kbdmap, 0, sizeof(input[dev].kbdmap));
					}
					input[dev].has_kbdmap = 1;
				}

				int key = input[dev].kbdmap[ev->code] ? input[dev].kbdmap[ev->code] : ev->code;
				key = (key < (sizeof(ev2usb) / sizeof(ev2usb[0]))) ? ev2usb[key] : NONE;
				if(key != NONE)
				{
					if (key & MODMASK)
					{
						modifiers = (ev->value) ? modifiers | (uint8_t)(key >> 8) : modifiers & ~(uint8_t)(key >> 8);
					}
					else
					{
						if (ev->value)
						{
							int found = 0;
							for (int i = 0; i < (sizeof(keys) / sizeof(keys[0])); i++) if (keys[i] == (uint8_t)key) found = 1;

							if (!found)
							{
								for (int i = 0; i < (sizeof(keys) / sizeof(keys[0])); i++)
								{
									if (!keys[i])
									{
										keys[i] = (uint8_t)key;
										break;
									}
								}
							}
						}
						else
						{
							for (int i = 0; i < (sizeof(keys) / sizeof(keys[0])); i++) if (keys[i] == (uint8_t)key) keys[i] = 0;
						}

						int j = 0;
						for (int i = 0; i < (sizeof(keys) / sizeof(keys[0])); i++) if (keys[i]) keys[j++] = keys[i];
						while (j < (sizeof(keys) / sizeof(keys[0]))) keys[j++] = 0;
					}

					user_io_kbd(modifiers, keys, input[dev].vid, input[dev].pid);
					return;
				}
			}
			break;

		//analog joystick
		case EV_ABS:
			if (!user_io_osd_is_visible())
			{
				if (mouse_emu)
				{
					if (ev->code == 0) // x
					{
						mouse_emu_x = 0;
						if (ev->value < 127 || ev->value>129) mouse_emu_x = ev->value - 128;
						mouse_emu_x /= 12;
						if (mouse_emu == 3 && mouse_emu_x > 0) mouse_emu_x = 2;
						if (mouse_emu == 3 && mouse_emu_x < 0) mouse_emu_x = -2;
						/*
						mouse_emu_x = 0;
						if (ev->value < 127) mouse_emu_x = -1;
							else if(ev->value > 129) mouse_emu_x = 1;
							*/
						return;
					}

					if (ev->code == 1) // y
					{
						mouse_emu_y = 0;
						if (ev->value < 127 || ev->value>129) mouse_emu_y = ev->value - 128;
						mouse_emu_y /= 12;
						if (mouse_emu == 3 && mouse_emu_y > 0) mouse_emu_y = 2;
						if (mouse_emu == 3 && mouse_emu_y < 0) mouse_emu_y = -2;
						/*
						mouse_emu_y = 0;
						if (ev->value < 127) mouse_emu_y = -1;
							else if (ev->value > 129) mouse_emu_y = 1;
							*/
						return;
					}
				}
				else
				{
					// skip if first joystick is not defined.
					if (first_joystick < 0) break;

					// TODO:
					// 1) add analog axis mapping
					// 2) enable invertion

					if (ev->code == 0) // x
					{
						int offset = 0;
						if (ev->value < 127 || ev->value>129) offset = ev->value - 128;
						joy_analog((first_joystick == dev) ? 0 : 1, 0, offset);
						return;
					}

					if (ev->code == 1) // y
					{
						int offset = 0;
						if (ev->value < 127 || ev->value>129) offset = ev->value - 128;
						joy_analog((first_joystick == dev) ? 0 : 1, 1, offset);
						return;
					}
				}
			}
			break;
		}
	}
}

static uint16_t read_hex(char *filename)
{
	FILE *in;
	unsigned int value;

	in = fopen(filename, "rb");
	if (!in) return 0;

	if (fscanf(in, "%x", &value) == 1)
	{
		fclose(in);
		return (uint16_t)value;
	}
	fclose(in);
	return 0;
}

static void getVidPid(int num, uint16_t* vid, uint16_t* pid)
{
	char name[256];
	sprintf(name, "/sys/class/input/event%d/device/id/vendor", num);
	*vid = read_hex(name);
	sprintf(name, "/sys/class/input/event%d/device/id/product", num);
	*pid = read_hex(name);
}

static struct pollfd pool[NUMDEV + 1];

int input_test(int getchar)
{
	static char   cur_leds = 0;
	static int    state = 0;

	char devname[20];
	struct input_event ev;

	if (state == 0)
	{
		signal(SIGINT, INThandler);
		pool[NUMDEV].fd = set_watch();
		pool[NUMDEV].events = POLLIN;
		state++;
	}

	if (state == 1)
	{
		printf("Open up to %d input devices.\n", NUMDEV);
		for (int i = 0; i<NUMDEV; i++)
		{
			sprintf(devname, "/dev/input/event%d", i);
			pool[i].fd = open(devname, O_RDWR);
			pool[i].events = POLLIN;
			memset(&input[i], 0, sizeof(input[i]));
			input[i].led = has_led(pool[i].fd);
			if (pool[i].fd > 0) getVidPid(i, &input[i].vid, &input[i].pid);
			if (pool[i].fd > 0) printf("opened %s (%04x:%04x)\n", devname, input[i].vid, input[i].pid);
		}

		cur_leds |= 0x80;
		state++;
	}

	if (state == 2)
	{
		int return_value = poll(pool, NUMDEV + 1, 0);
		if (return_value < 0)
		{
			printf("ERR: poll\n");
		}
		else if (return_value > 0)
		{
			if ((pool[NUMDEV].revents & POLLIN) && check_devs())
			{
				printf("Close all devices.\n");
				for (int i = 0; i<NUMDEV; i++)
				{
					if (pool[i].fd >= 0) close(pool[i].fd);
				}
				state = 1;
				return 0;
			}

			for (int i = 0; i<NUMDEV; i++)
			{
				if ((pool[i].fd >= 0) && (pool[i].revents & POLLIN))
				{
					memset(&ev, 0, sizeof(ev));
					if (read(pool[i].fd, &ev, sizeof(ev)) == sizeof(ev))
					{
						if (getchar)
						{
							if (ev.type == EV_KEY && ev.value >= 1)
							{
								return ev.code;
							}
						}
						else
						{
							if (is_menu_core())
							{
								switch (ev.type)
								{
									//keyboard, buttons
								case EV_KEY:
									printf("Input event: type=EV_KEY, code=%d(%x), value=%d\n", ev.code, ev.code, ev.value);
									break;

									//mouse
								case EV_REL:
									printf("Input event: type=EV_REL, Axis=%d, Offset:=%d\n", ev.code, ev.value);
									break;

								case EV_SYN:
								case EV_MSC:
									break;

									//analog joystick
								case EV_ABS:
									if (ev.code == 62) break;
									if (ev.code == 61) break; //ps3 accel axis
									if (ev.code == 60) break; //ps3 accel axis
									if (ev.code == 59) break; //ps3 accel axis

									//reduce flood from PS3 gamepad
									if (input[i].vid == 0x054c && input[i].pid == 0x0268)
									{
										if (ev.code <= 5 && ev.value > 118 && ev.value < 138) break;
									}

									printf("Input event: type=EV_ABS, Axis=%d, Offset:=%d\n", ev.code, ev.value);
									break;

								default:
									printf("Input event: type=%d, code=%d(%x), value=%d(%x)\n", ev.type, ev.code, ev.code, ev.value, ev.value);
								}
							}

							input_cb(&ev, i);

							//sumulate digital directions from analog
							if (ev.type == EV_ABS && !(ev.code<=1 && mouse_emu && !user_io_osd_is_visible()))
							{
								// some pads use axis 16 for L/R PAD, axis 17 for U/D PAD
								// emulate PAD on axis 0/1

								char l, r, u, d;
								l = r = u = d = 0;
								uint16_t offset = 0;
								if (ev.code < 16) offset += 4;
								if (ev.code < 2)  offset += 4;

								if(ev.code == 0 || ev.code == 2 || ev.code == 16) // x
								{
									if ((ev.code < 16) ? ev.value < 64  : ev.value == -1) l = 1;
									if ((ev.code < 16) ? ev.value > 192 : ev.value ==  1) r = 1;

									ev.type = EV_KEY;
									if (input[i].last_l != l)
									{
										ev.code = KEY_EMU_LEFT + offset;
										ev.value = l;
										input_cb(&ev, i);
										input[i].last_l = l;
									}

									if (input[i].last_r != r)
									{
										ev.code = KEY_EMU_RIGHT + offset;
										ev.value = r;
										input_cb(&ev, i);
										input[i].last_r = r;
									}
								}

								if (ev.code == 1 || ev.code == 5 || ev.code == 17) // y
								{
									if ((ev.code < 16) ? ev.value < 64  : ev.value == -1) u = 1;
									if ((ev.code < 16) ? ev.value > 192 : ev.value ==  1) d = 1;

									ev.type = EV_KEY;
									if (input[i].last_u != u)
									{
										ev.code = KEY_EMU_UP + offset;
										ev.value = u;
										input_cb(&ev, i);
										input[i].last_u = u;
									}

									if (input[i].last_d != d)
									{
										ev.code = KEY_EMU_DOWN + offset;
										ev.value = d;
										input_cb(&ev, i);
										input[i].last_d = d;
									}
								}
							}
						}
					}
				}
			}
		}

		if (cur_leds != leds_state)
		{
			cur_leds = leds_state;
			for (int i = 0; i<NUMDEV; i++)
			{
				if (input[i].led)
				{
					ev.type = EV_LED;

					ev.code = LED_SCROLLL;
					ev.value = (cur_leds&HID_LED_SCROLL_LOCK) ? 1 : 0;
					write(pool[i].fd, &ev, sizeof(struct input_event));

					ev.code = LED_NUML;
					ev.value = (cur_leds&HID_LED_NUM_LOCK) ? 1 : 0;
					write(pool[i].fd, &ev, sizeof(struct input_event));

					ev.code = LED_CAPSL;
					ev.value = (cur_leds&HID_LED_CAPS_LOCK) ? 1 : 0;
					write(pool[i].fd, &ev, sizeof(struct input_event));
				}
			}
		}
	}

	return 0;
}

int input_poll(int getchar)
{
	static int af[2] = { 0 };
	static uint32_t time[2] = { 0 };

	int ret = input_test(getchar);
	if (getchar) return ret;

	static int prev_dx = 0;
	static int prev_dy = 0;
	/*
	static int cx = 0;
	static int cy = 0;
	*/

	if (mouse_emu)
	{
		/*
		if (!mouse_emu_x || (prev_dx > 0 && mouse_emu_x < 0) || (prev_dx < 0 && mouse_emu_x>0)) cx = 0;
		if (!mouse_emu_y || (prev_dx > 0 && mouse_emu_y < 0) || (prev_dx < 0 && mouse_emu_y>0)) cy = 0;
		*/

		if((prev_dx || mouse_emu_x || prev_dy || mouse_emu_y) && (!mouse_timer || CheckTimer(mouse_timer)))
		{
			mouse_timer = GetTimer(20);
			user_io_mouse(mouse_btn, mouse_emu_x, mouse_emu_y);
			/*
			if (mouse_emu_x)
			{
				cx++;
				if (cx && !(cx % 10))
				{
					if (mouse_emu_x < 0)
					{
						mouse_emu_x -= 2;
						if (mouse_emu_x < -32) mouse_emu_x = -32;
					}

					if (mouse_emu_x > 0)
					{
						mouse_emu_x += 2;
						if (mouse_emu_x > 32)  mouse_emu_x = 32;
					}
				}
			}

			if (mouse_emu_y)
			{
				cy++;
				if (cy && !(cy % 10))
				{
					if (mouse_emu_y < 0)
					{
						mouse_emu_y -= 2;
						if (mouse_emu_y < -32) mouse_emu_y = -32;
					}

					if (mouse_emu_y > 0)
					{
						mouse_emu_y += 2;
						if (mouse_emu_y > 32)  mouse_emu_y = 32;
					}
				}
			}
			*/
			//printf("mouse_emu %d, %d, %d\n", mouse_btn, mouse_emu_x, mouse_emu_y);
			prev_dx = mouse_emu_x;
			prev_dy = mouse_emu_y;
		}
	}

	for (int i = 0; i < 2; i++)
	{
		if (!time[i]) time[i] = GetTimer(af_delay[i]);
		int send = 0;

		if (joy[i] != joy_prev[i])
		{
			if ((joy[i] ^ joy_prev[i]) & autofire[i])
			{
				time[i] = GetTimer(af_delay[i]);
				af[i] = 0;
			}

			send = 1;
			joy_prev[i] = joy[i];
		}

		if (CheckTimer(time[i]))
		{
			time[i] = GetTimer(af_delay[i]);
			af[i] = !af[i];
			if (joy[i] & autofire[i]) send = 1;
		}

		if (send)
		{
			user_io_digital_joystick(i, af[i] ? joy[i] & ~autofire[i] : joy[i]);
		}
	}
}

int is_key_pressed(int key)
{
	unsigned char bits[(KEY_MAX + 7) / 8];
	for (int i = 0; i < NUMDEV; i++)
	{
		if (pool[i].fd > 0)
		{
			unsigned long evbit = 0;
			if (ioctl(pool[i].fd, EVIOCGBIT(0, sizeof(evbit)), &evbit) >= 0)
			{
				if (evbit & (1 << EV_KEY))
				{
					memset(bits, 0, sizeof(bits));
					if (ioctl(pool[i].fd, EVIOCGKEY(sizeof(bits)), &bits) >= 0)
					{
						if (bits[key / 8] & (1 << (key % 8)))
						{
							return 1;
						}
					}
				}
			}
		}
	}

	return 0;
}
