// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// 项目自身文件
#include "KLUGenerator.h"
#include "DDLog.h"

// Qt库文件
#include <QLoggingCategory>
#include <QProcess>

// 其它头文件
#include "DeviceManager/DeviceManager.h"
#include "DeviceManager/DeviceGpu.h"
#include "DeviceManager/DeviceMonitor.h"
#include "DeviceManager/DeviceOthers.h"
#include "DeviceManager/DeviceStorage.h"
#include "DeviceManager/DeviceAudio.h"
#include "DeviceManager/DeviceComputer.h"
#include "DeviceManager/DevicePower.h"
#include "DeviceManager/DeviceInput.h"
#include "DeviceManager/DeviceNetwork.h"

using namespace DDLog;

KLUGenerator::KLUGenerator()
{
    qCDebug(appLog) << "KLUGenerator constructor";

}

void KLUGenerator::getDiskInfoFromLshw()
{
    qCDebug(appLog) << "KLUGenerator::getDiskInfoFromLshw start";
    QString bootdevicePath("/proc/bootdevice/product_name");
    QString modelStr = "";
    QFile file(bootdevicePath);
    if (file.open(QIODevice::ReadOnly)) {
        qCDebug(appLog) << "KLUGenerator::getDiskInfoFromLshw open bootdevice file success";
        modelStr = file.readLine().simplified();
        file.close();
    }

    const QList<QMap<QString, QString>> lstDisk = DeviceManager::instance()->cmdInfo("lshw_disk");
    QList<QMap<QString, QString> >::const_iterator dIt = lstDisk.begin();
    for (; dIt != lstDisk.end(); ++dIt) {
        // qCDebug(appLog) << "KLUGenerator::getDiskInfoFromLshw process lshw disk info";
        if ((*dIt).size() < 2) {
            // qCDebug(appLog) << "KLUGenerator::getDiskInfoFromLshw disk info not enough";
            continue;
        }

        // KLU的问题特殊处理
        QMap<QString, QString> tempMap;
        foreach (const QString &key, (*dIt).keys()) {
            tempMap.insert(key, (*dIt)[key]);
        }

//        qCInfo(appLog) << tempMap["product"] << " ***** " << modelStr << " " << (tempMap["product"] == modelStr);
        // HW写死
        if (tempMap["product"] == modelStr) {
            // qCDebug(appLog) << "KLUGenerator::getDiskInfoFromLshw find boot device";
            // 应HW的要求，将描述固定为   Universal Flash Storage
            tempMap["description"] = "Universal Flash Storage";
            // 应HW的要求，添加interface   UFS 3.0
            tempMap["interface"] = "UFS 3.0";

            // 读取interface版本
            QProcess process;
            process.start("cat /sys/devices/platform/f8200000.ufs/host0/scsi_host/host0/wb_en");
            process.waitForFinished(-1);
            int exitCode = process.exitCode();
            if (exitCode != 127 && exitCode != 126) {
                // qCDebug(appLog) << "KLUGenerator::getDiskInfoFromLshw get wb_en success";
                QString deviceInfo = process.readAllStandardOutput();
                if (deviceInfo.trimmed() == "true") {
                    // qCDebug(appLog) << "KLUGenerator::getDiskInfoFromLshw wb_en is true";
                    process.start("cat /sys/block/sdd/device/spec_version");
                    process.waitForFinished(-1);
                    exitCode = process.exitCode();
                    if (exitCode != 127 && exitCode != 126) {
                        // qCDebug(appLog) << "KLUGenerator::getDiskInfoFromLshw get spec_version success";
                        deviceInfo = process.readAllStandardOutput();
                        if (deviceInfo.trimmed() == "310") {
                            // qCDebug(appLog) << "KLUGenerator::getDiskInfoFromLshw spec_version is 310, set interface to UFS 3.1";
                            tempMap["interface"] = "UFS 3.1";
                        }
                    }
                }
            }
        }

        DeviceManager::instance()->addLshwinfoIntoStorageDevice(tempMap);
    }
    qCDebug(appLog) << "KLUGenerator::getDiskInfoFromLshw end";
}

void KLUGenerator::generatorNetworkDevice()
{
    qCDebug(appLog) << "KLUGenerator::generatorNetworkDevice start";
    const QList<QMap<QString, QString>> lstInfo = DeviceManager::instance()->cmdInfo("lshw_network");
    QList<QMap<QString, QString> >::const_iterator it = lstInfo.begin();
    for (; it != lstInfo.end(); ++it) {
        // qCDebug(appLog) << "KLUGenerator::generatorNetworkDevice process lshw network info";
        if ((*it).size() < 2) {
            // qCDebug(appLog) << "KLUGenerator::generatorNetworkDevice network info not enough";
            continue;
        }
        QMap<QString, QString> tempMap = *it;
        /*
        capabilities: ethernet physical wireless
        configuration: broadcast=yes ip=10.4.6.115 multicast=yes wireless=IEEE 802.11
        */
        if ((tempMap["configuration"].indexOf("wireless=IEEE 802.11") > -1) ||
                (tempMap["capabilities"].indexOf("wireless") > -1)) {
            // qCDebug(appLog) << "KLUGenerator::generatorNetworkDevice is wireless interface, skip";
            continue;
        }

        DeviceNetwork *device = new DeviceNetwork();
        device->setInfoFromLshw(*it);
        device->setCanEnale(false);
        device->setCanUninstall(false);
        device->setForcedDisplay(true);
        DeviceManager::instance()->addNetworkDevice(device);
    }
    // HW 要求修改名称,制造商以及类型
    getNetworkInfoFromCatWifiInfo();
    qCDebug(appLog) << "KLUGenerator::generatorNetworkDevice end";
}

void KLUGenerator::getNetworkInfoFromCatWifiInfo()
{
    qCDebug(appLog) << "KLUGenerator::getNetworkInfoFromCatWifiInfo start";
    QList<QMap<QString, QString> >  lstWifiInfo;
    QString wifiDevicesInfoPath("/sys/hisys/wal/wifi_devices_info");
    QFile file(wifiDevicesInfoPath);
    if (file.open(QIODevice::ReadOnly)) {
        qCDebug(appLog) << "KLUGenerator::getNetworkInfoFromCatWifiInfo open wifi info file success";
        QMap<QString, QString>  wifiInfo;
        QString allStr = file.readAll();
        file.close();

        // 解析数据
        QStringList items = allStr.split("\n");
        foreach (const QString &item, items) {
            if (item.isEmpty())
                continue;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            QStringList strList = item.split(':', QString::SkipEmptyParts);
#else
            QStringList strList = item.split(':', Qt::SkipEmptyParts);
#endif
            if (strList.size() == 2) {
                // qCDebug(appLog) << "KLUGenerator::getNetworkInfoFromCatWifiInfo get wifi info:" << strList[0] << strList[1];
                wifiInfo[strList[0] ] = strList[1];
            }
        }

        if (!wifiInfo.isEmpty()) {
            // qCDebug(appLog) << "KLUGenerator::getNetworkInfoFromCatWifiInfo wifi info not empty, add to list";
            lstWifiInfo.append(wifiInfo);
        }
    } else {
        qCWarning(appLog) << "KLUGenerator::getNetworkInfoFromCatWifiInfo open wifi info file failed";
    }
    if (lstWifiInfo.size() == 0) {
        qCWarning(appLog) << "KLUGenerator::getNetworkInfoFromCatWifiInfo no wifi info";
        return;
    }
    QList<QMap<QString, QString> >::const_iterator it = lstWifiInfo.begin();

    for (; it != lstWifiInfo.end(); ++it) {
        // qCDebug(appLog) << "KLUGenerator::getNetworkInfoFromCatWifiInfo process wifi info";
        if ((*it).size() < 3) {
            // qCDebug(appLog) << "KLUGenerator::getNetworkInfoFromCatWifiInfo wifi info not enough";
            continue;
        }

        // KLU的问题特殊处理
        QMap<QString, QString> tempMap;
        foreach (const QString &key, (*it).keys()) {
            tempMap.insert(key, (*it)[key]);
        }

        // cat /sys/hisys/wal/wifi_devices_info  获取结果为 HUAWEI HI103
        if (tempMap["Chip Type"].contains("HUAWEI", Qt::CaseInsensitive)) {
            // qCDebug(appLog) << "KLUGenerator::getNetworkInfoFromCatWifiInfo fix chip type";
            tempMap["Chip Type"] = tempMap["Chip Type"].remove("HUAWEI").trimmed();
        }

        // 按照华为的需求，设置蓝牙制造商和类型
        tempMap["Vendor"] = "HISILICON";
        tempMap["Type"] = "Wireless network";

        DeviceManager::instance()->setNetworkInfoFromWifiInfo(tempMap);
    }
    qCDebug(appLog) << "KLUGenerator::getNetworkInfoFromCatWifiInfo end";
}

void KLUGenerator::generatorMonitorDevice()
{
    qCDebug(appLog) << "KLUGenerator::generatorMonitorDevice start";
    QMap<QString, QString> mapInfo;
    mapInfo.insert("Name", "LCD");
//    mapInfo.insert("Vendor", "HUAWEI");
    mapInfo.insert("CurResolution", "2160x1440@60Hz");
    mapInfo.insert("SupportResolution", "2160x1440@60Hz");
    mapInfo.insert("Size", "14 Inch");
//    mapInfo.insert("Date", "2019年7月");

    DeviceMonitor *monitor = new  DeviceMonitor();
    monitor->setInfoFromSelfDefine(mapInfo);
    DeviceManager::instance()->addMonitor(monitor);
    qCDebug(appLog) << "KLUGenerator::generatorMonitorDevice end";
}

