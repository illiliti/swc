/* swc: swc/bindings.c
 *
 * Copyright (c) 2013 Michael Forney
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "swc.h"
#include "bindings.h"
#include "keyboard.h"
#include "util.h"

#include <wayland-util.h>

struct binding
{
    uint32_t value;
    uint32_t modifiers;
    swc_binding_handler_t handler;
    void * data;
};

static struct
{
    struct wl_array keys;
} bindings;

static bool handle_keysym(xkb_keysym_t keysym,
                          uint32_t modifiers, uint32_t time)
{
    struct binding * binding;

    wl_array_for_each(binding, &bindings.keys)
    {
        if (binding->value == keysym
            && (binding->modifiers == modifiers
                || binding->modifiers == SWC_MOD_ANY))
        {
            binding->handler(time, keysym, binding->data);
            return true;
        }
    }

    return false;
}

static bool handle_key(struct swc_keyboard * keyboard, uint32_t time,
                       uint32_t key, uint32_t state)
{
    if (state == WL_KEYBOARD_KEY_STATE_PRESSED)
    {
        xkb_keysym_t keysym;

        keysym = xkb_state_key_get_one_sym(keyboard->xkb.state, XKB_KEY(key));

        if (handle_keysym(keysym, keyboard->modifiers, time))
            return true;

        xkb_layout_index_t layout;
        const xkb_keysym_t * keysyms;

        layout = xkb_state_key_get_layout(keyboard->xkb.state, XKB_KEY(key));
        xkb_keymap_key_get_syms_by_level(keyboard->xkb.keymap.map, XKB_KEY(key),
                                         layout, 0, &keysyms);

        if (keysyms && handle_keysym(keysyms[0], keyboard->modifiers, time))
            return true;
    }

    return false;
}

static struct swc_keyboard_handler binding_handler = {
    .key = &handle_key,
};

const struct swc_bindings_global bindings_global = {
    .keyboard_handler = &binding_handler
};

bool swc_bindings_initialize()
{
    wl_array_init(&bindings.keys);

    return true;
}

void swc_bindings_finalize()
{
    wl_array_release(&bindings.keys);
}

EXPORT
void swc_add_key_binding(uint32_t modifiers, uint32_t value,
                         swc_binding_handler_t handler, void * data)
{
    struct binding * binding;

    binding = wl_array_add(&bindings.keys, sizeof *binding);
    binding->value = value;
    binding->modifiers = modifiers;
    binding->handler = handler;
    binding->data = data;
}

