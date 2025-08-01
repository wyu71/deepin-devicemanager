// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// 项目自身文件
#include "EDIDParser.h"
#include "DDLog.h"

// Qt库文件
#include<QLoggingCategory>
#include<QDate>

// 其它头文件
#include<qmath.h>

using namespace DDLog;

EDIDParser::EDIDParser()
    : m_Vendor()
    , m_ReleaseDate()
    , m_ScreenSize()
    , m_LittleEndianMode(true)
    , m_Width(0)
    , m_Height(0)
{
    qCDebug(appLog) << "EDIDParser constructor called";
    m_MapCh.insert("00001", "A");
    m_MapCh.insert("00010", "B");
    m_MapCh.insert("00011", "C");
    m_MapCh.insert("00100", "D");
    m_MapCh.insert("00101", "E");
    m_MapCh.insert("00110", "F");
    m_MapCh.insert("00111", "G");
    m_MapCh.insert("01000", "H");
    m_MapCh.insert("01001", "I");
    m_MapCh.insert("01010", "J");
    m_MapCh.insert("01011", "K");
    m_MapCh.insert("01100", "L");
    m_MapCh.insert("01101", "M");
    m_MapCh.insert("01110", "N");
    m_MapCh.insert("01111", "O");
    m_MapCh.insert("10000", "P");
    m_MapCh.insert("10001", "Q");
    m_MapCh.insert("10010", "R");
    m_MapCh.insert("10011", "S");
    m_MapCh.insert("10100", "T");
    m_MapCh.insert("10101", "U");
    m_MapCh.insert("10110", "V");
    m_MapCh.insert("10111", "W");
    m_MapCh.insert("11000", "X");
    m_MapCh.insert("11001", "Y");
    m_MapCh.insert("11010", "Z");
}

bool EDIDParser::setEdid(const QString &edid, QString &errorMsg, const QString &ch, bool littleEndianMode)
{
    qCDebug(appLog) << "Setting EDID data. Little endian mode:" << littleEndianMode;

    m_LittleEndianMode = littleEndianMode;
    // 判断是否是合理的edid
    if (m_LittleEndianMode) {
        if (!edid.startsWith("00ffffffffffff00")) {
            errorMsg = "Error edid info";
            qCWarning(appLog) << "Invalid EDID header in little endian mode";
            return false;
        }
    } else {
        if (!edid.startsWith("ff00ffffffff00ff")) {
            errorMsg = "Error edid info";
            qCWarning(appLog) << "Invalid EDID header in big endian mode";
            return false;
        }
    }

    QStringList lines = edid.split(ch);
    foreach (const QString &line, lines) {
        if (line == "")
            continue;

        m_ListEdid.append(line);
    }

    // 解析厂商信息
    qCDebug(appLog) << "Parsing vendor info";
    parserVendor();
    // 解析发布日期
    qCDebug(appLog) << "Parsing release date";
    parseReleaseDate();
    // 解析屏幕尺寸
    qCDebug(appLog) << "Parsing screen size";
    parseScreenSize();


    return true;
}

const QString &EDIDParser::vendor()const
{
    // qCDebug(appLog) << "Getting vendor";
    return m_Vendor;
}

const QString &EDIDParser::model()const
{
    // qCDebug(appLog) << "Getting model";
    return m_Model;
}

const QString &EDIDParser::releaseDate()const
{
    // qCDebug(appLog) << "Getting release date";
    return m_ReleaseDate;
}

const QString &EDIDParser::screenSize()const
{
    // qCDebug(appLog) << "Getting screen size";
    return m_ScreenSize;
}

int EDIDParser::width()
{
    // qCDebug(appLog) << "Getting width";
    return m_Width;
}

int EDIDParser::height()
{
    // qCDebug(appLog) << "Getting height";
    return m_Height;
}

void EDIDParser::parserVendor()
{
    qCDebug(appLog) << "Parsing vendor info from EDID";

    // 获取制造商信息，edid中的 08h 和 09h 是厂商信息
    // 08表示 第0行  第9个字节
    QString vendorStr;
    QString h08 = getBytes(0, 8);
    QString h09 = getBytes(0, 9);
    char h0809[2];
    if (m_LittleEndianMode) {
        qCDebug(appLog) << "Little endian mode";
        vendorStr = h08 + h09;
        h0809[0] = static_cast<char>(hexToDec(h08).toInt());
        h0809[1] = static_cast<char>(hexToDec(h09).toInt());
    } else {
        qCDebug(appLog) << "Big endian mode";
        vendorStr = h09 + h08;
        h0809[0] = static_cast<char>(hexToDec(h09).toInt());
        h0809[1] = static_cast<char>(hexToDec(h08).toInt());
    }

    char name[4];
    name[0] = ((h0809[0] & 0x7C) >> 2) + '@';
    name[1] = ((h0809[0] & 0x03) << 3) + ((h0809[1] & 0xE0) >> 5) + '@';
    name[2] = (h0809[1] & 0x1F) + '@';
    name[3] = 0;

    m_Vendor = QString(name);
    qCDebug(appLog) << "Vendor parsed:" << m_Vendor;

    QString h0a = getBytes(0, 10);
    QString h0b = getBytes(0, 11);

    m_Model = h0a + h0b;
    qCDebug(appLog) << "Model parsed:" << m_Model;

//    // 将16进制的厂商信息转换成二进制的厂商信息
//    QString binStr = hexToBin(vendorStr);

//    // 转换后的二进制厂商信息格式为(6,5,5),如：010110 10011 00011
//    // 分别表示三个字符，就是厂商的缩写
//    QString bin1 = binStr.mid(1, 5);
//    QString bin2 = binStr.mid(6, 5);
//    QString bin3 = binStr.mid(11, 5);

//    // 获取厂商信息
//    m_Vendor = m_MapCh[bin1] +  m_MapCh[bin2] + m_MapCh[bin3];
}

void EDIDParser::parseReleaseDate()
{
    qCDebug(appLog) << "Parsing release date from EDID";

    // edid中的  10H和11H就是发布日期信息
    QString hWeek = getBytes(1, m_LittleEndianMode ? 0 : 1);
    QString hYear = getBytes(1, m_LittleEndianMode ? 1 : 0);

    int week = hexToDec(hWeek).toInt();
    int year = hexToDec(hYear).toInt() + 1990;

    QDate date(year, 1, 1);
    date = date.addDays(week * 7 - 1);
    m_ReleaseDate = date.toString("yyyy-MM");
}

void EDIDParser::parseScreenSize()
{
    qCDebug(appLog) << "Parsing screen size from EDID";

    //Detailed Timing
    if(m_LittleEndianMode){
        QString tmpw = getBytes(4,2);
        QString tmph = getBytes(4,3);
        QString tmpshl = getBytes(4,4);
        m_Width = hexToDec(tmpshl.mid(0,1) + tmpw).toInt();
        m_Height = hexToDec(tmpshl.mid(1,1) + tmph).toInt();
        qCDebug(appLog) << "Parsed width and height from detailed timing:" << m_Width << "x" << m_Height;
    }

    // edid中的  15H和16H就是屏幕大小 , 与Detailed Timing相差超10mm 则用15H和16H的。
    int width15 = hexToDec(getBytes(1, m_LittleEndianMode ? 5 : 4)).toInt()*10;
    int height16 = hexToDec(getBytes(1, m_LittleEndianMode ? 6 : 7)).toInt()*10;
    qCDebug(appLog) << "Parsed width and height from bytes 15H/16H:" << width15 << "x" << height16;
    if(m_Width+10 < width15  || m_Height+10 < height16) {
        qCDebug(appLog) << "Detailed timing differs significantly, using 15H/16H values.";
        m_Width = width15;
        m_Height = height16;
    }

    double inch = sqrt((m_Width / 2.54) * (m_Width / 2.54) + (m_Height / 2.54) * (m_Height / 2.54))/10;
    m_ScreenSize = QString("%1 %2(%3mm X %4mm)").arg(QString::number(inch, '0', 1)).arg(QObject::tr("inch")).arg(m_Width).arg(m_Height);
    qCDebug(appLog) << "Screen size parsed:" << m_ScreenSize << "Width:" << m_Width << "Height:" << m_Height;
}

QString EDIDParser::binToDec(QString strBin)   //二进制转十进制
{
    // qCDebug(appLog) << "Converting binary to decimal";
    // 二进制转十进制
    QString decimal;
    int nDec = 0, nLen;
    int i, j, k;
    nLen = strBin.length();

    for (i = 0; i < nLen; i++) {
        if (strBin[nLen - i - 1] == "0")
            continue;
        else {
            k = 1;
            for (j = 0; j < i; j++)
                k = k * 2;
            nDec += k;
        }
    }

    decimal = QString::number(nDec);
    // qCDebug(appLog) << "Converted binary" << strBin << "to decimal" << decimal;
    return decimal;
}

QString EDIDParser::decTobin(QString strDec)   //十进制转二进制
{
    // qCDebug(appLog) << "Converting decimal to binary";
    int nDec = strDec.toInt();
    int nYushu = 0;
    int nShang = 0;
    QString strBin, strTemp;

    bool bContinue = true;
    while (bContinue) {
        nYushu = nDec % 2;
        nShang = nDec / 2;
        strBin = QString::number(nYushu) + strBin; //qCInfo(appLog)<<strBin;
        strTemp = strBin;
        //strBin.Format("%s%s", buf, strTemp);
        nDec = nShang;
        if (nShang == 0) {
            // qCDebug(appLog) << "Decimal to binary conversion finished.";
            bContinue = false;
        }
    }
    int nTemp = strBin.length() % 4;
    switch (nTemp) {
    case 1:
        //strTemp.Format(_T("000%s"), strBin);
        strTemp = "000" + strBin;
        strBin = strTemp;
        break;
    case 2:
        //strTemp.Format(_T("00%s"), strBin);
        strTemp = "00" + strBin;
        strBin = strTemp;
        break;
    case 3:
        //strTemp.Format(_T("0%s"), strBin);
        strTemp = "0" + strBin;
        strBin = strTemp;
        break;
    default:
        break;
    }
    return strBin;
}

QString EDIDParser::decToHex(QString strDec)   //十进制转十六进制
{
    // qCDebug(appLog) << "Converting decimal to hexadecimal";
    int hex = strDec.toInt();
    QString hex1 = QString("%1").arg(hex, 8, 16, QLatin1Char('0'));
    return hex1;
}

int EDIDParser::hex2(unsigned char ch)           //十六进制转换工具
{
    // qCDebug(appLog) << "Converting hexadecimal to decimal";
    if ((ch >= '0') && (ch <= '9'))
        return ch - 0x30;
    else if ((ch >= 'A') && (ch <= 'F'))
        return ch - 'A' + 10;
    else if ((ch >= 'a') && (ch <= 'f'))
        return ch - 'a' + 10;
    return 0;
}

QString EDIDParser::hexToDec(QString strHex)   //十六进制转十进制
{
    // qCDebug(appLog) << "Converting hexadecimal to decimal";
    int i;
    int v = 0;
    for (i = 0; i < strHex.length(); i++) {
        v *= 16;
        v += hex2(strHex[i].toLatin1());
    }
    return QString::number(v);
}

QString EDIDParser::hexToBin(QString strHex)
{
    // qCDebug(appLog) << "Converting hexadecimal to binary";
    // 十六进制转二进制
    QString dec = hexToDec(strHex);
    return decTobin(dec);
}

QString EDIDParser::getBytes(int l, int n)
{
    // qCDebug(appLog) << "Getting bytes";
    // 获取指定字节
    int index = n * 2;
    if (m_ListEdid.size() < l + 1) {
        qCWarning(appLog) << "Line index" << l << "is out of bounds.";
        return "";
    }

    QString line = m_ListEdid[l];
    if (line.length() < n + 1) {
        qCWarning(appLog) << "Byte index" << n << "is out of bounds for line" << l;
        return "";
    }

    return line.mid(index, 2);
}
