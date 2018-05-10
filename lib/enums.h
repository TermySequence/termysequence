// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

namespace Tsq {

    enum ServerFlavor {
        FlavorStandalone, FlavorNormal, FlavorActivated
    };

    enum ExitAction {
        ExitActionStop, ExitActionRestart, ExitActionClear
    };
    enum AutoClose {
        AutoCloseNever, AutoCloseZero, AutoCloseExit, AutoCloseAlways = 4
    };
    enum TermStatus {
        TermClosed, TermIdle, TermActive, TermBusy
    };
    enum TermOutcome {
        TermRunning, TermExit0, TermExitN, TermKilled, TermDumped
    };
    enum RegionType {
        RegionInvalid, RegionLowerBound = RegionInvalid, RegionSelection,
        RegionJob, RegionPrompt, RegionCommand, RegionOutput,
        RegionImage, RegionContent, RegionUser, RegionUpperBound,
        // Type codes reserved for client use
        RegionLocalLowerBound = 256
    };

    enum TaskConfig {
        TaskFail, TaskOverwrite, TaskRename, TaskAsk, TaskAskRecurse
    };
    enum TaskStatus {
        TaskRunning, TaskStarting, TaskAcking, TaskFinished, TaskError
    };
    enum TaskQuestion {
        TaskOverwriteRenameQuestion, TaskOverwriteQuestion, TaskRemoveQuestion,
        TaskRecurseQuestion
    };
    enum PortForwardTaskType {
        PortForwardTCP, PortForwardUNIX
    };

    enum GenericTaskError {
        TaskErrorCanceled,
        TaskErrorTimedOut,
        TaskErrorTargetInUse,
        NGenericTaskErrors,
    };
    enum UploadFileTaskError {
        UploadTaskErrorOpenFailed = NGenericTaskErrors,
        UploadTaskErrorWriteFailed,
        UploadTaskErrorFileExists,
    };
    enum DownloadFileTaskError {
        DownloadTaskErrorOpenFailed = NGenericTaskErrors,
        DownloadTaskErrorReadFailed,
    };
    enum DownloadImageTaskError {
        DownloadTaskErrorNoSuchImage = NGenericTaskErrors,
    };
    enum DeleteFileTaskError {
        DeleteTaskErrorFileNotFound = NGenericTaskErrors,
        DeleteTaskErrorRemoveFailed,
    };
    enum RenameFileTaskError {
        RenameTaskErrorFileExists = NGenericTaskErrors,
        RenameTaskErrorInvalidName,
        RenameTaskErrorRenameFailed,
    };
    enum PortForwardTaskError {
        PortForwardTaskErrorBadAddress = NGenericTaskErrors,
        PortForwardTaskErrorBindFailed,
    };
    enum MountTaskError {
        MountTaskErrorOpenFailed = NGenericTaskErrors,
    };
    enum MountTaskOpcode {
        MountTaskOpInvalid, MountTaskOpLookup, MountTaskOpStat,
        MountTaskOpOpen, MountTaskOpRead, MountTaskOpWrite,
        MountTaskOpAppend, MountTaskOpUpload, MountTaskOpDownload,
        MountTaskOpClose, MountTaskOpCreate,
        MountTaskOpChmod, MountTaskOpTrunc, MountTaskOpTouch,
        MountTaskOpOpendir, MountTaskOpReaddir, MountTaskOpClosedir,
    };
    enum MountTaskResult {
        MountTaskSuccess, MountTaskFailure, MountTaskExist, MountTaskFiletype,
    };
    enum ConnectTaskError {
        ConnectTaskErrorWriteFailed = NGenericTaskErrors,
        ConnectTaskErrorRemoteReadFailed,
        ConnectTaskErrorRemoteConnectFailed,
        ConnectTaskErrorRemoteHandshakeFailed,
        ConnectTaskErrorRemoteLimitExceeded,
        ConnectTaskErrorLocalReadFailed,
        ConnectTaskErrorLocalConnectFailed,
        ConnectTaskErrorLocalHandshakeFailed,
        ConnectTaskErrorLocalTransferFailed,
        ConnectTaskErrorLocalRejection,
        ConnectTaskErrorLocalBadProtocol,
        ConnectTaskErrorLocalBadResponse,
        ConnectTaskErrorReadIdFailed,
    };
}
