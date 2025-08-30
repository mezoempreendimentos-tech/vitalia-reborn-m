#ifndef _CRSKILL_H_
#define _CRSKILL_H_

// Define o número máximo de perícias de crafting que o sistema suportará.
#define MAX_CRAFT_SKILLS      50

// Estrutura que define uma única perícia de crafting.
struct craft_skill_data {
    const char *name;
};

// Declaração do array global que conterá as definições das perícias.
extern struct craft_skill_data craft_skills[];

// Protótipo da função que irá preencher o array acima com as perícias.
void assign_craft_skills(void);

#endif
