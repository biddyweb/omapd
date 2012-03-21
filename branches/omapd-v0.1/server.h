/*
server.h: Definition of Server class

Copyright (C) 2010  Sarab D. Mattes <mattes@nixnux.org>

This file is part of omapd.

omapd is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

omapd is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with omapd.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SERVER_H
#define SERVER_H

#include <QTcpServer>
#include <qtsoap.h>

#include "identifier.h"
#include "metadata.h"
#include "mapgraph.h"

enum IFMAP_ERRORCODES_1 {
    ErrorNone = 0,
    IfmapAccessDenied,
    IfmapFailure, // Unspecified failure
    IfmapInvalidIdentifier,
    IfmapInvalidIdentifierType,
    IfmapIdentifierTooLong,
    IfmapInvalidMetadata,
    IfmapInvalidMetadataListType,
    IfmapInvalidSchemaVersion,
    IfmapInvalidSessionID,
    IfmapMetadataTooLong,
    IfmapSearchResultsTooBig,
    IfmapSystemError // Server error
};

class Server : public QTcpServer
{
    Q_OBJECT
public:
    enum Debug {
                DebugNone = 0x00,
                ShowClientOps = 0x01,
                ShowXML = 0x02,
                ShowHTTPHeaders = 0x04,
                ShowHTTPState = 0x08,
                ShowXMLParsing = 0x10,
                ShowXMLFilterResults = 0x20,
                ShowXMLFilterStatements = 0x40
            };
    Q_DECLARE_FLAGS(DebugOptions, Debug);

    enum NonStdBehavior {
                DisableNonStdBehavior = 0x0,
                EnablePubIdHint = 0x1,
                IgnoreSessionId = 0x2
                      };
    Q_DECLARE_FLAGS(NonStdBehaviorOptions, NonStdBehavior);

    Server(MapGraph *mapGraph, quint16 port = 8081, QObject *parent = 0);
    //void incomingConnection(int socket);
    int readHeader(QTcpSocket *socket);
    int readRequestData(QTcpSocket *socket);
    void sendResponse(QTcpSocket *socket, const QtSoapMessage & respMsg);

private slots:
    void newClientConnection();
    void readClient();
    void discardClient();
    void processRequest(QTcpSocket *socket, QtSoapMessage reqMsg);
    void clientConnState(QAbstractSocket::SocketState sState);
    void buildPollResults();

signals:
    void headerReceived(QString hdr);
    void requestMessageReceived(QTcpSocket *socket, QtSoapMessage reqMsg);
    void checkSubscriptionsForPolls();

private:
    MapGraph* _mapGraph;

    QSet<QTcpSocket*> _headersReceived;
    QHash<QString, QTcpSocket*> _activePolls;  // pubId --> QTcpSocket

    /* TODO: If there are multiple Server instances (e.g. in a thread pool)
       these objects will need to be synchronized across those instances.
       That's why I created a MapSessions class.
    */
    QHash<QString, QList<SearchGraph> > _subscriptionLists;  // pubId --> all subscriptions for pubId
    QHash<QString, QString> _activeARCSessions;  // pubId --> sessId
    QHash<QString, QString> _activeSSRCSessions; // pubId --> sessId

    Link keyFromNodeList(QDomNodeList ids, bool isLink);
    Id idFromNode(QDomNode idNode);

    bool searchParameters(QDomNamedNodeMap searchAttrs, int *maxDepth, QString *matchLinks, int *maxSize, QString *resultFilter);

    int filteredMetadata(QList<Meta> metaList, QString filter, bool invert = false, QtSoapStruct *metaResult = 0);
    bool addSearchResultsWithResultFilter(QtSoapStruct *soapResponse, int maxSize, QString resultFilter, QSet<Id> idList, QSet<Link> linkList);
    void buildSearchGraph(Id startId, QString matchLinks, int maxDepth,
                int currentDepth,
                QSet<Id> *idList,
                QSet<Link> *linkList);
    Id otherIdForLink(Link link, Id targetId);
    void markSubscriptionsForPolls(Link link, bool isLink);

    int pollResultsForPublisherId(QtSoapStruct *pollResult, QString publisherId);
    QtSoapType* soapResponseForOperation(QString operation, IFMAP_ERRORCODES_1 operationError);
    QtSoapStruct* soapStructForId(Id id);

    bool terminateARCSession(QString publisherId);


    Server::DebugOptions _debug;
    Server::NonStdBehaviorOptions _nonStdBehavior;
};
Q_DECLARE_OPERATORS_FOR_FLAGS(Server::DebugOptions)
Q_DECLARE_OPERATORS_FOR_FLAGS(Server::NonStdBehaviorOptions)

#endif // SERVER_H