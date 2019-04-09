// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app.h"
#include "attr.h"
#include "config.h"
#include "logging.h"
#include "logwindow.h"
#include "reaper.h"
#include "v8util.h"
#include "os/signal.h"
#include "os/time.h"
#include "os/conn.h"
#include "os/fd.h"
#include "os/limits.h"
#include "os/plugins.h"
#include "os/attr.h"
#include "os/locale.h"

#include <QApplication>
#include <QTranslator>
#include <libplatform/libplatform.h>
#include <unistd.h>

#define TR_ERROR1 L("File %1 not found")
#define TR_ERROR2 L("InitializeICUDefaultLocation failed")

using namespace v8;
Isolate* i;
static int64_t s_sigtime;
static int s_sigfd[2];

#define V8_BLOB_DIR DATADIR "/" APP_NAME "/v8/"
#define V8_N V8_BLOB_DIR "natives_blob.bin"
#define V8_S V8_BLOB_DIR "snapshot_blob_trusted.bin"
#define XLATE_DIR L(DATADIR "/" APP_NAME "/i18n")

#define LC_PREFIX "app." APP_NAME "."
#define LC_PREFIX_LEN (sizeof(LC_PREFIX) - 1)

Q_LOGGING_CATEGORY(lcCommand, LC_PREFIX "command")
Q_LOGGING_CATEGORY(lcKeymap, LC_PREFIX "keymap")
Q_LOGGING_CATEGORY(lcLayout, LC_PREFIX "layout")
Q_LOGGING_CATEGORY(lcSettings, LC_PREFIX "settings")
Q_LOGGING_CATEGORY(lcTerm, LC_PREFIX "terminal")
Q_LOGGING_CATEGORY(lcPlugin, LC_PREFIX "plugin")

static QtMessageHandler s_prevHandler;

static void
logMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    const char *lc = context.category;
    bool callprev = true;

    if (lc && strncmp(lc, LC_PREFIX, LC_PREFIX_LEN) == 0 && g_logwin) {
        callprev = g_logwin->log(type, L("%1: %2").arg(lc + LC_PREFIX_LEN, msg));
    }

    if (callprev) {
        s_prevHandler(type, context, msg);
    }
}

static int
run(QApplication *app)
{
    TermApplication *tapp = new TermApplication(app);
    tapp->start(s_sigfd[0]);
    int rc = app->exec();
    tapp->cleanup();

    return rc;
}

static int
runV8(QApplication *app)
{
    V8::InitializeExternalStartupData(V8_BLOB_DIR);
    Platform *v8p = platform::CreateDefaultPlatform();
    V8::InitializePlatform(v8p);
    V8::Initialize();
    Isolate::CreateParams cp;
    cp.array_buffer_allocator = ArrayBuffer::Allocator::NewDefaultAllocator();
    i = Isolate::New(cp);

    Locker locker(i);
    Isolate::Scope isolate_scope(i);

    TermApplication *tapp = new TermApplication(app);
    tapp->start(s_sigfd[0]);
    int rc = app->exec();
    tapp->cleanup();

    return rc;
}

static inline bool
setV8Error(QApplication &app, const QString &reason)
{
    app.setProperty(OBJPROP_V8_ERROR, reason);
    return false;
}

static inline void
initTranslations(QApplication &app)
{
    QTranslator translator;
    if (translator.load(QLocale(), g_mtstr, g_mtstr, XLATE_DIR, A(".qm")))
        app.installTranslator(&translator);

    initGlobalStrings();
}

int
main(int argc, char **argv)
{
    int rc;
    bool fdpurge = true, doV8 = true, sysplugins = true;
    QString adir = USE_SYSTEMD ? APP_XDG_DIR : APP_TMP_DIR;
    QString sdir = SERVER_TMP_DIR;

    // Parse arguments
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "-t") || !strcmp(argv[i], "--rundir")) {
            if (i < argc - 1)
                sdir = argv[++i];
        }
        else if (!strcmp(argv[i], "--tmp"))
            adir = APP_TMP_DIR;
        else if (!strcmp(argv[i], "--noplugins"))
            doV8 = false;
        else if (!strcmp(argv[i], "--nosysplugins"))
            sysplugins = false;
        else if (!strcmp(argv[i], "--version"))
            osPrintVersionAndExit(APP_NAME);
        else if (!strcmp(argv[i], "--man"))
            execlp("man", "man", APP_NAME, NULL), _exit(127);
        else if (!strcmp(argv[i], "--nofdpurge"))
            fdpurge = false;
    }

    if (fdpurge)
        osPurgeFileDescriptors(APP_NAME);
    if (osPipe(s_sigfd) != 0)
        return EXITCODE_FAILED;

    osInitLocale();
    osInitMonotime();
    osSetupClientSignalHandlers();
    osAdjustLimits();
    osLoadPlugins();

    Q_INIT_RESOURCE(resource);
    qRegisterMetaType<AttributeMap>("AttributeMap");
    s_prevHandler = qInstallMessageHandler(logMessageHandler);

    {
        QApplication app(argc, argv);
        app.setOrganizationName(ORG_NAME);
        app.setOrganizationDomain(ORG_DOMAIN);
        app.setApplicationName(APP_NAME);
        app.setApplicationVersion(PROJECT_VERSION);
        app.setProperty(OBJPROP_SDIR, sdir);
        app.setProperty(OBJPROP_ADIR, adir);

        initTranslations(app);

        if (doV8) {
            app.setProperty(OBJPROP_V8_SYSPLUGINS, sysplugins);
            if (!osFileExists(V8_N)) {
                doV8 = setV8Error(app, TR_ERROR1.arg(V8_N));
            }
            else if (!osFileExists(V8_S)) {
                doV8 = setV8Error(app, TR_ERROR1.arg(V8_S));
            }
        }
        rc = doV8 ? runV8(&app) : run(&app);
    }

    if (i)
        i->Dispose();

    return rc;
}

extern "C" void
deathHandler(int signal)
{
    int64_t curtime = osSigtime();

    if (s_sigfd[1] != -1) {
        s_sigtime = curtime;
        close(s_sigfd[1]);
        s_sigfd[1] = -1;
    }
    else if (curtime - s_sigtime > 2) {
        _exit(EXITCODE_KILLED);
    }

    ReaperThread::s_deathSignal = signal;
}

extern "C" void
reloadHandler(int signal)
{
}
