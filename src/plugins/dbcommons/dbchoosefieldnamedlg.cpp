/***************************************************************************
 *   Copyright (C) 2011 by David Pinelo   *
 *   alepherp@alephsistemas.es   *
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
#include <QtCore>
#include <QtGui>

#include <alepherpdaobusiness.h>
#include <alepherpdbcommons.h>
#include "ui_dbchoosefieldnamedlg.h"

class DBChooseFieldNameDlgPrivate
{
public:
    QWidget *m_widget;
    DBChooseFieldNameDlg *q;
    QString m_field;
    bool m_openFailed;

    explicit DBChooseFieldNameDlgPrivate(DBChooseFieldNameDlg *q_ptr) : q(q_ptr)
    {
        m_openFailed = false;
        m_widget = NULL;
    }
};

DBChooseFieldNameDlg::DBChooseFieldNameDlg(QWidget *item, QWidget *parent) :
    QDialog(parent), d(new DBChooseFieldNameDlgPrivate(this)), ui(new Ui::DBChooseFieldNameDlg())
{
    ui->setupUi(this);
    d->m_widget = item;

    if ( d->m_widget == NULL )
    {
        d->m_openFailed = true;
        return;
    }

    CommonsFunctions::setOverrideCursor(QCursor(Qt::WaitCursor));
    if ( !BeansFactory::metadataSystemInited() )
    {
        QString trash;
        if ( !BeansFactory::initSystemsBeans(trash) )
        {
            /** TODO: Por alguna extraña razon... no carga el driver a la primera. Lo vuelvo a intentar... ¿Un bug mío? ¿De Qt Designer? */
            if ( !BeansFactory::metadataSystemInited() )
            {
                if ( !BeansFactory::initSystemsBeans(trash) )
                {
                    QMessageBox::warning(this, tr("AlephERP"), tr("No se ha podido inicializar la estructura de beans. No puede iniciarse este diálogo."), QMessageBox::Ok);
                    CommonsFunctions::restoreOverrideCursor();
                    d->m_openFailed = true;
                    return;
                }
            }
        }
    }
    CommonsFunctions::restoreOverrideCursor();

    // Cargamos los valores de las tablas en los combos
    BaseBeanMetadata *dialogMetadata = BeansFactory::instance()->metadataBean(DBBaseWidget::aerpBaseDialogTableName(d->m_widget));
    if ( dialogMetadata != NULL )
    {
        foreach ( DBFieldMetadata *field, dialogMetadata->fields() )
        {
            ui->lwFields->addItem(field->dbFieldName());
        }
    }
    if ( d->m_widget->property(AlephERP::stAerpControl).toBool() )
    {
        DBBaseWidget *baseWidget = dynamic_cast<DBBaseWidget *>(d->m_widget);
        QList<QListWidgetItem *> list = ui->lwFields->findItems(baseWidget->fieldName(), Qt::MatchExactly);
        if ( list.size() > 0 )
        {
            ui->lwFields->setItemSelected(list.at(0), true);
        }
    }
    connect (ui->lwFields, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(fieldSelected(QListWidtgetItem *)));
    connect (ui->pbOk, SIGNAL(clicked()), this, SLOT(ok()));
    connect (ui->pbCancel, SIGNAL(clicked()), this, SLOT(close()));
}

DBChooseFieldNameDlg::~DBChooseFieldNameDlg()
{
    delete ui;
}

QString DBChooseFieldNameDlg::field()
{
    return d->m_field;
}

bool DBChooseFieldNameDlg::openFailed()
{
    return d->m_openFailed;
}

void DBChooseFieldNameDlg::fieldSelected(QListWidgetItem *item)
{
    d->m_field = item->text();
    close();
}

void DBChooseFieldNameDlg::ok()
{
    QListWidgetItem *selected = ui->lwFields->currentItem();
    if ( selected != NULL )
    {
        d->m_field = selected->text();
    }
    close();
}
