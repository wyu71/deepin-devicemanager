// Copyright (C) 2019 ~ 2019 Deepin Technology Co., Ltd.
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// DTK
#include <DApplication>
#include <DGuiApplicationHelper>
#include <DPalette>
#include <DStyle>
#include <DStyleHelper>

// Qt
#include <QLoggingCategory>
#include <QModelIndex>
#include <QPainter>
#include <QStyleOptionViewItem>
#include <QPainterPath>

// other
#include "logviewitemdelegate.h"
#include "DDLog.h"

using namespace DDLog;

DWIDGET_USE_NAMESPACE

LogViewItemDelegate::LogViewItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
    qCDebug(appLog) << "LogViewItemDelegate instance created";
}

void LogViewItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                                const QModelIndex &index) const
{
    // qCDebug(appLog) << "Painting log view item delegate";
    if (!index.isValid()) {
        qCWarning(appLog) << "Invalid index";
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setOpacity(1);

    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    QWidget *wnd = DApplication::activeWindow();
    DPalette::ColorGroup cg;
    if (!(opt.state & DStyle::State_Enabled)) {
        cg = DPalette::Disabled;
        // qCDebug(appLog) << "Item is disabled";
    } else {
        if (!wnd) {
            cg = DPalette::Inactive;
            // qCDebug(appLog) << "Window is inactive";
        } else {
            cg = DPalette::Active;
            // qCDebug(appLog) << "Window is active";
        }
    }

    DStyle *style = dynamic_cast<DStyle *>(DApplication::style());
    if (!style) {
        qCWarning(appLog) << "Failed to get DStyle";
        return;
    }
    int margin = style->pixelMetric(DStyle::PM_ContentsMargins, &option);

    DGuiApplicationHelper *dAppHelper = DGuiApplicationHelper::instance();
    DPalette palette = dAppHelper->applicationPalette();
    QBrush background;
    QPen forground;
    if (opt.features & QStyleOptionViewItem::Alternate) {
        background = palette.color(cg, DPalette::AlternateBase);
    } else {
        background = palette.color(cg, DPalette::Base);
    }

    bool enableAndSelect = false;
    forground.setColor(palette.color(cg, DPalette::Text));
    if (opt.state & DStyle::State_Enabled) {
        if (opt.state & DStyle::State_Selected) {
            // qCDebug(appLog) << "Item is selected";
            background = palette.color(cg, DPalette::Highlight);
            forground.setColor(palette.color(cg, DPalette::HighlightedText));
            enableAndSelect = true;
        }
    }
    painter->setPen(forground);

    QRect rect = opt.rect;

    QFontMetrics fm(opt.font);
    QPainterPath path, clipPath;

    QRect textRect = rect;

    QRect iconRect = rect;
    if (opt.viewItemPosition == QStyleOptionViewItem::Beginning &&
            index.data(Qt::DecorationRole).isValid()) {
        // qCDebug(appLog) << "Painting icon for first item";
        iconRect.setX(rect.x() - margin);
        iconRect.setWidth(64);
        QIcon ic = index.data(Qt::DecorationRole).value<QIcon>();
        ic.paint(painter, iconRect);
    }

    if (opt.viewItemPosition == QStyleOptionViewItem::Beginning) {
        textRect.setX(textRect.x() + margin * 2);
    } else {
        textRect.setX(textRect.x() + margin);
    }

    QString text = fm.elidedText(opt.text, opt.textElideMode, textRect.width());

    QPen p = painter->pen();
    if (text.startsWith("(" + tr("Disable") + ")") && !enableAndSelect) {
        // qCDebug(appLog) << "Setting text color for disabled item";
        p.setColor(QColor("#FF5736"));
    }else if(text.startsWith("(" + tr("Unavailable") + ")")){
        // qCDebug(appLog) << "Setting text color for unavailable item";
        palette.color(cg, DPalette::PlaceholderText);
    }
    painter->setPen(p);
    painter->drawText(textRect, Qt::TextSingleLine | static_cast<int>(opt.displayAlignment), text);
    painter->restore();
}

QWidget *LogViewItemDelegate::createEditor(QWidget *, const QStyleOptionViewItem &,
                                           const QModelIndex &) const
{
    qCDebug(appLog) << "Creating editor, return nullptr";
    return nullptr;
}

QSize LogViewItemDelegate::sizeHint(const QStyleOptionViewItem &option,
                                    const QModelIndex &index) const
{
    // qCDebug(appLog) << "Getting size hint";
    QSize size = QStyledItemDelegate::sizeHint(option, index);
    size.setHeight(std::max(36, size.height()));
    return size;
}

void LogViewItemDelegate::initStyleOption(QStyleOptionViewItem *option,
                                          const QModelIndex &index) const
{
    // qCDebug(appLog) << "Initializing style options for row:" << index.row();
    option->showDecorationSelected = true;
    bool ok = false;
    if (index.data(Qt::TextAlignmentRole).isValid()) {
        // qCDebug(appLog) << "Text alignment role is valid";
        uint value = index.data(Qt::TextAlignmentRole).toUInt(&ok);
        option->displayAlignment = static_cast<Qt::Alignment>(value);
    }

    if (!ok) {
        // qCDebug(appLog) << "Using default alignment";
        option->displayAlignment = Qt::AlignLeft | Qt::AlignVCenter;
    }
    option->textElideMode = Qt::ElideRight;
    option->features = QStyleOptionViewItem::HasDisplay;
    if (index.row() % 2 == 0)
        option->features |= QStyleOptionViewItem::Alternate;

    if (index.data(Qt::DisplayRole).isValid())
        option->text = index.data().toString();
}
