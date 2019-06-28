/***************************************************************************
 *   Copyright (C) 2019 by Bubli                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "certificatetools.h"
#include <iostream>
#include <klocalizedstring.h>

#include <QFileDialog>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMimeDatabase>

CertificateTools::CertificateTools( QWidget * parent )
    : WidgetConfigurationToolsBase( parent )
{
}

QStringList CertificateTools::tools() const
{
    QStringList res;
    return res;
}

void CertificateTools::setTools(const QStringList& /*items*/)
{
    return;
}

void CertificateTools::slotAdd()
{
    QMimeDatabase mimeDatabase;
    QString filter = i18n("PKCS12 Digital IDs (%1)", mimeDatabase.mimeTypeForName(QStringLiteral("application/x-pkcs12")).globPatterns().join(QLatin1Char(' ')));

    QString file = QFileDialog::getOpenFileName( this, QString(), QString(), filter );

    if (file.isEmpty())
        return;

    // Create list entry
    QListWidgetItem * listEntry = new QListWidgetItem( file, m_list );

    // Select and scroll
    m_list->setCurrentItem( listEntry );
    m_list->scrollToItem( listEntry );
    updateButtons();
    emit changed();
}

void CertificateTools::slotEdit()
{
    std::cout << "edit" << std::endl;
}