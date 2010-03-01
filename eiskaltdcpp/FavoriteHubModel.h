/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef FavoriteHubModel_H
#define FavoriteHubModel_H

#include <QAbstractItemModel>

#define COLUMN_HUB_AUTOCONNECT      0
#define COLUMN_HUB_NAME             1
#define COLUMN_HUB_DESC             2
#define COLUMN_HUB_ADDRESS          3
#define COLUMN_HUB_NICK             4
#define COLUMN_HUB_PASSWORD         5
#define COLUMN_HUB_USERDESC         6
#define COLUMN_HUB_ENCODING         7


class FavoriteHubItem{

public:

    FavoriteHubItem(const QList<QVariant> &data, FavoriteHubItem *parent = NULL);
    ~FavoriteHubItem();

    void appendChild(FavoriteHubItem *child);

    FavoriteHubItem *child(int row);
    int childCount() const;
    int columnCount() const;
    QVariant data(int column) const;
    int row() const;
    FavoriteHubItem *parent();

    QList<FavoriteHubItem*> childItems;

    void updateColumn(unsigned, QVariant);

private:
    QList<QVariant> itemData;
    FavoriteHubItem *parentItem;
};

class FavoriteHubModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    FavoriteHubModel(QObject *parent = 0);
    virtual ~FavoriteHubModel();

    /** */
    QVariant data(const QModelIndex &, int) const;
    /** */
    Qt::ItemFlags flags(const QModelIndex &) const;
    /** */
    QVariant headerData(int section, Qt::Orientation, int role = Qt::DisplayRole) const;
    /** */
    QModelIndex index(int, int, const QModelIndex &parent = QModelIndex()) const;
    /** */
    QModelIndex parent(const QModelIndex &index) const;
    /** */
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    /** */
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    /** sort list */
    virtual void sort(int column, Qt::SortOrder order = Qt::AscendingOrder);

    /** */
    void addResult(QList<QVariant> &data);
    /** */
    const QList<FavoriteHubItem*> &getItems();

    /** Clear the model and redraw it*/
    void clearModel();

    /** */
    bool removeItem(const QModelIndex&);
    /** */
    bool removeItem(const FavoriteHubItem*);

    /** */
    void repaint();

private:

    FavoriteHubItem *rootItem;

    int sortColumn;
    Qt::SortOrder sortOrder;
};

#endif // FavoriteHubModel_H
