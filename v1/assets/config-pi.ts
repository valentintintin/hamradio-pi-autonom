import { ConfigInterface } from '../src/config/config.interface';

export const config: ConfigInterface = {
    callsign: 'F4HVV',
    lat: 45.196250,
    lng: 5.727160,
    databasePath: '/home/valentin/logic/data.db',
    debug: false,
    fakeGpio: false,
    audioDevice: 'pulse',
    dashboard: {
        enable: true,
        apikey: 'pixel',
        mail: 'valentin.s.10@gmail.com',
        port: 80
    },
    sensors: {
        enable: true,
        interval: 30
    },
    webcam: {
        enable: true,
        photosPath: '/home/valentin/logic/photos',
        interval: 30,
        fake: false
    },
    packetRadio: {
        dtmfCode: '25'
    },
    aprs: {
        enable: false,
        lat: 45.174651,
        lng: 5.677257,
        altitude: 440,
        callSrc: 'F4HVV-1',
        callDest: 'APFD38',
        path: 'WIDE1-1,WIDE2-1',
        symbolTable: '/',
        symbolCode: 'I',
        comment: 'Hamnet 44.151.38.221 - NPR 437.000 Mhz',
        interval: 900,
        waitDtmfInterval: 60
    },
    sstv: {
        enable: true,
        comment: 'F4HVV - JN25UE',
        mode: 'r36',
        dtmfCode: '421',
        pisstvPath: '/home/valentin/softs/PiSSTVpp/pisstvpp'
    },
    voice: {
        enable: true,
        sentence: 'Station autonome de Foxtrote 4 Hotel Victor Victor en JN25UE. Voltage de la batterie batteryVoltage volts, courant de charge chargeCurrent milliampères',
        language: 'fr-FR'
    },
    repeater: {
        enable: true,
        seconds: 10
    },
    mpptChd: {
        enable: true,
        fake: false,
        debugI2C: false,
        powerOffVolt: 11350,
        powerOnVolt: 12000,
        pauseBeforePowerOffReachedVolt: 11700,
        watchdog: true,
        batteryLowAlert: true,
        nightAlert: true,
        nightLimitVolt: 11520,
    },
    rsync: {
        enable: true,
        host: '44.151.38.183',
        username: 'f4hvv',
        interval: 600,
        remotePath: '/home/f4hvv/pixelOPi/',
        privateKeyPath: '/home/valentin/.ssh/id_rsa'
    }
};
