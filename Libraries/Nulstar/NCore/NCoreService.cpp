#include <QTimer>
#include "NCoreService.h"

namespace NulstarNS {
  const QString cConstantsFile("Constants.ncf");
  const QString cCommServerLabel("Nulstar Internal Communication");
  const QString cDefaultMinEventAndMinPeriod("0,0");
  const QString cServiceManagerName("ServiceManager") ;

  NCoreService::NCoreService(NWebSocketServer::SslMode lSslMode, ELogLevel lLogLevel, const QHostAddress& lIP, const QUrl& lServiceManagerUrl, const QList<QNetworkAddressEntry>& lAllowedNetworks,
                             QObject *rParent)
              : QObject(rParent), mLogLevel(lLogLevel), mServiceManagerUrl(lServiceManagerUrl), mIP(lIP), mSslMode(lSslMode), mAllowedNetworks(lAllowedNetworks) {
    QTimer::singleShot(0, this, [this] { mApiBuilder.fBuildApi(this); });
  }

  NCoreService::~NCoreService() {
    for(NWebSocketServer* rWebServer : mWebServers) {
      if(rWebServer->isListening()) rWebServer->close();
      rWebServer->deleteLater();
    }
    for(NWebSocket* rWebSocket : mWebSockets) {
      if(rWebSocket) {
        rWebSocket->fClose();
        rWebSocket->deleteLater();
      }
    }
  }

  bool NCoreService::fAddWebSocketServer(quint16 lPort, QHostAddress::SpecialAddress lBindAddress, const QString& lName, const QString& lLabel, bool lStartImmediatly) {
    if(mWebServers.contains(lName))
      return false;

    QString lEffectiveName(lName);
    QString lEffectiveLabel(lLabel);
    if(lEffectiveName.isEmpty())
      lEffectiveName = cCommServerName;
    if(lEffectiveLabel.isEmpty())
      lEffectiveLabel = cCommServerLabel;

    NWebSocketServer* pWebServer = new NWebSocketServer(lEffectiveName, lEffectiveLabel, mSslMode, fProtocolVersionsSupported(), nullptr);
    pWebServer->fSetPort(lPort);
    pWebServer->fSetBindAddress(lBindAddress);
    mWebServers[pWebServer->fName()] = pWebServer;
    if(lStartImmediatly)
      fControlWebServer(pWebServer->fName(), EServiceAction::eStartService);
    return true;
  }

  quint16 NCoreService::fCommPort() const {
    if(mWebServers.contains(cCommServerName))
      return mWebServers.value(cCommServerName)->fPort();
    return 0;
  }

  void NCoreService::fConnectToServiceManager(quint8 lReconnectionTryInterval) {
    NWebSocket* rWebSocket = nullptr;
    if(mWebSockets.contains(cServiceManagerName)) {
      rWebSocket = mWebSockets.value(cServiceManagerName);
      rWebSocket->fConnect();
    }
    else {
      rWebSocket = new NWebSocket(cServiceManagerName, fProtocolVersionsSupported().at(0).toString(), mServiceManagerUrl, lReconnectionTryInterval);
      connect(rWebSocket, &NWebSocket::sStateChanged, this, &NCoreService::fOnConnectionStateChanged);
      mWebSockets.insert(cServiceManagerName, rWebSocket);
      rWebSocket->fConnect();
    }
  }

  /*** NResponse NCoreService::fSetMaxConnections(const QString& lName, int lMaxconnections) {
    if(mWebServers.contains(lName)) {
      mWebServers[lName]->fSetMaxConnections(lMaxconnections);
      NResponse lResponse(true, true);
      return lResponse;
    }
    NResponse lResponse(false, false, QDate::currentDate().toString("yyyy-MM-dd"), QTime::currentTime().toString("hh:mm:ss"), tr("Web server '%1' not found.").arg(lName));
    return lResponse;
  } ***/

  void NCoreService::fOnConnectionStateChanged(NWebSocket::EConnectionState lNewState) {
     NWebSocket* rWebSocket = qobject_cast<NWebSocket* > (sender());
     if(rWebSocket) {
         switch(lNewState) {
           case NWebSocket::EConnectionState::eConnectionActive: {
             rWebSocket->fRegisterApi(mApiBuilder.fApi());
             break;
           }
           default:
             break;
         }
     }
  }

  bool NCoreService::fControlWebServer(const QString &lName, EServiceAction lAction) {
    QStringList lWebServerNames;
    if(lName.isEmpty()) lWebServerNames = mWebServers.keys();
    else lWebServerNames << lName;
    for( const QString& lCurrentName : lWebServerNames) {
      if(!mWebServers.contains(lCurrentName))
        return false;
      if(lAction == EServiceAction::eStartService) {
        if(!mWebServers[lCurrentName]->fListen()) {
          qDebug() << QString("Websockets server '%1' of module '%2' could not initiated!").arg(lCurrentName).arg(fName());
          return false;
        }
      }
      if(lAction == EServiceAction::eStopService) {
        mWebServers[lCurrentName]->close();
      }
      if(lAction == EServiceAction::eRestartService) {
        mWebServers[lCurrentName]->close();
        if(!mWebServers[lCurrentName]->fListen()) {
          qDebug() << QString("Websockets server '%1' of module '%2' could not initiated!").arg(lCurrentName).arg(fName());
          return false;
        }
      }
    }
    return true;
  }

 /*** NResponse NCoreService::fMaxConnections(const QString &lName) {
    if(mWebServers.contains(lName)) {
      NResponse lResponse(true, mWebServers[lName]->fMaxConnections());
      return lResponse;
    }
    NResponse lResponse(false, 0, tr("Web server '%1' not found.").arg(lName));
    return lResponse;
  }***/

  QString NCoreService::fMethodDescription(const QString& lMethodName) const {
    return mApiMethodDescription.value(lMethodName);
  }

  QString NCoreService::fMethodMinEventAndMinPeriod(const QString& lMethodName) const {
    if(mApiMethodMinEventAndMinPeriod.contains(lMethodName)) return mApiMethodMinEventAndMinPeriod.value(lMethodName);
    else return cDefaultMinEventAndMinPeriod;
  }

/***  NResponse NCoreService::fTotalConnections(const QString &lName) {
    if(mWebServers.contains(lName)) {
      NResponse lResponse(true, mWebServers[lName]->fTotalConnections());
      return lResponse;
    }
    NResponse lResponse(false, 0, tr("Web server '%1' not found.").arg(lName));
    return lResponse;
  } ***/
}
