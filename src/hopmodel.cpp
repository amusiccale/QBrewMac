#include <QApplication>
#include <QMessageBox>
#include <QPair>
#include <algorithm>          // ✅ ADD THIS

#include "data.h"
#include "hopmodel.h"

using namespace Resource;

//////////////////////////////////////////////////////////////////////////////
// Constructor

HopModel::HopModel(QObject *parent, HopList *list)
    : QAbstractTableModel(parent), list_(list)
{}

HopModel::~HopModel() {}

//////////////////////////////////////////////////////////////////////////////
// flush()

void HopModel::flush()
{
    beginResetModel();        // ✅ FIX
    endResetModel();          // ✅ FIX
}

//////////////////////////////////////////////////////////////////////////////
// data()

QVariant HopModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) return QVariant();
    if (index.row() >= list_->count()) return QVariant();

    const Hop &hop = list_->at(index.row());

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
          case NAME:   return hop.name();
          case WEIGHT: return hop.weight().toString(3);
          case ALPHA:  return QString::number(hop.alpha(), 'f', 1) + '%';
          case TIME:   return QString::number(hop.time()) + tr(" min", "minutes");
          case TYPE:   return hop.type();
          default:     return QVariant();
        }
    }
    else if (role == Qt::EditRole) {
        switch (index.column()) {
          case NAME:   return hop.name();
          case WEIGHT: {
              Weight weight = hop.weight();
              weight.convert(Data::instance()->defaultHopUnit());
              return weight.amount();
          }
          case ALPHA:  return hop.alpha();
          case TIME:   return hop.time();
          case TYPE:   return hop.type();
          default:     return QVariant();
        }
    }
    else if (role == Qt::TextAlignmentRole) {
        switch (index.column()) {
          case NAME:
          case TYPE:   return Qt::AlignLeft;
          default:     return Qt::AlignRight;
        }
    }

    return QVariant();
}

//////////////////////////////////////////////////////////////////////////////
// setData()

bool HopModel::setData(const QModelIndex &index,
                       const QVariant &value, int role)
{
    static bool deleting = false;

    if (!index.isValid()) return false;
    if (role != Qt::EditRole) return false;
    if (index.row() >= list_->count()) return false;

    Hop hop = list_->value(index.row());

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

          if (name == list_->at(index.row()).name()) return false;

          hop.setName(name);
          if (Data::instance()->hasHop(name)) {
              Hop newhop = Data::instance()->hop(name);
              hop.setType(newhop.type());
              hop.setAlpha(newhop.alpha());
          }
          break;
      }

      case WEIGHT:
          hop.setWeight(Weight(value.toDouble(),
                               Data::instance()->defaultHopUnit()));
          break;

      case ALPHA:
          hop.setAlpha(value.toDouble());
          break;

      case TIME:
          hop.setTime(value.toUInt());
          break;

      case TYPE:
          hop.setType(value.toString());
          break;

      default:
          return false;
    }

    list_->replace(index.row(), hop);
    emit modified();

    emit dataChanged(index.sibling(index.row(), NAME),
                     index.sibling(index.row(), TIME));

    return true;
}

//////////////////////////////////////////////////////////////////////////////
// insertRows()

bool HopModel::insertRows(int row, int count, const QModelIndex&)
{
    if (count != 1) return false;
    if (row < 0 || row > list_->count()) row = list_->count();

    Hop hop = Data::instance()->hop(tr("Generic"));

    beginInsertRows(QModelIndex(), row, row);
    list_->insert(row, hop);
    emit modified();
    endInsertRows();
    return true;
}

//////////////////////////////////////////////////////////////////////////////
// removeRows()

bool HopModel::removeRows(int row, int count, const QModelIndex&)
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

QVariant HopModel::headerData(int section, Qt::Orientation orientation,
                             int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
          case NAME:   return tr("Hop");
          case WEIGHT: return tr("Weight");
          case ALPHA:  return tr("Alpha");
          case TIME:   return tr("Time");
          case TYPE:   return tr("Type");
          default:     return QVariant();
        }
    }
    return QVariant();
}

//////////////////////////////////////////////////////////////////////////////
// flags()

Qt::ItemFlags HopModel::flags(const QModelIndex &index) const
{
    if (!index.isValid()) return Qt::ItemIsEnabled;
    return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
}

//////////////////////////////////////////////////////////////////////////////
// rowCount()

int HopModel::rowCount(const QModelIndex &) const
{
    return list_->count();
}

//////////////////////////////////////////////////////////////////////////////
// columnCount()

int HopModel::columnCount(const QModelIndex &) const
{
    return COUNT;
}

//////////////////////////////////////////////////////////////////////////////
// sort()

void HopModel::sort(int column, Qt::SortOrder order)
{
    QList<QPair<QString,Hop>> sortlist;

    foreach(Hop hop, *list_) {
        QString field;
        switch (column) {
          case NAME:   field = hop.name(); break;
          case WEIGHT: field = QString::number(hop.weight().amount()).rightJustified(8,'0'); break;
          case ALPHA:  field = QString::number(hop.alpha(),'f',1).rightJustified(8,'0'); break;
          case TYPE:   field = hop.type(); break;
          default:     field = QString::number(hop.time()).rightJustified(8,'0'); break;
        }
        sortlist.append(qMakePair(field, hop));
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