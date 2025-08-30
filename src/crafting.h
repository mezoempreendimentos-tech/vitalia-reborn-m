#ifndef _CRAFTING_H_
#define _CRAFTING_H_

#include "structs.h"
#include "crskill.h" // <-- Inclui as definições de perícia de crafting

/* --- Constantes e Definições --- */
#define RECIPE_PATH     "../lib/world/rcp/"
#define CS_FILE         "../lib/misc/crskills.dat"
#define MAX_RECIPE_COMPONENTS 5
#define MAX_LEGACY_RECIPES    10

/* --- Enumerações --- */
enum item_quality {
    QUALITY_COMUM,
    QUALITY_INCOMUM,
    QUALITY_RARO,
    QUALITY_EPICO,
    QUALITY_LENDARIO,
    QUALITY_OBRA_PRIMA,
    QUALITY_DIVINA,
    NUM_QUALITIES
};

/* --- Estruturas de Dados --- */
// A struct craft_skill_data foi REMOVIDA daqui, pois agora vem de crskill.h

struct recipe_component_data {
    obj_vnum vnum;
    int quality_points;
};

struct recipe_data {
    int vnum;
    char *name;
    char *enigma_name;
    obj_vnum result_vnum;
    int result_quantity;
    int crskill_id;
    int difficulty;
    obj_vnum station_vnum;
    bool is_master_recipe;
    struct recipe_component_data components[MAX_RECIPE_COMPONENTS];
    struct recipe_data *next;
};

/* --- Variáveis Globais --- */
extern struct recipe_data *recipe_list; 
extern const char *quality_names[NUM_QUALITIES];

/* --- Protótipos de Funções --- */
void load_recipes(void);
void free_recipes(void);
void load_crafting_skills(void);
void free_crafting_skills(void);

int calculate_quality(struct char_data *ch, int crskill_id, int difficulty, int material_bonus);
void apply_quality_to_obj(struct obj_data *obj, int quality);
int get_quality_value(struct obj_data *obj);

ACMD(do_gather);
ACMD(do_craft);
ACMD(do_legacy);

#endif
