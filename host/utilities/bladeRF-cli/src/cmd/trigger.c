#include <stdio.h>
#include "common.h"
#include "conversions.h"
#include <string.h>

int print_trigger(struct cli_state *state, bladerf_trigger target)
{
    int status = 0;
    uint8_t trigger_value;
    
    status = bladerf_read_trigger(state->dev, target, &trigger_value);
    if (status != 0)
        return status;
    
    bool armed = (trigger_value & BLADERF_TRIGGER_ARM_BIT);
    bool fire = (trigger_value & BLADERF_TRIGGER_FIRE_BIT);
    bool master = (trigger_value & BLADERF_TRIGGER_MASTER_BIT);
    bool linestate = (trigger_value & BLADERF_TRIGGER_LINE_BIT);
    
    printf("  %s-trigger register value: 0x%02x\n", trigger2str(target), trigger_value);
    if (!armed) {
        printf("   |- Trigger system is disabled\n");
    } else {
        printf("   |- Trigger system is enabled\n");
        if (!master) {
            printf("   |- Trigger configured as slave\n");
        } else {
            printf("   |- Trigger configured as master\n");
            if (!fire) {
                printf("   |- Fire request is not set\n");
            } else {
                printf("   |- Fire request is set\n");
            }
        }
        if (linestate) {
            printf("   |- mini_exp1 line HIGH, trigger NOT FIRED\n");
        } else {
            printf("   |- mini_exp1 line LOW, trigger FIRED\n");
        }
    }
    return 0;
}

int cmd_trigger(struct cli_state *state, int argc, char **argv)
{
    int status = 0;

    if (argc > 3) {
        return CLI_RET_NARGS;
    }

    if (argc == 1)
    {
        // Print status of both triggers
        status = print_trigger(state, BLADERF_TRIGGER_TX);
        if (status != 0)
            goto out;
        status = print_trigger(state, BLADERF_TRIGGER_RX);
        goto out;
    }
    
    bladerf_trigger target;
    
    if (!strcasecmp(argv[1], "tx")) {
        target = BLADERF_TRIGGER_TX;
    } else if (!strcasecmp(argv[1], "rx")) {
        target = BLADERF_TRIGGER_RX;
    } else {
        cli_err(state, argv[0], "Invalid trigger target: %s\n", argv[1]);
        return CLI_RET_INVPARAM;
    }
    
    if (argc == 2)
    {
        // Print status of target
        status = print_trigger(state, target);
        goto out;
    }

    // Dissect current value to print certain warnings
    uint8_t trigger_value;
    status = bladerf_read_trigger(state->dev, target, &trigger_value);
    if (status != 0)
        goto out;
    
    bool armed = (trigger_value & BLADERF_TRIGGER_ARM_BIT);
    bool fire = (trigger_value & BLADERF_TRIGGER_FIRE_BIT);
    bool master = (trigger_value & BLADERF_TRIGGER_MASTER_BIT);
    bool linestate = (trigger_value & BLADERF_TRIGGER_LINE_BIT);
    
    if (!strcasecmp(argv[2], "off")) {
        // Disarm trigger
        status = bladerf_write_trigger(state->dev, target, 0x00);
        if (status != 0)
            goto out;
        printf("  %s-trigger successfully disabled\n", trigger2str(target));
    } else if (!strcasecmp(argv[2], "slave")) {
        if (!linestate)
            printf("  WARNING: mini-exp1 line is low, trigger will fire immediately\n");
        if (master && fire)
            printf("  WARNING: Fire-request on master was active, triggered slaves will stop\n");
        status = bladerf_write_trigger(state->dev, target, BLADERF_TRIGGER_ARM_BIT);
        if (status != 0)
            goto out;
        printf("  %s-trigger successfully configured as slave\n", trigger2str(target));
    } else if (!strcasecmp(argv[2], "master")) {
        if (armed && !master)
            printf("  WARNING: Changing from slave to master, make sure there are no two masters on one chain\n");
        status = bladerf_write_trigger(state->dev, target, BLADERF_TRIGGER_ARM_BIT | BLADERF_TRIGGER_MASTER_BIT);
        if (status != 0)
            goto out;
        printf("  %s-trigger successfully configured as master\n", trigger2str(target));
    } else if (!strcasecmp(argv[2], "fire")) {
        if (!(armed && master))
        {
            cli_err(state, argv[0], "%s-trigger not configured as master, can't fire\n", trigger2str(target));
            return CLI_RET_CMD_HANDLED;
        }
        if (fire)
        {
            printf("  WARNING: Trigger already fired, ignoring request\n");
            goto out;
        }
        status = bladerf_write_trigger(state->dev, target, trigger_value | BLADERF_TRIGGER_FIRE_BIT);
        if (status != 0)
            goto out;
        printf("  Successfully set fire-request on %s-trigger\n", trigger2str(target));
    } else if (!strcasecmp(argv[2], "stop")) {
        if (!(armed && master)) {
            cli_err(state, argv[0], "%s-trigger not configured as master, can't stop fire\n", trigger2str(target));
            return CLI_RET_CMD_HANDLED;
        }
        if (!fire) {
            printf("  WARNING: Trigger not currently firing, ignoring request\n");
            goto out;
        }
        status = bladerf_write_trigger(state->dev, target, trigger_value & (~BLADERF_TRIGGER_FIRE_BIT));
        if (status != 0)
            goto out;
        printf("  Successfully cleared fire-request on %s-trigger\n", trigger2str(target));
    } else {
        cli_err(state, argv[0], "Invalid trigger command: %s\n", argv[2]);
        return CLI_RET_INVPARAM;
    }

out:
    if (status != 0) {
        state->last_lib_error = status;
        status = CLI_RET_LIBBLADERF;
    }

    return status;
}
