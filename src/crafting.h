#ifndef _CRAFTING_H_
#define _CRAFTING_H_
#include "crskill.h"
/* --- Constantes e Definições --- */

// Arquivos de dados para o sistema de crafting
#define RECIPE_PATH     "../lib/world/rcp/" // Define o caminho para a pasta de receitas
#define CS_FILE         "../lib/misc/crskills.dat" // Nosso novo arquivo de perícias de crafting

// Limites do sistema
#define MAX_RECIPE_COMPONENTS 5
#define MAX_LEGACY_RECIPES    10 // CORRIGIDO: Aumentado para 10


/* --- Enumerações --- */

// Níveis de Qualidade para itens
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

// Estrutura para uma perícia de crafting
struct craft_skill_data {
    int id;
    char *name;
    struct craft_skill_data *next;
};

// Estrutura para um componente de receita
struct recipe_component_data {
    obj_vnum vnum;
    int quality_points;
};

// Estrutura para uma receita completa
struct recipe_data {
    int vnum;
    char *name;
    char *enigma_name;
    obj_vnum result_vnum;
    int result_quantity;
    int crskill_id;           // CORRIGIDO: Renomeado de skill_id para crskill_id
    int difficulty;
    obj_vnum station_vnum;
    bool is_master_recipe;
    
    struct recipe_component_data components[MAX_RECIPE_COMPONENTS];
    struct recipe_data *next;
};


/* --- Variáveis Globais (declaradas em crafting.c) --- */

extern struct recipe_data *recipe_list; 
extern const char *quality_names[NUM_QUALITIES];
extern struct craft_skill_data *craft_skill_list;

/* --- Protótipos de Funções --- */

/* Funções de Carregamento e Liberação */
void load_recipes(void);
void free_recipes(void);
void load_crafting_skills(void);
void free_crafting_skills(void);

/* Funções de Lógica de Qualidade */
// CORRIGIDO: Parâmetro renomeado para crskill_id
int calculate_quality(struct char_data *ch, int crskill_id, int difficulty, int material_bonus);
void apply_quality_to_obj(struct obj_data *obj, int quality);
int get_quality_value(struct obj_data *obj);

/* Comandos do Jogador (ACMDs) */
ACMD(do_gather);
ACMD(do_craft);
ACMD(do_legacy);

#endif // _CRAFTING_H_
