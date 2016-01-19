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
#ifndef DBComboBox_H
#define DBComboBox_H

#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include <QtScript>
#include <alepherpglobal.h>
#include <aerpcommon.h>
#include "widgets/dbbasewidget.h"
#include "dao/beans/basebean.h"

class DBComboBoxPrivate;

/**
    @author David Pinelo <alepherp@alephsistemas.es>
    Este combo será el utilizado para presentar datos asociados a los items. La idea es
    que cada item, lleve asociado un objeto de la clase BaseBean. Para ello, este combo,
    tiene un método setup, por el que es posible pasarle un objeto de tipo ComboModel, que
    devuelve un conjunto de objetos BaseBean, que son los que poblarán de información
    el combo.

    Notas sobre el despliegue automatico en
    http://aicastell.blogspot.com/2009/08/qt4-qcomboboxes-y-el-foco-del-teclado.html
*/
class ALEPHERP_DLL_EXPORT DBComboBox : public QComboBox, public DBBaseWidget
{
    Q_OBJECT
    /** Campo en el que se guardará el valor seleccionado por el usuario. Si se especifica
      un relationName, entonces este será el fieldName del que depende esa relación, y el valor
      se almacenará en el bean hijo, en el field relationFieldName y en el bean hijo que pase
      el filtro relationFilter */
    Q_PROPERTY (QString fieldName READ fieldName WRITE setFieldName)
    Q_PROPERTY (QString relationFilter READ relationFilter WRITE setRelationFilter)
    /** Indica qué tabla poblará de items el combobox */
    Q_PROPERTY (QString listTableModel READ listTableModel WRITE setListTableModel)
    /** Esta será la columna que se visualizará y que el usuario podrá escoger */
    Q_PROPERTY (QString listColumnName READ listColumnName WRITE setListColumnName)
    /** Filtro para el modelo que presenta los Items. Este filtro no es SQL, sino que se aplica con un proxy
     * por lo que podría ser más lento.
        TODO: OJO, Hay un bug documentado en 4.7.X
          https://bugreports.qt.nokia.com/browse/QTBUG-18215
          que provoca que este mapFromSource pueda no funcionar. Por eso
          hago este workaround cutre, que deja sin posibilidad de filtrar
          los datos de la lista. El mapFromSource deja de funcionar por utilizar
          filterAcceptColumn
    */
    Q_PROPERTY (QString listFilter READ listFilter WRITE setListFilter)
    /** Realiza un filtrado SQL de los datos obtenidos */
    Q_PROPERTY (QString listSqlFilter READ listSqlFilter WRITE setListSqlFilter)
    /** Esta será la columna que se almacenará en el fieldName del bean que da soporte al formulario */
    Q_PROPERTY (QString listColumnToSave READ listColumnToSave WRITE setListColumnToSave)
    Q_PROPERTY (bool dataEditable READ dataEditable WRITE setDataEditable)
    Q_PROPERTY (bool aerpControl READ aerpControl)
    Q_PROPERTY (bool userModified READ userModified WRITE setUserModified)
    Q_PROPERTY (QVariant value READ value WRITE setValue)
    Q_PROPERTY (int currentIndex READ currentIndex WRITE setCurrentIndex)
    Q_PROPERTY (bool dataFromParentDialog READ dataFromParentDialog WRITE setDataFromParentDialog)
    /** A veces, es deseable que cuando el control recibe el foco, automáticamente despliegue su lista. Esta propiedad
     permite controlar ese comportamiento */
    Q_PROPERTY (bool openListOnGetFocus READ openListOnGetFocus WRITE setOpenListOnGetFocus)
    Q_PROPERTY (QString reportParameterBinding READ reportParameterBinding WRITE setReportParameterBinding)
    Q_PROPERTY (BaseBeanSharedPointer selectedBean READ selectedBean)
    /** En aquellos casos en los que el combobox se use como filtro (y no almacene valores a través de fieldName)
     * se permitirá añadir a la vista un "View All" o "Ver todos" o lo que el usuario seleccione. */
    Q_PROPERTY (bool addCustomItem READ addCustomItem WRITE setAddCustomItem DESIGNABLE enableCustomItem)
    /** Caso de tener un item "Ver todos", esta propiedad establecerá el valor que devolverá el combobox. */
    Q_PROPERTY (QVariant customItemValue READ customItemValue WRITE setCustomItemValue DESIGNABLE enableCustomItem)
    /** Caso de tener un item "Ver todos", u otra parecida, aquí se establecerá el texto que el usuario verá. */
    Q_PROPERTY (QString customItemText READ customItemText WRITE setCustomItemText DESIGNABLE enableCustomItem)
    /** Icono para esa propiedad adicional que se mezcla con los registros */
    Q_PROPERTY (QString customItemIcon READ customItemIcon WRITE setCustomItemIcon DESIGNABLE enableCustomItem)

private:
    DBComboBoxPrivate *d;
    Q_DECLARE_PRIVATE(DBComboBox)
    Q_DISABLE_COPY(DBComboBox)

    void conexiones();
    void desconexiones();
    void createCompleter();
    void setSelectionModel();
    void setModelColumn();
    void init();

public:
    explicit DBComboBox(QWidget * parent = 0);
    ~DBComboBox();


    AlephERP::ObserverType observerType(BaseBean *)
    {
        return AlephERP::DbField;
    }

    QString listTableModel() const;
    void setListTableModel(const QString &name);
    void setListColumnName (const QString &visibleColumn);
    QString listColumnName () const;
    void setListColumnToSave ( const QString &dbColumn );
    QString listColumnToSave () const;
    void setListFilter (const QString &filter);
    QString listFilter() const;
    QString listSqlFilter() const;
    void setListSqlFilter(const QString &filter);
    bool openListOnGetFocus();
    void setOpenListOnGetFocus(bool value);
    bool addCustomItem() const;
    void setAddCustomItem(bool value);
    QVariant customItemValue() const;
    void setCustomItemValue(const QVariant &value);
    QString customItemText() const;
    void setCustomItemText(const QString &value);
    QString customItemIcon() const;
    void setCustomItemIcon(const QString &value);
    bool enableCustomItem();

    Q_INVOKABLE BaseBeanSharedPointer bean(int index);

    // TODO Si utilizamos directamente un QComboBox, QtScript no reconoce directamente estas funciones
    // Hago este truquito mientras
    Q_INVOKABLE void addItem (const QString & text, const QVariant & userData = QVariant());
    Q_INVOKABLE QVariant itemData (int index, int role = Qt::UserRole) const;
    Q_INVOKABLE BaseBeanSharedPointer selectedBean();

    void applyFieldProperties();

    QVariant value();

    int currentIndex() const;
    void setCurrentIndex ( int index );

    static QScriptValue toScriptValue(QScriptEngine *engine, DBComboBox * const &in);
    static void fromScriptValue(const QScriptValue &object, DBComboBox * &out);

protected:
    bool eventFilter(QObject * obj, QEvent * event);
    void focusInEvent(QFocusEvent * event);
    void showEvent(QShowEvent *event);
    void hideEvent(QHideEvent *event)
    {
        DBBaseWidget::hideEvent(event);
        QWidget::hideEvent(event);
    }
    void keyPressEvent(QKeyEvent * event);
    void focusOutEvent(QFocusEvent * event);

public slots:
    void setCurrentIndexByDbColumn (const QString &dbModelColumn, const QVariant &value);
    void setValue(const QVariant &value);
    void refresh();
    void observerUnregistered();
    int findText(const QString &text, Qt::MatchFlags flags = static_cast<Qt::MatchFlags>(Qt::MatchExactly|Qt::MatchCaseSensitive)) const;
    virtual void askToRecalculateCounterField()
    {
        DBBaseWidget::askToRecalculateCounterField();
    }
    void setFocus() { QWidget::setFocus(); }

private slots:
    void itemChanged(int index);
    void currentTextChanged(const QString &text);
    void userModifiedCombo(int index);
    void setValueFromModel(const QModelIndex &index);

signals:
    /** Esta señal propia, es igual que currentIndexChanged, pero emite el id del bean del item. El id hace referencia
      a la primary key */
    void valueEdited(const QVariant &value);
    void destroyed(QWidget *widget);
    void fieldNameChanged();

};

Q_DECLARE_METATYPE(DBComboBox*)
Q_SCRIPT_DECLARE_QMETAOBJECT(DBComboBox, DBComboBox*)

#endif
