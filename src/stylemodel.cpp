#include <QApplication>
#include <QMessageBox>
#include <QPair>
#include <algorithm>   // ✅ ADD

#include "data.h"
#include "stylemodel.h"

using namespace Resource;

//////////////////////////////////////////////////////////////////////////////
// Constructor

StyleModel::StyleModel(QObject *parent, StyleList *list)
    : QAbstractTableModel(parent), list_(list)
{}

StyleModel::~StyleModel(){}

//////////////////////////////////////////////////////////////////////////////
// flush()

void StyleModel::flush()
{
    beginResetModel();   // ✅ FIX
    endResetModel();     // ✅ FIX
}

//////////////////////////////////////////////////////////////////////////////
// data()

QVariant StyleModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) return QVariant();
    if (index.row() >= list_->count()) return QVariant();

    const Style &style = list_->at(index.row());

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
          case NAME:   return style.name();
          case OGLOW:  return QString::number(style.oglow_, 'f', 3);
          case OGHI:   return QString::number(style.oghi_, 'f', 3);
          case FGLOW:  return QString::number(style.fglow_, 'f', 3);
          case FGHI:   return QString::number(style.fghi_, 'f', 3);
          case IBULOW: return QString::number(style.ibulow_);
          case IBUHI:  return QString::number(style.ibuhi_);
          case SRMLOW: return QString::number(style.srmlow_);
          case SRMHI:  return QString::number(style.srmhi_);
          default:     return QVariant();
        }
    }
    else if (role == Qt::EditRole) {
        switch (index.column()) {
          case NAME:   return style.name();
          case OGLOW:  return style.oglow_;
          case OGHI:   return style.oghi_;
          case FGLOW:  return style.fglow_;
          case FGHI:   return style.fghi_;
          case IBULOW: return style.ibulow_;
          case IBUHI:  return style.ibuhi_;
          case SRMLOW: return style.srmlow_;
          case SRMHI:  return style.srmhi_;
          default:     return QVariant();
        }
    }
    else if (role == Qt::TextAlignmentRole) {
        switch (index.column()) {
          case NAME: return Qt::AlignLeft;
          default:   return Qt::AlignRight;
        }
    }

    return QVariant();
}

//////////////////////////////////////////////////////////////////////////////
// setData()

bool StyleModel::setData(const QModelIndex &index,
                         const QVariant &value, int role)
{
    static bool deleting = false;

    if (!index.isValid()) return false;
    if (role != Qt::EditRole) return false;

    int row = index.row();
    int column = index.column();

    if ((row >= list_->count()) && (column != NAME)) return false;

    Style style;
    if (row < list_->count()) style = list_->value(row);

    switch (column) {
      case NAME: {
          QString name = value.toString();

          if (name.isEmpty()) {
              if (row >= list_->count()) return false;
              if (deleting) return false;
              deleting = true;

              beginRemoveRows(index.parent(), row, row);  // ✅ OK already
              list_->removeAt(row);
              emit modified();
              endRemoveRows();

              deleting = false;
              return true;
          }

          style.setName(name);
          break;
      }

      case OGLOW:  style.oglow_ = value.toDouble(); break;
      case OGHI:   style.oghi_ = value.toDouble(); break;
      case FGLOW:  style.fglow_ = value.toDouble(); break;
      case FGHI:   style.fghi_ = value.toDouble(); break;
      case IBULOW: style.ibulow_ = value.toUInt(); break;
      case IBUHI:  style.ibuhi_ = value.toUInt(); break;
      case SRMLOW: style.srmlow_ = value.toUInt(); break;
      case SRMHI:  style.srmhi_ = value.toUInt(); break;

      default:
          return false;
    }

    list_->replace(row, style);
    emit modified();

    emit dataChanged(index.sibling(row, NAME),
                     index.sibling(row, SRMHI));

    return true;
}

//////////////////////////////////////////////////////////////////////////////
// insertRows()

bool StyleModel::insertRows(int row, int count, const QModelIndex&)
{
    if (count != 1) return false;
    if (row < 0 || row >= list_->count()) row = list_->count();

    Style style = Data::instance()->style(tr("Generic"));

    beginInsertRows(QModelIndex(), row, row);
    list_->insert(row, style);
    emit modified();
    endInsertRows();
    return true;
}

//////////////////////////////////////////////////////////////////////////////
// removeRows()

bool StyleModel::removeRows(int row, int count, const QModelIndex&)
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

QVariant StyleModel::headerData(int section, Qt::Orientation orientation,
                                int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
          case NAME:   return tr("Style");
          case OGLOW:  return tr("Min. OG");
          case OGHI:   return tr("Max. OG");
          case FGLOW:  return tr("Min. FG");
          case FGHI:   return tr("Max. FG");
          case IBULOW: return tr("Min. IBU");
          case IBUHI:  return tr("Max. IBU");
          case SRMLOW: return tr("Min. SRM");
          case SRMHI:  return tr("Max. SRM");
          default:     return QVariant();
        }
    }
    return QVariant();
}

//////////////////////////////////////////////////////////////////////////////
// flags()

Qt::ItemFlags StyleModel::flags(const QModelIndex &index) const
{
    if (!index.isValid()) return Qt::ItemIsEnabled;
    return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
}

//////////////////////////////////////////////////////////////////////////////
// rowCount()

int StyleModel::rowCount(const QModelIndex &) const
{
    return list_->count();
}

//////////////////////////////////////////////////////////////////////////////
// columnCount()

int StyleModel::columnCount(const QModelIndex &) const
{
    return COUNT;
}

//////////////////////////////////////////////////////////////////////////////
// sort()

void StyleModel::sort(int column, Qt::SortOrder order)
{
    QList<QPair<QString,Style>> sortlist;

    foreach(Style style, *list_) {
        QString field;
        switch (column) {
          case NAME:   field = style.name(); break;
          case OGLOW:  field = QString::number(style.oglow_).rightJustified(6,'0'); break;
          case OGHI:   field = QString::number(style.oghi_).rightJustified(6,'0'); break;
          case FGLOW:  field = QString::number(style.fglow_).rightJustified(6,'0'); break;
          case FGHI:   field = QString::number(style.fghi_).rightJustified(6,'0'); break;
          case IBULOW: field = QString::number(style.ibulow_).rightJustified(6,'0'); break;
          case IBUHI:  field = QString::number(style.ibuhi_).rightJustified(6,'0'); break;
          case SRMLOW: field = QString::number(style.srmlow_).rightJustified(6,'0'); break;
          case SRMHI:  field = QString::number(style.srmhi_).rightJustified(6,'0'); break;
          default:     field = QString(); break;
        }
        sortlist.append(qMakePair(field, style));
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