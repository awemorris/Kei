/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * ws081-p019 (BUG-156): the host test of the PS/2 mouse driver's
 * IntelliMouse protocols.  The driver (src/drivers/platform/pcat/ps2-8042.c)
 * is compiled with WS018_INPUT_HID_HOST_TEST, so its ports are the model
 * here: an 8042 with a mouse that speaks up to a given protocol (a plain
 * mouse, an IntelliMouse, an IntelliMouse Explorer) and answers the
 * sample-rate knocks as a real one does.  The kernel's locks, interrupts,
 * log and input core are stubs; the input events the mouse publishes are
 * recorded.  For each mouse:
 *   - opening the device identifies it (the ID it ends with, the packet
 *     size, the sample rate back at 100);
 *   - packets fed byte by byte through the interrupt give the motion
 *     (with the ninth bit of the sign), the wheels and the buttons.
 * Prints one line a check and "host-ps2-wheel: PASS" or FAIL at the end.
 *
 *   plan/ws081/tests/host-ps2-wheel.sh
 */

#define WS018_INPUT_HID_HOST_TEST 1

#include "src/drivers/platform/pcat/ps2-8042.c"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* The most bytes the model's controller holds for the driver, and the most events recorded. */
#define MODEL_QUEUE	64U
#define MODEL_EVENTS	64U

/* The controller's commands the model knows. */
#define MODEL_CONTROLLER_IDLE	0U
#define MODEL_CONTROLLER_CONFIG	1U
#define MODEL_CONTROLLER_AUX	2U

/*
 * The model: the bytes waiting for the driver (and whether each is the
 * mouse's), what the next data byte is for, the controller's
 * configuration, and the mouse: the richest protocol it speaks, the one it
 * speaks now, whether its next byte is a rate, the last three rates and
 * the rate it runs at.
 */
struct model {
	uint8_t queue[MODEL_QUEUE];
	int queue_aux[MODEL_QUEUE];
	unsigned queue_head;
	unsigned queue_count;
	unsigned controller_state;
	uint8_t configuration;
	uint8_t richest;
	uint8_t id;
	int rate_next;
	uint8_t rates[3];
	uint8_t rate;
};

/* One event the mouse published. */
struct recorded_event {
	uint16_t type;
	uint16_t code;
	int32_t value;
};

/* The one model and the events recorded since the last clear. */
static struct model model;
static struct recorded_event recorded[MODEL_EVENTS];
static unsigned recorded_count;

/* The mouse's interrupt handler and the devices the driver registered. */
static kern_irq_handler_t model_mouse_handler;
static struct input_device_info model_mouse_copy;
static const struct input_device_info *model_mouse_info;
static int model_failures;

static void model_push(uint8_t value, int aux);
static void model_mouse_byte(uint8_t value);
static void model_reset(uint8_t richest);
static void feed(const uint8_t *bytes, unsigned count);
static int find_event(uint16_t type, uint16_t code, int32_t *value);
static void check(const char *what, int passed);
static void test_mouse(const char *name, uint8_t richest, unsigned size);

/* The kernel's pieces the driver calls, as no-ops or recorders. */
void
spin_init(
	struct spinlock *lock,
	enum lock_rank rank,
	const char *name)
{
	(void)lock;
	(void)rank;
	(void)name;
}

/* Takes no lock on the host. */
unsigned long
spin_lock_irqsave(
	struct spinlock *lock)
{
	(void)lock;
	return 0;
}

/* Releases no lock on the host. */
void
spin_unlock_irqrestore(
	struct spinlock *lock,
	unsigned long enabled)
{
	(void)lock;
	(void)enabled;
}

/* Makes no mutex on the host. */
int
mutex_init(
	struct mutex *mutex,
	enum lock_rank rank,
	const char *name)
{
	(void)mutex;
	(void)rank;
	(void)name;
	return 0;
}

/* Takes no mutex on the host. */
void
mutex_lock(
	struct mutex *mutex)
{
	(void)mutex;
}

/* Releases no mutex on the host. */
void
mutex_unlock(
	struct mutex *mutex)
{
	(void)mutex;
}

/* Keeps the mouse's handler (IRQ 12) for feed(). */
int
kern_irq_register(
	int irq,
	kern_irq_handler_t handler,
	void *argument)
{
	(void)argument;
	if (irq == PS2_MOUSE_IRQ)
		model_mouse_handler = handler;
	return 0;
}

/* Forgets nothing on the host. */
int
kern_irq_unregister(
	int irq,
	kern_irq_handler_t handler,
	void *argument)
{
	(void)irq;
	(void)handler;
	(void)argument;
	return 0;
}

/* Masks nothing on the host. */
void
kern_irq_mask(
	int irq)
{
	(void)irq;
}

/* Unmasks nothing on the host. */
void
kern_irq_unmask(
	int irq)
{
	(void)irq;
}

/* Acknowledges nothing on the host. */
void
kern_irq_send_eoi(
	kern_irq_ack_t acknowledge)
{
	(void)acknowledge;
}

/* Prints the kernel's log lines. */
void
kern_logf(
	const char *format,
	...)
{
	va_list arguments;

	va_start(arguments, format);
	vprintf(format, arguments);
	va_end(arguments);
}

/* Keeps a copy of the mouse's description (the driver's is on its stack): its open and close; gives each device a pointer of its own. */
int
drv_input_device_register(
	const struct input_device_info *info,
	struct input_device **device)
{
	static int devices[2];

	if (strcmp(info->name, "PC/AT PS/2 mouse") == 0) {
		model_mouse_copy = *info;
		model_mouse_info = &model_mouse_copy;
		*device = (struct input_device *)&devices[0];
		return 0;
	}
	*device = (struct input_device *)&devices[1];
	return 0;
}

/* Forgets nothing on the host. */
void
drv_input_device_unregister(
	struct input_device *device)
{
	(void)device;
}

/* Records the mouse's events (the keyboard's are not looked at). */
void
drv_input_device_emit(
	struct input_device *device,
	uint16_t type,
	uint16_t code,
	int32_t value)
{
	(void)device;
	if (recorded_count == MODEL_EVENTS)
		return;
	recorded[recorded_count].type = type;
	recorded[recorded_count].code = code;
	recorded[recorded_count].value = value;
	recorded_count++;
}

/* Gives every key symbol a code, so that the keyboard's table builds. */
uint16_t
drv_input_key_from_symbol(
	const char *symbol)
{
	(void)symbol;
	return 1;
}

/* The status port: a byte waiting (and whether it is the mouse's); the input buffer is always empty. */
uint8_t
ws018_input_hid_test_inb(
	uint16_t port)
{
	uint8_t value;

	/* The status. */
	if (port == I8042_STATUS) {
		value = 0;
		if (model.queue_count != 0U) {
			value |= I8042_STATUS_OUTPUT;
			if (model.queue_aux[model.queue_head])
				value |= I8042_STATUS_AUX;
		}
		return value;
	}

	/* The data: the oldest byte waiting. */
	if (model.queue_count == 0U)
		return 0;
	value = model.queue[model.queue_head];
	model.queue_head = (model.queue_head + 1U) % MODEL_QUEUE;
	model.queue_count--;
	return value;
}

/* A command to the controller, or a data byte for it or for the mouse. */
void
ws018_input_hid_test_outb(
	uint16_t port,
	uint8_t value)
{
	/* The controller's commands. */
	if (port == I8042_COMMAND) {
		if (value == I8042_READ_CONFIG)
			model_push(model.configuration, 0);
		else if (value == I8042_WRITE_CONFIG)
			model.controller_state = MODEL_CONTROLLER_CONFIG;
		else if (value == I8042_WRITE_AUX)
			model.controller_state = MODEL_CONTROLLER_AUX;
		return;
	}

	/* A data byte: the configuration, or the mouse's. */
	if (model.controller_state == MODEL_CONTROLLER_CONFIG) {
		model.configuration = value;
	} else if (model.controller_state == MODEL_CONTROLLER_AUX) {
		model_mouse_byte(value);
	}
	model.controller_state = MODEL_CONTROLLER_IDLE;
}

/* Puts a byte where the driver reads it. */
static void
model_push(
	uint8_t value,
	int aux)
{
	unsigned at;

	if (model.queue_count == MODEL_QUEUE)
		return;
	at = (model.queue_head + model.queue_count) % MODEL_QUEUE;
	model.queue[at] = value;
	model.queue_aux[at] = aux;
	model.queue_count++;
}

/*
 * The mouse takes one byte: a rate after SET_SAMPLE_RATE (the knocks
 * 200, 100, 80 and 200, 200, 80 change its ID as far as it can go), or a
 * command, each acknowledged.
 */
static void
model_mouse_byte(
	uint8_t value)
{
	/* A rate, kept with the two before it. */
	if (model.rate_next) {
		model.rate_next = 0;
		model.rates[0] = model.rates[1];
		model.rates[1] = model.rates[2];
		model.rates[2] = value;
		model.rate = value;
		model_push(PS2_ACK, 1);

		/* The knocks. */
		if (model.rates[0] == 200U && model.rates[1] == 100U && model.rates[2] == 80U && model.richest >= PS2_ID_WHEEL && model.id == PS2_ID_STANDARD)
			model.id = PS2_ID_WHEEL;
		if (model.rates[0] == 200U && model.rates[1] == 200U && model.rates[2] == 80U && model.richest >= PS2_ID_EXPLORER && model.id == PS2_ID_WHEEL)
			model.id = PS2_ID_EXPLORER;
		return;
	}

	/* The commands. */
	model_push(PS2_ACK, 1);
	if (value == PS2_SET_SAMPLE_RATE)
		model.rate_next = 1;
	else if (value == PS2_GET_DEVICE_ID)
		model_push(model.id, 1);
	else if (value == PS2_SET_DEFAULTS)
		model.rate = 100U;
}

/* A new controller and mouse that speaks up to a protocol. */
static void
model_reset(
	uint8_t richest)
{
	memset(&model, 0, sizeof(model));
	model.richest = richest;
	model.id = PS2_ID_STANDARD;
	model.rate = 100U;
}

/* Gives the driver a packet's bytes, one interrupt a byte, as the 8042 raises them. */
static void
feed(
	const uint8_t *bytes,
	unsigned count)
{
	unsigned index;

	recorded_count = 0;
	for (index = 0; index < count; index++) {
		model_push(bytes[index], 1);
		model_mouse_handler(PS2_MOUSE_IRQ, 0, NULL);
	}
}

/* Finds a recorded event; returns 1 with its value, 0 when there is none. */
static int
find_event(
	uint16_t type,
	uint16_t code,
	int32_t *value)
{
	unsigned index;

	for (index = 0; index < recorded_count; index++) {
		if (recorded[index].type == type && recorded[index].code == code) {
			*value = recorded[index].value;
			return 1;
		}
	}
	return 0;
}

/* Prints one check's verdict. */
static void
check(
	const char *what,
	int passed)
{
	printf("%s: %s\n", what, passed ? "ok" : "FAIL");
	if (!passed)
		model_failures++;
}

/* Opens a mouse that speaks up to a protocol, checks what the driver found, feeds packets, closes it. */
static void
test_mouse(
	const char *name,
	uint8_t richest,
	unsigned size)
{
	static const uint8_t plain_motion[] = { 0x28, 0x05, 0xfd };
	static const uint8_t far_right[] = { 0x08, 0x90, 0x00 };
	static const uint8_t far_left[] = { 0x18, 0x90, 0x00 };
	static const uint8_t wheel_up[] = { 0x08, 0x00, 0x00, 0xff };
	static const uint8_t wheel_down_two[] = { 0x08, 0x00, 0x00, 0x02 };
	static const uint8_t explorer_up[] = { 0x08, 0x00, 0x00, 0x0f };
	static const uint8_t explorer_side[] = { 0x08, 0x00, 0x00, 0x10 };
	static const uint8_t explorer_left[] = { 0x08, 0x00, 0x00, 0x41 };
	uint8_t padded[4];
	char what[128];
	int32_t value;
	int found;
	int error;

	/* Open: the knocks and the protocol the driver settles on. */
	model_reset(richest);
	error = model_mouse_info->open(NULL);
	snprintf(what, sizeof(what), "%s: open", name);
	check(what, error == 0);
	snprintf(what, sizeof(what), "%s: id %u packet %u", name, (unsigned)mouse_protocol, packet_size);
	check(what, mouse_protocol == richest && packet_size == size);
	snprintf(what, sizeof(what), "%s: rate back at %u", name, (unsigned)model.rate);
	check(what, model.rate == 100U);

	/* Motion: right 5, down 3 (PS/2 up is positive; 0xfd with the Y sign bit 0x20 is -3). */
	memset(padded, 0, sizeof(padded));
	memcpy(padded, plain_motion, sizeof(plain_motion));
	feed(padded, size);
	found = find_event(EV_REL, REL_X, &value);
	snprintf(what, sizeof(what), "%s: motion x 5", name);
	check(what, found && value == 5);
	found = find_event(EV_REL, REL_Y, &value);
	snprintf(what, sizeof(what), "%s: motion y 3", name);
	check(what, found && value == 3);

	/* The ninth bit: 0x90 without the sign is 144 to the right, with it 112 to the left. */
	memset(padded, 0, sizeof(padded));
	memcpy(padded, far_right, sizeof(far_right));
	feed(padded, size);
	found = find_event(EV_REL, REL_X, &value);
	snprintf(what, sizeof(what), "%s: motion x 144", name);
	check(what, found && value == 144);
	memset(padded, 0, sizeof(padded));
	memcpy(padded, far_left, sizeof(far_left));
	feed(padded, size);
	found = find_event(EV_REL, REL_X, &value);
	snprintf(what, sizeof(what), "%s: motion x -112", name);
	check(what, found && value == -112);

	/* The IntelliMouse's wheel: Z -1 is a notch up (+1), Z 2 two down (-2). */
	if (richest == PS2_ID_WHEEL) {
		feed(wheel_up, sizeof(wheel_up));
		found = find_event(EV_REL, REL_WHEEL, &value);
		snprintf(what, sizeof(what), "%s: wheel up", name);
		check(what, found && value == 1);
		feed(wheel_down_two, sizeof(wheel_down_two));
		found = find_event(EV_REL, REL_WHEEL, &value);
		snprintf(what, sizeof(what), "%s: wheel down two", name);
		check(what, found && value == -2);
	}

	/* The Explorer: a notch up in four bits, the side button, a horizontal notch. */
	if (richest == PS2_ID_EXPLORER) {
		feed(explorer_up, sizeof(explorer_up));
		found = find_event(EV_REL, REL_WHEEL, &value);
		snprintf(what, sizeof(what), "%s: wheel up", name);
		check(what, found && value == 1);
		feed(explorer_side, sizeof(explorer_side));
		found = find_event(EV_KEY, BTN_SIDE, &value);
		snprintf(what, sizeof(what), "%s: side button", name);
		check(what, found && value == 1);
		feed(explorer_left, sizeof(explorer_left));
		found = find_event(EV_REL, REL_HWHEEL, &value);
		snprintf(what, sizeof(what), "%s: horizontal wheel", name);
		check(what, found && value == -1);
		found = find_event(EV_KEY, BTN_SIDE, &value);
		snprintf(what, sizeof(what), "%s: side button kept through a scroll packet", name);
		check(what, !found);
	}

	/* A plain mouse never reports a wheel. */
	if (richest == PS2_ID_STANDARD) {
		feed(plain_motion, sizeof(plain_motion));
		found = find_event(EV_REL, REL_WHEEL, &value);
		snprintf(what, sizeof(what), "%s: no wheel", name);
		check(what, !found);
	}

	/* Closed, a side button still held is released. */
	recorded_count = 0;
	model_mouse_info->close(NULL);
}

/*
 * Runs the three mice.
 */
int
main(void)
{
	int error;

	/* The driver, registered with the stubs. */
	error = drv_pcat_ps2_8042_init();
	check("init", error == 0 && model_mouse_info != NULL && model_mouse_handler != NULL);
	if (error != 0 || model_mouse_info == NULL || model_mouse_handler == NULL) {
		printf("host-ps2-wheel: FAIL\n");
		return 1;
	}

	/* A plain mouse, an IntelliMouse and an Explorer. */
	test_mouse("plain", PS2_ID_STANDARD, 3U);
	test_mouse("intellimouse", PS2_ID_WHEEL, 4U);
	test_mouse("explorer", PS2_ID_EXPLORER, 4U);

	/* The verdict. */
	if (model_failures != 0) {
		printf("host-ps2-wheel: FAIL (%d)\n", model_failures);
		return 1;
	}
	printf("host-ps2-wheel: PASS\n");
	return 0;
}
