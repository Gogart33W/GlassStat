import QtQuick

Canvas {
    id: canvas

    property var   historyData: []
    property color lineColor:   "#8b5cf6"
    property color fillColor:   Qt.rgba(lineColor.r, lineColor.g, lineColor.b, 0.15)
    property real  lineWidth:   1.5
    property real  maxValue:    100.0

    antialiasing: true

    onHistoryDataChanged: requestPaint()
    onWidthChanged:       requestPaint()
    onHeightChanged:      requestPaint()

    onPaint: {
        var ctx = canvas.getContext("2d");
        ctx.clearRect(0, 0, canvas.width, canvas.height);

        if (!historyData || historyData.length < 2 || canvas.width <= 0 || canvas.height <= 0)
            return;

        var count = historyData.length;
        var step  = canvas.width / (count - 1);
        var h     = canvas.height;

        ctx.lineWidth   = canvas.lineWidth;
        ctx.strokeStyle = canvas.lineColor;

        ctx.beginPath();
        for (var i = 0; i < count; i++) {
            var val   = Math.max(0.0, Math.min(canvas.maxValue, historyData[i]));
            var normY = h - (val / canvas.maxValue) * (h - 2) - 1;
            var x     = i * step;

            if (i === 0) {
                ctx.moveTo(x, normY);
            } else {
                ctx.lineTo(x, normY);
            }
        }
        ctx.stroke();

        ctx.lineTo(canvas.width, h);
        ctx.lineTo(0, h);
        ctx.closePath();

        var gradient = ctx.createLinearGradient(0, 0, 0, h);
        gradient.addColorStop(0.0, canvas.fillColor);
        gradient.addColorStop(1.0, Qt.rgba(canvas.lineColor.r, canvas.lineColor.g, canvas.lineColor.b, 0.0));
        ctx.fillStyle = gradient;
        ctx.fill();
    }
}
