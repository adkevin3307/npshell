#pragma once

namespace Constant {
    enum IOTARGET {
        IN,
        OUT,
        ERR
    };

    enum IO {
        STANDARD,
        PIPE,
        FILE
    };

    enum BUILTIN {
        EXIT,
        CD,
        SETENV,
        PRINTENV,
        NONE
    };
};