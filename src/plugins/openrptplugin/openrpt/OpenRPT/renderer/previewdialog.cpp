///////////////////////////////////////////////////////////////////////////////
// previewdialog.h
// -------------------
// Copyright (c) 2007 David Johnson <david@usermode.org>
// Please see the header file for copyright and license information.
///////////////////////////////////////////////////////////////////////////////

#include "previewdialog.h"
#include "orprerender.h"
#include "orprintrender.h"
#include "renderobjects.h"
// dpinelo ----------------------------------
#include "configuracion.h"
// dpinelo ----------------------------------
#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include <math.h>

const int spacing = 30;


///////////////////////////////////////////////////////////////////////////////
// PreviewDialog                                                             //
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// PreviewDialog()
///////////////////////////////////////////////////////////////////////////////
/// Construct a PreviewDialog object. 
///////////////////////////////////////////////////////////////////////////////

PreviewDialog::PreviewDialog(ORODocument *document,
                             QPrinter *pPrinter,
                             QWidget *parent)
  : QDialog(parent, Qt::Window), _view(0)
{
    setWindowTitle(tr("Print Preview - OpenRPT modified for AlephERP"));

    // dpinelo ----------------------------------
    setObjectName("OpenRPTPluginPreviewDialog");
    // dpinelo ----------------------------------

    // widgets
    _view = new PreviewWidget(document, pPrinter, this);

    QToolButton *zoominbutton = new QToolButton(this);
    zoominbutton->setText(tr("Zoom in"));
    zoominbutton->setToolTip(tr("Zoom in"));
    zoominbutton->setIcon(QIcon(":/preview/viewzoomin.png"));

    QToolButton *zoomoutbutton = new QToolButton(this);
    zoomoutbutton->setText(tr("Zoom out"));
    zoomoutbutton->setToolTip(tr("Zoom out"));
    zoomoutbutton->setIcon(QIcon(":/preview/viewzoomout.png"));

    QDialogButtonBox *buttonbox = new QDialogButtonBox(this);
    buttonbox->setOrientation(Qt::Horizontal);
    // dpinelo ----------------------------------------
    QPushButton *pbPrint = buttonbox->addButton(tr("Print"), QDialogButtonBox::AcceptRole);
    QPushButton *pbCancel = buttonbox->addButton(tr("Cancel"), QDialogButtonBox::RejectRole);
    pbPrint->setIcon(QIcon(":/preview/printer_32.png"));
    pbCancel->setIcon(QIcon(":/preview/close.png"));

    m_chkAdjustView = new QCheckBox(this);
    m_chkAdjustView->setText(tr("Adjust view page size to dialog size"));
    m_chkAdjustView->setChecked(false);
    m_chkAdjustView->setVisible(false);
    _view->adjustToDialogWidth(Qt::Unchecked);
    // dpinelo ----------------------------------------

    // layouts
    QVBoxLayout *mainlayout = new QVBoxLayout(this);
    QHBoxLayout *buttonlayout = new QHBoxLayout();
    
    buttonlayout->addWidget(zoominbutton);
    buttonlayout->addWidget(zoomoutbutton);
    buttonlayout->addWidget(m_chkAdjustView);
    buttonlayout->addStretch(10);
    buttonlayout->addWidget(buttonbox);

    mainlayout->addWidget(_view, 10);
    mainlayout->addLayout(buttonlayout);

    // dpinelo: Comentado
    // resize(800, 600);
    // setWindowState(Qt::WindowMaximized);
    // dpinelo

    connect(zoominbutton, SIGNAL(clicked()), _view, SLOT(zoomIn()));
    connect(zoomoutbutton, SIGNAL(clicked()), _view, SLOT(zoomOut()));
    connect(buttonbox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonbox, SIGNAL(rejected()), this, SLOT(reject()));
    connect(m_chkAdjustView, SIGNAL(clicked()), _view, SLOT(updateView()));
    connect(m_chkAdjustView, SIGNAL(stateChanged(int)), _view, SLOT(adjustToDialogWidth(int)));
}

///////////////////////////////////////////////////////////////////////////////
// ~PreviewDialog()
///////////////////////////////////////////////////////////////////////////////
/// Destroy the dialog.
///////////////////////////////////////////////////////////////////////////////

PreviewDialog::~PreviewDialog()
{
	if (_view) {
		delete _view;
		_view = 0;
	}
}

///////////////////////////////////////////////////////////////////////////////
// PreviewWidget                                                             //
///////////////////////////////////////////////////////////////////////////////

// PreviewWidget() ////////////////////////////////////////////////////////////
// constructor

PreviewWidget::PreviewWidget(ORODocument *document,
                             QPrinter *pPrinter,
                             QWidget *parent)
    : QAbstractScrollArea(parent),
      _doc(document), _pPrinter(pPrinter), _zoom(1.0), mousepos(), scrollpos()
{
    viewport()->setBackgroundRole(QPalette::Dark);
    verticalScrollBar()->setSingleStep(25);
    horizontalScrollBar()->setSingleStep(25);
}

// ~PreviewWidget() ///////////////////////////////////////////////////////////
// destructor

PreviewWidget::~PreviewWidget()
{
}

// updateView() ///////////////////////////////////////////////////////////////
// update the view
void PreviewWidget::updateView()
{
    resizeEvent(0);
    viewport()->update();
}

void PreviewWidget::adjustToDialogWidth(int value)
{
    _adjustToDialogWidth = value;
}


// zoomIn() ///////////////////////////////////////////////////////////////////
// zoom in to view

void PreviewWidget::zoomIn()
{
    _zoom += 0.2;
    // update
    resizeEvent(0);
    viewport()->update();

}

// zoomOut() //////////////////////////////////////////////////////////////////
// zoom out from view

void PreviewWidget::zoomOut()
{
    _zoom = qMax(_zoom - 0.2, 0.2);
    // update
    resizeEvent(0);
    viewport()->update();
}

// mousePressEvent() //////////////////////////////////////////////////////////
// click to start view drag

void PreviewWidget::mousePressEvent(QMouseEvent *e)
{
    mousepos = e->pos();
    scrollpos.rx() = horizontalScrollBar()->value();
    scrollpos.ry() = verticalScrollBar()->value();
    e->accept();
}

// mouseMoveEvent() ///////////////////////////////////////////////////////////
// drag view

void PreviewWidget::mouseMoveEvent(QMouseEvent *e)
{
    if (mousepos.isNull()) {
        e->ignore();
        return;
    }

    horizontalScrollBar()->setValue(scrollpos.x() - e->pos().x() + mousepos.x());
    verticalScrollBar()->setValue(scrollpos.y() - e->pos().y() + mousepos.y());
    horizontalScrollBar()->update();
    verticalScrollBar()->update();
    e->accept();
}

// mouseReleaseEvent() ////////////////////////////////////////////////////////
// release finished view drag

void PreviewWidget::mouseReleaseEvent(QMouseEvent *e)
{
    mousepos = QPoint();
    e->accept();
}

// paintEvent() ///////////////////////////////////////////////////////////////
// paint document on widget

void PreviewWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(viewport());

    painter.translate(-horizontalScrollBar()->value(),
                      -verticalScrollBar()->value());
    painter.translate(spacing, spacing);

    int nbCol = nbColumns();
    double paperwidth = paperRect(viewport()).width();
    double paperheight = paperRect(viewport()).height();

    if ( nbCol > 1 )
    {
        painter.translate(spacing, spacing);
    }
    else
    {
        // dpinelo: Aquí centraremos el papel en la vista, caso de tener una sola columna
        int x = (width() / 2) - (columnWidth() / 2);
        painter.translate(x, spacing);
    }

    for(int page = 0; page < _doc->pages(); page++)
    {
        int column = page % nbCol;

        painter.save();

        // dpinelo: Si tenemos activado el que la hoja de papel se redimensione acorde a la página, se ejecuta
        qreal xScale = (qreal)(width() - spacing) / (qreal)columnWidth() + (_zoom - 1);
        if ( nbCol == 1 && _adjustToDialogWidth == Qt::Checked )
        {
            qDebug() << "xScale: " << xScale;
            painter.scale(xScale, xScale);
        }
        else
        {
        painter.scale(_zoom, _zoom);
        }

        // draw outline and shadow
        painter.setPen(Qt::black);
        painter.setBrush(Qt::white);
        painter.drawRect(QRectF(0, 0, paperwidth, paperheight));
        painter.setBrush(Qt::NoBrush);

        painter.drawLine(QLineF(paperwidth+1, 2, paperwidth+1, paperheight+2));
        painter.drawLine(QLineF(paperwidth+2, 2, paperwidth+2, paperheight+2));
        painter.drawLine(QLineF(2, paperheight+1, paperwidth, paperheight+1));
        painter.drawLine(QLineF(2, paperheight+2, paperwidth, paperheight+2));

        qreal xDpi = parentWidget()->logicalDpiX();
        qreal yDpi = parentWidget()->logicalDpiY();

        QSize margins(_pPrinter->paperRect().left() - _pPrinter->pageRect().left(), _pPrinter->paperRect().top() - _pPrinter->pageRect().top());

        ORPrintRender::renderPage(_doc, page, &painter, xDpi, yDpi, margins, 100);

        painter.restore();
        int xTranslation = column==nbCol-1 ? (columnWidth()* -(nbCol-1)) : columnWidth();
        int yTranslation = column==nbCol-1 ? (int)(spacing + paperRect(viewport()).height() * _zoom) : 0;
        // dpinelo --------------------------------------
        if ( nbCol == 1 && _adjustToDialogWidth == Qt::Checked )
        {
            xTranslation = columnWidth();
            yTranslation = (int)(spacing + (paperRect(viewport()).height() * xScale));
        }
        // dpinelo --------------------------------------
        painter.translate(xTranslation, yTranslation);
    }
}

// resizeEvent() //////////////////////////////////////////////////////////////
// view has resized

void PreviewWidget::resizeEvent(QResizeEvent *)
{
    QSize docsize;
    const QSize viewsize = viewport()->size();

    docsize.setWidth(qRound(paperRect(viewport()).width() *
                            _zoom + 2 * spacing));

    int nbLines = (int)ceil ((double)_doc->pages() / (double)nbColumns());
    docsize.setHeight(qRound(nbLines * paperRect(viewport()).height() *
                             _zoom + (nbLines + 1) * spacing));

    horizontalScrollBar()->setRange(0, docsize.width() - viewsize.width());
    horizontalScrollBar()->setPageStep(viewsize.width());

    verticalScrollBar()->setRange(0, docsize.height() - viewsize.height());
    verticalScrollBar()->setPageStep(viewsize.height());

}

// paperRect() ////////////////////////////////////////////////////////////////
// Return the size of the paper, adjusted for DPI

QRectF PreviewWidget::paperRect(QPaintDevice *device)
{
    // calculate size of paper
    QRectF rect = _pPrinter->paperRect();
    // adjust for DPI
    rect.setWidth(rect.width() *
        device->logicalDpiX() / _pPrinter->logicalDpiX());
    rect.setHeight(rect.height() *
        device->logicalDpiY() / _pPrinter->logicalDpiY());

    return rect;
}


int PreviewWidget::columnWidth()
{
    return (int)(spacing + paperRect(viewport()).width() * _zoom);
}

int PreviewWidget::nbColumns()
{
    int colWidth = columnWidth();
    int viewWidth = width() - spacing;
    return viewWidth<=colWidth ? 1 : viewWidth/colWidth;
}

// dpinelo ---------------------------------------------------------------
void PreviewDialog::closeEvent(QCloseEvent *event)
{
    // Guardamos las dimensiones del usuario
    alephERPSettings->savePosForm(this);
    alephERPSettings->saveDimensionForm(this);
    event->accept();
}

void PreviewDialog::showEvent(QShowEvent *event)
{
    // Cargamos las dimensiones guardadas de la ventana
    alephERPSettings->applyPosForm(this);
    alephERPSettings->applyDimensionForm(this);
    event->accept();
}

void PreviewDialog::reject()
{
    // Guardamos las dimensiones del usuario
    alephERPSettings->savePosForm(this);
    alephERPSettings->saveDimensionForm(this);
    QDialog::reject();
}
// dpinelo ---------------------------------------------------------------
