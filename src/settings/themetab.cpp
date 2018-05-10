// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "app/config.h"
#include "app/exception.h"
#include "app/iconbutton.h"
#include "app/messagebox.h"
#include "base/selwidget.h"
#include "themetab.h"
#include "thememodel.h"
#include "themenew.h"
#include "settings.h"
#include "theme.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QPainter>
#include <QtMath>

#define TR_ASK1 TL("question", "Theme \"%1\" exists. Overwrite?")
#define TR_ASK2 TL("question", "Really delete theme \"%1\"?")
#define TR_BUTTON1 TL("input-button", "Save Theme") + A("...")
#define TR_BUTTON2 TL("input-button", "Rename Theme") + A("...")
#define TR_BUTTON3 TL("input-button", "Delete Theme")
#define TR_BUTTON4 TL("input-button", "Reload Files")
#define TR_ERROR1 TL("error", "Group name must not be empty")
#define TR_TEXT1 TL("window-text", "Current Palette")
#define TR_TITLE1 TL("window-title", "Confirm Overwrite")
#define TR_TITLE2 TL("window-title", "Confirm Delete")

//
// Tab Widget
//
ThemeTab::ThemeTab(TermPalette &palette, const TermPalette &saved, const QFont &font) :
    m_palette(palette)
{
    m_model = new ThemeModel(palette, saved, font, this);
    m_view = new ThemeView(m_model);
    m_preview = new ThemePreview(palette, font);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::NoButton, Qt::Vertical);
    auto *saveButton = new IconButton(ICON_SAVE, TR_BUTTON1);
    m_renameButton = new IconButton(ICON_RENAME_ITEM, TR_BUTTON2);
    m_deleteButton = new IconButton(ICON_REMOVE_ITEM, TR_BUTTON3);
    auto *reloadButton = new IconButton(ICON_RELOAD, TR_BUTTON4);
    buttonBox->addButton(saveButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_renameButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_deleteButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(reloadButton, QDialogButtonBox::ActionRole);

    QHBoxLayout *viewLayout = new QHBoxLayout;
    viewLayout->addWidget(m_view, 1);
    viewLayout->addWidget(buttonBox);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(m_preview);
    QGroupBox *box = new QGroupBox(TR_TEXT1);
    box->setLayout(layout);

    layout = new QVBoxLayout;
    layout->addLayout(viewLayout, 1);
    layout->addWidget(box);
    setLayout(layout);

    connect(m_view, SIGNAL(rowClicked(int)), SLOT(handleRowClicked(int)));
    connect(m_model, SIGNAL(rowChanged()), SLOT(handleRowChanged()));

    connect(saveButton, SIGNAL(clicked()), SLOT(handleSave()));
    connect(m_renameButton, SIGNAL(clicked()), SLOT(handleRename()));
    connect(m_deleteButton, SIGNAL(clicked()), SLOT(handleDelete()));
    connect(reloadButton, SIGNAL(clicked()), SLOT(handleReload()));

    handleRowChanged();
}

void
ThemeTab::reload()
{
    m_model->reloadData();
    m_preview->update();
}

void
ThemeTab::handleRowClicked(int row)
{
    const TermPalette &palette = m_model->palette(row);
    m_model->setRowHint(row);

    if (m_palette != palette) {
        m_palette = palette;
        emit modified();
    }

    reload();
}

void
ThemeTab::handleRowChanged()
{
    bool removable = m_model->themeRemovable();
    m_renameButton->setEnabled(removable);
    m_deleteButton->setEnabled(removable);
}

inline ThemeSettings *
ThemeTab::doSave()
{
    auto *theme = g_settings->saveTheme(m_dialog->name());
    theme->setGroup(m_dialog->group());
    theme->setLesser(m_dialog->lesser());
    theme->setPalette(m_palette.tStr());
    theme->setDircolors(m_palette.dStr());
    g_settings->updateTheme(theme);
    return theme;
}

inline ThemeSettings *
ThemeTab::doRename()
{
    auto *theme = m_dialog->from();
    QString name = m_dialog->name();

    if (name == theme->name()) {
        // Update only
        theme->setGroup(m_dialog->group());
        theme->setLesser(m_dialog->lesser());
        g_settings->updateTheme(theme);
        return theme;
    } else {
        return g_settings->renameTheme(theme, name);
    }
}

void
ThemeTab::handleSaveConfirm(int result)
{
    if (result == QMessageBox::Yes)
        try {
            auto *theme = m_dialog->from() ? doRename() : doSave();
            m_dialog->deleteLater();
            m_view->selectRow(m_model->indexOf(theme));
        }
        catch (const StringException &e) {
            errBox(e.message(), m_dialog)->show();
        }
}

void
ThemeTab::handleSaveAnswer()
{
    if (m_dialog->group().trimmed().isEmpty()) {
        errBox(TR_ERROR1, m_dialog)->show();
        return;
    }

    QString name = m_dialog->name();
    bool exists = false;

    if (!m_dialog->from() || m_dialog->from()->name() != name)
        // Not an update
        try {
            exists = g_settings->validateThemeName(name, false);
        } catch (const StringException &e) {
            errBox(e.message(), m_dialog)->show();
            return;
        }

    if (exists) {
        auto *box = askBox(TR_TITLE1, TR_ASK1.arg(name), m_dialog);
        connect(box, SIGNAL(finished(int)), SLOT(handleSaveConfirm(int)));
        box->show();
    } else {
        handleSaveConfirm(QMessageBox::Yes);
    }
}

void
ThemeTab::handleSave()
{
    const auto *theme = m_model->currentTheme();
    m_dialog = new NewThemeDialog(this);

    if (theme) {
        m_dialog->setName(theme->name());
        m_dialog->setGroup(theme->group());
        m_dialog->setLesser(theme->lesser());
    }

    connect(m_dialog, SIGNAL(okayed()), SLOT(handleSaveAnswer()));
    m_dialog->show();
}

void
ThemeTab::handleRename()
{
    auto *theme = m_model->currentTheme();
    if (!theme || theme->builtin())
        return;

    m_dialog = new NewThemeDialog(this, theme);
    m_dialog->setName(theme->name());
    m_dialog->setGroup(theme->group());
    m_dialog->setLesser(theme->lesser());

    connect(m_dialog, SIGNAL(okayed()), SLOT(handleSaveAnswer()));
    m_dialog->show();
}

void
ThemeTab::handleDelete()
{
    auto *theme = m_model->currentTheme();
    if (!theme)
        return;

    QString name = theme->name();

    auto *box = askBox(TR_TITLE2, TR_ASK2.arg(name), this);
    connect(box, &QDialog::finished, [=](int result) {
        if (result == QMessageBox::Yes) {
            try {
                g_settings->deleteTheme(theme);
            } catch (const StringException &e) {
                errBox(e.message(), this)->show();
            }
        }
    });
    box->show();
}

void
ThemeTab::handleReload()
{
    for (auto i: g_settings->themes()) {
        i->sync();
        i->loadSettings();
    }

    g_settings->rescanThemes();
}

//
// Preview Widget
//
struct PreviewText {
    int row, col;
    int bg, fg;
    const char *text, *category;
    unsigned flags;
};

static const PreviewText s_previewText[] = {
    { 1, 1, -1, PALETTE_APP_FG,
      TN("theme-preview", "Dark colors") ":", "theme-preview" },
    { 1, -1, -1, 0,
      TN("theme-preview", "black"), "theme-preview" },
    { 1, -1, -1, 1,
      TN("theme-preview", "red"), "theme-preview" },
    { 1, -1, -1, 2,
      TN("theme-preview", "green"), "theme-preview" },
    { 1, -1, -1, 3,
      TN("theme-preview", "yellow"), "theme-preview" },
    { 1, -1, -1, 4,
      TN("theme-preview", "blue"), "theme-preview" },
    { 1, -1, -1, 5,
      TN("theme-preview", "magenta"), "theme-preview" },
    { 1, -1, -1, 6,
      TN("theme-preview", "cyan"), "theme-preview" },
    { 1, -1, -1, 7,
      TN("theme-preview", "white"), "theme-preview" },

    { 2, 1, -1, PALETTE_APP_FG,
      TN("theme-preview", "Light colors") ":", "theme-preview" },
    { 2, -1, -1, 8,
      TN("theme-preview", "black"), "theme-preview" },
    { 2, -1, -1, 9,
      TN("theme-preview", "red"), "theme-preview" },
    { 2, -1, -1, 10,
      TN("theme-preview", "green"), "theme-preview" },
    { 2, -1, -1, 11,
      TN("theme-preview", "yellow"), "theme-preview" },
    { 2, -1, -1, 12,
      TN("theme-preview", "blue"), "theme-preview" },
    { 2, -1, -1, 13,
      TN("theme-preview", "magenta"), "theme-preview" },
    { 2, -1, -1, 14,
      TN("theme-preview", "cyan"), "theme-preview" },
    { 2, -1, -1, 15,
      TN("theme-preview", "white"), "theme-preview" },

    { 4, 5, -1, PALETTE_APP_FG,
      TN("theme-preview", "Preview text"), "theme-preview" },
    { 4, -1, -1, PALETTE_APP_FG,
      TN("theme-preview", "Preview text"), "theme-preview" },
    { 4, -1, -1, PALETTE_APP_FG,
      TN("theme-preview", "Preview text"), "theme-preview" },
    { 4, -1, -1, PALETTE_APP_FG,
      TN("theme-preview", "Preview text"), "theme-preview" },

    { 5, 5, -1, PALETTE_APP_FG,
      TN("theme-preview", "Bold text"), "theme-preview", 1 },
    { 5, -1, -1, PALETTE_APP_FG,
      TN("theme-preview", "Bold text"), "theme-preview", 1 },
    { 5, -1, -1, PALETTE_APP_FG,
      TN("theme-preview", "Bold text"), "theme-preview", 1 },
    { 5, -1, -1, PALETTE_APP_FG,
      TN("theme-preview", "Bold text"), "theme-preview", 1 },

    { 6, 5, -1, PALETTE_APP_FG,
      TN("theme-preview", "Underline text"), "theme-preview", 2 },
    { 6, -1, -1, PALETTE_APP_FG,
      TN("theme-preview", "Underline text"), "theme-preview", 2 },
    { 6, -1, -1, PALETTE_APP_FG,
      TN("theme-preview", "Underline text"), "theme-preview", 2 },
    { 6, -1, -1, PALETTE_APP_FG,
      TN("theme-preview", "Underline text"), "theme-preview", 2 },

    { 8, 5, PALETTE_SH_PROMPT_BG, PALETTE_SH_PROMPT_FG,
      TN("theme-preview", "Normal-Prompt-text") "$", "theme-preview" },
    { 8, -1, PALETTE_SH_COMMAND_BG, PALETTE_SH_COMMAND_FG,
      TN("theme-preview", "Normal Command text" ), "theme-preview" },

    { 9, 5, PALETTE_SH_SELECTED_BG, PALETTE_SH_SELECTED_FG,
      TN("theme-preview", "Selected-Prompt-text") "$", "theme-preview" },
    { 9, -1, PALETTE_SH_COMMAND_BG, PALETTE_SH_COMMAND_FG,
      TN("theme-preview", "Normal Command text"), "theme-preview" },

    { 10, 5, PALETTE_SH_MATCHLINE_BG, PALETTE_SH_MATCHLINE_FG,
      TN("theme-preview", "Line containing search match"), "theme-preview" },
    { 10, -1, PALETTE_SH_MATCHTEXT_BG, PALETTE_SH_MATCHTEXT_FG,
      TN("theme-preview", "Search match"), "theme-preview" },

    { 11, 5, PALETTE_SH_NOTE_BG, PALETTE_SH_NOTE_FG,
      TN("theme-preview", "Annotated text"), "theme-preview" },

    { 4, 1, PALETTE_SH_MARK_RUNNING_BG, PALETTE_SH_MARK_RUNNING_FG,
      TN("prompt-mark", "R") "<", "prompt-mark", 1 },
    { 5, 1, PALETTE_SH_MARK_EXIT0_BG, PALETTE_SH_MARK_EXIT0_FG,
      TN("prompt-mark", "E") "0", "prompt-mark", 1 },
    { 6, 1, PALETTE_SH_MARK_EXITN_BG, PALETTE_SH_MARK_EXITN_FG,
      TN("prompt-mark", "E") "1", "prompt-mark", 1 },
    { 7, 1, PALETTE_SH_MARK_SIGNAL_BG, PALETTE_SH_MARK_SIGNAL_FG,
      TN("prompt-mark", "S") "2", "prompt-mark", 1 },
    { 8, 1, PALETTE_SH_MARK_PROMPT_BG, PALETTE_SH_MARK_PROMPT_FG,
      TN("prompt-mark", "P") ">", "prompt-mark", 1 },
    { 9, 1, PALETTE_SH_MARK_SELECTED_BG, PALETTE_SH_MARK_SELECTED_FG,
      TN("prompt-mark", "P") ">", "prompt-mark", 1 },
    { 10, 1, PALETTE_SH_MARK_MATCH_BG, PALETTE_SH_MARK_MATCH_FG,
      TN("prompt-mark", "M") ">", "prompt-mark", 1 },
    { 11, 1, PALETTE_SH_MARK_NOTE_BG, PALETTE_SH_MARK_NOTE_FG,
      TN("prompt-mark", "N") ">", "prompt-mark", 1 },

    { 7, 5, PALETTE_APP_FG, PALETTE_APP_BG,
      TN("theme-preview", "Selection"), "theme-preview", 4 },

    { 0 }
};

ThemePreview::ThemePreview(const TermPalette &palette, const QFont &font) :
    m_palette(palette),
    m_font(font)
{
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
    calculateCellSize(m_font);
    m_sizeHint = QSize(m_cellSize.width() * 64, m_cellSize.height() * 13);
}

void
ThemePreview::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.fillRect(rect(), m_palette.bg());

    qreal lastx = 0;

    for (const PreviewText *t = s_previewText; t->text; ++t)
    {
        painter.setFont(m_font);

        if (t->flags & 1) {
            QFont font(painter.font());
            font.setBold(true);
            painter.setFont(font);
        }
        if (t->flags & 2) {
            QFont font(painter.font());
            font.setUnderline(true);
            painter.setFont(font);
        }

        QString text = QCoreApplication::translate(t->category, t->text);
        qreal cw = m_cellSize.width(), ch = m_cellSize.height();
        qreal x = (t->col == -1) ? lastx : cw * t->col;
        qreal y = ch * t->row;
        qreal w = cw * text.size();
        QRectF rect(x, y, w, ch);

        if (t->bg != -1 && !PALETTE_IS_DISABLED(m_palette[t->bg]))
            painter.fillRect(rect, QColor(m_palette[t->bg]));

        painter.setPen(QColor(m_palette[t->fg]));
        painter.drawText(rect, Qt::AlignCenter, text);

        lastx = x + w + cw;

        if (t->flags & 4) {
            int line = 1 + (int)cw / CURSOR_BOX_INCREMENT;
            qreal half = line / 2.0;

            rect.translate(half, half);
            rect.setWidth(rect.width() - line);
            rect.setHeight(rect.height() - line);

            qreal dashSize = qCeil(cw / 2);
            QVector<qreal> dashPattern = { dashSize, dashSize };

            QPen pen(painter.pen());
            pen.setWidth(line);
            painter.setPen(pen);
            painter.drawRect(rect);
            pen.setDashPattern(dashPattern);
            pen.setColor(m_palette.fg());
            painter.setPen(pen);
            painter.drawRect(rect);

            painter.save();
            painter.setRenderHint(QPainter::Antialiasing, true);
            painter.translate(QPoint(x - cw / 2, y - cw / 2));
            SelectionHandle::paintPreview(&painter, &m_palette, m_cellSize, false);
            painter.resetTransform();
            painter.translate(QPoint(x + w - cw / 2, y));
            SelectionHandle::paintPreview(&painter, &m_palette, m_cellSize, true);
            painter.restore();
        }
    }
}

QSize
ThemePreview::sizeHint() const
{
    return m_sizeHint;
}
