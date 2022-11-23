const commandUrl = "/command"
const telemetryUrl = "/telemetry"
const telemetryTime = 1500 // cyclic requests for telemetry [ms] 
const commandTime = 750 // cyclic send of commands [ms]
const maxPing = 200 // max. ping time [ms]
const batteryLimit = 3.8 // [V]

var throttleTouch = 0 // forward=1, backward=-1
var steeringTouch = 0 // left=-1, right=1
var battery = null // [V]
var deviceState = null
var ping = null // [ms]

function fmtNum(value, digits = 0) {
    return value.toLocaleString(
        "en-EN",
        { minimumFractionDigits: digits, maximumFractionDigits: digits }
    )
}

function Board() {
    var commandTimer

    function sendCommand() {
        if (commandTimer != null) {
            clearTimeout(commandTimer)
        }
        const start = new Date().getTime()
        m.request({
            method: "PUT",
            url: commandUrl,
            body: { Throttle: throttleTouch, Steering: steeringTouch }
        }).then(function () {
            ping = new Date().getTime() - start
            // cyclic send commands
            commandTimer = setTimeout(sendCommand, commandTime)
        }).catch(function () {
            ping = null // error
            // cyclic send commands
            commandTimer = setTimeout(sendCommand, commandTime)
        })
    }

    function handleTouch(evt) {
        evt.preventDefault()
        var touched = null
        switch (evt.type) {
            case "mousedown":
                touched = true
                break
            case "mouseup":
                touched = false
                break
            case "touchstart":
            case "touchmove":
                if (evt.targetTouches.length > 0) {
                    touched = true
                }
                break
            case "touchend":
            case "touchcancel":
                if (evt.targetTouches.length === 0) {
                    touched = false
                }
                break
        }
        if (touched !== null) {
            const prevThrottleTouch = throttleTouch
            const prevSteeringTouch = steeringTouch
            switch (evt.target.id) {
                case "forward":
                    if (touched) {
                        throttleTouch = 1
                    } else if (throttleTouch === 1) {
                        throttleTouch = 0
                    }
                    break
                case "backward":
                    if (touched) {
                        throttleTouch = -1
                    } else if (throttleTouch === -1) {
                        throttleTouch = 0
                    }
                    break
                case "left":
                    if (touched) {
                        steeringTouch = -1
                    } else if (steeringTouch === -1) {
                        steeringTouch = 0
                    }
                    break
                case "right":
                    if (touched) {
                        steeringTouch = 1
                    } else if (steeringTouch === 1) {
                        steeringTouch = 0
                    }
                    break
            }
            if (throttleTouch !== prevThrottleTouch || steeringTouch !== prevSteeringTouch) {
                sendCommand()
            }
        }
    }

    function receiveTelemetry() {
        const start = new Date().getTime()
        m.request({
            url: telemetryUrl,
        }).then(function (tele) {
            ping = new Date().getTime() - start
            battery = tele.Battery
            deviceState = tele.State
        }).catch(function () {
            ping = null // error
            battery = null // error
            deviceState = null // error
        })
    }

    function handleTelemetryTimer() {
        receiveTelemetry()
        setTimeout(handleTelemetryTimer, telemetryTime)
    }

    return {
        oninit: function () {
            handleTelemetryTimer()
            sendCommand()
        },
        view: function () {
            return m("",
                m(".h-5-1-2",
                    m(".v-2-1-1",
                        m(".panel.touch#forward", {
                            class: throttleTouch === 1 ? "throttleon" : "throttleoff",
                            onmousedown: handleTouch, onmouseup: handleTouch,
                            ontouchstart: handleTouch, ontouchmove: handleTouch,
                            ontouchend: handleTouch, ontouchcancel: handleTouch
                        }, "↑F")
                    ),
                    m(".v-2-2-2",
                        m(".panel.touch#backward", {
                            class: throttleTouch === -1 ? "throttleon" : "throttleoff",
                            onmousedown: handleTouch, onmouseup: handleTouch,
                            ontouchstart: handleTouch, ontouchmove: handleTouch,
                            ontouchend: handleTouch, ontouchcancel: handleTouch
                        }, "↓B")
                    )
                ),
                m(".h-5-3-3",
                    m(".v-5-1-1",
                        m(".panel.info", m.trust("ESP8266-<br>RC&nbsp;&nbsp;©MDZ"))
                    ),
                    m(".v-5-2-2",
                        m(
                            ".panel",
                            { class: deviceState !== null ? "info" : "error" },
                            "Status:",
                            m("br"),
                            deviceState === null ? "Error" : deviceState
                        )
                    ),
                    m(".v-5-3-3",
                        m(
                            ".panel",
                            { class: battery !== null && battery > batteryLimit ? "info" : "error" },
                            "Battery:",
                            m("br"),
                            battery === null ? "Error" : (fmtNum(battery, 2) + " V")
                        )
                    ),
                    m(".v-5-4-4",
                        m(".panel.info")
                    ),
                    m(".v-5-5-5",
                        m(
                            ".panel",
                            { class: ping !== null && ping < maxPing ? "info" : "error" },
                            "Connect.:",
                            m("br"),
                            ping === null ? "Error" : fmtNum(ping) + " ms"
                        )
                    )
                ),
                m(".h-5-4-5",
                    m(".h-2-1-1",
                        m(".panel.touch#left", {
                            class: steeringTouch === -1 ? "steeringon" : "steeringoff",
                            onmousedown: handleTouch, onmouseup: handleTouch,
                            ontouchstart: handleTouch, ontouchmove: handleTouch,
                            ontouchend: handleTouch, ontouchcancel: handleTouch
                        }, m.trust("←<br>L"))
                    ),
                    m(".h-2-2-2",
                        m(".panel.touch#right", {
                            class: steeringTouch === 1 ? "steeringon" : "steeringoff",
                            onmousedown: handleTouch, onmouseup: handleTouch,
                            ontouchstart: handleTouch, ontouchmove: handleTouch,
                            ontouchend: handleTouch, ontouchcancel: handleTouch
                        }, m.trust("→<br>R"))
                    )
                )
            );
        },
        onremove: function () {
            clearTimeout(telemetryTimer)
        }
    }
}

m.mount(document.body, Board);
