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
#include <ArduinoBLE.h>

/* ===================================================================
 * VARIABLES EN MÉMOIRE
 * Accessibles en assembleur par :  LDR Rx, =nom_de_la_variable
 * =================================================================== */

volatile char tampon[32];            // ligne saisie au moniteur série
volatile char texte_operande1[12];   // premier opérande, en texte
volatile char texte_operande2[12];   // second opérande, en texte
volatile char texte_resultat[12];    // résultat converti en texte (A2)
volatile int  saisie_invalide;       // 1 si un opérande est mal formé (A1)
volatile int  dernier_code;           // dernier code retourné par valider()

/* Codes d'erreur retournés par valider() (critère C1) */
#define OK                   0
#define ERR_DIVISION_ZERO    1
#define ERR_HORS_BORNES      2
#define ERR_OPERATEUR        3
#define ERR_SAISIE           4

/* Couleurs passées à led_allumer() (critère F1) */
#define ROUGE  (1UL << 24)
#define VERT   (1UL << 16)
#define BLEU   (1UL << 6)

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
    "PUSH  {R4}              \n"
    "MOVS  R1, #0            \n"   // R1 = résultat
    "MOVS  R3, #10           \n"   // R3 = 10
    "LDRB  R4, [R0]          \n"   // R4 = 1er caractère (sert à savoir si c'est '-')
    "CMP   R4, #45           \n"
    "BNE   debut             \n"
    "ADDS  R0, R0, #1        \n"   // on saute le '-'
    "debut:                  \n"
    "LDRB  R2, [R0]          \n"
    "CMP   R2, #0            \n"
    "BEQ   invalide          \n"   // vide ou "-" seul
    "boucle:                 \n"
    "SUBS  R2, R2, #48       \n"   // caractère -> chiffre
    "CMP   R2, #9            \n"
    "BHI   invalide          \n"   // pas un chiffre
    "MULS  R1, R3, R1        \n"   // résultat *= 10
    "ADDS  R1, R1, R2        \n"   // résultat += chiffre
    "ADDS  R0, R0, #1        \n"
    "LDRB  R2, [R0]          \n"
    "CMP   R2, #0            \n"
    "BNE   boucle            \n"
    "CMP   R4, #45           \n"   // le texte commençait par '-' ?
    "BNE   fin               \n"
    "RSBS  R1, R1, #0        \n"   // oui : résultat = -résultat
    "fin:                    \n"
    "MOVS  R0, R1            \n"
    "POP   {R4}              \n"
    "BX    LR                \n"
    "invalide:               \n"
    "LDR   R3, =0x5000050C   \n"
    "MOVS  R2, #1            \n"
    "STR   R2, [R3]          \n"   // saisie_invalide = 1
    "MOVS  R1, #0            \n"
    "B     fin               \n"   // on réutilise la sortie normale
    ".ltorg                  \n"
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
    "CMP   R0, #0            \n"
    "BGE   ent_positif       \n"
    "MOVS  R2, #45           \n"   // '-'
    "STRB  R2, [R1]          \n"
    "ADDS  R1, R1, #1        \n"
    "RSBS  R0, R0, #0        \n"   // valeur = -valeur
    "ent_positif:            \n"
    "MOVS  R3, #0            \n"   // R3 = nombre de chiffres empilés
    "ent_boucle:             \n"   // do { ... } while (valeur != 0)
    "MOVS  R2, #0            \n"   // R2 = quotient de valeur / 10
    "ent_div:                \n"
    "CMP   R0, #10           \n"
    "BLT   ent_reste         \n"
    "SUBS  R0, R0, #10       \n"   // division par 10 par soustractions
    "ADDS  R2, R2, #1        \n"
    "B     ent_div           \n"
    "ent_reste:              \n"   // R0 = reste (0 à 9) = dernier chiffre
    "PUSH  {R0}              \n"   // les chiffres sortent à l'envers : on les empile
    "ADDS  R3, R3, #1        \n"
    "MOVS  R0, R2            \n"   // valeur = quotient
    "CMP   R0, #0            \n"
    "BNE   ent_boucle        \n"
    "ent_ecrire:             \n"   // on dépile : le plus significatif sort en premier
    "POP   {R0}              \n"
    "ADDS  R0, R0, #48       \n"   // chiffre -> caractère ASCII
    "STRB  R0, [R1]          \n"
    "ADDS  R1, R1, #1        \n"
    "SUBS  R3, R3, #1        \n"
    "BNE   ent_ecrire        \n"
    "MOVS  R0, #0            \n"
    "STRB  R0, [R1]          \n"   // terminateur 0
    "BX    LR                \n"
    ".ltorg                  \n"
  );
}

/* -------------------------------------------------------------------
 * B1. Addition
 * Responsable : Félix-Antoine Guimont
 * R0, R1 (entrée) : opérandes ; R0 (sortie) : R0 + R1
 * ------------------------------------------------------------------- */
extern "C" __attribute__((naked)) int addition(int a, int b) {
  asm volatile(
    "add r0, r0, r1 \n"    // Addition de "a" et "b" dans la "a"
    "BX LR \n"
    ".ltorg \n"
  );
}

/* -------------------------------------------------------------------
 * B2. Soustraction
 * Responsable : Hugo Savard
 * R0, R1 (entrée) : opérandes ; R0 (sortie) : R0 − R1
 * ------------------------------------------------------------------- */
extern "C" __attribute__((naked)) int soustraction(int a, int b) {
  asm volatile(
    "SUB R0 , R0, R1       \n"
    "BX    LR              \n"
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
    "PUSH {R4, R5}         \n"  //Modif du LR, pas obligatoire on ne lui touche pas
    "EOR  R5, R0, R1       \n"   // bit 31 de R5 = signe du résultat

    //Valeur absolu de R0
    "CMP  R0, #0           \n"
    "BGE  mul_a_pos        \n"
    "RSBS R0, R0, #0       \n"   // |a|
    "mul_a_pos:            \n"

    //Valeur absolu de R1
    "CMP  R1, #0           \n"
    "BGE  mul_b_pos        \n"
    "RSBS R1, R1, #0       \n"   // |b|
    "mul_b_pos:            \n"

    //R4 = 0
    "MOVS R4, #0           \n"

    // boucle principale pour la multiplication
    "boucle_mul:           \n"
    "CMP  R1, #0           \n"  //Condition pour mettre fin a la boucle R1 == 0
    "BEQ  fin_mul          \n"
    "ADDS R4, R4, R0       \n"  // on ajoute R0 dans R4, R1 fois (R4 = R1 * R2)

    "SUBS R1, R1, #1       \n"  // R1 = R1 - 1
    "B    boucle_mul       \n"
    "fin_mul:              \n"

    "MOVS R0, R4           \n"  // R0 = R4 (R0, variable de retour)

    "CMP  R5, #0           \n"  // le signe de R5 sera le signe de la réponse (car EOR)
    "BGE  retour           \n"
    "RSBS R0, R0, #0       \n"

    "retour:               \n"
    "POP  {R4, R5}         \n"
    "BX   LR               \n"
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
    "EOR   R3, R0, R1        \n"   // bit 31 de R3 = signe du quotient
    "CMP   R0, #0            \n"
    "BGE   div_a_pos         \n"
    "RSBS  R0, R0, #0        \n"   // |a|
    "div_a_pos:              \n"
    "CMP   R1, #0            \n"
    "BGE   div_b_pos         \n"
    "RSBS  R1, R1, #0        \n"   // |b|
    "div_b_pos:              \n"
    "MOVS  R2, #0            \n"   // R2 = quotient
    "div_boucle:             \n"
    "CMP   R0, R1            \n"
    "BLT   div_fin           \n"   // reste < diviseur : terminé
    "SUBS  R0, R0, R1        \n"   // on retire le diviseur
    "ADDS  R2, R2, #1        \n"   // un de plus au quotient
    "B     div_boucle        \n"
    "div_fin:                \n"
    "CMP   R3, #0            \n"
    "BGE   div_retour        \n"
    "RSBS  R2, R2, #0        \n"   // signes différents : quotient négatif
    "div_retour:             \n"
    "MOVS  R0, R2            \n"
    "BX    LR                \n"
    ".ltorg                  \n"
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
    // 1) saisie invalide ? (priorité la plus haute)
    "LDR   R3, =saisie_invalide \n"
    "LDR   R3, [R3]          \n"
    "CMP   R3, #0            \n"
    "BNE   val_err_saisie    \n"
    // 2) opérateur connu ? ( '+'=43  '-'=45  '*'=42  '/'=47 )
    "CMP   R1, #43           \n"
    "BEQ   val_op_ok         \n"
    "CMP   R1, #45           \n"
    "BEQ   val_op_ok         \n"
    "CMP   R1, #42           \n"
    "BEQ   val_op_ok         \n"
    "CMP   R1, #47           \n"
    "BEQ   val_op_ok         \n"
    "B     val_err_op        \n"
    "val_op_ok:              \n"
    // 3) bornes : x dans [-100,100]  <=>  (x+100) dans [0,200] en non signé
    "ADDS  R3, R0, #100      \n"
    "CMP   R3, #200          \n"
    "BHI   val_err_bornes    \n"
    "ADDS  R3, R2, #100      \n"
    "CMP   R3, #200          \n"
    "BHI   val_err_bornes    \n"
    // 4) division par zéro
    "CMP   R1, #47           \n"
    "BNE   val_ok            \n"
    "CMP   R2, #0            \n"
    "BEQ   val_err_zero      \n"
    "val_ok:                 \n"
    "MOVS  R0, #0            \n"
    "B     val_sortie        \n"
    "val_err_zero:           \n"
    "MOVS  R0, #1            \n"
    "B     val_sortie        \n"
    "val_err_bornes:         \n"
    "MOVS  R0, #2            \n"
    "B     val_sortie        \n"
    "val_err_op:             \n"
    "MOVS  R0, #3            \n"
    "B     val_sortie        \n"
    "val_err_saisie:         \n"
    "MOVS  R0, #4            \n"
    "val_sortie:             \n"
    "LDR   R1, =dernier_code \n"
    "STR   R0, [R1]          \n"   // on sauvegarde le code
    "BX    LR                \n"
    ".ltorg                  \n"
  );
}

/* -------------------------------------------------------------------
 * F1. LED par MMIO — initialisation
 * Responsable : Félix-Antoine Guimont
 * Configure en sortie les trois broches de la LED RGB (registre DIRSET)
 * et éteint la LED.
 * ------------------------------------------------------------------- */
extern "C" __attribute__((naked)) void led_init(void) {
  asm volatile(
      "LDR r0, =0x50000518    \n"  // Charge l'adresse du registre P0_DIRSET dans R0
      "LDR r1, =0x01010040    \n"  // Masque combiné pour les broches 24, 16 et 6
      "STR R1, [R0]           \n"  // Écrit la valeur de R1 à l'adresse pointée par R0
      "LDR r0, =0x50000508    \n"  // P0_OUTSET : LED à logique inversée, 1 = éteinte
      "STR r1, [r0]           \n"  // éteint les trois couleurs
      "BX    LR               \n"
      ".ltorg                 \n"
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
      "LDR r1, =0x50000508    \n"   // P0_OUTSET
      "LDR r2, =0x01010040    \n"   // masque des trois couleurs
      "STR r2, [r1]           \n"   // éteint les trois couleurs d'abord
      "LDR r1, =0x5000050C    \n"   // P0_OUTCLR
      "STR r0, [r1]           \n"   // allume la couleur demandée (0 = allumée)
      "BX    LR               \n"
      ".ltorg                 \n"
  );
}

/* -------------------------------------------------------------------
 * F1. LED par MMIO — tout éteindre
 * Responsable : Félix-Antoine Guimont
 * ------------------------------------------------------------------- */
extern "C" __attribute__((naked)) void led_eteindre(void) {
  asm volatile(
      "ldr r0, =0x50000508  \n"   // Charge l'adresse matérielle de ADR_OUTCLR dans le registre r1 
      "ldr r1, =0x01010040  \n"   // Charge le masque combiné (Rouge, Vert, Bleu)
      "str r1, [r0]         \n"   // Écrit la valeur 'couleur' (r0) à l'adresse pointée par r1
      "bx lr                \n"
      ".ltorg               \n"   // Indispensable pour stocker la constante 0x5000050C en mémoire
  );
}

/* -------------------------------------------------------------------
 * H. Fonctions originales de l'équipe
 * Ajouter ici vos propres fonctions et les appeler depuis
 * executer_fonction_originale(), plus bas.
 * ------------------------------------------------------------------- */
BLEService serviceCalc("19B10000-E8F2-537E-4F6C-D104768A1214");
BLEStringCharacteristic carResultat("19B10001-E8F2-537E-4F6C-D104768A1214",
                                    BLERead | BLENotify, 20);
static bool ble_pret = false;


static void ble_demarrer(void) {
  if (!BLE.begin()) return;                 // échec : on réessaiera plus tard
  BLE.setLocalName("CalcGIF1001");
  BLE.setAdvertisedService(serviceCalc);
  static const uint8_t formatTexte[7] = {0x19, 0x00, 0x00, 0x27, 0x01, 0x00, 0x00};
  static BLEDescriptor descFormat("2904", formatTexte, 7);
  carResultat.addDescriptor(descFormat);
  serviceCalc.addCharacteristic(carResultat);
  BLE.addService(serviceCalc);
  carResultat.writeValue("pret");
  BLE.advertise();
  ble_pret = true;
}

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
  if (!ble_pret) ble_demarrer();
  if (!ble_pret) return;

  String msg;
  if (dernier_code == OK) {
    msg = "= ";
    msg += (const char *)texte_resultat;    // texte produit par convertir_entier (asm)
  } else {
    msg = "Erreur ";
    msg += dernier_code;
  }
  carResultat.writeValue(msg);              // notifie le téléphone s'il est abonné

  while (!Serial.available()) {             // on garde le BLE actif en attendant
    BLE.poll();                             // la prochaine saisie
  }
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
