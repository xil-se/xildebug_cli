#include <getopt.h>
#include <libusb.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define XILDEBUG_VID 0x1209
#define XILDEBUG_PID 0x9450
#define XILDEBUG_HID_IFACE_NO 2

static bool quit = false;
static libusb_device_handle *handle = NULL;
static libusb_device *dev = NULL;

typedef struct
{
	char *name;
	int (*func)(const char *);
	char *doc;
} command_t;

int cmd_init(const char *);
int cmd_deinit(const char *);
int cmd_transfer(const char *);
int cmd_help(const char *);
int cmd_quit(const char *);

command_t commands[] =
{
	{ "init",			cmd_init,			"Initiailize xildebug" },
	{ "deinit",			cmd_deinit,			"Deinitialize xildebug" },

	{ "transfer",		cmd_transfer,		"Transfer data" },

	{ "help",			cmd_help,			"Display this message" },
	{ "?",				cmd_help,			NULL },

	{ "quit",			cmd_quit,			"Quit program" },
	{ "exit",			cmd_quit,			NULL },

	{ NULL }
};

char *strtrim(char *str)
{
	char *p = str;
	while(isspace(*p))
		++p;

	if(*p == '\0')
		return p;

	char *t = p + strlen(p) - 1;
	while((t > p) && isspace(*t))
		--t;

	*(++t) = '\0';

	return p;
}

int cmd_init(const char *arg)
{
	int r;

	r = libusb_init(NULL);
	if (r != LIBUSB_SUCCESS) {
		printf("Failed to init libusb\r\n");
		return 1;
	}

	// TODO: Allow to filter by serial
	handle = libusb_open_device_with_vid_pid(NULL, XILDEBUG_VID, XILDEBUG_PID);
	if (handle == NULL) {
		printf("Failed to open vid: 0x%04X, pid: 0x%04X\r\n", XILDEBUG_VID, XILDEBUG_PID);
		libusb_exit(NULL);
		return 1;
	}

	dev = libusb_get_device(handle);
	if (dev == NULL) {
		printf("Failed to get device pointer\r\n");
		libusb_close(handle);
		libusb_exit(NULL);
		return 1;
	}

	libusb_set_auto_detach_kernel_driver(handle, 1);

	r = libusb_claim_interface(handle, XILDEBUG_HID_IFACE_NO);
	if (r != LIBUSB_SUCCESS) {
		printf("Failed to get claim the HID interface\r\n");
		libusb_close(handle);
		libusb_exit(NULL);
		return 1;
	}

	printf("Initialized.\r\n");

	return 0;
}

int cmd_deinit(const char *arg)
{
	if (dev)
		libusb_release_interface(handle, XILDEBUG_HID_IFACE_NO);

	if (handle)
		libusb_close(handle);

	libusb_exit(NULL);

	handle = NULL;
	dev = NULL;

	printf("Deinitialized.\r\n");

	return 0;
}

int cmd_transfer(const char *arg)
{
	uint8_t buf[64] = { 0x00 };
	int xfer_len = 0;
	int r;

	if (handle == NULL) {
		printf("Please call 'init' first\r\n");
		return 1;
	}

	for (size_t i = 0; i < strlen(arg) / 2; ++i)
		sscanf(arg + i * 2, "%02hhx", &buf[i]);

	r = libusb_interrupt_transfer(handle, 0x03, buf, sizeof(buf), &xfer_len, 1000);
	if (r != LIBUSB_SUCCESS)
		return r;

	r = libusb_interrupt_transfer(handle, 0x83, buf, sizeof(buf), &xfer_len, 1000);
	if (r != LIBUSB_SUCCESS)
		return r;

	printf("Received %d bytes:\r\n", xfer_len);
	for (int i = 0; i < xfer_len; ++i)
		printf("0x%02X ", buf[i]);
	printf("\r\n");

	return 0;
}

int cmd_help(const char *arg)
{
	int printed = 0;

	for(int i = 0; commands[i].name; i++) {
		if (!(*arg) || (strcmp(arg, commands[i].name) == 0)) {
			if(commands[i].doc) {
				printf("%-20s%s.\n", commands[i].name, commands[i].doc);
				++printed;
			}
		}
	}

	if(!printed) {
		printf("No command matching '%s'. Available commands:\n", arg);

		for(int i = 0; commands[i].name; i++) {
			if(printed == 6) {
				printed = 0;
				printf("\n");
			}

			printf("%s\t", commands[i].name);
			++printed;
		}

		if(printed)
			printf("\n");
	}

	return 0;
}

int cmd_quit(const char *arg)
{
	quit = true;

	return 0;
}

command_t *find_cmd(char *name)
{
	for(int i = 0; commands[i].name; i++)
		if(strcmp(name, commands[i].name) == 0)
			return &commands[i];

	return NULL;
}

int exec_cmd(char *line)
{
	int i = 0;
	while (line[i] && isspace(line[i]))
		++i;

	char *word = line + i;

	while(line[i] && !isspace(line[i]))
		++i;

	if(line[i])
		line[i++] = '\0';

	command_t *cmd = find_cmd(word);
	if(!cmd) {
		printf("%s: no such command\n", word);
		return -1;
	}

	while(whitespace(line[i]))
		++i;

	word = line + i;

	return (*(cmd->func))(word);
}

char *cmd_generator(const char *text, int state)
{
	static int idx, len;

	if(!state) {
		idx = 0;
		len = strlen(text);
	}

	char *name;
	while((name = commands[idx].name)) {
		++idx;

		if(strncmp(name, text, len) == 0)
			return strdup(name);
	}

	return NULL;
}

char **cmd_completion(const char *text, int start, int end)
{
	if(start == 0)
		return rl_completion_matches(text, cmd_generator);

	return NULL;
}

void print_usage(const char *program)
{
	const char *idx = strrchr(program, '/');
	if(idx)
		program = ++idx;

	fprintf(stderr, "Usage:\n"
		"   %s [OPTION...]\n\n"
		"Help Options:\n"
		"   -h    --help              Print this message\n\n",
		program);
}

int main(int argc, char *argv[])
{
	const struct option long_opts[] =
	{
		{ "help", no_argument, NULL, 'h' },
		{ NULL }
	};

	opterr = false;

	while(true) {
		const int c = getopt_long(argc, argv, "h", long_opts, NULL);
		if(c == -1)
			break;

		switch(c) {
			case 'h':
			default:
				print_usage(argv[0]);
				return 1;
		}
	}

	// TODO: Leftover text is commands to execute

	rl_attempted_completion_function = cmd_completion;

	char *line = NULL;
	while(!quit) {
		line = readline("# ");

		char *input = strtrim(line);
		if(input) {
			add_history(input);

			if(exec_cmd(input) != 0)
				printf("Failed to exec command\r\n");
		}

		free(line);
		line = NULL;
	}

	if (handle != NULL)
		cmd_deinit(NULL);

	return 0;
}
