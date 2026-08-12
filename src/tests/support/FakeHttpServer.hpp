#ifndef READARY_TESTS_SUPPORT_FAKEHTTPSERVER_HPP
#define READARY_TESTS_SUPPORT_FAKEHTTPSERVER_HPP

#include <QByteArray>
#include <QHash>
#include <QHostAddress>
#include <QList>
#include <QString>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUrl>
#include <QUrlQuery>
#include <functional>
#include <utility>

namespace readary::tests {

class FakeHttpServer {
public:
  using Responder = std::function<QByteArray(const QUrl &target)>;

  explicit FakeHttpServer(Responder responder) : _responder{std::move(responder)} {
    QObject::connect(&_server, &QTcpServer::newConnection, [this] { acceptPending(); });
  }

  bool start() { return _server.listen(QHostAddress::LocalHost, 0); }

  QString endpoint(const QString &basePath = {}) const {
    return QStringLiteral("http://127.0.0.1:%1%2").arg(_server.serverPort()).arg(basePath);
  }

  void setFailureStatus(const QByteArray &statusLine) { _failureStatus = statusLine; }
  void setFailRequests(bool fail) { _failRequests = fail; }
  void setTransientFailures(int count) { _failFirst = count; }

  int requestCount() const { return _requestCount; }
  QUrl lastTarget() const { return _lastTarget; }
  QString lastQueryItem(const QString &name) const {
    return QUrlQuery{_lastTarget.query()}.queryItemValue(name, QUrl::FullyDecoded);
  }

private:
  void acceptPending() {
    while (QTcpSocket *socket = _server.nextPendingConnection()) {
      QObject::connect(socket, &QTcpSocket::readyRead, [this, socket] { onReadyRead(socket); });
      QObject::connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
    }
  }

  void onReadyRead(QTcpSocket *socket) {
    QByteArray &buffer = _buffers[socket];
    buffer.append(socket->readAll());
    if (!buffer.contains("\r\n\r\n")) {
      return; // headers still arriving
    }

    _lastTarget = requestTarget(buffer);
    ++_requestCount;
    _buffers.remove(socket);

    if (_failRequests || _requestCount <= _failFirst) {
      writeRaw(socket, "HTTP/1.1 " + _failureStatus + "\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
      return;
    }

    const QByteArray body = _responder ? _responder(_lastTarget) : QByteArray{};
    writeRaw(socket, "HTTP/1.1 200 OK\r\nContent-Type: application/json; charset=utf-8\r\nContent-Length: " +
                         QByteArray::number(body.size()) + "\r\nConnection: close\r\n\r\n" + body);
  }

  static QUrl requestTarget(const QByteArray &request) {
    const QByteArray requestLine = request.left(request.indexOf("\r\n"));
    const QList<QByteArray> parts = requestLine.split(' ');
    return QUrl{parts.size() >= 2 ? QString::fromUtf8(parts.at(1)) : QString{}};
  }

  static void writeRaw(QTcpSocket *socket, const QByteArray &response) {
    socket->write(response);
    socket->flush();
    socket->waitForBytesWritten(3000);
    socket->disconnectFromHost();
  }

  QTcpServer _server;
  Responder _responder;
  QHash<QTcpSocket *, QByteArray> _buffers;
  QUrl _lastTarget;
  QByteArray _failureStatus{"500 Internal Server Error"};
  int _requestCount{0};
  int _failFirst{0};
  bool _failRequests{false};
};

} // namespace readary::tests

#endif // READARY_TESTS_SUPPORT_FAKEHTTPSERVER_HPP
