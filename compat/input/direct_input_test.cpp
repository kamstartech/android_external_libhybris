#include <hybris/input/input_stack_compatibility_layer.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

static bool g_stop = false;

void signal_handler(int) {
    g_stop = true;
}

void on_new_event(Event* event, void* context) {
    printf("Event type=%d device=%d source=%d action=%d\n",
           event->type, event->device_id, event->source_id, event->action);

    if (event->type == MOTION_EVENT_TYPE) {
        printf("  TOUCH: x=%.1f y=%.1f pointers=%zu time=%lld\n",
               event->details.motion.pointer_coordinates[0].x,
               event->details.motion.pointer_coordinates[0].y,
               event->details.motion.pointer_count,
               (long long)event->details.motion.event_time);
    } else if (event->type == KEY_EVENT_TYPE) {
        printf("  KEY: code=%d scan=%d\n",
               event->details.key.key_code,
               event->details.key.scan_code);
    }
    fflush(stdout);
}

int main() {
    printf("Input test starting...\n");
    fflush(stdout);

    g_stop = false;
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    AndroidEventListener listener;
    listener.on_new_event = on_new_event;
    listener.context = NULL;

    InputStackConfiguration config = {
        false,  // no touch point visualization
        10000,  // layer
        1080,   // width
        2340    // height
    };

    printf("Initializing input stack...\n");
    fflush(stdout);
    android_input_stack_initialize(&listener, &config);

    printf("Starting input stack (touch the screen)...\n");
    fflush(stdout);
    android_input_stack_start_waiting_for_flag(&g_stop);

    printf("Stopping...\n");
    android_input_stack_stop();
    android_input_stack_shutdown();
    printf("Done.\n");
    return 0;
}
