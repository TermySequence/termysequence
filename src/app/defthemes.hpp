// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "deftheme.h"

#define NTHEMES 4

static const ThemeDef s_defaultThemes[NTHEMES] = {
    { TN("settings-name", "Default Light"),
      TN("settings-name", "Light background"),
    },
    { TN("settings-name", "Default Dark"),
      TN("settings-name", "Dark background"),
      A("104,eeeeec,105,292929,118,393939,119,ffaaff,11a,393939,11b,ee00"),
      A("+ln=38;5;14:$video=38;5;13:")
    },
    { TN("settings-name", "Solarized Light"),
      TN("settings-name", "Light background"),
      A("0,73642,1,dc322f,2,859900,3,b58900,4,268bd2,5,d33682,6,2aa198,7,eee8d5,8,2b36,9,cb4b16,a,586e75,b,657b83,c,839496,d,6c71c4,e,93a1a1,f,fdf6e3,104,657b83,105,fdf6e3,10e,d70000,10f,ffd700,110,dc322f,111,fdf6e3,112,cb4b16,113,eee8d5,114,b58900,115,eee8d5,116,eee8d5,117,cb4b16,118,eee8d5,119,586e75,11a,eee8d5,11b,b58900,11c,cb4b16,11d,73642,11f,268bd2,121,2aa198,123,859900,125,cb4b16,127,dc322f,128,dc322f,129,fdf6e3,12a,b58900,12b,eee8d5"),
      A("+di=36:ln=35:pi=30;44:so=35;44:bd=33;44:cd=37;44:or=48;5;236;31:mi=05;37;41:ex=38;5;9:$archive=38;5;13:$video=33:$audio=33:")
    },
    { TN("settings-name", "Solarized Dark"),
      TN("settings-name", "Dark background"),
      A("0,73642,1,dc322f,2,859900,3,b58900,4,268bd2,5,d33682,6,2aa198,7,eee8d5,8,2b36,9,cb4b16,a,586e75,b,657b83,c,839496,d,6c71c4,e,93a1a1,f,fdf6e3,104,839496,105,2b36,10e,d70000,10f,ffd700,110,dc322f,111,fdf6e3,112,cb4b16,113,eee8d5,114,b58900,115,eee8d5,116,73642,117,cb4b16,118,73642,119,93a1a1,11a,73642,11b,b58900,11c,cb4b16,11d,73642,11f,268bd2,121,2aa198,123,859900,125,cb4b16,127,dc322f,128,dc322f,129,fdf6e3,12a,b58900,12b,eee8d5"),
      A("+di=34:ln=35:pi=30;44:so=35;44:bd=33;44:cd=37;44:or=48;5;16;31:mi=05;37;41:ex=38;5;9:$archive=38;5;13:$video=33:$audio=33:")
    },
};
