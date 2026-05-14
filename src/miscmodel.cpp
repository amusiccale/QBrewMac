#include <QApplication>
#include <QMessageBox>
#include <QPair>
#include <algorithm>   // ✅ ADD THIS

#include "data.h"
#include "miscmodel.h"

using namespace Resource;

//////////////////////////////////////////////////////////////////////////////
// Constructor

MiscModel::MiscModel(QObject *parent, MiscList *list)
    : QAbstractTableModel(parent), list_(list)
{}

MiscModel::~MiscModel() {}

//////////////////////////////////////////////////////////////////////////////
// flush()

void MiscModel::flush()
{
    beginResetModel();   // ✅ FIX
    endResetModel();     // ✅ FIX
}

//////////////////////////////////////////////////////////////////////////////
// data()

QVariant MiscModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) return QVariant();
    if (index.row() >= list_->count()) return QVariant();

    const Misc &misc = list_->at(index.row());

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
          case NAME:     return misc.name();
          case QUANTITY: return misc.quantity().toString(3);
          case TYPE:     return misc.type();
          case NOTES:    return misc.notes();
          default:       return QVariant();
        }
    }
    else if (role == Qt::EditRole) {
        switch (index.column()) {
          case NAME: return misc.name();
          case QUANTITY: {
              Quantity quantity = misc.quantity();
              quantity.convert(Data::instance()->defaultMiscUnit());
              return quantity.amount();
          }
          case TYPE:  return misc.type();
          case NOTES: return misc.notes();
          default:    return QVariant();
        }
    }
    else if (role == Qt::TextAlignmentRole) {
        switch (index.column()) {
          case NAME:
          case TYPE:
          case NOTES:
              return Qt::AlignLeft;
          default:
              return Qt::AlignRight;
        }
    }

    return QVariant();
}

//////////////////////////////////////////////////////////////////////////////
// setData()

bool MiscModel::setData(const QModelIndex &index,
                        const QVariant &value, int role)
{
    static bool deleting = false;

    if (!index.isValid()) return false;
    if (role != Qt::EditRole) return false;
    if (index.row() >= list_->count()) return false;

    Misc misc = list_->value(index.row());

    switch (index.column()) {
      case NAME: {
          QString name = value.toString();

          if (name.isEmpty()) {
              if (deleting) return false;
              deleting = true;

              int status = QMessageBox::question(QApplication::activeWindow(),
                               TITLE + tr(" - Delete?"),
                               tr("Do you wish to remove this entry?"),
                               QMessageBox::Yes | QMessageBox::Cancel);

              if (status == QMessageBox::Yes) {
                  beginRemoveRows(index.parent(), index.row(), index.row());
                  list_->removeAt(index.row());
                  emit modified();
                  endRemoveRows();
                  deleting = false;
                  return true;
              }
              deleting = false;
              return false;
          }

          if (name == list_->at(index.row()).name())
              return false;

          misc.setName(name);
          if (Data::instance()->hasMisc(name)) {
              Misc newmisc = Data::instance()->misc(name);
              misc.setType(newmisc.type());
              misc.setNotes(newmisc.notes());
          }
          break;
      }

      case QUANTITY:
          misc.setQuantity(Quantity(value.toDouble(),
                                    Data::instance()->defaultMiscUnit()));
          break;

      case TYPE:
          misc.setType(value.toString());
          break;

      case NOTES:
          misc.setNotes(value.toString());
          break;

      default:
          return false;
    }

    list_->replace(index.row(), misc);
    emit modified();

    emit dataChanged(index.sibling(index.row(), NAME),
                     index.sibling(index.row(), NOTES));

    return true;
}

//////////////////////////////////////////////////////////////////////////////
// insertRows()

bool MiscModel::insertRows(int row, int count, const QModelIndex&)
{
    if (count != 1) return false;
    if (row < 0 || row > list_->count()) row = list_->count();

    Misc misc = Data::instance()->misc(tr("Generic"));

    beginInsertRows(QModelIndex(), row, row);
    list_->insert(row, misc);
    emit modified();
    endInsertRows();
    return true;
}

//////////////////////////////////////////////////////////////////////////////
// removeRows()

bool MiscModel::removeRows(int row, int count, const QModelIndex&)
{
    if (count != 1) return false;
    if (row < 0 || row >= list_->count()) return false;

    int status = QMessageBox::question(QApplication::activeWindow(),
                               TITLE + tr(" - Delete?"),
                               tr("Do you wish to remove this entry?"),
                               QMessageBox::Yes | QMessageBox::Cancel);

    if (status == QMessageBox::Cancel) return false;

    beginRemoveRows(QModelIndex(), row, row);
    list_->removeAt(row);
    emit modified();
    endRemoveRows();
    return true;
}

//////////////////////////////////////////////////////////////////////////////
// headerData()

QVariant MiscModel::headerData(int section, Qt::Orientation orientation,
                              int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
          case NAME:     return tr("Misc");
          case QUANTITY: return tr("Quantity");
          case TYPE:     return tr("Type");
          case NOTES:    return tr("Notes");
          default:       return QVariant();
        }
    }
    return QVariant();
}

//////////////////////////////////////////////////////////////////////////////
// flags()

Qt::ItemFlags MiscModel::flags(const QModelIndex &index) const
{
    if (!index.isValid()) return Qt::ItemIsEnabled;
    return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
}

//////////////////////////////////////////////////////////////////////////////
// rowCount()

int MiscModel::rowCount(const QModelIndex &) const
{
    return list_->count();
}

//////////////////////////////////////////////////////////////////////////////
// columnCount()

int MiscModel::columnCount(const QModelIndex &) const
{
    return COUNT;
}

//////////////////////////////////////////////////////////////////////////////
// sort()

void MiscModel::sort(int column, Qt::SortOrder order)
{
    QList<QPair<QString,Misc>> sortlist;

    foreach(Misc misc, *list_) {
        QString field;
        switch (column) {
          case NAME:     field = misc.name(); break;
          case QUANTITY: field = QString::number(misc.quantity().amount()).rightJustified(8,'0'); break;
          case TYPE:     field = misc.type(); break;
          default:       field = misc.notes(); break;
        }
        sortlist.append(qMakePair(field, misc));
    }

    std::sort(sortlist.begin(), sortlist.end());   // ✅ FIX

    emit layoutAboutToBeChanged();

    list_->clear();
    for (const auto &pair : sortlist) {
        if (order == Qt::AscendingOrder)
            list_->append(pair.second);
        else
            list_->prepend(pair.second);
    }

    emit layoutChanged();
}
