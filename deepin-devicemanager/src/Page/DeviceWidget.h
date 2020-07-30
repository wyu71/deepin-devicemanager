#ifndef DETAILWIDGET_H
#define DETAILWIDGET_H

#include <QObject>
#include <DWidget>
#include <DSplitter>

class PageListView;
class PageInfoWidget;
class DeviceBaseInfo;

using namespace Dtk::Widget;

class DeviceWidget : public DWidget
{
    Q_OBJECT
public:
    DeviceWidget(QWidget *parent = nullptr);
    ~DeviceWidget();

    void updateListView(const QList<QPair<QString, QString> > &lst);
    void updateDevice(const QString &itemStr, const QList<DeviceBaseInfo *> &lst);
    void updateOverview(const QString &itemStr, const QMap<QString, QString> &map);
signals:
    void itemClicked(const QString &itemStr);
    void refreshInfo();
    void exportInfo();

private slots:
    void slotListViewWidgetItemClicked(const QString &itemStr);
    void slotRefreshInfo();
    void slotExportInfo();

private:
    /**@brief:初始化界面布局*/
    void initWidgets();

private:
    PageListView              *mp_ListView;          //<! 左边的list
    PageInfoWidget            *mp_PageInfo;          //<! 右边的详细内容
    DSplitter                 *mp_Splitter;          //<! 左右两边分页器
};

#endif // DETAILWIDGET_H
