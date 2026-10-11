/* Native PufferLib 4 interface. No adjudication rules are changed here. */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "engine.h"

typedef struct {
    float avg_welfare;
    float avg_centers;
    float avg_units;
    float n;
} Log;

typedef struct {
    Log log;
    int num_agents;
    unsigned int rng;
    float *observations, *actions, *rewards, *terminals;
    DiplomacyEngine engine;
    int32_t engine_actions[MAX_POWERS];
    unsigned char engine_terminals[MAX_POWERS];
} Env;

void c_reset(Env* env) {
    env->engine.observations = env->observations;
    env->engine.actions = env->engine_actions;
    env->engine.rewards = env->rewards;
    env->engine.terminals = env->engine_terminals;
    diplomacy_engine_reset(&env->engine);
    memset(env->terminals, 0, MAX_POWERS * sizeof(float));
}

void c_step(Env* env) {
    for (int p = 0; p < MAX_POWERS; p++) {
        float action = env->actions[p];
        if (!isfinite(action) || action < 0 || action >= ACTION_SPACE_SIZE
                || action != floorf(action)) {
            fprintf(stderr, "Diplomacy requires integer actions in [0, %d).\n",
                    ACTION_SPACE_SIZE);
            abort();
        }
        env->engine_actions[p] = (int32_t)action;
    }
    diplomacy_engine_step(&env->engine);
    for (int p = 0; p < MAX_POWERS; p++) {
        env->terminals[p] = (float)env->engine_terminals[p];
    }
    if (env->engine.game->is_game_over) {
        env->log.avg_welfare += env->engine.log.avg_welfare;
        env->log.avg_centers += env->engine.log.avg_centers;
        env->log.avg_units += env->engine.log.avg_units;
        env->log.n += env->engine.log.n;
        memset(&env->engine.log, 0, sizeof(DiplomacyEngineLog));

        /* Return the completed transition's flags/rewards with next-game obs. */
        float final_rewards[MAX_POWERS];
        memcpy(final_rewards, env->rewards, sizeof(final_rewards));
        c_reset(env);
        memcpy(env->rewards, final_rewards, sizeof(final_rewards));
        for (int p = 0; p < MAX_POWERS; p++) env->terminals[p] = 1.0f;
    }
}

void c_render(Env* env) { diplomacy_engine_render(&env->engine); }
void c_close(Env* env) { diplomacy_engine_close(&env->engine); }

#define OBS_SIZE OBS_TOTAL_SIZE
#define NUM_ATNS 1
#define ACT_SIZES {ACTION_SPACE_SIZE}
#define OBS_TENSOR_T FloatTensor
#include "vecenv.h"

void my_init(Env* env, Dict* kwargs) {
    env->num_agents = MAX_POWERS;
    diplomacy_engine_init(&env->engine);
    if (!env->engine.game) abort();
    DictItem* welfare = dict_get_unsafe(kwargs, "welfare_mode");
    DictItem* years = dict_get_unsafe(kwargs, "max_years");
    diplomacy_engine_configure(&env->engine,
        welfare ? (int)welfare->value : 1, years ? (int)years->value : 10);
    /* vecenv binds buffers later; reset must not run during initialization. */
}

void my_log(Log* log, Dict* out) {
    dict_set(out, "avg_welfare", log->avg_welfare);
    dict_set(out, "avg_centers", log->avg_centers);
    dict_set(out, "avg_units", log->avg_units);
}
