import { ConfigInterface } from '../src/config/config.interface';

export const config: ConfigInterface = {
    lat: 45.196250,
    lng: 5.727160,
    logsPath: '/home/valentin/logic/logs',
    debug: true,
    fakeGpio: true,
    audioDevice: 'pulse',
    api: {
        enable: true
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
        sentence: 'Station autonome de Foxtrote 4, Hotel Victor Victor'
    },
    mpptChd: {
        enable: true,
        fake: true,
        debugI2C: false,
        powerOffVolt: 11400,
        watchdog: true,
        shutdownAlert: true,
        shutdownNight: true,
        nightLimitVolt: 11600
    },
    sftp: {
        enable: true,
        host: 'valentin.ddns.info',
        username: 'valentin',
        interval: 60,
        remotePath: '/',
        privateKeyPath: '/home/valentin/.ssh/id_rsa',
        privateKeyPassphrase: '******'
    }
};
