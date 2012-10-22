#include <ApplicationServices/ApplicationServices.h>

#include <err.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <unistd.h>

#include "extern.h"

CFRunLoopSourceRef runLoopSource;
CFMachPortRef eventTap = NULL;

int active_keycode;
CGEventFlags active_flags;

bool keys_configured = false;

CGEventRef
key_event_callback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *refcon)
{
	int keycode = CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
	CGEventFlags flags = CGEventGetFlags(event);

	if (type == kCGEventKeyUp && keycode == active_keycode)
		cw_keydown = false;
	if (type == kCGEventKeyDown && keycode == active_keycode && flags == active_flags)
		cw_keydown = true;

	return event;
}

CGEventRef
key_configure_callback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *refcon)
{
	active_keycode = CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
	active_flags = CGEventGetFlags(event);

	key_stop();

	return event;
}

int
key_configure(void)
{
	if (eventTap != NULL) {
		warnx("key configure called when events already initialized");
		return -1;
	}

	eventTap = CGEventTapCreate (
	    kCGHIDEventTap,
	    kCGSessionEventTap,
	    kCGEventTapOptionListenOnly,
	    CGEventMaskBit(kCGEventKeyDown),
	    key_configure_callback,
	    NULL
	);
	if (eventTap == NULL) {
		warnx("unable to create event tap");
		return -1;
	}
	runLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, eventTap, 0);
	CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopCommonModes);

	key_listen();

	key_cleanup();

	keys_configured = true;

	return 0;
}

int
key_init(void)
{
	if (!keys_configured) {
		warnx("key init called without an active key configured");
		return -1;
	}

	if (eventTap != NULL) {
		warnx("key init called when events already initialized");
		return -1;
	}

	eventTap = CGEventTapCreate (
	    kCGHIDEventTap,
	    kCGSessionEventTap,
	    kCGEventTapOptionListenOnly,
	    CGEventMaskBit(kCGEventKeyDown) | CGEventMaskBit(kCGEventKeyUp),
	    key_event_callback,
	    NULL
	);
	if (eventTap == NULL) {
		warnx("unable to create event tap");
		return -1;
	}

	runLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, eventTap, 0);
	CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopCommonModes);

	return 0;
}

int
key_listen(void)
{
	if (eventTap == NULL) {
		warnx("tried to listen when not initialized");
		return -1;
	}

	CGEventTapEnable(eventTap, true);
	CFRunLoopRun();

	return 0;
}

int
key_stop(void)
{
	if (eventTap == NULL) {
		warnx("tried to stop listening when not initialized");
		return -1;
	}

	CFRunLoopStop(CFRunLoopGetCurrent());
	CGEventTapEnable(eventTap, false);

	return 0;
}

int
key_cleanup(void)
{
	if (eventTap == NULL) {
		warnx("key cleanup called with nothing to cleanup");
		return -1;
	}

	CFRunLoopRemoveSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopCommonModes);

	CFRelease(eventTap);
	CFRelease(runLoopSource);
	eventTap = NULL;

	return 0;
}
