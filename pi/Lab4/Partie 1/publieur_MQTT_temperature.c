#include <stdio.h>
#include <stdlib.h>
#include <mosquitto.h>
#include <bcm2835.h>
#include <pthread.h>
#include <time.h>
#include <math.h>

// Énumération pour les configuration du MCP9808
enum mcp9808_hysteresis{MCP9808_HYSTERESIS_0 = 0b00, MCP9808_HYSTERESIS_1_5 = 0b01, MCP9808_HYSTERESIS_3 = 0b10, MCP9808_HYSTERESIS_6 = 0b11};
enum mcp9808_power_state{MCP9808_CONTINUOUS_CONVERSION = 0b0, MCP9808_SHUTDOWN = 0b1};
enum mcp9808_crit_lock{MCP9808_CRIT_REG_UNLOCKED = 0b0, MCP9808_CRIT_REG_LOCKED = 0b1};
enum mcp9808_win_lock{MCP9808_WIN_REG_UNLOCKED = 0b0, MCP9808_WIN_REG_LOCKED = 0b1};
enum mcp9808_resolution{MCP9808_RESOLUTION_0_5 = 0b00, MCP9808_RESOLUTION_0_25 = 0b01, MCP9808_RESOLUTION_0_125 = 0b10, MCP9808_RESOLUTION_0_0625 = 0b11};

/* Définition des fonction de conversion de données entre les bits du capteur et des données utilisables
 * 
 * data : tableau de données reçues du capteur
 * 
 * output : pointeur vers l'emplacment ou mettre les données formatées
 * 
 * retourne s'il y a eu un problème de conversion
 */
typedef int (*conversion_data)(char* data, void* output);

// Structure d'argument pour la lecture d'un capteur I2C et l'envois au broker MQTT
typedef struct
{
    // Adresse du capteur
    uint8_t addr;

    // Numéro du registre à lire
    uint8_t reg;

    // Nombre d'octets attendu en réponse
    uint nb_octets_attendus;

    // Fonction à utiliser pour faire la conversion de données
    conversion_data fonct_conversion;

    // Topic sur lequel publier
    char* topic;
}args_capteur_i2c_mqtt;

/* Fonction initialisant la structure d'argument pour la lecture d'un capter
 * i2c et communication de la valeur par MQTT
 * 
 * args : pointeur vers la structure de d'argument à initialiser
 * 
 * addr : adresse du capteur sur le bus i2c
 * 
 * reg : numéro du registre contenant la valeure à lire
 * 
 * nb_octets_attendus : nombre d'octet que le capteur va envoyer
 * 
 * fonct_conversion : fonction à utiliser pour faire la conversion de données
 * 
 * topic : topic MQTT sur lequel publier la valeur lue
 * 
 * retourne toujour EXIT_SUCCESS
 */
int init_param_capteur_i2c_mqtt(args_capteur_i2c_mqtt* args, uint8_t addr, uint8_t reg, uint nb_octets_attendus, conversion_data fonct_conversion, char* topic)
{
    args->addr = addr;
    args->reg = reg;
    args->nb_octets_attendus = nb_octets_attendus;
    args->fonct_conversion = fonct_conversion;
    args->topic = topic;

    return EXIT_SUCCESS;
}

/* Fonction pour convertir une température en un tableau de char formaté pour le MCP9808
 * 
 * temp : valeur de température à convertir
 * 
 * data : retour de l'information formatée pour le MCP9808
 * 
 * la fonction retourne toujours EDXIT_SUCCESS
 */
int convertion_temp_to_bits(double temp, char* data)
{
    if(temp > 255)
        temp = 255;
    
    if(temp < -255)
        temp = -255;
    
    int partie_entiere_abs = abs((int) temp);
    double partie_decimale_abs = fabs(temp) - partie_entiere_abs;

    double base = 0.5;

    data[0] = 0;
    data[1] = 0;

    data[0] |= (0b00010000 * (temp < 0));
    data[0] |= partie_entiere_abs >> 4;

    data[1] |= partie_entiere_abs << 4;
    
    for(int i = 3; i >= 0; --i) {
        data[1] |= ((int)(partie_decimale_abs / base)) << i;

        partie_decimale_abs = fmod(partie_decimale_abs, base);

        base /= 2;
    }

    return EXIT_SUCCESS;
}

/* Fonction pour initialiser le capteur MCP9808
 * 
 * temp_upper : limite supérieure de la plage de température d'opération
 * 
 * temp_lower : limite inférieure de la plage de température d'opération
 * 
 * temp_crit : température critique
 * 
 * resolution : résolution de la mesure de température
 * 
 * hysteresis : valeur d'histeresis pour les limites et la valeur critique
 * 
 * power_state : état que le capteur doit prendre
 * 
 * crit_lock : si le registre de la valeur critique doit être vérouillé
 * 
 * win_lock : si les registres des limites inférieure et suprérieure doivent être vérouillés
 * 
 * la fonction retourne s'il y a eu un problème avec l'initilaisation
 */
int mcp9808_init(double temp_upper, double temp_lower, double temp_crit, enum mcp9808_resolution resolution,
                    enum mcp9808_hysteresis hysteresis, enum mcp9808_power_state power_state,
                    enum mcp9808_crit_lock crit_lock, enum mcp9808_win_lock win_lock)
{
    uint8_t status = EXIT_SUCCESS;

    // Buffer pour l'envois et la réception de données
    char payload[3] = {0, 0};

    // Charger le payload avec la température limite supérieure
    payload[0] = 0b00000010;
    convertion_temp_to_bits(temp_upper, &(payload[1]));

    //printf("Valeurs à pousser pour temp upper\n");
    //printf("%02X%02X\n", payload[1], payload[2]);

    // Pousser les nouvelles valeurs sur le MCP9808
    if((status = bcm2835_i2c_write(payload, 3)))
    {
        printf("Fail to sent data because %d\n", status);
        return status;
    }

    // Pousser les nouvelles valeurs sur le MCP9808
    //if((status = bcm2835_i2c_write(payload, 1)))
    //{
    //    printf("Fail to sent data because %d\n", status);
    //    return status;
    //}

    //if((status = bcm2835_i2c_read(&(payload[1]), 2)))
    //{
    //    printf("Fail to read data because %d\n", status);
    //    return status;
    //}
    
    //printf("Valeurs du registre temp upper\n");
    //printf("%02X%02X\n\n", payload[1], payload[2]);

    // Charger le payload avec la température limite inférieure
    payload[0] = 0b00000011;
    convertion_temp_to_bits(temp_lower, &(payload[1]));

    //printf("Valeurs à pousser pour temp lower\n");
    //printf("%02X%02X\n", payload[1], payload[2]);

    // Pousser les nouvelles valeurs sur le MCP9808
    if((status = bcm2835_i2c_write(payload, 3)))
    {
        printf("Fail to sent data because %d\n", status);
        return status;
    }

    // Pousser les nouvelles valeurs sur le MCP9808
    //if((status = bcm2835_i2c_write(payload, 1)))
    //{
    //    printf("Fail to sent data because %d\n", status);
    //    return status;
    //}

    //if((status = bcm2835_i2c_read(&(payload[1]), 2)))
    //{
    //    printf("Fail to read data because %d\n", status);
    //    return status;
    //}
    
    //printf("Valeurs du registre temp lower\n");
    //printf("%02X%02X\n\n", payload[1], payload[2]);

    // Charger le payload avec la température critique
    payload[0] = 0b00000100;
    convertion_temp_to_bits(temp_crit, &(payload[1]));

    //printf("Valeurs à pousser pour temp crit\n");
    //printf("%02X%02X\n", payload[1], payload[2]);

    // Pousser les nouvelles valeurs sur le MCP9808
    if((status = bcm2835_i2c_write(payload, 3)))
    {
        printf("Fail to sent data because %d\n", status);
        return status;
    }

    // Pousser les nouvelles valeurs sur le MCP9808
    //if((status = bcm2835_i2c_write(payload, 1)))
    //{
    //    printf("Fail to sent data because %d\n", status);
    //    return status;
    //}

    //if((status = bcm2835_i2c_read(&(payload[1]), 2)))
    //{
    //    printf("Fail to read data because %d\n", status);
    //    return status;
    //}
    
    //printf("Valeurs du registre temp crit\n");
    //printf("%02X%02X\n\n", payload[1], payload[2]);

    // Charger le payload avec la résolution
    payload[0] = 0b00001000;
    payload[1] = resolution;

    //printf("Valeurs à pousser pour résolution\n");
    //printf("%02X\n", payload[1]);

    // Pousser les nouvelles valeurs sur le MCP9808
    if((status = bcm2835_i2c_write(payload, 2)))
    {
        printf("Fail to sent data because %d\n", status);
        return status;
    }

    // Pousser les nouvelles valeurs sur le MCP9808
    //if((status = bcm2835_i2c_write(payload, 1)))
    //{
    //    printf("Fail to sent data because %d\n", status);
    //    return status;
    //}

    //if((status = bcm2835_i2c_read(&(payload[1]), 2)))
    //{
    //    printf("Fail to read data because %d\n", status);
    //    return status;
    //}
    
    //printf("Valeurs du registre résolution\n");
    //printf("%02X\n\n", payload[1]);

    // Charger le payload avec les configuration
    payload[0] = 0b00000001;
    payload[1] = (hysteresis << 1) | power_state;
    payload[2] = (crit_lock << 7) | (win_lock << 6);

    //printf("Valeurs à pousser pour configuration\n");
    //printf("%02X%02X\n", payload[1], payload[2]);

    // Pousser les nouvelles valeurs sur le MCP9808
    if((status = bcm2835_i2c_write(payload, 3)))
    {
        printf("Fail to sent data because %d\n", status);
        return status;
    }

    // Pousser les nouvelles valeurs sur le MCP9808
    //if((status = bcm2835_i2c_write(payload, 1)))
    //{
    //    printf("Fail to sent data because %d\n", status);
    //    return status;
    //}

    //if((status = bcm2835_i2c_read(&(payload[1]), 2)))
    //{
    //    printf("Fail to read data because %d\n", status);
    //    return status;
    //}
    
    //printf("Valeurs du registre configuration\n");
    //printf("%02X%02X\n\n", payload[1], payload[2]);

    return status;
}

int main(int args, char* argv[])
{
    int status = EXIT_SUCCESS;

    // Initialisation du bcm2835
    if (!bcm2835_init())
    {
        printf("Can't init bcm2835\n");
        status = EXIT_FAILURE;
        goto init_fail;
    }

    // Initialisation du bcm2835
    if (!bcm2835_i2c_begin())
    {
        printf("Can't start i2c\n");
        status = EXIT_FAILURE;
        goto i2c_fail;
    }

    bcm2835_i2c_set_baudrate(100000);

    bcm2835_i2c_setSlaveAddress(0x18);

    mcp9808_init(40.0, -20, 75, MCP9808_RESOLUTION_0_125, MCP9808_HYSTERESIS_1_5, MCP9808_CONTINUOUS_CONVERSION,
                    MCP9808_CRIT_REG_UNLOCKED, MCP9808_WIN_REG_UNLOCKED);
    
    bcm2835_i2c_end();

i2c_fail:
    bcm2835_close();

init_fail:
    return status;
}