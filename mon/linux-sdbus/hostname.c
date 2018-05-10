// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "mon-sdbus.h"
#include "lib/attr.h"

static int
asyncHandler(sd_bus_message *reply, void *s, sd_bus_error *error)
{
    struct mon_state *state = s;
    const char *val;
    int rc;

    rc = sd_bus_message_enter_container(reply, 'v', "s");
    if (rc < 0)
        goto out;

    rc = sd_bus_message_read_basic(reply, 's', &val);
    if (rc < 0)
        goto out;

    stringReset(&state->str);
    stringInsert(&state->str, TSQ_ATTR_ICON, val);
    stringPrint(&state->str);
out:
    return 1;
}

void
startingProps(struct mon_state *state)
{
    const char *d = "org.freedesktop.DBus.Properties";
    const char *s, *p, *n;

    s = "org.freedesktop.hostname1";
    p = "/org/freedesktop/hostname1";
    n = "IconName";

    sd_bus_call_method_async(state->bus, NULL, s, p, d, "Get",
                             &asyncHandler, state, "ss", s, n);
}

static void
syncProp(struct mon_state *state, const char *service, const char *path,
             const char *prop, const char *attr)
{
    char *result = NULL;

    sd_bus_get_property_string(state->bus, service, path, service,
                               prop, NULL, &result);

    if (result) {
        stringInsert(&state->str, attr, result);
        free(result);
    }
}

void
hostnameProps(struct mon_state *state)
{
    const char *s = "org.freedesktop.hostname1";
    const char *p = "/org/freedesktop/hostname1";

    syncProp(state, s, p, "Hostname", TSQ_ATTR_HOST);
    syncProp(state, s, p, "IconName", TSQ_ATTR_ICON);
}

void
timedateProps(struct mon_state *state)
{
    const char *s = "org.freedesktop.timedate1";
    const char *p = "/org/freedesktop/timedate1";

    syncProp(state, s, p, "Timezone", TSQ_ATTR_TIMEZONE);
}
