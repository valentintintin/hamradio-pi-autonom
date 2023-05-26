import { ConfigInterface } from '../src/config/config.interface';

export const config: ConfigInterface = {
    callsign: 'F4HVV',
    lat: 45.196250,
    lng: 5.727160,
    databasePath: '/home/valentin/logic/data.db',
    debug: true,
    fakeGpio: true,
    audioDevice: 'pulse',
    dashboard: {
        enable: true,
        port: 3000,
        apikey: 'test'
    },
    sensors: {
        enable: true,
        csvPath: '/home/valentin/logic/sensors.csv',
        interval: 30
    },
    webcam: {
        enable: true,
        photosPath: '/home/valentin/logic/timelapses',
        interval: 30,
        fake: true
    },
    packetRadio: {
        dtmfCode: 'A25'
    },
    aprs: {
        enable: true,
        lat: 45.174651,
        lng: 5.677257,
        altitude: 440,
        callSrc: 'F4HVV-1',
        callDest: 'APFD38',
        path: 'WIDE1-1,WIDE2-1',
        symbolTable: '/',
        symbolCode: 'I',
        comment: 'Hamnet 44.151.38.221',
        interval: 900,
        waitDtmfInterval: 60
    },
    sstv: {
        enable: true,
        comment: 'F4HVV - JN25UE',
        mode: 'r36',
        dtmfCode: '421',
        pisstvPath: '/home/valentin/Softs/pisstv'
    },
    voice: {
        enable: true,
        sentence: 'Station autonome de Foxtrote 4, Hotel Victor Victor',
        language: 'fr-FR'
    },
    repeater: {
        enable: true,
        seconds: 10
    },
    mpptChd: {
        enable: true,
        fake: true,
        debugI2C: false,
        powerOffVolt: 11400,
        watchdog: true,
        batteryLowAlert: true,
        nightAlert: true,
        nightLimitVolt: 11600
    },
    rsync: {
        enable: true,
        host: '127.0.0.1',
        username: 'valentin',
        interval: 60,
        remotePath: '/',
        privateKeyPath: '/home/valentin/.ssh/id_rsa'
    }
};
