pragma Singleton

import QtQuick

QtObject {
    readonly property string blank: "—"

    function duration(totalSeconds) {
        const total = Math.max(0, Math.round(totalSeconds));
        const hours = Math.floor(total / 3600);
        const minutes = Math.floor((total % 3600) / 60);

        if (hours > 0 && minutes > 0)
            return qsTr("%1h %2m").arg(hours).arg(minutes);
        if (hours > 0)
            return qsTr("%1h").arg(hours);
        if (minutes > 0)
            return qsTr("%1m").arg(minutes);
        // Sessions can be seconds long — the timer logs whatever it measured.
        return qsTr("%1s").arg(total);
    }

    function compact(value) {
        const total = Math.max(0, Math.round(value));
        if (total < 1000)
            return total.toString();
        if (total < 1000000)
            return qsTr("%1K").arg(Format._oneDecimal(total / 1000));
        return qsTr("%1M").arg(Format._oneDecimal(total / 1000000));
    }

    function _oneDecimal(value) {
        return value < 10 ? value.toFixed(1) : Math.round(value).toString();
    }

    // The pattern stays out of the translation catalogue on purpose:
    // `bootstrap.py translate` machine translates what it scans and turns
    // "d MMM, HH:mm" into tokens Qt no longer recognises. The locale localizes
    // the month name anyway.
    function stamp(date) {
        if (!date || isNaN(date.getTime()))
            return "";
        return date.toLocaleString(Qt.locale(), "d MMM, HH:mm");
    }
}
