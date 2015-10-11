/***************************************************************************
 *   Copyright (C) 2007 by David Pinelo   *
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

#include "seleccionestilodlg.h"
#include "globales.h"
#include "ui_seleccionEstilo.h"
#include "configuracion.h"

StyleSelectDlg::StyleSelectDlg(QWidget* parent, Qt::WindowFlags fl)
    : QDialog( parent, fl ), ui(new Ui::SeleccionarEstiloDlg)
{
    ui->setupUi(this);
    m_rbs = new QButtonGroup(this);

    QStringList styles = QStyleFactory::keys();

    int id = 0;
    foreach (const QString &style, styles)
    {
        QRadioButton *rb = new QRadioButton(style, this);
        rb->setProperty("style", style);
        m_rbs->addButton(rb, id);
        ui->groupBox->layout()->addWidget(rb);
        id++;
    }

    m_pushedButton = alephERPSettings->lookAndFeel();
    selectButton (alephERPSettings->lookAndFeel());
    ui->chkDarkVersion->setChecked(alephERPSettings->lookAndFeelDark());
    connect (m_rbs, SIGNAL(buttonClicked(int)), this, SLOT(buttonPushed(int)));
    connect (ui->bbBotones, SIGNAL(accepted()), this, SLOT(ok()));
    connect (ui->bbBotones, SIGNAL(rejected()), this, SLOT(cancel()));
}

StyleSelectDlg::~StyleSelectDlg()
{
    delete m_rbs;
    delete ui;
}

/*$SPECIALIZATION$*/

void StyleSelectDlg::selectButton(const QString &boton)
{
    QList<QRadioButton *> rbs = findChildren<QRadioButton *>();
    foreach (QRadioButton *rb, rbs)
    {
        if ( rb->property("style").toString() == boton )
        {
            rb->setChecked(true);
            return;
        }
    }
}

void StyleSelectDlg::buttonPushed(int estilo)
{
    QRadioButton *rb = static_cast<QRadioButton *> (m_rbs->button(estilo));
    if ( rb != 0 )
    {
        if ( rb->isChecked() )
        {
            m_pushedButton = rb->property("style").toString();
        }
    }

}

void StyleSelectDlg::ok()
{
    alephERPSettings->setLookAndFeel(m_pushedButton);
    alephERPSettings->setLookAndFeelDark(ui->chkDarkVersion->isChecked());
    this->close();
}

void StyleSelectDlg::cancel()
{
    this->close();
}

void StyleSelectDlg::closeEvent(QCloseEvent * event)
{
    event->accept();
    emit closingWindow(this);
}


