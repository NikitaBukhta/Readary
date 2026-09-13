pragma Singleton

import QtQuick

QtObject {
    id: root

    signal requested(string message)

    function show(message) {
        root.requested(message);
    }
}
