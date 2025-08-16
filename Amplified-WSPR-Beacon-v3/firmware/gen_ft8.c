#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>

#include "common/common.h"
#include "ft8/message.h"
#include "ft8/encode.h"
#include "ft8/constants.h"

#define LOG_LEVEL LOG_INFO
#include "ft8/debug.h"

#define FT8_SYMBOL_BT 2.0f ///< symbol smoothing filter bandwidth factor (BT)
#define FT4_SYMBOL_BT 1.0f ///< symbol smoothing filter bandwidth factor (BT)

int encoder(char *message, uint8_t *tones, int is_ft4)
{
    float frequency = 1000.0;

    // First, pack the text data into binary message
    ftx_message_t msg;
    ftx_message_rc_t rc = ftx_message_encode(&msg, NULL, message);
    if (rc != FTX_MESSAGE_RC_OK)
    {
        printf("Cannot parse message!\n");
        printf("RC = %d\n", (int)rc);
        return -2;
    }

    printf("Packed data: ");
    for (int j = 0; j < 10; ++j)
    {
        printf("%02x ", msg.payload[j]);
    }
    printf("\n");

    int num_tones = (is_ft4) ? FT4_NN : FT8_NN;
    float symbol_period = (is_ft4) ? FT4_SYMBOL_PERIOD : FT8_SYMBOL_PERIOD;
    float symbol_bt = (is_ft4) ? FT4_SYMBOL_BT : FT8_SYMBOL_BT;
    float slot_time = (is_ft4) ? FT4_SLOT_TIME : FT8_SLOT_TIME;

    // Second, encode the binary message as a sequence of FSK tones
    if (is_ft4)
    {
        ft4_encode(msg.payload, tones);
    }
    else
    {
        ft8_encode(msg.payload, tones);
    }

    printf("FSK tones: ");
    for (int j = 0; j < num_tones; ++j)
    {
        printf("%d", tones[j]);
    }
    printf("\n");

    return 0;
}
