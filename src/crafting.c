/**************************************************************************
 * File: crafting.c                                Part of tbaMUD         *
 * Usage: Core logic for the Gathering and Crafting system.               *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h" // Necessário para a macro ACMD
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "constants.h"
#include "act.h"
#include "crafting.h"

/* --- Variáveis Globais --- */

struct recipe_data *recipe_list = NULL;
struct craft_skill_data *craft_skill_list = NULL;

const char *quality_names[NUM_QUALITIES] = {
    "§w[Comum]§n",
    "§g[Incomum]§n",
    "§b[Raro]§n",
    "§m[Épico]§n",
    "§Y[Lendário]§n",
    "§C[Obra Prima]§n",
    "§R[DIVINA]§n"
};


/* --- Funções de Carregamento --- */

void load_recipes(void) {
    FILE *fp;
    char line[256], tag[128], filename[256];
    struct recipe_data *current_recipe = NULL;
    int component_index = 0;
    int i;
    int total_recipes_loaded = 0;

    log1("...Carregando receitas de forma modular do diretório %s", RECIPE_PATH);

    for (i = 1; i <= MAX_CRAFT_SKILLS; i++) {
        snprintf(filename, sizeof(filename), "%s%d.rcp", RECIPE_PATH, i);
        if (!(fp = fopen(filename, "r"))) {
            continue;
        }
        while (get_line(fp, line)) {
            tag_argument(line, tag);

            if (str_cmp(tag, "Recipe") == 0) {
                CREATE(current_recipe, struct recipe_data, 1);
                component_index = 0;
                current_recipe->vnum = atoi(line);
                current_recipe->station_vnum = -1;
                current_recipe->enigma_name = NULL;
                current_recipe->is_master_recipe = FALSE;
                current_recipe->next = recipe_list;
                recipe_list = current_recipe;
                total_recipes_loaded++;
            } else if (current_recipe) {
                if (str_cmp(tag, "Name") == 0) current_recipe->name = strdup(line);
                else if (str_cmp(tag, "Enigma") == 0) current_recipe->enigma_name = strdup(line);
                else if (str_cmp(tag, "ResultVnum") == 0) current_recipe->result_vnum = atoi(line);
                else if (str_cmp(tag, "ResultQty") == 0) current_recipe->result_quantity = atoi(line);
                else if (str_cmp(tag, "CrSkill") == 0) current_recipe->crskill_id = atoi(line);
                else if (str_cmp(tag, "Difficulty") == 0) current_recipe->difficulty = atoi(line);
                else if (str_cmp(tag, "Station") == 0) current_recipe->station_vnum = atoi(line);
                else if (str_cmp(tag, "Master") == 0) current_recipe->is_master_recipe = (atoi(line) == 1);
                else if (str_cmp(tag, "Component") == 0) {
                    if (component_index < MAX_RECIPE_COMPONENTS) {
                        sscanf(line, "%hd %d", 
                            &current_recipe->components[component_index].vnum, 
                            &current_recipe->components[component_index].quality_points);
                        component_index++;
                    }
                }
            }
        }
        fclose(fp);
    }
    log1("...%d receitas carregadas de múltiplos arquivos.", total_recipes_loaded);
}

void free_recipes(void) {
    struct recipe_data *recipe, *next_recipe;
    for (recipe = recipe_list; recipe; recipe = next_recipe) {
        next_recipe = recipe->next;
        if (recipe->name) free(recipe->name);
        if (recipe->enigma_name) free(recipe->enigma_name);
        free(recipe);
    }
}

void load_crafting_skills(void) {
    FILE *fp;
    char line[256];
    
    if (!(fp = fopen(CS_FILE, "r"))) {
        log1("SYSERR: Não foi possível abrir o arquivo de perícias de crafting: %s", CS_FILE);
        return;
    }
    log1("...Carregando perícias de crafting de %s", CS_FILE);

    while(get_line(fp, line)) {
        if (*line == '$') break;
        struct craft_skill_data *skill;
        CREATE(skill, struct craft_skill_data, 1);
        skill->id = atoi(strtok(line, "~"));
        skill->name = strdup(strtok(NULL, "~"));
        skill->next = craft_skill_list;
        craft_skill_list = skill;
    }
    fclose(fp);
}

void free_crafting_skills(void) {
    struct craft_skill_data *skill, *next_skill;
    for (skill = craft_skill_list; skill; skill = next_skill) {
        next_skill = skill->next;
        if (skill->name) free(skill->name);
        free(skill);
    }
}

/* --- Funções de Lógica de Qualidade --- */
int calculate_quality(struct char_data *ch, int crskill_id, int difficulty, int material_bonus) {
    if (rand_number(1, 100) == 1) return QUALITY_OBRA_PRIMA;
    int roll = dice(1, 20);
    if (roll == 20) return QUALITY_LENDARIO;
    if (roll == 1) return QUALITY_COMUM;
    int skill_bonus = (GET_CRAFT_SKILL(ch, crskill_id) - difficulty) / 5;
    int final_result = roll + skill_bonus + material_bonus;
    if (final_result <= 5) return QUALITY_COMUM;
    if (final_result <= 10) return QUALITY_INCOMUM;
    if (final_result <= 16) return QUALITY_RARO;
    if (final_result <= 24) return QUALITY_EPICO;
    return QUALITY_EPICO;
}

void apply_quality_to_obj(struct obj_data *obj, int quality) {
    char buf[MAX_STRING_LENGTH];
    if (quality < QUALITY_COMUM || quality >= NUM_QUALITIES) return;
    GET_OBJ_QUALITY(obj) = quality;
    snprintf(buf, sizeof(buf), "%s %s", obj->short_description, quality_names[quality]);
    if (obj->short_description) free(obj->short_description);
    obj->short_description = strdup(buf);
    snprintf(buf, sizeof(buf), "%s jaz aqui, com um brilho %s.", obj->description, quality_names[quality]);
    if (obj->description) free(obj->description);
    obj->description = strdup(buf);
}

int get_quality_value(struct obj_data *obj) {
    switch (GET_OBJ_QUALITY(obj)) {
        case QUALITY_INCOMUM: return 2;
        case QUALITY_RARO:    return 3;
        case QUALITY_EPICO:   return 5;
        case QUALITY_LENDARIO:return 8;
        case QUALITY_OBRA_PRIMA: return 12;
        case QUALITY_DIVINA:     return 20;
        default: return 1;
    }
}

/* --- Comandos do Jogador (DEFINIÇÕES) --- */

// A DEFINIÇÃO de do_gather, do_craft, e do_legacy.
// É isso que estava faltando e causando o erro de "undefined reference".
ACMD(do_gather) {
    send_to_char(ch, "O sistema de coleta ainda está sendo desenvolvido.\r\n");
}

ACMD(do_craft)
{
    char arg[MAX_INPUT_LENGTH];
    struct recipe_data *recipe;
    struct obj_data *comp_list[MAX_RECIPE_COMPONENTS][100]; // Matriz para agrupar componentes por tipo
    int comp_count[MAX_RECIPE_COMPONENTS] = {0};
    int total_quality_pts[MAX_RECIPE_COMPONENTS] = {0};
    int material_bonus = 0;
    bool found = FALSE;
    int i, j, k;

    one_argument(argument, arg);

    if (!*arg) {
        send_to_char(ch, "Produzir o quê? (Ex: produzir 'Adaga de Ferro')\r\n");
        return;
    }

    // 1. Encontrar a receita pelo nome
    for (recipe = recipe_list; recipe; recipe = recipe->next) {
        if (is_abbrev(arg, recipe->name)) {
            found = TRUE;
            break;
        }
    }

    if (!found) {
        send_to_char(ch, "Você não conhece essa receita.\r\n");
        return;
    }

    // 2. Verificar perícia do jogador
    if (GET_CRAFT_SKILL(ch, recipe->crskill_id) <= 0) {
        send_to_char(ch, "Você não tem a perícia necessária para produzir isso.\r\n");
        return;
    }

    // 3. Verificar estação de trabalho
    if (recipe->station_vnum > 0) {
        bool station_found = FALSE;
        struct obj_data *obj;
        for (obj = world[IN_ROOM(ch)].contents; obj; obj = obj->next_content) {
            if (GET_OBJ_VNUM(obj) == recipe->station_vnum) {
                station_found = TRUE;
                break;
            }
        }
        if (!station_found) {
            send_to_char(ch, "Você precisa estar em uma estação de trabalho adequada para produzir isso.\r\n");
            return;
        }
    }

    // 4. Verificar e agrupar componentes
    for (i = 0; i < MAX_RECIPE_COMPONENTS; i++) {
        obj_vnum vnum_needed = recipe->components[i].vnum;
        if (vnum_needed <= 0) continue;

        struct obj_data *comp;
        for (comp = ch->carrying; comp; comp = comp->next_content) {
            if (GET_OBJ_VNUM(comp) == vnum_needed) {
                if (comp_count[i] < 100) {
                    comp_list[i][comp_count[i]++] = comp;
                    total_quality_pts[i] += get_quality_value(comp);
                }
            }
        }

        // Verifica se tem pontos de qualidade suficientes para este componente
        if (total_quality_pts[i] < recipe->components[i].quality_points) {
            send_to_char(ch, "Você não tem os componentes necessários (faltam materiais para o item de VNUM %d).\r\n", vnum_needed);
            return;
        }
    }

    // Se todas as verificações passaram, podemos consumir os itens
    send_to_char(ch, "Você começa a trabalhar na sua criação...\r\n");
    act("$n começa a trabalhar em um item.", TRUE, ch, 0, 0, TO_ROOM);

    // 5. Consumir componentes e calcular bônus de material
    for (i = 0; i < MAX_RECIPE_COMPONENTS; i++) {
        if (recipe->components[i].vnum <= 0) continue;

        int points_needed = recipe->components[i].quality_points;
        int points_consumed = 0;

        // Ordenar os componentes por qualidade (menor para maior) para consumir os piores primeiro
        for (j = 0; j < comp_count[i] - 1; j++) {
            for (k = j + 1; k < comp_count[i]; k++) {
                if (GET_OBJ_QUALITY(comp_list[i][j]) > GET_OBJ_QUALITY(comp_list[i][k])) {
                    struct obj_data *temp = comp_list[i][j];
                    comp_list[i][j] = comp_list[i][k];
                    comp_list[i][k] = temp;
                }
            }
        }
        
        // Consumir até atingir os pontos
        for (j = 0; j < comp_count[i]; j++) {
            if (points_consumed >= points_needed) break;
            
            struct obj_data *comp_to_use = comp_list[i][j];
            int q_value = get_quality_value(comp_to_use);
            points_consumed += q_value;
            material_bonus += q_value; // Adiciona ao bônus total
            extract_obj(comp_to_use);
        }
    }

    // 6. Calcular a qualidade final do item
    int final_quality = calculate_quality(ch, recipe->crskill_id, recipe->difficulty, material_bonus / 5); // Bônus de material escalado

    // 7. Criar o item resultante
    obj_rnum rnum = real_object(recipe->result_vnum);
    if (rnum == NOTHING) {
        send_to_char(ch, "Erro: A receita está corrompida. Avise um Imortal.\r\n");
        log1("SYSERR: do_craft: Receita %d aponta para vnum de resultado inválido %d.", recipe->vnum, recipe->result_vnum);
        return;
    }
    
    struct obj_data *result_obj = read_object(rnum, REAL);

    // 8. Aplicar qualidade ao item
    apply_quality_to_obj(result_obj, final_quality);
    
    // 9. Entregar o item ao jogador
    obj_to_char(result_obj, ch);

    send_to_char(ch, "Você produziu com sucesso %s!\r\n", result_obj->short_description);
    act("$n termina de produzir $p.", TRUE, ch, result_obj, 0, TO_ROOM);
}

ACMD(do_legacy) {
    send_to_char(ch, "O sistema de Legado ainda está sendo desenvolvido.\r\n");
}

ACMD(do_cset); // <-- ADICIONE ESTA LINHA AQUI
