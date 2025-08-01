// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// 项目自身文件
#include "DeviceMonitor.h"
#include "EDIDParser.h"
#include "commonfunction.h"
#include "DDLog.h"

#include <DApplication>

// Qt库文件
#include <QLoggingCategory>
#include <QDate>
#include <QSize>
#include <QRegularExpression>
// 其它头文件
#include <math.h>

DWIDGET_USE_NAMESPACE
using namespace DDLog;

DeviceMonitor::DeviceMonitor()
    : DeviceBaseInfo()
    , m_Model("")
    , m_DisplayInput("")
    , m_VGA("Disable")
    , m_HDMI("Disable")
    , m_DVI("Disable")
    , m_Interface("")
    , m_ScreenSize("")
    , m_AspectRatio("")
    , m_MainScreen("")
    , m_CurrentResolution("")
    , m_SerialNumber("")
    , m_ProductionWeek("")
    , m_Width(0)
    , m_Height(0)
    , m_IsTomlSet(false)
{
    qCDebug(appLog) << "DeviceMonitor constructor initialized";
    // 初始化可显示属性
    initFilterKey();
}

// 获得显示屏的大小
QString DeviceMonitor::parseMonitorSize(const QString &sizeDescription, double &inch, QSize &retSize)
{
    qCDebug(appLog) << "Parsing monitor size from description:" << sizeDescription;
    inch = 0.0;

    // 根据不同的正则表达式解析屏幕大小字符串
    QString res = sizeDescription;

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QRegExp re("^([\\d]*)x([\\d]*) mm$");
    if (re.exactMatch(sizeDescription)) {
        qCDebug(appLog) << "Matched size description with format: ^([\\d]*)x([\\d]*) mm$";
        // 获取屏幕宽高 int
        m_Width = re.cap(1).toInt();
        m_Height = re.cap(2).toInt();
        retSize = QSize(m_Width, m_Height);

        // 获取屏幕尺寸大小 inch
        double width = m_Width / 2.54;
        double height = m_Height / 2.54;
        inch = std::sqrt(width * width + height * height) / 10.0;
        res = QString::number(inch, 10, 1) + " " + QObject::tr("inch") + " (";
        res += sizeDescription;
        res += ")";
    }

    re.setPattern("([0-9]\\d*)mm x ([0-9]\\d*)mm");
    if (re.exactMatch(sizeDescription)) {
        qCDebug(appLog) << "Matched size description with format: ([0-9]\\d*)mm x ([0-9]\\d*)mm";
        // 获取屏幕宽高 int
        m_Width = re.cap(1).toInt();
        m_Height = re.cap(2).toInt();
        retSize = QSize(m_Width, m_Height);

        double width = m_Width / 2.54;
        double height = m_Height / 2.54;
        inch = std::sqrt(width * width + height * height) / 10.0;
        res = QString::number(inch, 10, 1) + " " + QObject::tr("inch") + " (";
        res += sizeDescription;
        res += ")";
    }
#else
    QRegularExpression re("^([\\d]*)x([\\d]*) mm$");
    QRegularExpressionMatch match = re.match(sizeDescription);
    if (match.hasMatch()) {
        qCDebug(appLog) << "Matched size description with format: ^([\\d]*)x([\\d]*) mm$";
        m_Width = match.captured(1).toInt();
        m_Height = match.captured(2).toInt();
        retSize = QSize(m_Width, m_Height);

        double width = m_Width / 2.54;
        double height = m_Height / 2.54;
        inch = std::sqrt(width * width + height * height) / 10.0;
        res = QString::number(inch, 10, 1) + " " + QObject::tr("inch") + " (";
        res += sizeDescription;
        res += ")";
    }

    re.setPattern("([0-9]\\d*)mm x ([0-9]\\d*)mm");
    match = re.match(sizeDescription);
    if (match.hasMatch()) {
        qCDebug(appLog) << "Matched size description with format: ([0-9]\\d*)mm x ([0-9]\\d*)mm";
        m_Width = match.captured(1).toInt();
        m_Height = match.captured(2).toInt();
        retSize = QSize(m_Width, m_Height);

        double width = m_Width / 2.54;
        double height = m_Height / 2.54;
        inch = std::sqrt(width * width + height * height) / 10.0;
        res = QString::number(inch, 10, 1) + " " + QObject::tr("inch") + " (";
        res += sizeDescription;
        res += ")";
    }
#endif

    qCDebug(appLog) << "Finished parsing monitor size. Result:" << res;
    return res;
}

void DeviceMonitor::setInfoFromHwinfo(const QMap<QString, QString> &mapInfo)
{
    qCDebug(appLog) << "Setting monitor info from hwinfo data";
    //设置由hwinfo --monitor获取信息
    setAttribute(mapInfo, "Model", m_Name);
    setAttribute(mapInfo, "Vendor", m_Vendor);
    setAttribute(mapInfo, "Model", m_Model);
    setAttribute(mapInfo, "", m_DisplayInput);
    setAttribute(mapInfo, "Size", m_ScreenSize);
    setAttribute(mapInfo, "", m_MainScreen);
    setAttribute(mapInfo, "Resolution", m_SupportResolution);
    qCDebug(appLog) << "Basic monitor attributes set - Name:" << m_Name << "Vendor:" << m_Vendor << "Model:" << m_Model;

    double inch = 0.0;
    QSize size(0, 0);
    QString inchValue = parseMonitorSize(m_ScreenSize, inch, size);
    m_ScreenSize = inchValue;
    qCDebug(appLog) << "Screen size parsed:" << m_ScreenSize << "Width:" << size.width() << "Height:" << size.height();

    // 获取当前分辨率 和 当前支持分辨率
    QStringList listResolution = m_SupportResolution.split(" ");
    m_SupportResolution = "";
    foreach (const QString &word, listResolution) {
        if (word.contains("@")) {
            m_SupportResolution.append(word);
            m_SupportResolution.append(", ");
        }
    }

    // 计算显示比例
    caculateScreenRatio();

    #if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        m_SupportResolution.replace(QRegExp(", $"), "");
    #else
        m_SupportResolution.replace(QRegularExpression(", $"), "");
    #endif
    qCDebug(appLog) << "Supported resolutions processed:" << m_SupportResolution;

    m_ProductionWeek  = transWeekToDate(mapInfo["Year of Manufacture"], mapInfo["Week of Manufacture"]);
    setAttribute(mapInfo, "Serial ID", m_SerialNumber);
    qCDebug(appLog) << "Production week:" << m_ProductionWeek << "Serial:" << m_SerialNumber;

    // 加载其他属性
    getOtherMapInfo(mapInfo);
    qCDebug(appLog) << "Finished setting monitor info from hwinfo";
}

TomlFixMethod DeviceMonitor::setInfoFromTomlOneByOne(const QMap<QString, QString> &mapInfo)
{
    qCDebug(appLog) << "Setting monitor info from TOML configuration";
    m_IsTomlSet = true;
    TomlFixMethod ret = TOML_None;
    // 添加基本信息
    ret = setTomlAttribute(mapInfo, "Type", m_Model);
    ret = setTomlAttribute(mapInfo, "Display Input", m_DisplayInput);
    ret = setTomlAttribute(mapInfo, "Interface Type", m_Interface);
    qCDebug(appLog) << "Basic monitor attributes set from TOML - Model:" << m_Model << "Input:" << m_DisplayInput << "Interface:" << m_Interface;

    // 添加其他信息,成员变量
    ret = setTomlAttribute(mapInfo, "Support Resolution", m_SupportResolution);
    ret = setTomlAttribute(mapInfo, "Current Resolution", m_CurrentResolution);
    ret = setTomlAttribute(mapInfo, "Display Ratio", m_AspectRatio);
    qCDebug(appLog) << "Resolution info set - Supported:" << m_SupportResolution << "Current:" << m_CurrentResolution << "Ratio:" << m_AspectRatio;

    ret = setTomlAttribute(mapInfo, "Primary Monitor", m_MainScreen);
    ret = setTomlAttribute(mapInfo, "Size", m_ScreenSize);
    ret = setTomlAttribute(mapInfo, "Serial Number", m_SerialNumber);
    ret = setTomlAttribute(mapInfo, "Product Date", m_ProductionWeek);
    qCDebug(appLog) << "Additional info set - Primary:" << m_MainScreen << "Size:" << m_ScreenSize << "Serial:" << m_SerialNumber << "Date:" << m_ProductionWeek;

    //3. 获取设备的其它信息
    getOtherMapInfo(mapInfo);
    qCDebug(appLog) << "Finished setting monitor info from TOML";
    return ret;
}

void DeviceMonitor::setInfoFromSelfDefine(const QMap<QString, QString> &mapInfo)
{
    qCDebug(appLog) << "Setting monitor info from self-defined map";
    setAttribute(mapInfo, "Name", m_Name);
    setAttribute(mapInfo, "Vendor", m_Vendor);
    setAttribute(mapInfo, "CurResolution", m_CurrentResolution);
    setAttribute(mapInfo, "SupportResolution", m_SupportResolution);
    setAttribute(mapInfo, "Size", m_ScreenSize);
    setAttribute(mapInfo, "Date", m_ProductionWeek);
    // 加载其他属性
    //loadOtherDeviceInfo(mapInfo);
    qCDebug(appLog) << "Finished setting monitor info from self-defined map";
}

void DeviceMonitor::setInfoFromEdid(const QMap<QString, QString> &mapInfo)
{
    qCDebug(appLog) << "Setting monitor info from EDID data";
    m_Name = "Monitor " + mapInfo["Vendor"];
    setAttribute(mapInfo, "Size", m_ScreenSize);
    setAttribute(mapInfo, "Vendor", m_Vendor);
    setAttribute(mapInfo, "Date", m_ProductionWeek);
    setAttribute(mapInfo, "Display Input", m_DisplayInput);
    setAttribute(mapInfo, "Model", m_Model);
    qCDebug(appLog) << "Monitor attributes set from EDID - Name:" << m_Name << "Vendor:" << m_Vendor << "Size:" << m_ScreenSize << "Date:" << m_ProductionWeek;
    getOtherMapInfo(mapInfo);
    qCDebug(appLog) << "Finished setting monitor info from EDID";
}

void DeviceMonitor::setInfoFromDbus(const QMap<QString, QString> &mapInfo)
{
    qCDebug(appLog) << "Setting monitor info from D-Bus data";
    if (mapInfo["Name"].toLower().contains(m_DisplayInput.toLower(), Qt::CaseInsensitive)) {
        qCDebug(appLog) << "Monitor name contains display input, setting current resolution";
        setAttribute(mapInfo, "CurResolution", m_CurrentResolution);
    }
}

QString DeviceMonitor::transWeekToDate(const QString &year, const QString &week)
{
    qCDebug(appLog) << "Transforming week to date. Year:" << year << "Week:" << week;
    int y = year.toInt();
    int w = week.toInt();
    QDate date(y, 1, 1);
    date = date.addDays(w * 7 - 1);
    qCDebug(appLog) << "Transformed date:" << date.toString("yyyy-MM");
    return date.toString("yyyy-MM");
}

bool DeviceMonitor::setInfoFromXradr(const QString &main, const QString &edid, const QString &rate)
{
    qCDebug(appLog) << "Setting monitor info from xrandr";
    if(m_IsTomlSet) {
        qCDebug(appLog) << "Monitor info already set from TOML, skipping xrandr";
        return false;
    }
    // 判断该显示器设备是否已经设置过从xrandr获取的消息
    if (!m_Interface.isEmpty()) {
        qCDebug(appLog) << "Interface is not empty, checking current resolution";
        // 设置当前分辨率
        if (m_CurrentResolution.isEmpty()) {
            qCDebug(appLog) << "Current resolution is empty, extracting from main string";
            QRegularExpression reScreenSize(".*([0-9]{1,5}x[0-9]{1,5}).*");
            QRegularExpressionMatch match = reScreenSize.match(main);
            if (match.hasMatch()) {
                qCDebug(appLog) << "Matched screen size from main string:" << match.captured(1);
                if (!rate.isEmpty()) {
                    qCDebug(appLog) << "Rate is not empty, extracting current resolution";
                    QString curRate = rate;
                    QRegularExpression rateStart("[a-zA-Z]");
                    QRegularExpressionMatch rateMatch = rateStart.match(curRate);
                    int pos = rateMatch.capturedStart();
                    if (pos > 0 && curRate.size() > pos && !Common::boardVendorType().isEmpty()) {
                        curRate = QString::number(ceil(curRate.left(pos).toDouble())) + curRate.right(curRate.size() - pos);
                    }
                    m_CurrentResolution = QString("%1@%2").arg(match.captured(1)).arg(curRate);
                } else {
                    qCDebug(appLog) << "Rate is empty";
                    m_CurrentResolution = QString("%1").arg(match.captured(1));
                }
            }
        }
        qCDebug(appLog) << "Interface already processed, returning false";
        return false;
    }

    if (main.contains("disconnected")) {
        qCDebug(appLog) << "Main string contains disconnected, returning false";
        return false;
    }

    // wayland xrandr --verbose无法获取edid信息
    if (qApp->isDXcbPlatform()) {
        // 根据edid计算屏幕大小
        if (edid.isEmpty()) {
            qCDebug(appLog) << "EDID is empty, returning false";
            return false;
        }
        if (!caculateScreenSize(edid)) {
            qCDebug(appLog) << "Failed to calculate screen size from EDID, returning false";
            return false;
        }
    }

    // 获取屏幕的主要信息，包括借口(HDMI VGA)/是否主显示器和屏幕大小，
    // 但是这里计算的屏幕大小仅仅用来匹配是否是同一个显示器,真正的屏幕大小计算是根据edid计算的
    if (!setMainInfoFromXrandr(main, rate)) {
        qCDebug(appLog) << "Failed to set main info from xrandr, returning false";
        return false;
    }

    return true;
}

const QString &DeviceMonitor::name()const
{
    // qCDebug(appLog) << "Getting monitor name:" << m_Name;
    return m_Name;
}

const QString &DeviceMonitor::vendor() const
{
    // qCDebug(appLog) << "Getting monitor vendor:" << m_Vendor;
    return m_Vendor;
}

const QString &DeviceMonitor::driver() const
{
    // qCDebug(appLog) << "Getting monitor driver:" << m_Driver;
    return m_Driver;
}

bool DeviceMonitor::available()
{
    // qCDebug(appLog) << "Checking monitor availability, returning true";
    return true;
}

QString DeviceMonitor::subTitle()
{
    // qCDebug(appLog) << "Getting monitor subtitle";
    return m_Name;
}

const QString DeviceMonitor::getOverviewInfo()
{
    qCDebug(appLog) << "Getting monitor overview information";
    QString ov;

    ov = QString("%1(%2)").arg(m_Name).arg(m_ScreenSize);
    qCDebug(appLog) << "Monitor overview:" << ov;

    return ov;
}

void DeviceMonitor::initFilterKey()
{
    qCDebug(appLog) << "Initializing filter keys for monitor";
    addFilterKey(QObject::tr("Date"));
}

void DeviceMonitor::loadBaseDeviceInfo()
{
    qCDebug(appLog) << "Loading base device info for monitor";
    // 添加基本信息
    addBaseDeviceInfo(tr("Name"), m_Name);
    addBaseDeviceInfo(tr("Vendor"), m_Vendor);
    addBaseDeviceInfo(tr("Type"), m_Model);
    addBaseDeviceInfo(tr("Display Input"), m_DisplayInput);
    addBaseDeviceInfo(tr("Interface Type"), m_Interface);
}

void DeviceMonitor::loadOtherDeviceInfo()
{
    qCDebug(appLog) << "Loading other device info for monitor";
    // 添加其他信息,成员变量
    addOtherDeviceInfo(tr("Support Resolution"), m_SupportResolution);
    if (m_CurrentResolution != "@Hz") {
        addOtherDeviceInfo(tr("Current Resolution"), m_CurrentResolution);
        addOtherDeviceInfo(tr("Display Ratio"), m_AspectRatio);
    }
    addOtherDeviceInfo(tr("Primary Monitor"), m_MainScreen);
    addOtherDeviceInfo(tr("Size"), m_ScreenSize);
    addOtherDeviceInfo(tr("Serial Number"), m_SerialNumber);
//    addOtherDeviceInfo(tr("Product Date"), m_ProductionWeek);

    mapInfoToList();
    qCDebug(appLog) << "Other device info loaded";
}

void DeviceMonitor::loadTableData()
{
    qCDebug(appLog) << "Loading table data for monitor";
    m_TableData.append(m_Name);
    m_TableData.append(m_Vendor);
    m_TableData.append(m_Model);
}

bool DeviceMonitor::setMainInfoFromXrandr(const QString &info, const QString &rate)
{
    qCDebug(appLog) << "Setting main info from xrandr";
    //  bug89456：显示设备接口类型DP，VGA，HDMI，eDP，DisplayPort
    //  还可能会有其它接口类型，为了避免每一次遇到新的接口类型就要修改代码
    //  使用正则表达式获取接口类型进行显示
    // 使用 QRegularExpression 替换 QRegExp
    QRegularExpression reStart("^([a-zA-Z]*)-[\\s\\S]*");
    QRegularExpressionMatch match = reStart.match(info);
    if (match.hasMatch()) {
        qCDebug(appLog) << "Matched interface from xrandr info:" << match.captured(1);
        m_Interface = match.captured(1);
    }

    // wayland xrandr --verbose无primary信息
    if (qApp->isDXcbPlatform()) {
        qCDebug(appLog) << "Running on XCB platform, checking for primary display";
        // 设置是否是主显示器
        if (info.contains("primary")) {
            qCDebug(appLog) << "Primary display";
            m_MainScreen = "Yes";
        } else {
            qCDebug(appLog) << "Not primary display";
            m_MainScreen = "NO";
        }
    }

    // 设置当前分辨率
    QRegularExpression reScreenSize(".*([0-9]{1,5}x[0-9]{1,5}).*");
    match = reScreenSize.match(info);
    if (match.hasMatch()) {
        qCDebug(appLog) << "Found screen size in xrandr info:" << match.captured(1);
        if (!rate.isEmpty()) {
            qCDebug(appLog) << "Rate is not empty, calculating current resolution with rate:" << rate;
            QString curRate = rate;
            QRegularExpression rateStart("[a-zA-Z]");
            QRegularExpressionMatch rateMatch = rateStart.match(curRate);
            int pos = rateMatch.capturedStart();
            if (pos > 0 && curRate.size() > pos && !Common::boardVendorType().isEmpty()) {
                qCDebug(appLog) << "Adjusting rate for board vendor type";
                curRate = QString::number(ceil(curRate.left(pos).toDouble())) + curRate.right(curRate.size() - pos);
            }
            m_CurrentResolution = QString("%1@%2").arg(match.captured(1)).arg(curRate);
        } else {
            qCDebug(appLog) << "Rate is empty, setting current resolution without rate";
            m_CurrentResolution = QString("%1").arg(match.captured(1));
        }
    }

    qCDebug(appLog) << "Finished setting main info from xrandr. Interface:" << m_Interface << "Primary:" << m_MainScreen << "Resolution:" << m_CurrentResolution;
    return true;
}

void DeviceMonitor::caculateScreenRatio()
{
    qCDebug(appLog) << "Calculating screen ratio for width:" << m_Width << "height:" << m_Height;
    QRegularExpression re("^([\\d]*)x([\\d]*)(.*)$");
    QRegularExpressionMatch match = re.match(m_CurrentResolution);
    if (match.hasMatch()) {
        int width = match.captured(1).toInt();
        int height = match.captured(2).toInt();
        int gys = gcd(width, height);
        int w = width / gys;
        int h = height / gys;

        if (w > 21)
            findAspectRatio(w, h, w, h);

        m_AspectRatio = QString::number(w) + " : " + QString::number(h);
    }
}

int DeviceMonitor::gcd(int a, int b)
{
    qCDebug(appLog) << "Calculating GCD for" << a << "and" << b;
    if (a < b)
        std::swap(a, b);

    if (a % b == 0) {
        qCDebug(appLog) << "GCD is" << b;
        return b;
    } else {
        return gcd(b, a % b);
    }
}

bool DeviceMonitor::findAspectRatio(int width, int height, int &ar_w, int &ar_h)
{
    qCDebug(appLog) << "Finding aspect ratio for width:" << width << "height:" << height;
    float r1 = float(width) / float(height);

    for (ar_w = 21; ar_w > 0; --ar_w) {
        for (ar_h = 21; ar_h > 0; --ar_h) {
            if (std::abs(r1 - float(ar_w) / float(ar_h)) / r1 < 0.01) {
                int r = gcd(ar_w, ar_h);
                ar_w /= r;
                ar_h /= r;
                qCDebug(appLog) << "Found aspect ratio match:" << ar_w << ":" << ar_h;
                return true;
            }
        }
    }

    qCDebug(appLog) << "Aspect ratio not found";
    return false;
}

void DeviceMonitor::caculateScreenSize()
{
    qCDebug(appLog) << "Calculating screen size from string:" << m_ScreenSize;
    // 527x296 mm
    QRegularExpression re(".*([0-9]{3,5})x([0-9]{3,5})\\smm$");
    QRegularExpressionMatch match = re.match(m_ScreenSize);
    if (match.hasMatch()) {
        qCDebug(appLog) << "Matched screen size pattern";
        m_Width = match.captured(1).toInt();
        m_Height = match.captured(2).toInt();

        double inch = std::sqrt((m_Width / 2.54) * (m_Width / 2.54) + (m_Height / 2.54) * (m_Height / 2.54)) / 10.0;
        m_ScreenSize = QString("%1 %2(%3mm X %4mm)").arg(QString::number(inch, '0', 1)).arg(QObject::tr("inch")).arg(m_Width).arg(m_Height);
        qCDebug(appLog) << "Calculated screen size:" << m_ScreenSize;
    }
}

bool DeviceMonitor::caculateScreenSize(const QString &edid)
{
    qCDebug(appLog) << "Calculating screen size from EDID";
    // edid parse
    EDIDParser edidParse;
    QString errormsg;
    if (!edidParse.setEdid(edid, errormsg)) {
        qCDebug(appLog) << "Failed to set EDID, returning false";
        return false;
    }
    // 使用edid的厂商信息
    if (!edidParse.vendor().isEmpty()) {
        qCDebug(appLog) << "Using EDID vendor:" << edidParse.vendor();
    }
        m_Vendor = edidParse.vendor();
    double height = edidParse.height();
    double width = edidParse.width();
    if (height <= 0 || width <= 0)
        return false;

    // 比对从hwinfo和xrandr里面获取日期，不一致返回
    if (!m_ProductionWeek.isEmpty() && m_ProductionWeek != edidParse.releaseDate())
        return false;
    m_ProductionWeek = edidParse.releaseDate();

    // 如果从hwinfo和edid里面获取的信息差距很小则使用hwinfo里面的
    // 如果从hwinfo和edid里面获取的信息差距很大则使用edid里面的
    if (fabs(width * 10 - m_Width) < 10 && fabs(height * 10 - m_Height) < 10)
        return true;

    double inch = std::sqrt(height * height + width * width) / 2.54 / 10;
    m_ScreenSize = QString("%1 %2(%3mm X %4mm)").arg(QString::number(inch, '0', 1)).arg(QObject::tr("inch")).arg(width).arg(height);
    return true;
}
