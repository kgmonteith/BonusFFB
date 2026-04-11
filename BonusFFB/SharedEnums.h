#pragma once


enum class QUADRANT {
	NW,
	SW,
	NE,
	SE
};

enum class NEUTRAL_SHAPE {
	SQUARE,
	ROUNDED,
	ANGLED
};

static LONG FORWARD[2] = { 0 , 0 };
static LONG BACK[2] = { 180 * DI_DEGREES , 0 };

enum class GrindEffectBehavior {
	MATCH_ENGINE_RPM,
	ADD_ENGINE_RPM,
	OVERRIDE_ENGINE_RPM
};

class SlotParameters : public QObject {
public slots:
	void setButtonZoneDepth(int t) {
		button_zone_depth_telemetry = double(t) * 0.01;
	}

public:
	unsigned int slot_count = 3;
	double pos_pct[4] = { 0, 0.34, 0.66, -1 };
	double depth = 0.75; // HALF the absolute front-to-back range of the slot

	double button_zone_depth_telemetry = 0.35;
	double grind_point_depth = 0.15;
	int middle_slot_half_width = 5000;
	double rounding_factor = JOY_MAXPOINT * .10;	// There's a state change issue or something that causes a light tapping when this is reduced to 5000, but good enough for now

	double asFFBOffset(unsigned int slot_num) {  // -10000 to 10000
		return (20000 * pos_pct[slot_num]) - 10000;
	}

	double asJoystickValue(unsigned int slot_num) { // 0 to 65535
		return (JOY_MAXPOINT * pos_pct[slot_num]);
	}

	double depthAsFFBOffsetFwd() {
		return depth * FFB_MINPOINT;
	}

	double depthAsFFBOffsetBack() {
		return depth * FFB_MAXPOINT;
	}

	double depthAsJoystick() {
		return depth * JOY_MIDPOINT;
	}

	double depthAsJoystickValueFwd() {
		return JOY_MIDPOINT  - (depth * JOY_MIDPOINT);
	}

	double depthAsJoystickValueBack() {
		return JOY_MIDPOINT + (depth * JOY_MIDPOINT);
	}

	double grindPointDepthAsJoystickValueFwd() {
		return JOY_MIDPOINT - (grind_point_depth * JOY_MIDPOINT);
	}

	double grindPointDepthAsJoystickValueBack() {
		return JOY_MIDPOINT + (grind_point_depth * JOY_MIDPOINT);
	}

	double buttonZoneDepthAsJoystickValueFwd() {
		return JOY_MIDPOINT - (button_zone_depth_telemetry * JOY_MIDPOINT);
	}

	double buttonZoneDepthAsJoystickValueBack() {
		return JOY_MIDPOINT + (button_zone_depth_telemetry * JOY_MIDPOINT);
	}

	int getNearestSlot(int lrValue) {
		unsigned int closest_slot = 0;
		int min_distance = std::abs(lrValue - asJoystickValue(0));
		for (unsigned int i = 1; i < slot_count; i++) {
			int t = std::abs(lrValue - asJoystickValue(i));
			if (t < min_distance) {
				min_distance = t;
				closest_slot = i;
			}
		}
		return closest_slot;
	}
};