/*
 * Copyright (C) 2013 Canonical Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Authored by: Thomas Voß <thomas.voss@canonical.com>
 *              Ricardo Salveti de Araujo <ricardo.salveti@canonical.com>
 */

#include <hybris/input/input_stack_compatibility_layer.h>

#include "InputListener.h"
#include "InputReader.h"

#include "PointerController.h"
#include "SpriteController.h"
#include <gui/ISurfaceComposer.h>
#include <gui/SurfaceComposerClient.h>
#include <input/Input.h>
#include <ui/DisplayId.h>

#undef LOG_TAG
#define LOG_TAG "InputStackCompatibilityLayer"
#include <utils/Log.h>

namespace
{
static bool enable_verbose_function_reporting = false;
}

#define REPORT_FUNCTION() ALOGV("%s\n", __PRETTY_FUNCTION__);

// Use android::ui namespace for Android 15
using android::ui::LogicalDisplayId;
using android::ui::Rotation;

namespace
{

class DefaultPointerControllerPolicy : public android::PointerControllerPolicyInterface
{
public:
	static const size_t bitmap_width = 64;
	static const size_t bitmap_height = 64;

	DefaultPointerControllerPolicy()
	{
		// For Android 15+, SpriteIcon no longer uses SkBitmap
		// Icons are loaded from resources instead
		// Just create empty icons as placeholders
		spotTouchIcon = android::SpriteIcon();
		spotAnchorIcon = android::SpriteIcon();
		spotHoverIcon = android::SpriteIcon();
	}

	virtual void loadPointerResources(android::PointerResources* outResources, LogicalDisplayId)
	{
		outResources->spotHover = spotHoverIcon;
		outResources->spotTouch = spotTouchIcon;
		outResources->spotAnchor = spotAnchorIcon;
	}

#if ANDROID_VERSION_MAJOR >= 7
        virtual void loadPointerIcon(android::SpriteIcon* icon, LogicalDisplayId)
        {
            if (icon) {
                *icon = spotTouchIcon;
            }
        }

        virtual void loadAdditionalMouseResources(std::map<android::PointerIconStyle, android::SpriteIcon>* /*outResources*/,
                                          std::map<android::PointerIconStyle, android::PointerAnimation>* /*outAnimationResources*/,
                                          LogicalDisplayId) {}
        virtual android::PointerIconStyle getDefaultPointerIconId() { return android::PointerIconStyle::TYPE_ARROW; }
        virtual android::PointerIconStyle getDefaultStylusIconId() { return android::PointerIconStyle::TYPE_ARROW; }
        virtual android::PointerIconStyle getCustomPointerIconId() { return android::PointerIconStyle::TYPE_ARROW; }
#endif

	android::SpriteIcon spotHoverIcon;
	android::SpriteIcon spotTouchIcon;
	android::SpriteIcon spotAnchorIcon;
};

class DefaultInputReaderPolicyInterface : public android::InputReaderPolicyInterface
{
public:
	static const int32_t internal_display_id = 0; // ui::LogicalDisplayId for main display
	static const int32_t external_display_id = 1; // ui::LogicalDisplayId for HDMI

	DefaultInputReaderPolicyInterface(
			InputStackConfiguration* configuration,
			const android::sp<android::Looper>& looper)
			: looper(looper),
		default_layer_for_touch_point_visualization(configuration->default_layer_for_touch_point_visualization),
		input_area_width(configuration->input_area_width),
		input_area_height(configuration->input_area_height)
	{
		// Android 15: Configure display viewport
		android::DisplayViewport viewport;
		viewport.setNonDisplayViewport(input_area_width, input_area_height);
		viewport.displayId = LogicalDisplayId(0); // Main display

		// Add viewport to configuration using public method
		default_configuration.setDisplayViewports({viewport});
	}

	void getReaderConfiguration(android::InputReaderConfiguration* outConfig)
	{
		*outConfig = default_configuration;
	}

	virtual void notifyInputDevicesChanged(const std::vector<android::InputDeviceInfo>& inputDevices) {
		mInputDevices.clear();
		for (const auto& device : inputDevices) {
			mInputDevices.push(device);
		}
	}

#if ANDROID_VERSION_MAJOR>=15
	virtual void notifyTouchpadHardwareState(const android::SelfContainedHardwareState& /*schs*/, int32_t /*deviceId*/) {}
	virtual void notifyTouchpadGestureInfo(GestureType /*type*/, int32_t /*deviceId*/) {}
	virtual void notifyTouchpadThreeFingerTap() {}

	virtual std::shared_ptr<android::KeyCharacterMap> getKeyboardLayoutOverlay(
			const android::InputDeviceIdentifier& /*identifier*/,
			const std::optional<android::KeyboardLayoutInfo> /*keyboardLayoutInfo*/) {
		return nullptr;
	}

	virtual std::string getDeviceAlias(const android::InputDeviceIdentifier& /*identifier*/) {
		return std::string();
	}

	virtual android::TouchAffineTransformation getTouchAffineTransformation(
			const std::string& /*inputDeviceDescriptor*/, android::ui::Rotation /*surfaceRotation*/) {
		return android::TouchAffineTransformation();
	}

	virtual void notifyStylusGestureStarted(int32_t /*deviceId*/, nsecs_t /*eventTime*/) {}

	virtual bool isInputMethodConnectionActive() {
		return false;
	}

	virtual std::optional<android::DisplayViewport> getPointerViewportForAssociatedDisplay(
			android::ui::LogicalDisplayId /*associatedDisplayId*/) {
		// Android 15: Methods not available in linkable form, return nullopt
		// Input will work without pointer viewport
		return std::nullopt;
	}
#endif

#if ANDROID_VERSION_MAJOR<=4
	virtual android::sp<android::KeyCharacterMap> getKeyboardLayoutOverlay(const android::String8& inputDeviceDescriptor) {
		return nullptr;
	}
#elif ANDROID_VERSION_MAJOR>=5 && ANDROID_VERSION_MAJOR<15
	virtual std::shared_ptr<android::KeyCharacterMap> getKeyboardLayoutOverlay(const android::InputDeviceIdentifier& identifier,
	                                                                             const std::optional<android::KeyboardLayoutInfo>) {
		return nullptr;
	}

	virtual std::string getDeviceAlias(const android::InputDeviceIdentifier& identifier) {
		return "";
	}
#endif

#if ANDROID_VERSION_MAJOR>=5 && ANDROID_VERSION_MAJOR<15
	virtual android::TouchAffineTransformation getTouchAffineTransformation(const std::string& inputDeviceDescriptor, Rotation surfaceRotation) {
		return android::TouchAffineTransformation();
	}

	virtual void notifyStylusGestureStarted(int32_t deviceId, nsecs_t eventTime) {
	}

	virtual void notifyTouchpadHardwareState(const android::SelfContainedHardwareState& schs,
	                                         int32_t deviceId) {
	}

	virtual void notifyTouchpadGestureInfo(GestureType type, int32_t deviceId) {
	}

	virtual void notifyTouchpadThreeFingerTap() {
	}

	virtual bool isInputMethodConnectionActive() {
		return false;
	}

	virtual std::optional<android::DisplayViewport> getPointerViewportForAssociatedDisplay(
	        LogicalDisplayId associatedDisplayId) {
		return std::nullopt;
	}
#endif

private:
	android::sp<android::Looper> looper;
	int default_layer_for_touch_point_visualization;
	android::InputReaderConfiguration default_configuration;
	android::Vector<android::InputDeviceInfo> mInputDevices;
	int input_area_width;
	int input_area_height;
};

class ExportedInputListener : public android::InputListenerInterface
{
public:
	ExportedInputListener(AndroidEventListener* external_listener) : external_listener(external_listener)
	{
	}

	void notifyConfigurationChanged(const android::ConfigurationChanges& changes)
	{
		REPORT_FUNCTION();
		(void) changes;
	}

	void notifyKey(const android::NotifyKeyArgs& args)
	{
		REPORT_FUNCTION();

		current_event.type = KEY_EVENT_TYPE;
		current_event.device_id = args.deviceId;
		current_event.source_id = args.source;
		current_event.action = args.action;
		current_event.flags = args.flags;
		current_event.meta_state = args.metaState;

		current_event.details.key.key_code = args.keyCode;
		current_event.details.key.scan_code = args.scanCode;
		current_event.details.key.down_time = args.downTime;
		current_event.details.key.event_time = args.eventTime;

		current_event.details.key.is_system_key = false;

		external_listener->on_new_event(&current_event, external_listener->context);
	}

	void notifyMotion(const android::NotifyMotionArgs& args)
	{
		REPORT_FUNCTION();

		current_event.type = MOTION_EVENT_TYPE;
		current_event.device_id = args.deviceId;
		current_event.source_id = args.source;
		current_event.action = args.action;
		current_event.flags = args.flags;
		current_event.meta_state = args.metaState;

		current_event.details.motion.button_state = args.buttonState;
		current_event.details.motion.down_time = args.downTime;
		current_event.details.motion.event_time = args.eventTime;
		current_event.details.motion.edge_flags = args.edgeFlags;
		current_event.details.motion.x_precision = args.xPrecision;
		current_event.details.motion.y_precision = args.yPrecision;
		current_event.details.motion.pointer_count = args.pointerProperties.size();

		for (unsigned int i = 0; i < current_event.details.motion.pointer_count; i++) {
			current_event.details.motion.pointer_coordinates[i].id = args.pointerProperties[i].id;
			current_event.details.motion.pointer_coordinates[i].x
				= current_event.details.motion.pointer_coordinates[i].raw_x
				= args.pointerCoords[i].getX();
			current_event.details.motion.pointer_coordinates[i].y
				= current_event.details.motion.pointer_coordinates[i].raw_y
				= args.pointerCoords[i].getY();
			current_event.details.motion.pointer_coordinates[i].touch_major
				= args.pointerCoords[i].getAxisValue(AMOTION_EVENT_AXIS_TOUCH_MAJOR);
			current_event.details.motion.pointer_coordinates[i].touch_minor
				= args.pointerCoords[i].getAxisValue(AMOTION_EVENT_AXIS_TOUCH_MINOR);
			current_event.details.motion.pointer_coordinates[i].pressure
				= args.pointerCoords[i].getAxisValue(AMOTION_EVENT_AXIS_PRESSURE);
			current_event.details.motion.pointer_coordinates[i].size
				= args.pointerCoords[i].getAxisValue(AMOTION_EVENT_AXIS_SIZE);
			current_event.details.motion.pointer_coordinates[i].orientation
				= args.pointerCoords[i].getAxisValue(AMOTION_EVENT_AXIS_ORIENTATION);

		}

		external_listener->on_new_event(&current_event, external_listener->context);
	}

	void notifySwitch(const android::NotifySwitchArgs& args)
	{
		REPORT_FUNCTION();
		current_event.type = HW_SWITCH_EVENT_TYPE;

		current_event.details.hw_switch.event_time = args.eventTime;
		current_event.details.hw_switch.policy_flags = args.policyFlags;
		current_event.details.hw_switch.switch_values = args.switchValues;
		current_event.details.hw_switch.switch_mask = args.switchMask;

		external_listener->on_new_event(&current_event, external_listener->context);
	}

	void notifyDeviceReset(const android::NotifyDeviceResetArgs& args)
	{
		REPORT_FUNCTION();
		(void) args;
	}

	void notifyPointerCaptureChanged(const android::NotifyPointerCaptureChangedArgs& args)
	{
		REPORT_FUNCTION();
		(void) args;
	}

	void notifyVibratorState(const android::NotifyVibratorStateArgs& args)
	{
		REPORT_FUNCTION();
		(void) args;
	}

	void notifySensor(const android::NotifySensorArgs& args)
	{
		REPORT_FUNCTION();
		(void) args;
	}

	void notifyInputDevicesChanged(const android::NotifyInputDevicesChangedArgs& args)
	{
		REPORT_FUNCTION();
		(void) args;
	}

private:
	AndroidEventListener* external_listener;
	Event current_event;
};

class LooperThread : public android::Thread
{
public:
	static const int default_poll_timeout_ms = -1;

	LooperThread(const android::sp<android::Looper>& looper) : looper(looper)
	{
	}

private:
	bool threadLoop()
	{
		if (ALOOPER_POLL_ERROR == looper->pollAll(default_poll_timeout_ms))
			return false;
		return true;
	}

	android::sp<android::Looper> looper;
};

struct State : public android::RefBase
{
	State(AndroidEventListener* listener,
			InputStackConfiguration* configuration)
			: looper(new android::Looper(false)),
			looper_thread(new LooperThread(looper)),
			event_hub(std::make_shared<android::EventHub>()),
			input_reader_policy_impl(new DefaultInputReaderPolicyInterface(configuration, looper)),
			input_reader_policy(input_reader_policy_impl),  // Wrap in sp
			input_listener_impl(new ExportedInputListener(listener))
	{
		// Initialize input_reader with reference to listener
		input_reader = std::make_shared<android::InputReader>(
						event_hub,
						input_reader_policy,
						*input_listener_impl);
	}

	~State()
	{
		// Android 15: No separate InputReaderThread
		// Clean up in reverse order
		input_reader.reset();
		delete input_listener_impl;
		input_reader_policy.clear();  // sp will handle deletion
	}

	android::sp<android::Looper> looper;
	android::sp<LooperThread> looper_thread;

	std::shared_ptr<android::EventHubInterface> event_hub;
	DefaultInputReaderPolicyInterface* input_reader_policy_impl;  // Raw pointer
	android::sp<android::InputReaderPolicyInterface> input_reader_policy;  // sp wrapper for InputReader
	ExportedInputListener* input_listener_impl;  // Raw pointer - doesn't inherit from RefBase
	std::shared_ptr<android::InputReaderInterface> input_reader;

	android::Condition wait_condition;
	android::Mutex wait_guard;
};

android::sp<State> global_state;

}

void android_input_stack_initialize(AndroidEventListener* listener, InputStackConfiguration* config)
{
	global_state = new State(listener, config);
}

void android_input_stack_loop_once()
{
	// Android 15: InputReader has its own thread via start()
	// This is kept for API compat but is a no-op
	global_state->looper->pollOnce(0);
}

void android_input_stack_start()
{
	// Android 15: InputReader::start() spawns its own InputThread
	global_state->input_reader->start();
}

void android_input_stack_start_waiting_for_flag(bool* flag)
{
	// Android 15: InputReader::start() spawns its own InputThread
	global_state->input_reader->start();

	while (!*flag) {
		global_state->wait_condition.waitRelative(
				global_state->wait_guard,
				10 * 1000 * 1000);
	}
}

void android_input_stack_stop()
{
	// Android 15: Stop the InputReader thread
	global_state->input_reader->stop();
}

void android_input_stack_shutdown()
{
	global_state = NULL;
}
