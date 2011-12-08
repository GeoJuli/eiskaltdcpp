/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "UserListModel.h"

#include <QtAlgorithms>
#include <QtGlobal>

#include "dcpp/stdinc.h"
#include "dcpp/FavoriteManager.h"
#include "dcpp/ClientManager.h"
#include "dcpp/Util.h"

#include "WulforUtil.h"

void UserListProxyModel::sort(int column, Qt::SortOrder order){
    if (sourceModel())
        sourceModel()->sort(column, order);
}

UserListModel::UserListModel(QObject * parent) : QAbstractItemModel(parent) {
    sortColumn = COLUMN_SHARE;
    sortOrder = Qt::DescendingOrder;
    stripper.setPattern("\\[.*\\]");
    stripper.setMinimal(true);

    _needResort = false;

    t = new QTimer();
    t->setSingleShot(true);
    t->setInterval(7000);
    connect(t, SIGNAL(timeout()), this, SLOT(slotResort()));

    rootItem = new UserListItem(NULL);

    WU = WulforUtil::getInstance();
}


UserListModel::~UserListModel() {
    delete rootItem;

    t->deleteLater();
}


int UserListModel::rowCount(const QModelIndex & ) const {
    return rootItem->childCount();
}

int UserListModel::columnCount(const QModelIndex & ) const {
    return 7;
}

bool UserListModel::hasChildren(const QModelIndex &parent) const{
    return (!parent.isValid());
}

bool UserListModel::canFetchMore(const QModelIndex &parent) const{
    return false;
}

QVariant UserListModel::data(const QModelIndex & index, int role) const {
    if (!index.isValid())
        return QVariant();

    UserListItem * item = static_cast<UserListItem*>(index.internalPointer());

    if (!item)
        return QVariant();

    switch (role){
        case Qt::DisplayRole:
        {
            switch (index.column()) {
                case COLUMN_NICK: return item->getNick();
                case COLUMN_COMMENT: return item->getComment();
                case COLUMN_TAG: return item->getTag();
                case COLUMN_CONN: return item->getConnection();
                case COLUMN_EMAIL: return item->getEmail();
                case COLUMN_SHARE: return WulforUtil::formatBytes(item->getShare());
                case COLUMN_IP: return item->getIP();
            }

            break;
        }
        case Qt::DecorationRole:
        {
            if (index.column() != COLUMN_NICK)
                break;

            return (*WU->getUserIcon(item->ptr, item->isAway(), item->isOP(), item->getConnection()));

            break;
        }
        case Qt::ToolTipRole:
        {
            if (index.column() == COLUMN_SHARE)
                return QString::number(item->getShare());
            else {
                QString ttip = "";

                ttip =  "<b>" + headerData(COLUMN_NICK, Qt::Horizontal, Qt::DisplayRole).toString() + "</b>: " + item->getNick() + "<br/>";
                ttip += "<b>" + headerData(COLUMN_COMMENT, Qt::Horizontal, Qt::DisplayRole).toString() + "</b>: " + item->getComment() + "<br/>";
                ttip += "<b>" + headerData(COLUMN_EMAIL, Qt::Horizontal, Qt::DisplayRole).toString() + "</b>: " + item->getEmail() + "<br/>";
                ttip += "<b>" + headerData(COLUMN_IP, Qt::Horizontal, Qt::DisplayRole).toString() + "</b>: " + item->getIP() + "<br/>";
                ttip += "<b>" + headerData(COLUMN_SHARE, Qt::Horizontal, Qt::DisplayRole).toString() + "</b>: " +
                        WulforUtil::formatBytes(item->getShare()) + "<br/>";

                QString tag = item->getTag();
                WulforUtil::getInstance()->textToHtml(tag, true);

                ttip += "<b>" + headerData(COLUMN_TAG, Qt::Horizontal, Qt::DisplayRole).toString() + "</b>: " + tag + "<br/>";
                ttip += "<b>" + headerData(COLUMN_CONN, Qt::Horizontal, Qt::DisplayRole).toString() + "</b>: " + item->getConnection() + "<br/>";

                if (item->isOP())
                    ttip += tr("<b>Hub role</b>: Operator");
                else
                    ttip += tr("<b>Hub role</b>: User");

                if (item->isFav())
                    ttip += tr("<br/><b>Favorite user</b>");

                return ttip;
            }

            break;
        }
        case Qt::TextAlignmentRole:
        {
            if (index.column() == COLUMN_SHARE)
                return static_cast<int>(Qt::AlignRight | Qt::AlignVCenter);

            break;
        }
        case Qt::FontRole:
        {
            QFont font;
            font.setBold(true);

            if (item->isFav() && WBGET(WB_CHAT_HIGHLIGHT_FAVS))
                return font;

            break;
        }
    }

    return QVariant();
}


QVariant UserListModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if ((orientation == Qt::Horizontal) && (role == Qt::DisplayRole)) {
        switch (section) {
            case COLUMN_NICK: return tr("Nick");
            case COLUMN_COMMENT: return tr("Comment");
            case COLUMN_TAG: return tr("Tag");
            case COLUMN_CONN: return tr("Connection");
            case COLUMN_EMAIL: return tr("E-mail");
            case COLUMN_SHARE: return tr("Share");
            case COLUMN_IP: return tr("IP");
        }
    }

    return QVariant();
}

namespace {

template <Qt::SortOrder order>
struct Compare {
    void static sort(unsigned column, QList<UserListItem*>& items) {
        if (column > COLUMN_EMAIL)
            return;
        
        typedef bool (*AttrComp)(const UserListItem* l, const UserListItem* r);
        
        static AttrComp attrs[7] = {    AttrCmp<QString, &UserListItem::getNick>,
                                        AttrCmp<qulonglong, &UserListItem::getShare>,
                                        AttrCmp<QString, &UserListItem::getComment>,
                                        AttrCmp<QString, &UserListItem::getTag>,
                                        AttrCmp<QString, &UserListItem::getConnection>,
                                        IPCmp,
                                        AttrCmp<QString, &UserListItem::getEmail> };

        qStableSort(items.begin(), items.end(), [&attrs,column] (const UserListItem *l, const UserListItem *r) { return attrs[column](l, r); });
    }

    QList<UserListItem*>::iterator static insertSorted(unsigned column, QList<UserListItem*>& items, UserListItem* item) {
        if (column > COLUMN_EMAIL)
            return items.end();
         
        typedef bool (*AttrComp)(const UserListItem* l, const UserListItem* r);
        
        static AttrComp attrs[7] = {    AttrCmp<QString, &UserListItem::getNick>,
                                        AttrCmp<qulonglong, &UserListItem::getShare>,
                                        AttrCmp<QString, &UserListItem::getComment>,
                                        AttrCmp<QString, &UserListItem::getTag>,
                                        AttrCmp<QString, &UserListItem::getConnection>,
                                        IPCmp,
                                        AttrCmp<QString, &UserListItem::getEmail> };

        return qLowerBound(items.begin(), items.end(), item, [&attrs,column] (const UserListItem *l, const UserListItem *r) { return attrs[column](l, r); });
    }

    private:
        typedef bool (*AttrComp)(const UserListItem* l, const UserListItem* r);

        template <typename T, T (UserListItem::*attr)() const >
        bool static AttrCmp(const UserListItem * l, const UserListItem * r) {
            if (l->isOP() != r->isOP())
                return l->isOP();
            else if (l->isFav() != r->isFav())
                return l->isFav();
            else
                return Cmp((const_cast<UserListItem*>(l)->*attr)(), (const_cast<UserListItem*>(r)->*attr)());
        }

        bool static IPCmp(const UserListItem * l, const UserListItem * r){
            if (l->isOP() != r->isOP())
                return l->isOP();
            else if (!(l->getIP().isEmpty() || r->getIP().isEmpty())){
                QString ip1 = l->getIP();
                QString ip2 = r->getIP();

                quint32 l_ip = ip1.section('.',0,0).toULong();
                l_ip <<= 8;
                l_ip |= ip1.section('.',1,1).toULong();
                l_ip <<= 8;
                l_ip |= ip1.section('.',2,2).toULong();
                l_ip <<= 8;
                l_ip |= ip1.section('.',3,3).toULong();

                quint32 r_ip = ip2.section('.',0,0).toULong();
                r_ip <<= 8;
                r_ip |= ip2.section('.',1,1).toULong();
                r_ip <<= 8;
                r_ip |= ip2.section('.',2,2).toULong();
                r_ip <<= 8;
                r_ip |= ip2.section('.',3,3).toULong();

                return Cmp(l_ip, r_ip);
            }

            return false;
        }

        template <typename T>
        inline bool static Cmp(const T& l, const T& r) __attribute__((always_inline));
};

template <> template <typename T>
bool inline Compare<Qt::AscendingOrder>::Cmp(const T& l, const T& r) {
    return l < r;
}

template <> template <typename T>
bool inline Compare<Qt::DescendingOrder>::Cmp(const T& l, const T& r) {
    return l > r;
}

template <> template <>
bool inline Compare<Qt::AscendingOrder>::Cmp(const QString& l, const QString& r) {
    return Cmp(QString::localeAwareCompare(l, r), 0);
}

template <> template <>
bool inline Compare<Qt::DescendingOrder>::Cmp(const QString& l, const QString& r) {
    return Cmp(QString::localeAwareCompare(l, r), 0);
}

} //namespace

typedef Compare<Qt::AscendingOrder>     AscendingCompare;
typedef Compare<Qt::DescendingOrder>    DescendingCompare;

void UserListModel::sort(int column, Qt::SortOrder order) {
    static AscendingCompare  acomp = AscendingCompare();
    static DescendingCompare dcomp = DescendingCompare();

    sortColumn = column;
    sortOrder = order;

    if (column < 0 || column > columnCount() - 1)
        return;

    if (rootItem->childItems.size() <= 0)
        return;

    emit layoutAboutToBeChanged();

    if (order == Qt::AscendingOrder)
        acomp.sort(column, rootItem->childItems);
    else if (order == Qt::DescendingOrder)
        dcomp.sort(column, rootItem->childItems);

    emit layoutChanged();
}

QModelIndex UserListModel::index(int row, int column, const QModelIndex &) const {
    if (row > (rootItem->childCount() - 1) || row < 0)
        return QModelIndex();

    return createIndex(row, column, rootItem->child(row));
}

QModelIndex UserListModel::parent(const QModelIndex & ) const {
    return QModelIndex();
}

void UserListModel::clear() {
    emit layoutAboutToBeChanged();

    users.clear();

    qDeleteAll(rootItem->childItems);

    rootItem->childItems.clear();

    emit layoutChanged();
}

void UserListModel::needResort(){
    if (_needResort)
        return;

    _needResort = true;

    t->start();
}

void UserListModel::removeUser(const UserPtr &ptr) {
    USRMap::iterator iter = users.find(ptr);

    if (iter == users.end())
        return;

    const int index = (iter.value())->row();

    beginRemoveRows(QModelIndex(), index, index);

    UserListItem *item = iter.value();

    rootItem->childItems.removeAt(index);
    delete item;

    users.erase(iter);

    endRemoveRows();
}

void UserListModel::addUser(const QString& nick,
                            const QString& cid,
                            const UserPtr &ptr)
{
    static AscendingCompare  acomp = AscendingCompare();
    static DescendingCompare dcomp = DescendingCompare();

    if (users.contains(ptr))
        return;

    UserListItem *item;

    item = new UserListItem(rootItem, ptr);
    item->cid = cid;

    users.insert(ptr, item);

    if (sortColumn == -1) // if sorting disabled
    {
        const int i = rootItem->childCount();

        beginInsertRows(QModelIndex(), i, i);
        rootItem->appendChild(item);
        endInsertRows();

        return;
    }

    QList<UserListItem*>::iterator it = rootItem->childItems.end();

    if (sortOrder == Qt::AscendingOrder)
        it = acomp.insertSorted(sortColumn, rootItem->childItems, item);
    else if (sortOrder == Qt::DescendingOrder)
        it = dcomp.insertSorted(sortColumn, rootItem->childItems, item);

    const int pos = it - rootItem->childItems.begin();

    beginInsertRows(QModelIndex(), pos, pos);

    rootItem->childItems.insert(it, item);

    endInsertRows();
}

UserListItem *UserListModel::itemForPtr(const UserPtr &ptr){
    USRMap::iterator iter = users.find(ptr);

    UserListItem *item = (iter != users.end())? (iter.value()) : (NULL);

    return item;
}

UserListItem *UserListModel::itemForNick(const QString &nick, const QString &aHubUrl){
    if (!nick.isEmpty()){
        dcpp::UserPtr ptr = dcpp::ClientManager::getInstance()->getUser(_tq(nick), _tq(aHubUrl));

        USRMap::const_iterator ut = users.find(ptr);

        if (ut != users.constEnd())
            return ut.value();
    }

    return NULL;
}

QString UserListModel::CIDforNick(const QString &nick, const QString &aHubUrl){
    Q_UNUSED(aHubUrl);

    QModelIndexList indexes = match(index(0, COLUMN_NICK, QModelIndex()), Qt::DisplayRole, nick, 1, Qt::MatchExactly);

    if (indexes.size() != 1)
        return "";

    QModelIndex i = indexes.takeFirst();
    UserListItem *item = reinterpret_cast<UserListItem*>(i.internalPointer());

    return (item? item->cid : "");
}

QStringList UserListModel::matchNicksContaining(const QString & part, bool stripTags) const {
    QStringList matches;

    if (part.isEmpty()) {
        return matches;
    }

    for (QList<UserListItem*>::const_iterator it = rootItem->childItems.constBegin(); it != rootItem->childItems.constEnd(); ++it) {
        QString nick_lc = (*it)->getNick().toLower();

        if (nick_lc.contains(part)) {
                matches << (*it)->getNick();
        }
    }

    return matches;
}

QStringList UserListModel::matchNicksStartingWith(const QString & part, bool stripTags) const {
    QStringList matches;

    if (part.isEmpty()) {
        return matches;
    }

    for (QList<UserListItem*>::const_iterator it = rootItem->childItems.constBegin(); it != rootItem->childItems.constEnd(); ++it) {
        QString nick_lc = (*it)->getNick().toLower();

        if (nick_lc.startsWith(part)) {
            matches << (*it)->getNick();
        }
    }

    return matches;
}

QStringList UserListModel::matchNicksAny(const QString &part, bool stripTags) const{
    QStringList matches;

    if (part.isEmpty()) {
        return matches;
    }

    for (QList<UserListItem*>::const_iterator it = rootItem->childItems.constBegin(); it != rootItem->childItems.constEnd(); ++it) {
        QString nick_lc = (*it)->getNick().toLower();

        if (nick_lc.startsWith(part) || nick_lc.contains(part)) {
            matches << (*it)->getNick();
        }
    }

    return matches;
}

QStringList UserListModel::findItems(const QString &part, Qt::MatchFlags flags, int column) const
{
    QModelIndexList indexes = match(index(0, column, QModelIndex()),
                                    Qt::DisplayRole, part, -1, flags);
    QStringList items;
    for (int i = 0; i < indexes.size(); ++i) {
        QModelIndex index = indexes.at(i);
        if (index.isValid())
            items.append( index.data().toString() );
    }
    return items;
}

void UserListModel::repaintItem(const UserListItem *item){
    int r = rootItem->childItems.indexOf(const_cast<UserListItem*>(item));

    if (!(item && r >= 0))
        return;

    repaintData(createIndex(r, COLUMN_NICK, const_cast<UserListItem*>(item)), createIndex(r, COLUMN_EMAIL, const_cast<UserListItem*>(item)));
}

UserListItem::UserListItem(UserListItem *parent, dcpp::UserPtr p) :
    parentItem(parent), ptr(p)
{
    updateIdentity();
}

UserListItem::~UserListItem()
{
    qDeleteAll(childItems);
}

void UserListItem::appendChild(UserListItem *item) {
    item->parentItem = this;
    childItems.append(item);
}

UserListItem *UserListItem::child(int row) {
    return childItems.value(row);
}

int UserListItem::childCount() const {
    return childItems.count();
}

int UserListItem::columnCount() const {
    return 7;
}
UserListItem *UserListItem::parent() {
    return parentItem;
}

int UserListItem::row() const {
    if (parentItem)
        return parentItem->childItems.indexOf(const_cast<UserListItem*>(this));

    return 0;
}

qulonglong UserListItem::getShare()  const{
    return id.getBytesShared();
}

QString UserListItem::getComment()  const{
    return _q(id.getDescription());
}

QString UserListItem::getConnection()  const{
    return _q(id.getConnection());
}

QString UserListItem::getEmail()  const{
    return _q(id.getEmail());
}

QString UserListItem::getIP()  const{
    return _q(id.getIp());
}   

QString UserListItem::getNick()  const{
    return _q(id.getNick());
}

QString UserListItem::getTag()  const{
    return _q(id.getTag());
}

bool UserListItem::isOP()  const{
    return id.isOp();
}

bool UserListItem::isFav()  const{
    return FavoriteManager::getInstance()->isFavoriteUser(ptr);
}

bool UserListItem::isAway() const {
    return id.isAway();
}

void UserListItem::updateIdentity() {
    if (ptr != NULL)
        id = dcpp::ClientManager::getInstance()->getOnlineUserIdentity(ptr);
}
