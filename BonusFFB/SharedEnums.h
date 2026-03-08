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
