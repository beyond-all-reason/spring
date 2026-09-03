/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef AI_S_INTENTS_H
#define AI_S_INTENTS_H

// IMPORTANT NOTE: unlike AISEvents.h external systems do not parse this file.

#ifdef	__cplusplus
extern "C" {
#endif

// NOTE structs should not be empty (C90), so add a useless member if needed

/**
 * Each intent type can be identified through a unique ID,
 * which we call intent topic.
 * Intents are sent from the game to AIs.
 *
 * Note: Do NOT change the values assigned to these topics,
 * as this would be bad for inter-version compatibility.
 * You should always append new intent topics at the end of this list,
 * and adjust NUM_INTENTS.
 *
 * @see SSkirmishAILibrary.handleIntent()
 */
enum IntentTopic {
	INTENT_NULL                        =  0,
	INTENT_INT                         =  1,
	INTENT_FLOAT                       =  2,
	INTENT_ARRAY_INT                   =  3,
	INTENT_ARRAY_FLOAT                 =  4,
	INTENT_DICT_INT                    =  5,
	INTENT_DICT_FLOAT                  =  6,
};
const int NUM_INTENTS = 7;


#define AIINTERFACE_INTENTS_ABI_VERSION     ( \
		  sizeof(struct SIntIntent) \
		+ sizeof(struct SFloatIntent) \
		+ sizeof(struct SArrayIntIntent) \
		+ sizeof(struct SArrayFloatIntent) \
		+ sizeof(struct SDictIntIntent) \
		+ sizeof(struct SDictFloatIntent) \
		)

/**
 * Sent by a Lua widget or unsynced gadget.
 */
struct SIntIntent {
	int topic;
	int objId;
	int value;
};

/**
 * Sent by a Lua widget or unsynced gadget.
 */
struct SFloatIntent {
	int topic;
	int objId;
	float value;
};

/**
 * Sent by a Lua widget or unsynced gadget.
 */
struct SArrayIntIntent {
	int topic;
	int objId;
	unsigned int size;
	const int* data;
};

/**
 * Sent by a Lua widget or unsynced gadget.
 */
struct SArrayFloatIntent {
	int topic;
	int objId;
	unsigned int size;
	const float* data;
};

/**
 * Sent by a Lua widget or unsynced gadget.
 */
struct SDictIntIntent {
	int topic;
	int objId;
	unsigned int size;
	const int* keys;
	const int* values;
};

/**
 * Sent by a Lua widget or unsynced gadget.
 */
struct SDictFloatIntent {
	int topic;
	int objId;
	unsigned int size;
	const int* keys;
	const float* values;
};

#ifdef	__cplusplus
} // extern "C"
#endif

#endif // AI_S_INTENTS_H
