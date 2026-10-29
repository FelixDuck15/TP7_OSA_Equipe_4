/*
 * GIF-1001 — TP7 : Calculatrice en assembleur sur l'Arduino Nano 33 BLE
 *
 * Fonctionnement
 *   1. Le programme lit une ligne au moniteur série (115 200 bauds),
 *      par exemple « 25 + 17 » ou « -12 * 7 ». Faites attention à l'espace entre l'opérateur et les opérandes. Si vous saisissez 25+17 il doit vous afficher une erreur et la led doit s'allumer en rouge
 *   2. Il découpe la ligne en trois éléments : opérande 1, opérateur, opérande 2.
 *   3. Il appelle vos fonctions en assembleur pour convertir, valider et calculer.
 *   4. La LED clignote en bleu pendant 15 s (période de 1 s), puis reste
 *      verte si le calcul a réussi, ou rouge en cas d'erreur.
 *
* Vous devez :
 *   Compléter chaque fonction 
 *   Ne pas modifier la section « PROGRAMME PRINCIPAL ».
 *
 * Règles
 *   - Chaque fonction est écrite entièrement en assembleur dans le bloc asm.
 *   - Paramètres dans R0 à R3, valeur de retour dans R0, retour par BX LR.
 *   - Instructions et fonctions interdites dans vos fonctions :
 *     MUL, MLA, SDIV, UDIV, digitalWrite, pinMode, delay, millis.
 *   - Ne pas retirer la ligne « .ltorg » à la fin de chaque bloc asm :
 *     elle place les adresses chargées par LDR Rd, =variable près du code.
 *   - Compléter l'en-tête de chaque fonction : membre responsable,
 *     paramètres et valeur de retour.
 */

#include <Arduino.h>

/* ===================================================================
 * VARIABLES EN MÉMOIRE
 * Accessibles en assembleur par :  LDR Rx, =nom_de_la_variable
 * =================================================================== */

volatile char tampon[32];            // ligne saisie au moniteur série
volatile char texte_operande1[12];   // premier opérande, en texte
volatile char texte_operande2[12];   // second opérande, en texte
volatile char texte_resultat[12];    // résultat converti en texte (A2)
volatile int  saisie_invalide;       // 1 si un opérande est mal formé (A1)

/* Codes d'erreur retournés par valider() (critère C1) */
#define OK                   0
#define ERR_DIVISION_ZERO    1
#define ERR_HORS_BORNES      2
#define ERR_OPERATEUR        3
#define ERR_SAISIE           4

/* Couleurs passées à led_allumer() (critère F1) */
#define ROUGE  0
#define VERT   1
#define BLEU   2

#define PO_DIRSET "0x50000518"
#define PO_OUTSET "0x50000508"
#define PO_OUTCLR "0x5000050C"

/* ===================================================================
 * FONCTIONS EN ASSEMBLEUR — À COMPLÉTER
 * =================================================================== */

/* -------------------------------------------------------------------
 * A1. Conversion ASCII vers entier
 * Responsable : ...
 * R0 (entrée) : adresse d'un texte terminé par 0, par exemple "-45"
 * R0 (sortie) : valeur entière signée
 * Si le texte est vide ou contient un caractère autre qu'un chiffre
 * (après un « - » facultatif), écrire 1 dans saisie_invalide.
 * ------------------------------------------------------------------- */
extern "C" __attribute__((naked)) int convertir_ascii(volatile char *texte) {
  asm volatile(
    "MOV   R0, #0          \n"    // à remplacer
    "BX    LR              \n"    // Retourne de la fonction
    ".ltorg                \n"    // Indispensable pour stocker la constante 0x5000050C en mémoire
  );
}

/* -------------------------------------------------------------------
 * A2. Conversion entier vers ASCII
 * Responsable : ...
 * R0 (entrée) : valeur entière signée
 * R1 (entrée) : adresse du texte de sortie (texte_resultat)
 * Écrit le texte terminé par 0, par exemple "-84".
 * ------------------------------------------------------------------- */
extern "C" __attribute__((naked)) void convertir_entier(int valeur, volatile char *sortie) {
  asm volatile(
    "BX    LR              \n"   // à compléter
    ".ltorg                \n"
  );
}

/* -------------------------------------------------------------------
 * B1. Addition
 * Responsable : ...
 * R0, R1 (entrée) : opérandes ; R0 (sortie) : R0 + R1
 * ------------------------------------------------------------------- */
extern "C" __attribute__((naked)) int addition(int a, int b) {
  asm volatile(
    "add r0, r0, r1 \n"    // Addition de "a" et "b" dans la "a"
    "bx lr \n"   // à compléter
    ".ltorg \n"
  );
}

/* -------------------------------------------------------------------
 * B2. Soustraction
 * Responsable : ...
 * R0, R1 (entrée) : opérandes ; R0 (sortie) : R0 − R1
 * ------------------------------------------------------------------- */
extern "C" __attribute__((naked)) int soustraction(int a, int b) {
  asm volatile(
    "BX    LR              \n"   // à compléter
    ".ltorg                \n"
  );
}

/* -------------------------------------------------------------------
 * B3. Multiplication (MUL et MLA interdites)
 * Responsable : Hugo Savard
 * R0, R1 (entrée) : opérandes ; R0 (sortie) : R0 × R1
 * ------------------------------------------------------------------- */
extern "C" __attribute__((naked)) int multiplication(int a, int b) {
  asm volatile(
    "PUSH {R4, R5, LR} \n"


    "EOR R5, R0, R1 \n" // ou exclusif. si le bit de signe est le meme -> + 
                        // sinon -> -

    //Valeur absolu de R0
    "CMP R0, #0 \n"
    "RSBLT R0, R0, #0 \n" //R0 = -R0 si R0 < 0

    //Valeur absolu de R1
    "CMP R1, #0 \n"
    "RSBLT R1, R1, #0 \n" //R1 = -R1 si R1 < 0


    "MOV R4, #0 \n" //R4 = 0

    // boucle principale pour la multiplication
    "boucle_mul: \n" 
    "CMP R1, #0 \n" //Condition pour mettre fin a la boucle R1 == 0
    "BEQ fin_mul \n"
    "ADD R4, R4, R0 \n" // on ajoute R0 dans R4, R1 fois (R4 = R1 * R2)

    "SUB R1, R1, #1 \n" // R1 = R1 - 1
    "B boucle_mul \n"

    "fin_mul: \n"


    "MOV R0, R4 \n" // R0 = R4 (R0, variable de retour)

    "CMP R5, #0 \n" // le signe de R5 sera le signe de la réponse (car EOR)
    "BGE retour \n" 
    "RSB R0, R0, #0 \n"


    "retour \n"
    "POP {R4, R5, LR} \n"
    "BX    LR              \n"   // à compléter
    ".ltorg                \n"
  );
}

/* -------------------------------------------------------------------
 * B4. Division (SDIV et UDIV interdites)
 * Responsable : ...
 * R0, R1 (entrée) : dividende, diviseur (non nul)
 * R0 (sortie) : quotient tronqué vers zéro (-7 / 2 = -3)
 * ------------------------------------------------------------------- */
extern "C" __attribute__((naked)) int division(int a, int b) {
  asm volatile(
    "BX    LR              \n"   // à compléter
    ".ltorg                \n"
  );
}

/* -------------------------------------------------------------------
 * C1. Validation et exceptions
 * Responsable : ...
 * R0 (entrée) : opérande 1
 * R1 (entrée) : code ASCII de l'opérateur ('+', '-', '*', '/'),
 *               ou 0 si l'opérateur n'a pas exactement un caractère
 * R2 (entrée) : opérande 2
 * R0 (sortie) : OK (0) ou un code d'erreur, dans cet ordre de priorité :
 *               ERR_SAISIE (4)        si saisie_invalide vaut 1
 *               ERR_OPERATEUR (3)     si l'opérateur est inconnu
 *               ERR_HORS_BORNES (2)   si un opérande est hors de [-100, 100]
 *               ERR_DIVISION_ZERO (1) si l'opérateur est '/' et R2 = 0
 * ------------------------------------------------------------------- */
extern "C" __attribute__((naked)) int valider(int a, int operateur, int b) {
  asm volatile(
    "MOV   R0, #0          \n"   // à remplacer
    "BX    LR              \n"
    ".ltorg                \n"
  );
}

/* -------------------------------------------------------------------
 * F1. LED par MMIO — initialisation
 * Responsable : ...
 * Configure en sortie les trois broches de la LED RGB (registre DIRSET)
 * et éteint la LED.
 * ------------------------------------------------------------------- */
extern "C" __attribute__((naked)) void led_init(void) {
  asm volatile(
    "BX    LR              \n"   // à compléter
    ".ltorg                \n"
  );
}

/* -------------------------------------------------------------------
 * F1. LED par MMIO — allumer une couleur
 * Responsable : Félix-Antoine Guimont
 * R0 (entrée) : couleur (ROUGE = 0, VERT = 1, BLEU = 2)
 * Éteint les deux autres couleurs et allume celle qui est demandée.
 * ------------------------------------------------------------------- */
extern "C" __attribute__((naked)) void led_allumer(int couleur) {
  asm volatile(
      "ldr r1, =" PO_OUTCLR " \n"   // Charge l'adresse matérielle de ADR_OUTCLR dans le registre r1
      "str r0, [r1] \n"             // Écrit la valeur 'couleur' (r0) à l'adresse pointée par r1
      "bx lr \n"                    // Retourne de la fonction
      ".ltorg \n"                   // Indispensable pour stocker la constante 0x5000050C en mémoire
  );
}

/* -------------------------------------------------------------------
 * F1. LED par MMIO — tout éteindre
 * Responsable : Félix-Antoine Guimont
 * ------------------------------------------------------------------- */
extern "C" __attribute__((naked)) void led_eteindre(void) {
  asm volatile(
    "ldr r1, =" PO_OUTSET " \n"   // Charge l'adresse matérielle de ADR_OUTCLR dans le registre r1 
    "str r0, [r1] \n"             // Écrit la valeur 'couleur' (r0) à l'adresse pointée par r1
    "bx lr \n"                    // Retourne de la fonction
    ".ltorg \n"                   // Indispensable pour stocker la constante 0x5000050C en mémoire
  );
}

/* -------------------------------------------------------------------
 * H. Fonctions originales de l'équipe
 * Ajouter ici vos propres fonctions et les appeler depuis
 * executer_fonction_originale(), plus bas.
 * ------------------------------------------------------------------- */


/* ===================================================================
 * PROGRAMME PRINCIPAL — NE PAS MODIFIER
 * (sauf executer_fonction_originale pour le critère H)
 * =================================================================== */

static void lire_ligne(void) {
  int n = 0;
  while (true) {
    while (!Serial.available()) { }
    char c = Serial.read();
    if (c == '\r') continue;
    if (c == '\n') break;
    if (n < (int)sizeof(tampon) - 1) tampon[n++] = c;
  }
  tampon[n] = 0;
}

/* Copie le prochain mot de tampon (à partir de *pos) dans dest. */
static void extraire_mot(int *pos, volatile char *dest, int taille) {
  int n = 0;
  while (tampon[*pos] == ' ') (*pos)++;
  while (tampon[*pos] != 0 && tampon[*pos] != ' ') {
    if (n < taille - 1) dest[n++] = tampon[*pos];
    (*pos)++;
  }
  dest[n] = 0;
}

static void phase_bleue(void) {
  for (int i = 0; i < 15; i++) {     // 15 clignotements de période 1 s
    led_allumer(BLEU);
    delay(500);
    led_eteindre();
    delay(500);
  }
}

/* Critère H : appeler ici vos fonctions originales, au besoin. */
static void executer_fonction_originale(void) {
}

void setup() {
  Serial.begin(115200);
  while (!Serial) { }
  led_init();
  Serial.println("Calculatrice GIF-1001 : entrer par exemple  25 + 17");
}

void loop() {
  volatile char texte_operateur[4];
  int pos = 0;

  lire_ligne();
  Serial.print("> ");
  Serial.println((const char *)tampon);

  extraire_mot(&pos, texte_operande1, sizeof(texte_operande1));
  extraire_mot(&pos, texte_operateur, sizeof(texte_operateur));
  extraire_mot(&pos, texte_operande2, sizeof(texte_operande2));

  saisie_invalide = 0;
  int a = convertir_ascii(texte_operande1);
  int b = convertir_ascii(texte_operande2);
  int op = (texte_operateur[1] == 0) ? texte_operateur[0] : 0;

  int code = valider(a, op, b);
  int r = 0;
  if (code == OK) {
    switch (op) {
      case '+': r = addition(a, b);       break;
      case '-': r = soustraction(a, b);   break;
      case '*': r = multiplication(a, b); break;
      case '/': r = division(a, b);       break;
    }
  }

  phase_bleue();

  if (code == OK) {
    convertir_entier(r, texte_resultat);
    Serial.print("= ");
    Serial.println((const char *)texte_resultat);
    led_allumer(VERT);
  } else {
    Serial.print("Erreur ");
    Serial.println(code);
    led_allumer(ROUGE);
  }

  executer_fonction_originale();
}
