// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

/*
 * Protocol version
 */
#define TSQT_PROTOCOL_VERSION           1

/*
 * Commands
 */
// IN serverspec
// RESP success:0|error:1 pipepath|errstr
#define TSQT_UPLOAD_PIPE                1
// IN serverspec
// RESP success:0|error:1 pipepath|errstr
#define TSQT_DOWNLOAD_PIPE              2
// IN serverspec
// RESP serverspec
#define TSQT_LIST_SERVERS               3
// IN slot
// RESP success:0|error:1 errstr
#define TSQT_RUN_ACTION                 4
