/***************************************************************************
 *   Copyright (C) 2014 by David Pinelo                                    *
 *   alepherp@alephsistemas.es                                             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include "aerphelpwidget.h"
#include <QtHelp>
#include "widgets/aerphelpbrowser.h"
#include "qlogger.h"
#include <aerpcommon.h>
#include "configuracion.h"

class AERPHelpWidgetPrivate
{
public:
    AERPHelpWidget *q_ptr;
    QPointer<QHelpEngine> m_helpEngine;
    QPointer<AERPHelpBrowser> m_browser;
    QUrl m_actualUrl;
    QStringList m_helpFiles;
    QString m_collectionFile;

    AERPHelpWidgetPrivate(AERPHelpWidget *qq) : q_ptr(qq)
    {

    }

    void registerHelpFiles();
};

void AERPHelpWidgetPrivate::registerHelpFiles()
{
    if ( m_collectionFile.isEmpty() )
    {
        QLogger::QLog_Error(AlephERP::stLogOther, QString("AERPHelpWidgetPrivate::registerHelpFiles: No se ha registrado ningún colletion file"));
        return;
    }

    QDir dir(alephERPSettings->dataPath());
    QStringList nameFilters;
    nameFilters << "*.qch";
    QStringList helpFiles = dir.entryList(nameFilters, QDir::Files, QDir::Name);
    if ( !helpFiles.isEmpty() )
    {
        foreach (const QString helpFile, helpFiles)
        {
            QString fileHelp = QString("%1/%2").arg(alephERPSettings->dataPath(), helpFile);
            if ( QFile::exists(fileHelp) )
            {
                if ( !m_helpFiles.contains(fileHelp) )
                {
                    m_helpFiles.append(fileHelp);
                    if ( !m_helpEngine.isNull() )
                    {
                        if ( !m_helpEngine->registerDocumentation(fileHelp) )
                        {
                            QLogger::QLog_Error(AlephERP::stLogOther, QString("AERPHelpWidgetPrivate::registerHelpFiles: Error registrando el archivo de documentación: [%1]. ERROR: [%2]").
                                                arg(fileHelp, m_helpEngine->error()));
                        }
                    }
                }
            }
        }
    }
}

AERPHelpWidget::AERPHelpWidget(const QString &collectionFile, QWidget *parent) :
    QWidget(parent),
    d(new AERPHelpWidgetPrivate(this))
{
    setCollectionFile(collectionFile);
    setupUi();
}

AERPHelpWidget::~AERPHelpWidget()
{
    delete d;
}

QString AERPHelpWidget::collectionFile() const
{
    return d->m_collectionFile;
}

void AERPHelpWidget::setCollectionFile(const QString &file)
{
    d->m_collectionFile = file;
    if ( !d->m_collectionFile.isEmpty() )
    {
        // Veamos si la ayuda está disponible
        QFileInfo helpFile(d->m_collectionFile);
        if ( helpFile.exists() )
        {
            d->m_helpEngine = new QHelpEngine(helpFile.absoluteFilePath(), this);
            if ( !d->m_helpEngine->setupData() )
            {
                delete d->m_helpEngine;
            }
            else
            {
                // Se registran los archivos de ayuda existentes en los metadatos.
                d->registerHelpFiles();
            }
        }
    }
}

QUrl AERPHelpWidget::url() const
{
    return d->m_actualUrl;
}

void AERPHelpWidget::showUrl(const QUrl &url)
{
    d->m_actualUrl = url;
    d->m_browser->setSource(d->m_actualUrl);
}

void AERPHelpWidget::showUrl(const QString &url)
{
    d->m_actualUrl = QUrl(url);
    d->m_browser->setSource(d->m_actualUrl);
}

void AERPHelpWidget::setupUi()
{
    QSplitter *helpWidget = new QSplitter(this);

    QVBoxLayout *vLayout = new QVBoxLayout(helpWidget);
    QComboBox *cb = new QComboBox(helpWidget);
    cb->addItem(tr("Contenido"));
    cb->addItem(tr("Índice"));

    QSizePolicy hSize (QSizePolicy::Preferred, QSizePolicy::Preferred);
    hSize.setHorizontalStretch(1);
    cb->setSizePolicy(hSize);

    QStackedWidget *sw = new QStackedWidget(helpWidget);
    sw->addWidget(d->m_helpEngine->contentWidget());
    sw->addWidget(d->m_helpEngine->indexWidget());
    sw->setSizePolicy(hSize);

    connect(cb, SIGNAL(currentIndexChanged(int)), sw, SLOT(setCurrentIndex(int)));

    vLayout->setContentsMargins(0, 0, 0, 0);
    vLayout->addWidget(cb);
    vLayout->addWidget(sw);
    QWidget *idxWidget = new QWidget(helpWidget);
    idxWidget->setLayout(vLayout);

    d->m_browser = new AERPHelpBrowser(helpWidget);
    hSize.setHorizontalStretch(3);
    d->m_browser->setSizePolicy(hSize);
    if ( !d->m_helpEngine.isNull() )
    {
        d->m_browser->setHelpEngine(d->m_helpEngine);
    }

    helpWidget->addWidget(idxWidget);
    helpWidget->addWidget(d->m_browser);

    connect(d->m_helpEngine->indexWidget(), SIGNAL(linkActivated(QUrl,QString)), d->m_browser, SLOT(setSource(QUrl, QString)));
    connect(d->m_helpEngine->contentWidget(), SIGNAL(linkActivated(QUrl)), d->m_browser, SLOT(setSource(QUrl)));

    QHBoxLayout *mainLayout = new QHBoxLayout;
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(helpWidget);
    setLayout(mainLayout);
}

QByteArray AERPHelpWidget::helpItem(const QUrl &url)
{
    if ( d->m_helpEngine.isNull() )
    {
        return QByteArray();
    }
    return d->m_helpEngine->fileData(url);
}

bool AERPHelpWidget::existsUrl(const QString &strUrl)
{
    QUrl url(strUrl);
    if ( d->m_helpEngine.isNull() )
    {
        return false;
    }
    QUrl destUrl = d->m_helpEngine->findFile(url);
    return destUrl.isValid();
}


