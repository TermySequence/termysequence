// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "deftheme.h"

#define NTHEMES 12

static const ThemeDef s_defaultThemes[NTHEMES] = {
    //
    // Basic defaults
    //
    { TN("settings-name", "Default Light"),
      TN("settings-name", "Light background"),
    },
    { TN("settings-name", "Default Dark"),
      TN("settings-name", "Dark background"),
      A("104,eeeeec,105,292929,118,393939,119,ffaaff,11a,393939,11b,ee00"),
      A("+ln=38;5;14:$video=38;5;13:")
    },

    //
    // Solarized
    //
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

    //
    // Light themes
    //
    { TN("settings-name", "Light Pastels"),
      TN("settings-name", "Light background"),
      A("1,d32e27,2,3aa34e,3,ffb908,4,198ab9,5,5a397f,6,2ca99c,7,dddddd,8,4c4c4d,9,f05253,a,6fc178,b,ffd067,c,4bbce3,d,866fa6,e,76c7bd,104,5d5e5d,105,eaeaea,10e,f05253,10f,ffc226,110,dd0000,111,ffffff,112,f05253,113,ffc226,114,ffd067,115,5d5e5d,116,f99d2f,117,ffffff,118,f5f6f6,119,ed5897,11a,f5f6f6,11b,3aafb5,11c,f99d2f,11d,ffffff,11f,27aae1,121,32bcae,123,41b657,125,de554f,127,866fa6,128,dd0000,129,ffffff,12a,ffd067,12b,5d5e5d"),
      A("+di=38;5;32:ex=38;5;34:$archive=38;5;125:$video=38;5;134:$audio=38;5;136:")
    },
    { TN("settings-name", "Light Gray"),
      TN("settings-name", "Light background"),
      A("1,d32e27,2,3aa34e,3,ffb908,4,198ab9,5,5a397f,6,2ca99c,7,dddddd,8,4c4c4d,9,f05253,a,6fc178,b,ffd067,c,4bbce3,d,866fa6,e,76c7bd,104,4c4c4d,105,d1d3d4,10e,f05253,10f,ffc226,110,dd0000,111,ffffff,112,f04f65,113,ffc226,114,ffc226,115,5d5e5d,116,f99d2f,117,ffffff,118,dddddd,119,d53f7e,11a,dddddd,11b,22969c,11c,f99d2f,11d,ffffff,11f,198ab9,121,32bcae,123,3aa34e,125,d32e27,127,f05253,128,dd0000,129,ffffff,12a,ffc226,12b,5d5e5d"),
      A("+ex=38;5;34:$archive=38;5;124:$video=38;5;93:$audio=38;5;136:")
    },
    { TN("settings-name", "Light Yellow"),
      TN("settings-name", "Light background"),
      A("1,d32e27,2,3aa34e,3,ffb908,4,198ab9,5,5a397f,6,2ca99c,7,dddddd,8,4c4c4d,9,f05253,a,6fc178,b,ffd067,c,4bbce3,d,866fa6,e,76c7bd,105,feeb7c,10e,d32e27,10f,eaa800,110,dd0000,111,ffffff,112,d32e27,113,ffd067,114,ffc226,115,d32e27,116,67c1b6,117,ffffff,118,f7d95c,119,412e5b,11a,f7d95c,11b,79609c,11c,67c1b6,11d,ffffff,11f,198ab9,121,43c0bc,123,3aa34e,125,d32e27,127,f05253,128,dd0000,129,ffffff,12a,ffc226,12b,d32e27"),
      A("+ex=38;5;34:$archive=38;5;124:$video=38;5;93:$audio=38;5;136:")
    },
    { TN("settings-name", "Light Pink"),
      TN("settings-name", "Light background"),
      A("1,d32e27,2,3aa34e,3,ffb908,4,198ab9,5,5a397f,6,2ca99c,7,dddddd,8,4c4c4d,9,f05253,a,6fc178,b,ffd067,c,4bbce3,d,866fa6,e,76c7bd,104,2c2c2d,105,f8c8dd,10e,d32e27,10f,ffe6a8,110,dd0000,111,ffffff,112,e75041,113,ffc226,114,ffd067,115,e75041,116,f99d2f,117,ffffff,118,f7b4d1,119,603101,11a,f7b4d1,11b,263960,11c,f99d2f,11d,ffffff,11f,b2d1,121,32bcae,123,41b657,125,d32e27,127,f05253,128,dd0000,129,ffffff,12a,ffd067,12b,e75041"),
      A("+di=38;5;32:ex=38;5;34:$archive=38;5;124:$video=38;5;93:$audio=38;5;136:")
    },

    //
    // Dark themes
    //
    { TN("settings-name", "Dark Red"),
      TN("settings-name", "Dark background"),
      A("1,a71e22,2,178b40,3,f2a603,4,b89b4,5,5a397f,6,b9988,7,d5d5d5,8,4c4c4d,9,f05253,a,6fc178,b,ffd067,c,69c7e8,d,937fb0,e,67c1b6,104,fef2d3,105,ce373e,10e,6b0d0e,10f,fcb315,110,6b0d0e,111,ffffff,112,ffd067,113,42464d,114,f05253,115,fcc955,116,fcb315,117,ffffff,118,bf2632,119,fcb315,11a,bf2632,11b,69c7e8,11c,fcb315,11d,ffffff,11f,27aae1,121,67c1b6,123,6fc178,125,a71e22,127,f05253,128,6b0d0e,129,ffffff,12a,ffd067,12b,42464d"),
      A("+di=38;5;21:ln=38;5;12:su=48;5;202;38;5;231:ca=48;5;202;38;5;226:$archive=38;5;226:")
    },
    { TN("settings-name", "Dark Brown"),
      TN("settings-name", "Dark background"),
      A("1,d32e27,2,3aa34e,3,fcb315,4,27aae1,5,694395,6,2ca99c,7,d5d5d5,8,4c4c4d,9,f05253,a,6fc178,b,ffd067,c,69c7e8,d,866fa6,e,67c1b6,104,fef2d3,105,5b3822,10e,d32e27,10f,ffc226,110,d32e27,111,ffffff,112,f05253,113,ffd067,114,ffd067,115,5b3822,116,f99d2f,117,5b3822,118,4c2c19,119,f179ac,11a,4c2c19,11b,32bc8a,11c,f99d2f,11d,5b3822,11f,27aae1,121,32bcae,123,41b657,125,d32e27,127,f05253,128,d32e27,129,ffffff,12a,ffd067,12b,5b3822"),
      A("+$video=38;5;141:")
    },
    { TN("settings-name", "Dark Blue"),
      TN("settings-name", "Dark background"),
      A("1,d32e27,2,3aa34e,3,ffb908,4,27aae1,5,5a397f,6,2ca99c,7,d5d5d5,8,4c4c4d,9,f05253,a,6fc178,b,ffd067,c,69c7e8,d,79609c,e,67c1b6,104,fef2d3,105,93655,10e,f05253,10f,ffc226,110,d32e27,111,ffffff,112,f05253,113,ffd067,114,ffd067,115,93655,116,fbae17,117,ffffff,118,22639,119,27aae1,11a,22639,11b,67c1b6,11c,fbae17,11d,ffffff,11f,27aae1,121,32bcae,123,41b657,125,d32e27,127,f05253,128,d32e27,129,ffffff,12a,ffd067,12b,93655"),
      A("+$video=38;5;141:")
    },
    { TN("settings-name", "Dark Pastels"),
      TN("settings-name", "Dark background"),
      A("1,d32e27,2,3aa34e,3,ffb908,4,198ab9,5,5a397f,6,2ca99c,7,d5d5d5,8,4c4c4d,9,f05253,a,6fc178,b,ffd067,c,4bbce3,d,866fa6,e,76c7bd,104,ffffff,105,2a1a29,10e,f05253,10f,ffd067,110,d32e27,111,ffffff,112,f05253,113,ffd067,114,ffd067,115,2a1a29,116,f99d2f,117,2a1a29,118,140d14,119,f179ac,11a,140d14,11b,2fb182,11c,f99d2f,11d,2a1a29,11f,b2d1,121,2ca99c,123,6fc178,125,d32e27,127,f05253,128,d32e27,129,ffffff,12a,ffd067,12b,2a1a29"),
      A("+di=38;5;32:ln=38;5;45:ex=38;5;41:$archive=38;5;125:$video=38;5;140:$audio=38;5;11:")
    },
};
