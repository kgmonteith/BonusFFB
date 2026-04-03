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

class SlotParameters {
public:
	unsigned int slot_count = 3;
	double pos_pct[4] = { 0, 0.5, 1.0, -1 };
	double depth = 1.0; // HALF the absolute front-to-back range of the slot
	int middle_slot_half_width = 1200;

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
};