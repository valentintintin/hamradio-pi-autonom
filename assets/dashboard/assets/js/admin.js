$(function () {

    const url = 'http://127.0.0.1:3000/api/'
    const apikeyInput = $('#apikey');
    const dataBrut = $('#data-json');
    const dataJsonWrapper = $('#data-json-wrapper');

    getLogs();

    $('#see-logs').click(function () {
        getLogs();
    });

    $('#see-sensors').click(function () {
        getSensors();
    });

    $('#switch-off-radio').click(function () {
        switchRadio(false);
    });

    $('#switch-on-radio').click(function () {
        switchRadio(true);
    });

    $('#take-photo').click(function () {
        takePhoto();
    });

    $('#send-sstv').click(function () {
        sendSstv();
    });

    $('#send-voice').click(function () {
        sendVoice();
    });

    $('#send-aprs-beacon').click(function () {
        sendAprsBeacon();
    });

    $('#send-aprs-telem').click(function () {
        sendAprsTelem();
    });

    $('#run-repeater').click(function () {
        runRepeater();
    });

    $('#enable-watchdog').click(function () {
        watchdog(true);
    });

    $('#disable-watchdog').click(function () {
        watchdog(false);
    });

    $('#shutdown-pi').click(function () {
        const date = new Date();
        date.setMinutes(date.getMinutes() + 5);
        shutdownPi(prompt('Timestamp', date.getTime()));
    });

    function getLogs() {
        $.get(url + 'logs?apikey=' + apikeyInput.val())
            .done(showData)
            .fail(showData);
    }

    function getSensors() {
        $.get(url + 'sensors?apikey=' + apikeyInput.val())
            .done(showData)
            .fail(showData);
    }

    function switchRadio(state) {
        $.post(url + 'gpio/0/' + (state ? '1' : '0') + '?apikey=' + apikeyInput.val())
            .done(showData)
            .fail(showData);
    }

    function takePhoto() {
        $.post(url + 'webcam?apikey=' + apikeyInput.val())
            .done(showData)
            .fail(showData);
    }

    function sendSstv() {
        $.post(url + 'sstv?apikey=' + apikeyInput.val())
            .done(showData)
            .fail(showData);
    }

    function sendVoice() {
        $.post(url + 'voice?apikey=' + apikeyInput.val())
            .done(showData)
            .fail(showData);
    }

    function sendAprsBeacon() {
        $.post(url + 'aprs/beacon?apikey=' + apikeyInput.val())
            .done(showData)
            .fail(showData);
    }

    function sendAprsTelem() {
        $.post(url + 'aprs/telemetry?apikey=' + apikeyInput.val())
            .done(showData)
            .fail(showData);
    }

    function runRepeater() {
        $.post(url + 'repeater?apikey=' + apikeyInput.val())
            .done(showData)
            .fail(showData);
    }

    function watchdog(state) {
        $.post(url + 'watchdog/' + (state ? 'start' : 'stop') + '?apikey=' + apikeyInput.val())
            .done(showData)
            .fail(showData);
    }

    function shutdownPi(timestamp) {
        if (!timestamp) {
            return;
        }
        $.post(url + 'shutdown/' + timestamp + '?apikey=' + apikeyInput.val())
            .done(showData)
            .fail(showData);
    }

    function showData(data) {
        console.log(data);
        dataBrut.html(JSON.stringify(data, null, 4));
        dataJsonWrapper.attr('open', '');
    }
});
