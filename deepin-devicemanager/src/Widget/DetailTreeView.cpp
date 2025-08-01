// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// 项目自身文件
#include "DetailTreeView.h"
#include "DDLog.h"

// Qt库文件
#include <QHeaderView>
#include <QPainter>
#include <QKeyEvent>
#include <QHBoxLayout>
#include <QLoggingCategory>
#include <QThread>
#include <QToolTip>
#include <QDateTime>
#include <QTimer>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QDesktopWidget>
#else
#include <QGuiApplication>
#include <QScreen>
#endif

#include <QPainterPath>

// Dtk头文件
#include <DApplication>
#include <DGuiApplicationHelper>
#include <DStyle>
#include <DFontSizeManager>

// 其它头文件
#include "PageDetail.h"
#include "DetailViewDelegate.h"
#include "MacroDefinition.h"
#include "PageInfo.h"
#include "PageTableWidget.h"
#include "CmdButtonWidget.h"
#include "TipsWidget.h"

using namespace DDLog;

BtnWidget::BtnWidget()
{

}

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
void BtnWidget::enterEvent(QEvent *event)
{
    qCDebug(appLog) << "Mouse entered button widget";
{
    emit enter();
    return DWidget::enterEvent(event);
}
#else
void BtnWidget::enterEvent(QEnterEvent *event)
{
    emit enter();
    return DWidget::enterEvent(event);
}
#endif

void BtnWidget::leaveEvent(QEvent *event)
{
    emit leave();
    return DWidget::leaveEvent(event);
}

DetailTreeView::DetailTreeView(DWidget *parent)
    : DTableWidget(parent)
    , mp_ItemDelegate(nullptr)
    , mp_CommandBtn(nullptr)
    , m_LimitRow(13)
    , m_IsExpand(false)
    , m_IsEnable(true)
    , m_IsAvailable(true)
    , mp_OldItem(nullptr)
    , mp_CurItem(nullptr)
    , m_TimeStep(0)
    , mp_Timer(new QTimer(this))
    , mp_ToolTips(nullptr)
{
    qCDebug(appLog) << "DetailTreeView constructor called";
    setMouseTracking(true);
    // 初始化界面
    initUI();

    // 连接槽函数
    connect(mp_Timer, &QTimer::timeout, this, &DetailTreeView::slotTimeOut);
    connect(this, &DetailTreeView::itemEntered, this, &DetailTreeView::slotItemEnterd);

    // 启动定时器
    mp_Timer->start(100);
}

void DetailTreeView::setColumnAndRow(int row, int column)
{
    qCDebug(appLog) << "Setting table dimensions. Rows:" << row << "Columns:" << column;

    // 设置表格行数列数
    setRowCount(row);
    setColumnCount(column);

    // 当前页为主板页面时,且信息已展开,展示更多/收起按钮
    PageTableWidget *pageTableWidget = dynamic_cast<PageTableWidget *>(this->parent());
    if (!pageTableWidget) {
        qCDebug(appLog) << "Parent is not a PageTableWidget, returning.";
        return;
    }
    if (pageTableWidget->isBaseBoard() && m_IsExpand) {
        qCDebug(appLog) << "Current page is baseboard, and information is expanded, showing command link button.";
        setCommanLinkButton(row);
        showRow(row - 1);
    } else {
        qCDebug(appLog) << "Table row count is greater than the limit, adding expand button";
        // 表格行数大于限制行数时，添加展开button
        if (row > m_LimitRow + 1) {
            setCommanLinkButton(row);
        }
        // 如果行数少于限制行数，则影藏最后一行
        if (row <= m_LimitRow + 1) {
            qCDebug(appLog) << "The number of rows in the table is less than the limit, and the last row is hidden";
            hideRow(row - 1);
        }
    }
}

void DetailTreeView::setItem(int row, int column, QTableWidgetItem *item)
{
    qCDebug(appLog) << "Setting item at row:" << row << "column:" << column;

    // 设置表格高度
    setFixedHeight(ROW_HEIGHT * std::min((row + 1), m_LimitRow + 1));

    // 添加表格内容
    DTableWidget::setItem(row, column, item);

    // 行数大于限制行数隐藏信息，展示展开button
    if (!m_IsExpand) {
        if (row >= m_LimitRow) {
            qCDebug(appLog) << "The number of rows is greater than the limit, hide the information, and display the expand button";
            hideRow(row);
            showRow(this->rowCount() - 1);
        }
    }
}

void DetailTreeView::clear()
{
    qCDebug(appLog) << "Clearing table contents";

    mp_OldItem = nullptr;
    mp_CurItem = nullptr;
    // 清空表格内容
    DTableWidget::clear();

    // 删除表格行列
    setRowCount(0);
    setColumnCount(0);

    if (mp_CommandBtn != nullptr) {
        qCDebug(appLog) << "Deleting command button widget";
        delete mp_CommandBtn;
        mp_CommandBtn = nullptr;
    }
}

void DetailTreeView::setCommanLinkButton(int row)
{
    qCDebug(appLog) << "Setting command link button at row:" << row;

    // 设置mp_CommandBtn属性
    mp_CommandBtn = new DCommandLinkButton(tr("More"), this);

    // 当页面已展开按钮文字为收起
    if (m_IsExpand) {
        mp_CommandBtn->setText(tr("Collapse"));
    }
    // 设置字号
    DFontSizeManager::instance()->bind(mp_CommandBtn, DFontSizeManager::T8);
    //  合并最后一行
    this->setSpan(row - 1, 0, 1, 2);

    // 将mp_CommandBtn放到布局中，居中
    QHBoxLayout *pHBoxLayout = new QHBoxLayout();
    pHBoxLayout->addStretch();
    pHBoxLayout->addWidget(mp_CommandBtn);
    pHBoxLayout->addStretch();

    QVBoxLayout *pVBoxLayout = new QVBoxLayout();
    pVBoxLayout->addLayout(pHBoxLayout);

    // 新建放置按钮的widget,并设置鼠标出入对应的槽函数
    BtnWidget *btnwidget = new BtnWidget();
    connect(btnwidget, &BtnWidget::enter, this, &DetailTreeView::slotEnterBtnWidget);
    connect(btnwidget, &BtnWidget::leave, this, &DetailTreeView::slotLeaveBtnWidget);
    btnwidget->setLayout(pVBoxLayout);

    // 将btnwidget填充到表格中，并隐藏
    setCellWidget(row - 1, 1, btnwidget);

    // 点击按钮槽函数
    connect(mp_CommandBtn, &DCommandLinkButton::clicked, this, &DetailTreeView::expandCommandLinkClicked);
}

int DetailTreeView::setTableHeight(int paintHeight)
{
    qCDebug(appLog) << "Setting table height:" << paintHeight << "Device enabled:" << m_IsEnable << "Available:" << m_IsAvailable;

    // 设备禁用状态下,只显示一行
    if (!m_IsEnable  || !m_IsAvailable) {
        qCDebug(appLog) << "Device is disabled or unavailable, setting height to 40";
        paintHeight = 40;
        this->setFixedHeight(paintHeight);
        return paintHeight;
    }

    // 父窗口
    PageTableWidget *pageTableWidget = dynamic_cast<PageTableWidget *>(this->parent());
    if (!pageTableWidget) {
        qCWarning(appLog) << "Parent is not a PageTableWidget, cannot set table height.";
        return -1;
    }
    PageInfo *par = dynamic_cast<PageInfo *>(pageTableWidget->parent());
    if (!par) {
        qCWarning(appLog) << "Parent is not a PageInfo, cannot set table height.";
        return -1;
    }
    // 父窗口可显示的最大表格行数
    // 最多显示行数与父窗口高度相关,需减去Label以及Spacing占用空间
    int maxRow = 0;
    maxRow = par->height() / ROW_HEIGHT - 2;

    // 当前页面为概况时，展开更多信息，页面显示的表格的最大行数需减一，避免表格边框显示不完整
    if (pageTableWidget->isOverview()) {
        // 有更多信息并且已展开
        if (hasExpendInfo() && m_IsExpand) {
            qCDebug(appLog) << "The current page is the overview, expand more information, the maximum number of rows displayed in the table needs to be reduced by one";
            --maxRow;
        }
    }

    // 主板界面的表格高度
    if (pageTableWidget->isBaseBoard()) {
        qCDebug(appLog) << "The current page is the base board, setting table height";
        // 表格未展开
        if (false == m_IsExpand) {
            this->setFixedHeight(ROW_HEIGHT * (m_LimitRow + 1));
            return this->height();
        } else {
            qCDebug(appLog) << "The table is expanded, and the parent window can accommodate the maximum height";
            // 表格展开,父窗口最大容纳高度
            this->setFixedHeight(ROW_HEIGHT * maxRow);
            return this->height();
        }
    }

    // 处理多行
    if (par->getMultiFlag()) {
        qCDebug(appLog) << "The table is expanded, and the parent window can accommodate the maximum height";
        // 表格展开,父窗口最大容纳高度
        this->setFixedHeight(ROW_HEIGHT * maxRow);
        return this->height();
    }

    // 信息行 <= m_LimitRow + 1 不影响表格大小
    if (rowCount() <= m_LimitRow + 1) {
        qCDebug(appLog) << "Row count is less than or equal to the limit, setting height to single row height";
        this->setFixedHeight((rowCount() - 1) * ROW_HEIGHT);
        return (rowCount() - 1) * ROW_HEIGHT;
    } else {
        qCDebug(appLog) << "Row count is greater than the limit, and the table is not expanded";
        // 未展开,窗口高度始终等于ROW_HEIGHT * (m_LimitRow+1)
        if (false == m_IsExpand) {
            this->setFixedHeight(ROW_HEIGHT * (m_LimitRow + 1));
            return this->height();

        } else {
            // 展开，表格高度<父窗口高度
            this->setFixedHeight(ROW_HEIGHT * std::min(rowCount(), maxRow));
            return this->height();
        }
    }
}

bool DetailTreeView::hasExpendInfo()
{
    qCDebug(appLog) << "Checking if there is expandable information";
    // 指针不为空，设备有其他信息
    if (mp_CommandBtn != nullptr) {
        qCDebug(appLog) << "Command button widget is not null, expandable information exists";
        return true;
    } else {
        qCDebug(appLog) << "Command button widget is null, no expandable information";
        // 指针为空，设备没有更多信息
        return false;
    }
}

void DetailTreeView::setLimitRow(int row)
{
    qCDebug(appLog) << "Setting limit row:" << row;

    // 设置页面显示行数
    m_LimitRow = row;

    // 父窗口
    PageTableWidget *p = dynamic_cast<PageTableWidget *>(this->parent());
    PageInfo *par = dynamic_cast<PageInfo *>(p->parent());

    // 最多显示行数与父窗口高度相关,需减去Label以及Spacing占用空间
    int maxRow = par->height() / ROW_HEIGHT - 3;

    // 主板界面可显示的行数与主板信息内容有关
    if (p->isBaseBoard() && m_IsExpand) {
        qCDebug(appLog) << "Current page is base board, setting limit row based on device info number";
        m_LimitRow = std::min(maxRow, par->getDeviceInfoNum());
    }
}

QString DetailTreeView::toString()
{
    // qCDebug(appLog) << "Converting table content to string";
    // 表格行内容转为字符串
    QString str;

    // 表格行,列
    int row = rowCount();
    int column = columnCount();

    // 遍历所有行
    for (int i = 0; i < row; i++) {
        for (int j = 0; j < column; j++) {
            QTableWidgetItem *sItem = this->item(i, j);
            if (sItem) {
                // 第一列内容后加冒号,否则后面加换行符
                QString se = (j == 0) ? " : " : "\n";
                str += sItem->text() + se;
            }
        }
    }
    return str;
}

bool DetailTreeView::isCurDeviceEnable()
{
    // qCDebug(appLog) << "Checking if the current device is enabled. Current state:" << m_IsEnable;
    return m_IsEnable;
}

bool DetailTreeView::isCurDeviceAvailable()
{
    // qCDebug(appLog) << "Checking if the current device is available. Current state:" << m_IsAvailable;
    return m_IsAvailable;
}

void DetailTreeView::setCurDeviceState(bool enable, bool available)
{
    qCDebug(appLog) << "Setting device state. Enabled:" << enable << "Available:" << available;

    // 设置当前设备状态
    m_IsEnable = enable;
    m_IsAvailable = available;

    // 禁用状态
    if (!m_IsEnable || !m_IsAvailable) {
        qCDebug(appLog) << "Device is disabled or unavailable, hiding all rows except the first";
        // 隐藏除第一行以外的所有行
        for (int i = 1; i < this->rowCount(); ++i) {
            this->hideRow(i);
        }
        qCDebug(appLog) << "The device is disabled, hiding all rows except the first";

        // 设置表格高度为单行高度
        this->setTableHeight(ROW_HEIGHT);
    } else {
        qCDebug(appLog) << "The device is enabled, setting the table height";
        // 启用状态
        this->setTableHeight(ROW_HEIGHT);

        // 展开状态，且有更多信息
        if (m_IsExpand && hasExpendInfo()) {
            for (int i = 1; i < this->rowCount(); ++i) {
                this->showRow(i);
            }
        }

        // 未展开,但是由更多信息
        if (!m_IsExpand && hasExpendInfo()) {
            qCDebug(appLog) << "Not expanded, but there is more information, displayed according to the restricted number of lines";

            // 按照限制行数显示
            for (int i = 1; i < this->m_LimitRow; ++i) {
                this->showRow(i);
            }

            // 显示展开按钮
            this->showRow(this->rowCount() - 1);
        }

        // 没有更多信息
        if (!hasExpendInfo()) {
            qCDebug(appLog) << "No more information, show all information, but not the last line";

            // 显示所有信息,但不显示最后一行
            for (int i = 1; i < this->rowCount() - 1; ++i) {
                this->showRow(i);
            }
            this->hideRow(this->rowCount() - 1);
        }
    }
}

bool DetailTreeView::isExpanded()
{
    // qCDebug(appLog) << "Checking if the table is expanded. Current state:" << m_IsExpand;
    return m_IsExpand;
}

void DetailTreeView::expandCommandLinkClicked()
{
    qCInfo(appLog) << "Expand/collapse button clicked. Current state:" << m_IsExpand;

    // 当前已展开详细信息
    if (m_IsExpand) {
        qCDebug(appLog) << "Current is expanded, collapsing the details";
        mp_CommandBtn->setText(tr("More"));
        m_IsExpand = false;

        for (int i = m_LimitRow; i < rowCount() - 1; ++i) {
            hideRow(i);
        }
    } else { // 当前未展开详细信息
        qCDebug(appLog) << "Current is not expanded, expanding the details";
        mp_CommandBtn->setText(tr("Collapse"));
        m_IsExpand = true;

        for (int i = m_LimitRow; i < rowCount() - 1; ++i) {
            showRow(i);
        }
    }
}

void DetailTreeView::initUI()
{
    qCDebug(appLog) << "Initializing UI components";

    // 设置Item自定义代理
    mp_ItemDelegate = new DetailViewDelegate(this);
    setItemDelegate(mp_ItemDelegate);

    // 设置不可编辑模式
    this->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // 设置表格一次滚动一个Item，dtk默认一次滚动一个像素
    this->setVerticalScrollMode(QAbstractItemView::ScrollMode::ScrollPerPixel);

    // 设置不可选择
//    setSelectionBehavior(QAbstractItemView::SelectItems);
    setSelectionMode(QAbstractItemView::NoSelection);

    // 设置无边框
    this->setFrameStyle(QFrame::NoFrame);

    // 隐藏网格线
    this->setShowGrid(false);
    this->viewport()->setAutoFillBackground(true);

    // 设置各行变色
    setAlternatingRowColors(true);

    // 隐藏表头
    this->verticalHeader()->setVisible(false);
    this->horizontalHeader()->setVisible(false);

    // 设置不可排序
    this->setSortingEnabled(false);

    // 设置最后一个section拉伸填充
    this->horizontalHeader()->setStretchLastSection(true);

    // 设置行高
    this->verticalHeader()->setDefaultSectionSize(ROW_HEIGHT);

    // 设置section行宽
    this->horizontalHeader()->setDefaultSectionSize(180);

    this->setAttribute(Qt::WA_TranslucentBackground);//设置窗口背景透明
    this->setWindowFlags(Qt::FramelessWindowHint);   //设置无边框窗口
}

void DetailTreeView::paintEvent(QPaintEvent *event)
{
    // qCDebug(appLog) << "DetailTreeView paint event called.";
    DTableView::paintEvent(event);

    QWidget *wnd = DApplication::activeWindow();
    DPalette::ColorGroup cg;
    if (!wnd) {
        cg = DPalette::Inactive;
        // qCDebug(appLog) << "The window is not activated";
    } else {
        // qCDebug(appLog) << "The window is activated";
        cg = DPalette::Active;
    }

    auto *dAppHelper = DGuiApplicationHelper::instance();
    auto palette = dAppHelper->applicationPalette();

    QRect rect = viewport()->rect();
    int pHeight = setTableHeight(rect.height());

    // 窗口大小发生变化时，需重新设置表格大小
    this->setFixedHeight(pHeight);
    rect = viewport()->rect();

    QPainter painter(this->viewport());
    painter.save();

    // 绘制表格外边框采用填充方式,通过内矩形与外矩形相差得到填充区域
    int width = 1;
    int radius = 8;
    QPainterPath paintPath, paintPathOut, paintPathIn;

    // 外圆角矩形路径,即表格的外边框路径
    paintPathOut.addRoundedRect(rect, radius, radius);

    // 内圆角矩形路径,与外圆角矩形上下左右相差1个像素
    QRect rectIn = QRect(rect.x() + width, rect.y() + width, rect.width() - width * 2, rect.height() - width * 2);
    paintPathIn.addRoundedRect(rectIn, radius, radius);

    // 填充路径
    paintPath = paintPathOut.subtracted(paintPathIn);

    // 填充
    QBrush bgBrush(palette.color(cg, DPalette::FrameBorder));
    painter.fillPath(paintPath, bgBrush);

    QPen pen = painter.pen();
    pen.setWidth(1);
    QColor frameColor = palette.color(cg, DPalette::FrameBorder);
    pen.setColor(frameColor);
    painter.setPen(pen);

    QLine line(rect.topLeft().x() + 179, rect.topLeft().y(), rect.bottomLeft().x() + 179, rect.bottomLeft().y());

    // 有展开信息，且未展开，button行不绘制竖线
    if (this->hasExpendInfo() && !m_IsExpand) {
        line.setP2(QPoint(rect.bottomLeft().x() + 179, rect.bottomLeft().y() - 40));
        // qCDebug(appLog) << "There is expanded information, and it is not expanded, the button line does not draw a vertical line";

        // 绘制横线
        if (m_IsEnable || m_IsAvailable) {
            QLine hline(rect.bottomLeft().x(), rect.bottomLeft().y() - 39, rect.bottomRight().x(), rect.bottomRight().y() - 39);
            painter.drawLine(hline);
        }

    } else if (hasExpendInfo() && m_IsExpand) {
        QTableWidgetItem *it = this->itemAt(QPoint(this->rect().bottomLeft().x(), this->rect().bottomLeft().y()));
        if (nullptr == it) {
            // qCDebug(appLog) << "The expansion button row starts to appear in the visible area";
            // 由于展开按钮行是DWidget无法获取item，所以，再在这种情况下，展开按钮行开始出现再可视区域
            for (int i = 1; i <= 40; ++i) {

                // 获取上一行item的下边距离表格边框的像素距离
                QTableWidgetItem *lastItem = this->itemAt(QPoint(this->rect().bottomLeft().x(), this->rect().bottomLeft().y() - i));
                if (lastItem != nullptr) {
                    // 竖线再 button行不显示
                    line.setP2(QPoint(rect.bottomLeft().x() + 179, rect.bottomLeft().y() - i));

                    // 绘制横线
                    if (m_IsEnable || m_IsAvailable) {
                        // qCDebug(appLog) << "Draw a horizontal line";
                        QLine hline(rect.bottomLeft().x(), rect.bottomLeft().y() - i + 1, rect.bottomRight().x(), rect.bottomRight().y() - i + 1);
                        painter.drawLine(hline);
                    }

                    break;

                }
            }
        }
    }
    // 控制透明度大于等于20
    if (frameColor.alpha() < 20)
        frameColor.setAlpha(20);
    pen.setColor(frameColor);
    painter.setPen(pen);
    painter.drawLine(line);
    painter.restore();
}

void DetailTreeView::resizeEvent(QResizeEvent *event)
{
    // qCDebug(appLog) << "DetailTreeView resize event called. New size:" << event->size();
    DTableWidget::resizeEvent(event);

    // 解决　调整窗口大小时tooltip未及时刷新
    QPoint pt = this->mapFromGlobal(QCursor::pos());
    mp_CurItem = itemAt(pt);
}

void DetailTreeView::mousePressEvent(QMouseEvent *event)
{
    // qCDebug(appLog) << "DetailTreeView mouse press event called. Button:" << event->button();
    // 鼠标右键事件
    if (event->button() == Qt::RightButton) {
        if (mp_ToolTips) {
            // 隐藏toopTips
            mp_CurItem = nullptr;
            mp_ToolTips->hide();
            // qCDebug(appLog) << "Right mouse button clicked, hiding tooltips";
        }
    }
    DTableWidget::mousePressEvent(event);
}

void DetailTreeView::mouseMoveEvent(QMouseEvent *event)
{
    // qCDebug(appLog) << "DetailTreeView mouse move event called. Position:" << event->pos();
    // 鼠标移动获取位置
    mp_Point = event->pos();
    DTableWidget::mouseMoveEvent(event);
}

void DetailTreeView::leaveEvent(QEvent *event)
{
    // qCDebug(appLog) << "DetailTreeView mouse leave event called.";
    // 鼠标移出事件
    if (mp_ToolTips) {
        // 隐藏toopTips
        mp_CurItem = nullptr;
        mp_ToolTips->hide();
        qCDebug(appLog) << "Mouse left, hiding tooltips";
    }
    DTableWidget::leaveEvent(event);
}

void DetailTreeView::slotTimeOut()
{
    // qCDebug(appLog) << "DetailTreeView timeout event called.";
    // tooltips显示当前Item内容
    if (this->isActiveWindow()) {
        // qCDebug(appLog) << "Window is active, showing tooltips for current item";
        showTips(mp_CurItem);
    } else {// 如果窗口不是激活状态，则影藏tips
        if (mp_ToolTips) {
            mp_ToolTips->hide();
            // qCDebug(appLog) << "Window is not active, hiding tooltips";
        }
    }
}

void DetailTreeView::slotItemEnterd(QTableWidgetItem *item)
{
    // qCDebug(appLog) << "Item entered event called. Item:" << item;
    // 设置当前鼠标所在位置Item
    mp_CurItem = item;
}

void DetailTreeView::slotEnterBtnWidget()
{
    // qCDebug(appLog) << "Mouse entered button widget, setting current item to nullptr";
    // 鼠标进入BtnWidget,当前Item设置为nullptr,防止tooltips显示
    mp_CurItem = nullptr;
}

void DetailTreeView::slotLeaveBtnWidget()
{
    // qCDebug(appLog) << "Mouse left button widget, getting current item based on mouse position";
    // 鼠标移出btnWidget,根据鼠标位置获取当前item
    QPoint pt = this->mapFromGlobal(QCursor::pos());
    mp_CurItem = itemAt(pt);
}

void DetailTreeView::showTips(QTableWidgetItem *item)
{
    qCDebug(appLog) << "Showing tooltips for item:" << item;
    // 确保tooltips Widget有效存在
    if (!mp_ToolTips) {
        qCDebug(appLog) << "Creating new TipsWidget instance";
        mp_ToolTips = new TipsWidget(this);
    }

    // 当前Item不为空且与上一个显示的Item一致
    if (item && item == mp_OldItem) {
        // 通过计时器控制toolTips的刷新
        qint64 curMS = QDateTime::currentDateTime().toMSecsSinceEpoch();
        if (curMS - m_TimeStep > 1000 && mp_ToolTips->isHidden()) {
            qCDebug(appLog) << "Show tooltips";
            // tooltips内容
            QString text = item->text();

            // 设置toolTips显示位置
            QPoint showRealPos(QCursor::pos().x(), QCursor::pos().y() + 10);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            QRect screenRect = QApplication::desktop()->screenGeometry();
#else
            QRect screenRect = QGuiApplication::primaryScreen()->geometry();
#endif
            if (mp_ToolTips) {
                // 确保tips不会出现一半在窗口，一半看不到
                if (showRealPos.x() + mp_ToolTips->width() > screenRect.width()) {
                    showRealPos.setX(showRealPos.x() - mp_ToolTips->width());
                    qCDebug(appLog) << "Adjust tooltips position to prevent overflow";
                }

                // 显示toolTips
                mp_ToolTips->setText(text);
                mp_ToolTips->move(showRealPos);
                mp_ToolTips->show();
            }
        }
    } else {
        qCDebug(appLog) << "Reset timer and hide tooltips";
        // 重新计时,等待下一个tooltips的显示
        m_TimeStep = QDateTime::currentDateTime().toMSecsSinceEpoch();
        mp_OldItem = item;

        // 隐藏tooltips
        if (mp_ToolTips) {
            mp_ToolTips->hide();
        }
    }
}
