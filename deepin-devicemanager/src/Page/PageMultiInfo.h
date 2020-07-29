#ifndef DEVICEPAGE_H
#define DEVICEPAGE_H

#include <QObject>
#include <DWidget>

#include "PageInfo.h"

class PageTableHeader;
class PageDetail;
class DeviceBaseInfo;

using namespace Dtk::Widget;
class PageMultiInfo : public PageInfo
{
public:
    PageMultiInfo(QWidget *parent = nullptr);
    ~PageMultiInfo() override;


    void updateInfo(const QList<DeviceBaseInfo *> &lst)override;


    void setLabel(const QString &itemstr) override;

private slots:
    void slotItemClicked(int row);

private:
    /**
     * @brief initWidgets : 初始化控件布局
     */
    void initWidgets();

private:
    PageTableHeader           *mp_Table;       //<! 上面的表格
    PageDetail                *mp_Detail;      //<! 下面的详细内容
};

#endif // DEVICEPAGE_H