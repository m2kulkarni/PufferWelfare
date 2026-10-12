/* Keep the tested engine separate from PufferLib 4's Env and callbacks. */
#ifndef PUFFERWELFARE_ENGINE_ADAPTER_H
#define PUFFERWELFARE_ENGINE_ADAPTER_H
#define Env DiplomacyEngine
#define Log DiplomacyEngineLog
#define c_init diplomacy_engine_init
#define c_reset diplomacy_engine_reset
#define c_step diplomacy_engine_step
#define c_render diplomacy_engine_render
#define c_close diplomacy_engine_close
#define c_configure diplomacy_engine_configure
#include "diplomacy.h"
#undef Env
#undef Log
#undef c_init
#undef c_reset
#undef c_step
#undef c_render
#undef c_close
#undef c_configure
#endif
