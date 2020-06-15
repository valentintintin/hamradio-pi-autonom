$(function () {

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

    $('#see-config').click(function () {
        getConfig();
    });

    $('#switch-off-radio').click(function () {
        switchRadio(true);
    });

    $('#switch-on-radio').click(function () {
        switchRadio(false);
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

    $('#enable-halt').click(function () {
        doNotShutdown(false);
    });

    $('#disable-halt').click(function () {
        doNotShutdown(true);
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
        $.get('/api/logs?apikey=' + apikeyInput.val())
            .done(showData)
            .fail(showData);
    }

    function getSensors() {
        $.get('/api/sensors?apikey=' + apikeyInput.val())
            .done(showData)
            .fail(showData);
    }

    function getConfig() {
        $.get('/api/config?apikey=' + apikeyInput.val())
            .done(showData)
            .fail(showData);
    }

    function switchRadio(state) {
        $.post('/api/gpio/0/' + (state ? '1' : '0') + '?apikey=' + apikeyInput.val())
            .done(showData)
            .fail(showData);
    }

    function takePhoto() {
        $.post('/api/webcam?apikey=' + apikeyInput.val())
            .done(showData)
            .fail(showData);
    }

    function sendSstv() {
        $.post('/api/sstv?apikey=' + apikeyInput.val())
            .done(showData)
            .fail(showData);
    }

    function sendVoice() {
        $.post('/api/voice?apikey=' + apikeyInput.val())
            .done(showData)
            .fail(showData);
    }

    function sendAprsBeacon() {
        $.post('/api/aprs/beacon?apikey=' + apikeyInput.val())
            .done(showData)
            .fail(showData);
    }

    function sendAprsTelem() {
        $.post('/api/aprs/telemetry?apikey=' + apikeyInput.val())
            .done(showData)
            .fail(showData);
    }

    function runRepeater() {
        $.post('/api/repeater?apikey=' + apikeyInput.val())
            .done(showData)
            .fail(showData);
    }

    function doNotShutdown(state) {
        $.post('/api/do-not-shutdown/' + (state ? '1' : '0') + '?apikey=' + apikeyInput.val())
            .done(showData)
            .fail(showData);
    }

    function watchdog(state) {
        $.post('/api/watchdog/' + (state ? 'start' : 'stop') + '?apikey=' + apikeyInput.val())
            .done(showData)
            .fail(showData);
    }

    function shutdownPi(timestamp) {
        if (!timestamp) {
            return;
        }
        $.post('/api/shutdown/' + timestamp + '?apikey=' + apikeyInput.val())
            .done(showData)
            .fail(showData);
    }

    function showData(data) {
        console.log(data);
        dataBrut.html(JSON.stringify(data, null, 4));
        dataJsonWrapper.attr('open', '');
    }
});
