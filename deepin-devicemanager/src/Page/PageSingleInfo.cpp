#include "PageSingleInfo.h"
#include "MacroDefinition.h"
#include "DeviceInfo.h"

#include <QVBoxLayout>


#include <DStandardItem>
#include <DTableWidget>
#include <DMenu>

PageSingleInfo::PageSingleInfo(QWidget *parent)
    : PageInfo(parent)
    , mp_Content(new DetailTreeView(this))
    , mp_Label(new DLabel(this))
    , mp_Refresh(new QAction(QIcon::fromTheme("view-refresh"), tr("Refresh (F5)"), this))
    , mp_Export(new QAction(QIcon::fromTheme("document-new"), tr("Export (E)"), this))
    , mp_Menu(new DMenu(this))
{
    initWidgets();

    mp_Content->setContextMenuPolicy(Qt::CustomContextMenu);
    bool a = connect(mp_Content, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(slotShowMenu(const QPoint &)));
    connect(mp_Refresh, &QAction::triggered, this, &PageSingleInfo::slotActionRefresh);
    connect(mp_Export, &QAction::triggered, this, &PageSingleInfo::slotActionExport);
}

PageSingleInfo::~PageSingleInfo()
{
    DELETE_PTR(mp_Content);
}

void PageSingleInfo::setLabel(const QString &itemstr)
{
    mp_Label->setText(itemstr);
    DFontSizeManager::instance()->bind(mp_Label, DFontSizeManager::T3);

}

void PageSingleInfo::updateInfo(const QList<DeviceBaseInfo *> &lst)
{
    clearContent();

    QList<QPair<QString, QString>> baseInfoMap = lst[0]->getBaseAttribs();
    QList<QPair<QString, QString>> otherInfoMap = lst[0]->getOtherAttribs();
    baseInfoMap = baseInfoMap + otherInfoMap;

    loadDeviceInfo(baseInfoMap);
}

void PageSingleInfo::loadDeviceInfo(const QList<QPair<QString, QString>> &lst)
{
    if (lst.size() < 1) {
        return;
    }

    int row = lst.size();
    mp_Content->setColumnAndRow(row + 1, 2);

    for (int i = 0; i < row; ++i) {

        QTableWidgetItem *itemFirst = new QTableWidgetItem(lst[i].first);
        mp_Content->setItem(i, 0, itemFirst);
        QTableWidgetItem *itemSecond = new QTableWidgetItem(lst[i].second);
        mp_Content->setItem(i, 1, itemSecond);
    }
}

void PageSingleInfo::clearContent()
{
    mp_Content->clear();
}

void PageSingleInfo::slotShowMenu(const QPoint &)
{
    mp_Menu->clear();
    mp_Menu->addSeparator();
    mp_Menu->addAction(mp_Refresh);
    mp_Menu->addAction(mp_Export);
    mp_Menu->exec(QCursor::pos());
}
void PageSingleInfo::slotActionRefresh()
{

}
void PageSingleInfo::slotActionExport()
{

}

void PageSingleInfo::initWidgets()
{
    QVBoxLayout *hLayout = new QVBoxLayout(this);
    hLayout->addWidget(mp_Label);
    hLayout->addWidget(mp_Content);
    hLayout->addStretch();
    setLayout(hLayout);
}
