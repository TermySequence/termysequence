// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "lib/enums.h"

namespace Tsqt {

    enum RegionType {
        // Redefinition of server region types
        RegionInvalid, RegionLowerBound = RegionInvalid, RegionSelection,
        RegionJob, RegionPrompt, RegionCommand, RegionOutput,
        RegionImage, RegionContent, RegionUser,
        // Client region types
        RegionLink = Tsq::RegionLocalLowerBound, RegionSemantic, RegionSearch,
        RegionUpperBound
    };

    enum PromptClose {
        PromptCloseNever, PromptCloseSubproc, PromptCloseProc, PromptCloseAlways
    };
    enum ConnectionType {
        ConnectionPersistent, ConnectionTransient, ConnectionBatch,
        ConnectionGeneric, ConnectionSsh,
        ConnectionUserSudo, ConnectionUserSu,
        ConnectionUserMctl, ConnectionUserPkexec,
        ConnectionMctl, ConnectionDocker,
        ConnectionKubectl, ConnectionRkt,
        ConnectionNTypes,
    };
    enum SearchType {
        SingleLinePlainText, SingleLineEcmaRegex
    };
}
