// Copyright (C) 2019 ~ 2020 UnionTech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "UrlChooserEdit.h"
#include "DDLog.h"

#include <DGuiApplicationHelper>
#include <DStyle>
#include <DLineEdit>
#include <DSuggestButton>
#include <DFontSizeManager>

#include <QHBoxLayout>
#include <QFileDialog>
#include <QDir>
#include <QStorageInfo>
#include <QFontMetrics>
#include <QLoggingCategory>

using namespace DDLog;

UrlChooserEdit::UrlChooserEdit(QWidget *parent) : DWidget(parent)
  ,mp_urlEdit(new DLineEdit)
  ,mp_urlBtn(new DSuggestButton)
{
    qCDebug(appLog) << "UrlChooserEdit instance created";
   initUI();
   initConnections();
}

void UrlChooserEdit::initUI()
{
    qCDebug(appLog) << "Initializing UI";
   setFixedSize(460,36);
   mp_urlEdit->setFixedSize(410,36);
   mp_urlEdit->setText(QDir::homePath());
   mp_folderPath = mp_urlEdit->text();
   mp_urlEdit->setClearButtonEnabled(false);
   mp_urlEdit->lineEdit()->setReadOnly(true);
   mp_urlEdit->setFocusPolicy(Qt::NoFocus);  
   mp_urlBtn->setFixedSize(40,36);
   mp_urlBtn->setIcon(DStyleHelper(mp_urlEdit->style()).standardIcon(DStyle::SP_SelectElement, nullptr));
   mp_urlBtn->setIconSize(QSize(24,24));
   QHBoxLayout *mainLayout = new QHBoxLayout;
   setContentsMargins(0,0,0,0);
   mainLayout->setContentsMargins(0,0,0,0);
   mainLayout->addWidget(mp_urlEdit);
   mainLayout->addSpacing(10);
   mainLayout->addWidget(mp_urlBtn);
   setLayout(mainLayout);
   mp_folderPath = mp_urlEdit->text();
   mp_elidParh = mp_folderPath;
}

void UrlChooserEdit::initConnections()
{
    qCDebug(appLog) << "Initializing connections";
    connect(mp_urlBtn,&DSuggestButton::clicked, this, &UrlChooserEdit::slotChooseUrl);
}

QString UrlChooserEdit::text() const
{
    // qCDebug(appLog) << "Getting text";
    return  mp_folderPath;
}

void UrlChooserEdit::slotChooseUrl()
{
    qCDebug(appLog) << "Choosing URL path";

    QString path = QFileDialog::getExistingDirectory(this,"","",QFileDialog::ReadOnly);
    QFontMetrics fEdlit(mp_urlEdit->font());
    QString floderPath = fEdlit.elidedText(path, Qt::ElideMiddle, mp_urlEdit->width() - 80);
    if (path.isEmpty()){
        qCDebug(appLog) << "No path selected, restoring previous path";
        floderPath = mp_elidParh;         //带...的路径
        path = mp_folderPath;
    }
    mp_urlEdit->setText(floderPath);
    mp_elidParh = floderPath;
    mp_folderPath = path;
    checkLocalFolder(mp_folderPath);
}

void UrlChooserEdit::checkLocalFolder(const QString &path)
{
    qCDebug(appLog) << "Checking if folder is local:" << path;

    Q_UNUSED(path);
    bool isLocal = true;
    mp_urlEdit->setAlert(false);
    QStorageInfo info(mp_folderPath);                               //获取路径信息
    qCInfo(appLog) << __func__ <<info.device();
    if (!info.isValid() || !info.device().startsWith("/dev/")) {     //判断路径信息是不是本地路径
        qCWarning(appLog) << "Selected folder is not local";
        mp_urlEdit->setAlert(true);
        mp_urlEdit->showAlertMessage(tr("Select a local folder please"),this,1000);
        isLocal = false;
    }
    emit signalNotLocalFolder(isLocal);
}
